#include <map>
#include <tuple>
#include <set>
#include <vector>

/*

MACRO - STRUCTURE

PIECE

DRUM TRACK
...
DRUM TRACK END




*/

namespace mmm {

enum TOKEN_TYPE {
  PIECE_START,
  NOTE_ONSET,
  NOTE_OFFSET,
  PITCH,
  NON_PITCH,
  VELOCITY,
  TIME_DELTA,
  TIME_ABSOLUTE,
  INSTRUMENT,
  BAR,
  BAR_END,
  TRACK,
  TRACK_END,
  DRUM_TRACK,
  FILL_IN,
  FILL_IN_PLACEHOLDER,
  FILL_IN_START,
  FILL_IN_END,
  HEADER,
  VELOCITY_LEVEL,
  GENRE,
  DENSITY_LEVEL,
  TIME_SIGNATURE,
  SEGMENT,
  SEGMENT_END,
  SEGMENT_FILL_IN,
  NOTE_DURATION,
  AV_POLYPHONY,
  MIN_POLYPHONY,
  MAX_POLYPHONY,
  MIN_NOTE_DURATION,
  MAX_NOTE_DURATION,
  NUM_BARS,
  MIN_POLYPHONY_HARD,
  MAX_POLYPHONY_HARD,
  MIN_NOTE_DURATION_HARD,
  MAX_NOTE_DURATION_HARD,
  REST_PERCENTAGE,
  MIN_PITCH,
  MAX_PITCH,
  PITCH_CLASS,
  NONE
};

inline const char* toString(mmm::TOKEN_TYPE tt) {
  switch (tt) {
    case PIECE_START: return "PIECE_START";
    case NOTE_ONSET: return "NOTE_ONSET";
    case NOTE_OFFSET: return "NOTE_OFFSET";
    case PITCH: return "PITCH";
    case NON_PITCH: return "NON_PITCH";
    case VELOCITY: return "VELOCITY";
    case TIME_DELTA: return "TIME_DELTA";
    case TIME_ABSOLUTE: return "TIME_ABSOLUTE";
    case INSTRUMENT: return "INSTRUMENT";
    case BAR: return "BAR";
    case BAR_END: return "BAR_END";
    case TRACK: return "TRACK";
    case TRACK_END: return "TRACK_END";
    case DRUM_TRACK: return "DRUM_TRACK";
    case FILL_IN: return "FILL_IN";
    case FILL_IN_PLACEHOLDER: return "FILL_IN_PLACEHOLDER";
    case FILL_IN_START: return "FILL_IN_START";
    case FILL_IN_END: return "FILL_IN_END";
    case HEADER: return "HEADER";
    case VELOCITY_LEVEL: return "VELOCITY_LEVEL";
    case GENRE: return "GENRE";
    case DENSITY_LEVEL: return "DENSITY_LEVEL";
    case TIME_SIGNATURE: return "TIME_SIGNATURE";
    case SEGMENT: return "SEGMENT";
    case SEGMENT_END: return "SEGMENT_END";
    case SEGMENT_FILL_IN: return "SEGMENT_FILL_IN";
    case NOTE_DURATION: return "NOTE_DURATION";
    case AV_POLYPHONY: return "AV_POLYPHONY";
    case MIN_POLYPHONY: return "MIN_POLYPHONY";
    case MAX_POLYPHONY: return "MAX_POLYPHONY";
    case MIN_NOTE_DURATION: return "MIN_NOTE_DURATION";
    case MAX_NOTE_DURATION: return "MAX_NOTE_DURATION";
    case NUM_BARS: return "NUM_BARS";
    case MIN_POLYPHONY_HARD: return "MIN_POLYPHONY_HARD";
    case MAX_POLYPHONY_HARD: return "MAX_POLYPHONY_HARD";
    case MIN_NOTE_DURATION_HARD: return "MIN_NOTE_DURATION_HARD";
    case MAX_NOTE_DURATION_HARD: return "MAX_NOTE_DURATION_HARD";
    case REST_PERCENTAGE: return "REST_PERCENTAGE";
    case MIN_PITCH: return "MIN_PITCH";
    case MAX_PITCH: return "MAX_PITCH";
    case PITCH_CLASS: return "PITCH_CLASS";
    case NONE: return "NONE";
  }
}

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

using TOKEN_EDGE = std::pair<mmm::TOKEN_TYPE,mmm::TOKEN_TYPE>;

class REP_NODE {
public:
  REP_NODE(int k, mmm::TOKEN_TYPE x) {
    key = k;
    tt = x;
  }
  std::set<int> edges;
  std::set<int> in_edges;
  mmm::TOKEN_TYPE tt;
  int key;
};

class REP_GRAPH {
public:

};


/*

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

*/

}