
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

const std::string MODEL_FOLDER = "/users/jeff/CODE/MMM_TRAINING/models/";

std::map<std::string,std::string> CKPT_ENCODER_MAP {
  {"paper_bar_4.pt", "TRACK_BAR_FILL_DENSITY_ENCODER"},
  {"trash.pt", "TRACK_BAR_FILL_DENSITY_ENCODER"}
};

mmm::ENCODER* get_encoder(std::string &ckpt) {
  std::string estr = CKPT_ENCODER_MAP.find(ckpt)->second;
  return mmm::getEncoder(mmm::getEncoderType(estr));
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

std::string encoder_string_from_ckpt(const std::string &ckpt) {
  ModelMeta m;
  load_model(ckpt, &m);
  return m.meta.encoder();
}

ENCODER *encoder_from_ckpt(midi::SampleParam *param) {
  std::string estr = param->ckpt();
  if (!param->internal_random_sample_mode()) {
    estr = encoder_string_from_ckpt(param->ckpt());
  }
  return getEncoder(getEncoderType(estr));
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

/*
midi::SampleParam build_sample_param_for_testing(const char *ckpt_str) {
  midi::SampleParam param;
  std::string ckpt(ckpt_str);
  bool random_sample_mode = !(ends_with(ckpt, std::string(".pt")));
  if (!random_sample_mode) {
    ckpt = MODEL_FOLDER + ckpt;
  }
  param.set_tracks_per_step(1);
  param.set_bars_per_step(2);
  param.set_model_dim(4);
  param.set_shuffle(true);
  param.set_percentage(100);
  param.set_temperature(1.);
  param.set_batch_size(1);
  param.set_verbose(false);
  param.set_ckpt(ckpt);
  param.set_polyphony_hard_limit(5);
  param.set_internal_skip_preprocess(false);
  param.set_internal_random_sample_mode(random_sample_mode);
  return param;
}
*/

/*
void add_new_track(midi::Status *status, int track_type, std::string instrument, int density, std::vector<bool> selected_bars, bool resample=false) {
  midi::StatusTrack *track = status->add_tracks();
  track->set_track_id((int)status->tracks_size() - 1);
  track->set_track_type((midi::TRACK_TYPE)track_type);
  track->set_instrument(instrument);
  track->set_density(density);
  track->set_resample(resample);
  for (const auto x : selected_bars) {
    track->add_selected_bars( x );
  }
}
*/

void write_piece(midi::Piece x, const char *path) {
  std::string json_string;
  google::protobuf::util::JsonPrintOptions opt;
  opt.add_whitespace = true;
  google::protobuf::util::MessageToJsonString(x, &json_string, opt);
  std::ofstream out(path);
  out << json_string;
  out.close();
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
    strack->set_resample(false);
    strack->set_ignore(false);
    for (int i=0; i<track.bars_size(); i++) {
      strack->add_selected_bars( false );
    }
    track_num++;
  }
}

/*
void run_generate(midi::Piece *p, midi::Status *s, mmm::Debugger *debug, const char *ckpt_str) {
  std::string ckpt = ckpt_str;
  midi::SampleParam param = build_sample_param_for_tes(ckpt);
  param.set_random_sample_mode(true);
  mmm::sample_w_debug(p, s, &param, debug);
}
*/


int count_token_type(mmm::ENCODER *enc, mmm::TOKEN_TYPE tt, std::vector<int> &tokens) {
  int count = 0;
  for (const auto token : tokens) {
    if (enc->rep->is_token_type(token, tt)) {
      count++;
    }
  }
  return count;
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

std::mt19937 e;

using BOOL_MATRIX = std::vector<std::vector<bool>>;

std::vector<std::string> ENCODERS_TO_TEST = {
  "POLYPHONY_ENCODER",
  "POLYPHONY_DURATION_ENCODER",
  "DURATION_ENCODER",
  "NEW_DURATION_ENCODER",
  "TE_ENCODER",
  "NEW_VELOCITY_ENCODER",
  "ABSOLUTE_ENCODER",
  "MULTI_LENGTH_ENCODER"
};

std::vector<std::string> MODELS_TO_TEST = {
  "paper_bar_4_WMETA.pt"
};

std::vector<int> MODEL_DIMS_TO_TEST = {1,2,4,8};

const int num_trials = 10;

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
};

generation_inputs random_generation_inputs(int max_tracks=4, int max_bars=16, bool use_models=false) {
  generation_inputs g;

  std::string estr = random_element(ENCODERS_TO_TEST, &e);
  bool opz = is_opz_encoder(estr);
  if (use_models) {
    estr = random_element(MODELS_TO_TEST, &e);
    opz = is_opz_encoder(encoder_string_from_ckpt(estr));
  }
  
  g.model_dim = random_element(MODEL_DIMS_TO_TEST, &e);
  g.num_tracks = random_on_range(1, max_tracks, &e);
  g.num_bars = random_on_range(g.model_dim, max_bars, &e);

  g.piece = random_piece(g.num_tracks, g.num_bars, opz, &e);
  status_from_piece(&g.status, &g.piece, &e);
  random_protobuf(&g.param, &e);
  add_checkpoint_to_param(estr.c_str(), &g.param);

  g.param.set_verbose(false);
  g.param.set_internal_skip_preprocess(false);
  g.param.set_max_steps(0);
  g.param.set_model_dim(g.model_dim);

  return g;
}

// =====================================================================

void dev(void) {  
  e.seed(time(NULL));
}

// =====================================================================

// pretty sure that getEncoder is a memory leak

// write a basic readme

// any inputs should either succeed or return
// invalid error


// check generating using a checkpoint


// time_sig mismatch should throw invalid argument error
void test_time_sig_mismatch() {
  midi::Piece p = random_piece(4, 4, false, &e);
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
  generation_inputs g = random_generation_inputs(4,8);

  BOOL_MATRIX m = ones_boolean_matrix(g.num_tracks, g.num_bars);  
  set_selected_bars(&g.status, m);

  g.param.set_tracks_per_step(g.num_tracks);
  g.param.set_bars_per_step(g.num_bars);
  mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

  TEST_ASSERT(g.debug.tokens.size() == 1);

}

// test generation with random status
void test_random_status() {

  for (int i=0; i<10; i++) {

    generation_inputs g = random_generation_inputs();
    random_protobuf_inner(&g.status, &e, {"track_id","track_type"});
    
    //g.param.set_verbose(true);
    g.param.set_internal_skip_preprocess(false);

    //print_protobuf(&g.param);
    //print_protobuf(&g.status);

    // two OK possibilities 
    // 1) generation succeeds
    // 2) validation of inputs fails throwing invalid_argument

    try {
      mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);
    }
    catch (const std::invalid_argument& error) { 
      std::cout << error.what() << std::endl;
    }
  }
}

