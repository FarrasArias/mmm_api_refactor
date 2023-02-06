#include <vector>
#include <string>
#include <map>
#include <tuple>

#include "midi_io.h" // only needed for MIDI input/output
#include "sampling/sample_internal.h"
#include "sampling/multi_step_sample.h"
#include "protobuf/util.h"
#include "protobuf/midi.pb.h"

#include <google/protobuf/util/json_util.h>

void load_piece_from_json_file(const char *path, midi::Piece *x) {
  std::ifstream t(path);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  google::protobuf::util::JsonStringToMessage(str.c_str(), x);
}

void load_status_from_json_file(const char *path, midi::Status *x) {
  std::ifstream t(path);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  google::protobuf::util::JsonStringToMessage(str.c_str(), x);
}

midi::SampleParam build_sample_param(std::string estr, std::string ckpt, bool new_state) {
  midi::SampleParam param;
  param.set_tracks_per_step(1);
  param.set_bars_per_step(2);
  param.set_model_dim(4);
  param.set_shuffle(true);
  param.set_percentage(83);
  param.set_temperature(1.);
  param.set_batch_size(1);
  param.set_verbose(true);
  param.set_encoder(estr);
  param.set_ckpt(ckpt);
  param.set_skip_preprocess(false);
  param.set_new_state(new_state);
  param.set_polyphony_hard_limit(5);
  return param;
}

void add_new_track(midi::Status *status, int track_type, std::string instrument, int density, std::vector<bool> selected_bars, bool resample=false) {
  midi::StatusTrack *track = status->add_tracks();
  track->set_track_id((int)status->tracks_size() - 1);
  track->set_track_type(track_type);
  track->set_instrument(instrument);
  track->set_density(density);
  track->set_resample(resample);
  for (const auto x : selected_bars) {
    track->add_selected_bars( x );
  }
}

void write_piece(midi::Piece x, const char *path) {
  std::string json_string;
  google::protobuf::util::JsonPrintOptions opt;
  opt.add_whitespace = true;
  google::protobuf::util::MessageToJsonString(x, &json_string, opt);
  std::ofstream out(path);
  out << json_string;
  out.close();
}

// this is a helper function that converts a midi::Piece into a midi::Status
void status_from_piece(midi::Status *status, midi::Piece *piece) {
  status->Clear();
  int track_num = 0;
  for (const auto track : piece->tracks()) {
    midi::StatusTrack *strack = status->add_tracks();
    strack->set_track_id( track_num );
    strack->set_track_type( track.type() );
    strack->set_instrument( mmm::gm_inst_to_string(track.type(),track.instrument()));
    strack->set_density( 4 );
    for (int i=0; i<4; i++) {
      strack->add_selected_bars( false );
    }
    track_num++;
  }
}

void run_generate(midi::Piece *p, midi::Status *s, std::string estr, std::string ckpt, std::string &save_path, midi::Piece *output, bool multi) {
  bool new_state = true;
  if (estr == "TRACK_BAR_FILL_DENSITY_ENCODER") {
    // support legacy models with alternate huggingface state configuration
    new_state = false;
  }
  midi::SampleParam param = build_sample_param(estr, ckpt, new_state);

  mmm::ENCODER_TYPE et = mmm::getEncoderType(estr);
  mmm::CKPT_MAP_TYPE ckpt_map = {
    {{4,mmm::TRACK_MODEL},{et,ckpt}},
    {{4,mmm::BAR_INFILL_MODEL},{et,ckpt}}
  };

  if (multi) {
    mmm::sample(p, s, &param);
    *output = *p;
  }
  else {
    *output = mmm::generate(s, p, &param, NULL, ckpt_map)[0];
  }

  write_piece(*output, "test.json");
  mmm::write_midi(output, save_path);

}


////////////////////////////////////////////////////////////////
/// TESTS
////////////////////////////////////////////////////////////////

void test(std::string estr, std::string ckpt) {
  midi::Piece piece;
  midi::Piece output;
  midi::Status status;
  std::string save_path = "test.mid";

  // ===================================
  // STATUS TEST
  load_piece_from_json_file(
    "/users/jeff/CODE/MMM_API/debug/jun_7/piece.json", &piece);

  load_status_from_json_file(
    "/users/jeff/CODE/MMM_API/debug/jun_7/status.json", &status);
  
  run_generate(&piece, &status, estr, ckpt, save_path, &output, true);

  /*
  // ===================================
  // SINGLE-STEP TESTS

  // generate three tracks from scratch
  save_path = "test.mid";
  add_new_track(&status, STANDARD_DRUM_TRACK, "any", 0, {true,true,true,true});
  add_new_track(&status, STANDARD_TRACK, "any", 4, {true,true,true,true});
  add_new_track(&status, STANDARD_BOTH, "any", 4, {true,true,true,true});
  run_generate(&piece, &status, estr, ckpt, save_path, &output, false);

  // generate via bar-infilling
  piece = output;
  status_from_piece(&status, &piece);
  for (int i=0; i<5; i++) {
    int track_num = rand() % piece.tracks_size();
    int bar_num = rand() % piece.tracks(track_num).bars_size();
    status.mutable_tracks(track_num)->set_selected_bars(bar_num, true);
  }
  run_generate(&piece, &status, estr, ckpt, save_path, &output, false);

  // ===================================
  // MULTI-STEP TESTS
  */

  /*
  status.Clear();
  
  save_path = "test.mid";
  piece.set_resolution(12);

  std::vector<bool> bars = {true,true,true,true,true,true,true};
  add_new_track(&status, STANDARD_DRUM_TRACK, "any", -1, bars, true);
  add_new_track(&status, STANDARD_TRACK, "any", -1, bars, true);
  
  run_generate(&piece, &status, estr, ckpt, save_path, &output, true);

  */


}


int main() {

  std::string ckpt_path;
  std::string estr;
  
  //ckpt_path = "/users/jeff/CODE/MMM_TRAINING/models/new_duration.pt";
  //estr = "NEW_DURATION_ENCODER";
  //test(estr, ckpt_path);

  //ckpt_path = "/users/jeff/CODE/MMM_API/paper_bar_4.pt";
  ckpt_path = "paper_bar_4.pt";
  estr = "TRACK_BAR_FILL_DENSITY_ENCODER";
  test(estr, ckpt_path);

}

