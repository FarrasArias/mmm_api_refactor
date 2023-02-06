#pragma once

#include <assert.h>
#include <algorithm>

#include "sample_internal.h"
#include "../protobuf/util.h"

namespace mmm {

template <typename T>
void show_vector(const std::vector<T> &x, bool show_shape=true) {
  if (show_shape) {
    std::cout << "SHAPE : (" << x.size() << ",)" << std::endl;
  }
  for (const auto value : x) {
    std::cout << value << " ";
  }
  std::cout << std::endl;
}

template <typename T>
void show_matrix(const std::vector<std::vector<T>> &x) {
  std::cout << "SHAPE : (" << x.size() << "," << x[0].size() << ")" << std::endl;
  for (const auto row : x) {
    show_vector(row, false);
  }
}

bool any(const std::vector<bool> &x) {
  for (const auto value : x) {
    if (value) {
      return true;
    }
  }
  return false;
}

bool any(const std::vector<std::vector<bool>> &x) {
  for (const auto row : x) {
    if (any(row)) {
      return true;
    }
  }
  return false;
}

bool all(const std::vector<bool> &x) {
  for (const auto value : x) {
    if (!value) {
      return false;
    }
  }
  return true;
}

bool all(const std::vector<std::vector<bool>> &x) {
  for (const auto row : x) {
    if (!all(row)) {
      return false;
    }
  }
  return true;
}

std::vector<std::vector<bool>> status_to_selection_mask(midi::Status *status) {
  // convert the status message into a track x bar matrix
  // which indicates which bars are selected
  int ntracks = status->tracks_size();
  int nbars = status->tracks(0).selected_bars_size();
  std::vector<std::vector<bool>> x(ntracks, std::vector<bool>(nbars,false));
  int track_num = 0;
  for (const auto track : status->tracks()) {
    int bar_num = 0;
    for (const auto bar : track.selected_bars()) {
      x[track_num][bar_num] = bar;
      bar_num++;
    }
    track_num++;
  }
  return x;
}

std::vector<bool> status_to_resample_mask(midi::Status *status) {
  // get a boolean vector that indicates which tracks to resample
  std::vector<bool> resample_mask;
  for (const auto track : status->tracks()) {
    resample_mask.push_back( track.autoregressive() );
  }
  return resample_mask;
}

std::vector<bool> status_to_ignore_mask(midi::Status *status) {
  // get a boolean vector that indicates which tracks to ignore
  std::vector<bool> ignore_mask;
  for (const auto track : status->tracks()) {
    ignore_mask.push_back( track.ignore() );
  }
  return ignore_mask;
}


void status_rehighlight(midi::Status *status, std::set<std::tuple<int,int>> &bar_list) {
  // modify status with a new set of selected bars
  // this works inplace
  int num_tracks = status->tracks_size();
  for (int track_num=0; track_num<num_tracks; track_num++) {
    midi::StatusTrack *track = status->mutable_tracks(track_num);
    int num_bars = track->selected_bars_size();
    track->clear_selected_bars();
    for (int bar_num=0; bar_num<num_bars; bar_num++) {
      bool x = bar_list.find(std::make_tuple(track_num,bar_num)) != bar_list.end();
      track->add_selected_bars(x);

      /// update resample paramater
      if ((track->autoregressive()) && (!x)) {
        track->set_autoregressive( false );
      }
    }
  }
}

midi::Status status_subset(midi::Status *status, int start_bar, int end_bar, std::vector<int> &track_indices) {
  midi::Status subset;
  int track_count = 0;
  for (const auto track_index : track_indices) {
    const midi::StatusTrack track = status->tracks(track_index);
    midi::StatusTrack *t = subset.add_tracks();
    t->CopyFrom(track);
    t->set_track_id(track_count);
    t->clear_selected_bars();
    t->clear_internal_ts_numerators();
    t->clear_internal_ts_denominators();
    for (int i=start_bar; i<end_bar; i++) {
      t->add_selected_bars( track.selected_bars(i) );
      t->add_internal_ts_numerators( track.internal_ts_numerators(i) );
      t->add_internal_ts_denominators( track.internal_ts_denominators(i) );
    }
    track_count++;
  }
  return subset;
}

void update_status_instruments(midi::Piece *piece, midi::Status *status) {

  for (int track_num=0; track_num<status->tracks_size(); track_num++) {
    midi::StatusTrack *st = status->mutable_tracks(track_num);
    midi::Track t = piece->tracks(st->track_id());
    st->set_instrument( gm_inst_to_string(t.track_type(), t.instrument()) );
    st->set_track_type( t.track_type() );
  }
}

midi::Piece piece_subset(midi::Piece *piece, int start_bar, int end_bar, std::vector<int> &track_indices) {
  // grab a subset of a piece
  // NOTE : we do this the lazy way leaving extra events in the events field
  midi::Piece subset;
  subset.set_resolution( piece->resolution() );
  subset.set_tempo( piece->tempo() );
  int track_count = 0;
  for (const auto track_index : track_indices) {
    if (track_index >= piece->tracks_size()) {
      throw std::runtime_error("TRYING TO ACCESS TRACK OUT OF RANGE. PIECE IS LIKELY MALFORMED");
    }
    const midi::Track track = piece->tracks(track_index);
    midi::Track *t = subset.add_tracks();
    t->CopyFrom(track);
    t->clear_bars();
    for (int i=start_bar; i<end_bar; i++) {
      midi::Bar *b = t->add_bars();
      b->CopyFrom( track.bars(i) );
      b->clear_events();

      for (const auto event : track.bars(i).events()) {
        b->add_events( subset.events_size() );
        midi::Event *e = subset.add_events();
        e->CopyFrom( piece->events(event) );
      }
    }
    track_count++;
  }
  return subset;
}

void add_timesigs_to_status(midi::Piece *piece, midi::Status *status) {
  int track_num = 0;
  for (const auto track : piece->tracks()) {
    midi::StatusTrack *st = status->mutable_tracks(track_num);
    for (const auto bar : track.bars()) {
      st->add_internal_ts_numerators( bar.ts_numerator() );
      st->add_internal_ts_denominators( bar.ts_denominator() );
    }
    track_num++;
  }
}

void fix_duplicate_density_quantiles(midi::Status *status) {
  for (int track_num=0; track_num<status->tracks_size(); track_num++) {
    midi::StatusTrack *st = status->mutable_tracks(track_num);
    if ((is_opz_track(st->track_type())) && (st->density() != midi::DENSITY_ANY)) {
      std::tuple<int,int> key = std::make_tuple(
        st->track_type(), st->instrument());
      auto itt = OPZ_FIX_DUPLICATE_QUANTILES.find(key);
		  if (itt != OPZ_FIX_DUPLICATE_QUANTILES.end()) {
        // note : we have to add and subtract to avoid DENSITY_ANY
        st->set_density( 
          static_cast<midi::DensityLevel>(itt->second[st->density()-1]+1) );
      }
    }
  }
}

void override_piece_features(midi::Piece *piece, midi::Status *status) {
  // calculate features first
  // then only override if the controls are not ANY
  update_note_density(piece);
  update_av_polyphony_and_note_duration(piece);

  for (const auto track : status->tracks()) {
    midi::TrackFeatures *f = get_track_features(piece, track.track_id());
    if (track.density() > 0) {
      f->set_note_density_v2( track.density() - 1);
    }
    if (track.min_polyphony_q() > 0) {
      f->set_min_polyphony_q( track.min_polyphony_q() - 1 );
    }
    if (track.max_polyphony_q() > 0) {
      f->set_max_polyphony_q( track.max_polyphony_q() - 1 );
    }
    if (track.min_note_duration_q() > 0) {
      f->set_min_note_duration_q( track.min_note_duration_q() - 1 );
    }
    if (track.max_note_duration_q() > 0) {
      f->set_max_note_duration_q( track.max_note_duration_q() - 1 );
    }

    //f->set_genre_str( track.internal_genre() );
  }
  /*
  if (status->tracks_size()) {
    midi::GenreData *g = piece->add_internal_genre_data();
    g->set_discogs( status->tracks(0).internal_genre() );
    g->set_lastfm( status->tracks(0).internal_genre() );
    g->set_tagtraum( status->tracks(0).internal_genre() );
  }
  */
}

void piece_insert(midi::Piece *piece, midi::Piece *x, std::vector<std::tuple<int,int,int,int>> &bar_mapping, bool verbose) {

  for (const auto ii : bar_mapping) {
    // x track, x bar, piece track, piece bar
    if (std::get<0>(ii) >= x->tracks_size()) {
      throw std::runtime_error("PIECE INSERT :: INVALID TRACK INDEX FOR X");
    }
    if (std::get<2>(ii) >= piece->tracks_size()) {
      throw std::runtime_error("PIECE INSERT :: INVALID TRACK INDEX FOR PIECE");
    }
    const midi::Track src_track = x->tracks(std::get<0>(ii));
    const midi::Bar src = src_track.bars(std::get<1>(ii));
    midi::Track *dst_track = piece->mutable_tracks(std::get<2>(ii));
    midi::Bar *dst = dst_track->mutable_bars(std::get<3>(ii));

    if (verbose) {
      std::cout << "INSERTING (" << std::get<0>(ii) << "," << std::get<1>(ii) << ") into (" << std::get<2>(ii) << "," << std::get<3>(ii) << ")" << std::endl;
    }

    // overwrite instrument and track type (for autoregressive)
    dst_track->set_track_type( src_track.track_type() );
    dst_track->set_instrument( src_track.instrument() );

    // overwrite bar from src
    dst->clear_events();
    for (const auto event_index : src.events()) {
      dst->add_events( piece->events_size() );
      midi::Event *e = piece->add_events();
      e->CopyFrom( x->events(event_index) );
    }
  }
}


void piece_insert_old(midi::Piece *piece, midi::Piece *x, int start, std::vector<int> &track_indices) {
  // insert x into a piece
  // NOTE : we do this the lazy way leaving extra events in the events field
  int track_count = 0;
  for (const auto track_index : track_indices) {
    if (track_count >= x->tracks_size()) {
      throw std::runtime_error("PIECE INSERT :: INVALID TRACK INDEX FOR X");
    }
    if (track_index >= piece->tracks_size()) {
      throw std::runtime_error("PIECE INSERT :: INVALID TRACK INDEX FOR PIECE");
    }
    const midi::Track track = x->tracks(track_count);
    midi::Track *t = piece->mutable_tracks(track_index);
    t->set_track_type( track.track_type() );
    t->set_instrument( track.instrument() );
    //if (track.features_size()) {
    //  get_track_features(piece, track_index)->set_genre_str( 
    //    track.features(0).genre_str());
    //}
    int bar_num = 0;
    for (const auto bar : track.bars()) {
      if (bar_num + start < t->bars_size()) {
        std::cout << "CLEARING BAR :: " << track_index << " " << bar_num + start << std::endl;
        midi::Bar *b = t->mutable_bars(bar_num + start);
        b->clear_events();
        for (const auto event_index : bar.events()) {
          b->add_events( piece->events_size() );
          midi::Event *e = piece->add_events();
          e->CopyFrom( x->events(event_index) );
        }
      }
      bar_num++;
    }
    track_count++;
  }
}

class STEP {
public:
  STEP (int sstart, int eend, std::vector<std::vector<bool>> &sstep, std::vector<std::vector<bool>> &ccontext) {
    start = sstart;
    end = eend;
    step = sstep;
    context = ccontext;
  }

