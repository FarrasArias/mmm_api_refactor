#pragma once

#include "encoder_base.h"
#include "util.h"
#include "util_legacy.h"

class TrackEncoder : public ENCODER  {
public:
  TrackEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}}},
      "no_velocity");
  }
  ~TrackEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true; // WATCH OUT
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class TrackBarMajorEncoder : public ENCODER  {
public:
  TrackBarMajorEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}}},
      "no_velocity");
  }
  ~TrackBarMajorEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    e->bar_major = true;
    e->num_bars = 16;
    e->piece_header = false;
    return main_enc(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return main_dec(tokens, p, rep, e);
  }
};

class SegmentEncoder : public ENCODER  {
public:
  SegmentEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{FILL_IN,3}},
      {{SEGMENT,1}},
      {{SEGMENT_END,1}},
      {{SEGMENT_FILL_IN,3}}
      },
      "no_velocity");
  }
  ~SegmentEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    e->segment_mode = true;
    e->num_bars = 16;
    e->min_tracks = 2;
    e->piece_header = false;
    e->do_multi_fill = true;
    e->fill_percentage = .5;
    update_valid_segments(p, e);
    if (p->valid_segments_size() == 0) {
      throw(1); // need to start over!
    }
    return main_enc(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    //throw(1); // its not implemented yet ...
    // split at each bar and create [segment][track][bar] of vectors
    int seg_num = 0;
    int track_num = 0;
    int bar_num = 0;
    int fill_num = 0;

    // also need to capture track headers
    std::vector<std::tuple<int,int,int>> fill_holder;
    std::vector<std::vector<std::vector<std::vector<int>>>> holder;
    bool bar_started = false;
    std::set<int> track_nums;

    for (const auto token : tokens) {
      if (rep->is_token_type(token, BAR)) {
        bar_started = true;
        holder[seg_num][track_num].push_back( std::vector<int>() );  
      }
      else if (rep->is_token_type(token, TRACK)) {
        holder[seg_num].push_back( std::vector<std::vector<int>>() );
        track_nums.insert( track_num );
      }
      else if (rep->is_token_type(token, SEGMENT)) {
        holder.push_back( std::vector<std::vector<std::vector<int>>>() );
      }
      else if (rep->is_token_type(token, BAR_END)) {
        holder[seg_num][track_num][bar_num].push_back(token);
        bar_started = false;
        bar_num++;
      }
      else if (rep->is_token_type(token, TRACK_END)) { 
        track_num++;
        bar_num = 0;
      }
      else if (rep->is_token_type(token, SEGMENT_END)) {
        seg_num++;
        track_num = 0;
        bar_num = 0;
      }
      else if (rep->is_token_type(token, FILL_IN)) {
        int value = rep->decode(token, FILL_IN);
        if (value == 0) {
          fill_holder.push_back( std::make_tuple(seg_num,track_num,bar_num) );
        }
        if (value == 1) {
          // reset seg_num, track_num and bar_num when fill segment starts
          std::tuple<int,int,int> loc = fill_holder[fill_num];
          seg_num = std::get<0>(loc);
          track_num = std::get<1>(loc);
          bar_num = std::get<2>(loc);
          bar_started = true;
        }
        else if (value == 2) {
          // want to push back BAR_END instead
          holder[seg_num][track_num][bar_num].push_back(token);
          bar_started = false;
          fill_num++;
        }
      }

      if (bar_started) {
        holder[seg_num][track_num][bar_num].push_back(token);
      }
    }

    // iterate over the bars are put in normal order
    std::vector<int> ftokens;
    for (const auto track_num : track_nums) {
      // add track header
      for (int seg_num=0; seg_num<4; seg_num++) {
        for (int bar_num=0; bar_num<4; bar_num++) {
          if ((holder[seg_num].size() > track_num) && (holder[seg_num][track_num].size() > bar_num)) {
            for (const auto token : holder[seg_num][track_num][bar_num]) {
              ftokens.push_back( token );
            }
          }
        }
      }
      // add track footer

    }
    
    //return main_dec(tokens, p, rep, e);
  }
};



