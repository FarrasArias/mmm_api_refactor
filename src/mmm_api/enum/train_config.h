#pragma once

// START OF NAMESPACE
namespace mmm {

class TrainConfig {
public:
  TrainConfig() {
    num_bars = 4;
    min_tracks = 1;
    max_tracks = 12;
    max_mask_percentage = 0.75;
    opz = false;
    no_max_length = false;
  }
  std::map<std::string,std::string> to_json() {
    std::map<std::string,std::string> x;
    x["num_bars"] = std::to_string(num_bars);
    x["min_tracks"] = std::to_string(min_tracks);
    x["max_tracks"] = std::to_string(max_tracks);
    x["max_mask_percentage"] = std::to_string(max_mask_percentage);
    x["opz"] = std::to_string((int)opz);
    x["no_max_length"] = std::to_string((int)no_max_length);
    return x;
  }
  void from_json(std::map<std::string,std::string> &x) {
    num_bars = stoi(x["num_bars"]);
    min_tracks = stoi(x["min_tracks"]);
    max_tracks = stoi(x["max_tracks"]);
    max_mask_percentage = stof(x["max_mask_percentage"]);
    opz = (bool)stoi(x["opz"]);
    no_max_length = (bool)stoi(x["no_max_length"]);
  }
  int num_bars;
  int min_tracks;
  int max_tracks;
  float max_mask_percentage;
  bool opz;
  bool no_max_length;
};

}
// END OF NAMESPACE