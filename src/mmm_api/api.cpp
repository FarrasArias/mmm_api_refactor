#include "api.h"


int main() {

  int num_tracks = 1;
  int num_bars = 4;

  // construct empty piece with no notes
  midi::Piece piece;
  piece.set_tempo(120);
  piece.set_resolution(12);
  for (int track_num=0; track_num<num_tracks; track_num++) {
    midi::Track *track = piece.add_tracks();
    track->set_instrument( 0 ); // midi instrument 0
    track->set_track_type( midi::STANDARD_TRACK ); // specifies if drum or not
    for (int bar_num=0; bar_num<num_bars; bar_num++) {
      midi::Bar *bar = track->add_bars();
      bar->set_ts_numerator( 4 ); // time signature
      bar->set_ts_denominator( 4 ); // time signature
    }
  }

  // construct status from the piece
  midi::Status status;
  for (int track_num=0; track_num<num_tracks; track_num++) {
    midi::Track track = piece.tracks(track_num);
    midi::StatusTrack *strack = status.add_tracks();
    strack->set_track_id( track_num );
    strack->set_track_type( track.track_type() );
    strack->set_density( midi::DENSITY_ANY ); // allow model to pick density
    // we use an enum for instrument to allow for multiple instruments to be masked / selected
    strack->set_instrument(
      mmm::gm_inst_to_string(track.track_type(),track.instrument()));
    strack->set_polyphony_hard_limit( 10 );
    strack->set_temperature( 1. );
    for (int i=0; i<track.bars_size(); i++) {
      strack->add_selected_bars( true );
    }
    strack->set_autoregressive( true ); // enable auto-regressive sampling rather than infilling
  }

  // some basic sample parameters
  midi::SampleParam param;
  param.set_tracks_per_step(1);
  param.set_bars_per_step(4);
  param.set_model_dim(4);
  param.set_percentage(100); // this has no effect when using auto-regressive
  param.set_batch_size(1); // this is decrecated and to be removed soon
  param.set_temperature(1);
  param.set_use_per_track_temperature(false);
  param.set_max_steps(0); // this can be used to "give up" after generating n tokens
  param.set_verbose(false);
  param.set_ckpt("/users/jeff/CODE/MMM_TRAINING/models/paper_bar_4_WMETA.pt");

  mmm_api_sample(&piece, &status, &param, NULL);

}