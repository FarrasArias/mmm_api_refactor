# help generate code for features
# we need to generate the following for each feature

#py::class_<feat_test>(m, "feat_test")
#    .def(py::init<>());

#m.def("get_feature", static_cast<vector<double>(*)(string&,feat_test)>(&get_feature));

import json

feature_pybind_template = """
  py::class_<feat_{name}>(m, "feat_{name}")
    .def(py::init<>());

  m.def("get_feature", static_cast<{return_type}(*)(string&,EncoderConfig*,feat_{name})>(&get_feature));
"""

feature_template = """
typedef struct feat_{name} {{ }} feat_{name};

{return_type} get_feature(string &path, EncoderConfig *config, feat_{name} x) {{
  midi::Piece p;
  parse_new(path, &p, config);
  return calc_feat_{name}(&p);
}}
"""

feature_header = """
#pragma once

#include "feature.h"
#include "../midi_io.h"
"""

def build_features():

  with open("feature/feature.json", "r") as f:
    data = json.load(f)  

  with open("feature/feature_internal.h", "w") as f:
    f.write(feature_header)
    for feature in data:
      f.write(feature_template.format(**feature))
  
  code = []
  for feature in data:
    code.append(feature_pybind_template.format(**feature))

  return "\n".join(code)

if __name__ == "__main__":

  print( build_features() )