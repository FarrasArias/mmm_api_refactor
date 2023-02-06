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
#include "protobuf/util.h"
#include "enum/constants.h"
#include "enum/density.h"
#include "enum/track_type.h"
#include "enum/encoder_config.h"

#include "adjacent_range.h"

#include <google/protobuf/util/json_util.h>

// START OF NAMESPACE
namespace mmm {

// =============================================================
// =============================================================
// =============================================================
static const int DRUM_CHANNEL = 9;

#define QUIET_CALL(noisy) { \
    std::cout.setstate(std::ios_base::failbit);\
    std::cerr.setstate(std::ios_base::failbit);\
    (noisy);\
    std::cout.clear();\
    std::cerr.clear();\
}

int quantize_beat(double x, double TPQ, double SPQ, double cut=.5) {
  return (int)((x / TPQ * SPQ) + (1.-cut)) * (TPQ / SPQ);
}

int quantize_second(double x, double spq, double ticks, double steps_per_second, double cut=.5) {
  return (int)((x / ticks * spq * steps_per_second) + (1.-cut));
}


bool event_comparator(const midi::Event a, const midi::Event b) { 
  if (a.time() != b.time()) {
    return a.time() < b.time();
  }
  if (std::min(a.velocity(),1) != std::min(b.velocity(),1)) {
    return std::min(a.velocity(),1) < std::min(b.velocity(),1);
  }
  return a.pitch() < b.pitch();
}


// =============================================================
// =============================================================
// =============================================================
// =============================================================
// =============================================================
// =============================================================

using TT_VOICE_TUPLE = std::tuple<int,int,int,int>; // CHANGE

// currently we don't do quantization
// this can be done elsewhere

class PH {
public:
  PH(std::string filepath, midi::Piece *piece, EncoderConfig *config) {
    parse(filepath, piece, config);
  }
  EncoderConfig *ec;
  int track_count;
  int TPQ;
  int SPQ;
  int current_track;
  int max_tick;
  smf::MidiEvent *mevent;
  std::map<TT_VOICE_TUPLE,int> track_map;
  std::map<int,TT_VOICE_TUPLE> rev_track_map;
  std::map<int,std::tuple<int,int,int>> timesigs;
  std::map<int,std::tuple<int,int,int,int>> bars; // value is (beatlength,count,num,dem)
  std::vector<std::vector<midi::Event>> events; // events split into tracks
  std::array<int,64> instruments; // instruments on each channel

  std::map<TT_VOICE_TUPLE,std::array<int,2>> event_counts;


