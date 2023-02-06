#pragma once

#include <google/protobuf/util/json_util.h>

#include "../enum/encoder_config.h"
#include "../enum/constants.h"
#include "../enum/density.h"

#include "midi.pb.h"
#include <vector>
#include <string>


// provides functionality for operations on midi::piece

std::vector<midi::Event> get_sorted_events(midi::Piece *p) {
  std::vector<midi::Event> events;
  for (auto event : p->events()) {
    events.push_back( event );
  }
  std::sort(events.begin(), events.end(), [](const midi::Event a, const midi::Event b) { 
    if (a.time() != b.time()) {
      return a.time() < b.time();
    }
    if (std::min(a.velocity(),1) != std::min(b.velocity(),1)) {
      return std::min(a.velocity(),1) < std::min(b.velocity(),1);
    }
    if (a.pitch() != b.pitch()) {
      return a.pitch() < b.pitch();
    }
    return a.track() < b.track();
  });
  return events;
}

void update_pitch_limits(midi::Piece *p) {
  int min_pitch = INT_MAX;
  int max_pitch = 0;
  int track_num = 0;
  for (const auto track : p->tracks()) {
    int track_min_pitch = INT_MAX;
    int track_max_pitch = 0;
    for (const auto bar : track.bars()) {
      for (const auto event_id : bar.events()) {
        int pitch = p->events(event_id).pitch();
        track_min_pitch = std::min(pitch, track_min_pitch);
        track_max_pitch = std::max(pitch, track_max_pitch);
        min_pitch = std::min(pitch, min_pitch);
        max_pitch = std::max(pitch, max_pitch);
      }
    }
    p->mutable_tracks(track_num)->set_min_pitch( track_min_pitch );
    p->mutable_tracks(track_num)->set_max_pitch( track_max_pitch );
    track_num++;
  }
  p->set_min_pitch( min_pitch );
  p->set_max_pitch( max_pitch );
}

void update_valid_segments(midi::Piece *p, EncoderConfig *ec) {
  p->clear_valid_segments();
  p->clear_valid_tracks();

  // these should be able to passed in somewhere
  int seglen = ec->num_bars;
  int min_non_empty_bars = round(seglen * .75);
  int min_tracks = ec->min_tracks;

  if (p->tracks_size() < min_tracks) { return; } // no valid tracks

  // seglen + 1 crashes all the time ???
  for (int i=0; i<p->tracks(0).bars_size()-seglen+1; i++) {
    
    // check that all time sigs are supported
    bool supported_ts = true;
    bool is_four_four = true;
    for (int k=0; k<seglen; k++) {
      int beat_length = p->tracks(0).bars(i+k).internal_beat_length();
      supported_ts &= (time_sig_map.find(beat_length) != time_sig_map.end());
      //is_four_four &= (bool)p->tracks(0).bars(i+k).is_four_four();
      is_four_four &= (beat_length == 4);
    }

    // check which tracks are valid
    uint32_t vtracks = 0;
    for (int j=0; j<p->tracks_size(); j++) {
      int non_empty_bars = 0;
      for (int k=0; k<seglen; k++) {
        if (p->tracks(j).bars(i+k).internal_has_notes()) {
          non_empty_bars++;
        }
      }
      std::cout << "non empty bars " << non_empty_bars << std::endl;
      if (non_empty_bars >= min_non_empty_bars) {
        vtracks |= ((uint32_t)1 << j);
      }
    }

    std::cout << "VTRRACKS : " << vtracks << " " << p->tracks_size() << std::endl;

    // add to list if it meets the requirements
    if (!ec->mark_time_sigs) {
      supported_ts = is_four_four;
    }

    if ((__builtin_popcount(vtracks) >= min_tracks) && (supported_ts)) {
      p->add_valid_tracks(vtracks);
      p->add_valid_segments(i);
    }
  }
}

// =======================================================================
// average polyphony calculation