// we use CD2 genre as it has 14138 matches
class TrackGenreEncoder : public ENCODER  {
public:
  TrackGenreEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{GENRE,16}}},
      "no_velocity");
  }
  ~TrackGenreEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    e->genre_header = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class TrackVelocityLevelEncoder : public ENCODER {
public:
  TrackVelocityLevelEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{VELOCITY_LEVEL,32}},
      {{TIME_DELTA,48}}},
      "magenta");
  }
  ~TrackVelocityLevelEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    e->use_velocity_levels = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class TrackVelocityEncoder : public ENCODER {
public:
  TrackVelocityEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,32}},
      {{TIME_DELTA,48}}},
      "magenta");
  }
  ~TrackVelocityEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class TrackInstHeaderEncoder : public ENCODER  {
public:
  TrackInstHeaderEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{HEADER,2}}},
      "no_velocity");
  }
  ~TrackInstHeaderEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    e->instrument_header = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class TrackOneBarFillEncoder : public ENCODER  {
public:
  TrackOneBarFillEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{FILL_IN,3}}},
      "no_velocity");
  }
  ~TrackOneBarFillEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->do_fill = true;
    e->force_instrument = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &raw_tokens, midi::Piece *p, EncoderConfig *e) {
    // before decoding we just need to insert the last section
    // this needs to be fixed for fill in end token
    int fill_placeholder = rep->encode({{FILL_IN,0}});
    int fill_start = rep->encode({{FILL_IN,1}});

    raw_tokens.pop_back(); // might not need this at all .. 
    std::vector<int> tokens;

    auto src = find(raw_tokens.begin(), raw_tokens.end(), fill_start);
    tokens.insert(tokens.begin(), raw_tokens.begin(), src);
    auto dst = find(tokens.begin(), tokens.end(), fill_placeholder);
    tokens.insert(dst, next(src), raw_tokens.end());
    tokens.erase(find(tokens.begin(), tokens.end(), fill_placeholder));

    return decode_track(tokens, p, rep, e);
  }
};

class TrackOneTwoThreeBarFillEncoder : public ENCODER {
public:
  TrackOneTwoThreeBarFillEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{FILL_IN,3}}},
      "no_velocity");
  }
  ~TrackOneTwoThreeBarFillEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->do_multi_fill = true;
    e->force_instrument = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &raw_tokens, midi::Piece *p, EncoderConfig *e) {
    // before decoding we insert fills into sequence
    // this solution works with any number of fills
    int fill_pholder = rep->encode({{FILL_IN,0}});
    int fill_start = rep->encode({{FILL_IN,1}});
    int fill_end = rep->encode({{FILL_IN,2}});

    std::vector<int> tokens;

    auto start_pholder = raw_tokens.begin();
    auto start_fill = raw_tokens.begin();
    auto end_fill = raw_tokens.begin();

    while (start_pholder != raw_tokens.end()) {
      start_pholder = next(start_pholder); // FIRST TOKEN IS PIECE_START ANYWAYS
      auto last_start_pholder = start_pholder;
      start_pholder = find(start_pholder, raw_tokens.end(), fill_pholder);
      if (start_pholder != raw_tokens.end()) {
        start_fill = find(next(start_fill), raw_tokens.end(), fill_start);
        end_fill = find(next(end_fill), raw_tokens.end(), fill_end);

        // insert from last_start_pholder --> start_pholder
        tokens.insert(tokens.end(), last_start_pholder, start_pholder);
        tokens.insert(tokens.end(), next(start_fill), end_fill);
      }
      else {
        // insert from last_start_pholder --> end of sequence (excluding fill)
        start_fill = find(raw_tokens.begin(), raw_tokens.end(), fill_start);
        tokens.insert(tokens.end(), last_start_pholder, start_fill);
      }
    }
    return decode_track(tokens, p, rep, e);
  }
};

class TrackBarFillEncoder : public ENCODER {
public:
  TrackBarFillEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{FILL_IN,3}}},
      "no_velocity");
  }
  ~TrackBarFillEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->do_multi_fill = true;
    e->fill_percentage = .5;
    e->force_instrument = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &raw_tokens, midi::Piece *p, EncoderConfig *e) {
    // before decoding we insert fills into sequence
    // this solution works with any number of fills
    int fill_pholder = rep->encode({{FILL_IN,0}});
    int fill_start = rep->encode({{FILL_IN,1}});
    int fill_end = rep->encode({{FILL_IN,2}});

    std::vector<int> tokens;

    auto start_pholder = raw_tokens.begin();
    auto start_fill = raw_tokens.begin();
    auto end_fill = raw_tokens.begin();

    while (start_pholder != raw_tokens.end()) {
      start_pholder = next(start_pholder); // FIRST TOKEN IS PIECE_START ANYWAYS
      auto last_start_pholder = start_pholder;
      start_pholder = find(start_pholder, raw_tokens.end(), fill_pholder);
      if (start_pholder != raw_tokens.end()) {
        start_fill = find(next(start_fill), raw_tokens.end(), fill_start);
        end_fill = find(next(end_fill), raw_tokens.end(), fill_end);

        // insert from last_start_pholder --> start_pholder
        tokens.insert(tokens.end(), last_start_pholder, start_pholder);
        tokens.insert(tokens.end(), next(start_fill), end_fill);
      }
      else {
        // insert from last_start_pholder --> end of sequence (excluding fill)
        start_fill = find(raw_tokens.begin(), raw_tokens.end(), fill_start);
        tokens.insert(tokens.end(), last_start_pholder, start_fill);
      }
    }
    return decode_track(tokens, p, rep, e);
  }
};

