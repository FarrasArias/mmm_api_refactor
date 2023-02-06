#include <torch/script.h> // One-stop header.
#include <torch/nn/functional/activation.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <set>

;

#include "encoder/encoder_all.h"
#include "enum/gm.h"
#include "enum/model_type.h"

#include "protobuf/validate.h"
#include "protobuf/util.h"

static const int NUM_LAYERS = 6;

// sample config for the common preferences
class SampleConfig {
public:
  SampleConfig() {
    batch_size = 1;
    temperature = 1.;
    verbose = false;
    num_bars = 4;
  }
  int batch_size;
  float temperature;
  bool verbose;
  int num_bars;
};

class Control {
public:
  Control(ENCODER *e) {
    encoder = e;
    int vocab_size = encoder->rep->max_token();
    null_trigger = std::vector<int>(vocab_size,0);
    null_mask = std::vector<int>(vocab_size,1);
  }
  std::vector<int> encode(mmm::TOKEN_TYPE tt, std::vector<int> values) {
    if (tt == NONE) {
      return null_mask;
    }
    return encoder->rep->encode_to_one_hot(tt, values);
  }
  void add(mmm::TOKEN_TYPE trigger_tt, std::vector<int> trigger_values, mmm::TOKEN_TYPE mask_tt, std::vector<int> mask_values) {
    controls.push_back( std::make_pair(
      encode(trigger_tt, trigger_values), encode(mask_tt, mask_values)) );
  }
  void start(int n) {
    pos = std::vector<int>(n,0);
  }
  void show_mask(std::vector<int> mask) {
    for (const auto v : mask) {
      if (v==0) { std::cout << "-"; }
      else { std::cout << "x"; }
    }
    std::cout << std::endl;
  }
  void show() {
    for (const auto control : controls) {
      std::cout << "TRIGGER : ";
      show_mask(std::get<0>(control));
      std::cout << "MASK : ";
      show_mask(std::get<1>(control));
    }
  }
  bool is_finished(int index) {
    if (index >= pos.size()) {
      throw std::runtime_error("CONTROL INDEX OUT OF RANGE");
    }
    return pos[index] >= controls.size();
  }
  bool all_finished() {
    for (int i=0; i<pos.size(); i++) {
      if (!is_finished(i)) {
        return false;
      }
    }
    return true;
  }
  bool check_trigger(int index, int value) {
    if (is_finished(index)) {
      return false; // no more triggers
    }
    return (std::get<0>(controls[pos[index]])[value] == 1);
  }
  std::vector<int> get_mask(int index) {
    return std::get<1>(controls[pos[index]]);
  }
  void increment(int index) {
    pos[index]++;
  }
  std::vector<std::tuple<std::vector<int>,std::vector<int>>> controls;
  ENCODER *encoder;
  std::vector<int> pos;
  std::vector<int> null_trigger; // all zeros
  std::vector<int> null_mask; // all ones
};

class Sampler {
public:
  Sampler(std::string ckpt_path, ENCODER_TYPE e) {
    // ckpt_path must be absolute path
    load_model(ckpt_path);
    encoder = getEncoder(e);
    //TODO : check that the encoder and model have the same dimensionality
    //for (const auto v : model.attributes()) {
    //  std::cout << v << std::endl;
    //}
  }
  void load_model(std::string &ckpt_path) {
    try {
      model = torch::jit::load(ckpt_path);
    }
    catch (const c10::Error& e) {
      std::cerr << "error loading the model\n";
      return;
    }
  }

  void sample(std::vector<int> &prompt, Control *ctrl, std::vector<midi::Piece> &output, SampleConfig *sc) {

    num_bars_sampled = 0;

    if (sc->verbose) {
      std::cout << "PROMPT LENGTH : " << prompt.size() << std::endl;
    }
    
    // this is not thread safe maybe change
    inputs.clear(); // empty previous inputs

    // create the inputs by tileing prompt
    auto opts = torch::TensorOptions().dtype(torch::kInt64);
    torch::Tensor x = torch::zeros({sc->batch_size, (int)prompt.size()}, opts);
    for (int k=0; k<sc->batch_size; k++) {
      for (int i=0; i<prompt.size(); i++) {
        x[k][i] = prompt[i];
      }
    }
    inputs.push_back( x );

    // create empty state
    // TODO :: infer the rest of the state dimensions from the model
    std::vector<torch::jit::IValue> state;
    for (int i=0; i<NUM_LAYERS; i++) {
      state.push_back(torch::zeros({2, sc->batch_size, 8, 0, 64}));
    }
    inputs.push_back( torch::ivalue::Tuple::create(state) );

    // create empty sequnces
    std::vector<std::vector<int>> seqs;
    for (int k=0; k<sc->batch_size; k++) {
      seqs.push_back( prompt );
    }

    ctrl->start(sc->batch_size); // initialize the control
    int num_steps = 0;
    while (!ctrl->all_finished()) {
      //cout << "start while loop " << std::endl;
      sample_inner(ctrl, seqs, sc->temperature);
      num_steps++;
      if (sc->verbose) {
        std::cout << num_steps << " | "; 
        for (int i=0; i<seqs.size(); i++) {
          std::cout << encoder->rep->pretty(seqs[i].back()) << " ";
        }
        std::cout << std::endl;
      }
    }

    // convert back to piece
    EncoderConfig ec;
    encoder->tokens_to_json_array(seqs, output);
  }

