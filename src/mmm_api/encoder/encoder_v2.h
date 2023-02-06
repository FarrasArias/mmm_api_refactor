#pragma once

#include <limits>

#include "encoder_base.h"
#include "util.h"
#include "../enum/track_type.h"
#include "../enum/genres.h"
#include "../protobuf/util.h"
#include "../protobuf/validate.h"

#include "../enum/timesigs.h"

// START OF NAMESPACE
namespace mmm {

const float FLOAT_MAX = std::numeric_limits<float>::max();

class TrackDensityEncoder : public ENCODER {
public:
  TrackDensityEncoder() {
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)}
    });
    config = get_encoder_config();
  }
  ~TrackDensityEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->te = false;
    e->force_instrument = true;
    e->mark_density = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    update_note_density(p);
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
    //return to_performance_w_tracks_dev(p, rep, config);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    return decode_track_dev(tokens, p, rep, config);
  }
};


class TrackDensityEncoderV2 : public ENCODER {
public:
  TrackDensityEncoderV2() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)}, // 1 is bar infill
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)}
      });
    config = get_encoder_config();
  }
  ~TrackDensityEncoderV2() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->te = false;
    e->force_instrument = true;
    e->mark_density = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->use_drum_offsets = false;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    // add validate function here ...
    update_note_density(p);
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
    //return to_performance_w_tracks_dev(p, rep, config);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    if (config->do_multi_fill == true) {
      tokens = resolve_bar_infill_tokens(tokens, rep);
    }
    return decode_track_dev(tokens, p, rep, config);
  }
};

class TrackBarFillDensityEncoder : public ENCODER {
public:
  TrackBarFillDensityEncoder() {
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)}
    });
    config = get_encoder_config();
  }
  ~TrackBarFillDensityEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->do_multi_fill = true;
    e->force_instrument = true;
    e->mark_density = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    update_note_density(p);
  }
};

//==========================================================================


class TrackInterleavedEncoder : public ENCODER {
public:
  TrackInterleavedEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(256)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      });
    config = get_encoder_config();
  }
  ~TrackInterleavedEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->te = false;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    return to_interleaved_performance(p, rep, config);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    return decode_track_dev(tokens, p, rep, config);
  }
};

class TrackInterleavedWHeaderEncoder : public ENCODER {
public:
  TrackInterleavedWHeaderEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(256)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      {HEADER, TOKEN_DOMAIN(2)}
      });
    config = get_encoder_config();
  }
  ~TrackInterleavedWHeaderEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->te = false;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->interleaved = true;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    return to_interleaved_performance(p, rep, config);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    tokens = strip_header(tokens, rep);
    return decode_track_dev(tokens, p, rep, config);
  }
};


class TrackEncoder : public ENCODER {
public:
  TrackEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      });
    config = get_encoder_config();
  }
  ~TrackEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->force_instrument = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
    //return to_performance_w_tracks_dev(p, rep, config);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    return decode_track_dev(tokens, p, rep, config);
  }
};

class TrackNoInstEncoder : public ENCODER {
public:
  TrackNoInstEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      });
    config = get_encoder_config();
  }
  ~TrackNoInstEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->mark_instrument = false;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
    //return to_performance_w_tracks_dev(p, rep, config);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    return decode_track_dev(tokens, p, rep, config);
  }
};

class TrackUnquantizedEncoder : public ENCODER {
public:
  TrackUnquantizedEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
    });
    config = get_encoder_config();
  }
  ~TrackUnquantizedEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->resolution = 12;
    e->unquantized = true;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    throw std::runtime_error("can't call this function!");
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    throw std::runtime_error("can't call this function!");
  }
};

//==========================================================================
// roll models into one using piece start
// and allow for unlimited n-bar segments to be encoded

/*
class TrackSegmentEncoder : public ENCODER {
public:
  TrackSegmentEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(1)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      {SEGMENT, TOKEN_DOMAIN(1)},
      {SEGMENT_END, TOKEN_DOMAIN(1)},
      });
    config = get_encoder_config();
  }
  ~TrackSegmentEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->force_instrument = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->multi_segment = true;
    return e;
  }
  std::vector<int> encode(midi::Piece *p) {
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
    //return to_performance_w_tracks_dev(p, rep, config);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    return decode_track_dev(tokens, p, rep, config);
  }
};
*/

/// development encoders