std::vector<midi::Note> track_events_to_notes(midi::Piece *p, int track_num) {
  midi::Event e;
  std::map<int,midi::Event> onsets;
  std::vector<midi::Note> notes;
  for (auto bar : p->tracks(track_num).bars()) {
    for (auto event_id : bar.events()) {
      e = p->events(event_id);
      if (e.velocity() > 0) {
        onsets[e.pitch()] = e;
      }
      else {
        auto it = onsets.find(e.pitch());
        if (it != onsets.end()) {
          midi::Event onset = it->second;
          midi::Note note;
          note.set_start( onset.time() );
          note.set_qstart( onset.qtime() );
          note.set_end( e.time() );
          note.set_qend( e.qtime() );
          note.set_velocity( onset.velocity() );
          note.set_pitch( onset.pitch() );
          note.set_instrument( onset.instrument() );
          note.set_track( onset.track() );
          note.set_bar( onset.bar() );
          note.set_is_drum( onset.is_drum() );
          notes.push_back(note);
          
          onsets.erase(it); // remove note
        }
      }
    }
  }
  return notes;
}

// for each tick measure amount of overlap
float average_polyphony(std::vector<midi::Note> &notes) {
  int max_tick = 0;
  for (auto note : notes) {
    max_tick = std::max(max_tick, note.end());
  }
  max_tick = std::min(max_tick, 1000000); // don't be too large
  std::vector<int> counts(max_tick,0);
  for (auto note : notes) {
    int s = std::max(0,note.start());
    int e = std::min(max_tick,note.end());
    for (int i=s; i<e; i++) {
      counts[i]++;
    }
  }
  float sum = 0;
  int total = 0;
  for (auto count : counts) {
    total += (int)(count > 0);
    sum += count;
  }
  return sum / total;
}

void update_polyphony(midi::Piece *p) {
  for (int i=0; i<p->tracks_size(); i++) {
    std::vector<midi::Note> notes = track_events_to_notes(p, i);
    p->mutable_tracks(i)->set_av_polyphony( average_polyphony(notes) );
  }
}

bool notes_overlap(midi::Note *a, midi::Note *b) {
  return (a->start() >= b->start()) && (a->start() < b->end());
}

int max_polyphony(std::vector<midi::Note> &notes) {
  int max_poly = 0;
  std::vector<int> overlap_counts(notes.size(), 0);
  for (int i=0; i<notes.size(); i++) {
    for (int j=0; j<notes.size(); j++) {
      if ((i!=j) && notes_overlap(&notes[i],&notes[j])) {
        overlap_counts[i]++;
        max_poly = std::max(max_poly, overlap_counts[i]);
      }
    }
  }
  return max_poly;
}

void update_max_polyphony(midi::Piece *p) {
  for (int i=0; i<p->tracks_size(); i++) {
    std::vector<midi::Note> notes = track_events_to_notes(p, i);
    p->mutable_tracks(i)->set_max_polyphony( max_polyphony(notes) );
  }
}

// =======================================================================

std::vector<int> MONO_DENSITY_QNT = {7, 10, 12, 14, 16, 18, 21, 26, 32, INT_MAX};
std::vector<int> POLY_DENSITY_QNT = {11, 16, 21, 28, 33, 41, 51, 64, 85, INT_MAX};

void update_note_density(midi::Piece *src) {
  for (int i=0; i<src->tracks_size(); i++) {
    // count the number of notes in a track
    int num_notes = 0;
    for (auto bar : src->tracks(i).bars()) {
      for (auto event_id : bar.events()) {
        if (src->events(event_id).velocity()) {
          num_notes++;
        }
      }
    }
    // map num notes to a density bin
    int index = 0;
    if (src->tracks(i).av_polyphony() < 1.1) {
      while (num_notes > MONO_DENSITY_QNT[index]) { index++; }
    }
    else {
      while (num_notes > POLY_DENSITY_QNT[index]) { index++; }
    }
    // update the protobuf message
    src->mutable_tracks(i)->set_note_density(index);

    // =======================================================
    // update note_density v2

    const midi::Track track = src->tracks(i);

    // calculate average notes per bar
    num_notes = 0;
    int bar_num = 0;
    std::set<int> valid_bars;
    for (auto bar : track.bars()) {
      for (auto event_id : bar.events()) {
        if (src->events(event_id).velocity()) {
          valid_bars.insert(bar_num);
          num_notes++;
        }
      }
      bar_num++;
    }
    int av_notes = round((double)num_notes / valid_bars.size());

    // calculate the density bin
    int qindex = track.instrument();
    if (track.is_drum()) {
      qindex = 128;
    }
    int bin = 0;
    while (av_notes > DENSITY_QUANTILES[qindex][bin]) { 
      bin++;
    }

    // update protobuf
    src->mutable_tracks(i)->set_note_density_v2(bin);
  }
}

