#pragma once

#include <google/protobuf/util/json_util.h>

#include <cmath>
#include <vector>
#include <algorithm>
#include "midi.pb.h"
#include "../enum/density.h"
#include "../enum/density_opz.h"
#include "../enum/constants.h"
#include "../enum/track_type.h"
#include "../enum/gm.h"
#include "../enum/encoder_config.h"
#include "../random.h"

#define M_LOG2E 1.4426950408889634074

// START OF NAMESPACE
namespace mmm {

//utils 

template <typename T>
void string_to_protobuf(std::string &s, T *x) {
	google::protobuf::util::JsonStringToMessage(s, x);
}

template <typename T>
std::string protobuf_to_string(T *x) {
	std::string output;
	google::protobuf::util::JsonPrintOptions opt;
	opt.add_whitespace = true;
	google::protobuf::util::MessageToJsonString(*x, &output, opt);
	return output;
}

template <typename T>
void print_protobuf(T *x) {
	std::cout << protobuf_to_string(x) << std::endl;
}

// function to make sure track has features
midi::TrackFeatures *get_track_features(midi::Piece *p, int track_num) {
	if ((track_num < 0) || (track_num >= p->tracks_size())) {
		throw std::runtime_error("TRACK FEATURE REQUEST OUT OF RANGE");
	}
	midi::Track *t = p->mutable_tracks(track_num);
	midi::TrackFeatures *tf = NULL;
	if (t->internal_features_size() == 0) {
		return t->add_internal_features();
	}
	return t->mutable_internal_features(0);
}

struct RNG {
	int operator() (int n) {
		return std::rand() / (1.0 + RAND_MAX) * n;
	}
};

// select tracks for te rep

int get_num_bars(midi::Piece *x) {
	if (x->tracks_size() == 0) {
		return 0;
	}
	std::set<int> lengths;
	for (const auto track : x->tracks()) {
		lengths.insert( track.bars_size() );
	}
	if (lengths.size() > 1) {
		throw std::runtime_error("Each track must have the same number of bars!");
	}
	return *lengths.begin();
}



// =======================================
// adding note durations to events
void calculate_note_durations(midi::Piece *p) {
	// to start set all durations == 0
	for (int i=0; i<p->events_size(); i++) {
		p->mutable_events(i)->set_internal_duration(0);
	}

	for (const auto track : p->tracks()) {
		// pitches to (abs_time, event_index)
		std::map<int,std::tuple<int,int>> onsets; 
		int bar_start = 0;
		for (const auto bar : track.bars()) {
			for (auto event_id : bar.events()) {
				midi::Event e = p->events(event_id);
				//std::cout << "PROC EVENT :: " << e.pitch() << " " << e.velocity() << " " << e.time() << std::endl;
				if (e.velocity() > 0) {
					if (is_drum_track(track.track_type())) {
						// drums always have duration of 1 timestep
						p->mutable_events(event_id)->set_internal_duration(1);
					}
					else {
						onsets[e.pitch()] = std::make_tuple(bar_start + e.time(), event_id);
					}
				}
				else {
					auto it = onsets.find(e.pitch());
					if (it != onsets.end()) {
						int index = std::get<1>(it->second);
						int duration = (bar_start + e.time()) - std::get<0>(it->second);
						p->mutable_events(index)->set_internal_duration(duration);
						//std::cout << "SET DURATION :: " << bar_start << " " << duration << std::endl;
					}
				}
			}
			// move forward a bar
			bar_start += p->resolution() * bar.internal_beat_length(); 
		}
	}
}


// ================================================================
// basic function for getting notes

std::vector<midi::Note> events_to_notes(midi::Piece *p, std::vector<int> track_nums, std::vector<int> bar_nums, int *max_tick=NULL, bool include_onsetless=false) {
	midi::Event e;
	std::vector<midi::Note> notes;
	for (const auto track_num : track_nums) {
		std::map<int,int> onsets;
		int bar_start = 0;
		for (const auto bar_num : bar_nums) {
			const midi::Bar bar = p->tracks(track_num).bars(bar_num);
			for (auto event_id : bar.events()) {
				e = p->events(event_id);
				if (e.velocity() > 0) {
					if (is_drum_track(p->tracks(track_num).track_type())) {
						midi::Note note;
						note.set_start( bar_start + e.time() );
						note.set_end( bar_start + e.time() + 1 );
						note.set_pitch( e.pitch() );
						notes.push_back(note);
					}
					else {
						onsets[e.pitch()] = bar_start + e.time();
					}
				}
				else {
					auto it = onsets.find(e.pitch());
					if (it != onsets.end()) {
						midi::Note note;
						note.set_start( it->second );
						note.set_end( bar_start + e.time() );
						note.set_pitch( it->first );
						notes.push_back(note);
						
						onsets.erase(it); // remove note
					}
					else if (include_onsetless) {
						midi::Note note;
						note.set_start( bar_start );
						note.set_end( bar_start + e.time() );
						note.set_pitch( e.pitch() );
						notes.push_back(note);
					}
				}
				if (max_tick) {
					*max_tick = std::max(*max_tick, bar_start + e.time());
				}
			}
			// move forward a bar
			bar_start += p->resolution() * bar.internal_beat_length(); 
		}
	}
	return notes;
}

std::vector<midi::Note> track_events_to_notes(midi::Piece *p, int track_num, int *max_tick=NULL) {
	int num_bars = p->tracks(track_num).bars_size();
	return events_to_notes(p, {track_num}, arange(num_bars), max_tick);
}

/*
std::vector<midi::Note> track_events_to_notes(midi::Piece *p, int track_num, int *max_tick=NULL, bool no_drum_offsets=false) {
	midi::Event e;
	std::map<int,int> onsets;
	std::vector<midi::Note> notes;
	int bar_start = 0;
	for (auto bar : p->tracks(track_num).bars()) {
		for (auto event_id : bar.events()) {
			e = p->events(event_id);
			if (e.velocity() > 0) {
				if (is_drum_track(p->tracks(track_num).track_type()) && no_drum_offsets) {
					midi::Note note;
					note.set_start( bar_start + e.time() );
					note.set_end( bar_start + e.time() + 1 );
					note.set_pitch( e.pitch() );
					notes.push_back(note);
				}
				else {
					onsets[e.pitch()] = bar_start + e.time();
				}
			}
			else {
				auto it = onsets.find(e.pitch());
				if (it != onsets.end()) {
					midi::Note note;
					note.set_start( it->second );
					note.set_end( bar_start + e.time() );
					note.set_pitch( it->first );
					notes.push_back(note);
					
					onsets.erase(it); // remove note
				}
			}

			if (max_tick) {
				*max_tick = std::max(*max_tick, bar_start + e.time());
			}
		}
		// move forward a bar
		bar_start += p->resolution() * bar.internal_beat_length(); 
	}
	return notes;
}
*/

// ========================================================================
// PAD PIECE TO MATCH STATUS

void pad_piece_with_status(midi::Piece *p, midi::Status *s, int min_bars) {
	// add tracks when status references ones that do not exist
	for (const auto track : s->tracks()) {
		midi::Track *t = NULL;
		if (track.track_id() >= p->tracks_size()) {
			t = p->add_tracks();
			t->set_track_type( track.track_type() );
			midi::TrackFeatures *f = t->add_internal_features();
			//f->set_note_density_v2( 4 );
		}
		else {
			t = p->mutable_tracks(track.track_id());
		}
		int num_bars = std::max(track.selected_bars_size(), min_bars);
		for (int i=t->bars_size(); i<num_bars; i++) {
			midi::Bar *b = t->add_bars();
			b->set_internal_beat_length( 4 );
		}
	}
}

// ========================================================================
// distribution of onset times / relative to bar

std::vector<int> piece_to_onset_distribution(midi::Piece *p, int size) {
	std::vector<int> d(size,0);
	for (const auto e : p->events()) {
		if (e.velocity()) {
			d[e.time() % size] += 1;
		}
	}
	return d;
}

// ========================================================================
// RANDOMLY PERTURB PERCENTAGE OF NOTES

// get a list of events that are linked (ie. onsets and offsets)
std::map<int,int> link_events(midi::Piece *p, std::vector<int> track_nums, std::vector<int> bar_nums) {
	midi::Event e;
	std::map<int,int> pairs;
	for (const auto track_num : track_nums) {
		std::map<int,int> onsets;
		int bar_start = 0;
		for (const auto bar_num : bar_nums) {
			const midi::Bar bar = p->tracks(track_num).bars(bar_num);
			for (auto event_id : bar.events()) {
				e = p->events(event_id);
				if (e.velocity() > 0) {
					onsets[e.pitch()] = event_id;
				}
				else {
					auto it = onsets.find(e.pitch());
					if (it != onsets.end()) {
						pairs[it->second] = event_id;
						onsets.erase(it); // remove note
					}
				}
			}
		}
	}
	return pairs;
}

std::map<int,std::tuple<int,int>> link_events_to_track_bar(midi::Piece *p) {
	std::map<int,std::tuple<int,int>> events_to_track_bar;
	int track_num = 0;
	for (const auto track : p->tracks()) {
		int bar_num = 0;
		for (const auto bar : track.bars()) {
			for (const auto event_id : bar.events()) {
				events_to_track_bar[event_id] = std::make_tuple(track_num,bar_num);
			}
			bar_num++;
		}
		track_num++;
	}
	return events_to_track_bar;
}

void random_perturb(midi::Piece *p, midi::Status *s, double per, int seed) {
	std::mt19937 engine(seed);
	int notes_perturbed = 0;
	int eligible_notes = 0;
	auto pairs = link_events(p,arange(p->tracks_size()),arange(get_num_bars(p)));
	auto events_to_track_bar = link_events_to_track_bar(p);

	std::vector<std::tuple<int,int>> notes;
	int track_num = 0;
	for (const auto track : s->tracks()) {
		int bar_num = 0;
		for (const auto bar : track.selected_bars()) {
			std::tuple<int,int> cur = std::make_tuple(track_num, bar_num);
			if (bar) {
				// go through all the notes in this bar
				// check that it is linked to offset within the bar
				// then randomly perturb note
				for (const auto eid : p->tracks(track_num).bars(bar_num).events()) {
					auto oid = pairs.find(eid);
					if ((oid!=pairs.end()) && (events_to_track_bar[oid->second]==cur)) {
						notes.push_back( std::make_tuple(eid, oid->second) );
					}
				}
			}
			bar_num++;
		}
		track_num++;
	}

	shuffle(notes.begin(), notes.end(), engine);
	int total = std::max((int)(notes.size() * per), 1);
	if (per >= 1) {
		total = (int)round(per);
	}
	if (total > notes.size()) {
		throw std::runtime_error("NOT ENOUGH NOTES TO PERTURB");
	}
	for (int i=0; i<total; i++) {
		int pitch = random_on_range(128, &engine);
		p->mutable_events(std::get<0>(notes[i]))->set_pitch(pitch);
		p->mutable_events(std::get<1>(notes[i]))->set_pitch(pitch);
	}
}

// ========================================================================
// APPEND ONE PIECE TO ANOTHER

void append_piece(midi::Piece *p, midi::Piece *q) {
	// we have the following assumptions
	// 1. we will only add the same number of tracks in p
	// 2. it will fail if the beatLength is different between bars
	int p_num_bars = get_num_bars(p);
	int q_num_bars = get_num_bars(q);

	// first we check that we have the right bar setup
	if ((p->tracks_size() > 0) && (q->tracks_size() > 0)) {
		for (int b=0; b<p_num_bars; b++) {
			if (b < q_num_bars) {
				if (p->tracks(0).bars(b).internal_beat_length() != q->tracks(0).bars(b).internal_beat_length()) {
					throw std::runtime_error("BAR LENGTHS NOT EQUAL!");
				}
			}
		}
	}

	// add each of the tracks
	for (const auto track : q->tracks()) {
		midi::Track *t = p->add_tracks();
		t->CopyFrom(track);
		t->clear_bars(); // remove bars
		int bar_num = 0;
		for (const auto bar : track.bars()) {
			if (bar_num < p_num_bars) {
				midi::Bar * b = t->add_bars();
				b->CopyFrom(bar);
				b->clear_events();
				for (const auto index : bar.events()) {
					b->add_events(p->events_size());
					midi::Event *e = p->add_events();
					e->CopyFrom(q->events(index));
				}
			}      
			bar_num++;
		}
		
		// if track is shorter add blank bars
		for (int i=t->bars_size(); i<p->tracks(0).bars_size(); i++) {
			midi::Bar *b = t->add_bars();
			b->CopyFrom(p->tracks(0).bars(i));
			b->clear_events();
		}
	}

}

// ========================================================================
// MAX POLYPHONY

bool notes_overlap(midi::Note *a, midi::Note *b) {
	return (a->start() >= b->start()) && (a->start() < b->end());
}

int max_polyphony(std::vector<midi::Note> &notes, int max_tick) {
	if (max_tick > 100000) {
		throw std::runtime_error("MAX TICK TO LARGE!");
	}
	int max_polyphony = 0;
	std::vector<int> flat_roll(max_tick,0);
	for (const auto note : notes) {
		for (int t=note.start(); t<note.end(); t++) {
			flat_roll[t]++;
			max_polyphony = std::max(flat_roll[t],max_polyphony);
		}
	}
	return max_polyphony;
}

void update_max_polyphony(midi::Piece *p) {
	for (int i=0; i<p->tracks_size(); i++) {
		int max_tick = 0;
		std::vector<midi::Note> notes = track_events_to_notes(p, i, &max_tick);
		get_track_features(p,i)->set_max_polyphony(max_polyphony(notes, max_tick));
	}
}

// ========================================================================
// NOTE DURATION / AV POLYPHONY

template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
	return std::max(lower, std::min(n, upper));
}

