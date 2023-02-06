#pragma once

#include <torch/script.h> // One-stop header.
#include <torch/nn/functional/activation.h>
//#include <torch/nn/modules/functional.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <set>

#include "encoder/encoder_all.h"
#include "enum/model_type.h"
#include "sample_fix.h"

// additional constraints
// 1. can't start a note that is already sounding ()

void load_model(std::string &ckpt_path, torch::jit::Module *m) {
  try {
    *m = torch::jit::load(ckpt_path);
  }
  catch (const c10::Error& e) {
    throw std::runtime_error("ERROR LOADING MODEL.");
  }
}

void sample_inner(Control *ctrl, std::vector<std::vector<int>> &seqs, float temperature, torch::jit::Module *model, std::vector<torch::jit::IValue> &inputs, ENCODER *encoder, int *num_bars_sampled) {

  auto outputs = model->forward(inputs).toTuple();
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
    // always apply global mask
    for (int j=0; j<ctrl->global_mask.size(); j++) {
      if (ctrl->global_mask[j] == 0) {
        logits[i][j] = -1 * std::numeric_limits<float>::max();
      }
    }
  }

  namespace F = torch::nn::functional;
  auto probs = (logits / temperature).softmax(1);
  auto next_tokens = probs.multinomial(1);

  inputs.clear();
  inputs.push_back( next_tokens );

  if (encoder->config->embed_dim) {

    int embed_dim = encoder->config->embed_dim;
    auto opts = torch::TensorOptions().dtype(torch::kFloat32);
    // WARNING THE 3 is hard coded here
    torch::Tensor c = torch::zeros({next_tokens.size(0),1,embed_dim},opts);
    for (int i=0; i<seqs.size(); i++) {
      std::vector<double> embed = ctrl->get_bar_embed(i);
      for (int j=0; j<embed.size(); j++) {
        c[i][0][j] = embed[j];
      }
      if (seqs[i].back() == encoder->rep->encode(FILL_IN,2)) {
        ctrl->increment_bar_count(i);
      }
    }
    inputs.push_back( c );
  
  }

  inputs.push_back( past_key_values );

  // add next token to the sequences
  for (int i=0; i<seqs.size(); i++) {
    if (!ctrl->is_finished(i)) {
      seqs[i].push_back( next_tokens[i][0].item<int64_t>() );
    }
  }

  if (encoder->rep->is_token_type(seqs[0].back(),BAR_END)) {
    (*num_bars_sampled)++;
  }
}

std::vector<std::vector<int>> sample_deprecated(SampleConfig *sc) {

  torch::jit::Module m;
  load_model(sc->ckpt_path, &m);

  int num_bars_sampled = 0;

  if (sc->verbose) {
    std::cout << "PROMPT BEGIN" << std::endl;
    std::cout << "PROMPT LENGTH : " << sc->prompt.size() << std::endl;
    for (const auto token : sc->prompt) {
      std::cout << sc->encoder->rep->pretty(token) << std::endl;
    }
    std::cout << "PROMPT END" << std::endl;
  }
  
  std::vector<torch::jit::IValue> inputs;

  // create the inputs by tileing prompt
  auto opts = torch::TensorOptions().dtype(torch::kInt64);
  torch::Tensor x = torch::zeros({sc->batch_size,(int)sc->prompt.size()},opts);
  for (int k=0; k<sc->batch_size; k++) {
    for (int i=0; i<sc->prompt.size(); i++) {
      x[k][i] = sc->prompt[i];
    }
  }
  inputs.push_back( x );

  // here we add in the control ids
  /*
  opts = torch::TensorOptions().dtype(torch::kFloat32);
  torch::Tensor c = torch::zeros({sc->batch_size,(int)sc->prompt.size(),3},opts);
  inputs.push_back( c );
  */

  // create empty state
  // TODO :: infer the rest of the state dimensions from the model
  /*
  std::vector<torch::jit::IValue> state;
  for (int i=0; i<NUM_LAYERS; i++) {
    state.push_back(torch::zeros({2, sc->batch_size, 8, 0, 64}));
  }
  inputs.push_back( torch::ivalue::Tuple::create(state) );
  */

  // create empty state v2
  std::vector<torch::jit::IValue> state;
  for (int i=0; i<NUM_LAYERS; i++) {
    std::vector<torch::jit::IValue> tuple;
    for (int j=0; j<2; j++) {
      tuple.push_back( torch::zeros({sc->batch_size, 8, 0, 64}) );
    }
    state.push_back( torch::ivalue::Tuple::create(tuple) );
  }
  //inputs.push_back( torch::ivalue::Tuple::create(state) );

  // create empty sequnces
  std::vector<std::vector<int>> seqs;
  for (int k=0; k<sc->batch_size; k++) {
    seqs.push_back( sc->prompt );
  }

  sc->control.start(sc->batch_size); // initialize the control
  int num_steps = 0;
  while (!sc->control.all_finished()) {
    sample_inner(&sc->control, seqs, sc->temperature, &m, inputs, sc->encoder, &num_bars_sampled);
    num_steps++;
    if (sc->verbose) {
      std::cout << num_steps << " | "; 
      for (int i=0; i<seqs.size(); i++) {
        std::cout << sc->encoder->rep->pretty(seqs[i].back()) << " ";
      }
      std::cout << std::endl;
    }
    if ((sc->max_steps > 0) && (num_steps >= sc->max_steps)) {
      break;
    }
  }

  return seqs;
}

