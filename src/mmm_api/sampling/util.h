// tools to help with sampling

#pragma once

#include <string>
#include <vector>
#include <tuple>

#include "../protobuf/midi.pb.h"
#include "../protobuf/util.h"
#include "../random.h"

#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/message_differencer.h>

namespace mmm {

std::vector<midi::TRACK_TYPE> OPZ_TRACK_TYPES = {
  midi::OPZ_KICK_TRACK,
  midi::OPZ_SNARE_TRACK,
  midi::OPZ_HIHAT_TRACK,
  midi::OPZ_SAMPLE_TRACK,
  midi::OPZ_BASS_TRACK,
  midi::OPZ_LEAD_TRACK,
  midi::OPZ_ARP_TRACK,
  midi::OPZ_CHORD_TRACK
};


std::vector<midi::TRACK_TYPE> OPZ_DRUM_TRACK_TYPES = {
  midi::OPZ_SNARE_TRACK
};

std::vector<midi::TRACK_TYPE> STANDARD_TRACK_TYPES = {
  midi::STANDARD_TRACK,
  midi::STANDARD_DRUM_TRACK
};

using BOOL_MATRIX = std::vector<std::vector<bool>>;

//void validate_insts(std::vector<std::string> &insts) {
//  for (const auto inst : insts) {
//    if (GM_KEYS.find(inst) == GM_KEYS.end()) {
//      throw std::runtime_error("INVALID INSTRUMENT.");
//    }
//  }
//}

// quick way to set resample bars
void set_selected_bars(midi::Status *status, std::vector<std::vector<bool>> bars) {
  int track_num = 0;
  for (const auto track : status->tracks()) {
    int bar_num = 0;
    for (const auto bar : track.selected_bars()) {
      if ((track_num < bars.size()) && (bar_num < bars[track_num].size())) {
        status->mutable_tracks(track_num)->set_selected_bars(
          bar_num, bars[track_num][bar_num]);
      }
      bar_num++;
    }
    track_num++;
  }
}

// set one or more tracks to reamplse
void set_resample_tracks(midi::Status *status, std::vector<int> track_nums, bool value=true) {
  for (const auto track : track_nums) {
    midi::StatusTrack *strack = status->mutable_tracks(track);
    for (int bar_num=0; bar_num<strack->selected_bars_size(); bar_num++) {
      strack->set_selected_bars(bar_num, value);
    }
    strack->set_autoregressive(value);
  }
}

BOOL_MATRIX random_boolean_matrix(int rows, int columns, std::mt19937 *engine) {
  BOOL_MATRIX m;
  for (int i=0; i<rows; i++) {
    std::vector<bool> row;
    for (int j=0; j<columns; j++) {
      row.push_back( random_on_unit(engine) < .5 );
    }
    m.push_back( row );
  }
  return m;
}

BOOL_MATRIX ones_boolean_matrix(int rows, int columns) {
  return BOOL_MATRIX(rows, std::vector<bool>(columns,true));
}

// quick way to randomly resample bars
//void set_random_selected_bars(midi::Status *status) {


// minimal example of how to add a track
void add_blank_track(midi::Piece *piece, int num_bars, int instrument, midi::TRACK_TYPE tt, std::vector<std::tuple<int,int>> &time_sigs) {
  if ((time_sigs.size() != 0) && (time_sigs.size() != num_bars)) {
    throw std::invalid_argument("TIME SIGS NEEDS TO BE SAME LENGTH AS BARS");
  }
  piece->set_resolution(12); // make sure
  midi::Track *t = piece->add_tracks();
  t->set_instrument( instrument );
  t->set_track_type( (midi::TRACK_TYPE)tt );
  for (int i=0; i<num_bars; i++) {
    midi::Bar *b = t->add_bars();
    if (time_sigs.size()) {
      b->set_ts_numerator( std::get<0>(time_sigs[i]) );
      b->set_ts_denominator( std::get<1>(time_sigs[i]) );
    }
    else {
      b->set_ts_numerator( 4 );
      b->set_ts_denominator( 4 );
    }
  }
}

// add tracks with random notes
void add_random_note_track(midi::Piece *piece, int num_bars, int instrument, midi::TRACK_TYPE tt, std::vector<std::tuple<int,int>> &timesigs, std::mt19937 *engine) {

  add_blank_track(piece, num_bars, instrument, tt, timesigs);
  midi::Track *t = piece->mutable_tracks(piece->tracks_size() - 1);
  for (int bar_num=0; bar_num<t->bars_size(); bar_num++) {
    midi::Bar *b = t->mutable_bars(bar_num);
    for (int i=0; i<0; i++) {
      float beat_length = (double)b->ts_numerator() / b->ts_denominator() * 4;
      int pitch = random_on_range(128, engine);
      int max_time = beat_length * piece->resolution();
      int start = random_on_range(max_time - 1, engine);
      int end = start + random_on_range(max_time - start, engine) + 1;
      int num_events = piece->events_size();
      
      midi::Event *e = piece->add_events();
      e->set_pitch( pitch );
      e->set_time( start );
      e->set_velocity( 8 + random_on_range(120, engine) );

      e = piece->add_events();
      e->set_pitch( pitch );
      e->set_time( end );
      e->set_velocity( 0 );

      if (start == end) {
        throw std::runtime_error("START AND END ARE EQUIVALENT");
      }

      b->add_events( num_events );
      b->add_events( num_events + 1);
    }
  }
}

// add multiple tracks with random notes
void add_random_note_tracks(midi::Piece *piece, int num_bars, std::vector<int> instruments, std::vector<midi::TRACK_TYPE> tts, std::vector<std::tuple<int,int>> &timesigs, std::mt19937 *engine) {
  if (instruments.size() != tts.size()) {
    throw std::runtime_error("MUST HAVE SAME NUMBER OF INSTRUMENTS AND TRACK_TYPES");
  }
  for (int i=0; i<instruments.size(); i++) {
    add_random_note_track(piece, num_bars, instruments[i], tts[i], timesigs, engine);
  }
  sort_piece_events(piece);
}

midi::Piece random_piece(int num_tracks, int num_bars, bool opz, std::vector<std::tuple<int,int>> &timesigs, std::mt19937 *engine) {
  midi::Piece p;
  p.set_tempo(120);
  std::vector<int> instruments;
  std::vector<midi::TRACK_TYPE> tts;
  for (int i=0; i<num_tracks; i++) {
    instruments.push_back( random_on_range(128, engine) );
    if (opz) {
      tts.push_back( random_element(OPZ_TRACK_TYPES, engine) );
    }
    else {
      // one drum track per piece
      if (i == 0) {
        tts.push_back( midi::STANDARD_DRUM_TRACK );
      }
      else {
        tts.push_back( midi::STANDARD_TRACK );
      }
      //tts.push_back( random_element(STANDARD_TRACK_TYPES, engine) );
    }
  }
  add_random_note_tracks(&p, num_bars, instruments, tts, timesigs, engine);
  return p;
}

// set the polyphony hard limit on all tracks
void set_polyphony_hard_limit(midi::Status *status, int limit) {
  for (int track_num=0; track_num<status->tracks_size(); track_num++) {
    midi::StatusTrack *track = status->mutable_tracks(track_num);
    track->set_polyphony_hard_limit(limit);
  }
}

// this is a helper function that converts a midi::Piece into a midi::Status
void status_from_piece(midi::Piece *piece, midi::Status *status) {
  status->Clear();
  int track_num = 0;
  for (const auto track : piece->tracks()) {
    midi::StatusTrack *strack = status->add_tracks();
    strack->set_track_id( track_num );
    strack->set_track_type( track.track_type() );
    strack->set_density( midi::DENSITY_ANY ); //
    strack->set_instrument(gm_inst_to_string(track.track_type(),track.instrument()));
    strack->set_polyphony_hard_limit( 10 );
    strack->set_temperature( 1. );
    for (int i=0; i<track.bars_size(); i++) {
      strack->add_selected_bars( false );
    }
    track_num++;
  }
}

void autoregressive_inputs(std::vector<midi::GM_TYPE> &insts, int nbars, midi::Piece *piece, midi::Status *status) {
  
  //validate_insts(insts);
  piece->Clear();
  status->Clear();

  piece->set_resolution(12);
  piece->set_tempo(120);

  int track_num = 0;
  for (const auto inst : insts) {

    midi::TRACK_TYPE tt = GM_TO_TRACK_TYPE[inst];

    midi::StatusTrack *strack = status->add_tracks(); 
    strack->set_track_id( track_num );
    strack->set_track_type( (midi::TRACK_TYPE)tt );
    strack->set_instrument( inst );
    for (int i=0; i<nbars; i++) {
      strack->add_selected_bars( true );
    }
    strack->set_density( midi::DENSITY_ANY );
    strack->set_autoregressive( true );
    strack->set_ignore( false );

    midi::Track *track = piece->add_tracks();
    //track->set_is_drum( is_drum_track(tt) );
    track->set_instrument( 0 ); // just a stand in value
    track->set_track_type( (midi::TRACK_TYPE)tt );

    track_num++;
  }
}

// ===================================================================
// check which bars are identical

void print_event(midi::Event x) {
  std::cout << "EVENT(pitch=" << x.pitch() << ",time=" << x.time() << ",velocity=" << x.velocity() << ")" << std::endl;
  /*
  std::ostringstream buffer;
  buffer << "EVENT(pitch=" << x.pitch() << ",time=" << x.time() << ",velocity=" << x.velocity() << ")";
  throw std::invalid_argument(buffer.str());
  */
} 

bool bar_is_identical(const midi::Bar &a, midi::Piece *ap, const midi::Bar &b, midi::Piece *bp, bool verbose=true, bool allow_offsets=false) {
  //if (!allow_offsets) {
    if (a.events_size() != b.events_size()) {
      if (verbose) {
        std::cout << "NUMBER OF EVENTS DIFFERS :: " << a.events_size() << " != " << b.events_size() << std::endl;
        //for (auto event : a.events()) {
        //  print_event(ap->events(event));
        //}
      }
      return false;
    }
    for (int i=0; i<a.events_size(); i++) {
      const midi::Event ae = ap->events(a.events(i));
      const midi::Event be = bp->events(b.events(i));
      google::protobuf::util::MessageDifferencer md;
      if (!md.Equals(ae, be)) {
        print_event(ae);
        print_event(be);
        return false;
      }
    }
    return true;
  //}
  /*
  else {
    // only consider onsets
    std::vector<midi::Event> aonsets = ap->events()
  }
  */
}

bool bars_are_identical(midi::Piece *a, midi::Piece *b, std::vector<std::vector<bool>> selected_bars) {
  sort_piece_events(a);
  sort_piece_events(b);
  bool result = true;
  for (int track_num=0; track_num<selected_bars.size(); track_num++) {
    for (int bar_num=0; bar_num<selected_bars[track_num].size(); bar_num++) {
      if (!selected_bars[track_num][bar_num]) {
        if (!bar_is_identical(a->tracks(track_num).bars(bar_num), a, b->tracks(track_num).bars(bar_num), b)) {
          std::cout << "ERROR ON TRACK " << track_num << " BAR " << bar_num << std::endl;
          result = false;
        }
      }
    }
  }
  return result;
}

bool tracks_are_different(midi::Piece *a, midi::Piece *b, std::vector<std::vector<bool>> selected_bars) {
  sort_piece_events(a);
  sort_piece_events(b);
  for (int track_num=0; track_num<selected_bars.size(); track_num++) {
    bool selected = false;
    int num_diff = 0;
    for (int bar_num=0; bar_num<selected_bars[track_num].size(); bar_num++) {
      if (selected_bars[track_num][bar_num]) {
        selected = true;
        if (!bar_is_identical(a->tracks(track_num).bars(bar_num), a, b->tracks(track_num).bars(bar_num), b, false)) {
          num_diff += 1;
        }
      }
    }
    if ((selected) && (num_diff == 0)) {
      return false;
    }
  }
  return true;
}

bool tracks_are_different(midi::Piece *a, midi::Piece *b, int i, int j) {
  int num_diff = 0;
  for (int bar_num=0; bar_num<a->tracks(i).bars_size(); bar_num++) {
    if (!bar_is_identical(a->tracks(i).bars(bar_num), a, b->tracks(j).bars(bar_num), b, false)) {
      num_diff += 1;
    }
  }
  return num_diff > 0;
}

bool tracks_are_identical(midi::Piece *a, midi::Piece *b, bool pretrain_map=false) {
  if (a->tracks_size() != b->tracks_size()) {
    return false;
  }
  for (int track_num=0; track_num<a->tracks_size(); track_num++) {
    midi::Track at = a->tracks(track_num);
    midi::Track bt = b->tracks(track_num);
    if (pretrain_map) {
      auto end = PRETRAIN_GROUPING_V2.end();
      auto ainst = PRETRAIN_GROUPING_V2.find(at.instrument());
      auto binst = PRETRAIN_GROUPING_V2.find(bt.instrument());
      if ((ainst == end) || (binst == end) || (ainst->second != binst->second)) {
        std::cout << "INSTS DON'T MATCH WITH PRETRAIN GROUPINGS :: " << at.instrument() << " != " << bt.instrument() << " on track " << track_num << std::endl;
        return false;
      }
    }
    else if (at.instrument() != bt.instrument()) {
      std::cout << "INSTRUMENTS DON'T MATCH :: " << at.instrument() << " != " << bt.instrument() << " on track " << track_num << std::endl;
      return false;
    }
    if (at.track_type() != bt.track_type()) {
      std::cout << "TRACKS DON'T MATCH :: " << at.track_type() << " != " << bt.track_type() << " on track " << track_num << std::endl;
      return false;
    }
  }
  return true;
}

// ===================================================================
// python wrappers

std::string status_from_piece_py(std::string &piece_str) {
  midi::Piece p = string_to_piece(piece_str);
  midi::Status s;
  status_from_piece(&p, &s);
  return status_to_string(s);
}

/*
std::tuple<std::string,std::string> autoregressive_inputs_py(std::vector<std::string> &insts, int nbars) {
  midi::Piece p;
  midi::Status s;
  autoregressive_inputs(insts, nbars, &p, &s);
  return std::make_tuple(piece_to_string(p), status_to_string(s));
}
*/

}