// function to get quantile
template<typename T>
std::vector<T> quantile(std::vector<T> &x, std::vector<double> qs) {
	std::vector<T> vals;
	for (const auto q : qs) {
		if (x.size()) {
			int index = std::min((int)round((double)x.size() * q), (int)x.size() - 1);
			std::nth_element(x.begin(), x.begin() + index, x.end());
			vals.push_back( x[index] );
		}
		else {
			vals.push_back( 0 );
		}
	}
	return vals;
}

template<typename T>
T min_value(std::vector<T> &x) {
	auto result = std::min_element(x.begin(), x.end());
	if (result == x.end()) {
		return 0;
	}
	return *result;
}

template<typename T>
T max_value(std::vector<T> &x) {
	auto result = std::max_element(x.begin(), x.end());
	if (result == x.end()) {
		return 0;
	}
	return *result;
}

inline double mmm_log2(const long double x){
	return std::log(x) * M_LOG2E;
}

std::vector<int> get_note_durations(std::vector<midi::Note> &notes) {
	std::vector<int> durations;
	for (const auto note : notes) {
		double d = note.end() - note.start();
		durations.push_back( (int)clip(mmm_log2(std::max(d/3.,1e-6)) + 1, 0., 5.) );
	}
	return durations;
}

double note_duration_inner(std::vector<midi::Note> &notes) {
	double total_diff = 0;
	for (const auto note : notes) {
		total_diff += (note.end() - note.start());
	}
	return total_diff / std::max((int)notes.size(),1);
}

