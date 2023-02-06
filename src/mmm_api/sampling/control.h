// this code tracks progress

// constrain bars per track
// constrain timesteps per bar
// constrain max polyphony
// constrain offsets to notes that have been onset

#pragma once

#include <map>
#include <tuple>
#include <set>
#include <vector>

#include "../encoder/representation.h"
#include "../encoder/encoder_base.h"
#include "../enum/token_types.h"
#include "../enum/encoder_types.h"
#include "callback_base.h"

namespace mmm {

// this map is used to force model to use particular notes
// for opz tracks

const std::map<midi::TRACK_TYPE,std::vector<int>> OPZ_NOTE_DOMAINS = {
  {midi::OPZ_KICK_TRACK, {35,36}},
  {midi::OPZ_SNARE_TRACK, {37,38,39,40}},
  {midi::OPZ_HIHAT_TRACK, {42,44,46,49,51,55,57,59}}
};

// need to pass in a class that grabs tokens at each step so that
// we can do checks on the model
class Debugger {
public:
  std::vector<std::vector<int>> tokens;
  std::vector<std::vector<int>> orders;
  std::vector<MODEL_TYPE> modes;
  std::vector<std::vector<bool>> is_autoregressive;
};

using TOKEN_EDGE = std::pair<mmm::TOKEN_TYPE,mmm::TOKEN_TYPE>;
using CKPT_MAP_TYPE = std::map<std::tuple<int,MODEL_TYPE>,std::tuple<ENCODER_TYPE,std::string>>;

std::vector<std::vector<mmm::TOKEN_TYPE>> DEFAULT_ABSOLUTE_GRAPH = {
  {PIECE_START, NUM_BARS, TRACK},
  
  {TRACK, GENRE, INSTRUMENT},

  {INSTRUMENT, DENSITY_LEVEL, MIN_POLYPHONY, MAX_POLYPHONY, MIN_NOTE_DURATION, MAX_NOTE_DURATION, BAR},

  {BAR, TIME_SIGNATURE},

  {TIME_SIGNATURE, TIME_ABSOLUTE},
  {TIME_SIGNATURE, VELOCITY_LEVEL},
  {TIME_SIGNATURE, FILL_IN_PLACEHOLDER},

  //{BAR, TIME_ABSOLUTE},
  //{BAR, NOTE_ONSET},
  //{BAR, VELOCITY_LEVEL},
  //{BAR, FILL_IN_PLACEHOLDER},
  //{BAR, BAR_END}, // if you remove this no empty bars
  
  {FILL_IN_PLACEHOLDER, BAR_END},

  {VELOCITY_LEVEL, NOTE_ONSET},

  {NOTE_ONSET, NOTE_DURATION},

  {NOTE_DURATION, TIME_ABSOLUTE},
  {NOTE_DURATION, NOTE_ONSET},
  {NOTE_DURATION, VELOCITY_LEVEL},
  {NOTE_DURATION, BAR_END},
  {NOTE_DURATION, FILL_IN_END},

  {TIME_ABSOLUTE, NOTE_ONSET},
  {TIME_ABSOLUTE, VELOCITY_LEVEL},
  {TIME_ABSOLUTE, BAR_END},
  {TIME_ABSOLUTE, FILL_IN_END},

  {BAR_END, BAR},
  {BAR_END, TRACK_END},

  {TRACK_END, TRACK},
  {TRACK_END, FILL_IN_START},

  //{FILL_IN_START, FILL_IN_END}, // remove to ban empty bars
  {FILL_IN_START, TIME_ABSOLUTE},
  //{FILL_IN_START, NOTE_ONSET},
  {FILL_IN_START, VELOCITY_LEVEL},

  {FILL_IN_END, FILL_IN_START},

};

std::vector<std::vector<mmm::TOKEN_TYPE>> DEFAULT_DURATION_GRAPH = {
  {PIECE_START, TRACK},
  
  {TRACK, GENRE, INSTRUMENT},

  {INSTRUMENT, DENSITY_LEVEL, MIN_POLYPHONY, MAX_POLYPHONY, MIN_NOTE_DURATION, MAX_NOTE_DURATION, BAR},

  {BAR, TIME_DELTA},
  //{BAR, NOTE_ONSET},
  {BAR, VELOCITY_LEVEL},
  {BAR, FILL_IN_PLACEHOLDER},
  //{BAR, BAR_END},
  
  {FILL_IN_PLACEHOLDER, BAR_END},

  {VELOCITY_LEVEL, NOTE_ONSET},

  {NOTE_ONSET, NOTE_DURATION},

  {NOTE_DURATION, TIME_DELTA},
  {NOTE_DURATION, NOTE_ONSET},
  {NOTE_DURATION, VELOCITY_LEVEL},
  {NOTE_DURATION, BAR_END},
  {NOTE_DURATION, FILL_IN_END},

  {TIME_DELTA, TIME_DELTA}, // THIS NEEDS A FIX
  {TIME_DELTA, NOTE_ONSET},
  {TIME_DELTA, VELOCITY_LEVEL},
  {TIME_DELTA, BAR_END},
  {TIME_DELTA, FILL_IN_END},

  {BAR_END, BAR},
  {BAR_END, TRACK_END},

  {TRACK_END, TRACK},
  {TRACK_END, FILL_IN_START},

  {FILL_IN_START, FILL_IN_END},
  {FILL_IN_START, TIME_DELTA},
  //{FILL_IN_START, NOTE_ONSET},
  {FILL_IN_START, VELOCITY_LEVEL},

  {FILL_IN_END, FILL_IN_START},

};

std::vector<std::vector<mmm::TOKEN_TYPE>> DEFAULT_EVENT_GRAPH = {
  {PIECE_START, TRACK},
  
  {TRACK, GENRE, INSTRUMENT},
  
  {INSTRUMENT, DENSITY_LEVEL, MIN_POLYPHONY, MAX_POLYPHONY, MIN_NOTE_DURATION, MAX_NOTE_DURATION, BAR},

  {BAR, TIME_DELTA},
  {BAR, NOTE_ONSET},
  {BAR, FILL_IN_PLACEHOLDER},
  {BAR, BAR_END},

  {FILL_IN_PLACEHOLDER, BAR_END},

  {NOTE_ONSET, TIME_DELTA},
  {NOTE_ONSET, NOTE_ONSET},
  {NOTE_ONSET, BAR_END},
  {NOTE_ONSET, FILL_IN_END},

  {NOTE_OFFSET, TIME_DELTA},
  {NOTE_OFFSET, NOTE_ONSET},
  {NOTE_OFFSET, NOTE_OFFSET},
  {NOTE_OFFSET, BAR_END},
  {NOTE_OFFSET, FILL_IN_END},

  {TIME_DELTA, TIME_DELTA}, // THIS NEEDS A FIX
  {TIME_DELTA, NOTE_ONSET},
  {TIME_DELTA, NOTE_OFFSET},
  {TIME_DELTA, BAR_END},
  {TIME_DELTA, FILL_IN_END},

  {BAR_END, BAR},
  {BAR_END, TRACK_END},

  {TRACK_END, TRACK},
  {TRACK_END, FILL_IN_START},

  {FILL_IN_START, FILL_IN_END},
  {FILL_IN_START, TIME_DELTA},
  {FILL_IN_START, NOTE_ONSET},

  {FILL_IN_END, FILL_IN_START},

};

class REP_NODE {
public:
  REP_NODE(mmm::TOKEN_TYPE x) {
    tt = x;
  }
  std::set<mmm::TOKEN_TYPE> edges;
  std::set<mmm::TOKEN_TYPE> in_edges;
  mmm::TOKEN_TYPE tt;
};

class REP_GRAPH {
public:
  REP_GRAPH(ENCODER *e, MODEL_TYPE mt) {
    enc = e;
    initialize(mt);
  }
  //REP_GRAPH(ENCODER_TYPE et, MODEL_TYPE mt) {
  //  enc = getEncoder(et);
  //  initialize(mt);
  //}
  void initialize(MODEL_TYPE mt) {
    if (enc->config->use_absolute_time_encoding) {
      build_from_paths(DEFAULT_ABSOLUTE_GRAPH);
    }
    else if (enc->config->use_note_duration_encoding) {
      build_from_paths(DEFAULT_DURATION_GRAPH);
    }
    else {
      build_from_paths(DEFAULT_EVENT_GRAPH);
    }

    std::vector<mmm::TOKEN_TYPE> to_remove;
    for (const auto kv : nodes) {
      if (!enc->rep->has_token_type(kv.first)) {
        to_remove.push_back(kv.first);
      }
    }
    remove_nodes(to_remove);

    if (mt == TRACK_MODEL) {
      initialize_autoregressive();
    }
    else {
      initialize_bar_infilling();
    }
  }
  void initialize_bar_infilling() {
    std::set<mmm::TOKEN_TYPE> to_keep = {
      VELOCITY_LEVEL,
      NOTE_ONSET, 
      NOTE_OFFSET, 
      NOTE_DURATION,
      TIME_DELTA,
      TIME_ABSOLUTE,
      FILL_IN_START,
      FILL_IN_END,
    };
    std::vector<mmm::TOKEN_TYPE> to_remove;
    for (const auto kv : nodes) {
      if (to_keep.find(kv.first) == to_keep.end()) {
        to_remove.push_back(kv.first);
      }
    }
    remove_nodes_wo_connecting(to_remove);
  }
  void initialize_autoregressive() {
    std::vector<mmm::TOKEN_TYPE> to_remove = {
      FILL_IN_PLACEHOLDER,
      FILL_IN_START,
      FILL_IN_END
    };
    remove_nodes_wo_connecting(to_remove);
  }
  void remove_edges_to_node(mmm::TOKEN_TYPE v) {
    for (auto kv : nodes) {
      nodes.find(kv.first)->second.edges.erase(v);
      nodes.find(kv.first)->second.in_edges.erase(v);
    }
  }
  void remove_node(mmm::TOKEN_TYPE v) {
    // connect paths
    std::set<mmm::TOKEN_TYPE> out_edges = nodes.find(v)->second.edges;
    for (const auto pre : nodes.find(v)->second.in_edges) {
      for (const auto e : out_edges) {
        nodes.find(pre)->second.edges.insert( e );
      }
      nodes.find(pre)->second.edges.erase( v );
    }
    std::set<mmm::TOKEN_TYPE> in_edges = nodes.find(v)->second.in_edges;
    for (const auto post : nodes.find(v)->second.edges) {
      for (const auto e : in_edges) {
        nodes.find(post)->second.in_edges.insert( e );
      }
      nodes.find(post)->second.in_edges.erase( v );
    }
    // remove node
    remove_edges_to_node(v);
    nodes.erase(v);
  }
  void remove_nodes(std::vector<mmm::TOKEN_TYPE> &vs) {
    for (const auto v : vs) {
      remove_node(v);
    }
  }
  void remove_nodes_wo_connecting(std::vector<mmm::TOKEN_TYPE> &vs) {
    for (const auto v : vs) {
      remove_edges_to_node(v);
      nodes.erase(v);
    }
  }
  void add_node(mmm::TOKEN_TYPE v) {
    if (nodes.find(v) == nodes.end()) {
      nodes.insert( std::make_pair(v,REP_NODE(v)) );
    }
  }
  void add_edge(mmm::TOKEN_TYPE u, mmm::TOKEN_TYPE v) {
    add_node(u);
    add_node(v);
    nodes.find(u)->second.edges.insert(v);
    nodes.find(v)->second.in_edges.insert(u);
  }
  void build(std::vector<TOKEN_EDGE> &edges) {
    for (const auto edge : edges) {
      add_edge( std::get<0>(edge), std::get<1>(edge) );
    }
  }
  void add_path(const std::vector<mmm::TOKEN_TYPE> &path) {
    for (int i=0; i<(int)path.size()-1; i++) {
      add_edge(path[i], path[i+1]);
    }
  }
  void build_from_paths(std::vector<std::vector<mmm::TOKEN_TYPE>> &paths) {
    for (const auto path : paths) {
      add_path(path);
    }
  }
  void check_edge(mmm::TOKEN_TYPE u, mmm::TOKEN_TYPE v) {
    auto uit = nodes.find(u);
    if (uit == nodes.end()) {
      throw std::runtime_error("INVALID NODE");
    }
    auto vit = nodes.find(v);
    if (uit == nodes.end()) {
      throw std::runtime_error("INVALID NODE");
    }
    auto eit = nodes.find(u)->second.edges.find(v);
    if (eit == nodes.find(u)->second.edges.end()) {
      std::ostringstream buffer;
      buffer << "INVALID EDGE " << toString(u) << "-->" << toString(v);
      throw std::runtime_error(buffer.str());
    }
  }
  void show() {
    // quick way to see the structure of the graph
    for (const auto kv : nodes) {
      for (const auto e : kv.second.edges) {
        std::cout << "(N) " << toString(kv.second.tt) << " ---> " << toString(e) << std::endl;
      }
      for (const auto e : kv.second.in_edges) {
        std::cout << toString(e) << " ---> (N) " << toString(kv.second.tt) << std::endl;
      }
    }
  }
  void validate_sequence(std::vector<int> &tokens) {
    for (int i=0; i<(int)tokens.size()-1; i++) {
      check_edge(
        enc->rep->get_token_type(tokens[i]),
        enc->rep->get_token_type(tokens[i+1]));
    }
  }
  void set_mask(int last_token, std::vector<int> &mask) {
    mmm::TOKEN_TYPE tt = enc->rep->get_token_type(last_token);
    auto it = nodes.find(tt);
    if (it == nodes.end()) {
      std::ostringstream buffer;
      buffer << "ERROR : INVALID NODE IN REP GRAPH (" << toString(tt) << ")";
      throw std::runtime_error(buffer.str());
    }
    for (const auto e : it->second.edges) {

      enc->rep->set_mask(e, {-1}, mask, 1);
    }
  }
  std::vector<int> get_mask(int last_token) {
    std::vector<int> mask(enc->rep->max_token(), 0);
    set_mask(last_token, mask);
    return mask;
  }
  std::set<mmm::TOKEN_TYPE> all_token_types;
  ENCODER *enc;
  //std::unique_ptr<ENCODER> enc;
  REPRESENTATION *rep;
  std::map<mmm::TOKEN_TYPE,REP_NODE> nodes;
};







class SAMPLE_CONTROL {
public:
  SAMPLE_CONTROL(midi::Piece *piece, midi::Status *status, midi::SampleParam *param, midi::ModelMetadata *meta) {    

    verbose = param->verbose();
    //polyphony_hard_limit = param->polyphony_hard_limit();
    
    initialize(piece, status, param, meta);

    // initialize() will set enc
    rep = enc->rep;
    rg = new REP_GRAPH(enc.get(), model_type);

    if (!rg) {
      std::runtime_error("REG GRAPH CONSTRUCTOR FAILED");
    }

    parse_status(status);
    initialize_members();

  }

