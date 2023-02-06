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

std::tuple<std::string,std::string,std::string> blank(std::string &piece_json, std::string &status_json, std::string &param_json) {
  midi::Piece p;
  midi::Status s;
  midi::SampleParam h;
  google::protobuf::util::JsonStringToMessage(piece_json.c_str(), &p);
  google::protobuf::util::JsonStringToMessage(status_json.c_str(), &s);
  google::protobuf::util::JsonStringToMessage(param_json.c_str(), &h);
  std::string po;
  std::string so;
  std::string ho;
  google::protobuf::util::MessageToJsonString(p, &po);
  google::protobuf::util::MessageToJsonString(s, &so);
  google::protobuf::util::MessageToJsonString(h, &ho);
  return std::make_tuple(po, so, ho);
}

PYBIND11_MODULE(mmm_api,m) {

  m.def("get_genres", &mmm::get_genres);

  // functions from piano roll.h
  m.def("fast_bit_roll64", &mmm::fast_bit_roll64);
  m.def("bit_to_bool", &mmm::bit_to_bool);
  m.def("bool_to_bit", &mmm::bool_to_bit);

  m.def("status_from_piece", &mmm::status_from_piece_py);
  m.def("blank", &blank);
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