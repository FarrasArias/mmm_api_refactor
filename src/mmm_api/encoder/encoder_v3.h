#pragma once

#include "encoder_base.h"
#include "util.h"
#include "../enum/track_type.h"
#include "../enum/velocity.h"
#include "../enum/timesigs.h"
#include "../enum/pretrain_group.h"
#include "../protobuf/util.h"
#include "../protobuf/validate.h"

// START OF NAMESPACE
namespace mmm {


class EncoderVt : public ENCODER {
public:
  EncoderVt() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~EncoderVt() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {NUM_BARS, TOKEN_DOMAIN({4,8}, INT_VALUES_DOMAIN)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TIME_SIGNATURE, TOKEN_DOMAIN(
        BLUE_TS_MAP,TIMESIG_MAP_DOMAIN)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK  
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {NOTE_DURATION, TOKEN_DOMAIN(96)},
      {TIME_ABSOLUTE, TOKEN_DOMAIN(192)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {MIN_NOTE_DURATION_HARD, TOKEN_DOMAIN(6)},
      {MAX_NOTE_DURATION_HARD, TOKEN_DOMAIN(6)},
      {MIN_POLYPHONY_HARD, TOKEN_DOMAIN(10)},
      {MAX_POLYPHONY_HARD, TOKEN_DOMAIN(10)},
      {REST_PERCENTAGE, TOKEN_DOMAIN(10)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)},
      {PITCH_CLASS, TOKEN_DOMAIN(12)},
      {VELOCITY_LEVEL, TOKEN_DOMAIN(DEFAULT_VELOCITY_MAP,INT_MAP_DOMAIN)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->do_pretrain_map = true;
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
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
    update_pitch_range(p);
    update_pitch_class(p);
    update_note_density(p);
  }
};


}
// END OF NAMESPACE