  ~SAMPLE_CONTROL() {
    delete rg;
  }

  void initialize_members() {
    barlength = 4 * enc->config->resolution;
    timestep = 0;
    absolute_timestep = 0;
    bar_count = 0;
    track_count = 0;
    infill_bar_count = 0;
    finished = false;
    token_position = 0;
  }

  void set_bar_infill_prompt(std::vector<std::tuple<int,int>> &bars, midi::Piece *p, midi::Status *status, midi::SampleParam *param) {

    if (p) {
      std::set<std::tuple<int,int>> barset;
      std::copy(bars.begin(), bars.end(), std::inserter(barset, barset.end()));
      enc->config->do_multi_fill = true;
      enc->config->multi_fill = barset;
      enc->config->num_bars = status->tracks(0).selected_bars_size();
      
      if (enc->config->embed_dim) {
        auto out = enc->encode_w_embeds(p);
        prompt = std::get<0>(out);
        embeds = std::get<1>(out);
      }
      else {
        // if any of the attributes is NONE
        // we need to preprocess
        //if 

        if (param->internal_skip_preprocess()) {
          calculate_note_durations(p); // just in case
          prompt = enc->encode_wo_preprocess(p);
        }
        else {
          prompt = enc->encode(p);
        }
      }

      if (param->verbose()) {
        std::cout << "FULL PROMPT " << std::endl;
        for (int i=0; i<prompt.size(); i++) {
          std::cout << enc->rep->pretty(prompt[i]) << std::endl;
        }
        std::cout << "FULL PROMPT " << std::endl;
      }
      
      // remove everything after fill_start token
      int fill_start = enc->rep->encode(FILL_IN_START,0);
      for (int index=0; index<prompt.size(); index++) {
        if (prompt[index] == fill_start) {
          prompt.resize(index+1);
          break;
        }
      }
    }
    else {
      throw std::runtime_error("MUST PROVIDE midi::Piece FOR BAR INFILL MODE");
    }
  }