void update_note_duration(midi::Piece *p) {
	for (int track_num=0; track_num<p->tracks_size(); track_num++) {
		std::vector<midi::Note> notes = track_events_to_notes(p, track_num);
		get_track_features(p,track_num)->set_note_duration(note_duration_inner(notes));
	}
}

std::tuple<double,double,double,double,double,double> av_polyphony_inner(std::vector<midi::Note> &notes, int max_tick, midi::TrackFeatures *f) {
	if (max_tick > 100000) {
		throw std::runtime_error("MAX TICK TO LARGE!");
	}
	int nonzero_count = 0;
	double count = 0;
	std::vector<int> flat_roll(max_tick,0);
	for (const auto note : notes) {
		for (int t=note.start(); t<std::min(note.end(),max_tick-1); t++) {
			if (flat_roll[t]==0) {
				nonzero_count += 1;
			}
			flat_roll[t]++;
			count++;
		}
	}

	std::vector<int> nz;
	for (const auto x : flat_roll) {
		if (x > 0) {
			nz.push_back(x);
			if (f) {
				f->add_polyphony_distribution(x);
			}
		}
	}

	double silence = max_tick - nonzero_count;

	std::vector<int> poly_qs = quantile<int>(nz, {.15,.85});

	double min_polyphony = min_value(nz);
	double max_polyphony = max_value(nz);

	double av_polyphony = count / std::max(nonzero_count,1);
	double av_silence = silence / std::max(max_tick,1);
	return std::make_tuple(av_polyphony, av_silence, poly_qs[0], poly_qs[1], min_polyphony, max_polyphony);
}

void update_av_polyphony(midi::Piece *p) {
	for (int track_num=0; track_num<p->tracks_size(); track_num++) {
		int max_tick = 0;
		std::vector<midi::Note> notes = track_events_to_notes(p, track_num, &max_tick);
		get_track_features(p,track_num)->set_av_polyphony(
			std::get<0>(av_polyphony_inner(notes,max_tick,NULL)));
	}
}

