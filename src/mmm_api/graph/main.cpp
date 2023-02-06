#include <vector>
#include <tuple>
#include <string>
#include <map>
#include <set>

#include <iostream>
#include <fstream>

// graph will be 

using EDGE_LIST = std::vector<std::pair<std::string,std::string>>;
using COMPOUND_EDGE = std::vector<std::string>;

template <class dtype>
class Graph {
public:
  Graph(EDGE_LIST edges) {
    for (const auto e : edges) {
      add_edge( std::get<0>(e), std::get<1>(e) );
    }
  }
  Graph(std::vector<std::string> edges, bool extra) {
    for (int i=0; i<(int)edges.size()-1; i++) {
      add_edge( edges[i], edges[i+1] );
    }
  }
  ~Graph() {

  }

  void add_edge(const std::string &u, const std::string &v) {
    out[u].insert(v);
    in[v].insert(u);
  }

  EDGE_LIST edge_list() {
    EDGE_LIST el;
    for (const auto kv : out) {
      for (const auto node : std::get<1>(kv)) {
        el.push_back( std::make_pair(std::get<0>(kv), node) );
      }
    }
    return el;
  }

  void insert_subgraph(EDGE_LIST edges, std::string prefix, EDGE_LIST in_links, EDGE_LIST out_links) {
    // do filter first
    for (const auto e : edges) {
      add_edge( prefix + "x" + std::get<0>(e), prefix + "x" + std::get<1>(e) );
    }
    for (const auto e : in_links) {
      add_edge( std::get<0>(e), prefix + "x" + std::get<1>(e) );
    }
    for (const auto e : out_links) {
      add_edge( prefix + "x" + std::get<0>(e), std::get<1>(e) );
    }
  }

  void remove_node(std::string v) {
 
    std::set<std::string> out_edges = out.find(v)->second;
    std::set<std::string> in_edges = in.find(v)->second;

    for (const auto pre : in_edges) {
      for (const auto post : out_edges) {
        out[pre].insert( post );
        in[post].insert( pre );
        in[post].erase( v );
      }
      out[pre].erase( v );
    }

    out.erase( v );
    in.erase( v );
    nodes.erase( v );
  }

  void show() {
    std::cout << "====================" << std::endl;
    for (const auto e : edge_list()) {
      std::cout << std::get<0>(e) << " --> " << std::get<1>(e) << std::endl;
    }
  }

  void make_dot(std::string path) {
    std::ofstream f;
    f.open(path);
    f << "digraph graphname {\n";
    for (const auto e : edge_list()) {
      f << "\t";
      f << std::get<0>(e) << " -> " << std::get<1>(e) << "\n";
    }
    f << "}\n";
    f.close();
  }

  std::map<std::string,std::set<std::string>> out;
  std::map<std::string,std::set<std::string>> in;
  std::map<std::string,dtype> nodes;
};

// make a traversal class
// consumes a graph and then traverses it
// can do this either manually / automatically or for validation of a sequence

template <class dtype>
class GraphPath {
public:

  GraphPath(Graph<dtype> *g, std::string &p) {
    graph = g;
    position = p;
  }

  std::vector<std::string> get_choice_labels() {

  }

  std::vector<std::string 



  Graph<dtype> *graph;
  std::string position;
};


//template <typename dtype>
Graph<int> make_track() {

  COMPOUND_EDGE track_param = {
    {"GENRE", "INSTRUMENT", "NOTE_ONSET", "DENSITY_LEVEL", "MIN_POLYPHONY", "MAX_POLYPHONY", "MIN_NOTE_DURATION", "MAX_NOTE_DURATION"}
  };

  EDGE_LIST bar_in = {
    {"BAR","TIME_DELTA"},
    {"BAR","NOTE_ONSET"},
    {"BAR","FILL_IN_PLACEHOLDER"},
  };

  EDGE_LIST bar_out = {
    {"NOTE_OFFSET","BAR_END"},
    {"NOTE_ONSET","BAR_END"},
    {"TIME_DELTA","BAR_END"},
    {"FILL_IN_PLACEHOLDER", "BAR_END"}
  };

  EDGE_LIST bar = {
    {"NOTE_ONSET","TIME_DELTA"},
    {"NOTE_ONSET","NOTE_ONSET"},
    {"NOTE_OFFSET","TIME_DELTA"},
    {"NOTE_OFFSET","NOTE_ONSET"},
    {"NOTE_OFFSET","NOTE_OFFSET"},
    {"TIME_DELTA","TIME_DELTA"},
    {"TIME_DELTA","NOTE_ONSET"},
    {"TIME_DELTA","NOTE_OFFSET"},
  };

  EDGE_LIST bar_absolute_in = {
    {"BAR","TIME_SIGNATURE"}
  };

  EDGE_LIST bar_absolute_out = {
    {"TIME_ABSOLUTE", "BAR_END"},
    {"TIME_SIGNATURE","BAR_END"},
    {"NOTE_DURATION", "BAR_END"},
    {"FILL_IN_PLACEHOLDER", "BAR_END"}
  };

  EDGE_LIST bar_absolute = {
    {"TIME_SIGNATURE","TIME_ABSOLUTE"},
    {"TIME_SIGNATURE","VELOCITY_LEVEL"},
    {"VELOCITY_LEVEL","NOTE_ONSET"},
    {"NOTE_ONSET","NOTE_DURATION"},
    {"NOTE_DURATION","TIME_ABSOLUTE"},
    {"NOTE_DURATION", "NOTE_ONSET"},
    {"NOTE_DURATION", "VELOCITY_LEVEL"},
    {"TIME_ABSOLUTE", "NOTE_ONSET"},
    {"TIME_ABSOLUTE", "VELOCITY_LEVEL"},
    {"TIME_SIGNATURE","FILL_IN_PLACEHOLDER"},
  };

  EDGE_LIST track = {
    {"TRACK_END","TRACK"},
    {"BAR","BAR_END"},
    {"BAR_END","BAR"},
    {"BAR_END","TRACK_END"}
  };

  Graph<int> g(track);
  g.insert_subgraph( Graph<int>(track_param,true).edge_list(), "tparam", {{"TRACK",track_param[0]}}, {{track_param.back(),"BAR"}} );

  //g.insert_subgraph( bar_absolute, "bar", bar_absolute_in, bar_absolute_out);
  g.insert_subgraph( bar, "bar", bar_in, bar_out);

  return g;
}


int main() {


  EDGE_LIST base = {
    {"PIECE_START","NUM_BARS"}
  };

  Graph<int> g(base);
  g.insert_subgraph(
    make_track().edge_list(), "inst", {{"NUM_BARS","TRACK"}}, {});
  g.insert_subgraph(
    make_track().edge_list(), "drum", {{"NUM_BARS","TRACK"}}, {});
  g.show();
  g.make_dot(std::string("test.dot"));

}