  void set_autoregressive_prompt(std::vector<midi::StatusTrack> &tracks, midi::Piece *p, midi::Status *status, midi::SampleParam *param) {

    enc->config->do_multi_fill = false;

    if (p->tracks_size()) {
      prompt = enc->encode(p);
    }
    else {
      prompt.push_back( enc->rep->encode(PIECE_START,0) );
    }
  }

  void initialize(midi::Piece *piece, midi::Status *status, midi::SampleParam *param, midi::ModelMetadata *meta) {

    if (param->verbose()) {
      std::cout << "PREPARE GENERATION" << std::endl;
    }
    validate_inputs(piece, status, param);
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
    if (param->verbose()) {
      int track_num = 0;
      for (const auto track_type : track_types) {
        std::cout << "TRACK " << track_num << " -> " << track_type << std::endl;
        track_num++;
      }
    }

    // select the correct model
    int nb = status->tracks(0).selected_bars_size();
    //std::tuple<int,MODEL_TYPE> model_key = std::make_tuple(nb,TRACK_MODEL);
    //if (num_infill_tracks > 0) {
    //  model_key = std::make_tuple(nb,BAR_INFILL_MODEL);
    //}
    //std::tuple<ENCODER_TYPE,std::string> ckpt_info = ckpt_map[model_key];

    //enc = getEncoder(std::get<0>(ckpt_info));
    //ckpt_path = std::get<1>(ckpt_info);

    enc = getEncoder(getEncoderType(meta->encoder()));

    if (num_infill_tracks > 0) {
      
      model_type = BAR_INFILL_MODEL;

      if (param->verbose()) {
        std::cout << "GENERATING " << bars.size() << " BARS" << std::endl;
      }

      // remove excess bars if any
      prune_tracks_dev2(
        piece, arange(0,piece->tracks_size(),1), arange(0,nb,1));

      //print_piece_summary(piece);
      //print_status(status);

      // here track ordering are preserved
      inverse_order = arange(piece->tracks_size());

      set_bar_infill_prompt(bars, piece, status, param);

    }
    else {

      model_type = TRACK_MODEL;

      if (param->verbose()) {
        std::cout << "GENERATING " << num_resample_tracks << " TRACKS" << std::endl;
      }

      // fix the order
      // order is the output position for each track
      for (track_num=0; track_num<status->tracks_size(); track_num++) {
        if (track_types[track_num] == RESAMPLE) {
          order[track_num] = order[track_num] + num_cond_tracks;
        }
      }
      //std::vector<int> inverse_order(order.size(),0);
      inverse_order.resize(order.size());
      for (int i=0; i<order.size(); i++) {
        inverse_order[order[i]] = i;
      }

      //sc->order = inverse_order;

      // prune unneeded tracks
      prune_tracks_dev2(piece, cond_tracks, arange(0,nb,1));

      if (param->verbose()) {
        std::cout << "AFTER PRUNE TRACKS DEV ...." << std::endl;
        print_piece_summary(piece);
        std::cout << "============================" << std::endl;
      }

      set_autoregressive_prompt(tracks, piece, status, param);

    }
  }