  void parse(std::string filepath, midi::Piece *piece, EncoderConfig* config) {

    smf::MidiFile mfile;
    QUIET_CALL(mfile.read(filepath));
    mfile.makeAbsoluteTicks();
    mfile.linkNotePairs();
    track_count = mfile.getTrackCount();
    ec = config;  
    max_tick = 0;
    current_track = 0;
    TPQ = mfile.getTPQ();
    if (ec->unquantized) {
      SPQ = TPQ;
    }
    else {
      SPQ = ec->resolution;
    }

    piece->set_resolution(SPQ);

    // extract information from the midi file
    for (int track=0; track<track_count; track++) {
      current_track = track;
      std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
      for (int event=0; event<mfile[track].size(); event++) { 
        mevent = &(mfile[track][event]);
        if (mevent->isPatchChange()) {
          handle_patch_message(mevent);
        }
        else if (mevent->isTimeSignature()) {
          handle_time_sig_message(mevent);
        }
        else if (mevent->isTempo()) {
          piece->set_tempo(mevent->getTempoBPM());
        }
        else if (mevent->isNoteOn() || mevent->isNoteOff()) {
          handle_note_message(mevent);
        }
      }
    }

    if (TPQ < SPQ) {
      throw std::runtime_error("MIDI FILE HAS INVALID TICKS PER QUARTER.");
    }

    if (max_tick <= 0) {
      throw std::runtime_error("MIDI FILE HAS NO NOTES");
    }

    // add a timesig at beginning and end
    // and then make a mapping from tick to bar_number and bar_length
    int count = 0;
    if (timesigs.find(0) == timesigs.end()) {
      timesigs[0] = std::make_tuple(TPQ*4,4,4); // assume 4/4
    }
    // if we do max_tick + TPQ instead we end up with an extra bar
    timesigs[max_tick] = std::make_tuple(0,0,0); // no bar length
    for (const auto &p : make_adjacent_range(timesigs)) {
      if (std::get<0>(p.first.second) > 0) {
        for (int t=p.first.first; t<p.second.first; t+=std::get<0>(p.first.second)) {
          auto ts = p.first.second;
          bars[t] = std::make_tuple(std::get<0>(ts), count, std::get<1>(ts), std::get<2>(ts));
          count++;
        }
      }
    }

    // construct the piece
    midi::Track *track = NULL;
    midi::Bar *bar = NULL;
    midi::Event *event = NULL;

    for (int track_num=0; track_num<events.size(); track_num++) {

      // sort the events in each track
      // at this point ticks are still absolute
      std::sort(events[track_num].begin(), events[track_num].end(), event_comparator);

      // add track and track metadata
      track = piece->add_tracks();
      track->set_instrument( std::get<2>(rev_track_map[track_num]) );
      track->set_track_type( 
        (midi::TRACK_TYPE)std::get<3>(rev_track_map[track_num]) );
      //track->set_is_drum( is_drum_track(std::get<3>(rev_track_map[track_num])) );

      // apply force monophonic to bass track
      if (track->track_type() == midi::OPZ_BASS_TRACK) {
        std::vector<midi::Event> mono_events = force_monophonic(events[track_num]);
        std::sort(mono_events.begin(), mono_events.end(), event_comparator);
        events[track_num] = mono_events;
      }

      // add bars and bar metadata
      for (const auto &bar_info : bars) {
        bar = track->add_bars();
        bar->set_internal_beat_length( std::get<0>(bar_info.second) / TPQ );
        bar->set_ts_numerator( std::get<2>(bar_info.second) );
        bar->set_ts_denominator( std::get<3>(bar_info.second) );
      }

      // add events
      for (int j=0; j<events[track_num].size(); j++) {
        int velocity = events[track_num][j].velocity();
        int tick = events[track_num][j].time();
        auto bar_info = get_bar_info( tick, velocity>0 );

        bar = track->mutable_bars( std::get<2>(bar_info) ); // bar_num
        
        bar->add_events( piece->events_size() );
        event = piece->add_events();
        event->CopyFrom( events[track_num][j] );

        int rel_tick = round((double)(tick - std::get<0>(bar_info)) / TPQ * SPQ);
        event->set_time( rel_tick ); // relative
      }
    }

    // remap track types based on max polyphony
    if (ec->te) {
      update_max_polyphony(piece);
      for (int track_num=0; track_num<piece->tracks_size(); track_num++) {
        midi::Track *track = piece->mutable_tracks(track_num);
        int max_polyphony = track->internal_features(0).max_polyphony();
        midi::TRACK_TYPE tt = track->track_type();
        if (tt == midi::AUX_INST_TRACK) {
          if (max_polyphony <= 2) {
            track->add_internal_train_types( midi::OPZ_ARP_TRACK );
          }
          if (max_polyphony <= 3) { // previous was else if ????
            track->add_internal_train_types( midi::OPZ_LEAD_TRACK );
          }
          if (max_polyphony <= 4) {
            track->add_internal_train_types( midi::OPZ_CHORD_TRACK );
          }
          else {
            track->add_internal_train_types( midi::AUX_INST_TRACK ); // unused
          }
        }
        else if ((tt==midi::OPZ_KICK_TRACK) && (max_polyphony > 2)) {
          track->add_internal_train_types(    
            midi::OPZ_INVALID_DRUM_TRACK );
        }
        else if ((tt==midi::OPZ_SNARE_TRACK) && (max_polyphony > 2)) {
          track->add_internal_train_types( 
            midi::OPZ_INVALID_DRUM_TRACK );
        }
        else if ((tt==midi::OPZ_HIHAT_TRACK) && (max_polyphony > 2)) {
          track->add_internal_train_types( 
            midi::OPZ_INVALID_DRUM_TRACK );
        }
        else if ((tt==midi::OPZ_BASS_TRACK) && (max_polyphony > 1)) {
          track->add_internal_train_types( 
            midi::OPZ_INVALID_BASS_TRACK );
        }
        else if ((tt==midi::OPZ_SAMPLE_TRACK) && (max_polyphony > 2)) {
          track->add_internal_train_types( 
            midi::OPZ_INVALID_SAMPLE_TRACK );
        }
        else {
          track->add_internal_train_types( tt );
        }
      }
    }

    // update density information
    //update_note_density(piece);

    // print summary
    //for (const auto kv : event_counts) {
    //  std::cout << kv.second[0] << " onsets, " << kv.second[1] << " offsets" << std::endl;
    //}

  }

