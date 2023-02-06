#pragma once

#include <vector>
#include <string>

#include <google/protobuf/util/json_util.h>

#include "../protobuf/midi.pb.h"
#include "../midi_io.h"

// =====================================================================
// helpers

template<class T>
using matrix = std::vector<std::vector<T>>;

template<class T>
using tensor = std::vector<std::vector<std::vector<T>>>;

template<class T>
using tensorf = std::vector<std::vector<std::vector<std::vector<T>>>>;

/*

template<typename T>
vector<T> arange(T start, T stop, T step = 1) {
 std::vector<T> values;
  for (T value = start; value < stop; value += step)
    values.push_back(value);
  return values;
}

template<typename T>
vector<T> arange(T stop) {
  return arange(0, stop, 1);
}

struct RNG {
  int operator() (int n) {
    return std::rand() / (1.0 + RAND_MAX) * n;
  }
};

midi::Piece string_to_piece(string json_string) {
  midi::Piece x;
  google::protobuf::util::JsonStringToMessage(json_string.c_str(), &x);
  return x;
}

string piece_to_string(midi::Piece x) {
  string json_string;
  google::protobuf::util::MessageToJsonString(x, &json_string);
  return json_string;
}

int get_num_bars(midi::Piece *x) {
  if (x->tracks_size() == 0) {
    return 0;
  }
  set<int> lengths;
  for (const auto track : x->tracks()) {
    lengths.insert( track.bars_size() );
  }
  if (lengths.size() > 1) {
    throw std::runtime_error("Each track must have the same number of bars!");
  }
  return *lengths.begin();
}

*/

// =====================================================================
// piano roll computation

/*
vector<midi::Note> events_to_notes(midi::Piece *p,std::vector<int> track_nums,std::vector<int> bar_nums, bool include_onsetless=false) {
  midi::Event e;
 std::vector<midi::Note> notes;
  for (const auto track_num : track_nums) {
    map<int,int> onsets;
    int bar_start = 0;
    for (const auto bar_num : bar_nums) {
      const midi::Bar bar = p->tracks(track_num).bars(bar_num);
      for (auto event_id : bar.events()) {
        e = p->events(event_id);
        if (e.velocity() > 0) {
          if (p->tracks(track_num).type() == DRUM_TRACK) {
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
      }
      // move forward a bar
      bar_start += p->resolution() * bar.internal_beat_length(); 
    }
  }
  return notes;
}
*/

std::vector<std::vector<int>> to_pianoroll(midi::Piece *p, std::vector<int> tracks, std::vector<int> bars, int tick_bound=0) {
  if ((tracks.size() == 0) || (p->tracks_size() <= tracks[0])) {
    throw std::runtime_error("INVALID ARGS ...");
  }
  int max_tick = 0;
  for (const auto b : bars) {
    max_tick += p->tracks(tracks[0]).bars(b).internal_beat_length() * p->resolution();
  }
  if (tick_bound) {
    max_tick = std::min(tick_bound, max_tick);
  }
 std::vector<std::vector<int>> roll(max_tick,std::vector<int>(128,false));
  for (const auto track_num : tracks) {
   std::vector<midi::Note> notes = events_to_notes(p,{track_num},bars,NULL,true);
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

tensorf<int> piece_to_track_barrolls(midi::Piece *p) {
  tensorf<int> out;
  for (int track_num=0; track_num<p->tracks_size(); track_num++) {
    tensor<int> track_rolls;
    for (int bar_num=0; bar_num<p->tracks(track_num).bars_size(); bar_num++) {
      track_rolls.push_back( to_pianoroll(p, {track_num}, {bar_num}) );
    }
    out.push_back( track_rolls );
  }
  return out;
}

tensorf<int> piece_to_track_barrolls_py(std::string json_string) {
  midi::Piece p = string_to_piece(json_string);
  return piece_to_track_barrolls(&p);
}

std::vector<std::vector<std::vector<int>>> piece_to_barrolls(midi::Piece *p) {
 std::vector<std::vector<std::vector<int>>> rolls;
  for (int track_num=0; track_num<p->tracks_size(); track_num++) {
    for (int bar_num=0; bar_num<p->tracks(track_num).bars_size(); bar_num++) {
      rolls.push_back( to_pianoroll(p, {track_num}, {bar_num}) );
    }
  }
  return rolls;
}

std::vector<std::vector<std::vector<int>>> piece_to_barrolls_py(std::string json_string) {
  midi::Piece p = string_to_piece(json_string);
  return piece_to_barrolls(&p);
}

std::vector<std::vector<std::vector<int>>> piece_to_trackrolls(midi::Piece *p, int tick_bound) {
  int num_bars = get_num_bars(p);
 std::vector<std::vector<std::vector<int>>> rolls;
  for (int track_num=0; track_num<p->tracks_size(); track_num++) {
    rolls.push_back( 
      to_pianoroll(p, {track_num}, arange(num_bars), tick_bound) );
  }
  return rolls;
}

std::vector<std::vector<std::vector<int>>> piece_to_trackrolls_py(std::string json_string, int tick_bound) {
  midi::Piece p = string_to_piece(json_string);
  return piece_to_trackrolls(&p, tick_bound);
}

std::vector<midi::Note> piece_to_notes(midi::Piece *p) {
  return events_to_notes(p, arange(p->tracks_size()), arange(get_num_bars(p)));
}

std::vector<std::tuple<int,int,int>> piece_to_notes_py(std::string json_string) {
  // return (start, end, pitch) tuples
  midi::Piece p = string_to_piece(json_string);
 std::vector<midi::Note> notes = piece_to_notes(&p);
 std::vector<std::tuple<int,int,int>> py_notes;
  py_notes.reserve(notes.size());
  for (const auto note : notes) {
    py_notes.push_back( std::make_tuple(note.start(),note.end(),note.pitch()) );
  }
  return py_notes;
}

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