#include <vector>
#include <tuple>
#include <string>
#include <map>
#include <set>

#include <iostream>
#include <fstream>

using GRAPH = std::vector<std::vector<std::string>>;
using KEYMAP = std::map<std::string,std::string>;
using STRVEC = std::vector<std::string>;

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

GRAPH TRACK_PARAM = {
  {"START", "GENRE", "INSTRUMENT", "NOTE_ONSET", "DENSITY_LEVEL", "MIN_POLYPHONY", "MAX_POLYPHONY", "MIN_NOTE_DURATION", "MAX_NOTE_DURATION", "END"}
};

GRAPH BAR_ABSOLUTE = {
  {"START","TIME_SIGNATURE"},
  {"TIME_SIGNATURE","TIME_ABSOLUTE"},
  {"TIME_SIGNATURE","VELOCITY_LEVEL"},
  {"TIME_SIGNATURE","END"},
  {"VELOCITY_LEVEL","NOTE_ONSET"},
  {"NOTE_ONSET","NOTE_DURATION"},
  {"NOTE_DURATION","TIME_ABSOLUTE"},
  {"NOTE_DURATION", "NOTE_ONSET"},
  {"NOTE_DURATION", "VELOCITY_LEVEL"},
  {"NOTE_DURATION", "END"},
  {"TIME_ABSOLUTE", "NOTE_ONSET"},
  {"TIME_ABSOLUTE", "VELOCITY_LEVEL"},
  {"TIME_ABSOLUTE", "END"},
  {"TIME_SIGNATURE","FILL_IN_PLACEHOLDER"}, // always have time sig
  {"FILL_IN_PLACEHOLDER","END"}, // handle fill in
  {"START","END"} // empty bar
};

GRAPH BAR_DEFAULT = {
  {"BAR_START","TIME_DELTA"},
  {"BAR_START","NOTE_ONSET"},
  {"NOTE_ONSET","TIME_DELTA"},
  {"NOTE_ONSET","NOTE_ONSET"},
  {"NOTE_ONSET","BAR_END"},
  {"NOTE_OFFSET","TIME_DELTA"},
  {"NOTE_OFFSET","NOTE_ONSET"},
  {"NOTE_OFFSET","NOTE_OFFSET"},
  {"NOTE_OFFSET","BAR_END"},
  {"TIME_DELTA","TIME_DELTA"},
  {"TIME_DELTA","NOTE_ONSET"},
  {"TIME_DELTA","NOTE_OFFSET"},
  {"TIME_DELTA","BAR_END"},
  {"BAR_START","FILL_IN_PLACEHOLDER"}, // handle fill in
  {"FILL_IN_PLACEHOLDER","BAR_END"}, // handle fill in
  {"BAR_START","BAR_END"} // empty bar
};

GRAPH BASE = {
  {"PIECE_START","NUM_BARS"},
  {"NUM_BARS","INSTxTRACK"},
  {"INSTxTRACK","INSTxTRACK_PARAM"},
  {"INSTxTRACK_PARAM","INSTxBAR"},
  {"INSTxBAR","INSTxBAR_CONTENT"},
  {"INSTxBAR_CONTENT","INSTxBAR_END"},
  {"INSTxBAR_END","INSTxBAR"},
  {"INSTxBAR_END","INSTxTRACK_END"},
  {"INSTxTRACK_END","INSTxTRACK"},
  {"INSTxTRACK_END","DRUMxTRACK"},
  {"INSTxTRACK_END","PIECE_END"},
  {"INSTxTRACK_END","FILL_IN_START"},

  {"NUM_BARS","DRUMxTRACK"},
  {"DRUMxTRACK","DRUMxTRACK_PARAM"},
  {"DRUMxTRACK_PARAM","DRUMxBAR"},
  {"DRUMxBAR","DRUMxBAR_CONTENT"},
  {"DRUMxBAR_CONTENT","DRUMxBAR_END"},
  {"DRUMxBAR_END","DRUMxBAR"},
  {"DRUMxBAR_END","DRUMxTRACK_END"},
  {"DRUMxTRACK_END","DRUMxTRACK"},
  {"DRUMxTRACK_END","INSTxTRACK"},
  {"DRUMxTRACK_END","PIECE_END"},
  {"DRUMxTRACK_END","FILL_IN_START"},

  {"FILL_IN_START","FILLxBAR_CONTENT"},
  {"FILLxBAR_CONTENT","FILL_IN_END"},
  {"FILL_IN_END","FILL_IN_START"},
  {"FILL_IN_END","PIECE_END"},

};

void build_string_to_token_map(GRAPH &graph) {
  std::map<std::string,std::string> m;
  for (const auto edge : graph) {
    for (const auto node : edge) {
      auto index = node.find_first_of("x");
      if (index != std::string::npos) {
        m.insert({node, node.substr(index+1)});
      }
    }
  }
  for (const auto kv : m) {
    std::cout << kv.first << " : " << kv.second << std::endl;
  }
}