  void finalize(midi::Piece *piece) {
    if (model_type == TRACK_MODEL) {
      reorder_tracks(piece, inverse_order);
    }
  }

  void parse_time_signatures(midi::Piece *piece) {
    // we have validated piece so we know time signatures are
    // identical accross tracks. we can just get time-sigs from first
    
    

  }

  void parse_status(midi::Status *status) {

    if (verbose) {
      std::cout << "PARSING STATUS ..." << std::endl;
    }

    // for bar-infilling we have to determine one thing
    // 1) the number of bars to be infilled
    int tnum = 0;
    num_infill_bars = 0;
    for (const auto track : status->tracks()) {
      int bnum = 0;
      for (const auto bar : track.selected_bars()) {
        // keep track of selected bars
        selected_bars.push_back( std::pair(tnum,bnum) );
        num_infill_bars += (int)bar;
        bnum++;
      }
      tnum++;
    }

    // for track generation we have to determine two things
    // 1) the number of bars per track
    // 2) the token restrictions for attribute control
    //    for this we can use a static mask for each track
    //    as there should be no overlap
    num_bars = status->tracks(0).selected_bars_size();
    num_tracks = status->tracks_size();
    
    //for (const auto track : status->tracks()) {
    for (int i=0; i<status->tracks_size(); i++) {
      midi::StatusTrack track = status->tracks(inverse_order[i]);
      std::vector<int> mask(rep->max_token(),0);

      // add polyphony hard limit
      polyphony_hard_limits.push_back( track.polyphony_hard_limit() );

      // add per-track temperature
      track_temperatures.push_back( track.temperature() );

      // num bars
      rep->set_mask(NUM_BARS, {track.selected_bars_size()}, mask, 1);
      
      // track type
      int tt = track.track_type();
      if (tt == midi::STANDARD_BOTH) {
        rep->set_mask(TRACK, {-1}, mask, 1);
      }
      else {
        rep->set_mask(TRACK, {tt}, mask, 1);
      }

      // depending on track type
      // notes might need to be restricted
      bool pitch_subset = false;
      if (tt == midi::OPZ_KICK_TRACK) {
        rep->set_mask(NOTE_ONSET, {35,36}, mask, 1);
        rep->set_mask(NOTE_OFFSET, {35,36}, mask, 1);
        pitch_subset = true;
      }
      else if (tt == midi::OPZ_SNARE_TRACK) {
        rep->set_mask(NOTE_ONSET, {37,38,39,40}, mask, 1);
        rep->set_mask(NOTE_OFFSET, {37,38,39,40}, mask, 1);
        pitch_subset = true;
      }
      else if (tt == midi::OPZ_HIHAT_TRACK) {
        rep->set_mask(NOTE_ONSET, {42,44,46,49,51,55,57,59}, mask, 1);
        rep->set_mask(NOTE_OFFSET, {42,44,46,49,51,55,57,59}, mask, 1);
        pitch_subset = true;
      }      

      // genre
      std::string genre = track.internal_genre();
      rep->set_mask(GENRE, {genre}, mask, 1, STRING_VECTOR);

      // instrument
      std::vector<int> insts = GM_MOD[track.instrument()];
      if ((enc->config->do_pretrain_map) && (tt == midi::STANDARD_TRACK)) {
        // on drum tracks we don't map instruments
        for (int i=0; i<insts.size(); i++) {
          auto it = PRETRAIN_GROUPING_V2.find(insts[i]);
          if (it != PRETRAIN_GROUPING_V2.end()) {
            insts[i] = it->second;
          }
          else {
            throw std::runtime_error("CAN NOT FIND INSTRUMENT IN PRETRAIN GROUPING");
          }
        }
      }
      rep->set_mask(INSTRUMENT, insts, mask, 1);

      // density level
      rep->set_mask(DENSITY_LEVEL, {track.density()-1}, mask, 1);
      
      // min-max polyphony
      rep->set_mask(MIN_POLYPHONY, {track.min_polyphony_q()-1}, mask, 1);
      rep->set_mask(MAX_POLYPHONY, {track.max_polyphony_q()-1}, mask, 1);

      // min-max duration
      rep->set_mask(
        MIN_NOTE_DURATION, {track.min_note_duration_q()-1}, mask, 1);
      rep->set_mask(
        MAX_NOTE_DURATION, {track.max_note_duration_q()-1}, mask, 1);

      std::set<mmm::TOKEN_TYPE> fixed = {
        NUM_BARS,
        TRACK,
        GENRE,
        INSTRUMENT,
        DENSITY_LEVEL,
        MIN_POLYPHONY,
        MAX_POLYPHONY,
        MIN_NOTE_DURATION,
        MAX_NOTE_DURATION,
        TIME_SIGNATURE
      };

      if (pitch_subset) {
        fixed.insert( NOTE_ONSET );
        fixed.insert( NOTE_OFFSET );
      }

      
      if (verbose) {
        // show the attribute mask
        std::cout << "=======================" << std::endl;
        std::cout << "ATTRIBUTE MASK : " << std::endl;
        for (int i=0; i<mask.size(); i++) {
          if (mask[i]) {
            std::cout << rep->pretty(i) << std::endl;
          }
        }
        std::cout << "=======================" << std::endl;
      }

      for (const auto kv : rep->token_domains) {
        if (fixed.find(kv.first) == fixed.end()) {
          rep->set_mask(kv.first, {-1}, mask, 1);
        }
      }

      attribute_masks.push_back( mask );
      

      // loop over bars and set time signatures
      std::vector<std::vector<int>> bar_masks;
      for (int bn=0; bn<track.internal_ts_numerators_size(); bn++) {
        std::vector<int> bar_mask(mask);
        if (rep->has_token_type(TIME_SIGNATURE)) {
          int tstoken = rep->encode(TIME_SIGNATURE, 
            std::make_tuple(
              track.internal_ts_numerators(bn),
              track.internal_ts_denominators(bn)));
          bar_mask[tstoken] = 1; // only allow time signature
        }
        bar_masks.push_back( bar_mask );
      }
      attribute_bar_masks.push_back( bar_masks );
    
    }
  }

