#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <set>

;

#include "encoder/encoder_all.h"
#include "enum/gm.h"
#include "enum/model_type.h"
#include "enum/encoder_types.h"

#include "protobuf/validate.h"
#include "protobuf/util.h"

static const int NUM_LAYERS = 6;

class Progress {
public:
  Progress(REPRESENTATION *r) {
    rep = r;
  }
  void update(int token) {
    if (rep->is_token_type(token,TIME_DELTA)) {
      timestep += rep->decode(token) + 1;
    }
  }
  int timestep;
  int bar;
  // set of (pitch,duration) tuples
  //set<std::tuple<int,int>> active_notes;
  REPRESENTATION *rep;
};

class Control {
public:
  Control() {
    initialize(NULL);
  }
  Control(ENCODER *e) {
    initialize(e);
  }
  void initialize(ENCODER *e) {
    if (e) {
      encoder = e;
      int vocab_size = encoder->rep->max_token();
      null_trigger = std::vector<int>(vocab_size,0);
      null_mask = std::vector<int>(vocab_size,1);
      global_mask = std::vector<int>(vocab_size,1);
    }
  }
  std::vector<int> encode(mmm::TOKEN_TYPE tt, std::vector<int> values) {
    if (tt == NONE) {
      return null_mask;
    }
    return encoder->rep->encode_to_one_hot(tt, values);
  }
  void add(mmm::TOKEN_TYPE trigger_tt, std::vector<int> trigger_values, mmm::TOKEN_TYPE mask_tt, std::vector<int> mask_values) {
    controls.push_back( std::make_pair(
      encode(trigger_tt, trigger_values), encode(mask_tt, mask_values)) );
  }
  void start(int n) {
    pos = std::vector<int>(n,0);
    bar_counts = std::vector<int>(n,0);
  }
  void show_mask(std::vector<int> mask) {
    for (const auto v : mask) {
      if (v==0) { std::cout << "-"; }
      else { std::cout << "x"; }
    }
    std::cout << std::endl;
  }
  void show() {
    for (const auto control : controls) {
      std::cout << "TRIGGER : ";
      show_mask(std::get<0>(control));
      std::cout << "MASK : ";
      show_mask(std::get<1>(control));
    }
  }
  bool is_finished(int index) {
    if (index >= pos.size()) {
      throw std::runtime_error("CONTROL INDEX OUT OF RANGE");
    }
    return pos[index] >= controls.size();
  }
  bool all_finished() {
    for (int i=0; i<pos.size(); i++) {
      if (!is_finished(i)) {
        return false;
      }
    }
    return true;
  }
  bool check_trigger(int index, int value) {
    if (is_finished(index)) {
      return false; // no more triggers
    }
    return (std::get<0>(controls[pos[index]])[value] == 1);
  }
  std::vector<int> get_mask(int index) {
    return std::get<1>(controls[pos[index]]);
  }
  void increment(int index) {
    pos[index]++;
  }
  // functions for handling the number of onsets
  void update(int token) {
    REPRESENTATION *rep = encoder->rep;
    if (rep->is_token_type(token,NOTE_ONSET)) {
      int pitch = rep->decode(token);

    }

  }

  // three functions for handling bar embeddings
  // we could do a similar thing for track embeddings
  // or just be lazy and modulo here
  void add_bar_embed(std::vector<double> embed) {
    bar_embeds.push_back( embed );
  }
  std::vector<double> get_bar_embed(int index) {
    return bar_embeds[bar_counts[index]];
  }
  void increment_bar_count(int index) {
    bar_counts[index]++;
  }
  void set_entire_global_mask(int mask_value) {
    std::fill(global_mask.begin(), global_mask.end(), mask_value);
  }
  void set_global_mask(mmm::TOKEN_TYPE tt, std::vector<int> values, int mask_value) {
    auto it = encoder->rep->token_domains.find(tt);
    if (it != encoder->rep->token_domains.end()) {
      for (const auto v : values) {
        if (v == -1) {
          for (const auto i : it->second.input_domain) {
            global_mask[encoder->rep->encode(tt,i)] = mask_value;
          }
        }
        else {
          global_mask[encoder->rep->encode(tt,v)] = mask_value;
        }
      }
    }
  }
  std::vector<int> get_global_mask() {
    return global_mask;
  }
  std::vector<std::tuple<std::vector<int>,std::vector<int>>> controls;
  ENCODER *encoder;
  std::vector<int> pos;
  std::vector<int> null_trigger; // all zeros
  std::vector<int> null_mask; // all ones
  std::vector<int> global_mask; // ban certain tokens
  