void create_dot_file(GRAPH &graph, std::string path) {
  std::ofstream f;
  f.open(path);
  f << "digraph graphname {\n";
  for (const auto edge : graph) {
    f << "\t";
    for (int i=0; i<(int)edge.size()-1; i++) {
      f << edge[i] << " -> ";
    }
    f << edge.back() << "\n";
  }
  f << "}\n";
  f.close();
}

GRAPH insert_graph(GRAPH &graph, GRAPH &subgraph, std::string key, std::string prefix, std::string start_node, std::string end_node, std::set<std::string> filter={}) {
  // insert subgraph in the place of a key
  
  GRAPH output;
  for (const auto edge : graph) {
    if ((edge[0] != key) && (edge[1] != key)) {
      output.push_back(edge);
    }
  }
  for (const auto edge : subgraph) {
    std::vector<std::string> pedge;
    for (const auto node : edge) {
      if (node == "START") {
        pedge.push_back( start_node );
      }
      else if (node == "END") {
        pedge.push_back( end_node );
      }
      else if (filter.find(node) == filter.end()) {
        pedge.push_back( prefix + node );
      }
    }
    output.push_back(pedge);
  }
  return output;
}

GRAPH BUILD_GRAPH(bool autoreg) {
  GRAPH x(BASE);

  STRVEC filter;

  if (autoreg) {
    // we have no infilling tokens at all
    STRVEC add = {"FILL_IN_START", "FILL_IN_END", "FILL_IN_PLACEHOLDER"};
    for (const auto x : add) {
      filter.push_back( x );
    }
  }

  /*

  x = insert_graph(x, BAR_DEFAULT, "INST_BAR_CONTENT", "INST_BAR_");
  x = insert_graph(x, BAR_DEFAULT, "DRUM_BAR_CONTENT", "DRUM_BAR_");
  x = insert_graph(x, BAR_DEFAULT, "FILL_BAR_CONTENT", "FILL_IN_", {"FILL_IN_PLACEHOLDER"});

  x = insert_graph(x, TRACK_PARAM, "INST_TRACK_PARAM", "INST_", {"DENSITY_LEVEL"});
  x = insert_graph(x, TRACK_PARAM, "DRUM_TRACK_PARAM", "DRUM_", {"MIN_POLYPHONY","MAX_POLYPHONY","MIN_NOTE_DURATION","MAX_NOTE_DURATION"});
  */

  return x;
}

class SequenceValidator {
public:
  SequenceValidator(GRAPH &g) {
    state = "PIECE_START";
    for (const auto edge : g) {
      for (int i=0; i<(int)edge.size()-1; i++) {
        forward[edge[i]].insert( edge[i+1] );
        backward[edge[i+1]].insert( edge[i] ); 
      }
    }
  }
  ~SequenceValidator() { }

  

  /*
  void step(TOKEN_TYPE tt, int value) {

  }
  */


  void set_mask(TOKEN_TYPE last_tt, std::vector<int> &mask) {
    auto it = forward_token[state].find(last_tt);
    if (it == forward_token[state].end()) {
      throw std::runtime_error("BAD GRAPH!");
    }
    state = it->second;
    for (const auto kv : forward_token[state]) {
      enc->rep->set_mask(kv.first, {-1}, mask, 1);
    }
  }
  std::vector<int> get_mask(int last_token) {
    std::vector<int> mask(enc->rep->max_token(), 0);
    set_mask(last_token, mask);
    return mask;
  }

  void random_walk(int n) {
    for (int i=0; i<n; i++) {
      std::cout << state << std::endl;
      int num_choices = forward[state].size();
      auto it = std::begin(forward[state]);
      std::advance(it, rand() % num_choices);
      state = *it;
      if (state == "PIECE_END") {
        break;
      }
    }
  }

  std::map<std::string,std::set<std::string>> forward;
  std::map<std::string,std::set<std::string>> backward;

  std::map<std::string,std::map<TOKEN_TYPE,std::string>> forward_token;
  std::string state;
};


int main() {

  srand(time(NULL));

  std::string path = "base.dot";

  GRAPH x(BASE);
  GRAPH b(BAR_ABSOLUTE);

  x = insert_graph(x, b, "INSTxBAR_CONTENT", "INST_BARx", "INSTxBAR", "INSTxBAR_END");
  x = insert_graph(x, b, "DRUMxBAR_CONTENT", "DRUM_BARx", "DRUMxBAR", "DRUMxBAR_END");
  //x = insert_graph(x, BAR_ABSOLUTE, "FILLxBAR_CONTENT", "FILLx", {"FILL_IN_PLACEHOLDER"});

  x = insert_graph(x, TRACK_PARAM, "INSTxTRACK_PARAM", "INST_PARAMx", "INSTxTRACK", "INSTxBAR", {"DENSITY_LEVEL"});
  x = insert_graph(x, TRACK_PARAM, "DRUMxTRACK_PARAM", "DRUM_PARAMx", "DRUMxTRACK", "DRUMxBAR", {"MIN_POLYPHONY","MAX_POLYPHONY","MIN_NOTE_DURATION","MAX_NOTE_DURATION"});


  //build_string_to_token_map(x);

  SequenceValidator v(x);
  v.random_walk(80);

  create_dot_file(x, path);

}