  void update(int token) {
    mmm::TOKEN_TYPE tt = rep->get_token_type(token);
    switch (tt) {
      case TRACK: {
        bar_count = 0;
        absolute_timestep = 0;
        bar_start_timestep = 0;
        onsets.clear();
        note_expiry.clear();
        current_track_type = static_cast<midi::TRACK_TYPE>(rep->decode(token));
        break;
      }
      case TRACK_END: {
        track_count += 1;
        break;
      }
      case BAR: {
        timestep = 0;
        barlength = 4 * enc->config->resolution;
        //bar_count += 1;
        absolute_timestep = bar_start_timestep;
        break;
      }
      case BAR_END: {
        bar_count += 1;
        bar_start_timestep += barlength;
        break;
      }
      case FILL_IN_START: {
        // clear onsets and read in the token sequence
        // we backfill events in between FILL_IN_PLACEHOLDERs
        // so that we don't skip context
        int fillp_token = enc->rep->encode(FILL_IN_PLACEHOLDER,0);
        auto it = history.begin();
        auto prev = it;
        for (int i=0; i<=infill_bar_count; i++) {
          prev = it;
          it = find(next(it), history.end(), fillp_token);
        }
        for (auto i=next(prev); i!=it; i++) {
          if (verbose) {
            std::cout << "BACKFILLING :: " << enc->rep->pretty(*i) << std::endl;
          }
          update(*i);
        }
        break;
      }
      case FILL_IN_END: {
        infill_bar_count += 1;
        break;
      }
      case TIME_SIGNATURE: {
        std::tuple<int,int> ts = rep->decode_timesig(token);
        double ts_ratio = ((double)std::get<0>(ts) / std::get<1>(ts));
        barlength = ts_ratio * 4 * enc->config->resolution;
        break;
      }
      case TIME_DELTA: {
        int delta = rep->decode(token) + 1;
        timestep += delta;
        absolute_timestep += delta; 
        break;
      }
      case TIME_ABSOLUTE: {
        int t = rep->decode(token);
        timestep = t;
        absolute_timestep = bar_start_timestep + t;
        break;
      }
      case NOTE_ONSET: {
        int pitch = rep->decode(token);
        onsets.insert( pitch );

        if ((is_drum_track(current_track_type)) && (!enc->config->use_drum_offsets)) {
          // artificially add the note duration of 1
          last_token = token;
          update(rep->encode(NOTE_DURATION,0));
        }

        break;
      }
      case NOTE_OFFSET: {
        int pitch = rep->decode(token);
        onsets.erase( pitch );
        break;
      }
      case NOTE_DURATION: {
        int dur = rep->decode(token) + 1;
        int pitch = rep->decode(last_token);
        note_expiry[dur + absolute_timestep].push_back(pitch);
        break;
      }
      default:
        // nothing to do
        break;
    }

    // remove notes that have "expired"
    std::vector<int> to_remove;
    for (const auto kv : note_expiry) {
      if (kv.first <= absolute_timestep) {
        for (const auto pitch : kv.second) {
          onsets.erase( pitch );
        }
        to_remove.push_back( kv.first );
      }
    }
    // remove these lists from note expiry
    for (const auto t : to_remove) {
      note_expiry.erase( t ); 
    }

    last_token = token;

    if (verbose) {
      std::cout << "ONSETS : " << onsets.size() << std::endl;
    }
    

  }