  std::vector<int> bar_counts;
  std::vector<std::vector<double>> bar_embeds;

  // members for tracking progress

};

// sample config for the common preferences
class SampleConfig {
public:
  SampleConfig() {
    batch_size = 1;
    temperature = 1.;
    verbose = false;
    max_steps = 0; // no limit
    num_bars = 4;
    encoder_type = NO_ENCODER;
    encoder = NULL;
    ckpt_path = "";
    model_type = TRACK_MODEL;
    skip_preprocess = false;
  }
  int batch_size;
  float temperature;
  bool verbose;
  int max_steps;
  int num_bars;
  bool skip_preprocess;
  MODEL_TYPE model_type;
  ENCODER_TYPE encoder_type;
  ENCODER *encoder;
  std::string ckpt_path;
  std::vector<int> order;
  std::vector<int> prompt;
  std::vector<std::vector<double>> embeds;
  Control control;
};

// helper for generating interleaved
void prepare_generate_interleaved(std::vector<midi::StatusTrack> &tracks, SampleConfig *sc) {

  if (sc->verbose) {
    std::cout << "GENERATING " << sc->num_bars << " BARS (INTERLEAVED)" << std::endl;
  }

  sc->control.initialize(sc->encoder);

  sc->control.add(PIECE_START, {0}, HEADER, {0});
  int track_num = 0;
  for (const auto track : tracks) {
    std::vector<int> insts = GM[track.instrument()];
    if (track_num==0) {
      sc->control.add(HEADER, {0}, INSTRUMENT, insts);
    }
    else {
      sc->control.add(INSTRUMENT, {-1}, INSTRUMENT, insts);
    }
    track_num++;
  }
  sc->control.add(INSTRUMENT, {-1}, HEADER, {1});
  sc->control.add(HEADER, {1}, BAR, {0}); // start with bar_start

  // make sure we generate the right number of bars
  for (int i=0; i<sc->num_bars; i++) {
    sc->control.add(BAR, {0}, NONE, {});
  }
  sc->control.add(BAR_END, {0}, NONE, {});

  if (sc->verbose) {
    sc->control.show();
  }

  sc->prompt.push_back( sc->encoder->rep->encode(PIECE_START,0) );

}


void prepare_generate_bars(std::vector<std::tuple<int,int>> &bars, midi::Piece *p, midi::Status *status, SampleConfig *sc) {

  std::vector<int> prompt;
  std::vector<std::vector<double>> embeds;
  if (p) {
    std::set<std::tuple<int,int>> barset;
    std::copy(bars.begin(), bars.end(), std::inserter(barset, barset.end()));
    sc->encoder->config->do_multi_fill = true;
    sc->encoder->config->multi_fill = barset;
    sc->encoder->config->num_bars = sc->num_bars;
    
    if (sc->encoder->config->embed_dim) {
      auto out = sc->encoder->encode_w_embeds(p);
      prompt = std::get<0>(out);
      embeds = std::get<1>(out);
    }
    else {
      if (sc->skip_preprocess) {
        prompt = sc->encoder->encode_wo_preprocess(p);
      }
      else {
        prompt = sc->encoder->encode(p);
      }
    }
    
    // remove everything after fill_start token
    int fill_start = sc->encoder->rep->encode(FILL_IN,1);
    for (int index=0; index<prompt.size(); index++) {
      if (prompt[index] == fill_start) {
        prompt.resize(index+1);
        break;
      }
    }
  }
  else {
    throw std::runtime_error("MUST PROVIDE midi::Piece FOR BAR INFILL MODE");
    prompt.push_back( sc->encoder->rep->encode(PIECE_START,0) );
  }

  sc->control.initialize(sc->encoder);
  for (int i=0; i<bars.size(); i++) {
    sc->control.add(FILL_IN, {2}, NONE, {});
    
    // get embedding for this bar and add to list of embeddings in control
    // this is only for some encodings
    // NOTE : need to fix this
    //midi::ContinuousFeature f = status->tracks(std::get<0>(bars[i])).embeds(std::get<1>(bars[i]));
    //sc->control.add_bar_embed( sc->encoder->convert_feature(f) );

  }
  if (sc->verbose) {
    sc->control.show();
  }

  // certain tokens should never appear when generating with infill mode
  // we only need onset, offset, time_delta and fill_in tokens
  sc->control.set_entire_global_mask(0); // mask everything
  sc->control.set_global_mask(NOTE_ONSET,{-1},1); // allow note onsets
  sc->control.set_global_mask(NOTE_OFFSET,{-1},1); // allow note offsets
  sc->control.set_global_mask(NOTE_DURATION,{-1},1); // allow note offsets
  sc->control.set_global_mask(FILL_IN,{1,2},1); // allow some FILL_IN
  sc->control.set_global_mask(TIME_DELTA,{1,2},1); // allow time delta

  // copy prompt and control into sample config
  sc->prompt = prompt;
  if (embeds.size()) {
    sc->embeds = embeds;
  }
}