class TrackNoteDurationEncoder : public ENCODER {
public:
  TrackNoteDurationEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)}, // 1 is bar infill
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {NOTE_DURATION, TOKEN_DOMAIN(
        {.25,.5,1.,2.,FLOAT_MAX},CONTINUOUS_DOMAIN)},
      {AV_POLYPHONY, TOKEN_DOMAIN(
        {1.25,2.25,3.25,4.25,FLOAT_MAX},CONTINUOUS_DOMAIN)},
      {TIME_SIGNATURE, TOKEN_DOMAIN(
        FOUR_FOUR_TIMESIG_MAP,TIMESIG_MAP_DOMAIN)}
      });
    config = get_encoder_config();
  }
  ~TrackNoteDurationEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->te = false;
    e->force_instrument = true;
    e->mark_note_duration = true;
    e->mark_polyphony = true;
    e->mark_time_sigs = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->use_drum_offsets = false;
    return e;
  }
  void preprocess_piece(midi::Piece *p) {
    update_av_polyphony_and_note_duration(p);
  }
  void decode(std::vector<int> &tokens, midi::Piece *p) {
    if (config->do_multi_fill == true) {
      tokens = resolve_bar_infill_tokens(tokens, rep);
    }
    return decode_track_dev(tokens, p, rep, config);
  }
};

class TrackNoteDurationContEncoder : public ENCODER {
public:
  TrackNoteDurationContEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)}, // 1 is bar infill
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {TIME_SIGNATURE, TOKEN_DOMAIN(
        BASIC_TIMESIG_MAP,TIMESIG_MAP_DOMAIN)}
      });
    config = get_encoder_config();
  }
  ~TrackNoteDurationContEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->te = false;
    e->force_instrument = true;
    e->embed_dim = 3; // poly silence note_duration
    e->density_continuous = true;
    e->mark_time_sigs = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->use_drum_offsets = false;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    update_density_trifeature(p);
  }

  std::vector<double> convert_feature(midi::ContinuousFeature f) {
    if (!f.has_av_silence()) {
      return empty_embedding();
    }
    return {f.av_polyphony(),f.av_silence(),f.note_duration()};
  }
};

class TrackNoteDurationEmbedEncoder : public ENCODER {
public:
  TrackNoteDurationEmbedEncoder() {    
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)}, // 1 is bar infill
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {TIME_SIGNATURE, TOKEN_DOMAIN(
        BASIC_TIMESIG_MAP,TIMESIG_MAP_DOMAIN)}
      });
    config = get_encoder_config();
  }
  ~TrackNoteDurationEmbedEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->te = false;
    e->force_instrument = true;
    e->embed_dim = 1; // note_duration (normalized)
    e->density_continuous = true;
    e->mark_time_sigs = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    e->use_drum_offsets = false;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    update_density_trifeature(p);
  }

  std::vector<double> convert_feature(midi::ContinuousFeature f) {
    if (!f.has_note_duration_norm()) {
      return empty_embedding();
    }
    return {f.note_duration_norm()};
  }
};

// ===============================================================
// GENRE EXPERIMENTS


class DensityGenreEncoder : public ENCODER {
public:
  DensityGenreEncoder() {
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)},
      {GENRE, TOKEN_DOMAIN(GENRE_MAP_DISCOGS,STRING_MAP_DOMAIN)}
    });
    config = get_encoder_config();
  }
  ~DensityGenreEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_density = true;
    e->mark_genre = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }
  void preprocess_piece(midi::Piece *p) {
    std::string genre = "NONE";
    if (p->internal_genre_data_size()) {
      genre = p->internal_genre_data(0).discogs();
    }
    for (int i=0; i<p->tracks_size(); i++) {
      midi::TrackFeatures *f = get_track_features(p,i);
      if (f->genre_str().size() == 0) {
        f->set_genre_str(genre);
      }
    }
    update_note_density(p);
  }
};

class DensityGenreTagtraumEncoder : public ENCODER {
public:
  DensityGenreTagtraumEncoder() {
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)},
      {GENRE, TOKEN_DOMAIN(GENRE_MAP_TAGTRAUM,STRING_MAP_DOMAIN)}
    });
    config = get_encoder_config();
  }
  ~DensityGenreTagtraumEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_density = true;
    e->mark_genre = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }
  void preprocess_piece(midi::Piece *p) {
    std::string genre = "NONE";
    if (p->internal_genre_data_size()) {
      genre = p->internal_genre_data(0).tagtraum();
    }
    for (int i=0; i<p->tracks_size(); i++) {
      midi::TrackFeatures *f = get_track_features(p,i);
      if (f->genre_str().size() == 0) {
        f->set_genre_str(genre);
      }
    }
    update_note_density(p);
  }
};

class DensityGenreLastfmEncoder : public ENCODER {
public:
  DensityGenreLastfmEncoder() {
    rep = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {DENSITY_LEVEL, TOKEN_DOMAIN(10)},
      {GENRE, TOKEN_DOMAIN(GENRE_MAP_LASTFM,STRING_MAP_DOMAIN)}
    });
    config = get_encoder_config();
  }
  ~DensityGenreLastfmEncoder() {
    delete rep;
    delete config;
  }
  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_density = true;
    e->mark_genre = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }
  void preprocess_piece(midi::Piece *p) {
    std::string genre = "NONE";
    if (p->internal_genre_data_size()) {
      genre = p->internal_genre_data(0).lastfm();
    }
    for (int i=0; i<p->tracks_size(); i++) {
      midi::TrackFeatures *f = get_track_features(p,i);
      if (f->genre_str().size() == 0) {
        f->set_genre_str(genre);
      }
    }
    update_note_density(p);
  }
};


