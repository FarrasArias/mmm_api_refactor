#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
namespace py = pybind11;

#include "midi_io.h"
#include "dataset/jagged.h"
#include "encoder/encoder_all.h"
#include "enum/model_type.h"
#include "sampling/control.h"
#include "sampling/util.h"
#include "version.h"

#include "piano_roll.h"

#ifndef NOTORCH
#include "sampling/sample_internal.h"
#include "sampling/multi_step_sample.h"

namespace mmm {
std::string generate_py(std::string &status_str, std::string &piece_str, std::string &param_str) {
  midi::Piece piece;
  google::protobuf::util::JsonStringToMessage(piece_str.c_str(), &piece);
  midi::Status status;
  google::protobuf::util::JsonStringToMessage(status_str.c_str(), &status);
  midi::SampleParam param;
  google::protobuf::util::JsonStringToMessage(param_str.c_str(), &param);

  sample(&piece, &status, &param);
  std::string output_str;
  google::protobuf::util::MessageToJsonString(piece, &output_str);
  //std::vector<midi::Piece> output = mmm::generate(
  //  &status, &piece, NULL, temp, batch_size, verbose, ckpt_map, max_steps);
  //std::vector<std::string> output_str;
  //for (const auto x : output) {
  //  std::string json_string;
  //  google::protobuf::util::MessageToJsonString(x, &json_string);
  //  output_str.push_back( json_string );
  //}
  return output_str;
}
}
#else 
namespace mmm {
void generate_py() { }
void sample_multi_step_py() { }
}
#endif