void update_av_polyphony_and_note_duration(midi::Piece *p) {
	for (int track_num=0; track_num<p->tracks_size(); track_num++) {
		int max_tick = 0;
		std::vector<midi::Note> notes = track_events_to_notes(
			p, track_num, &max_tick);
		std::vector<int> durations = get_note_durations(notes);
		midi::TrackFeatures *f = get_track_features(p,track_num);
		auto stat = av_polyphony_inner(notes,max_tick,f);
		f->set_note_duration(note_duration_inner(notes));
		f->set_av_polyphony( std::get<0>(stat) );
		f->set_min_polyphony_q(
			std::max(std::min((int)std::get<2>(stat),10),1)-1 );
		f->set_max_polyphony_q(
			std::max(std::min((int)std::get<3>(stat),10),1)-1 );
		
		std::vector<int> dur_qs = quantile(durations, {.15,.85});
		f->set_min_note_duration_q( dur_qs[0] );
		f->set_max_note_duration_q( dur_qs[1] );

		// new hard upper lower limits
		f->set_min_polyphony_hard( std::get<4>(stat) );
		f->set_max_polyphony_hard( std::get<5>(stat) );
		f->set_rest_percentage( std::get<1>(stat) );

		f->set_min_note_duration_hard( min_value(durations) );
		f->set_max_note_duration_hard( max_value(durations) );
		
	}
}

void update_density_trifeature_bar(midi::Piece *p, int track_num, int bar_num) {
	std::vector<midi::Note> notes = events_to_notes(
		p, {track_num}, {bar_num}, NULL, true);
	
	midi::Bar *b = p->mutable_tracks(track_num)->mutable_bars(bar_num);
	midi::ContinuousFeature *f = b->add_internal_feature();
	double note_dur = note_duration_inner(notes);
	auto stat = av_polyphony_inner(
		notes, p->resolution() * b->internal_beat_length(), NULL);
	f->set_av_polyphony( std::get<0>(stat) );
	f->set_av_silence( std::get<1>(stat) );
	f->set_note_duration( note_dur );
	f->set_note_duration_norm( note_dur / (p->resolution() * b->internal_beat_length()) );
}

void update_density_trifeature(midi::Piece *p) {
	for (int track_num=0; track_num<p->tracks_size(); track_num++) {
		for (int bar_num=0; bar_num<p->tracks(track_num).bars_size(); bar_num++) {
			update_density_trifeature_bar(p, track_num, bar_num);
		}
	}
}

// ========================================================================
// PITCH CLASS / PITCH RANGE

void update_pitch_range(midi::Piece *p) {
	int track_num = 0;
	for (const auto track : p->tracks()) {
		int min_pitch = INT_MAX;
		int max_pitch = 0;
		midi::TrackFeatures *f = get_track_features(p,track_num);
		for (const auto bar : track.bars()) {
			for (const auto event : bar.events()) {
				const midi::Event e = p->events(event);
				if (e.velocity() > 0) {
					min_pitch = std::min(min_pitch, e.pitch());
					max_pitch = std::max(max_pitch, e.pitch());
				}				
			}
		}
		f->set_min_pitch(min_pitch);
		f->set_max_pitch(max_pitch);
		track_num++;
	}	
}

void update_pitch_class(midi::Piece *p) {
	int track_num = 0;
	for (const auto track : p->tracks()) {
		std::vector<bool> pc(12,false);
		midi::TrackFeatures *f = get_track_features(p,track_num);
		for (const auto bar : track.bars()) {
			for (const auto event : bar.events()) {
				const midi::Event e = p->events(event);
				if (e.velocity() > 0) {
					pc[ e.pitch() % 12 ] = true;
				}
			}
		}
		f->clear_pitch_classes(); // make sure empty
		for (int i=0; i<12; i++) {
			f->add_pitch_classes(pc[i]);
		}
		track_num++;
	}	
}

// ========================================================================
// FORCE MONOPHONIC

std::vector<midi::Event> force_monophonic(std::vector<midi::Event> &events) {
	bool note_sounding = false;
	midi::Event last_onset;
	std::vector<midi::Event> mono_events;

	for (const auto event : events) {
		if (event.velocity() > 0) {
			if ((note_sounding) && (event.time() > last_onset.time())) {
				// add note if it has nonzero length
				mono_events.push_back( last_onset );
				midi::Event offset;
				offset.CopyFrom(last_onset);
				offset.set_time( event.time() );
				offset.set_velocity( 0 );
				mono_events.push_back( offset );
				note_sounding = false;
			}

			// update last onset
			last_onset.CopyFrom( event );
			note_sounding = true;
		}
		else if ((note_sounding) && (last_onset.pitch() == event.pitch())) {
			// if we reach offset before next onset note remains unchanged
			mono_events.push_back( last_onset );
			mono_events.push_back( event );
			note_sounding = false;
		}
	}

	return mono_events;
}

// ========================================================================
// NOTE DENSITY

std::vector<std::tuple<int,int,int>> calculate_note_density(midi::Piece *x) {
	std::vector<std::tuple<int,int,int>> density;
	int num_notes;
	for (const auto track : x->tracks()) {
		for (const auto bar : track.bars()) {
			num_notes = 0;
			for (const auto event_index : bar.events()) {
				if (x->events(event_index).velocity()) {
					num_notes++;
				}
			}
			density.push_back(
				std::make_tuple(track.track_type(), track.instrument(), num_notes));
		}
	}
	return density;
}

// function to get note density value
int get_note_density_target(midi::Track *track, int bin) {
	int qindex = track->instrument();
	int tt = track->track_type();
	if (is_opz_track(tt)) {
		std::tuple<int,int> key = std::make_tuple(track->track_type(),qindex);
		if (OPZ_DENSITY_QUANTILES.find(key) != OPZ_DENSITY_QUANTILES.end()) {
			return OPZ_DENSITY_QUANTILES[key][bin];
		}
		return 0;
	}
	if (is_drum_track(tt)) {
		qindex = 128;
	}
	return DENSITY_QUANTILES[qindex][bin];
}

