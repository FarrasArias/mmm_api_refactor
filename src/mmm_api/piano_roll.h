/// single file that turns midi into a piano roll as quick as possible
#pragma once

#include <iostream>
#include <vector>
#include <tuple>
#include <array>
#include <map>
#include <set>

#include <iostream>
#include <fstream>
#include <sstream>

#include "../../midifile/include/Binasc.h"
#include "../../midifile/include/MidiFile.h"
#include "xxh/xxh3.h"

// START OF NAMESPACE
namespace mmm {

template<class T>
using vector = std::vector<T>;

template<class T>
using matrix = std::vector<std::vector<T>>;

template<class T>
using tensor = std::vector<std::vector<std::vector<T>>>;

template<class T>
using qtensor = std::vector<std::vector<std::vector<std::vector<T>>>>;

using hash_type = uint32_t;

static const int DRUM_CHAN = 9;

using TT_VOICE_TUPLE = std::tuple<int,int,int,int>;

#define QUIET_CALL(noisy) { \
    std::cout.setstate(std::ios_base::failbit);\
    std::cerr.setstate(std::ios_base::failbit);\
    (noisy);\
    std::cout.clear();\
    std::cerr.clear();\
}

int quantize(double x, double TPQ, double SPQ, double cut) {
  return (int)((x / TPQ * SPQ) + (1.-cut));
}

bool is_event_offset(smf::MidiEvent *mevent) {
  return ((*mevent)[2]==0) || (mevent->isNoteOff());
}

TT_VOICE_TUPLE infer_voice(int track, int channel, int inst, int pitch) {
  int track_type = midi::STANDARD_TRACK;
  if (channel == DRUM_CHAN) {
    track_type = midi::STANDARD_DRUM_TRACK;
  }
  return std::make_tuple(track,channel,inst,track_type);
}