  STEP (const STEP &old) {
    start = old.start;
    end = old.end;
    step = old.step;
    context = old.context;
  }

  STEP () {
    start = 0;
    end = 0;
  }

  int generated_bar_count() const {
    int count = 0;
    for (const auto x : step) {
      for (const auto xx : x) {
        count += (int)xx;
      }
    }
    return count;
  }

  int start;
  int end;
  std::vector<std::vector<bool>> step;
  std::vector<std::vector<bool>> context;
};

void find_steps_inner(std::vector<STEP> &steps, std::vector<std::vector<bool>> &selection_matrix, std::vector<bool> &resample_mask, std::vector<bool> &ignore_mask, bool autoregressive, std::vector<std::vector<bool>> &generated, midi::SampleParam *param) {

  int tracks_per_step = param->tracks_per_step();
  int bars_per_step = param->bars_per_step();
  int model_dim = param->model_dim();
  int current_num_steps = steps.size();

  std::vector<std::vector<bool>> sel(selection_matrix);

  int nt = sel.size();
  int nb = sel[0].size();
  
  // if autoregressive only consider resample_mask
  for (int i=0; i<sel.size(); i++) {
    for (int j=0; j<sel[i].size(); j++) {
      if (autoregressive) {
        sel[i][j] = sel[i][j] & resample_mask[i];
      }
      else {
        sel[i][j] = sel[i][j] & ~resample_mask[i];
      }
    }
  }

  if (param->verbose()) {
    std::cout << "AUTOREGRESSIVE = " << autoregressive << std::endl;
    std::cout << "SELECTION MATRIX : " << std::endl;
    show_matrix(sel);
    std::cout << "GLOBAL SELECTION MATRIX : " << std::endl;
    show_matrix(selection_matrix);
  }

  // generated should be a zeros matrix
  std::vector<std::vector<bool>> covered(sel.size(), std::vector<bool>(sel[0].size(),false));

  // the min tracks per step is 1
  // the max tracks per step is the number of tracks in the piece
  tracks_per_step = std::max(std::min(tracks_per_step, (int)sel.size()), 1);

  // the min bars per step is 1
  // the max bars per step is the model dim
  bars_per_step = std::max(std::min(bars_per_step, model_dim), 1);

  std::vector<int> tracks_to_consider = arange(0, (int)sel.size(), 1);

  int num_context = (model_dim - bars_per_step) / 2;
  if (autoregressive) {
    int num_context = (model_dim - bars_per_step);
  }

  std::vector<std::tuple<int,int>> ijs;
  for (int i=0; i<sel.size(); i=i+tracks_per_step) {
    for (int j=0; j<sel[i].size(); j=j+bars_per_step) {
      ijs.push_back( std::make_tuple(i,j) );
    }
  }

  // we should do these things afterwards
  /*
  if ((param->shuffle()) && (!autoregressive)) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ijs.begin(), ijs.end(), g);
  }
  if ((param->percentage() < 100) && (!autoregressive)) {
    int new_size = ijs.size() * ((float)param->percentage() / 100.);
    ijs.resize(std::max(new_size,1));
  }
  */

  //for (int i=0; i<sel.size(); i=i+tracks_per_step) {
  //  for (int j=0; j<sel[i].size(); j=j+bars_per_step) {
  
  for (const auto ij : ijs) {

    int i = std::get<0>(ij);
    int j = std::get<1>(ij);

    int kii, kjj; // iterators for the kernel

    int num_tracks = std::min(tracks_per_step,(int)sel.size()-i);
    std::vector<std::vector<bool>> kernel(num_tracks,std::vector<bool>(model_dim,false));

    int t = 0;
    if (autoregressive) {
      // for the first step we have no generated material to 
      // condition on so we use entire model window
      // after the first step (j>0) we only generate bars_per_step bars
      int right_offset = std::max((j + model_dim) - nb,0);
      t = std::min(j, nb - model_dim);

      for (int k=(j>0)*(num_context+right_offset); k<model_dim; k++) {
        for (int n=0; n<num_tracks; n++) {
          kernel[n][k] = true;
        }
      } 
    }
    else {
      // we want to have the generated bars at the center
      // this is not possible at beginning and end so we adjust
      // for those cases
      t = std::min(std::max(j - num_context,0), nb - model_dim);
      
      for (int k=j-t; k<j-t+bars_per_step; k++) {
        for (int n=0; n<num_tracks; n++) {
          kernel[n][k] = true;
        }
      }
    }

    
    if (param->verbose()) {
      std::cout << "CHECKING STEP : " << i << " " << j << std::endl;
      std::cout << "T = " << t << std::endl;
      std::cout << "NUM TRACKS = " << num_tracks << std::endl;
    }
    
    
    // check
    //assert not np.any(covered[i:i+num_tracks,t:t+model_dim] & kernel)

    std::vector<std::vector<bool>> step(nt,std::vector<bool>(nb,false));
    std::vector<std::vector<bool>> context(nt,std::vector<bool>(nb,false));

    kii = 0;
    for (int k=i; k<i+num_tracks; k++) {
      kjj = 0;
      for (int n=t; n<t+model_dim; n++) {
        step[k][n] = sel[k][n] * kernel[kii][kjj];
        if (autoregressive) {
          // if autoregressive don't generate what
          // we already have generated ...
          step[k][n] = step[k][n] & (!generated[k][n]);
        }
        kjj++;
      }
      kii++;
    }

    if (param->verbose()) {
      show_matrix(step);
      show_matrix(generated);
    }

    // set context
    for (int k=0; k<nt; k++) {
      for (int n=t; n<t+model_dim; n++) {
        context[k][n] = ~ignore_mask[k] & ~step[k][n];
      }
    }

    // refine context so that we don't look into future
    // on tracks we are auto-regressively sampling
    if (autoregressive) {
      for (int k=0; k<nt; k++) {
        if (any(sel[k])) {
          for (int n=t; n<t+model_dim; n++) {
            context[k][n] = generated[k][n];
          }
        }
      }
    }

    if (any(step)) {
      steps.push_back(STEP(t, t+model_dim, step, context));
    }

    kii = 0;
    for (int k=i; k<i+num_tracks; k++) {
      kjj = 0;
      for (int n=t; n<t+model_dim; n++) {
        generated[k][n] = std::max(generated[k][n], step[k][n]);
        covered[k][n] = std::max(covered[k][n], kernel[kii][kjj]);
        kjj++;
      }
      kii++;
    }

  }

  if (param->verbose()) {
    std::cout << "COVERED MATRIX : " << std::endl;
    show_matrix(covered);
  }
  if (!all(covered)) {
    throw std::runtime_error("PIECE IS ONLY PARTIALLY COVERED");
  }

  if ((!autoregressive) && (param->shuffle())) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(steps.begin() + current_num_steps, steps.end(), g);
  }
  if ((!autoregressive) && (param->percentage() < 100) && (steps.size() > current_num_steps)) {
    int non_autoreg_steps = steps.size() - current_num_steps;
    int new_size = non_autoreg_steps * ((float)param->percentage() / 100.);
    steps.resize(current_num_steps + std::max(new_size,1));
  }

}