void make_state_legacy(std::vector<torch::jit::IValue> *state, int batch_size) {
  for (int i=0; i<NUM_LAYERS; i++) {
    state->push_back(torch::zeros({2, batch_size, 8, 0, 64}));
  }
}

void make_state(std::vector<torch::jit::IValue> *state, int batch_size) {
  for (int i=0; i<NUM_LAYERS; i++) {
    std::vector<torch::jit::IValue> tuple;
    for (int j=0; j<2; j++) {
      tuple.push_back( torch::zeros({batch_size, 8, 0, 64}) );
    }
    state->push_back( torch::ivalue::Tuple::create(tuple) );
  }
}

std::vector<midi::Piece> generate(midi::Status *status, midi::Piece *piece, midi::SampleParam *param, float temp, int batch_size, bool verbose, std::map<std::tuple<int,MODEL_TYPE>,std::tuple<ENCODER_TYPE,std::string>> &ckpt_map, int max_steps=0) {

  bool terminated = false;

  // returns prompt control, encoder_type, ckpt_path, order
  SampleConfig sc;
  sc.temperature = temp;
  sc.batch_size = batch_size;
  sc.verbose = verbose;
  sc.skip_preprocess = param->skip_preprocess(); // WATCH OUT
  prepare_generate(status, piece, &sc, ckpt_map);
  
  torch::jit::Module m;
  load_model(sc.ckpt_path, &m);

  int num_bars_sampled = 0;

  if (verbose) {
    std::cout << "PROMPT BEGIN" << std::endl;
    std::cout << "PROMPT LENGTH : " << sc.prompt.size() << std::endl;
    for (const auto token : sc.prompt) {
      std::cout << sc.encoder->rep->pretty(token) << std::endl;
    }
    std::cout << "PROMPT END" << std::endl;
  }
  
  std::vector<torch::jit::IValue> inputs;

  // create the inputs by tileing prompt
  auto opts = torch::TensorOptions().dtype(torch::kInt64);
  torch::Tensor x = torch::zeros({batch_size, (int)sc.prompt.size()}, opts);
  for (int k=0; k<batch_size; k++) {
    for (int i=0; i<sc.prompt.size(); i++) {
      x[k][i] = sc.prompt[i];
    }
  }
  inputs.push_back( x );

  if (sc.encoder->config->embed_dim) {

    int embed_dim = sc.encoder->config->embed_dim;
    opts = torch::TensorOptions().dtype(torch::kFloat32);
    torch::Tensor c = torch::zeros({batch_size,(int)sc.prompt.size(),embed_dim},opts);
    for (int k=0; k<batch_size; k++) {
      for (int i=0; i<sc.prompt.size(); i++) {
        for (int j=0; j<embed_dim; j++) {
          c[k][i][j] = sc.embeds[i][j];
        }
      }
    }
    inputs.push_back( c );


    // create empty state v2
    std::vector<torch::jit::IValue> state;
    make_state(&state, sc.batch_size);
    inputs.push_back( torch::ivalue::Tuple::create(state) );

  }
  else {
    // create empty state
    // TODO :: infer the rest of the state dimensions from the model
    std::vector<torch::jit::IValue> state;
    if ((param) && (param->new_state())) {
      make_state(&state, sc.batch_size);
    }
    else {
      make_state_legacy(&state, sc.batch_size);
    }
    inputs.push_back( torch::ivalue::Tuple::create(state) );

    /*
    std::vector<torch::jit::IValue> state;
    for (int i=0; i<NUM_LAYERS; i++) {
      state.push_back(torch::zeros({2, batch_size, 8, 0, 64}));
    }
    inputs.push_back( torch::ivalue::Tuple::create(state) );
    */
  }

  // create empty sequnces
  std::vector<std::vector<int>> seqs;
  for (int k=0; k<batch_size; k++) {
    seqs.push_back( sc.prompt );
  }

  sc.control.start(batch_size); // initialize the control
  int num_steps = 0;
  while (!sc.control.all_finished()) {
    sample_inner(&sc.control, seqs, temp, &m, inputs, sc.encoder, &num_bars_sampled);
    num_steps++;
    if (verbose) {
      std::cout << num_steps << " | "; 
      for (int i=0; i<seqs.size(); i++) {
        std::cout << sc.encoder->rep->pretty(seqs[i].back()) << " ";
      }
      std::cout << std::endl;
    }
    // set the new embeds here

    // quit if we get stuck repeating token
    //if ((param->max_repeats() > 0) && ()) {
    //
    //}

    // quit if we go for too long
    if ((param->max_steps() > 0) && (num_steps >= param->max_steps())) {
      terminated = true;
      break;
    }
  }

  // convert back to piece
  std::vector<midi::Piece> output(batch_size);
  if (!terminated) {
    sc.encoder->tokens_to_json_array(seqs, output);
  }
  return output;
}