tensor<int> fast_roll(std::string &filepath, int resolution, int nbars, int start) {
  
  smf::MidiFile mfile;
  QUIET_CALL(mfile.read(filepath));
  mfile.makeAbsoluteTicks();
  mfile.linkNotePairs();
  
  int track_count = mfile.getTrackCount();  

  int max_tick = 0;
  int current_track = 0;
  int TPQ = mfile.getTPQ();
  int SPQ = TPQ;
  if (resolution > 0) {
    SPQ = resolution;
  }

  smf::MidiEvent *mevent;
  std::array<int,64> instruments; // instruments on each channel
  std::map<int,int> timesigs; // store timesigs
  vector<std::tuple<int,int,int>> bars;

  std::map<TT_VOICE_TUPLE,int> track_map;
  std::map<int,TT_VOICE_TUPLE> rev_track_map;

  // extract information from the midi file
  for (int track=0; track<track_count; track++) {
    current_track = track;
    std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
    for (int event=0; event<mfile[track].size(); event++) { 
      mevent = &(mfile[track][event]);
      if (mevent->isTimeSignature()) {
        int top = (*mevent)[3];
        int bottom = 1<<(*mevent)[4];
        int barlength = (double)(TPQ * 4 * top / bottom);
        if (barlength >= 0) {
          timesigs[mevent->tick] = barlength;
        }
      }
      if (mevent->isNoteOff() || mevent->isNoteOn()) {
        int tick = mevent->tick;
        max_tick = std::max(max_tick, tick);
      }
    }
  }

  // add a timesig at beginning and end
  // and then make a mapping from tick to bar_number and bar_length
  int count = 0;
  if (timesigs.find(0) == timesigs.end()) {
    timesigs[0] = TPQ*4;
  }
  timesigs[max_tick + TPQ * 4] = 0; // no bar length

  for (const auto &p : make_adjacent_range(timesigs)) {
    if (p.first.second > 0) {
      for (int t=p.first.first; t<p.second.first; t+=p.first.second) {
        bars.push_back( std::make_tuple(t, p.first.second, count) );
        count++;
      }
    }
  }

  // then only grab the notes within a random bar
  // and turn them into a piano roll
  // pick a random bar
  int total_bars = bars.size();
  
  int index = 0;
  if (total_bars < nbars) {
    throw std::runtime_error("not enough bars");
  }
  else if (total_bars > nbars) {
    index = start % (total_bars - nbars);
  }

  if (nbars == 0) {
    // encode entire piece
    nbars = total_bars - 1;
    start = 0;
    index = 0;
  }

  int start_ts = std::get<0>(bars[index]);
  int end_ts = std::get<0>(bars[index + nbars]);

  if (resolution > 0) {
    start_ts = quantize(start_ts, TPQ, SPQ, .5);
    end_ts = quantize(end_ts, TPQ, SPQ, .5);
  }
  int ts = end_ts - start_ts; /// nbars;

  tensor<int> roll;
  if ((end_ts > start_ts) && (ts < 1000)) {

    for (int track=0; track<track_count; track++) {
      current_track = track;
      std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
      for (int event=0; event<mfile[track].size(); event++) { 
        mevent = &(mfile[track][event]);
        if (mevent->isPatchChange()) {
          int channel = mevent->getChannelNibble();
          instruments[channel] = (int)((*mevent)[1]);
        }
        else if (mevent->isNoteOn()) {
          int channel = mevent->getChannelNibble();
          int pitch = (int)(*mevent)[1];

          int start = mevent->tick;
          int end = 0;

          if (mevent->isLinked()) {
            smf::MidiEvent *levent = mevent->getLinkedEvent();
            end = levent->tick;
          }
          if (channel == DRUM_CHAN) {
            end = start + 1; // force uniform drum notes
          }
          if (resolution > 0) {
            start = quantize(start, TPQ, SPQ, .5);
            end = quantize(end, TPQ, SPQ, .5);
          }
          if ((end > start) && (end > 0) && (start >= 0)) {

            TT_VOICE_TUPLE vtup = infer_voice(
            current_track,channel,instruments[channel],pitch);

            // update track map
            if (track_map.find(vtup) == track_map.end()) {
              int current_size = track_map.size();
              track_map[vtup] = current_size;
              roll.push_back( matrix<int>(ts,vector<int>(128,0)) );
            }

            int track_num = track_map[vtup];

            for (int t=std::max(start,start_ts); t<std::min(end,end_ts); t++) {
              roll[track_num][t-start_ts][pitch] = 1;
            }
          }
        }
      }
    }
  }

  return roll;
}

// ========================================================
// implementation for hashing