  void set_mask(int last_token, std::vector<int> &mask) {

    // basic constraints of the representation

    // hacky way to accomodate mark_drum_density / note_duration / poly
    bool dual = rep->has_token_types({MAX_NOTE_DURATION, DENSITY_LEVEL});
    TOKEN_TYPE last_tt = rep->get_token_type(last_token);
    bool is_drum = is_drum_track(current_track_type);
    bool drum_density = enc->config->mark_drum_density;
    if ((dual) && (drum_density) && (is_drum) && (last_tt==DENSITY_LEVEL)) {
      // skip polyphony / duration tokesn
      rg->set_mask(rep->encode(MAX_NOTE_DURATION, 0), mask);
    }
    else if ((dual) && (drum_density) && (!is_drum) && (last_tt==INSTRUMENT)) {
      // skip density tokens
      rg->set_mask(rep->encode(DENSITY_LEVEL, 0), mask);
    }
    else if ((is_drum) && (!enc->config->use_drum_offsets) && (last_tt==NOTE_ONSET)) {
      // fast forward past NOTE_DURATION token
      rg->set_mask(rep->encode(NOTE_DURATION,0), mask);
    }
    else {
      rg->set_mask(last_token, mask);
    }
    
    // can't have onset for note that is already sounding
    for (const auto pitch : onsets) {
      mask[rep->encode(NOTE_ONSET,pitch)] = 0;
    }
    
    // can't have offset for note that is not sounding
    if (rep->has_token_type(NOTE_OFFSET)) {
      for (int pitch=0; pitch<128; pitch++) {
        if (onsets.find(pitch) == onsets.end()) {
          mask[rep->encode(NOTE_OFFSET,pitch)] = 0;
        }
      }
    }

    // can't have note onsets when timestep == barlength
    // you can only have note offsets
    if (timestep == barlength) {
      if (verbose) {
        std::cout << "HIT TIME LIMIT >>>>>>>>>>>>>>>>>>>> " << std::endl;
      }
      rep->set_mask(NOTE_ONSET, {-1}, mask, 0);
      rep->set_mask(VELOCITY_LEVEL, {-1}, mask, 0);
    }

    // determine what the hard limit is
    // the hard limit may be smaller when a limited domain
    // of notes is allowed on a track (i.e. OPZ_KICK_TRACK)
    int hard_limit = 0;
    if (model_type == TRACK_MODEL) {
      int index = std::min(track_count, num_tracks-1);
      hard_limit = polyphony_hard_limits[index];
    }
    else {
      int index = std::min(infill_bar_count, num_infill_bars-1);
      hard_limit = polyphony_hard_limits[std::get<0>(selected_bars[index])];
    }
    //int hard_limit = polyphony_hard_limits[0]; //std::min(track_count,num_tracks)];
    auto it = OPZ_NOTE_DOMAINS.find(current_track_type);
    if (it != OPZ_NOTE_DOMAINS.end()) {
      hard_limit = it->second.size();
    }

    // can't have more than n simultaneous notes
    if (onsets.size() >= hard_limit) {
      if (verbose) {
        std::cout << "HIT HARD LIMIT >>>>>>>>>>>>>>>>>>>> " << std::endl;
      }
      rep->set_mask(NOTE_ONSET, {-1}, mask, 0);
      rep->set_mask(VELOCITY_LEVEL, {-1}, mask, 0); 
      // will be ignored if token doesn't exist
    }

    // can't have more timesteps than barlength
    int domain_limit = rep->get_domain_size(TIME_DELTA);
    if (domain_limit) {
      int max_td = std::max(std::min(barlength - timestep, domain_limit), 0);
      for (int td=max_td; td<domain_limit; td++) {
        mask[rep->encode(TIME_DELTA,td)] = 0;
      }
    }

    // also restrict this for absolute time
    // but this should be limited based on time_signature / barlength
    domain_limit = rep->get_domain_size(TIME_ABSOLUTE);
    if (domain_limit) {
      for (int td=0; td<=timestep; td++) {
        mask[rep->encode(TIME_ABSOLUTE,td)] = 0;
      }
      for (int td=barlength+1; td<domain_limit; td++) {
        mask[rep->encode(TIME_ABSOLUTE,td)] = 0;
      }
    }
    
    if (model_type == TRACK_MODEL) {
      // limit number of bars
      if (bar_count != num_bars) {
        rep->set_mask(TRACK_END, {-1}, mask, 0);
      }
      else {
        rep->set_mask(BAR, {-1}, mask, 0);
      }
      // limit the track count
      if (track_count >= num_tracks) {
        std::fill(mask.begin(), mask.end(), 0);
        finished = true;
      }
      // only add attribute mask if not finished
      // otherwise it will crash with track_count out of range
      if (!finished) {
        for (int i=0; i<mask.size(); i++) {
          int num_bars = attribute_bar_masks[track_count].size();
          int safe_bar_index = std::min(bar_count, num_bars - 1);
          mask[i] *= attribute_bar_masks[track_count][safe_bar_index][i];
          //mask[i] *= attribute_masks[track_count][i];
        }
      }
    }
    else if (model_type == BAR_INFILL_MODEL) {
      // limit the bar infill count
      if (infill_bar_count >= num_infill_bars) {
        std::fill(mask.begin(), mask.end(), 0);
        finished = true;
      }
    }

    // if mask is all zeros we have a problem as the model has
    // no 'valid' path forward
    if ((std::find(mask.begin(), mask.end(), 1) == mask.end()) && (!finished)) {
      throw std::runtime_error("FATAL ERROR : EVERY TOKEN IS MASKED");
    }

  }