// ===============================================================
// POLYPHONY EXPERIMENTS


class PolyphonyEncoder : public ENCODER {
public:
  PolyphonyEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~PolyphonyEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {MIN_POLYPHONY, TOKEN_DOMAIN(10)},
      {MAX_POLYPHONY, TOKEN_DOMAIN(10)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_polyphony_quantile = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    update_av_polyphony_and_note_duration(p);
  }
};


class PolyphonyDurationEncoder : public ENCODER {
public:
  PolyphonyDurationEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~PolyphonyDurationEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {MIN_POLYPHONY, TOKEN_DOMAIN(10)},
      {MAX_POLYPHONY, TOKEN_DOMAIN(10)},
      {MIN_NOTE_DURATION, TOKEN_DOMAIN(6)},
      {MAX_NOTE_DURATION, TOKEN_DOMAIN(6)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_polyphony_quantile = true;
    e->mark_note_duration_quantile = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    update_av_polyphony_and_note_duration(p);
  }
};

class DurationEncoder : public ENCODER {
public:
  DurationEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~DurationEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_OFFSET, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {MIN_NOTE_DURATION, TOKEN_DOMAIN(6)},
      {MAX_NOTE_DURATION, TOKEN_DOMAIN(6)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_note_duration_quantile = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    update_av_polyphony_and_note_duration(p);
  }
};



class NewDurationEncoder : public ENCODER {
public:
  NewDurationEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~NewDurationEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
      },INT_VALUES_DOMAIN)},
      {TRACK_END, TOKEN_DOMAIN(1)},
      {INSTRUMENT, TOKEN_DOMAIN(128)},
      {NOTE_ONSET, TOKEN_DOMAIN(128)},
      {NOTE_DURATION, TOKEN_DOMAIN(96)},
      {TIME_DELTA, TOKEN_DOMAIN(48)},
      //{FILL_IN, TOKEN_DOMAIN(3)},
      {FILL_IN_PLACEHOLDER, TOKEN_DOMAIN(1)},
      {FILL_IN_START, TOKEN_DOMAIN(1)},
      {FILL_IN_END, TOKEN_DOMAIN(1)},
      {MIN_NOTE_DURATION, TOKEN_DOMAIN(6)},
      {MAX_NOTE_DURATION, TOKEN_DOMAIN(6)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_note_duration_quantile = true;
    e->use_note_duration_encoding = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
  }
};

// =======================================

class NewVelocityEncoder : public ENCODER {
public:
  NewVelocityEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~NewVelocityEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
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
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->use_note_duration_encoding = true;
    e->use_velocity_levels = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
  }
};


// =============================================

class AbsoluteEncoder : public ENCODER {
public:
  AbsoluteEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~AbsoluteEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
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
      {MAX_NOTE_DURATION, TOKEN_DOMAIN(6)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_note_duration_quantile = true;
    e->use_note_duration_encoding = true;
    e->use_absolute_time_encoding = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
  }
};



class MultiLengthEncoder : public ENCODER {
public:
  MultiLengthEncoder() {
    config = get_encoder_config();
    rep = get_encoder_rep();
  }
  ~MultiLengthEncoder() {
    delete rep;
    delete config;
  }

  REPRESENTATION *get_encoder_rep() {
    REPRESENTATION *r = new REPRESENTATION({
      {PIECE_START, TOKEN_DOMAIN(2)},
      {NUM_BARS, TOKEN_DOMAIN({1,2,4,8}, INT_VALUES_DOMAIN)},
      {BAR, TOKEN_DOMAIN(1)},
      {BAR_END, TOKEN_DOMAIN(1)},
      {TRACK, TOKEN_DOMAIN({
        midi::STANDARD_TRACK,
        midi::STANDARD_DRUM_TRACK,   
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
      {MAX_NOTE_DURATION, TOKEN_DOMAIN(6)}
    });
    return r;
  }

  EncoderConfig *get_encoder_config() {
    EncoderConfig *e = new EncoderConfig();
    e->both_in_one = true;
    e->force_instrument = true;
    e->mark_note_duration_quantile = true;
    e->use_note_duration_encoding = true;
    e->use_absolute_time_encoding = true;
    e->min_tracks = 1; // not sure this is used anywhere
    e->resolution = 12;
    return e;
  }

  void preprocess_piece(midi::Piece *p) {
    calculate_note_durations(p);
    update_av_polyphony_and_note_duration(p);
  }
};

}
// END OF NAMESPACE