  void sample_inner(Control *ctrl, std::vector<std::vector<int>> &seqs, float temperature) {
    auto outputs = model.forward(inputs).toTuple();
    auto logits = outputs->elements()[0].toTensor().index({torch::indexing::Slice(),-1,torch::indexing::Slice()});
    auto past_key_values = outputs->elements()[1];

    // override values in next_tokens if necessary
    // set avoided values to a very small value 

    for (int i=0; i<seqs.size(); i++) {
      if ((seqs[i].size() > 0) && (ctrl->check_trigger(i, seqs[i].back()))) {
        std::vector<int> mask = ctrl->get_mask(i);
        for (int j=0; j<mask.size(); j++) {
          if (mask[j] == 0) {
            // set this to a very small possibility
            logits[i][j] = -1 * std::numeric_limits<float>::max();
          }
        }
        ctrl->increment(i);
      }
    }

    namespace F = torch::nn::functional;
    auto probs = (logits / temperature).softmax(1);
    auto next_tokens = probs.multinomial(1);

    inputs.clear();
    inputs.push_back( next_tokens );
    inputs.push_back( past_key_values );

    // add next token to the sequences
    for (int i=0; i<seqs.size(); i++) {
      if (!ctrl->is_finished(i)) {
        seqs[i].push_back( next_tokens[i][0].item<int64_t>() );
      }
    }

    if (encoder->rep->is_token_type(seqs[0].back(),BAR_END)) {
      num_bars_sampled++;
    }
  }

  // api will accept midi::Piece and return midi::Piece
  void generate_bars(std::vector<std::tuple<int,int>> &bars, midi::Piece *p, std::vector<midi::Piece> &seqs, SampleConfig *sc) {

    std::vector<int> prompt;
    if (p) {
      std::set<std::tuple<int,int>> barset;
      std::copy(bars.begin(), bars.end(), std::inserter(barset, barset.end()));
      encoder->config->multi_fill = barset;
      encoder->config->num_bars = sc->num_bars;
      prompt = encoder->encode(p);

      // remove everything after fill_start token
      int fill_start = encoder->rep->encode(FILL_IN,1);
      for (int index=0; index<prompt.size(); index++) {
        if (prompt[index] == fill_start) {
          prompt.resize(index+1);
          break;
        }
      }
    }
    else {
      prompt.push_back( encoder->rep->encode(PIECE_START,0) );
    }

    Control control(encoder);
    for (int i=0; i<bars.size(); i++) {
      control.add(FILL_IN, {2}, NONE, {});
    }
    if (sc->verbose) {
      control.show();
    }

    sample(prompt, &control, seqs, sc);
  }

  void generate_tracks(std::vector<std::tuple<int,std::string,int>> &tracks, midi::Piece *p, std::vector<midi::Piece> &seqs, SampleConfig *sc) {
    
    Control control(encoder);

    int track_num = 0;
    bool piece_is_empty = p->tracks_size()==0;
    for (const auto track : tracks) {
      std::vector<int> insts = GM[std::get<1>(track)];
      int track_type = std::get<0>(track);
      int density = std::get<2>(track);

      if (track_type == STANDARD_BOTH) {
        track_type = -1;
      }

      if (sc->verbose) {
        std::cout << "GENERATING : " << track_type << " with density " << density << std::endl;
      }
      
      for (int i=0; i<insts.size(); i++) {
        insts[i] %= 128;
      }
      if ((track_num == 0) && (piece_is_empty)) {
        control.add(PIECE_START, {0}, TRACK, {track_type});
      }
      else {
        control.add(TRACK_END, {0}, TRACK, {track_type});
      }
      control.add(TRACK, {-1}, INSTRUMENT, insts);

      // density control
      if (density >= 0) {
        control.add(INSTRUMENT, {-1}, DENSITY_LEVEL, {density});
      }
      
      track_num++;
    }
    control.add(TRACK_END, {0}, NONE, {});

    if (sc->verbose) {
      control.show();
    }

    std::vector<int> prompt;
    if (!piece_is_empty) {
      prompt = encoder->encode(p);
    }
    else {
      prompt.push_back( encoder->rep->encode(PIECE_START,0) );
    }

    sample(prompt, &control, seqs, sc);

  }

  torch::jit::script::Module model;
  std::vector<torch::jit::IValue> inputs;
  ENCODER *encoder;
  int num_bars_sampled;
};

class Generator {
public:
  Generator(std::map<std::tuple<int,MODEL_TYPE,ENCODER_TYPE>,std::string> &ckpt_paths) {
    for (const auto kv : ckpt_paths) {
      samplers[std::make_tuple(std::get<0>(kv.first),std::get<1>(kv.first))] = new Sampler(
        kv.second, std::get<2>(kv.first));
    }
  }

