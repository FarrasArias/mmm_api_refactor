#pragma once

#include <iostream>
#include <vector>
#include <tuple>
#include <map>
#include <set>

#include <iostream>
#include <fstream>
#include <sstream>

#include "../../midifile/include/Binasc.h"
#include "../../midifile/include/MidiFile.h"

#include "protobuf/midi.pb.h"
#include "protobuf/piece_util.h"
#include "enum/constants.h"
#include "enum/density.h"
#include "enum/te.h"
#include "enum/encoder_config.h"

#include "adjacent_range.h"

// =============================================================
// =============================================================
// =============================================================

int check_type_inner(std::istream& input) {

  if (input.peek() != 'M') {
		std::stringstream binarydata;
		smf::Binasc binasc;
		binasc.writeToBinary(binarydata, input);
		binarydata.seekg(0, std::ios_base::beg);
		if (binarydata.peek() != 'M') {
			return -1;
		} else {
			return check_type_inner(binarydata);
		}
	}

	int    character;
	unsigned long  longdata;
	ushort shortdata;

	// Read the MIDI header (4 bytes of ID, 4 byte data size,
	// anticipated 6 bytes of data.

	character = input.get();
	if (character == EOF) {
		return -1;
	} else if (character != 'M') {
		return -1;
	}

	character = input.get();
	if (character == EOF) {
		return -1;
	} else if (character != 'T') {
		return -1;
	}

	character = input.get();
	if (character == EOF) {
		return -1;
	} else if (character != 'h') {
		return -1;
	}

	character = input.get();
	if (character == EOF) {
		return -1;
	} else if (character != 'd') {
		return -1;
	}

	// read header size (allow larger header size?)
	longdata = smf::MidiFile::readLittleEndian4Bytes(input);
	if (longdata != 6) {
		return -1;
	}

	// Header parameter #1: format type
	return smf::MidiFile::readLittleEndian2Bytes(input);
}

int check_type(const std::string& filename) {
  std::fstream input;
	input.open(filename.c_str(), std::ios::binary | std::ios::in);
  if (!input.is_open()) { return false; }
  return check_type_inner(input);
}


// =============================================================
// =============================================================
// =============================================================
static const int DRUM_CHANNEL = 9;

#define QUIET_CALL(noisy) { \
    std::cout.setstate(ios_base::failbit);\
    std::cerr.setstate(ios_base::failbit);\
    (noisy);\
    std::cout.clear();\
    std::cerr.clear();\
}

using Note = std::tuple<int,int,int,int,int>; // (ONSET,PITCH,DURATION,VELOCITY,INST)
using Event = std::tuple<int,int,int,int>; // (TIME,VELOCITY,PITCH,INSTRUMENT) 

using VOICE_TUPLE = std::tuple<int,int,int>; // CHANGE

int quantize_beat(double x, double TPQ, double SPQ, double cut=.5) {
  return (int)((x / TPQ * SPQ) + (1.-cut)) * (TPQ / SPQ);
}

int quantize_second(double x, double spq, double ticks, double steps_per_second, double cut=.5) {
  return (int)((x / ticks * spq * steps_per_second) + (1.-cut));
}

bool sort_events(const midi::Event *a, const midi::Event *b) { 
  if (a->time() != b->time()) {
    return a->time() < b->time();
  }
  if (std::min(a->velocity(),1) != std::min(b->velocity(),1)) {
    return std::min(a->velocity(),1) < std::min(b->velocity(),1);
  }
  if (a->pitch() != b->pitch()) {
    return a->pitch() < b->pitch();
  }
  return a->track() < b->track();
}

// ==================================================================
// go through midi once 
// divide MIDI messages into tracks
// divide each track into bars

using TT_VOICE_TUPLE = std::tuple<int,int,int,int>; // CHANGE

class PH {
public:
  PH(smf::MidiFile *mfile, midi::MPiece *piece) {
    int max_tick = 0;
    int current_track = 0;
    track_count = mfile->getTrackCount();
    TPQ = mfile->getTPQ();
    parse(mfile, piece);
  }
  int track_count;
  int TPQ;
  int current_track;
  int max_tick;
  smf::MidiEvent *mevent;
  std::map<TT_VOICE_TUPLE,int> track_map;
  std::map<int,TT_VOICE_TUPLE> rev_track_map;
  std::map<int,int> timesigs;
  std::map<int,std::tuple<int,int>> bars;
  std::vector<std::vector<midi::MEvent>> events; // events split into tracks
  std::array<int,64> instruments; // instruments on each channel

