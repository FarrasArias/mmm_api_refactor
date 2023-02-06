
#include <vector>
#include <string>
#include <map>
#include <tuple>

#include "../midi_io.h" // only needed for MIDI input/output
#include "../sampling/sample_internal.h"
#include "../sampling/multi_step_sample.h"
#include "../sampling/util.h"
#include "../protobuf/util.h"
#include "../protobuf/midi.pb.h"
#include "../random.h"

#include "acutest.h"

#include <google/protobuf/util/json_util.h>

using namespace mmm;

// =========================================
// configuration
//
//
// before running unit tests, do the following
//
// 1. change the MODEL_FOLDER variable below
// 2. add or remove the models you want to test to the MODELS_TO_TEST vector
// 
// NOTE : CKPT_TO_TEST is used for other evaluation measures (not unit tests).
// NOTE : I believe the tests which generate MIDIs (el_test and opz_test) may require a folder to be created beforehand.

const std::string MODEL_FOLDER = "/users/jeff/CODE/MMM_TRAINING/models/";
std::vector<int> MODEL_DIMS_TO_TEST = {1,2,4,8};
const int num_trials = 10;
std::vector<std::string> MODELS_TO_TEST = {
  "el_yellow_ts_fixed.pt"
};

// this model is used for polyphony/note duration control error measurements
const std::string CKPT_TO_TEST = "el_yellow_ts_fixed.pt"; 


std::mt19937 e;

using BOOL_MATRIX = std::vector<std::vector<bool>>;

// these encoders will be tested with test_infill
// using random sampling from uniform token distribution
std::vector<std::string> ENCODERS_TO_TEST = {
  //TE_VELOCITY_DURATION_POLYPHONY_ENCODER",
  //"TE_VELOCITY_ENCODER",
  "EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER"
};

// standard tracks types
std::vector<midi::TRACK_TYPE> STANDARD_TRACK_TYPES = {
  midi::STANDARD_TRACK,
  midi::STANDARD_DRUM_TRACK
};

std::string vector_to_string(std::vector<int> values) {
  std::string output;
  int count = 0;
  for (const auto value : values) {
    output += std::to_string(value);
    count += 1;
    if (count != values.size()) {
      output += "_"; // delimiter
    }
  }
  return output;
}

void print_instruments(midi::Piece *p) {
  for (const auto track : p->tracks()) {
    std::cout << track.instrument() << " | ";
  }
  std::cout << std::endl;
}

void print_tokens(ENCODER *e, std::vector<int> &x) {
  std::cout << "VECTOR ::" << std::endl;
  for (const auto xx : x) {
    std::cout << e->rep->pretty(xx) << std::endl;
  }
}

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

template <typename T>
T mean(std::vector<T> &x) {
  return std::accumulate(x.begin(), x.end(), 0.0) / x.size();
}

// randomly populate protobuf using internal ranges
template <typename T>
void random_protobuf_inner(T *x, std::mt19937 *e, std::set<std::string> ignores) {

  const google::protobuf::Reflection* reflection = x->GetReflection();
  const google::protobuf::Descriptor* descriptor = x->GetDescriptor();
  for (int i=0; i<descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor *fd = descriptor->field(i);
    const google::protobuf::FieldOptions opt = fd->options();

    bool is_repeated = fd->is_repeated();
    int field_count = 1;
    if (is_repeated) {
      field_count = reflection->FieldSize(*x, fd);
    }

    if (ignores.find(fd->name()) != ignores.end()) {
      field_count = 0; // don't manipulate
    }

    for (int index=0; index<field_count; index++) {
    
      google::protobuf::FieldDescriptor::Type ft = fd->type();
      if (ft == google::protobuf::FieldDescriptor::Type::TYPE_FLOAT) {
        float minval = opt.GetExtension(midi::fminval);
        float maxval = opt.GetExtension(midi::fmaxval);
        float value = random_on_range(minval, maxval, e);
        if (is_repeated) {
          reflection->SetRepeatedFloat(x, fd, index, value);
        }
        else {
          reflection->SetFloat(x, fd, value);
        }
      }
      else if (ft == google::protobuf::FieldDescriptor::Type::TYPE_INT32) {
        int minval = opt.GetExtension(midi::minval);
        int maxval = opt.GetExtension(midi::maxval);
        int value = random_on_range(minval, maxval, e);
        if (is_repeated) {
          reflection->SetRepeatedInt32(x, fd, index, value);
        }
        else {
          reflection->SetInt32(x, fd, value);
        }
      }
      else if (ft == google::protobuf::FieldDescriptor::Type::TYPE_BOOL) {
        bool value = (bool)random_on_range(2,e);
        if (is_repeated) {
          reflection->SetRepeatedBool(x, fd, index, value);
        }
        else {
          reflection->SetBool(x, fd, value);
        }
      }
      else if (ft == google::protobuf::FieldDescriptor::Type::TYPE_ENUM) {
        const google::protobuf::EnumDescriptor *ed = fd->enum_type();
        int value = random_on_range(ed->value_count(), e);
        if (is_repeated) {
          reflection->SetRepeatedEnumValue(x, fd, index, value);
        }
        else {
          reflection->SetEnumValue(x, fd, value);
        }
      }
      else if (ft == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE) {
        if (is_repeated) {
          random_protobuf_inner(
            reflection->MutableRepeatedMessage(x, fd, index), e, ignores);
        }
        else {
          random_protobuf_inner(
            reflection->MutableMessage(x, fd), e, ignores);
        }
      }
    }
  }
}

