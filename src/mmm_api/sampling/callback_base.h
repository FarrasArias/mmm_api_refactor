#pragma once

#include "../encoder/encoder_base.h"

namespace mmm {

  // this can bs used as a base class for callbacks
  class CallbackManager {
  public:
    CallbackManager () { 
      progress_count = 0;
      generated_bar_count = 0;
    }
    ~CallbackManager () { }
    void set_generated_bar_count(int x) {
      generated_bar_count = x;
    }
    void update(std::unique_ptr<ENCODER> *encoder, int token) {

      if (generated_bar_count == 0) {
        throw std::runtime_error("GENERATED BAR COUNT WAS NEVER SPECIFIED");
      }
      
      if ((!encoder) || (!encoder->get())) {
        throw std::runtime_error("CALLBACK MANAGER RECIEVED NULL ENCODER");
      }

      REPRESENTATION *rep = encoder->get()->rep;
      if ((rep->is_token_type(token, BAR_END)) || (rep->is_token_type(token, FILL_IN_END))) {
        progress_count++;
        on_bar_end( float(progress_count) / generated_bar_count );
      }

    }
    virtual void on_bar_end (float) = 0;

    int generated_bar_count;
    int progress_count;
  };

}