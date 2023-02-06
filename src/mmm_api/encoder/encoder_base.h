#pragma once

#include "representation.h"
#include "util.h"

#include "../protobuf/midi.pb.h"
#include "../enum/encoder_config.h"
#include "../enum/train_config.h"
#include "../midi_io.h"

// START OF NAMESPACE
namespace mmm {

template<class T>
using matrix = std::vector<std::vector<T>>;

template<class T>
using tensor = std::vector<std::vector<std::vector<T>>>;

std::vector<int> resolve_bar_infill_tokens(std::vector<int> &raw_tokens, REPRESENTATION *rep) {
  int fill_pholder = rep->encode(FILL_IN_PLACEHOLDER, 0);
  int fill_start = rep->encode(FILL_IN_START, 0);
  int fill_end = rep->encode(FILL_IN_END, 0);

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
  return tokens;
}


class ENCODER {
public:

  virtual REPRESENTATION *get_encoder_rep() {
    throw std::runtime_error("ENCODER CLASS MUST DEFINE get_encoder_rep()");
  }

  virtual EncoderConfig *get_encoder_config() {
    throw std::runtime_error("ENCODER CLASS MUST DEFINE get_encoder_config()");
  }

  //virtual std::vector<int> encode(midi::Piece *p) {}
  //virtual std::tuple<std::vector<int>,std::vector<std::vector<double>>> encode_w_features(midi::Piece *p) {}
  //virtual std::vector<double> convert_feature(midi::ContinuousFeature f) {}
  //virtual void decode(std::vector<int> &tokens, midi::Piece *p) {}
  
  

  // simple function for empty embeddings
  std::vector<double> empty_embedding() {
    return std::vector<double>(config->embed_dim,0);
  }

  virtual void preprocess_piece(midi::Piece *p) {
    // default is to do nothing
  }

  std::vector<int> encode(midi::Piece *p) {
    preprocess_piece(p);
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
  }

  /*
  std::vector<int> encode_for_generation(midi::Piece *p, midi::Status *s,  MODEL_TYPE mt) {
    if (mt == TRACK_MODEL) {
      // we don't need to do anything as everything will be determined in generation

    }

  }
  */

  std::vector<int> encode_wo_preprocess(midi::Piece *p) {
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    return ts.tokens;
  }

  virtual std::vector<double> convert_feature(midi::ContinuousFeature f) {
    // default is to return single zero
    return std::vector<double>(1,0);
  }

  std::tuple<std::vector<int>,matrix<double>> encode_w_embeds(midi::Piece *p) {
    preprocess_piece(p);
    TokenSequence ts = to_performance_w_tracks_dev(p, rep, config);
    matrix<double> embeds;
    for (auto f : ts.features) {
      embeds.push_back( convert_feature(f) );
    }
    return make_tuple(ts.tokens, embeds);
  }

  virtual void decode(std::vector<int> &tokens, midi::Piece *p) {
    if (config->do_multi_fill == true) {
      tokens = resolve_bar_infill_tokens(tokens, rep);
    }
    return decode_track_dev(tokens, p, rep, config);
  }

  std::string midi_to_json(std::string &filepath) {
    midi::Piece p;
    parse_new(filepath, &p, config);
    preprocess_piece(&p); // add features that the encoder may need
    std::string json_string;
    google::protobuf::util::MessageToJsonString(p, &json_string);
    return json_string;
  }

  void midi_to_piece(std::string &filepath, midi::Piece *p) {
    parse_new(filepath, p, config);
  }

  //void piece_to_mid(imidi::Piece *p, std::string &filepath) {
  //  write_midi(p, filepath);
  //}

  //std::vector<double> midi_to_feature(std::string &filepath) {
  //  midi::Piece p;
  //  parse_new(filepath, &p, config);
  //  return piece_to_quantize_info(&p);
  //}
  
  #ifdef PYBIND
  py::bytes midi_to_json_bytes(std::string &filepath, TrainConfig *tc, std::string &genre_data) {
    std::string x;
    midi::Piece p;
    parse_new(filepath, &p, config, NULL);
    update_valid_segments(&p, tc->num_bars, tc->min_tracks, config->te);
    if (!p.internal_valid_segments_size()) {
      return py::bytes(x); // empty bytes
    }
    // insert genre data in here
    midi::GenreData g;
    google::protobuf::util::JsonStringToMessage(genre_data.c_str(), &g);
    midi::GenreData *gd = p.add_internal_genre_data();
    gd->CopyFrom(g);
    // insert genre data in here
    p.SerializeToString(&x);
    return py::bytes(x);
  }
  #endif
  

  std::vector<int> midi_to_tokens(std::string &filepath) {
    midi::Piece p;
    parse_new(filepath, &p, config);
    return encode(&p);
  }

  void json_to_midi(std::string &json_string, std::string &filepath) {
    midi::Piece p;
    google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);
    write_midi(&p, filepath, -1);
  }

  void json_track_to_midi(std::string &json_string, std::string &filepath, int single_track) {
    midi::Piece p;
    google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);
    write_midi(&p, filepath, single_track);
  }

  std::vector<int> json_to_tokens(std::string &json_string) {
    midi::Piece p;
    google::protobuf::util::JsonStringToMessage(json_string.c_str(), &p);
    return encode(&p); 
  }

  std::string tokens_to_json(std::vector<int> &tokens) {
    midi::Piece p;
    decode(tokens, &p);
    std::string json_string;
    google::protobuf::util::MessageToJsonString(p, &json_string);
    return json_string;
  }

  void tokens_to_json_array(std::vector<std::vector<int>> &seqs, std::vector<midi::Piece> &output) {
    for (int i=0; i<seqs.size(); i++) {
      decode(seqs[i], &(output[i]));
    }
  }

  void tokens_to_midi(std::vector<int> &tokens, std::string &filepath) {
    midi::Piece p;
    decode(tokens, &p);
    write_midi(&p, filepath, -1);
  }

  EncoderConfig *config;
  REPRESENTATION *rep;
};

}
// END OF NAMESPACE