template <typename T>
void random_protobuf(T *x, std::mt19937 *e) {
  random_protobuf_inner(x, e, {});
}


// split a token sequence into tracks
std::vector<std::vector<int>> split_tokens_into_tracks(std::vector<int> &tokens, REPRESENTATION *rep) {
  std::vector<std::vector<int>> tracks;
  int last_start = -1;
  for (int i=0; i<tokens.size(); i++) {
    if (rep->is_token_type(tokens[i], TRACK)) {
      if (last_start != -1) {
        tracks.push_back({});
        for (int j=last_start; j<i; j++) {
          tracks.back().push_back( tokens[j] );
        }
      }
      last_start = i;
    }
  }
  tracks.push_back({});
  for (int j=last_start; j<tokens.size(); j++) {
    tracks.back().push_back( tokens[j] );
  }
  return tracks;
}

inline bool file_exists(const std::string& name) {
  std::ifstream f(name.c_str());
  return f.good();
}

std::string encoder_string_from_ckpt(const std::string &ckpt) {
  ModelMeta m;
  if (file_exists(ckpt)) {
    load_model(ckpt, &m);
  }
  else {
    load_model(MODEL_FOLDER + ckpt, &m);
  }
  return m.meta.encoder();
}

std::unique_ptr<ENCODER> encoder_from_ckpt(const std::string &ckpt) {
  std::string estr = ckpt;
  if (ends_with(ckpt, std::string(".pt"))) {
    estr = encoder_string_from_ckpt(ckpt);
  }
  return getEncoder(getEncoderType(estr));
} 

std::unique_ptr<ENCODER> encoder_from_ckpt(midi::SampleParam *param) {
  return encoder_from_ckpt(param->ckpt());
}

void add_checkpoint_to_param(const char *ckpt_str, midi::SampleParam *param) {
  std::string ckpt(ckpt_str);
  bool random_sample_mode = !(ends_with(ckpt, std::string(".pt")));
  if (!random_sample_mode) {
    ckpt = MODEL_FOLDER + ckpt;
  }
  param->set_ckpt(ckpt);
  param->set_internal_random_sample_mode(random_sample_mode);
}

// this is a helper function that converts a midi::Piece into a midi::Status
void status_from_piece(midi::Status *status, midi::Piece *piece, std::mt19937 *e=NULL) {
  status->Clear();
  int track_num = 0;
  for (const auto track : piece->tracks()) {
    midi::StatusTrack *strack = status->add_tracks();
    if (e) {
      random_protobuf(strack, e);
    }
    strack->set_track_id( track_num );
    strack->set_track_type( track.track_type() );
    strack->set_instrument( 
      mmm::gm_inst_to_string(track.track_type(),track.instrument()));
    strack->set_autoregressive(false);
    strack->set_ignore(false);
    for (int i=0; i<track.bars_size(); i++) {
      strack->add_selected_bars( false );
    }
    track_num++;
  }
}