void update_note_density(midi::Piece *x) {

	int track_num = 0;
	int num_notes, bar_num;
	for (const auto track : x->tracks()) {

		// calculate average notes per bar
		num_notes = 0;
		int bar_num = 0;
		std::set<int> valid_bars;
		for (const auto bar : track.bars()) {
			for (const auto event_index : bar.events()) {
				if (x->events(event_index).velocity()) {
					valid_bars.insert(bar_num);
					num_notes++;
				}
			}
			bar_num++;
		}
		int num_bars = std::max((int)valid_bars.size(),1);
		double av_notes_fp = (double)num_notes / num_bars;
		int av_notes = round(av_notes_fp);

		// calculate the density bin
		int qindex = track.instrument();
		int bin = 0;

		if (is_opz_track(track.track_type())) {
			std::tuple<int,int> key = std::make_tuple(track.track_type(),qindex);
			if (OPZ_DENSITY_QUANTILES.find(key) != OPZ_DENSITY_QUANTILES.end()) {
				while (av_notes > OPZ_DENSITY_QUANTILES[key][bin]) { 
					bin++;
				}

			}
		}
		else {
			if (is_drum_track(track.track_type())) {
				qindex = 128;
			}
			while (av_notes > DENSITY_QUANTILES[qindex][bin]) { 
				bin++;
			}
		}

		// update protobuf
		midi::TrackFeatures *tf = get_track_features(x,track_num);
		tf->set_note_density_v2(bin);
		tf->set_note_density_value(av_notes_fp);
		track_num++;


	}
}

// ========================================================================
// EMPTY BARS

void update_has_notes(midi::Piece *x) {
	//cout << "entering update_has_notes" << std::endl;
	int track_num = 0;
	for (const auto track : x->tracks()) {
		int bar_num = 0;
		for (const auto bar : track.bars()) {
			bool has_notes = false;
			for (const auto event_index : bar.events()) {
				if (x->events(event_index).velocity()>0) {
					has_notes = true;
					break;
				}
			}
			x->mutable_tracks(track_num)->mutable_bars(bar_num)->set_internal_has_notes(has_notes);
			bar_num++;
		}
		track_num++;
	}
}

void reorder_tracks(midi::Piece *x, std::vector<int> track_order) {
	int num_tracks = x->tracks_size();
	if (num_tracks != track_order.size()) {
		std::cout << num_tracks << " " << track_order.size() << std::endl;
		throw std::runtime_error("Track order does not match midi::Piece.");
	}
	/*
	std::cout << "REORDER :: " << std::endl;
	for (const auto o : track_order) {
		std::cout << o << std::endl;
	}
	std::cout << "END REORDER :: " << std::endl;
	*/
	for (int track_num=0; track_num<num_tracks; track_num++) {
		get_track_features(x,track_num)->set_order(track_order[track_num]);
	}
	std::sort(
		x->mutable_tracks()->begin(), 
		x->mutable_tracks()->end(), 
		[](const midi::Track &a, const midi::Track &b){ 
			return a.internal_features(0).order() < b.internal_features(0).order();
		}
	);
}

void prune_tracks_dev2(midi::Piece *x, std::vector<int> tracks, std::vector<int> bars) {

	if (x->tracks_size() == 0) {
		return;
	}

	midi::Piece tmp(*x);

	int num_bars = get_num_bars(x);
	bool remove_bars = bars.size() > 0;
	x->clear_tracks();
	x->clear_events();

	std::vector<int> tracks_to_keep;
	for (const auto track_num : tracks) {
		if ((track_num >= 0) && (track_num < tmp.tracks_size())) {
			tracks_to_keep.push_back(track_num);
		}
	}

	std::vector<int> bars_to_keep;
	for (const auto bar_num : bars) {
		if ((bar_num >= 0) && (bar_num < num_bars)) {
			bars_to_keep.push_back(bar_num);
		}
	}

	for (const auto track_num : tracks_to_keep) {
		const midi::Track track = tmp.tracks(track_num);
		midi::Track *t = x->add_tracks();
		t->CopyFrom( track );
		if (remove_bars) {
			t->clear_bars();
			for (const auto bar_num : bars_to_keep) {
				const midi::Bar bar = track.bars(bar_num);
				midi::Bar *b  = t->add_bars();
				b->CopyFrom( bar );
				b->clear_events();
				for (const auto event_index : bar.events()) {
					b->add_events( x->events_size() );
					midi::Event *e = x->add_events();
					e->CopyFrom( tmp.events(event_index) );
				}
			}
		}
	}

	//if (x->events_size() == 0) {
	//  throw std::runtime_error("NO EVENTS COPIED");
	//}
}

bool track_has_notes(midi::Piece *x, const midi::Track &track, std::vector<int> &bars_to_keep) {
	for (const auto bar_num : bars_to_keep) {
		for (const auto event_index : track.bars(bar_num).events()) {
			if (x->events(event_index).velocity()>0) {
				return true;
			}
		}
	}
	return false;
}

void prune_empty_tracks(midi::Piece *x, std::vector<int> &bars_to_keep) {
	std::vector<int> tracks_to_keep;
	int track_num = 0;
	for (const auto track : x->tracks()) {
		if (track_has_notes(x,track,bars_to_keep)) {
			tracks_to_keep.push_back(track_num);
		}
		track_num++;
	}
	prune_tracks_dev2(x, tracks_to_keep, bars_to_keep);
}

void shuffle_tracks_dev(midi::Piece *x, std::mt19937 *engine) {
	std::vector<int> tracks = arange(0,x->tracks_size(),1);
	shuffle(tracks.begin(), tracks.end(), *engine);
	prune_tracks_dev2(x, tracks, {});
}

void prune_events(midi::Piece *x, std::set<int> &events_to_prune) {
	midi::Piece tmp(*x);
	x->clear_events();

	int track_num = 0;
	for (const auto track : tmp.tracks()) {
		int bar_num = 0;
		for (const auto bar : track.bars()) {
			// clear this bars events in tmp and repopulate
			midi::Bar *b = x->mutable_tracks(track_num)->mutable_bars(bar_num);
			b->clear_events();

			for (const auto index : bar.events()) {
				if (events_to_prune.find(index) == events_to_prune.end()) {
					// add this event
					b->add_events( x->events_size() );
					midi::Event *e = x->add_events();
					e->CopyFrom( tmp.events(index) );
				}
			}
			bar_num++;
		}
		track_num++;
	}

}