  void parse(smf::MidiFile *mfile, midi::MPiece *piece) {

    // extract information from the midi file
    for (int track=0; track<track_count; track++) {
      current_track = track;
      std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
      for (int event=0; event<(*mfile)[track].size(); event++) { 
        mevent = &((*mfile)[track][event]);
        if (mevent->isPatchChange()) {
          handle_patch_message(mevent);
        }
        else if (mevent->isTimeSignature()) {
          handle_time_sig_message(mevent);
        }
        else if ((mevent->isNoteOn() || mevent->isNoteOff()) && (mevent->isLinked())) {
          handle_note_message(mevent);
        }
      }
    }

    // add a timesig at beginning and end
    // and then make a mapping from tick to bar_number and bar_length
    int count = 0;
    if (timesigs.find(0) == timesigs.end()) {
      timesigs[0] = TPQ*4;
    }
    timesigs[max_tick + TPQ] = 0; // no bar length
    for (const auto &p : make_adjacent_range(timesigs)) {
      for (int tick=p.first.first; tick<p.second.first; tick+=p.first.second) {
        bars[tick] = std::make_tuple(p.first.second, count);
        count++;
      }
    }

    // construct the piece
    midi::MTrack *track = NULL;
    midi::MBar *bar = NULL;
    midi::MEvent *event = NULL;

    // sort the events in each track
    // should we do this here?

    for (int i=0; i<events.size(); i++) {
      // add track and track metadata
      track = piece->add_tracks();
      track->set_instrument( std::get<2>(rev_track_map[i]) );
      track->set_type( std::get<3>(rev_track_map[i]) );

      // add bars and bar metadata
      for (const auto &bar_info : bars) {
        bar = track->add_bars();
        bar->set_internal_beat_length( std::get<0>(bar_info) / TPQ );
      }

      // add events
      for (int j=0; j<events[i].size(); j++) {
        int bar_num = get_bar_num( events[i][j].time() );
        bar = track->mutable_bars( bar_num );
        
        bar->add_events( piece->events_size() );
        event = piece->add_events();
        event->CopyFrom( events[i][j] );
      }
    }
  }

  /*
  TT_VOICE_TUPLE infer_voice(int track, int channel, int inst, int pitch) {
    int tt = (int)TE_INST_MAP[inst];
    if (channel == DRUM_CHANNEL) {
      tt = (int)TE_DRUM_MAP[pitch];
    }
    return std::make_tuple(track,channel,inst,tt);
  }
  */

  TT_VOICE_TUPLE infer_voice(int track, int channel, int inst, int pitch) {
    // typically there are just two types of tracks DRUM and pitched instruments
    int track_type = channel == DRUM_CHANNEL;
    return std::make_tuple(track,channel,inst,track_type);
  }

  int get_bar_num(int tick) {
    auto it = bars.upper_bound(tick);
    assert(it != bars.begin());
    return std::get<1>(prev(it)->second);
  }

  void handle_patch_message(smf::MidiEvent *mevent) {
    int channel = mevent->getChannelNibble();
    instruments[channel] = (int)((*mevent)[1]);
  }

  void handle_time_sig_message(smf::MidiEvent *mevent) {
    int barlength = (double)(TPQ * 4 * (*mevent)[3]) / (1<<(*mevent)[4]);
    timesigs[mevent->tick] = barlength;
  }

  void handle_note_message(smf::MidiEvent *mevent) {
    int channel = mevent->getChannelNibble();
    int pitch = (int)(*mevent)[1];
    int velocity = (int)(*mevent)[2];

    TT_VOICE_TUPLE vtup = infer_voice(
      current_track,channel,instruments[channel],pitch);

    // update track map
    if (track_map.find(vtup) == track_map.end()) {
      int current_size = track_map.size();
      track_map[vtup] = current_size;
      rev_track_map[current_size] = vtup;
      events.push_back( std::vector<midi::MEvent>() );
    }

    // add to list of events per track
    midi::MEvent event;
    event.set_time( mevent->tick );
    event.set_pitch( pitch );
    event.set_velocity( velocity );
    events[track_map[vtup]].push_back( event );

    max_tick = std::max(max_tick, mevent->tick);
  }
};