qtensor<bool> fast_bar_roll(std::string &filepath, int resolution, int nbars, int start) {
  
  smf::MidiFile mfile;
  QUIET_CALL(mfile.read(filepath));
  mfile.makeAbsoluteTicks();
  mfile.linkNotePairs();
  
  int track_count = mfile.getTrackCount();  

  int max_tick = 0;
  int current_track = 0;
  int TPQ = mfile.getTPQ();
  int SPQ = TPQ;
  if (resolution > 0) {
    SPQ = resolution;
  }

  smf::MidiEvent *mevent;
  std::array<int,64> instruments; // instruments on each channel
  std::map<int,int> timesigs; // store timesigs
  vector<std::tuple<int,int,int>> bars;

  std::map<TT_VOICE_TUPLE,int> track_map;
  std::map<int,TT_VOICE_TUPLE> rev_track_map;

  // extract information from the midi file
  for (int track=0; track<track_count; track++) {
    current_track = track;
    std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
    for (int event=0; event<mfile[track].size(); event++) { 
      mevent = &(mfile[track][event]);
      if (mevent->isTimeSignature()) {
        int top = (*mevent)[3];
        int bottom = 1<<(*mevent)[4];
        int barlength = (double)(TPQ * 4 * top / bottom);
        if (barlength >= 0) {
          timesigs[mevent->tick] = barlength;
        }
      }
      if (mevent->isNoteOff() || mevent->isNoteOn()) {
        int tick = mevent->tick;
        max_tick = std::max(max_tick, tick);
      }
    }
  }

  // add a timesig at beginning and end
  // and then make a mapping from tick to bar_number and bar_length
  int count = 0;
  if (timesigs.find(0) == timesigs.end()) {
    timesigs[0] = TPQ*4;
  }
  timesigs[max_tick + TPQ * 4] = 0; // no bar length

  for (const auto &p : make_adjacent_range(timesigs)) {
    if (p.first.second > 0) {
      for (int t=p.first.first; t<p.second.first; t+=p.first.second) {
        bars.push_back( std::make_tuple(t, p.first.second, count) );
        count++;
      }
    }
  }

  // then only grab the notes within a random bar
  // and turn them into a piano roll
  // pick a random bar
  int total_bars = bars.size();
  
  int index = 0;
  if ((total_bars < nbars) || (total_bars == 0)) {
    throw std::runtime_error("not enough bars");
  }
  else if (total_bars > nbars) {
    index = start % (total_bars - nbars);
  }

  if (nbars == 0) {
    // encode entire piece
    nbars = total_bars - 1;
    start = 0;
    index = 0;
  }

  if (resolution > 0) {
    for (int i=0; i<bars.size(); i++) {
      int qed = quantize(std::get<0>(bars[i]), TPQ, SPQ, .5);
      bars[i] = std::make_tuple(qed, std::get<1>(bars[i]), std::get<2>(bars[i]));
    }
  }

  int start_ts = std::get<0>(bars[index]);
  int end_ts = std::get<0>(bars[index + nbars]);

  //std::cout << start_ts << " " << end_ts << std::endl;

  qtensor<bool> roll;
  if ((end_ts > start_ts) && ((end_ts - start_ts) < 50000)) {

    for (int track=0; track<track_count; track++) {
      current_track = track;
      std::fill(instruments.begin(), instruments.end(), 0); // zero instruments
      for (int event=0; event<mfile[track].size(); event++) { 
        mevent = &(mfile[track][event]);
        if (mevent->isPatchChange()) {
          int channel = mevent->getChannelNibble();
          instruments[channel] = (int)((*mevent)[1]);
        }
        else if (mevent->isNoteOn()) {
          int channel = mevent->getChannelNibble();
          int pitch = (int)(*mevent)[1];

          int start = mevent->tick;
          int end = 0;

          if (mevent->isLinked()) {
            smf::MidiEvent *levent = mevent->getLinkedEvent();
            end = levent->tick;
          }
          if (channel == DRUM_CHAN) {
            end = start + 1; // force uniform drum notes
          }
          if (resolution > 0) {
            start = quantize(start, TPQ, SPQ, .5);
            end = quantize(end, TPQ, SPQ, .5);
          }
          if ((end > start) && (end > 0) && (start >= 0)) {

            TT_VOICE_TUPLE vtup = infer_voice(
            current_track,channel,instruments[channel],pitch);

            // update track map
            if (track_map.find(vtup) == track_map.end()) {
              int current_size = track_map.size();
              track_map[vtup] = current_size;
              roll.push_back( 
                tensor<bool>(nbars,matrix<bool>(48,std::vector<bool>(128,0))));
            }

            int track_num = track_map[vtup];

            for (int t=std::max(start,start_ts); t<std::min(end,end_ts); t++) {
              int bar_num = 0;
              while ((bar_num < bars.size()) && (t-48) >= std::get<0>(bars[bar_num])) {
                bar_num++;
              }
              int btime = t-std::get<0>(bars[bar_num]);
              if ((btime >= 0) && (btime < 48) && (bar_num < nbars)) {
                roll[track_num][bar_num][btime][pitch] = true;
              }
            }
          }
        }
      }
    }
  }

  return roll;
}


// convert each column (i.e. single timestep)
// into two uint64 which we can hash
// then do min hash