// removing held notes that have no offset
void prune_notes_wo_offset(midi::Piece *x, bool ignore_drums) {
	std::set<int> bad_onsets;
	for (const auto track : x->tracks()) {
		std::map<int,int> pitch_to_index;
		//cout << ((!ignore_drums) || (!track.is_drum())) << std::endl;
		if ((!ignore_drums) || (!is_drum_track(track.track_type()))) {
			for (const auto bar : track.bars()) {
				for (const auto index : bar.events()) {
					int pitch = x->events(index).pitch();
					int velocity = x->events(index).velocity();
					if (velocity > 0) {
						if (pitch_to_index.find(pitch) != pitch_to_index.end()) {
							bad_onsets.insert( pitch_to_index[pitch] );
						}
						pitch_to_index[pitch] = index;
					}
					else {
						pitch_to_index.erase(pitch);
					}
				}
			}
		}
		for (const auto kv : pitch_to_index) {
			bad_onsets.insert( kv.second );
		}
	}
	// do the actual pruning inplace
	prune_events(x, bad_onsets);
}




// ========================================================================
// RANDOM SEGMENT SELECTION FOR TRAINING
// 
// 1. we select an index of a random segment


void update_valid_segments(midi::Piece *x, int seglen, int min_tracks, bool opz) {
	update_has_notes(x);
	x->clear_internal_valid_segments();
	x->clear_internal_valid_tracks();

	if (x->tracks_size() < min_tracks) { return; } // no valid tracks

	int min_non_empty_bars = round(seglen * .75);
	int num_bars = get_num_bars(x);
	
	for (int start=0; start<num_bars-seglen+1; start++) {
		
		// check that all time sigs are supported
		bool supported_ts = true;
		bool is_four_four = true;
		// for now we ignore this as its better to just keep all the data and
		// prune at training time, as the encoding will fail.
		/*
		for (int k=0; k<seglen; k++) {
			int beat_length = x->tracks(0).bars(start+k).internal_beat_length();
			supported_ts &= (time_sig_map.find(beat_length) != time_sig_map.end());
			is_four_four &= (beat_length == 4);
		}
		*/

		// check which tracks are valid
		midi::ValidTrack vtracks;
		std::map<int,int> used_track_types;
		for (int track_num=0; track_num<x->tracks_size(); track_num++) {
			int non_empty_bars = 0;
			for (int k=0; k<seglen; k++) {
				if (x->tracks(track_num).bars(start+k).internal_has_notes()) {
					non_empty_bars++;
				}
			}
			if (non_empty_bars >= min_non_empty_bars) {
				vtracks.add_tracks( track_num );
				if (opz) {
					// product of train types should be different
					int combined_train_type = 1;
					for (const auto train_type : x->tracks(track_num).internal_train_types()) {
						combined_train_type *= train_type;
					}
					used_track_types[combined_train_type]++;
				}
			}
		}

		// check if there are enough tracks
		bool enough_tracks = vtracks.tracks_size() >= min_tracks;
		if (opz) {
			// for OPZ we can't count repeated track types
			// as we train on only one track per track type
			bool opz_valid = used_track_types.size() >= min_tracks;
			
			// also valid if we have more than one multi possibility track
			auto it = used_track_types.find(
				midi::OPZ_ARP_TRACK * midi::OPZ_LEAD_TRACK);
			opz_valid |= ((it != used_track_types.end()) && (it->second > 1));

			it = used_track_types.find(
				midi::OPZ_ARP_TRACK * midi::OPZ_LEAD_TRACK * midi::OPZ_CHORD_TRACK);
			opz_valid |= ((it != used_track_types.end()) && (it->second > 1));

			enough_tracks &= opz_valid;
		}

		if (enough_tracks && is_four_four) {
			midi::ValidTrack *v = x->add_internal_valid_tracks_v2();
			v->CopyFrom(vtracks);
			x->add_internal_valid_segments(start);
		}
	}
}

void select_random_segment_indices(midi::Piece *x, int num_bars, int min_tracks, int max_tracks, bool opz, std::mt19937 *engine, std::vector<int> &valid_tracks, int *start) {
	update_valid_segments(x, num_bars, min_tracks, opz);
	
	if (x->internal_valid_segments_size() == 0) {
		throw std::runtime_error("NO VALID SEGMENTS");
	}

	//int index = rand() % x->internal_valid_segments_size();
	int index = random_on_range(x->internal_valid_segments_size(), engine);
	(*start) = x->internal_valid_segments(index);
	for (const auto track_num : x->internal_valid_tracks_v2(index).tracks()) {
		valid_tracks.push_back(track_num);
	}
	shuffle(valid_tracks.begin(), valid_tracks.end(), *engine);
	
	if (opz) {
		// filter out duplicate OPZ tracks
		// convert train_track_types to type
		// randomly pick a track type for each track

		std::vector<int> pruned_tracks;
		std::vector<int> used(midi::NUM_TRACK_TYPES,0);
		for (const auto track_num : valid_tracks) {
			std::vector<midi::TRACK_TYPE> track_options;
			for (const auto track_type : x->tracks(track_num).internal_train_types()) {
				track_options.push_back( (midi::TRACK_TYPE)track_type );
			}
			shuffle(track_options.begin(), track_options.end(), *engine);
			for (const auto track_type : track_options) {
				if ((track_type >= 0) && (track_type < midi::NUM_TRACK_TYPES)) {
					if ((used[track_type] == 0) && (track_type <= midi::OPZ_CHORD_TRACK)) {
						pruned_tracks.push_back( track_num );
						// set the track type to the one randomly selected
						x->mutable_tracks(track_num)->set_track_type( track_type );
						used[track_type] = 1;
						break;
					}
				}
			}
		}
		valid_tracks = pruned_tracks;
		
		// it is possible that we have less than min tracks here
		// throw an exception if this is the case
		if (valid_tracks.size() < min_tracks) {
			throw std::runtime_error("LESS THAN MIN TRACKS");
		}
	}
	else {
		// limit the tracks
		int ntracks = std::min((int)valid_tracks.size(), max_tracks);
		valid_tracks.resize(ntracks);
	}
}