void test_infill_w_model(void) {
  generation_inputs g = random_generation_inputs(4,16,true);
  // print sample param so we can see what is going on
  print_protobuf(&g.param);

  BOOL_MATRIX m = random_boolean_matrix(g.num_tracks, g.num_bars, &e);
  set_selected_bars(&g.status, m);

  midi::Piece orig(g.piece);
  mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

  // make sure bars / tracks don't change
  tracks_are_identical(&g.piece, &orig);
  bars_are_identical(&g.piece, &orig, m);

}

void test_infill(void) {

  for (int i=0; i<num_trials; i++) {

    generation_inputs g = random_generation_inputs();

    // print sample param so we can see what is going on
    print_protobuf(&g.param);

    BOOL_MATRIX m = random_boolean_matrix(g.num_tracks, g.num_bars, &e);
    set_selected_bars(&g.status, m);

    midi::Piece orig(g.piece);
    mmm::sample_w_debug(&g.piece, &g.status, &g.param, &g.debug);

    // make sure bars / tracks don't change
    tracks_are_identical(&g.piece, &orig);
    bars_are_identical(&g.piece, &orig, m);

  }
}

void test_autoreg(void) {

  for (int i=0; i<num_trials; i++) {

    generation_inputs g = random_generation_inputs();

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
    tracks_are_identical(&g.piece, &orig);
    bars_are_identical(&g.piece, &orig, m);

    // make sure that status selections
    // have been applied to the token sequence
    // split sequence into tokens per track
    // if we store track order in debug
    // then we can figure out which track corresponds to which
    ENCODER *enc = encoder_from_ckpt(&g.param);
    REPRESENTATION *rep = enc->rep;

    for (int step=0; step<g.debug.tokens.size(); step++) {
      if (g.debug.modes[step] == TRACK_MODEL) {

        std::vector<std::vector<int>> tracks = split_tokens_into_tracks(
          g.debug.tokens[step], rep);

        for (int track_num=0; track_num<tracks.size(); track_num++) {
          int index = g.debug.orders[step][track_num];
          midi::StatusTrack st = g.status.tracks(index);

          bool is_resample = st.resample();
          bool is_drum = is_drum_track(st.track_type());
          bool can_polyphony = ((!enc->config->mark_drum_density) | (!is_drum));

          if (is_resample) {

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
            int t = find_token(enc, INSTRUMENT, tracks[track_num]);
            TEST_ASSERT( t >= 0 );
            TEST_ASSERT( std::find(
              insts.begin(), insts.end(), enc->rep->decode(t)) != insts.end() );
          }

        }
      }
    }
  }
}



