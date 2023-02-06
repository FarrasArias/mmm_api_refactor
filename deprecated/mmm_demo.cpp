#include <vector>
#include <string>
#include <map>
#include <tuple>

#include "midi_io.h" // only needed for MIDI input/output
#include "sample_internal.h"
#include "protobuf/midi.pb.h"

#include <google/protobuf/util/json_util.h>

// I believe that you need the full path to the model file.
// Relative paths did not seem to work on my machine.
// You can replace this with opz.pt to try unquantized version.
std::string ckpt_path = "/Users/Jeff/Code/MMM_OPZ/opz_quant.pt";

// This is the structure we use to define different models that are used.
// The keys in the map are the number of bars and the model type.
// The values in the map are the encoding class and the ckpt path.
std::map<std::tuple<int,MODEL_TYPE>,std::tuple<ENCODER_TYPE,std::string>> ckpt_map = {
    {{4,TRACK_MODEL},{TE_TRACK_DENSITY_ENCODER,ckpt_path}},
    {{4,BAR_INFILL_MODEL},{TE_TRACK_DENSITY_ENCODER,ckpt_path}}
};

// this is a helper function that creates a StatusTrack given the
// appropriate arguments
void add_new_track(midi::Status *status, int track_id, int track_type, std::string instrument, int density, std::vector<bool> selected_bars) {
  midi::StatusTrack *track = status->add_tracks();
  track->set_track_id(track_id);
  track->set_track_type(track_type);
  track->set_instrument(instrument);
  track->set_density(density);
  for (const auto x : selected_bars) {
    track->add_selected_bars( x );
  }
}

// this is a helper function that converts a midi::Piece into a midi::Status
void status_from_piece(midi::Status *status, midi::Piece *piece) {
  status->Clear();
  int track_num = 0;
  for (const auto track : piece->tracks()) {
    midi::StatusTrack *strack = status->add_tracks();
    strack->set_track_id( track_num );
    strack->set_track_type( track.type() );
    strack->set_instrument(gm_inst_to_string(track.type(),track.instrument()));
    strack->set_density( track.note_density_v2() );
    for (int i=0; i<4; i++) {
      strack->add_selected_bars( false );
    }
    track_num++;
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

void write_status(midi::Status x, const char *path) {
  std::string json_string;
  google::protobuf::util::JsonPrintOptions opt;
  opt.add_whitespace = true;
  google::protobuf::util::MessageToJsonString(x, &json_string, opt);
  std::ofstream out(path);
  out << json_string;
  out.close();
}

int main() {

  int BATCH_SIZE = 1;
  bool VERBOSE = true;
  std::vector<midi::Piece> output;
  std::string path;
  midi::Status status;
  midi::Piece piece;
  float temp = .975; // temperature for sampling

  // ex 1.
  // generate some new tracks from scratch
  // For a list of track types, see src/mmm_opz/enum/te.h.
  // There is one corresponding to each track on the OPZ.
  // For a list of instruments, see GM in src/mmm_opz/gm.h.
  // Any allows the model to pick any instrument.
  add_new_track(&status, 0, OPZ_KICK_TRACK, "any", -1, {true,true,true,true});
  add_new_track(&status, 1, OPZ_SNARE_TRACK, "any", -1, {true,true,true,true});
  add_new_track(&status, 2, OPZ_HIHAT_TRACK, "any", -1, {true,true,true,true});
  add_new_track(&status, 3, OPZ_BASS_TRACK, "any", -1, {true,true,true,true});
  output = generate(&status, &piece, temp, BATCH_SIZE, VERBOSE, ckpt_map);
  piece = output[0]; // since BATCH_SIZE==1 just use the first output

  // output to midi if needed / wanted
  path = "ex_1.mid";
  write_midi(&piece, path);

  // for examining the relation between midi and piece
  write_piece(piece, "ex_1_piece.json");
  write_status(status, "ex_1_status.json");

  // ex 2.
  // using the previously generated output lets resample some bars
  // specifically we are resampling the 3rd bar in the 2nd track
  // and the 2nd bar in the 4th track.
  status_from_piece(&status, &piece);
  status.mutable_tracks(1)->set_selected_bars(2,true);
  status.mutable_tracks(3)->set_selected_bars(1,true);
  output = generate(&status, &piece, temp, BATCH_SIZE, VERBOSE, ckpt_map);
  piece = output[0]; // since BATCH_SIZE==1 just use the first output

  // output to midi if needed / wanted
  path = "ex_2.mid";
  write_midi(&piece, path);

  write_piece(piece, "ex_2_piece.json");
  write_status(status, "ex_2_status.json");

  // ex 3.
  // using the previously generated output lets resample 
  // specifically we are resampling the 4th track
  // we also set the density and instrument
  status_from_piece(&status, &piece);
  midi::StatusTrack *track = status.mutable_tracks(3);
  for (int bar_num=0; bar_num<4; bar_num++) {
    track->set_selected_bars(bar_num,true);
  }
  track->set_density(7);
  track->set_instrument("Acoustic Bass");
  output = generate(&status, &piece, temp, BATCH_SIZE, VERBOSE, ckpt_map);
  piece = output[0]; // since BATCH_SIZE==1 just use the first output

  // output to midi if needed / wanted
  path = "ex_3.mid";
  write_midi(&piece, path);

  write_piece(piece, "ex_3_piece.json");
  write_status(status, "ex_3_status.json");

}