// transpose
vector<uint64_t> transpose(vector<uint64_t> &bar, int t) {
  for (int i=0; i<(int)bar.size() / 2; i++) {
    if (t < 0) {
      bar[2*i] = (bar[2*i] >> t) | (bar[2*i+1] << (64-t));
      bar[2*i+1] = (bar[2*i+1] >> t);
    }
    else if (t > 0) {
      bar[2*i] = (bar[2*i] << t);
      bar[2*i+1] = (bar[2*i+1] << t) | (bar[2*i] >> (64-t));
    }
  }
  return bar;
}

vector<bool> bit_to_bool(vector<uint64_t> &x) {
  int n = (int)x.size() * 64;
  vector<bool> y(n,0);
  for (int i=0; i<n; i++) {
    y[i] = (x[i/64] >> (i%64)) & 1;
  }
  return y;
}

vector<uint64_t> bool_to_bit(vector<bool> &x) {
  int n = (int)x.size() / 64;
  vector<uint64_t> y(n,0);
  for (int i=0; i<x.size(); i++) {
    y[i/64] |= ((uint64_t)x[i] << (i%64));
  }
  return y;
}


tensor<uint64_t> fast_bit_roll64(std::string &filepath, int resolution, int nbars, int start) {
  qtensor<bool> roll = fast_bar_roll(filepath, resolution, nbars, start);
  tensor<uint64_t> bit_roll;
  for (int track=0; track<roll.size(); track++) {
    matrix<uint64_t> bit_track(roll[track].size(),vector<uint64_t>(2*48,0));
    for (int bar=0; bar<roll[track].size(); bar++) {
      for (int step=0; step<roll[track][bar].size(); step++) {
        for (int i=0; i<128; i++) {
          if (roll[track][bar][step][i]) {
            bit_track[bar][step*2 + i/64] |= ((uint64_t)1<<(i%64));
          }
        }
      }
    }
    bit_roll.push_back( bit_track );
  }
  return bit_roll;
}

tensor<uint8_t> fast_bit_roll(std::string &filepath, int resolution, int nbars, int start) {
  qtensor<bool> roll = fast_bar_roll(filepath, resolution, nbars, start);
  tensor<uint8_t> bit_roll;
  for (int track=0; track<roll.size(); track++) {
    matrix<uint8_t> bit_track(roll[track].size(),vector<uint8_t>(16*48,0));
    for (int bar=0; bar<roll[track].size(); bar++) {
      for (int step=0; step<roll[track][bar].size(); step++) {
        for (int i=0; i<128; i++) {
          if (roll[track][bar][step][i]) {
            bit_track[bar][step*16 + i/8] |= ((uint8_t)1<<(i%8));
          }
        }
      }
    }
    bit_roll.push_back( bit_track );
  }
  return bit_roll;
}

std::vector<hash_type> min_hash(std::vector<uint8_t> &data, std::vector<int> &seeds, int k) {
  std::vector<hash_type> result(seeds.size(), 0);
  for (int i=0; i<seeds.size(); i++) {
    hash_type mh = UINT32_MAX;
    for (int s=0; s<(int)data.size()-k+1; s++) {
      hash_type h = XXH32(
        reinterpret_cast<char*>(data.data()+s), k*sizeof(char), seeds[i]);
      mh = std::min(mh, h);
    }
    result[i] = mh;
  }
  return result;
}

std::vector<hash_type> empty_hash(std::vector<int> &seeds, int k) {
  std::vector<uint8_t> data(16*48,0);
  return min_hash(data, seeds, k);
}

// actually do the hashing
// (track x bar x hash) tensor
tensor<hash_type> fast_min_hash(std::string &filepath, int resolution, int bars, int start, std::vector<int> seeds, int k) {
  tensor<uint8_t> bit_roll = fast_bit_roll(filepath,resolution,bars,start);
  tensor<hash_type> hashes;
  for (int track=0; track<bit_roll.size(); track++) {
    matrix<hash_type> track_hashes;
    for (int bar=0; bar<bit_roll[track].size(); bar++) {
      track_hashes.push_back( min_hash(bit_roll[track][bar], seeds, k) );
    }
    hashes.push_back( track_hashes );
  }
  return hashes;
}

}
// END OF NAMESPACE