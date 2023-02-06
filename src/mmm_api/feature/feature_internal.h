
#pragma once

#include "feature.h"
#include "../midi_io.h"

typedef struct feat_density { } feat_density;

vector<tuple<int,int,int>> get_feature(string &path, EncoderConfig *config, feat_density x) {
  midi::Piece p;
  parse_new(path, &p, config);
  return calc_feat_density(&p);
}

typedef struct feat_test { } feat_test;

vector<double> get_feature(string &path, EncoderConfig *config, feat_test x) {
  midi::Piece p;
  parse_new(path, &p, config);
  return calc_feat_test(&p);
}
