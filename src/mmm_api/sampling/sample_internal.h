#pragma once

#include <torch/script.h> // One-stop header.
#include <torch/nn/functional/activation.h>
//#include <torch/nn/modules/functional.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <set>

#include "../encoder/encoder_all.h"
#include "../enum/model_type.h"
#include "control.h"

namespace mmm {

class ModelMeta {
public:
  torch::jit::Module model;
  midi::ModelMetadata meta;
};

static const int NUM_LAYERS = 6;

// additional constraints
// 1. can't start a note that is already sounding ()

void load_model(const std::string &ckpt_path, ModelMeta *m) {
  try {
    std::unordered_map<std::string, std::string> loaded_extra_files;
    loaded_extra_files["metadata.json"] = "";
    m->model = torch::jit::load(ckpt_path, torch::kCPU, loaded_extra_files);
    if (loaded_extra_files["metadata.json"].size() == 0) {
      throw std::runtime_error("ERROR LOADING MODEL : MODEL CONTAINS NO METADATA!");
    }
    string_to_protobuf(loaded_extra_files["metadata.json"], &m->meta);
    //std::cout << "METADATA :: " << protobuf_to_string(&m->meta) << std::endl;
  }
  catch (const c10::Error& e) {
    std::cout << e.what() << std::endl;
    throw std::runtime_error("ERROR LOADING MODEL.");
  }
}

void sample_inner(std::vector<std::unique_ptr<SAMPLE_CONTROL>> &scon, std::vector<std::vector<int>> &seqs, torch::jit::Module *model, std::vector<torch::jit::IValue> &inputs, midi::SampleParam *param, CallbackManager *callbacks) {
  
  if ((!model) && (!param->internal_random_sample_mode())) {
    throw std::runtime_error("ERROR : MODEL IS INVALID.");
  }

  torch::Tensor logits;
  torch::jit::IValue past_key_values;
  if (param->internal_random_sample_mode()) {
    int vocab_size = scon[0]->enc->rep->max_token();
    auto opts = torch::TensorOptions().dtype(torch::kFloat32);
    logits = torch::ones({param->batch_size(),vocab_size},opts);
  }
  else {
    auto outputs = model->forward(inputs).toTuple();
    logits = outputs->elements()[0].toTensor().index(
      {torch::indexing::Slice(),-1,torch::indexing::Slice()});
    past_key_values = outputs->elements()[1];
  }

  // set masks
  for (int i=0; i<seqs.size(); i++) {
    std::vector<int> mask = scon[i]->get_mask( seqs[i] );
    if (param->verbose()) {
      scon[i]->rep->show_mask_token_types(mask);
    }
    if ((!scon[i]->finished) && (!param->internal_disable_masking())) {
      for (int j=0; j<mask.size(); j++) {
        if (mask[j] == 0) {
          // set this to a very small possibility
          logits[i][j] = -1 * std::numeric_limits<float>::max();
        }
      }
    }
  }

  //for (int i=0; i<logits.sizes()[1]; i++) {
  //  std::cout << "logits[" << i << "] = " << logits[0][i].item<float_t>() << /std::endl;
  //}

  // sample from probs
  float temperature = param->temperature();
  if (param->use_per_track_temperature()) {
    temperature = scon[0]->get_temperature();
  }
  auto probs = (logits / temperature).softmax(1);
  auto next_tokens = probs.multinomial(1);

  if (!param->internal_random_sample_mode()) {
    inputs.clear();
    inputs.push_back( next_tokens );

    /*
    ENCODER *encoder = scon[0]->enc;
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
      }
      inputs.push_back( c );
    }
    */

    inputs.push_back( past_key_values );
  }
  

  // add next token to the sequences
  for (int i=0; i<seqs.size(); i++) {
    if (!scon[i]->finished) {
      int next_token = next_tokens[i][0].item<int64_t>();
      seqs[i].push_back( next_token );
      if (callbacks) {
        callbacks->update(&scon[i]->enc, next_token);
      }
    }
  }
}

void make_state_legacy(std::vector<torch::jit::IValue> *state, int batch_size, midi::ModelMetadata *meta) {
  for (int i=0; i<meta->num_layers(); i++) {
    state->push_back(torch::zeros({2, batch_size, meta->num_heads(), 0, meta->num_hidden()}));
  }
}

void make_state(std::vector<torch::jit::IValue> *state, int batch_size, midi::ModelMetadata *meta) {
  for (int i=0; i<meta->num_layers(); i++) {
    std::vector<torch::jit::IValue> tuple;
    for (int j=0; j<2; j++) {
      tuple.push_back( torch::zeros({batch_size, meta->num_heads(), 0, meta->num_hidden()}) );
    }
    state->push_back( torch::ivalue::Tuple::create(tuple) );
  }
}

std::vector<midi::Piece> generate(midi::Status *status, midi::Piece *piece, midi::SampleParam *param, Debugger *debug, ModelMeta *mm, CallbackManager *callbacks) {

  float temp = param->temperature();
  int batch_size = param->batch_size();
  bool verbose = param->verbose();

  // make sure that temperature is not zero
  param->set_temperature( std::max((double)param->temperature(), 1e-6) );

  bool terminated = false;

  // returns prompt control, encoder_type, ckpt_path, order
  //SampleConfig sc;
  //sc.temperature = temp;
  //sc.batch_size = batch_size;
  //sc.verbose = verbose;
  //sc.internal_skip_preprocess = param->internal_skip_preprocess(); // WATCH OUT
  //prepare_generate(status, piece, &sc, ckpt_map);

  std::vector<std::unique_ptr<SAMPLE_CONTROL>> scon;
  for (int i=0; i<param->batch_size(); i++) {
    scon.push_back( std::move(
      std::make_unique<SAMPLE_CONTROL>(piece,status,param,&mm->meta)) );
  }

  std::vector<int> prompt = scon[0]->prompt;
  
  //torch::jit::Module m;
  //midi::ModelMetadata meta;
  //if (!param->internal_random_sample_mode()) {
  //  load_model(scon[0]->ckpt_path, &m, &meta);
  //}

  if (param->verbose()) {
    //std::cout << "REP GRAPH" << std::endl;
    //scon[0]->rg->show();
    //std::cout << "REP GRAPH END" << std::endl;
    std::cout << "PROMPT BEGIN" << std::endl;
    std::cout << "PROMPT LENGTH : " << prompt.size() << std::endl;
    for (const auto token : prompt) {
      std::cout << scon[0]->rep->pretty(token) << std::endl;
    }
    std::cout << "PROMPT END" << std::endl;
  }
  


  std::vector<torch::jit::IValue> inputs;

  // we don't need inputs if we are randomly sampling?

  if (!param->internal_random_sample_mode()) {

    // create the inputs by tileing prompt
    auto opts = torch::TensorOptions().dtype(torch::kInt64);
    torch::Tensor x = torch::zeros({batch_size, (int)prompt.size()}, opts);
    for (int k=0; k<batch_size; k++) {
      for (int i=0; i<prompt.size(); i++) {
        x[k][i] = prompt[i];
      }
    }
    inputs.push_back( x );

    if (scon[0]->enc->config->embed_dim) {

      int embed_dim = scon[0]->enc->config->embed_dim;
      opts = torch::TensorOptions().dtype(torch::kFloat32);
      torch::Tensor c = torch::zeros({batch_size,(int)prompt.size(),embed_dim},opts);
      for (int k=0; k<batch_size; k++) {
        for (int i=0; i<prompt.size(); i++) {
          for (int j=0; j<embed_dim; j++) {
            c[k][i][j] = scon[0]->embeds[i][j];
          }
        }
      }
      inputs.push_back( c );


      // create empty state v2
      std::vector<torch::jit::IValue> state;
      make_state(&state, param->batch_size(), &mm->meta);
      inputs.push_back( torch::ivalue::Tuple::create(state) );

    }
    else {
      // create empty state
      // TODO :: infer the rest of the state dimensions from the model
      std::vector<torch::jit::IValue> state;
      if ((param) && (mm->meta.new_state())) {
        make_state(&state, param->batch_size(), &mm->meta);
      }
      else {
        make_state_legacy(&state, param->batch_size(), &mm->meta);
      }
      inputs.push_back( torch::ivalue::Tuple::create(state) );
    }
  }

  // create empty sequnces
  std::vector<std::vector<int>> seqs;
  for (int k=0; k<batch_size; k++) {
    seqs.push_back( prompt );
  }

  int num_steps = 0;
  while (!scon[0]->finished) {
    sample_inner(scon, seqs, &mm->model, inputs, param, callbacks);
    num_steps++;
    
    // quit if we go for too long
    if ((param->max_steps() > 0) && (num_steps >= param->max_steps())) {
      terminated = true;
      break;
    }
  }

  if (debug) {
    // get tokens
    debug->tokens.push_back(seqs[0]);
    debug->orders.push_back(scon[0]->inverse_order);
    debug->modes.push_back(scon[0]->model_type);
  }

  // convert back to piece
  std::vector<midi::Piece> output(batch_size);
  if (!terminated) {
    scon[0]->enc->tokens_to_json_array(seqs, output);
    scon[0]->finalize(&output[0]); // batch size should be 1 anyways
  }
  return output;
}

}