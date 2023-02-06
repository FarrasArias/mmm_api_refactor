#pragma once

#include "encoder_base.h"
#include "util.h"
#include "../enum/track_type.h"
#include "../enum/velocity.h"
#include "../enum/timesigs.h"
#include "../protobuf/util.h"
#include "../protobuf/validate.h"

// ==========================================================================
// ==========================================================================
// Teenage Engineering Representations

// START OF NAMESPACE
namespace mmm {

class TeTrackDensityEncoder : public ENCODER {
public:
  TeTrackDensityEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)}, // 1 is bar infill
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::OPZ_KICK_TRACK,
        midi::OPZ_SNARE_TRACK,
        midi::OPZ_HIHAT_TRACK,
        midi::OPZ_SAMPLE_TRACK,
        midi::OPZ_BASS_TRACK,
        midi::OPZ_LEAD_TRACK,
        midi::OPZ_ARP_TRACK,
        midi::OPZ_CHORD_TRACK   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      {FILL_IN, TOKEN_DOMAIN(3)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)}
      });
    config = get_encoder_config();
  }
  ~TeTrackDensityEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_density = true;
    e->min_tracks = 1;
    e->use_drum_offsets = false;
    e->te = true;
    e->resolution = 12;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    // add validate function here ...
    update_note_density(p);
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    if (config->do_multi_fill == true) {
      tokens = resolve_bar_infill_tokens(tokens, rep);
    }
    return decode_track_dev(tokens, p, rep, config);
  }
};

/*
class TeEncoder : public ENCODER {
public:
  TeEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)}, // 1 is bar infill
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::OPZ_KICK_TRACK,
        midi::OPZ_SNARE_TRACK,
        midi::OPZ_HIHAT_TRACK,
        midi::OPZ_SAMPLE_TRACK,
        midi::OPZ_BASS_TRACK,
        midi::OPZ_LEAD_TRACK,
        midi::OPZ_ARP_TRACK,
        midi::OPZ_CHORD_TRACK   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {NOTE_DURATION, TOKEN_DOMAIN(96)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {VELOCITY_LEVEL, TOKEN_DOMAIN(DEFAULT_VELOCITY_MAP,INT_MAP_DOMAIN)}
      });
    config = get_encoder_config();
  }
  ~TeEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    //e->mark_density = true;
    e->use_velocity_levels = true;
    e->use_note_duration_encoding = true;
    e->min_tracks = 1;
    e->use_drum_offsets = false;
    e->te = true;
    e->resolution = 12;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    // add validate function here ...
    update_note_density(p);
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    if (config->do_multi_fill == true) {
      tokens = resolve_bar_infill_tokens(tokens, rep);
    }
    return decode_track_dev(tokens, p, rep, config);
  }
};
*/

/// v2 representations