class TrackBarFillSixteenEncoder : public ENCODER {
public:
  TrackBarFillSixteenEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{FILL_IN,3}}},
      "no_velocity");
  }
  ~TrackBarFillSixteenEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    // FIX THIS ....
    //e->do_multi_fill = true;
    //e->fill_percentage = .5;
    e->force_instrument = true;
    e->num_bars = 16;
    e->min_tracks = 2;
    if (p->valid_segments_size() == 0) {
      throw(1); // need to start over!
    }
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &raw_tokens, midi::Piece *p, EncoderConfig *e) {
    // before decoding we insert fills into sequence
    // this solution works with any number of fills
    int fill_pholder = rep->encode({{FILL_IN,0}});
    int fill_start = rep->encode({{FILL_IN,1}});
    int fill_end = rep->encode({{FILL_IN,2}});

    std::vector<int> tokens;

    auto start_pholder = raw_tokens.begin();
    auto start_fill = raw_tokens.begin();
    auto end_fill = raw_tokens.begin();

    while (start_pholder != raw_tokens.end()) {
      start_pholder = next(start_pholder); // FIRST TOKEN IS PIECE_START ANYWAYS
      auto last_start_pholder = start_pholder;
      start_pholder = find(start_pholder, raw_tokens.end(), fill_pholder);
      if (start_pholder != raw_tokens.end()) {
        start_fill = find(next(start_fill), raw_tokens.end(), fill_start);
        end_fill = find(next(end_fill), raw_tokens.end(), fill_end);

        // insert from last_start_pholder --> start_pholder
        tokens.insert(tokens.end(), last_start_pholder, start_pholder);
        tokens.insert(tokens.end(), next(start_fill), end_fill);
      }
      else {
        // insert from last_start_pholder --> end of sequence (excluding fill)
        start_fill = find(raw_tokens.begin(), raw_tokens.end(), fill_start);
        tokens.insert(tokens.end(), last_start_pholder, start_fill);
      }
    }
    return decode_track(tokens, p, rep, e);
  }
};

// this version will allow for control of note density
// and monophonic or polyphonic
// also includes time signature update
class TrackMonoPolyDensityEncoder : public ENCODER  {
public:
  TrackMonoPolyDensityEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,3}}, // {mono, drums, polyphonic}
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{DENSITY_LEVEL,10}}, // 10 density levels
      {{TIME_SIGNATURE,5}}}, // 5 main time signatures
      "no_velocity");
  }
  ~TrackMonoPolyDensityEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->mark_polyphony = true;
    e->force_instrument = true;
    e->mark_density = true;
    e->mark_time_sigs = true;
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};


class TrackMonoPolyEncoder : public ENCODER {
public:
  TrackMonoPolyEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,3}}, // {mono, drums, polyphonic}
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}}},
      "no_velocity");
  }
  ~TrackMonoPolyEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->mark_polyphony = true;
    e->force_instrument = true;
    //update_polyphony(p); // done in dataset v4
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class PerformanceEncoder : public ENCODER {
public:
  PerformanceEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{PITCH,128},{VELOCITY,2}},
      {{NON_PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}}},
      "no_velocity");
  }
  ~PerformanceEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    // get all the events sorted
    std::vector<midi::Event> events = get_sorted_events(p);
    std::vector<int> tokens;
    int current_step = 0;
    int current_instrument = -1;
    int N_TIME_TOKENS = rep->get_domain(TIME_DELTA);
    std::array<int,128> VMAP = velocity_maps[rep->velocity_map_name];
    for (const auto event : events) {

      if (event.time() > current_step) {
        while (event.time() > current_step + N_TIME_TOKENS) {
          tokens.push_back( rep->encode({{TIME_DELTA,N_TIME_TOKENS-1}}) );
          current_step += N_TIME_TOKENS;
        }
        if (event.time() > current_step) {
          tokens.push_back( rep->encode(
            {{TIME_DELTA,event.time()-current_step-1}}) );
        }
        current_step = event.time();
      }
      else if (event.time() < current_step) {
        std::cout << "ERROR : events are not sorted." << std::endl;
        throw(2);
      }
      if (event.is_drum()) {
        tokens.push_back( rep->encode({
          {NON_PITCH,event.pitch()},
          {VELOCITY,VMAP[event.velocity()]}
        }));
      }
      else {
        tokens.push_back( rep->encode({
          {PITCH,event.pitch()},
          {VELOCITY,VMAP[event.velocity()]}
        }));
      }
    }
    return tokens;

  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    throw(1);
  }
};