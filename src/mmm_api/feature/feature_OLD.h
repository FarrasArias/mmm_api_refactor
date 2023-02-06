#pragma once

#include <vector>
#include <string>

#include <google/protobuf/util/json_util.h>

#include "../protobuf/midi.pb.h"
#include "../protobuf/util.h"

// this is slightly different version

std::vector<midi::Note> track_events_to_notes_dev(midi::Piece *p, int track_num, std::vector<int> &bar_nums, bool no_drum_offsets=false) {
  midi::Event e;
  std::map<int,int> onsets;
  std::vector<midi::Note> notes;
  int bar_start = 0;
  for (const auto bar_num : bar_nums) {
    const midi::Bar bar = p->tracks(track_num).bars(bar_num);
    for (auto event_id : bar.events()) {
      e = p->events(event_id);
      if (e.velocity() > 0) {
        if (is_drum_track(p->tracks(track_num).type()) && no_drum_offsets) {
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
    }
    // move forward a bar
    bar_start += p->resolution() * bar.internal_beat_length(); 
  }
  return notes;
}


int get_max_tick(midi::Piece *p) {
  if (p->tracks_size() == 0) {
    return 0;
  }
  int max_tick = 0;
  for (const auto bar : p->tracks(0).bars()) {
    max_tick += bar.internal_beat_length() * p->resolution();
  }
  return max_tick;
}

// how to handle multiple tracks
/*
std::vector<std::vector<int>> to_pianoroll(std::string json_string, int max_tick) {
  midi::Piece p = string_to_piece(json_string);
  //int max_tick = get_max_tick(&p);
  std::vector<std::vector<int>> roll(max_tick, std::vector<int>(128,false));
  for (int track_num=0; track_num<p.tracks_size(); track_num++) {
    std::vector<midi::Note> notes = track_events_to_notes(&p,track_num,NULL,true);
    for (const auto note : notes) {
      for (int t=note.start(); t<note.end(); t++) {
        if ((t >= 0) && (t < max_tick)) {
          roll[t][note.pitch()] = true;
        }
      }
    }
  }
  return roll;
}
*/

// 1) sum of matrix
// 2) sorted list of the sums on each pitch. If the distribution
// is off they can't match (this is a lower bound on distance)
// 3) sum of pitches at each timestep (also lower bound on distance)
// how much work does this eliminate ?

std::vector<std::vector<int>> to_pianoroll(midi::Piece *p, std::vector<int> tracks, std::vector<int> bars) {
  int max_tick = p->resolution() * 4 * bars.size();
  std::vector<std::vector<int>> roll(max_tick, std::vector<int>(128,false));
  for (const auto track_num : tracks) {
    std::vector<midi::Note> notes = track_events_to_notes_dev(p,track_num,bars,true);
    for (const auto note : notes) {
      for (int t=note.start(); t<note.end(); t++) {
        if ((t >= 0) && (t < max_tick)) {
          roll[t][note.pitch()] = true;
        }
      }
    }
  }
  return roll;
}

std::vector<std::vector<int>> to_pianoroll_py(std::string json_string) {
  midi::Piece p = string_to_piece(json_string);
  return to_pianoroll(&p, arange(p.tracks_size()), arange(std::min(4,get_num_bars(&p))));
}

std::vector<std::vector<std::vector<int>>> piece_to_barrolls(midi::Piece *p, bool has_notes) {
  update_has_notes(p);
  std::vector<std::vector<std::vector<int>>> rolls;
  for (int track_num=0; track_num<p->tracks_size(); track_num++) {
    for (int bar_num=0; bar_num<p->tracks(track_num).bars_size(); bar_num++) {
      if ((!has_notes) || (p->tracks(track_num).bars(bar_num).internal_has_notes())) {
        rolls.push_back( to_pianoroll(p, {track_num}, {bar_num}) );
      }
    }
  }
  return rolls;
}

std::vector<std::vector<std::vector<int>>> piece_to_barrolls_py(std::string json_string, bool has_notes) {
  midi::Piece p = string_to_piece(json_string);
  return piece_to_barrolls(&p, has_notes);
}

// ============================================================================
// density

// ============================================================================
// quantized feaure

std::vector<double> piece_to_quantize_info(midi::Piece *p) {
  // 16th, 16th(trip), 32nd, 32nd(trip), 64th, 64th(trip) ...
  int onset_count = 0;
  //vector<int> levels = {4,6,8,12,16,24,32,48};
  std::vector<int> levels = {4,12,8,24,16,48,32,96};
  std::vector<double> counts(levels.size(),0);

  std::vector<int> tick_levels;
  for (int i=0; i<levels.size(); i++) {
    int res = p->resolution()/levels[i];
    if ((p->resolution() % levels[i]) != 0) {
      res = 0;
    }
    tick_levels.push_back( res );
  }

  for (const auto event : p->events()) {
    if (event.velocity() > 0) {
      for (int i=0; i<levels.size(); i++) {
        if (tick_levels[i] > 0) {
          counts[i] += (event.time() % tick_levels[i]) == 0;
        }
        else {
          counts[i] += 1;
        }
      }
      onset_count++;
    }
  }
  for (int i=0; i<levels.size(); i++) {
    if (counts[i] > 0) {
      counts[i] /= onset_count;
    }
  }
  return counts;
}

// ============================================================================

std::vector<std::tuple<int,int,int>> calc_feat_density(midi::Piece *x) {
  std::vector<std::tuple<int,int,int>> density;
  int num_notes;
  for (const auto track : x->tracks()) {
    int track_type = track.type();
    int inst = track.instrument();
    if (track_type == STANDARD_DRUM_TRACK) {
      inst = 0; // map all drums to same;
    }
    
    for (const auto bar : track.bars()) {
      num_notes = 0;
      for (const auto event_index : bar.events()) {
        if (x->events(event_index).velocity()) {
          num_notes++;
        }
      }
      if (track.train_types_size() > 0) {
        for (const auto tt : track.train_types()) {
          density.push_back(
            std::make_tuple(tt, inst, num_notes));
        }
      }
      else {
        density.push_back(
          std::make_tuple(track_type, inst, num_notes));
      }
    }
  }
  return density;
}

std::vector<double> calc_feat_test(midi::Piece *p) {
  return {0., 3.14};
}