std::vector<STEP> find_steps(std::vector<std::vector<bool>> &sel, std::vector<bool> &resample_mask, std::vector<bool> &ignore_mask, midi::SampleParam *param) {

  std::vector<STEP> steps;
  std::vector<std::vector<bool>> generated(sel.size(), std::vector<bool>(sel[0].size(),false));

  find_steps_inner(
    steps, sel, resample_mask, ignore_mask, true, generated, param);
  find_steps_inner(
    steps, sel, resample_mask, ignore_mask, false, generated, param);

  return steps;
}

midi::Piece generate_callback_inner(midi::Piece *piece, midi::Status *status, midi::SampleParam *param, Debugger *debug, ModelMeta *model, CallbackManager *callbacks) {

  return generate(status, piece, param, debug, model, callbacks)[0];

}

void sample_step(midi::Piece *piece, midi::Status *status, midi::SampleParam *param, Debugger *debug, ModelMeta *model, const STEP *s, CallbackManager *callbacks) {

  /*
  if (param->verbose()) {
    std::cout << "STARTING GENERATION STEP ::" << std::endl;
    print_piece_summary(piece);
    print_status(status);
  }
  */
  
  std::set<int> track_set;
  std::set<std::tuple<int,int>> bars_to_generate;
  std::vector<std::tuple<int,int,int,int>> bar_mapping;
  int track_count = 0;
  int num_tracks = s->step.size();
  for (int i=0; i<num_tracks; i++) {
    bool track_used = false;
    for (int j=s->start; j<s->end; j++) {
      if (s->step[i][j]) {
        bars_to_generate.insert( std::make_tuple(track_count,j-s->start) );
        bar_mapping.push_back( std::make_tuple(track_count,j-s->start,i,j) );
      }
      if (s->step[i][j] || s->context[i][j]) {
        track_set.insert( i );
        track_used = true;
      }
    }
    if (track_used) {
      track_count++;
    }
  }
  std::vector<int> tracks(track_set.begin(), track_set.end());
  //std::vector<int> gen_tracks(gen_track_set.begin(), gen_track_set.end());

  midi::Status step_status = status_subset(status, s->start, s->end, tracks);
  if (bars_to_generate.size()) {
    status_rehighlight(&step_status, bars_to_generate);
  }
  // ISSUE : likely fails when piece -> status track mapping is not identity
  midi::Piece step_piece = piece_subset(piece, s->start, s->end, tracks);

  if (param->verbose()) {
    std::cout << "GENERATION STEP INPUTS ::" << std::endl;
    show_vector(tracks);
    print_piece_summary(&step_piece);
    print_protobuf(&step_piece);
    print_status(&step_status);
  }

  //print_piece_summary(&step_piece); 

  midi::Piece gen_piece = generate_callback_inner(
    &step_piece, &step_status, param, debug, model, callbacks);

  // update debug.order here
  if (debug) {
    std::vector<bool> is_autoreg;
    for (int i=0; i<debug->orders.back().size(); i++) {
      is_autoreg.push_back( 
        step_status.tracks(debug->orders.back()[i]).autoregressive() );
      debug->orders.back()[i] = tracks[debug->orders.back()[i]];
    }
    debug->is_autoregressive.push_back( is_autoreg );
  }
  
  // NOTE : this inserts tracks that are just conditioned on as well
  //piece_insert_old(piece, &gen_piece, s->start, tracks);
  piece_insert(piece, &gen_piece, bar_mapping, param->verbose());
  
  //update_status_instruments(piece, status);
  override_piece_features(piece, status);

  /*
  if (param->verbose()) {
    std::cout << "GENERATION STEP COMPLETE ::" << std::endl;
    print_piece_summary(piece);
    print_status(status);
  }
  */

}

