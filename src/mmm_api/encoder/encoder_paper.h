#pragma once

#include "encoder_base.h"
#include "util.h"
#include "../protobuf/util.h"
#include "../protobuf/validate.h"

// =====================================================================
// FINAL PAPER ENCODINGS
// track model
// track model w velocity
// barfill model
// barfill model w velocity

// START OF NAMESPACE
namespace mmm {

class TrackDensityEncoder : public ENCODER {
public:
  TrackDensityEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{DENSITY_LEVEL,10}}}, // 10 density levels
      "no_velocity");
  }
  ~TrackDensityEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    e->mark_density = true;
    //e->num_bars = 4;
    e->min_tracks = 1;
    prepare_piece(p);
    update_note_density(p);
    if (!e->force_valid) {
      update_has_notes(p);
      update_valid_segments(p, e);
      if (p->valid_segments_size() == 0) {
        std::cout << "NO VALID SEGMENTS" << std::endl;
        throw(1); // need to start over!
      }
    }
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class TrackDensityVelocityEncoder : public ENCODER {
public:
  TrackDensityVelocityEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      {{DENSITY_LEVEL,10}}, // 10 density levels
      {{VELOCITY_LEVEL,32}}}, 
      "magenta");
  }
  ~TrackDensityVelocityEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->force_instrument = true;
    e->mark_density = true;
    //e->num_bars = 4;
    e->min_tracks = 1;
    e->use_velocity_levels = true;
    update_note_density(p);
    if (!e->force_valid) {
      update_valid_segments(p, e);
      if (p->valid_segments_size() == 0) {
        std::cout << "NO VALID SEGMENTS" << std::endl;
        throw(1); // need to start over!
      }
    }
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p, EncoderConfig *e) {
    return decode_track(tokens, p, rep, e);
  }
};

class TrackBarFillDensityEncoder : public ENCODER {
public:
  TrackBarFillDensityEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      //{{FILL_IN,3}},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {{DENSITY_LEVEL,10}}}, // 10 density levels
      "no_velocity");
  }
  ~TrackBarFillDensityEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->do_multi_fill = true;
    e->fill_percentage = .5;
    e->force_instrument = true;
    e->mark_density = true;
    // multi fill line used to be here
    update_note_density(p);
    if (!e->force_valid) {
      // only need to do these things in train
      e->multi_fill.clear(); // so it chooses new bars to fill
      update_valid_segments(p, e);
      if (p->valid_segments_size() == 0) {
        std::cout << "NO VALID SEGMENTS" << std::endl;
        throw(1); // need to start over!
      }
    }
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &raw_tokens, midi::Piece *p, EncoderConfig *e) {
    // before decoding we insert fills into sequence
    // this solution works with any number of fills
    int fill_pholder = rep->encode({{FILL_IN_PLACEHOLDER,0}});
    int fill_start = rep->encode({{FILL_IN_START,0}});
    int fill_end = rep->encode({{FILL_IN_END,0}});

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

class TrackBarFillDensityVelocityEncoder : public ENCODER {
public:
  TrackBarFillDensityVelocityEncoder() {
    rep = new REPRESENTATION({
      {{PIECE_START,1}},
      {{BAR,1}}, 
      {{BAR_END,1}}, 
      {{TRACK,2}},
      {{TRACK_END,1}},
      {{INSTRUMENT,128}},
      {{PITCH,128},{VELOCITY,2}},
      {{TIME_DELTA,48}},
      //{{FILL_IN,3}},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {{DENSITY_LEVEL,10}},
      {{VELOCITY_LEVEL,32}}}, // 10 density levels
      "magenta");
  }
  ~TrackBarFillDensityVelocityEncoder() {
    delete rep;
  }
  std::vector<int> encode(midi::Piece *p, EncoderConfig *e) {
    e->do_multi_fill = true;
    e->fill_percentage = .5;
    e->force_instrument = true;
    e->mark_density = true;
    e->use_velocity_levels = true;
    // multi fill line used to be here
    update_note_density(p);
    if (!e->force_valid) {
      e->multi_fill.clear(); // so it chooses new bars to fill
      update_valid_segments(p, e);
      if (p->valid_segments_size() == 0) {
        std::cout << "NO VALID SEGMENTS" << std::endl;
        throw(1); // need to start over!
      }
    }
    return to_performance_w_tracks(p, rep, e);
  }
  void decode(std::vector<int> &raw_tokens, midi::Piece *p, EncoderConfig *e) {
    // before decoding we insert fills into sequence
    // this solution works with any number of fills
    int fill_pholder = rep->encode({{FILL_IN_PLACEHOLDER,0}});
    int fill_start = rep->encode({{FILL_IN_START,0}});
    int fill_end = rep->encode({{FILL_IN_END,0}});

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


}
// END OF NAMESPACE