  TT_VOICE_TUPLE infer_voice(int track, int channel, int inst, int pitch, EncoderConfig *ec) {
    // special case for opz
    if (ec->te) {
      int track_type = (int)TE_INST_MAP[inst];
      if (channel == DRUM_CHANNEL) {
        track_type = (int)TE_DRUM_MAP[pitch];
      }
      return std::make_tuple(track,channel,inst,track_type);
    }
    // typically there are just two types of tracks DRUM and pitched instruments
    int track_type = midi::STANDARD_TRACK;
    if (channel == DRUM_CHANNEL) {
      track_type = midi::STANDARD_DRUM_TRACK;
    }
    return std::make_tuple(track,channel,inst,track_type);
  }

  std::tuple<int,int,int> get_bar_info(int tick, bool is_onset) {
    // returns bar_start, bar_length, bar_num tuple
    auto it = bars.upper_bound(tick);
    if (it == bars.begin()) {
      //cout << "TICK : " << tick << " - " << is_onset << std::endl;
      throw std::runtime_error("CAN'T GET BAR INFO FOR TICK!");
    }
    it = prev(it);
    if ((it->first == tick) && (!is_onset)) {
      // if the note is an offset and the time == the start of the bar
      // push it back to the previous bar
      if (it == bars.begin()) {
        //cout << "TICK : " << tick << " - " << is_onset << std::endl;
        throw std::runtime_error("CAN'T GET BAR INFO FOR TICK!");
      }
      it = prev(it);
    }
    return std::make_tuple(it->first, std::get<0>(it->second), std::get<1>(it->second));
  }

  void handle_patch_message(smf::MidiEvent *mevent) {
    int channel = mevent->getChannelNibble();
    instruments[channel] = (int)((*mevent)[1]);
  }

  void handle_time_sig_message(smf::MidiEvent *mevent) {
    int numerator = (*mevent)[3];
    int denominator = 1<<(*mevent)[4];
    int barlength = (double)(TPQ * 4 * numerator / denominator);
    if (barlength >= 0) {
      //timesigs[mevent->tick] = barlength;
      timesigs[mevent->tick] = std::make_tuple(barlength, numerator, denominator);
    }
  }

  bool is_event_offset(smf::MidiEvent *mevent) {
    return ((*mevent)[2]==0) || (mevent->isNoteOff());
  }

  void add_event(TT_VOICE_TUPLE &vtup, int tick, int pitch, int velocity) {
    midi::Event event;
    event.set_time( tick );
    //event.set_qtime( tick / (TPQ/SPQ) ); // we need this for force mono
    event.set_pitch( pitch );
    event.set_velocity( velocity );
    events[track_map[vtup]].push_back( event );
  }