// HELPER FOR MANIPULATING MIDI::PIECE
// maybe it should just skip if track_num is invalid
void partial_copy(midi::Piece *src, midi::Piece *dst, std::vector<int> &tracks, int start_bar, int n_bars) {
  midi::Track src_track;
  midi::Track *dst_track;
  midi::Bar src_bar;
  midi::Bar *dst_bar;
  midi::Event src_event;
  midi::Event *dst_event;

  int end_bar = start_bar + n_bars;
  if (n_bars == -1) {
    end_bar = src->tracks(0).bars_size();
  }

  // we can get bar_start by taking the time of the first event
  // and then subtract the qtime
  int event_id = src->tracks(tracks[0]).bars(start_bar).events(0);
  int start_time = src->events(event_id).time() - src->events(event_id).qtime();

  // copy piece metadata
  dst->set_resolution( src->resolution() );
  dst->set_tempo( src->tempo() );
  dst->set_min_pitch( src->min_pitch() ); // this may be wrong now ...
  dst->set_max_pitch( src->max_pitch() ); // this may be wrong now ...
  // valid tracks and valid_segments would need to be recalculated
  dst->add_valid_segments(0);
  dst->add_valid_tracks( (1<<(tracks.size())) - 1 );

  int track_count = 0;
  for (auto const track_num : tracks) {
    if ((track_num < 0) || (track_num >= src->tracks_size())) {
      std::cout << "ERROR : TRACK " << track_num << " IS OUT OF RANGE!" << std::endl;
      throw(1);
    }
    src_track = src->tracks(track_num);
    dst_track = dst->add_tracks();
    
    // set the other metadata for each track
    dst_track->set_is_drum( src_track.is_drum() );
    dst_track->set_min_pitch( src_track.min_pitch() );
    dst_track->set_max_pitch( src_track.max_pitch() );
    dst_track->set_instrument( src_track.instrument() );

    for (int bar_num=start_bar; bar_num<end_bar; bar_num++) {
      if ((bar_num < 0) || (bar_num >= src_track.bars_size())) {
        std::cout << "ERROR : BAR " << bar_num << " IS OUT OF RANGE!" << std::endl;
        throw(1);
      }
      src_bar = src_track.bars(bar_num);
      dst_bar = dst_track->add_bars();
      
      // set the other metadata for each bar
      dst_bar->set_is_four_four( src_bar.is_four_four() );
      dst_bar->set_internal_has_notes( src_bar.internal_has_notes() );

      for (int i=0; i<src_bar.events_size(); i++) {
        int current_event_count = dst->events_size();
        src_event = src->events(src_bar.events(i));
        dst_event = dst->add_events();
        
        // copy all members of the event
        // make time relative to time of start_bar
        dst_event->set_time( src_event.time() - start_time );
        dst_event->set_velocity( src_event.velocity() );
        dst_event->set_pitch( src_event.pitch() );
        dst_event->set_instrument( src_event.instrument() );
        dst_event->set_track( track_count );
        dst_event->set_qtime( src_event.qtime() );
        dst_event->set_bar( src_event.bar() );
        dst_event->set_is_drum( src_event.is_drum() );

        // add to the bar / track
        dst_bar->add_events( current_event_count );
      }
    }
    track_count++;
  }
}

std::string partial_copy_json(std::string &json_string, std::vector<int> &tracks, int start_bar, int n_bars) {
  std::string json_output;
  midi::Piece src, dst;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &src);
  partial_copy(&src, &dst, tracks, start_bar, n_bars);
  google::protobuf::util::MessageToJsonString(dst, &json_output);
  return json_output;
}