/*
void test_single(void) {
  midi::Piece p;
  midi::Status s;
  std::vector<std::string> insts = {"any"};
  autoregressive_inputs(insts, 4, &p, &s);
  mmm::Debugger debug;
  std::string ckpt = "random";
  midi::SampleParam param = build_sample_param(ckpt);

  // set the polyphony and note duration
  midi::StatusTrack *strack = s.mutable_tracks(0);
  strack->set_min_polyphony_q( midi::PolyphonyLevel::TWO );
  strack->set_max_polyphony_q( midi::PolyphonyLevel::THREE );
  strack->set_min_note_duration_q( midi::NoteDurationLevel::SIXTEENTH );
  strack->set_max_note_duration_q( midi::NoteDurationLevel::QUARTER );

  run_generate(&p, &s, &debug, ckpt.c_str());

  mmm::ENCODER *enc = get_encoder(ckpt);

  int target = enc->rep->encode(MIN_POLYPHONY, midi::PolyphonyLevel::TWO - 1);
  TEST_CHECK(contains_token(enc, target, debug.tokens[0]) );

}

// test malformed status

// both ignore and resample on one track
void test_ignore_resample_combination(void) {
  midi::Piece p;
  midi::Status s;

  add_blank_track(&p, 4, 0, OPZ_KICK_TRACK);
  status_from_piece(&p, &s);
  set_selected_bars(&s, {{true,true,true,true}});

  midi::StatusTrack *st = s.mutable_tracks(0);
  st->set_resample(true);
  st->set_ignore(true);

  run_generate(&p, &s, NULL, "random");
}



void test_example(void) {
    
  midi::Piece p;
  midi::Status s;
  mmm::Debugger debug;
  //std::string ckpt = "paper_bar_4.pt";
  std::string ckpt = "random";

  p.set_resolution(12);

  mmm::TRACK_TYPE tt = mmm::TRACK_TYPE::OPZ_SNARE_TRACK;

  midi::StatusTrack *strack = s.add_tracks();
  strack->set_track_id( 0 );
  strack->set_track_type( tt );
  strack->set_instrument( mmm::gm_inst_to_string(tt,0) );
  strack->set_density( 4 );
  strack->set_resample( true );
  for (int i=0; i<4; i++) {
    strack->add_selected_bars( true );
  }

  strack->set_min_polyphony_q( midi::PolyphonyLevel::TWO );
  strack->set_max_polyphony_q( midi::PolyphonyLevel::THREE );

  strack->set_min_note_duration_q( midi::NoteDurationLevel::SIXTEENTH );
  strack->set_max_note_duration_q( midi::NoteDurationLevel::QUARTER );


  run_generate(&p, &s, &debug, ckpt.c_str());

  mmm::ENCODER *enc = get_encoder(ckpt);
  //std::cout << count_token(enc, mmm::TOKEN_TYPE::BAR, debug.tokens[0]) << std::endl;

  //for (const auto token : debug.tokens[0]) {
  //  std::cout << enc->rep->pretty(token) << std::endl;
  //}

}
*/

TEST_LIST = {
  { "dev", dev },
  { "test_time_sig_mismatch", test_time_sig_mismatch },
  { "test_single_step", test_single_step },
  { "test_infill", test_infill },
  { "test_infill_w_model", test_infill_w_model },
  { "test_autoreg", test_autoreg },
  { "test_random_status", test_random_status },
   //{ "test_autoreg", test_autoreg },
   { NULL, NULL }     /* zeroed record marking the end of the list */
};