bool contains_token(int token, std::vector<int> &tokens) {
  return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

int find_token(ENCODER *enc, TOKEN_TYPE tt, std::vector<int> &tokens) {
  for (const auto token : tokens) {
    if (enc->rep->is_token_type(token,tt)) {
      return token;
    }
  }
  return -1;
}

bool is_opz_encoder(std::string estr) {
  return getEncoder(getEncoderType(estr))->config->te;
}

std::vector<std::tuple<int,int>> random_time_sigs(int num_bars, std::string estr, std::mt19937 *e) {
  auto ts_domain =  encoder_from_ckpt(estr)->rep->get_time_signature_domain();
  std::vector<std::tuple<int,int>> timesigs;
  for (int i=0; i<num_bars; i++) {
    timesigs.push_back( random_element(ts_domain, e) );
  }
  return timesigs;
}

std::vector<std::tuple<int,int>> one_random_time_sig(int num_bars, std::string estr, bool only_four_four, std::mt19937 *e) {
  auto ts_domain =  encoder_from_ckpt(estr)->rep->get_time_signature_domain();
  std::vector<std::tuple<int,int>> timesigs;
  std::tuple<int,int> ts = random_element(ts_domain, e);
  if (only_four_four) {
    ts = std::make_tuple(4,4);
  }
  for (int i=0; i<num_bars; i++) {
    timesigs.push_back( ts );
  }
  return timesigs;
}

// helper
class generation_inputs {
public:
  midi::Piece piece;
  midi::Status status;
  midi::SampleParam param;
  Debugger debug;
  int num_tracks;
  int num_bars;
  int model_dim;
  int ts_numerator;
  int ts_denominator;
};

generation_inputs build_generation_inputs(std::string estr, bool opz, int num_tracks, int num_bars, int model_dim, bool only_four_four=false) {
  generation_inputs g;
  g.model_dim = model_dim;
  g.num_tracks = num_tracks;
  g.num_bars = num_bars;

  auto timesigs = one_random_time_sig(g.num_bars, estr, only_four_four, &e);

  g.ts_numerator = std::get<0>(timesigs[0]);
  g.ts_denominator = std::get<1>(timesigs[0]);

  g.piece = random_piece(g.num_tracks, g.num_bars, opz, timesigs, &e);
  status_from_piece(&g.status, &g.piece, &e);
  random_protobuf(&g.param, &e);
  add_checkpoint_to_param(estr.c_str(), &g.param);

  g.param.set_verbose(false);
  g.param.set_internal_skip_preprocess(false);
  g.param.set_internal_disable_masking(false);
  g.param.set_max_steps(0);
  g.param.set_model_dim(g.model_dim);

  return g;
}

generation_inputs random_generation_inputs_inner(std::string estr, bool opz, int max_tracks, int max_bars, bool num_bars_model_dim=false, bool only_four_four=false) {

  // model dims should be inferred from the rep
  std::vector<int> model_dims = encoder_from_ckpt(estr)->rep->get_num_bars_domain();
  if (model_dims.size() == 0) {
    for (auto value : MODEL_DIMS_TO_TEST) {
      model_dims.push_back( value );
    }
  }

  int model_dim = random_element(model_dims, &e);
  int num_tracks = max_tracks; //random_on_range(1, max_tracks, &e);
  int num_bars = random_on_range(model_dim, max_bars, &e);
  if (num_bars_model_dim) {
    num_bars = model_dim;
  }

  generation_inputs g = build_generation_inputs(
    estr, opz, num_tracks, num_bars, model_dim, only_four_four);

  return g;
}

generation_inputs random_generation_inputs(int max_tracks=4, int max_bars=16, bool use_models=false) {
  
  std::string estr = random_element(ENCODERS_TO_TEST, &e);
  bool opz = is_opz_encoder(estr);
  if (use_models) {
    estr = random_element(MODELS_TO_TEST, &e);
    opz = is_opz_encoder(encoder_string_from_ckpt(estr));
  }
  return random_generation_inputs_inner(estr, opz, max_tracks, max_bars);
}

// =====================================================================

void set_random_seed(void) {  
  e.seed(time(NULL));
}

// =====================================================================

// this function is used to score the polyphony and duration control
// and density

void score_duration(midi::Piece *p, midi::Status *s, std::vector<double> &min_errs, std::vector<double> &max_errs) {
  update_av_polyphony_and_note_duration(p);

  for (int track_num=0; track_num<s->tracks_size(); track_num++) {
    midi::StatusTrack st = s->tracks(track_num); 
    midi::TrackFeatures tf = p->tracks(track_num).internal_features(0);
    int min_value = (int)tf.min_note_duration_q(); // first level starts at zero
    int min_target = (int)st.min_note_duration_q() - 1;

    int max_value = (int)tf.max_note_duration_q(); // first level starts at zero
    int max_target = (int)st.max_note_duration_q() - 1;

    min_errs.push_back( abs(min_target - min_value) );    
    max_errs.push_back( abs(max_target - max_value) );
  }
}

void score_polyphony(midi::Piece *p, midi::Status *s, std::vector<double> &min_errs, std::vector<double> &max_errs) {
  update_av_polyphony_and_note_duration(p);

  for (int track_num=0; track_num<s->tracks_size(); track_num++) {
    midi::StatusTrack st = s->tracks(track_num); 
    midi::TrackFeatures tf = p->tracks(track_num).internal_features(0);
    int min_value = (int)tf.min_polyphony_q(); // first level starts at zero
    int min_target = (int)st.min_polyphony_q() - 1;

    int max_value = (int)tf.max_polyphony_q(); // first level starts at zero
    int max_target = (int)st.max_polyphony_q() - 1;

    min_errs.push_back( abs(min_target - min_value) );    
    max_errs.push_back( abs(max_target - max_value) );
  }
}

void score_density(midi::Piece *p, midi::Status *s, std::vector<double> &errs) {
  update_note_density(p);

  for (int track_num=0; track_num<s->tracks_size(); track_num++) {
    midi::StatusTrack st = s->tracks(track_num); 
    midi::TrackFeatures tf = p->tracks(track_num).internal_features(0);
    int value = (int)tf.note_density_v2(); // first level starts at zero
    int target = (int)st.density() - 1;
    errs.push_back( abs(value - target) );    
  }
}



// TODO

// with bar-infilling empty bars seem to happen too easily
// we can prevent this by not allowing that transition in graph
// likely related to accidentally ommitting VELOCITY_LEVEL in infilling graph

// for EL model, the drum instruments have same subset mapping as instruments

// the poly controls do not appear to carry over for infilling on multi-step


// there might be an issue with density mapping
// as some values are never used because of repeats [1,2,2,2,2,2,4,6]
// for example. This might have an impact when requesting specific levels
// oh (problem below) ...

// in sampling we need to map density to correct values for
// OPZ in cases where there are duplicate values in quantile distribution

// add some example protobuf to the doc

// write generation scripts for TE model
// write unit test that produces outputs

// what happens when there are extra tracks in piece not refernce by status

// any inputs should either succeed or return
// invalid error

// testing model paths

void test_paths() {

  // to test this uncomment last line and run
  // ./build/mmm_api test_paths

  std::string abs_path = "/users/jeff/CODE/MMM_TRAINING/models/el_yellow_ts_fixed.pt";
  std::string rel_path = "../MMM_TRAINING/models/el_yellow_ts_fixed.pt";

  ModelMeta m;
  load_model(abs_path, &m);
  //load_model(rel_path, &m);
}

// testing callback feature / a simple test

class MyCallback : public CallbackManager {
public:
  void on_bar_end(float progress) {
    std::cout << "BAR HAS ENDED :: " << progress << std::endl;
  }
};

void test_callbacks() {

  set_random_seed();
  MyCallback callbacks;
  generation_inputs g = random_generation_inputs(4,8);
  BOOL_MATRIX m = random_boolean_matrix(g.num_tracks, g.num_bars, &e);
  set_selected_bars(&g.status, m);
  g.param.set_verbose(false);
  print_protobuf(&g.param);
  mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug, &callbacks);

}