// get valid segments
void find_valid_segments(midi::Piece *p) {
  p->clear_valid_tracks();
  p->clear_valid_segments();

  int seglen = 4;
  for (int i=0; i<p->tracks(0).bars_size()-seglen+1; i++) {
    // check that it is 4/4
    bool is_four_four = true;
    for (int k=0; k<seglen; k++) {
      is_four_four &= (bool)p->tracks(0).bars(i+k).is_four_four();
    }

    // check which tracks are valid
    uint32_t vtracks = 0;
    for (int j=0; j<p->tracks_size(); j++) {
      for (int k=0; k<seglen; k++) {
        if (p->tracks(j).bars(i+k).internal_has_notes()) {
          vtracks |= ((uint32_t)1 << j);
        }
      }
    }

    if ((__builtin_popcount(vtracks) >= 1) && (is_four_four)) {
      p->add_valid_tracks(vtracks);
      p->add_valid_segments(i);
    }
  }
}

/*
void remove_unused_events(midi::Piece *p) {
  std::set<int> used_events;
  for (const auto track : p->tracks()) {
    for (const auto bar : track.bars()) {
      for (const auto event : )
      used_events.add()
    }
  }
}
*/

// ========================================================================
// ========================================================================

void partial_copy_v2(midi::Piece *src, midi::Piece *dst, std::vector<int> tracks, std::vector<int> bars, std::set<int> *blanks) {
  //cout << "=======================" << std::endl;
  int event_count = 0;
  dst->CopyFrom(*src);
  dst->clear_tracks();
  dst->clear_events();
  for (const int track_num : tracks) {
    const midi::Track track = src->tracks(track_num);
    midi::Track *t = dst->add_tracks();
    t->CopyFrom(track);
    t->clear_bars();
    if ((!blanks) || (blanks->find(track_num) == blanks->end())) {
      int new_bar_num = 0;
      int start_time = track.bars(bars[0]).time();
      for (const int bar_num : bars) {
        midi::Bar *b = t->add_bars();
        // only copy over if it exists in original
        if (bar_num < track.bars_size()) {
          const midi::Bar bar = track.bars(bar_num);
          b->CopyFrom(bar);
          b->clear_events();
          b->set_time( b->time() - start_time );
          for (const auto event : bar.events()) {
            b->add_events(event_count);
            midi::Event *e = dst->add_events();
            e->CopyFrom(src->events(event));
            e->set_time(e->time() - start_time); // change the time
            e->set_bar(new_bar_num); // reset the bar number
            event_count++;
          }
        }
        new_bar_num++;
      }
    }
  }
}

template<typename T>
std::vector<T> arange(T start, T stop, T step = 1) {
  std::vector<T> values;
  for (T value = start; value < stop; value += step)
      values.push_back(value);
  return values;
}

template<typename T>
std::vector<T> arange(T stop) {
  return arange(0, stop, 1);
}

int get_bar_count(midi::Piece *p) {
  int bar_count = INT_MAX;
  for (const auto track : p->tracks()) {
    bar_count = std::min(bar_count, track.bars_size());
  }
  return bar_count;
}

std::string clear_track(std::string &json_string, int track_num) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);

  // inner
  midi::Piece o;
  std::vector<int> tracks = arange(p.tracks_size());
  std::vector<int> bars = arange(get_bar_count(&p));
  std::set<int> blanks;
  blanks.insert(track_num);
  partial_copy_v2(&p, &o, tracks, bars, &blanks);
  // inner

  std::string json_output;
  google::protobuf::util::MessageToJsonString(o, &json_output);
  return json_output;
}

std::string remove_track(std::string &json_string, int track_num) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);

  // inner
  midi::Piece o;
  std::vector<int> tracks = arange(p.tracks_size());
  std::vector<int> bars = arange(get_bar_count(&p));
  tracks.erase(tracks.begin() + track_num);
  partial_copy_v2(&p, &o, tracks, bars, NULL);
  find_valid_segments(&o);
  // inner

  std::string json_output;
  google::protobuf::util::MessageToJsonString(o, &json_output);
  return json_output;
}