int run_parse_helper(std::string &filepath) {
  smf::MidiFile mfile;
  try {
    QUIET_CALL(mfile.read(filepath));
    mfile.makeAbsoluteTicks();
    mfile.linkNotePairs();
  }
  catch(int e) {
    return INVALID_MIDI; // ERROR reading MIDI FILE
  }
  midi::MPiece piece;
  PH ph(&mfile, &piece);

}

// ==================================================================

// function which takes an event and determines to which track it belongs
// this is just for drums
// we can product labels which tell us if it can be considered a lead or whatever
// at the end
// actually we can determine bass right away too

using TE_VOICE_TUPLE = std::tuple<int,int,int,int>; // CHANGE



// function to determine voice tuples ...
// we will still keep separate drum tracks for extra reasons
// otherwise just direct map
TE_VOICE_TUPLE infer_voice(int track, int channel, int instrument, int pitch) {
  assert(pitch >= 0);
  assert(pitch < 128);
  assert(instrument >= 0);
  assert(instrument < 128);
  TE_TRACK_TYPE tt = TE_INST_MAP[instrument];
  if (channel == DRUM_CHANNEL) {
    tt = TE_DRUM_MAP[pitch];
  }
  assert(tt >= 0);
  assert(tt < 16);
  return std::make_tuple(track,channel,instrument,tt);
}