  float get_temperature() {
    return track_temperatures[std::min(track_count, num_tracks-1)];
  }

  std::vector<int> get_mask(int last_token) {
    std::vector<int> mask(rep->max_token(), 0);
    update(last_token);
    set_mask(last_token, mask);
    return mask;
  }

  std::vector<int> get_mask(std::vector<int> &tokens) {
    std::vector<int> mask(enc->rep->max_token(), 0);
    for (int t=token_position; t<tokens.size(); t++) {
      if (verbose) {
        std::cout << "UPDATING [" << token_position << "] :: " << enc->rep->pretty(tokens[t]) << std::endl;
      }
      update( tokens[t] );
      history.push_back( tokens[t] );
      token_position++;
    }

    set_mask(tokens.back(), mask);
    return mask;
  }

  std::vector<int> generate_random() {
    std::vector<int> tokens = {rep->encode(PIECE_START,0)};
    for (int i=0; i<100; i++) {
      std::vector<int> mask = get_mask(tokens.back());
      std::vector<int> choices;
      for (int m=0; m<mask.size(); m++) {
        if (mask[m]) {
          choices.push_back(m);
        }
      }
      if (choices.size() == 0) {
        break;
      }
      else {
        tokens.push_back( choices[rand() % choices.size()] );
      }
    }
    return tokens;
  }
  