void select_random_segment(midi::Piece *x, int num_bars, int min_tracks, int max_tracks, bool opz, std::mt19937 *engine) {

	/*
	update_valid_segments(x, num_bars, min_tracks, opz);
	
	if (x->internal_valid_segments_size() == 0) {
		throw std::runtime_error("NO VALID SEGMENTS");
	}

	//int index = rand() % x->internal_valid_segments_size();
	int index = random_on_range(x->internal_valid_segments_size(), engine);
	int start = x->internal_valid_segments(index);
	std::vector<int> valid_tracks;
	for (const auto track_num : x->internal_valid_tracks_v2(index).tracks()) {
		valid_tracks.push_back(track_num);
	}
	shuffle(valid_tracks.begin(), valid_tracks.end(), *engine);
	std::vector<int> bars = arange(start,start+num_bars,1);
	
	if (opz) {
		// filter out duplicate OPZ tracks
		// convert train_track_types to type
		// randomly pick a track type for each track

		std::vector<int> pruned_tracks;
		std::vector<int> used(NUM_TRACK_TYPES,0);
		for (const auto track_num : valid_tracks) {
			std::vector<int> track_options;
			for (const auto track_type : x->tracks(track_num).internal_train_types()) {
				track_options.push_back( track_type );
			}
			shuffle(track_options.begin(), track_options.end(), *engine);
			for (const auto track_type : track_options) {
				if ((track_type >= 0) && (track_type < NUM_TRACK_TYPES)) {
					if ((used[track_type] == 0) && (track_type <= midi::OPZ_CHORD_TRACK)) {
						pruned_tracks.push_back( track_num );
						// set the track type to the one randomly selected
						x->mutable_tracks(track_num)->set_track_type( track_type );
						used[track_type] = 1;
						break;
					}
				}
			}
		}
		valid_tracks = pruned_tracks;
		
		// it is possible that we have less than min tracks here
		// throw an exception if this is the case
		if (valid_tracks.size() < min_tracks) {
			throw std::runtime_error("LESS THAN MIN TRACKS");
		}
	}
	else {
		// limit the tracks
		int ntracks = std::min((int)valid_tracks.size(), max_tracks);
		valid_tracks.resize(ntracks);
	}
	*/
	int start;
	std::vector<int> valid_tracks;
	select_random_segment_indices(
		x, num_bars, min_tracks, max_tracks, opz, engine, valid_tracks, &start);
	std::vector<int> bars = arange(start,start+num_bars,1);
	prune_tracks_dev2(x, valid_tracks, bars);
}



// other helpers for training ...

std::tuple<int,int> get_pitch_extents(midi::Piece *x) {
	int min_pitch = INT_MAX;
	int max_pitch = 0;
	for (const auto track : x->tracks()) {
		if (!is_drum_track(track.track_type())) {
			for (const auto bar : track.bars()) {
				for (const auto event_index : bar.events()) {
					int pitch = x->events(event_index).pitch();
					min_pitch = std::min(pitch, min_pitch);
					max_pitch = std::max(pitch, max_pitch);
				}
			}
		}
	}
	return std::make_pair(min_pitch, max_pitch);
}

std::set<std::tuple<int,int>> make_bar_mask(midi::Piece *x, float proportion, std::mt19937 *engine) {
	int num_tracks = x->tracks_size();
	int num_bars = get_num_bars(x);
	int max_filled_bars = (int)round(num_tracks * num_bars * proportion);
	int n_fill = random_on_range(max_filled_bars, engine);
	//int n_fill = rand() % (int)round(num_tracks * num_bars * proportion);
	std::vector<std::tuple<int,int>> choices;
	for (int track_num=0; track_num<num_tracks; track_num++) {
		for (int bar_num=0; bar_num<num_bars; bar_num++) {
			choices.push_back(std::make_pair(track_num,bar_num));
		}
	}
	std::set<std::tuple<int,int>> mask;
	shuffle(choices.begin(), choices.end(), *engine);
	for (int i=0; i<n_fill; i++) {
		mask.insert(choices[i]);
	}
	return mask;
}

// conversion for gm
midi::GM_TYPE gm_inst_to_string(int track_type, int instrument) {
	return GM_REV[is_drum_track(track_type) * 128 + instrument];
}

// status // param utilities




void dump_string_to_file(std::string s, std::string path) {
	std::ofstream fp;
	fp.open(path.c_str());
	fp << s;
	fp.close();
}

void print_status(midi::Status *x) {
	std::string output;
	google::protobuf::util::JsonPrintOptions opt;
	opt.add_whitespace = true;
	google::protobuf::util::MessageToJsonString(*x, &output, opt);
	std::cout << output << std::endl;
}

std::string get_piece_string(midi::Piece *x) {
	std::string output;
	google::protobuf::util::JsonPrintOptions opt;
	opt.add_whitespace = true;
	google::protobuf::util::MessageToJsonString(*x, &output, opt);
	return output;
}

void print_piece(midi::Piece *x) {
	std::cout << get_piece_string(x) << std::endl;
}

void dump_piece(midi::Piece *x, std::string path) {
	dump_string_to_file(get_piece_string(x), path);
}

void print_piece_summary(midi::Piece *x) {
	midi::Piece c(*x);
	c.clear_events();
	for (int track_num=0; track_num<c.tracks_size(); track_num++) {
		c.mutable_tracks(track_num)->clear_bars();
	}
	print_piece(&c);
}