int parse_te(std::string filepath, midi::Piece *p, EncoderConfig *ec) {
  smf::MidiFile midifile;
  try {
    QUIET_CALL(midifile.read(filepath));
    midifile.makeAbsoluteTicks();
    midifile.linkNotePairs();
  }
  catch(int e) {
    return INVALID_MIDI; // ERROR reading MIDI FILE
  }

  bool is_offset;
  int pitch, time, velocity, channel, current_tick, bar, voice;
  int start, end, start_bar, end_bar, rel_start, rel_end;
  int track_count = midifile.getTrackCount();
  int TPQ = midifile.getTPQ();
  std::vector<int> instruments(16,0);

  smf::MidiEvent *mevent;
  midi::Event *e;
  
  // get time signature and track/channel info
  int max_tick = 0;
  std::map<TE_VOICE_TUPLE,int> track_map;
  std::map<int,TE_VOICE_TUPLE> rev_track_map;
  std::map<int,std::tuple<int,int,int>> timesigs;
  //timesigs[0] = std::make_tuple(4,4,TPQ * 4);

  // loop over each track and determine ...
  // 1) voice membership
  // 2) time signatures
  // 3) the maximum tick
  for (int track=0; track<track_count; track++) {
    std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
    for (int event=0; event<midifile[track].size(); event++) { 
      mevent = &(midifile[track][event]);
      if (mevent->isTimeSignature()) {
        int barlength = (double)(TPQ * 4 * (*mevent)[3]) / (1<<(*mevent)[4]);
        timesigs[mevent->tick] = std::make_tuple(
          (*mevent)[3], 1<<(*mevent)[4], barlength);
      }
      if ((mevent->isNoteOn()) || (mevent->isNoteOff())) {
        channel = mevent->getChannelNibble();
        pitch = (*mevent)[1];
        TE_VOICE_TUPLE vtup = infer_voice(
          track,channel,instruments[channel],pitch);

        if (track_map.find(vtup) == track_map.end()) {
          int current_size = track_map.size();
          track_map[vtup] = current_size;
          rev_track_map[current_size] = vtup;
        }
        max_tick = std::max(max_tick, mevent->tick);
      }
      if (mevent->isTempo()) {
        p->set_tempo(mevent->getTempoBPM());
      }
      if (mevent->isPatchChange()) {
        channel = mevent->getChannelNibble();
        instruments[channel] = (int)((*mevent)[1]);
      } 
    }
  }
  
  
  std::map<int,int> barlines;
  std::map<int,int> barlengths;

  // over-ride to 4/4
  if (ec->force_four_four) {
    timesigs.clear();
    timesigs[0] = std::make_tuple(4,4,TPQ * 4);
  }

  // we only consider midi files which have time signature information
  // unless an ignore flag is passed
  // here we want to determine ...
  // 1) the start position of each bar
  // 2) the length of each bar
  if (timesigs.size() > 0) {

    // add TPQ to max_tick to account for possible quantization later
    // only add if max tick is greater than last timesig
    if (max_tick + TPQ > timesigs.rbegin()->first) {
      timesigs[max_tick + TPQ] = std::make_tuple(0,0,0);
    }

    int barlength;
    int current_bar;
    auto it = timesigs.begin();
    while (it->first < timesigs.rbegin()->first) {
      barlength = std::get<2>(it->second);
      for (int i=it->first; i<next(it)->first; i=i+barlength) {
        current_bar = barlines.size();
        barlines[i] = current_bar;
        barlengths[current_bar] = barlength;
      }
      it++;
    }
    // add last barline
    current_bar = barlines.size();
    barlines[timesigs.rbegin()->first + barlength] = current_bar;
    barlengths[current_bar] = barlength;
  }
  else {
    return INVALID_TIME_SIGS; // don't have accurate time-sigs
  }
  
  int MIN_TICK = timesigs.begin()->first;
  std::vector<midi::Event*> events;
  //vector<int> min_pitches(track_map.size() + 1,128);
  //vector<int> max_pitches(track_map.size() + 1,0);

  // here we create a list of events
  for (int track=0; track<track_count; track++) {
    std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
    for (int event=0; event<midifile[track].size(); event++) {
      mevent = &(midifile[track][event]);

      // we ignore notes that do not have an end
      if ((mevent->isNoteOn()) && (mevent->isLinked())) {
        pitch = (*mevent)[1];
        velocity = (int)(*mevent)[2];
        channel = mevent->getChannelNibble();

        // determine the track
        // might add note to multiple tracks
        // keep the default mappings

        TE_VOICE_TUPLE vtup = infer_voice(
          track,channel,instruments[channel],pitch);
        voice = track_map.find(vtup)->second; // CAN THIS FAIL ?

        // NOTE : this keeps values in tick units but already quantized
        start = quantize_beat(mevent->tick, TPQ, ec->resolution);
        end = quantize_beat(mevent->getLinkedEvent()->tick, TPQ, ec->resolution);

        // a valid note must be on or after the first barline
        // and must have a positive duration
        if ((start >= MIN_TICK) && (end - start > 0)) {

          auto start_bar_it = barlines.lower_bound(start);
          auto end_bar_it = barlines.lower_bound(end);

          // onset should be >= start_bar tick
          if (start_bar_it->first > start) {
            start_bar_it--;
          }
          // offset should be > end_bar tick
          // this pushes offsets on the barline into the previous bar 
          if (end_bar_it->first >= end) {
            end_bar_it--;
          }

          start_bar = start_bar_it->second;
          end_bar = end_bar_it->second;

          // start and end relative to the barline
          rel_start = ((start - start_bar_it->first) * ec->resolution) / TPQ;
          rel_end = ((end - end_bar_it->first) * ec->resolution) / TPQ;

          if ((start_bar >= 0) && (end_bar >= 0)) {

            // add onset
            events.push_back( new midi::Event);
            e = events.back();
            e->set_time((start * ec->resolution) / TPQ);
            e->set_velocity(velocity);
            e->set_pitch(pitch);
            e->set_instrument(instruments[channel]);
            e->set_track(voice);
            e->set_qtime(rel_start);
            e->set_is_drum(channel == DRUM_CHANNEL);
            e->set_bar(start_bar);
            //e->set_te_track(std::get<3>(vtup));

            // add offset
            events.push_back( new midi::Event);
            e = events.back();
            e->set_time((end * ec->resolution) / TPQ);
            e->set_velocity(0);
            e->set_pitch(pitch);
            e->set_instrument(instruments[channel]);
            e->set_track(voice);
            e->set_qtime(rel_end);
            e->set_is_drum(channel == DRUM_CHANNEL);
            e->set_bar(end_bar);
            //e->set_te_track(std::get<3>(vtup));
          }
        }
      }

      if (midifile[track][event].isPatchChange()) {
        channel = midifile[track][event].getChannelNibble();
        instruments[channel] = (int)midifile[track][event][1];
      }      
    }
  }

  // sort the events
  std::sort(events.begin(), events.end(), sort_events);

  int ntracks = track_map.size();
  std::vector<std::vector<midi::Bar*>> bars(ntracks, std::vector<midi::Bar*>(barlines.size()));
  for (int i=0; i<ntracks; i++) {
    midi::Track *t = p->add_tracks();
    t->set_is_drum( (std::get<1>(rev_track_map[i]) == DRUM_CHANNEL) );
    t->set_instrument( std::get<2>(rev_track_map[i]) ); // inst in rev_track_map
    t->set_te_track( std::get<3>(rev_track_map[i]) );
    int j = 0;
    for (const auto kv : barlines) {
      midi::Bar *b = t->add_bars();
      b->set_is_four_four( barlengths[j] == (TPQ*4) );
      b->set_internal_has_notes( false );
      b->set_time( kv.first * ec->resolution / TPQ );
      b->set_internal_beat_length( (double)barlengths[j] / TPQ );
      bars[i][j] = b;
      j++;
    }
  }

  int n = 0;
  for (const auto event : events) {
    midi::Event *e = p->add_events();
    e->CopyFrom(*event);
    bars[event->track()][event->bar()]->add_events(n);
    if (e->velocity() > 0) {
      bars[event->track()][event->bar()]->set_internal_has_notes(true);
    }
    n++;
    delete event; // otherwise memory leak
  }

  // update meta-data about the piece
  // may need some additional meta-data for determining 
  // the te track type
  update_valid_segments(p, ec);
  update_pitch_limits(p);
  update_note_density(p);
  update_polyphony(p);
  
  p->set_segment_length(ec->num_bars);
  p->set_resolution(ec->resolution);

  // maybe get max polyphony
  update_max_polyphony(p);

  // map unassigned things to tracks
  for (int i=0; i<p->tracks_size(); i++) {
    if (p->tracks(i).te_track() == AUX_INST_TRACK) {
      int max_polyphony = p->tracks(i).max_polyphony();
      TE_TRACK_TYPE tt = AUX_INST_TRACK;
      if (max_polyphony <= 2) {
        tt = ARP_TRACK;
      }
      else if (max_polyphony <= 3) {
        tt = LEAD_TRACK;
      }
      else if (max_polyphony <= 4) {
        tt = CHORD_TRACK;
      }
      p->mutable_tracks(i)->set_te_track( tt );
    }
  }

}