std::string remove_tracks(std::string &json_string, std::vector<int> rtracks) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);

  // inner
  midi::Piece o;
  std::set<int> h(rtracks.begin(), rtracks.end());
  std::vector<int> tracks;
  for (int i=0; i<p.tracks_size(); i++) {
    if (h.find(i) == h.end()) {
      tracks.push_back(i);
    }
  }
  std::vector<int> bars = arange(get_bar_count(&p));
  partial_copy_v2(&p, &o, tracks, bars, NULL);
  find_valid_segments(&o);
  // inner

  std::string json_output;
  google::protobuf::util::MessageToJsonString(o, &json_output);
  return json_output;
}

std::string prune_tracks(std::string &json_string, std::vector<int> tracks) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);

  // inner
  midi::Piece o;
  std::vector<int> bars = arange(get_bar_count(&p));
  // remove tracks that don't exist
  std::vector<int> valid_tracks;
  for (const auto track : tracks) {
    if ((track < p.tracks_size()) && (track >= 0)) {
      valid_tracks.push_back( track );
    }
  }
  partial_copy_v2(&p, &o, valid_tracks, bars, NULL);
  EncoderConfig ec;
  // manually set valid segments
  o.clear_valid_segments();
  o.clear_valid_tracks();
  o.add_valid_segments(0);
  o.add_valid_tracks((1<<o.tracks_size())-1);
  // inner

  std::string json_output;
  google::protobuf::util::MessageToJsonString(o, &json_output);
  return json_output;
}

std::string prune_tracks_and_bars(std::string &json_string, std::vector<int> tracks, int nbars) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);

  // inner
  midi::Piece o;
  // remove tracks that don't exist
  if (tracks.size() == 0) {
    tracks = arange(p.tracks_size());
  }
  std::vector<int> valid_bars = arange(nbars);
  std::vector<int> valid_tracks;
  for (const auto track : tracks) {
    if ((track < p.tracks_size()) && (track >= 0)) {
      if (p.tracks(track).bars_size() >= nbars) {
        valid_tracks.push_back( track );
      }
    }
  }
  
  partial_copy_v2(&p, &o, valid_tracks, valid_bars, NULL);
  EncoderConfig ec;
  // manually set valid segments
  o.clear_valid_segments();
  o.clear_valid_tracks();
  o.add_valid_segments(0);
  o.add_valid_tracks((1<<o.tracks_size())-1);
  // inner

  std::string json_output;
  google::protobuf::util::MessageToJsonString(o, &json_output);
  return json_output;
}

std::string update_segments(std::string &json_string, EncoderConfig *ec) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);

  // inner
  update_valid_segments(&p, ec);
  // inner

  std::string json_output;
  google::protobuf::util::MessageToJsonString(p, &json_output);
  return json_output;
}

std::string add_track(std::string &json_string) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);

  // inner
  midi::Piece o;
  o.CopyFrom(p);
  midi::Track *t = o.add_tracks();
  // inner

  std::string json_output;
  google::protobuf::util::MessageToJsonString(o, &json_output);
  return json_output;
}

std::string select_segment(std::string &json_string, int max_tracks, int index) {
  midi::Piece p;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);
  std::string json_output;

  // inner
  if (p.valid_segments_size() == 0) { 
    return json_output; 
  }
  if (index == -1) {
    srand(time(NULL));
    index = rand() % p.valid_segments_size();
  }
  int start_bar = p.valid_segments(index);

  std::vector<int> vtracks;
  for (int i=0; i<32; i++) {
    if ((p.valid_tracks(index) & (1<<i)) && (vtracks.size()<max_tracks)) {
      vtracks.push_back(i);
    }
  }

  //cout << index << " " << vtracks.size() << std::endl;

  midi::Piece o; 
  std::vector<int> bars = arange(start_bar,start_bar+p.segment_length());
  partial_copy_v2(&p, &o, vtracks, bars, NULL);
  //find_valid_segments(&o);
  // manually set valid segments
  o.clear_valid_segments();
  o.clear_valid_tracks();
  o.add_valid_segments(0);
  o.add_valid_tracks((1<<o.tracks_size())-1);
  // inner
  
  google::protobuf::util::MessageToJsonString(o, &json_output);
  return json_output;
}