void sample_w_debug(midi::Piece *piece, midi::Status *raw_status, midi::SampleParam *param, Debugger *debug, CallbackManager *callbacks=NULL) {
  if ((!piece) || (!raw_status) || (!param)) {
    throw std::invalid_argument("Piece, Status or SampleParam is malformed");
  }

  // don't modify status in place
  midi::Status status_ob(*raw_status);
  midi::Status *status = &status_ob;

  // try to load model
  ModelMeta model;
  if (!param->internal_random_sample_mode()) {
    load_model(param->ckpt(), &model);
    if (model.meta.model_dim() != -1) {
      param->set_model_dim(model.meta.model_dim());
    }
  }
  else {
    if (!param->model_dim()) {
      throw std::invalid_argument
      ("MUST SET MODEL DIM MANUALLY IF USING RANDOM SAMPLE MODE");
    }
    model.meta.set_encoder(param->ckpt());
    model.meta.set_model_dim(param->model_dim());
  }

  // we run into problems if nb < model_dim

  std::unique_ptr<ENCODER> enc = getEncoder(
    getEncoderType(model.meta.encoder()));
  if (!enc.get()) {
    throw std::invalid_argument("INVALID ENCODER");
  }
  piece->set_resolution( enc->config->resolution );

  param->set_internal_skip_preprocess(true);
  param->set_batch_size(1);
  //param->set_polyphony_hard_limit(std::max(param->polyphony_hard_limit(),0));

  validate_inputs(piece, status, param);

  // before we start pad the piece if status references tracks
  // that do not exist yet
  pad_piece_with_status(piece, status, param->model_dim());

  // add time-signatures from piece into the status
  add_timesigs_to_status(piece, status);

  // fix duplicate density levels for OPZ tracks
  fix_duplicate_density_quantiles(status);

  // add features to piece when we are sampling auto-regressively
  // as these are perhaps not yet in the piece
  override_piece_features(piece, status);

  if (param->verbose()) {
    print_protobuf(piece);
    print_protobuf(status);
  }

  std::vector<std::vector<bool>> selection_mask = status_to_selection_mask(status);
  if (!any(selection_mask)) {
    return; // nothing to do
  }

  std::vector<bool> resample_mask = status_to_resample_mask(status);
  std::vector<bool> ignore_mask = status_to_ignore_mask(status);
  std::vector<STEP> steps = find_steps(
    selection_mask, resample_mask, ignore_mask, param);
  
  // if verbose summarize steps
  if (param->verbose()) {
    std::cout << "SELECTION MASK ..." << std::endl;
    show_matrix(selection_mask);
    std::cout << "RESAMPLE MASK ..." << std::endl;
    show_vector(resample_mask);
    std::cout << "IGNORE MASK ..." << std::endl;
    show_vector(ignore_mask);
    int step_num = 0;
    for (const auto step : steps) {
      std::cout << "=========================" << std::endl;
      std::cout << "STEP " << step_num << "/" << steps.size() << std::endl; 
      show_matrix(step.step);
      show_matrix(step.context);
      step_num++;
    }
  }

  if (steps.size() == 0) {
    // nothing to be done
    return;
  }

  // find the total number of bars to be generated
  int bar_count = 0;
  for (const auto step : steps) {
    bar_count += step.generated_bar_count();
  }
  if (callbacks) {
    callbacks->set_generated_bar_count(bar_count);
  }

  
  // get order and reverse order of tracks
  int nt = status->tracks_size();
  int nb = get_num_bars(piece);
  std::vector<int> order(nt,0);
  std::vector<int> reverse_order = arange(nt);
  for (int track_num=0; track_num<nt; track_num++) {
    midi::StatusTrack *st = status->mutable_tracks(track_num);
    order[track_num] = st->track_id();
    st->set_track_id(track_num); // now the mapping is the identity
  }
  std::sort(reverse_order.begin(), reverse_order.end(),
    [&order](size_t i, size_t j) {return order[i] < order[j];});
  
  reorder_tracks(piece, order);

  for (const auto step : steps) {
    sample_step(piece, status, param, debug, &model, &step, callbacks);
  }

  reorder_tracks(piece, reverse_order);
}

void sample(midi::Piece *piece, midi::Status *status, midi::SampleParam *param, CallbackManager *callbacks=NULL) {
  sample_w_debug(piece, status, param, NULL, callbacks);
}

std::string sample_multi_step_py(std::string &piece_json, std::string &status_json, std::string &param_json) {
  midi::Piece p;
  midi::Status s;
  midi::SampleParam h;
  google::protobuf::util::JsonStringToMessage(piece_json.c_str(), &p);
  google::protobuf::util::JsonStringToMessage(status_json.c_str(), &s);
  google::protobuf::util::JsonStringToMessage(param_json.c_str(), &h);
  sample(&p, &s, &h);
  std::string output;
  google::protobuf::util::MessageToJsonString(p, &output);
  return output;
}

}