// time_sig mismatch should throw invalid argument error
void test_time_sig_mismatch() {
  set_random_seed();
  std::vector<std::tuple<int,int>> timesigs = {{4,4},{4,4},{4,4},{4,4}};
  midi::Piece p = random_piece(4, 4, false, timesigs, &e);
  p.mutable_tracks(random_on_range(4,&e))->mutable_bars(random_on_range(4,&e))->set_ts_numerator(3); // change one bar
  try {
    validate_piece(&p);
    throw std::runtime_error("DID NOT CATCH ERROR!");
  }
  catch (const std::invalid_argument& error) {

  }
}

// verify that only one step happens when track_nums == tracks_per_step etc.
void test_single_step() {

  std::set<int> valid_num_bars = {4,8};

  set_random_seed();
  for (int i=0; i<num_trials; i++) {
    generation_inputs g = random_generation_inputs(4,8);
    while (valid_num_bars.find(g.num_bars) == valid_num_bars.end()) {
      g = random_generation_inputs(4,8);
    }

    BOOL_MATRIX m = ones_boolean_matrix(g.num_tracks, g.num_bars);  
    set_selected_bars(&g.status, m);

    g.param.set_tracks_per_step(g.num_tracks);
    g.param.set_bars_per_step(g.num_bars);
    for (const auto model_dim : MODEL_DIMS_TO_TEST) {
      if (model_dim >= g.num_bars) {
        g.param.set_model_dim(model_dim);
        break;
      }
    }

    print_protobuf(&g.param);    
    mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);
    TEST_ASSERT(g.debug.tokens.size() == 1);
  }

}

// test generation with random status
void test_random_status() {

  set_random_seed();
  for (int i=0; i<num_trials; i++) {

    generation_inputs g = random_generation_inputs();
    random_protobuf_inner(&g.status, &e, {"track_id","track_type"});
    g.param.set_internal_skip_preprocess(false);

    // two OK possibilities 
    // 1) generation succeeds
    // 2) validation of inputs fails throwing invalid_argument

    try {
      mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);
    }
    catch (const std::invalid_argument& error) { 
      std::cout << "SUCESSFULLY CAUGHT MALFORMED INPUT :: " << error.what() << std::endl;
    }
  }
}

void test_infill_w_model(void) {

  set_random_seed();
  for (int i=0; i<num_trials; i++) {

    generation_inputs g = random_generation_inputs(4,16,true);
    // print sample param so we can see what is going on
    g.param.set_temperature(1);
    print_protobuf(&g.param);
    
    BOOL_MATRIX m = random_boolean_matrix(g.num_tracks, g.num_bars, &e);
    set_selected_bars(&g.status, m);

    midi::Piece orig(g.piece);
    mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

    bool pretrain_map = encoder_from_ckpt(&g.param)->config->do_pretrain_map;
    TEST_CHECK( tracks_are_identical(&g.piece, &orig, pretrain_map) );
    TEST_CHECK( bars_are_identical(&g.piece, &orig, m) );
  }
}

