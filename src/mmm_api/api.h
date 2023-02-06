#include "sampling/multi_step_sample.h"

/*

For a thorough description of each of midi::Piece, midi::Status and midi::SampleParam please see the documentation in doc/docs.md.

*/

void mmm_api_sample(midi::Piece *piece, midi::Status *status, midi::SampleParam *param, mmm::CallbackManager *callbacks=NULL) {
  sample_w_debug(piece, status, param, NULL, callbacks);
}