  std::vector<midi::Piece> generate(midi::Status *status, midi::Piece *piece, float temp, int batch_size, bool verbose) {
    validate_status(status, piece, verbose);
    update_has_notes(piece);

    SampleConfig sc;
    sc.temperature = temp;
    sc.batch_size = batch_size;
    sc.verbose = verbose;
    sc.num_bars = status->tracks(0).selected_bars_size();

    std::vector<midi::Piece> output(batch_size);

    std::tuple<int,MODEL_TYPE> model_key;


    // for bar-fill each tradk_id must be in range
    // for track each track_id of condition must be in range
    // there should not be extra tracks in piece that are unreferenced

    std::vector<std::tuple<int,std::string,int>> tracks;
    std::vector<std::tuple<int,int>> bars;
    int num_cond_tracks = 0;
    int num_resample_tracks = 0;
    int num_infill_tracks = 0;
    std::vector<STATUS_TRACK_TYPE> track_types;
    std::vector<int> order;
    std::vector<int> cond_tracks;

    int track_num = 0;
    for (const auto track : status->tracks()) {
      STATUS_TRACK_TYPE tt = infer_track_type(track);
      switch( tt ) {
        case CONDITION :
          order.push_back( num_cond_tracks );
          cond_tracks.push_back( track.track_id() );
          num_cond_tracks++;
          break;
        case RESAMPLE : 
          order.push_back( num_resample_tracks );
          tracks.push_back(std::make_tuple(
            track.track_type(), track.instrument(), track.density()));
          num_resample_tracks++;
          break;
        case INFILL :     
          num_infill_tracks++;
          break;
      }
      track_types.push_back( tt );
      int bar_num = 0;
      for (const auto selected : track.selected_bars()) {
        if (selected) {
          bars.push_back( std::make_pair(track_num, bar_num) );
        }
        bar_num++;
      }
      track_num++;
    }

    // use bar infill model
    if (num_infill_tracks > 0) {

      if (sc.verbose) {
        std::cout << "GENERATING " << bars.size() << " BARS" << std::endl;
      }

      // remove excess bars if any
      prune_tracks_dev2(
        piece, arange(0,piece->tracks_size(),1), arange(0,sc.num_bars,1));

      model_key = std::make_pair(sc.num_bars, BAR_INFILL_MODEL);
      if (samplers.find(model_key) == samplers.end()) {
        throw invalid_argument("NO MODEL FOUND!");
      }
      samplers[model_key]->generate_bars(bars, piece, output, &sc);

    }
    else {

      if (sc.verbose) {
        std::cout << "GENERATING " << num_resample_tracks << " TRACKS" << std::endl;
      }

      // fix the order
      // order is the output position for each track
      for (track_num=0; track_num<status->tracks_size(); track_num++) {
        if (track_types[track_num] == RESAMPLE) {
          order[track_num] = order[track_num] + num_cond_tracks;
        }
      }
      std::vector<int> inverse_order(order.size(),0);
      for (int i=0; i<order.size(); i++) {
        inverse_order[order[i]] = i;
      }

      // prune unneeded tracks
      prune_tracks_dev2(piece, cond_tracks, arange(0,sc.num_bars,1));

      // call generation
      model_key = std::make_pair(sc.num_bars, TRACK_MODEL);
      if (samplers.find(model_key) == samplers.end()) {
        throw invalid_argument("NO MODEL FOUND!");
      }
      samplers[model_key]->generate_tracks(tracks, piece, output, &sc);

      // reorder the tracks
      for (int i=0; i<output.size(); i++) {
        reorder_tracks(&(output[i]), inverse_order);
      }

    }

    return output;
  }

  std::vector<std::string> generate_py(std::string &status_str, std::string &piece_str, float temp, int batch_size, bool verbose) {
    midi::Piece piece;
    google::protobuf::util::JsonStringToMessage(piece_str.c_str(), &piece);
    midi::Status status;
    google::protobuf::util::JsonStringToMessage(status_str.c_str(), &status);
    auto pieces = generate(&status, &piece, temp, batch_size, verbose);
    std::vector<std::string> output;
    for (const auto piece : pieces) {
      std::string json_str;
      google::protobuf::util::MessageToJsonString(piece, &json_str);
      output.push_back( json_str );
    }
    return output;
  }

  std::vector<std::string> generate_w_midi_py(std::string &status_str, std::string &filepath, float temp, int batch_size, bool verbose) {
    midi::Piece piece;
    EncoderConfig e; // this needs to be fixed
    parse_new(filepath, &piece, &e, NULL);
    midi::Status status;
    google::protobuf::util::JsonStringToMessage(status_str.c_str(), &status);
    auto pieces = generate(&status, &piece, temp, batch_size, verbose);
    std::vector<std::string> output;
    for (const auto piece : pieces) {
      std::string json_str;
      google::protobuf::util::MessageToJsonString(piece, &json_str);
      output.push_back( json_str );
    }
    return output;
  }

  std::map<std::tuple<int,MODEL_TYPE>,Sampler*> samplers;

};