void test_infill(void) {

  // Here we run infill using random sampling rather than
  // getting probabilites from a model. This is a more 
  // robust test of sampling constraints

  set_random_seed();
  for (int i=0; i<num_trials; i++) {

    generation_inputs g = random_generation_inputs();

    // print sample param so we can see what is going on
    g.param.set_verbose(false);
    set_polyphony_hard_limit(&g.status, 4);

    BOOL_MATRIX m = random_boolean_matrix(g.num_tracks, g.num_bars, &e);
    set_selected_bars(&g.status, m);

    print_protobuf(&g.status);
    print_protobuf(&g.param);

    midi::Piece orig(g.piece);
    mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

    bool pretrain_map = encoder_from_ckpt(&g.param)->config->do_pretrain_map;
    TEST_CHECK( tracks_are_identical(&g.piece, &orig, pretrain_map) );
    TEST_CHECK( bars_are_identical(&g.piece, &orig, m) );
  }
}

void test_ignore(void) {
  
  set_random_seed();
  for (int i=0; i<num_trials; i++) {
    std::string estr = random_element(MODELS_TO_TEST, &e);
    generation_inputs g = random_generation_inputs_inner(
      estr, false, 4, 8, true, false); // last bool sets only 4/4
    
    set_resample_tracks(&g.status, arange(g.num_tracks));
    g.param.set_temperature(1.);
    g.param.set_bars_per_step(g.model_dim);
    g.param.set_verbose(false);

    for (int i=0; i<g.num_tracks; i++) {
      midi::StatusTrack *st = g.status.mutable_tracks(i);
      st->set_instrument(midi::any);
      st->set_min_polyphony_q(midi::POLYPHONY_ANY);
      st->set_max_polyphony_q(midi::POLYPHONY_ANY);
      st->set_min_note_duration_q(midi::DURATION_ANY);
      st->set_max_note_duration_q(midi::DURATION_ANY);
      st->set_density(midi::DENSITY_ANY);
      if (random_on_unit(&e) < .5) {
        st->set_ignore(true);
        set_resample_tracks(&g.status, {i}, false);
      }
    }

    //print_protobuf(&g.param);
    //print_protobuf(&g.status);

    BOOL_MATRIX m = status_to_selection_mask(&g.status);

    midi::Piece orig(g.piece);
    mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

    show_matrix(m);

    // check
    TEST_CHECK( bars_are_identical(&g.piece, &orig, m) );
    TEST_CHECK( tracks_are_different(&g.piece, &orig, m) );
  }
}



