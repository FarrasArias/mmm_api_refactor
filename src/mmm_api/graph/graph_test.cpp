#include "graph.h"


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