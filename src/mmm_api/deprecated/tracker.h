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

#include "encoder/representation.h"
#include "encoder/encoder_base.h"
#include "enum/token_types.h"

using TOKEN_EDGE = std::pair<mmm::TOKEN_TYPE,mmm::TOKEN_TYPE>;

std::vector<std::vector<mmm::TOKEN_TYPE>> DEFAULT_DURATION_GRAPH = {
  {PIECE_START, TRACK},
  
  {TRACK, INSTRUMENT},

  {INSTRUMENT, DENSITY_LEVEL, MIN_POLYPHONY, MAX_POLYPHONY, MIN_NOTE_DURATION, MAX_NOTE_DURATION, BAR},

  {BAR, TIME_DELTA},
  {BAR, NOTE_ONSET},
  {BAR, FILL_IN_PLACEHOLDER},
  {BAR, BAR_END},

  {FILL_IN_PLACEHOLDER, BAR_END},

  {NOTE_ONSET, NOTE_DURATION},

  {NOTE_DURATION, NOTE_ONSET},
  {NOTE_DURATION, BAR_END},
  {NOTE_DURATION, TIME_DELTA},

  {TIME_DELTA, TIME_DELTA}, // THIS NEEDS A FIX
  {TIME_DELTA, NOTE_ONSET},
  {TIME_DELTA, BAR_END},

  {BAR_END, BAR},
  {BAR_END, TRACK_END},

  {TRACK_END, TRACK},
  {TRACK_END, FILL_IN_START},

  {FILL_IN_START, FILL_IN_END}

};

std::vector<std::vector<mmm::TOKEN_TYPE>> DEFAULT_EVENT_GRAPH = {
  {PIECE_START, TRACK},
  
  {TRACK, INSTRUMENT},
  
  {INSTRUMENT, DENSITY_LEVEL, MIN_POLYPHONY, MAX_POLYPHONY, MIN_NOTE_DURATION, MAX_NOTE_DURATION, BAR},

  {BAR, TIME_DELTA},
  {BAR, NOTE_ONSET},
  {BAR, FILL_IN_PLACEHOLDER},
  {BAR, BAR_END},

  {FILL_IN_PLACEHOLDER, BAR_END},

  {NOTE_ONSET, TIME_DELTA},
  {NOTE_ONSET, NOTE_ONSET},
  {NOTE_ONSET, BAR_END},

  {NOTE_OFFSET, NOTE_OFFSET},
  {NOTE_OFFSET, NOTE_ONSET},
  {NOTE_OFFSET, TIME_DELTA},
  {NOTE_OFFSET, BAR_END},

  {TIME_DELTA, TIME_DELTA}, // THIS NEEDS A FIX
  {TIME_DELTA, NOTE_ONSET},
  {TIME_DELTA, NOTE_OFFSET},
  {TIME_DELTA, BAR_END},

  {BAR_END, BAR},
  {BAR_END, TRACK_END},

  {TRACK_END, TRACK},
  {TRACK_END, FILL_IN_START},

  {FILL_IN_START, FILL_IN_END}

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
  REP_GRAPH(ENCODER *e) {
    enc = e;
    rep = e->rep;
    if (enc->config->use_note_duration_encoding) {
      build_from_paths(DEFAULT_DURATION_GRAPH);
    }
    else {
      build_from_paths(DEFAULT_EVENT_GRAPH);
    }
    std::vector<mmm::TOKEN_TYPE> to_remove;
    for (const auto kv : nodes) {
      if (!rep->has_token_type(kv.first)) {
        to_remove.push_back(kv.first);
      }
    }
    remove_nodes(to_remove);
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
    nodes.erase(v);
  }
  void remove_nodes(std::vector<mmm::TOKEN_TYPE> &vs) {
    for (const auto v : vs) {
      remove_node(v);
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
  void validate_sequence(std::vector<int> &tokens) {
    for (int i=0; i<(int)tokens.size()-1; i++) {
      check_edge(
        rep->get_token_type(tokens[i]),
        rep->get_token_type(tokens[i+1]));
    }
  }
  void set_mask(int last_token, std::vector<int> &mask) {
    mmm::TOKEN_TYPE tt = rep->get_token_type(last_token);
    auto it = nodes.find(tt);
    if (it == nodes.end()) {
      throw std::runtime_error("FATAL ERROR : INVALID NODE IN REP GRAPH!");
    }
    for (const auto e : it->second.edges) {
      rep->set_mask(e, {-1}, mask, 1);
    }
  }
  std::vector<int> get_mask(int last_token) {
    std::vector<int> mask(rep->max_token(), 0);
    set_mask(last_token, mask);
    return mask;
  }
  ENCODER *enc;
  REPRESENTATION *rep;
  std::map<mmm::TOKEN_TYPE,REP_NODE> nodes;
};

class CONSTRAINT_REP_GRAPH {
public:
  CONSTRAINT_REP_GRAPH(ENCODER_TYPE e) {
    enc = getEncoder(e);
    rep = enc->rep;
    rg = new REP_GRAPH(enc);
    barlength = 4 * enc->config->resolution;
    timestep = 0;
    absolute_timestep = 0;
    bar_count = 0;
    track_count = 0;
  }

  ~CONSTRAINT_REP_GRAPH() {
    delete rg;
  }

  void update(int token) {
    mmm::TOKEN_TYPE tt = rep->get_token_type(token);
    switch (tt) {
      case TRACK: {
        bar_count = 0;
        absolute_timestep = 0;
        onsets.clear();
        note_expiry.clear();
        break;
      }
      case BAR: {
        timestep = 0;
        barlength = 4 * enc->config->resolution;
        bar_count += 1;
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
      case NOTE_ONSET: {
        int pitch = rep->decode(token);
        onsets.insert( pitch );
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

  }

  void set_mask(int last_token, vector<int> &mask) {
    // basic constraints of the representation
    rg->set_mask(last_token, mask);

    // more complicated constraints

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

    // can't have more than 12 simultaneous notes
    if (onsets.size() >= 3) {
      for (int pitch=0; pitch<128; pitch++) {
        mask[rep->encode(NOTE_ONSET,pitch)] = 0;
      }
    }

    // can't have more timesteps than barlength
    int domain_limit = rep->get_domain_size(TIME_DELTA);
    int max_td = min(barlength - timestep, domain_limit);
    for (int td=max_td; td<domain_limit; td++) {
      mask[rep->encode(TIME_DELTA,td)] = 0;
    }

    // only allow track end when we have the correct number of bars
    // in this case don't allow another bar to be added
    if (bar_count != 4) {
      rep->set_mask(TRACK_END, {-1}, mask, 0);
    }
    else {
      rep->set_mask(BAR, {-1}, mask, 0);
    }


    
  }

  std::vector<int> get_mask(int last_token) {
    std::vector<int> mask(rep->max_token(), 0);
    update(last_token);
    set_mask(last_token, mask);
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
  int barlength;
  int timestep;
  int absolute_timestep;
  int bar_count;
  int track_count;
  int last_token;

  ENCODER *enc;
  REPRESENTATION *rep;
  REP_GRAPH *rg;

};