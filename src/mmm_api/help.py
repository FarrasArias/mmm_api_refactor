import json
TOKEN_TYPE = json.load(open("enum/token_types.json","r"))
ENCODER_TYPE = json.load(open("encoder/encoders.json","r"))

# tuples of type, name, default
ENCODER_CONFIG = [
  ("bool", "both_in_one", "false"),
  ("bool", "unquantized", "false"),
  ("bool", "interleaved", "false"),
  ("bool", "multi_segment", "false"),
  ("bool", "te", "false"),
  ("bool", "do_fill", "false"),
  ("bool", "do_multi_fill", "false"),
  ("bool", "do_track_shuffle", "true"),
  ("bool", "do_pretrain_map", "false"),
  ("bool", "force_instrument", "false"),
  ("bool", "mark_note_duration", "false"),
  ("bool", "mark_polyphony", "false"),
  ("bool", "density_continuous", "false"),
  ("bool", "mark_genre", "false"),
  ("bool", "mark_density", "false"),
  ("bool", "mark_instrument", "true"),
  ("bool", "mark_polyphony_quantile", "false"),
  ("bool", "mark_note_duration_quantile", "false"),
  ("bool", "mark_time_sigs", "false"),
  ("bool", "allow_beatlength_matches", "true"),
  ("bool", "instrument_header", "false"),
  ("bool", "use_velocity_levels", "false"),
  ("bool", "genre_header", "false"),
  ("bool", "piece_header", "true"),
  ("bool", "bar_major", "false"),
  ("bool", "force_four_four", "false"),
  ("bool", "segment_mode", "false"),
  ("bool", "force_valid", "false"),
  ("bool", "use_drum_offsets", "true"),
  ("bool", "use_note_duration_encoding", "false"),
  ("bool", "use_absolute_time_encoding", "false"),
  ("bool", "mark_num_bars", "false"),
  ("bool", "mark_drum_density", "false"),
  ("bool", "mark_pitch_limits", "false"),
  ("int", "embed_dim", "0"),
  ("int", "transpose", "0"),
  ("int", "seed", "-1"),
  ("int", "segment_idx", "-1"),
  ("int", "fill_track", "-1"),
  ("int", "fill_bar", "-1"),
  ("int", "max_tracks", "12"),
  ("int", "resolution", "12"),
  ("int", "default_tempo", "104"),
  ("int", "num_bars", "4"),
  ("int", "min_tracks", "1"),
  ("float", "fill_percentage", "0"),
  ("std::set<std::tuple<int,int>>", "multi_fill", None),
  ("std::vector<std::string>", "genre_tags", None)
]

def build_vector(name, values):
  template = """std::vector<std::string> {name} = {{\n {values}\n}};\n\n"""
  return template.format(name=name, values=',\n '.join(['"{}"'.format(v) for v in values]))

def build_enum(name, values):
  template = """enum {name} {{\n  {values}\n}};\n\n"""
  return template.format(name=name, values=",\n  ".join(values))

def build_py_enum(name, values):
  header = """py::enum_<{name}>(m, "{no_namespace_name}", py::arithmetic())
"""
  footer = """
  .export_values();\n\n"""
  content = []
  for value in values:
    content.append( """  .value("{val}", {name}::{val})""".format(val=value,name=name) )
  return header.format(name=name,no_namespace_name=name[5:]) + "\n".join(content) + footer

def build_get_encoder(values):
  header = """ENCODER* getEncoder(ENCODER_TYPE et) {
  switch (et) {
"""
  footer = """
    case NO_ENCODER: return NULL;
  }
}\n\n"""
  content = []
  for value in values:
    cname = "".join([w.capitalize() for w in value.split("_")])
    content.append( """    case {}: return new {}();""".format(value, cname) )
  return header + "\n".join(content) + footer

def build_get_encoder_uniq_ptr(values):
  header = """std::unique_ptr<ENCODER> getEncoder(ENCODER_TYPE et) {
  ENCODER *e;
  switch (et) {
"""
  footer = """
    case NO_ENCODER: return NULL;
  }
}\n\n"""
  content = []
  for value in values:
    cname = "".join([w.capitalize() for w in value.split("_")])
    content.append( """    case {}:\n\t\t\te = new {}();\n\t\t\treturn std::unique_ptr<ENCODER>(e);\n\t\t\tbreak;""".format(value, cname) )
  return header + "\n".join(content) + footer