void prepare_generate_tracks(std::vector<midi::StatusTrack> &tracks, midi::Piece *p, SampleConfig *sc) {
    
  sc->control.initialize(sc->encoder);

  int track_num = 0;
  bool piece_is_empty = p->tracks_size()==0;
  for (const auto track : tracks) {
    std::vector<int> insts = GM[track.instrument()];
    int track_type = track.track_type();
    int density = track.density();

    if (track_type == STANDARD_BOTH) {
      track_type = -1;
    }

    if (sc->verbose) {
      std::cout << "GENERATING : " << track_type << " with density " << density << std::endl;
    }
    
    for (int i=0; i<insts.size(); i++) {
      insts[i] %= 128;
    }
    if ((track_num == 0) && (piece_is_empty)) {
      sc->control.add(PIECE_START, {0}, TRACK, {track_type});
    }
    else {
      sc->control.add(TRACK_END, {0}, TRACK, {track_type});
    }
    
    if (sc->encoder->config->mark_genre) {
      //int genre = sc->encoder->rep->encode(GENRE, track.genre());
      // hacky way to get value of token
      //genre = std::get<1>(sc->encoder->rep->backward[genre]); 

      int genre = sc->encoder->rep->encode_partial(GENRE, track.genre());
      sc->control.add(TRACK, {-1}, GENRE, {genre});
      sc->control.add(GENRE, {-1}, INSTRUMENT, insts);
    }
    else {
      sc->control.add(TRACK, {-1}, INSTRUMENT, insts);
    }

    // density control
    if (density >= 0) {
      sc->control.add(INSTRUMENT, {-1}, DENSITY_LEVEL, {density});
    }
    
    track_num++;
  }
  sc->control.add(TRACK_END, {0}, NONE, {});

  if (sc->verbose) {
    sc->control.show();
  }

  std::vector<int> prompt;
  if (!piece_is_empty) {
    std::cout << "ENCODING" << std::endl;
    prompt = sc->encoder->encode(p);
    std::cout << "SUCCESS" << std::endl;
  }
  else {
    prompt.push_back( sc->encoder->rep->encode(PIECE_START,0) );
  }

  // copy prompt and control into sample config
  sc->prompt = prompt;
}