  void validate_sequence(std::vector<int> &tokens) {
    rg->validate_sequence(tokens);
  }

  // map from pitch to when it expires
  std::map<int,std::vector<int>> note_expiry; // time -> list of pitches
  std::set<int> onsets;
  int last_token;
  midi::TRACK_TYPE current_track_type;

  int barlength;
  int timestep;
  int absolute_timestep;
  int bar_start_timestep;
  
  int bar_count;
  int track_count;
  int infill_bar_count;
  
  int num_bars;
  int num_tracks;
  int num_infill_bars;
  std::vector<std::vector<int>> attribute_masks;
  std::vector<std::vector<std::vector<int>>> attribute_bar_masks;
  //std::vector<std::tuple<int,int>> time_signatures;

  std::vector<int> prompt;
  std::vector<int> inverse_order;
  std::vector<std::vector<double>> embeds;
  std::string ckpt_path;

  int token_position;
  bool finished;
  MODEL_TYPE model_type;
  std::vector<int> history;

  bool verbose;
  int polyphony_hard_limit;
  std::vector<int> polyphony_hard_limits;
  std::vector<float> track_temperatures;
  std::vector<std::pair<int,int>> selected_bars;

  //ENCODER *enc;
  std::unique_ptr<ENCODER> enc;
  REPRESENTATION *rep;
  REP_GRAPH *rg;

};


}