def build_get_encoder_type(values):
  header = """ENCODER_TYPE getEncoderType(const std::string &s) {
"""
  footer = """
  return NO_ENCODER;
}\n\n"""
  content = []
  for value in values:
    content.append( """  if (s == "{}") return {};""".format(value,value) )
  return header + "\n".join(content) + footer

def build_toString(values):
  header = """inline const char* toString(mmm::TOKEN_TYPE tt) {
  switch (tt) {
"""
  footer = """
  }
}\n\n"""
  content = []
  for value in values:
    content.append( """    case {}: return "{}";""".format(value,value) )
  return header + "\n".join(content) + footer

def build_py_encoder_class(name):
  template = """py::class_<{name}>(m, "{pyname}")
  .def(py::init<>())
  .def("encode", &{name}::encode)
  .def("decode", &{name}::decode)
  .def("midi_to_json", &{name}::midi_to_json)
  .def("midi_to_json_bytes", &{name}::midi_to_json_bytes)
  .def("midi_to_tokens", &{name}::midi_to_tokens)
  .def("json_to_midi", &{name}::json_to_midi)
  .def("json_track_to_midi", &{name}::json_track_to_midi)
  .def("json_to_tokens", &{name}::json_to_tokens)
  .def("tokens_to_json", &{name}::tokens_to_json)
  .def("tokens_to_midi", &{name}::tokens_to_midi)
  .def_readwrite("config", &{name}::config)
  .def_readwrite("rep", &{name}::rep);\n\n"""
  cname = "".join([w.capitalize() for w in name.split("_")])
  return template.format(name="mmm::" + cname, pyname=cname)

def build_encoder_config_class():
  template = """class EncoderConfig {{
public:
  EncoderConfig() {{
{init}
  }}
{declare}
}};"""
  init_content = []
  declare_content = []
  for ftype,fname,fdefault in ENCODER_CONFIG:
    if fdefault is not None:
      init_content.append( "    {} = {};".format(fname, fdefault))
    declare_content.append( "  {} {};".format(ftype, fname))
  return template.format(
    init="\n".join(init_content), 
    declare="\n".join(declare_content))

def build_py_encoder_config_class():
  template = """py::class_<EncoderConfig>(m, "EncoderConfig")
  .def(py::init<>())
{content};\n
"""
  content = []
  for ftype,fname,fdefault in ENCODER_CONFIG:
    content.append("""  .def_readwrite("{name}", &EncoderConfig::{name})""".format(name=fname))
  return template.format(content="\n".join(content))


# token_types.h
with open("enum/token_types.h", "w") as f:
  f.write("""#pragma once

namespace mmm {

""")
  f.write(build_enum("TOKEN_TYPE", TOKEN_TYPE))
  f.write(build_toString(TOKEN_TYPE))
  f.write("}")

# encoder_types.h
with open("enum/encoder_types.h", "w") as f:
  f.write("""#pragma once\n
#include "../encoder/encoder_all.h"
#include <string>

// START OF NAMESPACE
namespace mmm {

""")
  f.write(build_enum("ENCODER_TYPE", ENCODER_TYPE))
  f.write(build_vector("ENCODER_TYPE_STRINGS", ENCODER_TYPE))
  #f.write(build_get_encoder(ENCODER_TYPE[:-1]))
  f.write(build_get_encoder_uniq_ptr(ENCODER_TYPE[:-1]))
  f.write(build_get_encoder_type(ENCODER_TYPE[:-1]))
  f.write("""int getEncoderSize(ENCODER_TYPE et) {
  std::unique_ptr<ENCODER> encoder = getEncoder(et);
  if (!encoder) {
    return 0;
  }
  int size = encoder->rep->max_token();
  //delete encoder;
  return size;
}""")
  f.write("""
}
// END OF NAMESPACE
""")

# encoder_config.h
with open("enum/encoder_config.h", "w") as f:
  f.write("""#pragma once
  
#include <vector>
#include <tuple>
#include <map>
""")
  f.write(build_encoder_config_class())

# stuff for lib.cpp
header = open("lib_header.h").read()

footer = """}"""

#from feature import build_features

with open("lib.cpp", "w") as f:
  f.write(header + "\n\n")
  f.write(build_py_encoder_config_class())
  f.write(build_py_enum("mmm::TOKEN_TYPE", TOKEN_TYPE))
  f.write(build_py_enum("mmm::ENCODER_TYPE", ENCODER_TYPE))
  f.write(""" 

// =========================================================
// =========================================================
// ENCODERS
// =========================================================
// =========================================================

""")
  for enc in ENCODER_TYPE[:-1]:
    f.write(build_py_encoder_class(enc))
  #f.write(build_features())
  f.write(footer)