void test_autoreg(void) {

  set_random_seed();

  for (int i=0; i<num_trials; i++) {

    generation_inputs g = random_generation_inputs();

    bool pretrain_map = encoder_from_ckpt(&g.param)->config->do_pretrain_map;

    g.param.set_tracks_per_step(1);

    std::vector<int> track_nums = arange(g.num_tracks);
    set_resample_tracks(&g.status, random_subset(track_nums, &e));
    BOOL_MATRIX m = status_to_selection_mask(&g.status);

    // print sample param so we can see what is going on
    print_protobuf(&g.param);
    print_protobuf(&g.status);

    g.param.set_verbose(false);

    midi::Piece orig(g.piece);
    mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

    // make sure bars / tracks don't change
    //print_instruments(&g.piece);
    //print_instruments(&orig);
    //TEST_CHECK( tracks_are_identical(&g.piece, &orig) );
    //TEST_CHECK( bars_are_identical(&g.piece, &orig, m) );

    // each generated track should be different from all conditioned tracks
    // most of the time
    /*
    for (int i=0; i<g.num_tracks; i++) {
      for (int j=0; j<g.num_tracks; j++) {
        if ((i!=j) &&  (g.status.tracks(i).autoregressive())) {
          TEST_CHECK( tracks_are_different(&g.piece, &g.piece, i, j) );
        }
      }
    }
    */

    // make sure that status selections
    // have been applied to the token sequence
    // split sequence into tokens per track
    // if we store track order in debug
    // then we can figure out which track corresponds to which
    //ENCODER *enc = encoder_from_ckpt(&g.param);
    std::unique_ptr<ENCODER> enc = encoder_from_ckpt(&g.param);
    REPRESENTATION *rep = enc->rep;

    for (int step=0; step<g.debug.tokens.size(); step++) {
      if (g.debug.modes[step] == TRACK_MODEL) {

        std::vector<std::vector<int>> tracks = split_tokens_into_tracks(
          g.debug.tokens[step], rep);

        for (int track_num=0; track_num<tracks.size(); track_num++) {
          int index = g.debug.orders[step][track_num];
          midi::StatusTrack st = g.status.tracks(index);

          //bool is_autoregressive = st.autoregressive();
          bool is_autoregressive = g.debug.is_autoregressive[step][track_num];
          bool is_drum = is_drum_track(st.track_type());
          bool can_polyphony = ((!enc->config->mark_drum_density) | (!is_drum));

          //std::cout << "TESTING :: " << step << " TRACK_NUM :: " << track_num << std::endl;
          //print_tokens(enc.get(), tracks[track_num]); 
          //std::cout << "ORDER :: " << g.debug.orders[step] << std::endl;

          if (is_autoregressive) {

            if (can_polyphony) {

              // check polyphony enforcement
              if (rep->has_token_type(MIN_POLYPHONY)) {
                int min_poly = (int)st.min_polyphony_q() - 1;
                if (min_poly>=0) {
                  TEST_ASSERT(contains_token(
                    rep->encode(MIN_POLYPHONY,min_poly), tracks[track_num]));
                }
              }

              if (rep->has_token_type(MAX_POLYPHONY)) {
                int max_poly = (int)st.max_polyphony_q() - 1;
                if (max_poly>=0) {
                  TEST_ASSERT(contains_token(
                    rep->encode(MAX_POLYPHONY,max_poly), tracks[track_num]));
                }
              }

              // check note duration enforcement
              if (rep->has_token_type(MIN_NOTE_DURATION)) {
                int min_dur = (int)st.min_note_duration_q() - 1;
                if (min_dur>=0) {
                  TEST_ASSERT(contains_token(
                    rep->encode(MIN_NOTE_DURATION,min_dur), tracks[track_num]));
                }
              }

              if (rep->has_token_type(MAX_NOTE_DURATION)) {
                int max_dur = (int)st.max_note_duration_q() - 1;
                if (max_dur>=0) {
                  TEST_ASSERT(contains_token(
                    rep->encode(MAX_NOTE_DURATION,max_dur), tracks[track_num]));
                }
              }
            }
            else {

              // we are using density encoding
              if (rep->has_token_type(DENSITY_LEVEL)) {
                int density = (int)st.density() - 1;
                if (density>=0) {
                  TEST_ASSERT(contains_token(
                    rep->encode(DENSITY_LEVEL,density), tracks[track_num]));
                }
              }
            }
          }
          
          // check instrument enforcement
          if (rep->has_token_type(INSTRUMENT)) {
            std::vector<int> insts = GM_MOD[
              static_cast<midi::GM_TYPE>(st.instrument())];
            // to allow for EL method encode --> decode each instrument token
            if ((pretrain_map) && (!is_drum)) {
              for (int ii=0; ii<insts.size(); ii++) {
                insts[ii] = PRETRAIN_GROUPING_V2.find(insts[ii])->second;
              }
            }
            /*
            // OLD WAY OF PRETRAIN GROUPING
            for (int ii=0; ii<insts.size(); ii++) {
              insts[ii] = enc->rep->decode(
                enc->rep->encode(INSTRUMENT, insts[ii]));
            }
            */
            int t = enc->rep->decode(find_token(enc.get(), INSTRUMENT, tracks[track_num]));
            

            std::cout << "EXPECTED :: " << vector_to_string(insts) << std::endl;
            std::cout << "GENERATED :: " << t << std::endl;

            TEST_ASSERT( t >= 0 );
            TEST_ASSERT( std::find(
              insts.begin(), insts.end(), t) != insts.end() );
          }

        }
      }
    }
  }
}


// ==============================================
// ==============================================
// ==============================================
// ==============================================
// NOT REALLY UNIT TESTS

void test_polyphony(void) {

  set_random_seed();

  std::vector<midi::TRACK_TYPE> OPZ_POLY_TRACKS = {
    midi::OPZ_CHORD_TRACK
  };

  std::string estr = CKPT_TO_TEST;
  bool use_opz = encoder_from_ckpt(estr)->config->te;

  for (int minval=1; minval<5; minval++) {
    for (int maxval=minval; maxval<5; maxval++) {

      std::vector<double> min_errs;
      std::vector<double> max_errs;

      for (int i=0; i<num_trials; i++) {
        
        generation_inputs g = build_generation_inputs(estr, true, 1, 4, 4);

        set_resample_tracks(&g.status, arange(g.num_tracks));
        g.param.set_temperature(1.);
        g.param.set_bars_per_step(g.model_dim);
        set_polyphony_hard_limit(&g.status, 6); // allow model to exceed

        for (int i=0; i<g.num_tracks; i++) {
          midi::StatusTrack *st = g.status.mutable_tracks(i);
          st->set_instrument(midi::any);
          st->set_min_polyphony_q(static_cast<midi::PolyphonyLevel>(minval));
          st->set_max_polyphony_q(static_cast<midi::PolyphonyLevel>(maxval));

          st->set_min_note_duration_q(midi::DURATION_ANY);
          st->set_max_note_duration_q(midi::DURATION_ANY);
          st->set_density(midi::DENSITY_ANY);
          
          if (use_opz) {
            st->set_track_type( random_element(OPZ_POLY_TRACKS, &e) );
          }
          else {
            st->set_track_type( midi::STANDARD_TRACK ); // can't be drums
          }

        }

        //print_protobuf(&g.param);

        midi::Piece orig(g.piece);
        mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

        score_polyphony(&g.piece, &g.status, min_errs, max_errs);

      }

      // print error
      std::cout << "AVERAGE ERROR (" << minval << ", " << maxval << ")" << std::endl;
      std::cout << mean(min_errs) << std::endl;
      std::cout << mean(max_errs) << std::endl;

    }
  }
}