// ==================================================================

void parse_new(std::string filepath, midi::Piece *p, EncoderConfig *ec, std::map<std::string,std::vector<std::string>> *genre_data=NULL) {
  smf::MidiFile midifile;
  try {
    QUIET_CALL(midifile.read(filepath));
    midifile.makeAbsoluteTicks();
    midifile.linkNotePairs();
  }
  catch(int e) {
    //cerr << "MIDI PARSING FAILED ..." << std::endl;
    return; // nothing to parse
  }


  bool is_offset;
  int pitch, time, velocity, channel, current_tick, bar, voice;
  int start, end, start_bar, end_bar, rel_start, rel_end;
  int track_count = midifile.getTrackCount();
  int TPQ = midifile.getTPQ();
  std::vector<int> instruments(16,0);

  smf::MidiEvent *mevent;
  midi::Event *e;
  
  // get time signature and track/channel info
  int max_tick = 0;
  std::map<VOICE_TUPLE,int> track_map;
  std::map<int,VOICE_TUPLE> rev_track_map;
  std::map<int,std::tuple<int,int,int>> timesigs;
  //timesigs[0] = std::make_tuple(4,4,TPQ * 4);

  // loop over each track and determine ...
  // 1) voice membership
  // 2) time signatures
  // 3) the maximum tick
  for (int track=0; track<track_count; track++) {
    std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
    for (int event=0; event<midifile[track].size(); event++) { 
      mevent = &(midifile[track][event]);
      if (mevent->isTimeSignature()) {
        int barlength = (double)(TPQ * 4 * (*mevent)[3]) / (1<<(*mevent)[4]);
        timesigs[mevent->tick] = std::make_tuple(
          (*mevent)[3], 1<<(*mevent)[4], barlength);
      }
      if ((mevent->isNoteOn()) || (mevent->isNoteOff())) {
        channel = mevent->getChannelNibble();
        VOICE_TUPLE vtup = std::make_tuple(track,channel,instruments[channel]);
        if (track_map.find(vtup) == track_map.end()) {
          int current_size = track_map.size();
          track_map[vtup] = current_size;
          rev_track_map[current_size] = vtup;
        }
        max_tick = std::max(max_tick, mevent->tick);
      }
      if (mevent->isTempo()) {
        p->set_tempo(mevent->getTempoBPM());
      }
      if (mevent->isPatchChange()) {
        channel = mevent->getChannelNibble();
        instruments[channel] = (int)((*mevent)[1]);
      } 
    }
  }
  
  
  std::map<int,int> barlines;
  std::map<int,int> barlengths;

  // over-ride to 4/4
  if (ec->force_four_four) {
    timesigs.clear();
    timesigs[0] = std::make_tuple(4,4,TPQ * 4);
  }

  // we only consider midi files which have time signature information
  // unless an ignore flag is passed
  // here we want to determine ...
  // 1) the start position of each bar
  // 2) the length of each bar
  if (timesigs.size() > 0) {

    // add TPQ to max_tick to account for possible quantization later
    // only add if max tick is greater than last timesig
    if (max_tick + TPQ > timesigs.rbegin()->first) {
      timesigs[max_tick + TPQ] = std::make_tuple(0,0,0);
    }

    int barlength;
    int current_bar;
    auto it = timesigs.begin();
    while (it->first < timesigs.rbegin()->first) {
      barlength = std::get<2>(it->second);
      for (int i=it->first; i<next(it)->first; i=i+barlength) {
        current_bar = barlines.size();
        barlines[i] = current_bar;
        barlengths[current_bar] = barlength;
      }
      it++;
    }
    // add last barline
    current_bar = barlines.size();
    barlines[timesigs.rbegin()->first + barlength] = current_bar;
    barlengths[current_bar] = barlength;
  }
  else {
    //cerr << "NO TIME SIGNATURE DATA ..." << std::endl;
    return; // don't have accurate time-sigs
  }
  
  int MIN_TICK = timesigs.begin()->first;
  std::vector<midi::Event*> events;
  //vector<int> min_pitches(track_map.size() + 1,128);
  //vector<int> max_pitches(track_map.size() + 1,0);

  // here we create a list of events
  for (int track=0; track<track_count; track++) {
    std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
    for (int event=0; event<midifile[track].size(); event++) {
      mevent = &(midifile[track][event]);

      // we ignore notes that do not have an end
      if ((mevent->isNoteOn()) && (mevent->isLinked())) {
        pitch = (*mevent)[1];
        velocity = (int)(*mevent)[2];
        channel = mevent->getChannelNibble();
        VOICE_TUPLE vtup = std::make_tuple(track,channel,instruments[channel]);
        voice = track_map.find(vtup)->second; // CAN THIS FAIL ?

        // NOTE : this keeps values in tick units but already quantized
        start = quantize_beat(mevent->tick, TPQ, ec->resolution);
        end = quantize_beat(mevent->getLinkedEvent()->tick, TPQ, ec->resolution);

        // a valid note must be on or after the first barline
        // and must have a positive duration
        if ((start >= MIN_TICK) && (end - start > 0)) {

          auto start_bar_it = barlines.lower_bound(start);
          auto end_bar_it = barlines.lower_bound(end);

          // onset should be >= start_bar tick
          if (start_bar_it->first > start) {
            start_bar_it--;
          }
          // offset should be > end_bar tick
          // this pushes offsets on the barline into the previous bar 
          if (end_bar_it->first >= end) {
            end_bar_it--;
          }

          start_bar = start_bar_it->second;
          end_bar = end_bar_it->second;

          // start and end relative to the barline
          rel_start = ((start - start_bar_it->first) * ec->resolution) / TPQ;
          rel_end = ((end - end_bar_it->first) * ec->resolution) / TPQ;

          if ((start_bar >= 0) && (end_bar >= 0)) {

            // add onset
            events.push_back( new midi::Event);
            e = events.back();
            e->set_time((start * ec->resolution) / TPQ);
            e->set_velocity(velocity);
            e->set_pitch(pitch);
            e->set_instrument(instruments[channel]);
            e->set_track(voice);
            e->set_qtime(rel_start);
            e->set_is_drum(channel == DRUM_CHANNEL);
            e->set_bar(start_bar);

            // add offset
            events.push_back( new midi::Event);
            e = events.back();
            e->set_time((end * ec->resolution) / TPQ);
            e->set_velocity(0);
            e->set_pitch(pitch);
            e->set_instrument(instruments[channel]);
            e->set_track(voice);
            e->set_qtime(rel_end);
            e->set_is_drum(channel == DRUM_CHANNEL);
            e->set_bar(end_bar);
          }
        }
      }

      if (midifile[track][event].isPatchChange()) {
        channel = midifile[track][event].getChannelNibble();
        instruments[channel] = (int)midifile[track][event][1];
      }      
    }
  }

  // sort the events
  std::sort(events.begin(), events.end(), sort_events);

  int ntracks = track_map.size();
  std::vector<std::vector<midi::Bar*>> bars(ntracks, std::vector<midi::Bar*>(barlines.size()));
  for (int i=0; i<ntracks; i++) {
    midi::Track *t = p->add_tracks();
    t->set_is_drum( (std::get<1>(rev_track_map[i]) == DRUM_CHANNEL) );
    t->set_instrument( std::get<2>(rev_track_map[i]) ); // inst in rev_track_map
    int j = 0;
    for (const auto kv : barlines) {
      midi::Bar *b = t->add_bars();
      b->set_is_four_four( barlengths[j] == (TPQ*4) );
      b->set_internal_has_notes( false );
      b->set_time( kv.first * ec->resolution / TPQ );
      b->set_internal_beat_length( (double)barlengths[j] / TPQ );
      bars[i][j] = b;
      j++;
    }
  }

  int n = 0;
  for (const auto event : events) {
    midi::Event *e = p->add_events();
    e->CopyFrom(*event);
    bars[event->track()][event->bar()]->add_events(n);
    if (e->velocity() > 0) {
      bars[event->track()][event->bar()]->set_internal_has_notes(true);
    }
    n++;
    delete event; // otherwise memory leak
  }

  // update meta-data about the piece
  update_valid_segments(p, ec);
  update_pitch_limits(p);
  update_note_density(p);
  update_polyphony(p);
  
  p->set_segment_length(ec->num_bars);
  p->set_resolution(ec->resolution);

  // add genre information if available
  if (genre_data) {
    for (const auto genre : (*genre_data)["msd_tagtraum_cd1"]) {
      p->add_msd_cd1( genre );
    } 
    for (const auto genre : (*genre_data)["msd_tagtraum_cd2"]) {
      p->add_msd_cd2( genre );
    }
    for (const auto genre : (*genre_data)["msd_tagtraum_cd2c"]) {
      p->add_msd_cd2c( genre );
    }
  }
}

// turn a piece into midi
// should barlines be added somewhere ???
void write_midi(midi::Piece *p, std::string &path) {

  smf::MidiFile outputfile;
  outputfile.absoluteTicks();
  outputfile.setTicksPerQuarterNote(p->resolution());
  outputfile.addTempo(0, 0, p->tempo());
  outputfile.addTrack(16); // ensure drum channel
  std::vector<int> current_inst(16,0);

  for (const auto event : p->events()) {

    int channel = event.track();
    if (event.is_drum()) {
      channel = DRUM_CHANNEL;
    }

    if (current_inst[event.track()] != event.instrument()) {
      outputfile.addPatchChange(
        event.track(), event.time(), channel, event.instrument());
      current_inst[event.track()] = event.instrument();
    }

    outputfile.addNoteOn(
      event.track(), // track
      event.time(), // time
      channel, // channel  
      event.pitch(), // pitch
      event.velocity()); // velocity (need some sort of conversion)
  }
  outputfile.sortTracks();         // make sure data is in correct order
  outputfile.write(path.c_str()); // write Standard MIDI File twinkle.mid
}

// =============================================================
// =============================================================
// =============================================================
// =============================================================
// =============================================================
// =============================================================