class TeEncoder : public ENCODER {
public:
  TeEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~TeEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {NUM_BARS, TOKEN_DOMAIN({1,2,4,8}, INT_VALUES_DOMAIN)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TIME_SIGNATURE, TOKEN_DOMAIN(
        BASIC_TIMESIG_MAP,TIMESIG_MAP_DOMAIN)},
      {TRACK, TOKEN_DOMAIN({
        midi::OPZ_KICK_TRACK,
        midi::OPZ_SNARE_TRACK,
        midi::OPZ_HIHAT_TRACK,
        midi::OPZ_SAMPLE_TRACK,
        midi::OPZ_BASS_TRACK,
        midi::OPZ_LEAD_TRACK,
        midi::OPZ_ARP_TRACK,
        midi::OPZ_CHORD_TRACK   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {NOTE_DURATION, TOKEN_DOMAIN(96)},
      {TIME_ABSOLUTE, TOKEN_DOMAIN(96)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {MIN_NOTE_DURATION, TOKEN_DOMAIN(6)},
      {MAX_NOTE_DURATION, TOKEN_DOMAIN(6)},
      {MIN_POLYPHONY, TOKEN_DOMAIN(10)},
      {MAX_POLYPHONY, TOKEN_DOMAIN(10)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)},
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_note_duration_quantile = true;
    e->mark_polyphony_quantile = true;
    e->use_note_duration_encoding = true;
    e->use_absolute_time_encoding = true;
    e->mark_time_sigs = true;
    e->mark_drum_density = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->te = true;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
    update_note_density(p);
  }
};


// updated encoders
class TeVelocityEncoder : public ENCODER {
public:
  TeVelocityEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~TeVelocityEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {NUM_BARS, TOKEN_DOMAIN({1,2,4,8}, INT_VALUES_DOMAIN)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TIME_SIGNATURE, TOKEN_DOMAIN(
        BASIC_TIMESIG_MAP,TIMESIG_MAP_DOMAIN)},
      {TRACK, TOKEN_DOMAIN({
        midi::OPZ_KICK_TRACK,
        midi::OPZ_SNARE_TRACK,
        midi::OPZ_HIHAT_TRACK,
        midi::OPZ_SAMPLE_TRACK,
        midi::OPZ_BASS_TRACK,
        midi::OPZ_LEAD_TRACK,
        midi::OPZ_ARP_TRACK,
        midi::OPZ_CHORD_TRACK   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {NOTE_DURATION, TOKEN_DOMAIN(96)},
      {TIME_ABSOLUTE, TOKEN_DOMAIN(96)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)},
      {VELOCITY_LEVEL, TOKEN_DOMAIN(DEFAULT_VELOCITY_MAP,INT_MAP_DOMAIN)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->use_note_duration_encoding = true;
    e->use_absolute_time_encoding = true;
    e->mark_time_sigs = true;
    e->mark_density = true;
    e->mark_drum_density = true;
    e->use_drum_offsets = false;
    e->use_velocity_levels = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->te = true;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
    update_note_density(p);
  }
};


class TeVelocityDurationPolyphonyEncoder : public ENCODER {
public:
  TeVelocityDurationPolyphonyEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~TeVelocityDurationPolyphonyEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {NUM_BARS, TOKEN_DOMAIN({1,2,4,8}, INT_VALUES_DOMAIN)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TIME_SIGNATURE, TOKEN_DOMAIN(
        BASIC_TIMESIG_MAP,TIMESIG_MAP_DOMAIN)},
      {TRACK, TOKEN_DOMAIN({
        midi::OPZ_KICK_TRACK,
        midi::OPZ_SNARE_TRACK,
        midi::OPZ_HIHAT_TRACK,
        midi::OPZ_SAMPLE_TRACK,
        midi::OPZ_BASS_TRACK,
        midi::OPZ_LEAD_TRACK,
        midi::OPZ_ARP_TRACK,
        midi::OPZ_CHORD_TRACK   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {NOTE_DURATION, TOKEN_DOMAIN(96)},
      {TIME_ABSOLUTE, TOKEN_DOMAIN(96)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {MIN_NOTE_DURATION, TOKEN_DOMAIN(6)},
      {MAX_NOTE_DURATION, TOKEN_DOMAIN(6)},
      {MIN_POLYPHONY, TOKEN_DOMAIN(10)},
      {MAX_POLYPHONY, TOKEN_DOMAIN(10)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)},
      {VELOCITY_LEVEL, TOKEN_DOMAIN(DEFAULT_VELOCITY_MAP,INT_MAP_DOMAIN)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_note_duration_quantile = true;
    e->mark_polyphony_quantile = true;
    e->use_note_duration_encoding = true;
    e->use_absolute_time_encoding = true;
    e->mark_time_sigs = true;
    e->mark_drum_density = true;
    e->use_drum_offsets = false;
    e->use_velocity_levels = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->te = true;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
    update_note_density(p);
  }
};


}
// END OF NAMESPACE