void test_duration(void) {

  set_random_seed();

  std::vector<midi::TRACK_TYPE> OPZ_DUR_TRACKS = {
    midi::OPZ_BASS_TRACK,
    midi::OPZ_LEAD_TRACK,
    midi::OPZ_ARP_TRACK,
    midi::OPZ_CHORD_TRACK
  };

  std::string estr = CKPT_TO_TEST;
  bool use_opz = encoder_from_ckpt(estr)->config->te;

  for (int minval=1; minval<5; minval++) {
    for (int maxval=minval; maxval<5; maxval++) {

      std::vector<double> min_errs;
      std::vector<double> max_errs;

      for (int i=0; i<num_trials; i++) {
        
        generation_inputs g = build_generation_inputs(estr, true, 1, 4, 4);

        set_resample_tracks(&g.status, arange(g.num_tracks));
        g.param.set_temperature(1.);
        g.param.set_bars_per_step(g.model_dim);
        set_polyphony_hard_limit(&g.status, 4);

        for (int i=0; i<g.num_tracks; i++) {
          midi::StatusTrack *st = g.status.mutable_tracks(i);
          st->set_instrument(midi::any);
          st->set_min_note_duration_q(
            static_cast<midi::NoteDurationLevel>(minval));
          st->set_max_note_duration_q(
            static_cast<midi::NoteDurationLevel>(maxval));

          st->set_min_polyphony_q(midi::POLYPHONY_ANY);
          st->set_max_polyphony_q(midi::POLYPHONY_ANY);
          st->set_density(midi::DENSITY_ANY);

          if (use_opz) {
            st->set_track_type( random_element(OPZ_DUR_TRACKS, &e) );
          }
          else {
            st->set_track_type( midi::STANDARD_TRACK ); // can't be drums
          }
          
        }

        //print_protobuf(&g.param);

        midi::Piece orig(g.piece);
        mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

        score_duration(&g.piece, &g.status, min_errs, max_errs);

      }

      // print error
      std::cout << "AVERAGE ERROR (" << minval << ", " << maxval << ")" << std::endl;
      std::cout << mean(min_errs) << std::endl;
      std::cout << mean(max_errs) << std::endl;

    }
  }
}

void test_density(void) {

  set_random_seed();

  std::vector<midi::TRACK_TYPE> OPZ_DENSITY_TRACKS = {
    midi::OPZ_KICK_TRACK,
    midi::OPZ_SNARE_TRACK,
    midi::OPZ_HIHAT_TRACK,
    midi::OPZ_SAMPLE_TRACK
  };

  std::string estr = CKPT_TO_TEST;
  bool use_opz = encoder_from_ckpt(estr)->config->te;

  for (int val=1; val<10; val++) {

    std::vector<double> errs;

    for (int i=0; i<num_trials; i++) {
      
      generation_inputs g = build_generation_inputs(estr, true, 1, 4, 4);

      set_resample_tracks(&g.status, arange(g.num_tracks));
      g.param.set_temperature(1.);
      g.param.set_bars_per_step(g.model_dim);
      set_polyphony_hard_limit(&g.status, 4);

      for (int i=0; i<g.num_tracks; i++) {
        midi::StatusTrack *st = g.status.mutable_tracks(i);
        st->set_instrument(midi::any);        
        st->set_density(static_cast<midi::DensityLevel>(val));

        st->set_min_note_duration_q(midi::DURATION_ANY);
        st->set_max_note_duration_q(midi::DURATION_ANY);
        st->set_min_polyphony_q(midi::POLYPHONY_ANY);
        st->set_max_polyphony_q(midi::POLYPHONY_ANY);

        if (use_opz) {
          st->set_track_type( random_element(OPZ_DENSITY_TRACKS, &e) );
        }
        else {
          st->set_track_type( midi::STANDARD_DRUM_TRACK ); // must be drums
        }
      }

      //print_protobuf(&g.param);

      midi::Piece orig(g.piece);
      mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

      score_density(&g.piece, &g.status, errs);

    }

    // print error
    std::cout << "AVERAGE ERROR (" << val << ")" << std::endl;
    std::cout << mean(errs) << std::endl;
  }
}