PYBIND11_MODULE(mmm_api,m) {

  m.def("get_genres", &mmm::get_genres);

  // functions from piano roll.h
  m.def("fast_bit_roll64", &mmm::fast_bit_roll64);
  m.def("bit_to_bool", &mmm::bit_to_bool);
  m.def("bool_to_bit", &mmm::bool_to_bit);

  m.def("status_from_piece", &mmm::status_from_piece_py);
  //m.def("autoregressive_inputs", &mmm::autoregressive_inputs_py);

  m.def("random_perturb", &mmm::random_perturb_py);
  m.def("append_piece", &mmm::append_piece_py);
  m.def("update_note_density", &mmm::update_note_density_py);
  m.def("update_valid_segments", &mmm::update_valid_segments_py);
  m.def("select_random_segment", &mmm::select_random_segment_py);
  m.def("reorder_tracks", &mmm::reorder_tracks_py);
  m.def("prune_tracks", &mmm::prune_tracks_py);
  m.def("prune_empty_tracks", &mmm::prune_empty_tracks_py);
  m.def("prune_notes_wo_offset", &mmm::prune_notes_wo_offset_py);

  m.def("version", &version);
  m.def("getEncoderSize", &mmm::getEncoderSize);
  m.def("getEncoderType", &mmm::getEncoderType);
  m.def("getEncoder", &mmm::getEncoder);

  m.def("gm_inst_to_string", &mmm::gm_inst_to_string);

  m.def("generate", &mmm::generate_py);

  m.def("sample_multi_step", &mmm::sample_multi_step_py);
  m.def("piece_to_status", &mmm::piece_to_status_py);
  m.def("default_sample_param", &mmm::default_sample_param_py);
  m.def("print_piece_summary", &mmm::print_piece_summary_py);
  m.def("flatten_velocity", &mmm::flatten_velocity_py);
  m.def("update_av_polyphony_and_note_duration", &mmm::update_av_polyphony_and_note_duration_py);

  m.def("piece_to_onset_distribution", &mmm::piece_to_onset_distribution_py);

  py::enum_<mmm::MODEL_TYPE>(m, "MODEL_TYPE", py::arithmetic())
    .value("TRACK_MODEL", mmm::MODEL_TYPE::TRACK_MODEL)
    .value("BAR_INFILL_MODEL", mmm::MODEL_TYPE::BAR_INFILL_MODEL)
    .export_values();

  py::class_<mmm::Jagged>(m, "Jagged")
    .def(py::init<std::string &>())
    .def("set_seed", &mmm::Jagged::set_seed)
    .def("set_num_bars", &mmm::Jagged::set_num_bars)
    .def("set_min_tracks", &mmm::Jagged::set_min_tracks)
    .def("set_max_tracks", &mmm::Jagged::set_max_tracks)
    .def("set_max_seq_len", &mmm::Jagged::set_max_seq_len)
    .def("enable_write", &mmm::Jagged::enable_write)
    .def("enable_read", &mmm::Jagged::enable_read)
    .def("append", &mmm::Jagged::append)
    .def("read", &mmm::Jagged::read)
    .def("read_bytes", &mmm::Jagged::read_bytes)
    .def("read_json", &mmm::Jagged::read_json)
    .def("read_batch", &mmm::Jagged::read_batch)
    .def("read_batch_v2", &mmm::Jagged::read_batch_v2)
    .def("read_batch_w_feature", &mmm::Jagged::read_batch_w_feature)
    .def("load_piece", &mmm::Jagged::load_piece)
    .def("load_piece_pair", &mmm::Jagged::load_piece_pair)
    .def("load_piece_pair_batch", &mmm::Jagged::load_piece_pair_batch)
    .def("close", &mmm::Jagged::close)
    .def("get_size", &mmm::Jagged::get_size)
    .def("get_split_size", &mmm::Jagged::get_split_size);

  py::class_<mmm::TrainConfig>(m, "TrainConfig")
    .def(py::init<>())
    .def_readwrite("num_bars", &mmm::TrainConfig::num_bars)
    .def_readwrite("min_tracks", &mmm::TrainConfig::min_tracks)
    .def_readwrite("max_tracks", &mmm::TrainConfig::max_tracks)
    .def_readwrite("max_mask_percentage", &mmm::TrainConfig::max_mask_percentage)
    .def_readwrite("opz", &mmm::TrainConfig::opz)
    .def_readwrite("no_max_length", &mmm::TrainConfig::no_max_length)
    .def("to_json", &mmm::TrainConfig::to_json)
    .def("from_json", &mmm::TrainConfig::from_json);

  py::class_<mmm::REPRESENTATION>(m, "REPRESENTATION")
    .def(py::init<std::vector<std::pair<mmm::TOKEN_TYPE,mmm::TOKEN_DOMAIN>>>())
    .def("decode", &mmm::REPRESENTATION::decode)
    .def("is_token_type", &mmm::REPRESENTATION::is_token_type)
    .def("in_domain", &mmm::REPRESENTATION::in_domain)
    //.def("encode_timesig", &mmm::REPRESENTATION::encode_timesig)
    //.def("encode_continuous", &mmm::REPRESENTATION::encode_continuous)
    .def("encode", &mmm::REPRESENTATION::encode)
    .def("encode_partial", &mmm::REPRESENTATION::encode_partial_py_int)
    .def("encode_to_one_hot", &mmm::REPRESENTATION::encode_to_one_hot)
    .def("pretty", &mmm::REPRESENTATION::pretty)
    .def_readonly("vocab_size", &mmm::REPRESENTATION::vocab_size)
    .def("get_type_mask", &mmm::REPRESENTATION::get_type_mask)
    .def("max_token", &mmm::REPRESENTATION::max_token)
    .def_readonly("token_domains", &mmm::REPRESENTATION::token_domains);
  
  py::class_<mmm::TOKEN_DOMAIN>(m, "TOKEN_DOMAIN")
    .def(py::init<int>());
  
  py::class_<mmm::REP_NODE>(m, "REP_NODE")
    .def(py::init<mmm::TOKEN_TYPE>());

  /*
  py::class_<mmm::REP_GRAPH>(m, "REP_GRAPH")
    .def(py::init<mmm::ENCODER_TYPE,mmm::MODEL_TYPE>())
    .def("get_mask", &mmm::REP_GRAPH::get_mask)
    .def("validate_sequence", &mmm::REP_GRAPH::validate_sequence);
  */

py::class_<EncoderConfig>(m, "EncoderConfig")
  .def(py::init<>())
  .def_readwrite("both_in_one", &EncoderConfig::both_in_one)
  .def_readwrite("unquantized", &EncoderConfig::unquantized)
  .def_readwrite("interleaved", &EncoderConfig::interleaved)
  .def_readwrite("multi_segment", &EncoderConfig::multi_segment)
  .def_readwrite("te", &EncoderConfig::te)
  .def_readwrite("do_fill", &EncoderConfig::do_fill)
  .def_readwrite("do_multi_fill", &EncoderConfig::do_multi_fill)
  .def_readwrite("do_track_shuffle", &EncoderConfig::do_track_shuffle)
  .def_readwrite("do_pretrain_map", &EncoderConfig::do_pretrain_map)
  .def_readwrite("force_instrument", &EncoderConfig::force_instrument)
  .def_readwrite("mark_note_duration", &EncoderConfig::mark_note_duration)
  .def_readwrite("mark_polyphony", &EncoderConfig::mark_polyphony)
  .def_readwrite("density_continuous", &EncoderConfig::density_continuous)
  .def_readwrite("mark_genre", &EncoderConfig::mark_genre)
  .def_readwrite("mark_density", &EncoderConfig::mark_density)
  .def_readwrite("mark_instrument", &EncoderConfig::mark_instrument)
  .def_readwrite("mark_polyphony_quantile", &EncoderConfig::mark_polyphony_quantile)
  .def_readwrite("mark_note_duration_quantile", &EncoderConfig::mark_note_duration_quantile)
  .def_readwrite("mark_time_sigs", &EncoderConfig::mark_time_sigs)
  .def_readwrite("allow_beatlength_matches", &EncoderConfig::allow_beatlength_matches)
  .def_readwrite("instrument_header", &EncoderConfig::instrument_header)
  .def_readwrite("use_velocity_levels", &EncoderConfig::use_velocity_levels)
  .def_readwrite("genre_header", &EncoderConfig::genre_header)
  .def_readwrite("piece_header", &EncoderConfig::piece_header)
  .def_readwrite("bar_major", &EncoderConfig::bar_major)
  .def_readwrite("force_four_four", &EncoderConfig::force_four_four)
  .def_readwrite("segment_mode", &EncoderConfig::segment_mode)
  .def_readwrite("force_valid", &EncoderConfig::force_valid)
  .def_readwrite("use_drum_offsets", &EncoderConfig::use_drum_offsets)
  .def_readwrite("use_note_duration_encoding", &EncoderConfig::use_note_duration_encoding)
  .def_readwrite("use_absolute_time_encoding", &EncoderConfig::use_absolute_time_encoding)
  .def_readwrite("mark_num_bars", &EncoderConfig::mark_num_bars)
  .def_readwrite("mark_drum_density", &EncoderConfig::mark_drum_density)
  .def_readwrite("embed_dim", &EncoderConfig::embed_dim)
  .def_readwrite("transpose", &EncoderConfig::transpose)
  .def_readwrite("seed", &EncoderConfig::seed)
  .def_readwrite("segment_idx", &EncoderConfig::segment_idx)
  .def_readwrite("fill_track", &EncoderConfig::fill_track)
  .def_readwrite("fill_bar", &EncoderConfig::fill_bar)
  .def_readwrite("max_tracks", &EncoderConfig::max_tracks)
  .def_readwrite("resolution", &EncoderConfig::resolution)
  .def_readwrite("default_tempo", &EncoderConfig::default_tempo)
  .def_readwrite("num_bars", &EncoderConfig::num_bars)
  .def_readwrite("min_tracks", &EncoderConfig::min_tracks)
  .def_readwrite("fill_percentage", &EncoderConfig::fill_percentage)
  .def_readwrite("multi_fill", &EncoderConfig::multi_fill)
  .def_readwrite("genre_tags", &EncoderConfig::genre_tags);

py::enum_<mmm::TOKEN_TYPE>(m, "TOKEN_TYPE", py::arithmetic())
  .value("PIECE_START", mmm::TOKEN_TYPE::PIECE_START)
  .value("NOTE_ONSET", mmm::TOKEN_TYPE::NOTE_ONSET)
  .value("NOTE_OFFSET", mmm::TOKEN_TYPE::NOTE_OFFSET)
  .value("PITCH", mmm::TOKEN_TYPE::PITCH)
  .value("NON_PITCH", mmm::TOKEN_TYPE::NON_PITCH)
  .value("VELOCITY", mmm::TOKEN_TYPE::VELOCITY)
  .value("TIME_DELTA", mmm::TOKEN_TYPE::TIME_DELTA)
  .value("TIME_ABSOLUTE", mmm::TOKEN_TYPE::TIME_ABSOLUTE)
  .value("INSTRUMENT", mmm::TOKEN_TYPE::INSTRUMENT)
  .value("BAR", mmm::TOKEN_TYPE::BAR)
  .value("BAR_END", mmm::TOKEN_TYPE::BAR_END)
  .value("TRACK", mmm::TOKEN_TYPE::TRACK)
  .value("TRACK_END", mmm::TOKEN_TYPE::TRACK_END)
  .value("DRUM_TRACK", mmm::TOKEN_TYPE::DRUM_TRACK)
  .value("FILL_IN", mmm::TOKEN_TYPE::FILL_IN)
  .value("FILL_IN_PLACEHOLDER", mmm::TOKEN_TYPE::FILL_IN_PLACEHOLDER)
  .value("FILL_IN_START", mmm::TOKEN_TYPE::FILL_IN_START)
  .value("FILL_IN_END", mmm::TOKEN_TYPE::FILL_IN_END)
  .value("HEADER", mmm::TOKEN_TYPE::HEADER)
  .value("VELOCITY_LEVEL", mmm::TOKEN_TYPE::VELOCITY_LEVEL)
  .value("GENRE", mmm::TOKEN_TYPE::GENRE)
  .value("DENSITY_LEVEL", mmm::TOKEN_TYPE::DENSITY_LEVEL)
  .value("TIME_SIGNATURE", mmm::TOKEN_TYPE::TIME_SIGNATURE)
  .value("SEGMENT", mmm::TOKEN_TYPE::SEGMENT)
  .value("SEGMENT_END", mmm::TOKEN_TYPE::SEGMENT_END)
  .value("SEGMENT_FILL_IN", mmm::TOKEN_TYPE::SEGMENT_FILL_IN)
  .value("NOTE_DURATION", mmm::TOKEN_TYPE::NOTE_DURATION)
  .value("AV_POLYPHONY", mmm::TOKEN_TYPE::AV_POLYPHONY)
  .value("MIN_POLYPHONY", mmm::TOKEN_TYPE::MIN_POLYPHONY)
  .value("MAX_POLYPHONY", mmm::TOKEN_TYPE::MAX_POLYPHONY)
  .value("MIN_NOTE_DURATION", mmm::TOKEN_TYPE::MIN_NOTE_DURATION)
  .value("MAX_NOTE_DURATION", mmm::TOKEN_TYPE::MAX_NOTE_DURATION)
  .value("NUM_BARS", mmm::TOKEN_TYPE::NUM_BARS)
  .value("NONE", mmm::TOKEN_TYPE::NONE)
  .export_values();

py::enum_<mmm::ENCODER_TYPE>(m, "ENCODER_TYPE", py::arithmetic())
  .value("TE_TRACK_DENSITY_ENCODER", mmm::ENCODER_TYPE::TE_TRACK_DENSITY_ENCODER)
  .value("TRACK_DENSITY_ENCODER_V2", mmm::ENCODER_TYPE::TRACK_DENSITY_ENCODER_V2)
  .value("TRACK_INTERLEAVED_ENCODER", mmm::ENCODER_TYPE::TRACK_INTERLEAVED_ENCODER)
  .value("TRACK_INTERLEAVED_W_HEADER_ENCODER", mmm::ENCODER_TYPE::TRACK_INTERLEAVED_W_HEADER_ENCODER)
  .value("TRACK_ENCODER", mmm::ENCODER_TYPE::TRACK_ENCODER)
  .value("TRACK_NO_INST_ENCODER", mmm::ENCODER_TYPE::TRACK_NO_INST_ENCODER)
  .value("TRACK_UNQUANTIZED_ENCODER", mmm::ENCODER_TYPE::TRACK_UNQUANTIZED_ENCODER)
  .value("TRACK_DENSITY_ENCODER", mmm::ENCODER_TYPE::TRACK_DENSITY_ENCODER)
  .value("TRACK_BAR_FILL_DENSITY_ENCODER", mmm::ENCODER_TYPE::TRACK_BAR_FILL_DENSITY_ENCODER)
  .value("TRACK_NOTE_DURATION_ENCODER", mmm::ENCODER_TYPE::TRACK_NOTE_DURATION_ENCODER)
  .value("TRACK_NOTE_DURATION_CONT_ENCODER", mmm::ENCODER_TYPE::TRACK_NOTE_DURATION_CONT_ENCODER)
  .value("TRACK_NOTE_DURATION_EMBED_ENCODER", mmm::ENCODER_TYPE::TRACK_NOTE_DURATION_EMBED_ENCODER)
  .value("DENSITY_GENRE_ENCODER", mmm::ENCODER_TYPE::DENSITY_GENRE_ENCODER)
  .value("DENSITY_GENRE_TAGTRAUM_ENCODER", mmm::ENCODER_TYPE::DENSITY_GENRE_TAGTRAUM_ENCODER)
  .value("DENSITY_GENRE_LASTFM_ENCODER", mmm::ENCODER_TYPE::DENSITY_GENRE_LASTFM_ENCODER)
  .value("POLYPHONY_ENCODER", mmm::ENCODER_TYPE::POLYPHONY_ENCODER)
  .value("POLYPHONY_DURATION_ENCODER", mmm::ENCODER_TYPE::POLYPHONY_DURATION_ENCODER)
  .value("DURATION_ENCODER", mmm::ENCODER_TYPE::DURATION_ENCODER)
  .value("NEW_DURATION_ENCODER", mmm::ENCODER_TYPE::NEW_DURATION_ENCODER)
  .value("TE_ENCODER", mmm::ENCODER_TYPE::TE_ENCODER)
  .value("NEW_VELOCITY_ENCODER", mmm::ENCODER_TYPE::NEW_VELOCITY_ENCODER)
  .value("ABSOLUTE_ENCODER", mmm::ENCODER_TYPE::ABSOLUTE_ENCODER)
  .value("MULTI_LENGTH_ENCODER", mmm::ENCODER_TYPE::MULTI_LENGTH_ENCODER)
  .value("TE_VELOCITY_DURATION_POLYPHONY_ENCODER", mmm::ENCODER_TYPE::TE_VELOCITY_DURATION_POLYPHONY_ENCODER)
  .value("TE_VELOCITY_ENCODER", mmm::ENCODER_TYPE::TE_VELOCITY_ENCODER)
  .value("EL_VELOCITY_DURATION_POLYPHONY_ENCODER", mmm::ENCODER_TYPE::EL_VELOCITY_DURATION_POLYPHONY_ENCODER)
  .value("EL_VELOCITY_DURATION_POLYPHONY_YELLOW_ENCODER", mmm::ENCODER_TYPE::EL_VELOCITY_DURATION_POLYPHONY_YELLOW_ENCODER)
  .value("EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER", mmm::ENCODER_TYPE::EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER)
  .value("NO_ENCODER", mmm::ENCODER_TYPE::NO_ENCODER)
  .export_values();

 

// =========================================================
// =========================================================
// ENCODERS
// =========================================================
// =========================================================

py::class_<mmm::TeTrackDensityEncoder>(m, "TeTrackDensityEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TeTrackDensityEncoder::encode)
  .def("decode", &mmm::TeTrackDensityEncoder::decode)
  .def("midi_to_json", &mmm::TeTrackDensityEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TeTrackDensityEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TeTrackDensityEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TeTrackDensityEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TeTrackDensityEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TeTrackDensityEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TeTrackDensityEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TeTrackDensityEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TeTrackDensityEncoder::config)
  .def_readwrite("rep", &mmm::TeTrackDensityEncoder::rep);

py::class_<mmm::TrackDensityEncoderV2>(m, "TrackDensityEncoderV2")
  .def(py::init<>())
  .def("encode", &mmm::TrackDensityEncoderV2::encode)
  .def("decode", &mmm::TrackDensityEncoderV2::decode)
  .def("midi_to_json", &mmm::TrackDensityEncoderV2::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackDensityEncoderV2::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackDensityEncoderV2::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackDensityEncoderV2::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackDensityEncoderV2::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackDensityEncoderV2::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackDensityEncoderV2::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackDensityEncoderV2::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackDensityEncoderV2::config)
  .def_readwrite("rep", &mmm::TrackDensityEncoderV2::rep);

py::class_<mmm::TrackInterleavedEncoder>(m, "TrackInterleavedEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackInterleavedEncoder::encode)
  .def("decode", &mmm::TrackInterleavedEncoder::decode)
  .def("midi_to_json", &mmm::TrackInterleavedEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackInterleavedEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackInterleavedEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackInterleavedEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackInterleavedEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackInterleavedEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackInterleavedEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackInterleavedEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackInterleavedEncoder::config)
  .def_readwrite("rep", &mmm::TrackInterleavedEncoder::rep);

py::class_<mmm::TrackInterleavedWHeaderEncoder>(m, "TrackInterleavedWHeaderEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackInterleavedWHeaderEncoder::encode)
  .def("decode", &mmm::TrackInterleavedWHeaderEncoder::decode)
  .def("midi_to_json", &mmm::TrackInterleavedWHeaderEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackInterleavedWHeaderEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackInterleavedWHeaderEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackInterleavedWHeaderEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackInterleavedWHeaderEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackInterleavedWHeaderEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackInterleavedWHeaderEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackInterleavedWHeaderEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackInterleavedWHeaderEncoder::config)
  .def_readwrite("rep", &mmm::TrackInterleavedWHeaderEncoder::rep);

py::class_<mmm::TrackEncoder>(m, "TrackEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackEncoder::encode)
  .def("decode", &mmm::TrackEncoder::decode)
  .def("midi_to_json", &mmm::TrackEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackEncoder::config)
  .def_readwrite("rep", &mmm::TrackEncoder::rep);

py::class_<mmm::TrackNoInstEncoder>(m, "TrackNoInstEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackNoInstEncoder::encode)
  .def("decode", &mmm::TrackNoInstEncoder::decode)
  .def("midi_to_json", &mmm::TrackNoInstEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackNoInstEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackNoInstEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackNoInstEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackNoInstEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackNoInstEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackNoInstEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackNoInstEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackNoInstEncoder::config)
  .def_readwrite("rep", &mmm::TrackNoInstEncoder::rep);

py::class_<mmm::TrackUnquantizedEncoder>(m, "TrackUnquantizedEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackUnquantizedEncoder::encode)
  .def("decode", &mmm::TrackUnquantizedEncoder::decode)
  .def("midi_to_json", &mmm::TrackUnquantizedEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackUnquantizedEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackUnquantizedEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackUnquantizedEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackUnquantizedEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackUnquantizedEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackUnquantizedEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackUnquantizedEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackUnquantizedEncoder::config)
  .def_readwrite("rep", &mmm::TrackUnquantizedEncoder::rep);

py::class_<mmm::TrackDensityEncoder>(m, "TrackDensityEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackDensityEncoder::encode)
  .def("decode", &mmm::TrackDensityEncoder::decode)
  .def("midi_to_json", &mmm::TrackDensityEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackDensityEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackDensityEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackDensityEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackDensityEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackDensityEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackDensityEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackDensityEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackDensityEncoder::config)
  .def_readwrite("rep", &mmm::TrackDensityEncoder::rep);

py::class_<mmm::TrackBarFillDensityEncoder>(m, "TrackBarFillDensityEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackBarFillDensityEncoder::encode)
  .def("decode", &mmm::TrackBarFillDensityEncoder::decode)
  .def("midi_to_json", &mmm::TrackBarFillDensityEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackBarFillDensityEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackBarFillDensityEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackBarFillDensityEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackBarFillDensityEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackBarFillDensityEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackBarFillDensityEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackBarFillDensityEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackBarFillDensityEncoder::config)
  .def_readwrite("rep", &mmm::TrackBarFillDensityEncoder::rep);

py::class_<mmm::TrackNoteDurationEncoder>(m, "TrackNoteDurationEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackNoteDurationEncoder::encode)
  .def("decode", &mmm::TrackNoteDurationEncoder::decode)
  .def("midi_to_json", &mmm::TrackNoteDurationEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackNoteDurationEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackNoteDurationEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackNoteDurationEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackNoteDurationEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackNoteDurationEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackNoteDurationEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackNoteDurationEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackNoteDurationEncoder::config)
  .def_readwrite("rep", &mmm::TrackNoteDurationEncoder::rep);

py::class_<mmm::TrackNoteDurationContEncoder>(m, "TrackNoteDurationContEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackNoteDurationContEncoder::encode)
  .def("decode", &mmm::TrackNoteDurationContEncoder::decode)
  .def("midi_to_json", &mmm::TrackNoteDurationContEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackNoteDurationContEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackNoteDurationContEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackNoteDurationContEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackNoteDurationContEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackNoteDurationContEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackNoteDurationContEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackNoteDurationContEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackNoteDurationContEncoder::config)
  .def_readwrite("rep", &mmm::TrackNoteDurationContEncoder::rep);

py::class_<mmm::TrackNoteDurationEmbedEncoder>(m, "TrackNoteDurationEmbedEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TrackNoteDurationEmbedEncoder::encode)
  .def("decode", &mmm::TrackNoteDurationEmbedEncoder::decode)
  .def("midi_to_json", &mmm::TrackNoteDurationEmbedEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TrackNoteDurationEmbedEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TrackNoteDurationEmbedEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TrackNoteDurationEmbedEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TrackNoteDurationEmbedEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TrackNoteDurationEmbedEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TrackNoteDurationEmbedEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TrackNoteDurationEmbedEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TrackNoteDurationEmbedEncoder::config)
  .def_readwrite("rep", &mmm::TrackNoteDurationEmbedEncoder::rep);

py::class_<mmm::DensityGenreEncoder>(m, "DensityGenreEncoder")
  .def(py::init<>())
  .def("encode", &mmm::DensityGenreEncoder::encode)
  .def("decode", &mmm::DensityGenreEncoder::decode)
  .def("midi_to_json", &mmm::DensityGenreEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::DensityGenreEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::DensityGenreEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::DensityGenreEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::DensityGenreEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::DensityGenreEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::DensityGenreEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::DensityGenreEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::DensityGenreEncoder::config)
  .def_readwrite("rep", &mmm::DensityGenreEncoder::rep);

py::class_<mmm::DensityGenreTagtraumEncoder>(m, "DensityGenreTagtraumEncoder")
  .def(py::init<>())
  .def("encode", &mmm::DensityGenreTagtraumEncoder::encode)
  .def("decode", &mmm::DensityGenreTagtraumEncoder::decode)
  .def("midi_to_json", &mmm::DensityGenreTagtraumEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::DensityGenreTagtraumEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::DensityGenreTagtraumEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::DensityGenreTagtraumEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::DensityGenreTagtraumEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::DensityGenreTagtraumEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::DensityGenreTagtraumEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::DensityGenreTagtraumEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::DensityGenreTagtraumEncoder::config)
  .def_readwrite("rep", &mmm::DensityGenreTagtraumEncoder::rep);

py::class_<mmm::DensityGenreLastfmEncoder>(m, "DensityGenreLastfmEncoder")
  .def(py::init<>())
  .def("encode", &mmm::DensityGenreLastfmEncoder::encode)
  .def("decode", &mmm::DensityGenreLastfmEncoder::decode)
  .def("midi_to_json", &mmm::DensityGenreLastfmEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::DensityGenreLastfmEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::DensityGenreLastfmEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::DensityGenreLastfmEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::DensityGenreLastfmEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::DensityGenreLastfmEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::DensityGenreLastfmEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::DensityGenreLastfmEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::DensityGenreLastfmEncoder::config)
  .def_readwrite("rep", &mmm::DensityGenreLastfmEncoder::rep);

py::class_<mmm::PolyphonyEncoder>(m, "PolyphonyEncoder")
  .def(py::init<>())
  .def("encode", &mmm::PolyphonyEncoder::encode)
  .def("decode", &mmm::PolyphonyEncoder::decode)
  .def("midi_to_json", &mmm::PolyphonyEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::PolyphonyEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::PolyphonyEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::PolyphonyEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::PolyphonyEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::PolyphonyEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::PolyphonyEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::PolyphonyEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::PolyphonyEncoder::config)
  .def_readwrite("rep", &mmm::PolyphonyEncoder::rep);

py::class_<mmm::PolyphonyDurationEncoder>(m, "PolyphonyDurationEncoder")
  .def(py::init<>())
  .def("encode", &mmm::PolyphonyDurationEncoder::encode)
  .def("decode", &mmm::PolyphonyDurationEncoder::decode)
  .def("midi_to_json", &mmm::PolyphonyDurationEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::PolyphonyDurationEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::PolyphonyDurationEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::PolyphonyDurationEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::PolyphonyDurationEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::PolyphonyDurationEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::PolyphonyDurationEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::PolyphonyDurationEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::PolyphonyDurationEncoder::config)
  .def_readwrite("rep", &mmm::PolyphonyDurationEncoder::rep);

py::class_<mmm::DurationEncoder>(m, "DurationEncoder")
  .def(py::init<>())
  .def("encode", &mmm::DurationEncoder::encode)
  .def("decode", &mmm::DurationEncoder::decode)
  .def("midi_to_json", &mmm::DurationEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::DurationEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::DurationEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::DurationEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::DurationEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::DurationEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::DurationEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::DurationEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::DurationEncoder::config)
  .def_readwrite("rep", &mmm::DurationEncoder::rep);

py::class_<mmm::NewDurationEncoder>(m, "NewDurationEncoder")
  .def(py::init<>())
  .def("encode", &mmm::NewDurationEncoder::encode)
  .def("decode", &mmm::NewDurationEncoder::decode)
  .def("midi_to_json", &mmm::NewDurationEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::NewDurationEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::NewDurationEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::NewDurationEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::NewDurationEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::NewDurationEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::NewDurationEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::NewDurationEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::NewDurationEncoder::config)
  .def_readwrite("rep", &mmm::NewDurationEncoder::rep);

py::class_<mmm::TeEncoder>(m, "TeEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TeEncoder::encode)
  .def("decode", &mmm::TeEncoder::decode)
  .def("midi_to_json", &mmm::TeEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TeEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TeEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TeEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TeEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TeEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TeEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TeEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TeEncoder::config)
  .def_readwrite("rep", &mmm::TeEncoder::rep);

py::class_<mmm::NewVelocityEncoder>(m, "NewVelocityEncoder")
  .def(py::init<>())
  .def("encode", &mmm::NewVelocityEncoder::encode)
  .def("decode", &mmm::NewVelocityEncoder::decode)
  .def("midi_to_json", &mmm::NewVelocityEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::NewVelocityEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::NewVelocityEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::NewVelocityEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::NewVelocityEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::NewVelocityEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::NewVelocityEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::NewVelocityEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::NewVelocityEncoder::config)
  .def_readwrite("rep", &mmm::NewVelocityEncoder::rep);

py::class_<mmm::AbsoluteEncoder>(m, "AbsoluteEncoder")
  .def(py::init<>())
  .def("encode", &mmm::AbsoluteEncoder::encode)
  .def("decode", &mmm::AbsoluteEncoder::decode)
  .def("midi_to_json", &mmm::AbsoluteEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::AbsoluteEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::AbsoluteEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::AbsoluteEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::AbsoluteEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::AbsoluteEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::AbsoluteEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::AbsoluteEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::AbsoluteEncoder::config)
  .def_readwrite("rep", &mmm::AbsoluteEncoder::rep);

py::class_<mmm::MultiLengthEncoder>(m, "MultiLengthEncoder")
  .def(py::init<>())
  .def("encode", &mmm::MultiLengthEncoder::encode)
  .def("decode", &mmm::MultiLengthEncoder::decode)
  .def("midi_to_json", &mmm::MultiLengthEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::MultiLengthEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::MultiLengthEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::MultiLengthEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::MultiLengthEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::MultiLengthEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::MultiLengthEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::MultiLengthEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::MultiLengthEncoder::config)
  .def_readwrite("rep", &mmm::MultiLengthEncoder::rep);

py::class_<mmm::TeVelocityDurationPolyphonyEncoder>(m, "TeVelocityDurationPolyphonyEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TeVelocityDurationPolyphonyEncoder::encode)
  .def("decode", &mmm::TeVelocityDurationPolyphonyEncoder::decode)
  .def("midi_to_json", &mmm::TeVelocityDurationPolyphonyEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TeVelocityDurationPolyphonyEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TeVelocityDurationPolyphonyEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TeVelocityDurationPolyphonyEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TeVelocityDurationPolyphonyEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TeVelocityDurationPolyphonyEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TeVelocityDurationPolyphonyEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TeVelocityDurationPolyphonyEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TeVelocityDurationPolyphonyEncoder::config)
  .def_readwrite("rep", &mmm::TeVelocityDurationPolyphonyEncoder::rep);

py::class_<mmm::TeVelocityEncoder>(m, "TeVelocityEncoder")
  .def(py::init<>())
  .def("encode", &mmm::TeVelocityEncoder::encode)
  .def("decode", &mmm::TeVelocityEncoder::decode)
  .def("midi_to_json", &mmm::TeVelocityEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::TeVelocityEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::TeVelocityEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::TeVelocityEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::TeVelocityEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::TeVelocityEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::TeVelocityEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::TeVelocityEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::TeVelocityEncoder::config)
  .def_readwrite("rep", &mmm::TeVelocityEncoder::rep);

py::class_<mmm::ElVelocityDurationPolyphonyEncoder>(m, "ElVelocityDurationPolyphonyEncoder")
  .def(py::init<>())
  .def("encode", &mmm::ElVelocityDurationPolyphonyEncoder::encode)
  .def("decode", &mmm::ElVelocityDurationPolyphonyEncoder::decode)
  .def("midi_to_json", &mmm::ElVelocityDurationPolyphonyEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::ElVelocityDurationPolyphonyEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::ElVelocityDurationPolyphonyEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::ElVelocityDurationPolyphonyEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::ElVelocityDurationPolyphonyEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::ElVelocityDurationPolyphonyEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::ElVelocityDurationPolyphonyEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::ElVelocityDurationPolyphonyEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::ElVelocityDurationPolyphonyEncoder::config)
  .def_readwrite("rep", &mmm::ElVelocityDurationPolyphonyEncoder::rep);

py::class_<mmm::ElVelocityDurationPolyphonyYellowEncoder>(m, "ElVelocityDurationPolyphonyYellowEncoder")
  .def(py::init<>())
  .def("encode", &mmm::ElVelocityDurationPolyphonyYellowEncoder::encode)
  .def("decode", &mmm::ElVelocityDurationPolyphonyYellowEncoder::decode)
  .def("midi_to_json", &mmm::ElVelocityDurationPolyphonyYellowEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::ElVelocityDurationPolyphonyYellowEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::ElVelocityDurationPolyphonyYellowEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::ElVelocityDurationPolyphonyYellowEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::ElVelocityDurationPolyphonyYellowEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::ElVelocityDurationPolyphonyYellowEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::ElVelocityDurationPolyphonyYellowEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::ElVelocityDurationPolyphonyYellowEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::ElVelocityDurationPolyphonyYellowEncoder::config)
  .def_readwrite("rep", &mmm::ElVelocityDurationPolyphonyYellowEncoder::rep);

py::class_<mmm::ElVelocityDurationPolyphonyYellowFixedEncoder>(m, "ElVelocityDurationPolyphonyYellowFixedEncoder")
  .def(py::init<>())
  .def("encode", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::encode)
  .def("decode", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::decode)
  .def("midi_to_json", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::midi_to_json)
  .def("midi_to_json_bytes", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::midi_to_json_bytes)
  .def("midi_to_tokens", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::midi_to_tokens)
  .def("json_to_midi", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::json_to_midi)
  .def("json_track_to_midi", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::json_track_to_midi)
  .def("json_to_tokens", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::json_to_tokens)
  .def("tokens_to_json", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::tokens_to_json)
  .def("tokens_to_midi", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::tokens_to_midi)
  .def_readwrite("config", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::config)
  .def_readwrite("rep", &mmm::ElVelocityDurationPolyphonyYellowFixedEncoder::rep);

}