midi::Status piece_to_status(midi::Piece *p) {
	midi::Status status;
	int track_num = 0;
	for (const auto track : p->tracks()) {
		midi::StatusTrack *t = status.add_tracks();
		t->set_track_id( track_num );
		t->set_track_type( track.track_type() );
		t->set_instrument( gm_inst_to_string(track.track_type(),track.instrument()) );
		t->set_density( midi::DENSITY_ANY );
		for (int bar_num=0; bar_num<track.bars_size(); bar_num++) {
			t->add_selected_bars( false );
			//midi::ContinuousFeature *f = t->add_internal_embeds();
			//f->set_av_polyphony(1);
			//f->set_av_silence(0);
			//f->set_note_duration(12);
		}
		t->set_autoregressive( false );
		t->set_ignore( false );
		track_num++;
	}
	return status;
}

std::string piece_to_status_py(std::string &piece_json) {
	midi::Piece p;
	google::protobuf::util::JsonStringToMessage(piece_json.c_str(), &p);
	midi::Status s = piece_to_status(&p);
	std::string output;
	google::protobuf::util::MessageToJsonString(s, &output);
	return output;
}

void flatten_velocity(midi::Piece *p, int velocity) {
	int num_events = (int)p->events_size();
	for (int i=0; i<num_events; i++) {
		midi::Event *e = p->mutable_events(i);
		if ((e) && (e->velocity() > 0)) {
			e->set_velocity( velocity );
		}
	}
}

midi::SampleParam default_sample_param() {
	midi::SampleParam param;
	param.set_tracks_per_step( 1 );
	param.set_bars_per_step( 2 );
	//param.set_model_dim( 4 );
	param.set_shuffle( true );
	param.set_percentage( 100 );
	param.set_temperature( 1. );
	param.set_batch_size( 1 );
	param.set_max_steps( 0 );
	param.set_verbose( false );
	//param.set_polyphony_hard_limit( 5 );
	return param;
}

std::string default_sample_param_py() {
	midi::SampleParam param = default_sample_param();
	std::string output;
	google::protobuf::util::MessageToJsonString(param, &output);
	return output;
}

// wrap these for acccesibility in python
// ===================================================================
// ===================================================================
// ===================================================================
// ===================================================================
// ===================================================================

midi::Piece string_to_piece(std::string json_string) {
	midi::Piece x;
	google::protobuf::util::JsonStringToMessage(json_string.c_str(), &x);
	return x;
}

midi::Status string_to_status(std::string json_string) {
	midi::Status x;
	google::protobuf::util::JsonStringToMessage(json_string.c_str(), &x);
	return x;
}

std::string piece_to_string(midi::Piece x) {
	std::string json_string;
	google::protobuf::util::MessageToJsonString(x, &json_string);
	return json_string;
}

std::string status_to_string(midi::Status x) {
	std::string json_string;
	google::protobuf::util::MessageToJsonString(x, &json_string);
	return json_string;
}

std::string random_perturb_py(std::string pstr, std::string sstr, double per, int seed) {
	midi::Piece p = string_to_piece(pstr);
	midi::Status s = string_to_status(sstr);
	random_perturb(&p, &s, per, seed);
	return piece_to_string(p);
}

std::string append_piece_py(std::string pstr, std::string qstr) {
	midi::Piece p = string_to_piece(pstr);
	midi::Piece q = string_to_piece(qstr);
	append_piece(&p, &q);
	return piece_to_string(p);
}

std::string update_valid_segments_py(std::string json_string, int num_bars, int min_tracks, bool opz) {
	midi::Piece x = string_to_piece(json_string);
	update_valid_segments(&x, num_bars, min_tracks, opz);
	return piece_to_string(x);
}

std::string update_note_density_py(std::string json_string) {
	midi::Piece x = string_to_piece(json_string);
	update_note_density(&x);
	return piece_to_string(x);
}

std::string update_av_polyphony_and_note_duration_py(std::string json_string) {
	midi::Piece x = string_to_piece(json_string);
	update_av_polyphony_and_note_duration(&x);
	return piece_to_string(x);
}

std::string prune_empty_tracks_py(std::string json_string, std::vector<int> bars) {
	midi::Piece x = string_to_piece(json_string);
	prune_empty_tracks(&x, bars);
	return piece_to_string(x);
}

std::string prune_tracks_py(std::string json_string, std::vector<int> tracks, std::vector<int> bars) {
	midi::Piece x = string_to_piece(json_string);
	prune_tracks_dev2(&x, tracks, bars);
	return piece_to_string(x);
}

std::string prune_notes_wo_offset_py(std::string json_string, bool ignore_drums) {
	midi::Piece x = string_to_piece(json_string);
	prune_notes_wo_offset(&x, ignore_drums);
	return piece_to_string(x);
}

std::string select_random_segment_py(std::string json_string, int num_bars, int min_tracks, int max_tracks, bool opz, int seed) {
	std::mt19937 engine(seed);
	midi::Piece x = string_to_piece(json_string);
	select_random_segment(&x, num_bars, min_tracks, max_tracks, opz, &engine);
	return piece_to_string(x);
}

std::string reorder_tracks_py(std::string json_string, std::vector<int> &track_order) {
	midi::Piece x = string_to_piece(json_string);
	reorder_tracks(&x, track_order);
	return piece_to_string(x);
}

std::string flatten_velocity_py(std::string json_string, int velocity) {
	midi::Piece x = string_to_piece(json_string);
	flatten_velocity(&x, velocity);
	return piece_to_string(x);
}

void print_piece_summary_py(std::string json_string) {
	midi::Piece x = string_to_piece(json_string);
	print_piece_summary(&x);
}

std::vector<int> piece_to_onset_distribution_py(std::string &pstr, int size) {
	midi::Piece p = string_to_piece(pstr);
	return piece_to_onset_distribution(&p, size);
}


}
// END OF NAMESPACE