void el_test(void) {
  
  set_random_seed();
  std::string estr = CKPT_TO_TEST;

  for (int i=0; i<num_trials; i++) {
    generation_inputs g = random_generation_inputs_inner(
      estr, false, 4, 8, true, false); // last bool sets only 4/4
    
    set_resample_tracks(&g.status, arange(g.num_tracks));
    g.param.set_temperature(1.);
    g.param.set_bars_per_step(g.model_dim);
    g.param.set_verbose(true);

    for (int i=0; i<g.num_tracks; i++) {
      midi::StatusTrack *st = g.status.mutable_tracks(i);
      st->set_instrument(midi::any);
      st->set_min_polyphony_q(midi::POLYPHONY_ANY);
      st->set_max_polyphony_q(midi::POLYPHONY_ANY);
      st->set_min_note_duration_q(midi::DURATION_ANY);
      st->set_max_note_duration_q(midi::DURATION_ANY);
      st->set_density(midi::DENSITY_ANY);
    }

    print_protobuf(&g.param);
    print_protobuf(&g.status);

    midi::Piece orig(g.piece);
    mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

    // make sure bars / tracks don't change
    tracks_are_identical(&g.piece, &orig);

    std::unique_ptr<ENCODER> enc = encoder_from_ckpt(&g.param);
    print_tokens(enc.get(), g.debug.tokens[0]);

    // write the output to a file
    std::string save_path = "test_output_el/" + vector_to_string({g.model_dim, g.num_tracks, g.num_bars, g.ts_numerator, g.ts_denominator, i}) + ".mid";
    
    write_midi(&g.piece, save_path);
  }

}

void opz_test(void) {

  set_random_seed();
  std::string estr = CKPT_TO_TEST;
  bool use_opz = encoder_from_ckpt(estr)->config->te;
  
  if (use_opz) {

    for (int i=0; i<num_trials; i++) {

      generation_inputs g = random_generation_inputs_inner(
        estr, true, 4, 8, true);

      set_resample_tracks(&g.status, arange(g.num_tracks));
      g.param.set_temperature(1.);
      g.param.set_bars_per_step(g.model_dim);
      set_polyphony_hard_limit(&g.status, 4);

      //g.param.set_internal_random_sample_mode(true);
      g.param.set_verbose(true);

      // set the instrument to any
      // and polyphony etc
      for (int i=0; i<g.num_tracks; i++) {
        midi::StatusTrack *st = g.status.mutable_tracks(i);
        st->set_instrument(midi::any);
        st->set_min_polyphony_q(midi::POLYPHONY_ANY);
        st->set_max_polyphony_q(midi::POLYPHONY_ANY);
        st->set_min_note_duration_q(midi::DURATION_ANY);
        st->set_max_note_duration_q(midi::DURATION_ANY);
        st->set_density(midi::DENSITY_ANY);
      }

      print_protobuf(&g.param);
      print_protobuf(&g.status);

      midi::Piece orig(g.piece);
      mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

      // make sure bars / tracks don't change
      //TEST_CHECK( tracks_are_identical(&g.piece, &orig) );

      std::unique_ptr<ENCODER> enc = encoder_from_ckpt(&g.param);
      print_tokens(enc.get(), g.debug.tokens[0]);

      // write the output to a file
      std::string save_path = "test_output/" + vector_to_string({g.model_dim, g.num_tracks, g.num_bars, g.ts_numerator, g.ts_denominator, i}) + ".mid";
      
      write_midi(&g.piece, save_path);

    }
  }
  else {
    std::cout << "CAN'T TEST THIS MODEL SKIPPING" << std::endl;
  }
}

TEST_LIST = {
  { "test_paths", test_paths},
  { "test_callbacks", test_callbacks},
  { "test_time_sig_mismatch", test_time_sig_mismatch },
  { "test_single_step", test_single_step },
  { "test_infill", test_infill },
  { "test_infill_w_model", test_infill_w_model },
  { "test_autoreg", test_autoreg },
  { "test_random_status", test_random_status },
  { "test_ignore", test_ignore },

  // the following aren't really unit test just useful for general evaluation 
  // of the models
  { "test_polyphony", test_polyphony }, // measure accuracy of polyphony control
  { "test_duration", test_duration }, // measure accuracy of duration control
  { "test_density", test_density }, // measure accuracy of density control
  { "opz_test", opz_test }, // generate some MIDIs
  { "el_test", el_test }, // generate some MIDIs

  { NULL, NULL }     /* zeroed record marking the end of the list */
};
