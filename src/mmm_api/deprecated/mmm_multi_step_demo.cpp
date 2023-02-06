#include <vector>
#include <string>
#include <map>
#include <tuple>

#include "midi_io.h" // only needed for MIDI input/output
#include "sampling/multi_step_sample.h"
#include "protobuf/util.h"
#include "protobuf/midi.pb.h"

#include <google/protobuf/util/json_util.h>

void load_piece_from_json_file(std::string path, midi::Piece *x) {
  std::ifstream t(path.c_str());
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  google::protobuf::util::JsonStringToMessage(str.c_str(), x);
}

void load_status_from_json_file(std::string path, midi::Status *x) {
  std::ifstream t(path.c_str());
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  google::protobuf::util::JsonStringToMessage(str.c_str(), x);
}

int main() {

  //midi::Status s;

  std::string midi_path = "test_midi/test_8.mid";
  std::string ckpt_path = "paper_bar_4.pt";
  //std::string ckpt_path = "/users/jeff/CODE/MMM_TRAINING/model.pt";

  // before we generate we need to set the hyper paramaters for
  // generation

  midi::SampleParam param;
  param.set_tracks_per_step(1);
  param.set_bars_per_step(2);
  param.set_model_dim(4);
  param.set_shuffle(true);
  param.set_percentage(100);
  param.set_temperature(1.);
  param.set_batch_size(1);
  param.set_verbose(true);
  param.set_encoder("TRACK_BAR_FILL_DENSITY_ENCODER");
  param.set_ckpt(ckpt_path);

  // load a midi file
  //midi::Piece p;
  //TrackDensityEncoder encoder;
  //encoder.midi_to_piece(midi_path, &p);  

  // resample part bars 4, 5, 6, and 7 of track 0
  //std::set<std::tuple<int,int>> bar_list = {{0,0},{0,1},{0,2},{0,3}};
  //s = piece_to_status(&p);
  //status_rehighlight(&s, bar_list);

  midi::Piece p;
  midi::Status s;
  load_piece_from_json_file("/users/jeff/CODE/MMM_API/debug/piece.json", &p);
  load_status_from_json_file("/users/jeff/CODE/MMM_API/debug/status.json", &s);
  
  // note that sample works in place
  sample(&p, &s, &param);

  std::string save_path = "out.mid";
  write_midi(&p, save_path);


  /*

  // resample track 1 autoregressively
  bar_list.clear();
  for (int i=0; i<p.tracks(1).bars_size(); i++) {
    bar_list.insert( std::make_tuple(1,i) );
  }
  s = piece_to_status(&p);
  status_rehighlight(&s, bar_list);

  // do generation
  sample(&p, &s, &param);  

  */

}