  void handle_note_message(smf::MidiEvent *mevent) {
    int channel = mevent->getChannelNibble();
    int pitch = (int)(*mevent)[1];
    int velocity = (int)(*mevent)[2];

    if ((!mevent->isLinked()) && (channel != 9)) {
      // we do not include unlinked notes unless they are drum
      return;
    }

    if (mevent->isNoteOff()) {
      velocity = 0; // sometimes this is not the case
    }

    smf::MidiEvent *linked_event = mevent->getLinkedEvent();

    int tick = mevent->tick;
    if (!ec->unquantized) {
      tick = quantize_beat(mevent->tick, TPQ, SPQ);
    }

    bool is_offset = is_event_offset(mevent);

    // ignore note offsets at start of file
    if (is_offset && (tick==0)) {
      return;
    }

    // ignore double offset notes
    //if (is_offset && is_linked_offset) {
    //  return;
    //}

    // ignore notes of no length
    //if (abs(linked_tick - tick)==0) {
    //  return;
    //}

    TT_VOICE_TUPLE vtup = infer_voice(
      current_track,channel,instruments[channel],pitch,ec);
    
    // something here ...
    auto it = event_counts.find(vtup);
    if (it == event_counts.end()) {
      event_counts[vtup] = {0,0};
    }
    if (is_offset) {
      event_counts[vtup][1]++;
    }
    else {
      event_counts[vtup][0]++;
    }

    // update track map
    if (track_map.find(vtup) == track_map.end()) {
      int current_size = track_map.size();
      track_map[vtup] = current_size;
      rev_track_map[current_size] = vtup;
      events.push_back( std::vector<midi::Event>() );
    }

    // make all drum notes really short
    if (channel == 9) {
      if (!is_offset) {
        add_event(vtup, tick, pitch, velocity);
        add_event(vtup, tick + (TPQ/SPQ), pitch, 0);
      }
    }
    else {
      add_event(vtup, tick, pitch, velocity);
    }

    max_tick = std::max(max_tick, mevent->tick);
  }
};

// old signature
//void parse_new(std::string filepath, midi::Piece *p, EncoderConfig *ec, std::map<std::string,std::vector<std::string>> *genre_data=NULL) {

void parse_new(std::string filepath, midi::Piece *p, EncoderConfig *ec, std::map<std::string,std::vector<std::string>> *genre_data=NULL) {
  PH ph(filepath, p, ec);
}

void parse_te(std::string filepath, midi::Piece *p, EncoderConfig *ec) {
  PH ph(filepath, p, ec);
}

// =============================================================
// =============================================================
// =============================================================
// =============================================================
// =============================================================
// =============================================================


// turn a piece into midi
// should barlines be added somewhere ???
void write_midi(midi::Piece *p, std::string &path, int single_track=-1) {

  if (p->tracks_size() >= 15) {
    throw std::runtime_error("TOO MANY TRACKS FOR MIDI OUTPUT");
  }

  //set_beat_lengths(p); // make sure they exist

  // we should find a way not to output so many tracks

  smf::MidiFile outputfile;
  outputfile.absoluteTicks();
  outputfile.setTicksPerQuarterNote(p->resolution());
  outputfile.addTempo(0, 0, p->tempo());
  outputfile.addTrack(16); // ensure drum channel

  int track_num = 0;
  for (const auto track : p->tracks()) {
    if ((single_track < 0) || (track_num == single_track)) {
      int bar_start_time = 0;
      int patch = track.instrument();
      int channel = SAFE_TRACK_MAP[track_num];
      if (is_drum_track(track.track_type())) {
        channel = DRUM_CHANNEL;
      }
      outputfile.addPatchChange(channel, 0, channel, patch);

      for (const auto bar : track.bars()) {
        for (const auto event_index : bar.events()) {
          const midi::Event e = p->events(event_index);
          //std::cout << "WRITING :: " << bar_start_time + e.time() << " " << e.pitch() << " " << e.velocity() << std::endl;
          outputfile.addNoteOn(
            channel, // same as channel
            bar_start_time + e.time(), // time
            channel, // channel  
            e.pitch(), // pitch
            e.velocity()); // velocity (need some sort of conversion)
        }
        bar_start_time += bar.internal_beat_length() * p->resolution();
      }
    }
    track_num++;
  }

  outputfile.sortTracks();         // make sure data is in correct order
  outputfile.write(path.c_str()); // write Standard MIDI File twinkle.mid
}

}
// END OF NAMESPACE