//tuple<std::vector<int>,Control,ENCODER_TYPE,std::string,std::vector<int>> 
void prepare_generate(midi::Status *status, midi::Piece *piece, SampleConfig *sc, std::map<std::tuple<int,MODEL_TYPE>,std::tuple<ENCODER_TYPE,std::string>> &ckpt_map) {

  if (sc->verbose) {
    std::cout << "PREPARE GENERATION" << std::endl;
  }
  validate_status(status, piece, sc->verbose);
  update_has_notes(piece);

  //std::vector<std::tuple<int,std::string,int>> tracks;
  std::vector<midi::StatusTrack> tracks;
  std::vector<std::tuple<int,int>> bars;
  int num_cond_tracks = 0;
  int num_resample_tracks = 0;
  int num_infill_tracks = 0;
  std::vector<STATUS_TRACK_TYPE> track_types;
  std::vector<int> order;
  std::vector<int> cond_tracks;

  int track_num = 0;
  for (const auto track : status->tracks()) {
    STATUS_TRACK_TYPE tt = infer_track_type(track);
    // WARNING :: THIS IS AN OVERIDE
    if (tt == RESAMPLE) {
      tt = INFILL;
    }
    // WARNING :: THIS IS AN OVERIDE
    switch( tt ) {
      case CONDITION :
        order.push_back( num_cond_tracks );
        cond_tracks.push_back( track.track_id() );
        num_cond_tracks++;
        break;
      case RESAMPLE : 
        order.push_back( num_resample_tracks );
        tracks.push_back( track );
        num_resample_tracks++;
        break;
      case INFILL :     
        num_infill_tracks++;
        break;
    }
    track_types.push_back( tt );
    int bar_num = 0;
    for (const auto selected : track.selected_bars()) {
      if (selected) {
        bars.push_back( std::make_pair(track_num, bar_num) );
      }
      bar_num++;
    }
    track_num++;
  }

  // provide overview of tracks for sampling
  if (sc->verbose) {
    int track_num = 0;
    for (const auto track_type : track_types) {
      std::cout << "TRACK " << track_num << " -> " << track_type << std::endl;
      track_num++;
    }
  }

  int num_bars = status->tracks(0).selected_bars_size();

  std::tuple<int,MODEL_TYPE> model_key = std::make_tuple(num_bars,TRACK_MODEL);
  if (num_infill_tracks > 0) {
    model_key = std::make_tuple(num_bars,BAR_INFILL_MODEL);
  }
  std::tuple<ENCODER_TYPE,std::string> ckpt_info = ckpt_map[model_key];

  //SampleConfig sc;
  //sc.temperature = temp;
  //sc.batch_size = batch_size;
  //sc.verbose = verbose;

  sc->num_bars = num_bars;
  sc->encoder_type = std::get<0>(ckpt_info);
  sc->encoder = getEncoder(sc->encoder_type);
  sc->ckpt_path = std::get<1>(ckpt_info);

  if (sc->encoder_type == TRACK_INTERLEAVED_W_HEADER_ENCODER) {
    prepare_generate_interleaved(tracks, sc);
  }
  else if (num_infill_tracks > 0) {

    if (sc->verbose) {
      std::cout << "GENERATING " << bars.size() << " BARS" << std::endl;
    }

    sc->encoder->config->do_multi_fill = true;

    // remove excess bars if any
    prune_tracks_dev2(
      piece, arange(0,piece->tracks_size(),1), arange(0,sc->num_bars,1));

    return prepare_generate_bars(bars, piece, status, sc);

  }
  else {

    if (sc->verbose) {
      std::cout << "GENERATING " << num_resample_tracks << " TRACKS" << std::endl;
    }

    sc->encoder->config->do_multi_fill = false;

    // fix the order
    // order is the output position for each track
    for (track_num=0; track_num<status->tracks_size(); track_num++) {
      if (track_types[track_num] == RESAMPLE) {
        order[track_num] = order[track_num] + num_cond_tracks;
      }
    }
    std::vector<int> inverse_order(order.size(),0);
    for (int i=0; i<order.size(); i++) {
      inverse_order[order[i]] = i;
    }

    sc->order = inverse_order;

    // prune unneeded tracks
    prune_tracks_dev2(piece, cond_tracks, arange(0,sc->num_bars,1));

    // call generation
    return prepare_generate_tracks(tracks, piece, sc);

    // reorder the tracks
    //for (int i=0; i<output.size(); i++) {
    //  reorder_tracks(&(output[i]), inverse_order);
    //}

  }
}

std::tuple<std::vector<int>,Control,ENCODER_TYPE,std::string,std::vector<int>> prepare_generate_py(std::string &status_str, std::string &piece_str, float temp, int batch_size, bool verbose, std::map<std::tuple<int,MODEL_TYPE>,std::tuple<ENCODER_TYPE,std::string>> &ckpt_map) {
  midi::Piece piece;
  google::protobuf::util::JsonStringToMessage(piece_str.c_str(), &piece);
  midi::Status status;
  google::protobuf::util::JsonStringToMessage(status_str.c_str(), &status);
  // make a sample config
  SampleConfig sc;
  sc.temperature = temp;
  sc.batch_size = batch_size;
  sc.verbose = verbose;
  prepare_generate(&status, &piece, &sc, ckpt_map);
  return std::make_tuple(sc.prompt, sc.control, sc.encoder_type, sc.ckpt_path, sc.order);
}

