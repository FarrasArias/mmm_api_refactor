#pragma once

#include <google/protobuf/util/json_util.h>

#include <vector>
#include <map>
#include <tuple>
#include <array>
#include <utility>

#include "../protobuf/midi.pb.h"
#include "../protobuf/util.h"
#include "../enum/token_types.h"
#include "../enum/constants.h"
#include "../enum/encoder_config.h"
#include "../enum/pretrain_group.h"

// development versions

// START OF NAMESPACE
namespace mmm {

class TokenSequence {
public:
  void push_back( int token ) {
    tokens.push_back( token );
    features.push_back( empty );
  }
  void push_back( int token, midi::ContinuousFeature feature) {
    tokens.push_back( token );
    features.push_back( feature );
  }
  void insert( std::vector<int> &tokens) {
    for (auto token : tokens) {
      push_back(token);
    }
  }
  void insert( std::vector<int> &tokens, midi::ContinuousFeature feature) {
    for (auto token : tokens) {
      push_back(token, feature);
    }
  }
  std::vector<midi::ContinuousFeature> features;
  std::vector<int> tokens;
  midi::ContinuousFeature empty;
};

std::vector<int> to_performance_duration(midi::Bar *bar, midi::Piece *p, REPRESENTATION *rep, int transpose, bool is_drum, EncoderConfig *ec) {
  std::vector<int> tokens;
  int current_step = 0;
  int current_velocity = -1;
  int N_TIME_TOKENS = rep->get_domain_size(TIME_DELTA);
  int N_DURATION_TOKENS = rep->get_domain_size(NOTE_DURATION);
  bool added_instrument = false;
  for (const auto i : bar->events()) {
    midi::Event event = p->events(i);
    if ((event.internal_duration() > 0) && (event.velocity() > 0)) {
      
      int qvel = event.velocity() > 0;
      if (rep->has_token_type(VELOCITY_LEVEL)) {
        qvel = rep->encode_partial(VELOCITY_LEVEL, event.velocity());
      }
      if (event.time() > current_step) {
        if (ec->use_absolute_time_encoding) {
          tokens.push_back( rep->encode(TIME_ABSOLUTE, event.time()) );
        }
        else { 
          while (event.time() > current_step + N_TIME_TOKENS) {
            tokens.push_back( rep->encode(TIME_DELTA, N_TIME_TOKENS-1) );
            current_step += N_TIME_TOKENS;
          }
          if (event.time() > current_step) {
            tokens.push_back( rep->encode(
              TIME_DELTA, event.time()-current_step-1) );
          }
        }
        current_step = event.time();
      }
      else if (event.time() < current_step) {
        std::cout << "current step : " << current_step << std::endl;
        std::cout << "event.time() : " << event.time() << std::endl;
        std::cout << "event.pitch() : " << event.pitch() << std::endl;
        std::cout << "event.velocity() : " << event.velocity() << std::endl;
        throw std::runtime_error("Events are not sorted!");
      }
      if (ec->use_velocity_levels) {
        if ((qvel > 0) && (qvel != current_velocity)) {
          tokens.push_back( rep->encode(VELOCITY_LEVEL, event.velocity()) );
          current_velocity = qvel;
        }
      }
      
      // instead of representing notes using onset-offset pairs
      // we have a note onset (with the pitch) and a note duration
      tokens.push_back( rep->encode(NOTE_ONSET, event.pitch() + transpose) );
      int duration = std::min(event.internal_duration(), N_DURATION_TOKENS);
      if ((!is_drum) || (ec->use_drum_offsets)) {
        tokens.push_back( rep->encode(NOTE_DURATION, duration - 1) );
      }
      
    }
  }
  return tokens;
}


std::vector<int> to_performance_dev(midi::Bar *bar, midi::Piece *p, REPRESENTATION *rep, int transpose, bool is_drum, EncoderConfig *ec) {

  if (ec->use_note_duration_encoding) {
    return to_performance_duration(bar, p, rep, transpose, is_drum, ec);
  }

  std::vector<int> tokens;
  int current_step = 0;
  int current_velocity = -1;
  int current_instrument = -1;
  int N_TIME_TOKENS = rep->get_domain_size(TIME_DELTA);
  bool added_instrument = false;
  for (const auto i : bar->events()) {
    midi::Event event = p->events(i);
    if ((!is_drum) || (event.velocity()>0) || (ec->use_drum_offsets)) {
      //int qvel = velocity_maps[rep->velocity_map_name][event.velocity()];
      int qvel = event.velocity() > 0;

      if (event.time() > current_step) {
        while (event.time() > current_step + N_TIME_TOKENS) {
          tokens.push_back( rep->encode(TIME_DELTA, N_TIME_TOKENS-1) );
          current_step += N_TIME_TOKENS;
        }
        if (event.time() > current_step) {
          tokens.push_back( rep->encode(
            TIME_DELTA, event.time()-current_step-1) );
        }
        current_step = event.time();
      }
      else if (event.time() < current_step) {
        std::cout << "current step : " << current_step << std::endl;
        std::cout << "event.time() : " << event.time() << std::endl;
        std::cout << "event.pitch() : " << event.pitch() << std::endl;
        std::cout << "event.velocity() : " << event.velocity() << std::endl;
        throw std::runtime_error("Events are not sorted!");
      }
      // if the rep contains velocity levels
      if (ec->use_velocity_levels) {
        if ((qvel > 0) && (qvel != current_velocity)) {
          tokens.push_back( rep->encode(VELOCITY_LEVEL, qvel) );
          current_velocity = qvel;
        }
        qvel = std::min(1,qvel); // flatten down to binary for note
      }
      if (qvel==0) {
        tokens.push_back( rep->encode(NOTE_OFFSET, event.pitch() + transpose) );
      }
      else {
        tokens.push_back( rep->encode(NOTE_ONSET, event.pitch() + transpose) );
      }
    }
  }
  return tokens;
}

midi::ContinuousFeature get_feature(midi::Bar &b) {
  if (b.internal_feature_size() == 0) {
    midi::ContinuousFeature f;
    return f;
  }
  return b.internal_feature(0);
}

TokenSequence to_performance_w_tracks_dev(midi::Piece *p, REPRESENTATION *rep, EncoderConfig *e) {

  // make sure each bar has feature

  TokenSequence tokens;
  int cur_transpose;
  

  bool hard_poly_dur_limits = rep->has_token_type(MIN_POLYPHONY_HARD) & rep->has_token_type(MAX_POLYPHONY_HARD) & rep->has_token_type(MIN_NOTE_DURATION_HARD) & rep->has_token_type(MAX_NOTE_DURATION_HARD);
  
  if (rep->get_domain_size(PIECE_START) == 2) {
    // here we are combining bar infill and track infill
    tokens.push_back( rep->encode(PIECE_START, (int)e->do_multi_fill) );
  }
  else {
    tokens.push_back( rep->encode(PIECE_START, 0) );
  }

  int total_bars = get_num_bars(p);

  // optional token for NUMBER OF BARS
  if (rep->has_token_type(NUM_BARS)) {
    tokens.push_back( rep->encode(NUM_BARS, total_bars) );
  }

  
  std::vector<std::vector<int>> bar_segments;
  if (e->multi_segment) {
    int NB = 4; // TODO :: alow for control of this
    for (int SB=0; SB<(int)(total_bars/NB)*NB; SB=SB+NB) {
      bar_segments.push_back( arange(SB,SB+NB) );
    }
  }
  else {
    bar_segments.push_back( arange(0,total_bars) );
  }

  for (const auto bar_segment : bar_segments) {

    // start each segment with a segment token
    if (e->multi_segment) {
      tokens.push_back( rep->encode(SEGMENT, 0) );
    }

    for (int track_num=0; track_num<p->tracks_size(); track_num++) {

      midi::Track track = p->tracks(track_num);
      midi::TrackFeatures *f = get_track_features(p, track_num);

      bool is_drum = is_drum_track( track.track_type() );
      cur_transpose = e->transpose;
      if (is_drum) {
        cur_transpose = 0;
      }

      tokens.push_back( rep->encode(TRACK, track.track_type()) );

      if (e->mark_genre) {
        // this needs an error check as it will crash if we don't have features
        
        //tokens.push_back( 
        //  rep->encode_string(GENRE, track.internal_features(0).genre_str()) );
        tokens.push_back( 
          rep->encode(GENRE, track.internal_features(0).genre_str()) );
      }
      if (e->mark_instrument) {
        // apply pre-training mapping here
        int inst = track.instrument();
        if ((e->do_pretrain_map) && (!is_drum)) {
          auto inst_itt = PRETRAIN_GROUPING_V2.find(inst);
          if (inst_itt == PRETRAIN_GROUPING_V2.end()) {
            throw std::runtime_error("COULD NOT FIND PRETRAIN GROUPING FOR INST");
          }
          inst = inst_itt->second;
        }

        tokens.push_back( rep->encode(INSTRUMENT, inst) );
      }

      // pitch limits
      if (e->mark_pitch_limits) {
        tokens.push_back( rep->encode(PITCH, f->min_pitch()) );
        tokens.push_back( rep->encode(PITCH, f->min_pitch()) );
      }

      // pitch class mask
      // we only encode the pitch classses that are present
      if (!is_drum) {
        if (rep->has_token_type(PITCH_CLASS)) {
          for (int i=0; i<12; i++) {
            if (f->pitch_classes(i)) {
              tokens.push_back( rep->encode(PITCH_CLASS,i) );
            }
          }
        }
      }

      if ((e->mark_density) || ((e->mark_drum_density && is_drum))) {
        tokens.push_back( 
          rep->encode(DENSITY_LEVEL, 
          track.internal_features(0).note_density_v2()) );
      }
      /*
      if (e->mark_note_duration) {
        tokens.push_back( 
          rep->encode_continuous(NOTE_DURATION, 
          track.internal_features(0).note_duration()) );
      }
      if (e->mark_polyphony) {
        tokens.push_back( 
          rep->encode_continuous(AV_POLYPHONY, 
          track.internal_features(0).av_polyphony()) );
      }
      */
      if ((!e->mark_drum_density) || (!is_drum)) {
        
        // newer hard limits
        if (hard_poly_dur_limits) {
          tokens.push_back(
            rep->encode(MIN_POLYPHONY_HARD, f->min_polyphony_hard()));
          tokens.push_back(
            rep->encode(MAX_POLYPHONY_HARD, f->max_polyphony_hard()));
          tokens.push_back(
            rep->encode(MIN_NOTE_DURATION_HARD, f->min_note_duration_hard()));
          tokens.push_back(
            rep->encode(MAX_NOTE_DURATION_HARD, f->max_note_duration_hard()));

          // rest percentage
          int domain_size = rep->get_domain_size(REST_PERCENTAGE);
          int rp = (int)round(f->rest_percentage() * (domain_size-1));
          tokens.push_back( rep->encode(REST_PERCENTAGE, rp) );
        }
        else {
          if (e->mark_polyphony_quantile) {
            tokens.push_back(
              rep->encode(MIN_POLYPHONY, f->min_polyphony_q()));
            tokens.push_back(
              rep->encode(MAX_POLYPHONY, f->max_polyphony_q()));
          }
          if (e->mark_note_duration_quantile) {
            tokens.push_back(
              rep->encode(MIN_NOTE_DURATION, f->min_note_duration_q()));
            tokens.push_back(
              rep->encode(MAX_NOTE_DURATION, f->max_note_duration_q()));
          }

        }
      }

      for (const auto bar_num : bar_segment) {

        if (bar_num >= total_bars) {
          throw std::runtime_error("BAR NUMBER OUT OF RANGE!");
        }

        midi::Bar bar = track.bars(bar_num);
        tokens.push_back( rep->encode(BAR, 0), get_feature(bar) );
        if (e->mark_time_sigs) {
          //int ts = rep->encode_timesig(
          //  bar.ts_numerator(), bar.ts_denominator(),e->allow_beatlength_matches);
          int ts = rep->encode(TIME_SIGNATURE, std::make_tuple(bar.ts_numerator(), bar.ts_denominator()));
          tokens.push_back(ts, get_feature(bar));
        }
        else {
          // we assume it is in 4/4 so assert that beat length is that
          if (bar.internal_beat_length() != 4) {
            std::ostringstream buffer;
            buffer << bar.internal_beat_length() << " IS NOT A SUPPORTED TIME-SIGNATURE";
            throw std::runtime_error(buffer.str());
          }
        }
        // for multi fill i should include segment_num here
        // to make masking easier ...
        if ((e->do_multi_fill) && (e->multi_fill.find(std::make_pair(track_num,bar_num)) != e->multi_fill.end())) {
          tokens.push_back( rep->encode(FILL_IN_PLACEHOLDER, 0), get_feature(bar));
        }
        else {
          std::vector<int> bar_tokens = to_performance_dev(
            &bar, p, rep, cur_transpose, is_drum, e);
          tokens.insert( bar_tokens, get_feature(bar) );
        }
        tokens.push_back( rep->encode(BAR_END, 0), get_feature(bar) );
      }
      tokens.push_back( rep->encode(TRACK_END, 0) );
    }

    // end each segment with a segment token
    if (e->multi_segment) {
      tokens.push_back( rep->encode(SEGMENT_END, 0) );
    }
  }

  if (e->do_multi_fill) {
    for (const auto track_bar : e->multi_fill) {
      int fill_track = std::get<0>(track_bar);
      int fill_bar = std::get<1>(track_bar);
      bool is_drum = is_drum_track( p->tracks(fill_track).track_type() );

      cur_transpose = e->transpose;
      if (is_drum) {
        cur_transpose = 0;
      }
      midi::Bar bar = p->tracks(fill_track).bars(fill_bar);
      tokens.push_back( rep->encode(FILL_IN_START, 0), get_feature(bar) );
      std::vector<int> bar_tokens = to_performance_dev(
        &bar, p, rep, cur_transpose, is_drum, e);
      tokens.insert( bar_tokens, get_feature(bar) );
      tokens.push_back( rep->encode(FILL_IN_END, 0) ); // end fill-in
    }
  }

  return tokens;
}

// fix the deconversion
void decode_track_dev(std::vector<int> &tokens, midi::Piece *p, REPRESENTATION *rep, EncoderConfig *ec) {
  p->set_tempo(ec->default_tempo);
  p->set_resolution(ec->resolution);

  std::map<int,int> inst_to_track;
  midi::Event *e = NULL;
  midi::Track *t = NULL;
  midi::Bar *b = NULL;
  int current_time, current_instrument, bar_start_time, current_track;
  int beat_length = 0;
  int track_count = 0;
  int bar_count = 0;
  int last_token = -1;
  int current_velocity = 100;

  std::set<int> offset_remain;

  for (const auto token : tokens) {

    //std::cout << "DECODING ... " << rep->pretty(token) << std::endl;

    if (rep->is_token_type(token, SEGMENT)) {
      track_count = 0; // reset track count
      t = NULL;
      b = NULL;
    }
    if (rep->is_token_type(token, TRACK)) {
      current_time = 0; // restart the time
      current_instrument = 0; // reset instrument
      offset_remain.clear();
      if (track_count >= p->tracks_size()) {
        t = p->add_tracks();
      }
      else {
        t = p->mutable_tracks(track_count);
      }
      //t->set_is_drum( is_drum_track(rep->decode(token)) );
      t->set_track_type( (midi::TRACK_TYPE)rep->decode(token) );
    }
    else if (rep->is_token_type(token, TRACK_END)) {
      track_count++;
      t = NULL;
    }
    else if (rep->is_token_type(token, BAR)) {
      // when we start new bar we need to decrement time of remaining offsets
      for (const auto index : offset_remain) {
        midi::Event *e = p->mutable_events(index);
        e->set_time( e->time() - beat_length * p->resolution() );
      }
      current_time = 0; // restart the time
      beat_length = 4; // default value optionally overidden with TIME_SIGNATURE
      if ((!ec->interleaved) && (t)) {
        b = t->add_bars();
      }
      bar_count++;
    }
    else if (rep->is_token_type(token, TIME_SIGNATURE)) {
      std::tuple<int,int> ts = rep->decode_timesig(token);
      beat_length = 4 * std::get<0>(ts) / std::get<1>(ts);
    }
    else if (rep->is_token_type(token, BAR_END)) {
      if (b) {
        b->set_internal_beat_length(beat_length);
      }
      current_time = beat_length * p->resolution();
      //b = NULL;
    }
    else if (rep->is_token_type(token, TIME_DELTA)) {
      current_time += (rep->decode(token) + 1);
    }
    else if (rep->is_token_type(token, TIME_ABSOLUTE)) {
      current_time = rep->decode(token); // simply update instead of increment
    }
    else if (rep->is_token_type(token, INSTRUMENT)) {
        
      // if we are in track interleaved mode
      // we need to retrive track from instrument
      if (ec->interleaved) {
        current_instrument = rep->decode(token);
        auto it = inst_to_track.find( current_instrument );
        if (it != inst_to_track.end()) {
          t = p->mutable_tracks(it->second);
        }
        else {
          inst_to_track[current_instrument] = track_count;
          t = p->add_tracks();
          t->set_instrument( current_instrument % 128 );
          if (current_instrument >= 128) {
            //t->set_is_drum( true );
            t->set_track_type( midi::TRACK_TYPE::STANDARD_DRUM_TRACK );
          }
          else {
            //t->set_is_drum( false );
            t->set_track_type( midi::TRACK_TYPE::STANDARD_TRACK );
          }
          track_count++;
        }
        // add bars and get current bar
        int curr_bars = t->bars_size();
        for (int n=curr_bars; n<bar_count; n++) {
          b = t->add_bars();
          b->set_internal_beat_length( 4 ); // set to default
        }
        b = t->mutable_bars(bar_count-1); // make sure to get right bar
      }
      else if (t) {
        current_instrument = rep->decode(token);
        t->set_instrument( current_instrument );
      }
    }
    else if (rep->is_token_type(token, VELOCITY_LEVEL)) {
      current_velocity = rep->decode(token);
    }
    else if (rep->is_token_type(token, NOTE_ONSET) || rep->is_token_type(token, NOTE_OFFSET)) {
      if (b && t) {
        
        if (!ec->use_note_duration_encoding) {
          int current_note_index = p->events_size();
          e = p->add_events();
          e->set_pitch( rep->decode(token) );
          e->set_velocity( current_velocity );
          if (rep->is_token_type(token, NOTE_OFFSET)) {
            e->set_velocity( 0 );
          }
          e->set_time( current_time );
          b->add_events( current_note_index );
          b->set_internal_has_notes( true );
        }
        else if ((ec->use_note_duration_encoding) && (!ec->use_drum_offsets) && (is_drum_track(t->track_type()))) {
          
          int current_note_index = p->events_size();
          e = p->add_events();
          e->set_pitch( rep->decode(token) );
          e->set_velocity( current_velocity );
          e->set_time( current_time );
          b->add_events( current_note_index );
          b->set_internal_has_notes( true );

          current_note_index = p->events_size();
          e = p->add_events();
          e->set_pitch( rep->decode(token) );
          e->set_velocity( 0 );
          e->set_time( current_time + 1 );
          b->add_events( current_note_index );
          b->set_internal_has_notes( true );

        }
      }
    }
    else if (rep->is_token_type(token, NOTE_DURATION)) {
      if (b && t && (last_token >= 0) && (rep->is_token_type(last_token, NOTE_ONSET))) {

        // add onset
        int current_note_index = p->events_size();
        e = p->add_events();
        e->set_pitch( rep->decode(last_token) );
        e->set_velocity( current_velocity );
        e->set_time( current_time );
        b->add_events( current_note_index );

        // add offset
        current_note_index = p->events_size();
        e = p->add_events();
        e->set_pitch( rep->decode(last_token) );
        e->set_velocity( 0 );
        e->set_time( current_time + rep->decode(token) + 1 );

        if (e->time() <= beat_length * p->resolution()) {
          b->add_events( current_note_index );
        }
        else {
          // we need to add this to a later bar
          offset_remain.insert( current_note_index );
        }

        b->set_internal_has_notes( true );
      }
    }
    else if (rep->is_token_type(token, GENRE)) {
      midi::TrackFeatures *f;
      if (!t->internal_features_size()) {
        f = t->add_internal_features(); 
      }
      else {
        f = t->mutable_internal_features(0);
      }
      f->set_genre_str( rep->decode_string(token) );
    }

    // insert offsets from note_duration tokens when possible
    std::vector<int> to_remove;
    for (const auto index : offset_remain) {
      if (p->events(index).time() <= current_time) {
        b->add_events( index );
        to_remove.push_back( index );
      }
    }
    for (const auto index : to_remove) {
      offset_remain.erase(index);
    }

    last_token = token;
  }
  p->add_internal_valid_segments(0);
  p->add_internal_valid_tracks((1<<p->tracks_size())-1);

  // add extra bars if needed ...
  if (ec->interleaved) {
    for (int track_num=0; track_num<p->tracks_size(); track_num++) {
      t = p->mutable_tracks(track_num);
      int curr_bars = t->bars_size();
      for (int n=curr_bars; n<bar_count; n++) {
        t->add_bars();
      }
    }
  }

  // for debug
  /*
  int track_num = 0;
  for (const auto track : p->tracks()) {
    int bar_num = 0;
    for (const auto bar : track.bars()) {
      std::cout << "TRACK " << track_num << " BAR " << bar_num << " " << bar.events_size() << std::endl;
      bar_num++;
    }
    track_num++;
  }
  */
  
  // update note density
  update_note_density(p);
}

std::vector<int> to_interleaved_performance_inner(std::vector<midi::Event> &events, REPRESENTATION *rep, EncoderConfig *ec) {
  std::vector<int> tokens;
  int current_step = 0;
  int current_velocity = -1;
  int current_instrument = -1;
  int N_TIME_TOKENS = rep->get_domain_size(TIME_DELTA);
  bool added_instrument = false;
  for (const auto event : events) {
    bool is_drum = is_drum_track(event.internal_track_type());
    int current_transpose = ec->transpose * (!is_drum);
    if ((!is_drum) || (event.velocity()>0) || (ec->use_drum_offsets)) {
      int qvel = event.velocity() > 0;
      if (event.time() > current_step) {
        while (event.time() > current_step + N_TIME_TOKENS) {
          tokens.push_back( rep->encode(TIME_DELTA, N_TIME_TOKENS-1) );
          current_step += N_TIME_TOKENS;
        }
        if (event.time() > current_step) {
          tokens.push_back( rep->encode(
            TIME_DELTA, event.time()-current_step-1) );
        }
        current_step = event.time();
      }
      else if (event.time() < current_step) {
        throw std::runtime_error("Events are not sorted!");
      }
      if (event.internal_instrument() != current_instrument) {
        tokens.push_back( rep->encode(INSTRUMENT, event.internal_instrument()) );
        current_instrument = event.internal_instrument();
      }
      if (ec->use_velocity_levels) {
        if ((qvel > 0) && (qvel != current_velocity)) {
          tokens.push_back( rep->encode(VELOCITY_LEVEL, qvel) );
          current_velocity = qvel;
        }
        qvel = std::min(1,qvel); // flatten down to binary for note
      }
      if (qvel==0) {
        tokens.push_back( 
          rep->encode(NOTE_OFFSET, event.pitch() + current_transpose) );
      }
      else {
        tokens.push_back( 
          rep->encode(NOTE_ONSET, event.pitch() + current_transpose) );
      }
    }
  }
  return tokens;
}

bool sort_events_winst(const midi::Event a, const midi::Event b) { 
  if (a.time() != b.time()) {
    return a.time() < b.time();
  }
  if (std::min(a.velocity(),1) != std::min(b.velocity(),1)) {
    return std::min(a.velocity(),1) < std::min(b.velocity(),1);
  }
  if (a.internal_instrument() != b.internal_instrument()) {
    return a.internal_instrument() < b.internal_instrument();
  }
  return a.pitch() < b.pitch();
}

std::vector<int> to_interleaved_performance(midi::Piece *p, REPRESENTATION *rep, EncoderConfig *e) {

  std::vector<int> tokens;

  tokens.push_back( rep->encode(PIECE_START, 0) );

  // TODO : include a header here
  // to control the instruments in sampling
  if (rep->get_domain_size(HEADER) == 2) {
    tokens.push_back( rep->encode(HEADER,0) );
    std::set<int> insts;
    for (const auto track : p->tracks()) {
      insts.insert( track.instrument() + 128*is_drum_track(track.track_type()) );
    }
    for (const auto inst : insts) {
      tokens.push_back( rep->encode(INSTRUMENT,inst) );
    }
    tokens.push_back( rep->encode(HEADER,1) );
  }

  int num_bars = get_num_bars(p);
  for (int bar_num=0; bar_num<num_bars; bar_num++) {
    std::vector<midi::Event> events;
    for (int track_num=0; track_num<p->tracks_size(); track_num++) {
      int track_type = p->tracks(track_num).track_type();
      bool is_drum = is_drum_track(track_type); 
      for (const auto index : p->tracks(track_num).bars(bar_num).events()) {
        midi::Event e;
        e.CopyFrom( p->events(index) );
        e.set_internal_instrument( p->tracks(track_num).instrument() + 128*is_drum );
        e.set_internal_track_type( track_type );
        events.push_back( e );
      }
    }

    // sort the events by time, onset/offset, track_type, instrument, pitch
    std::sort(events.begin(), events.end(), sort_events_winst);

    // make bar
    tokens.push_back( rep->encode(BAR, 0) );
    std::vector<int> bar_tokens = to_interleaved_performance_inner(events, rep, e);
    tokens.insert(tokens.end(), bar_tokens.begin(), bar_tokens.end());
    tokens.push_back( rep->encode(BAR_END, 0) );
  }
  return tokens;
}


// ================================================================
// ================================================================
// ================================================================
// DECODING HELPERS

std::vector<int> strip_header(std::vector<int> &tokens, REPRESENTATION *rep) {
  std::vector<int> fixed;
  bool in_header = false;
  for (const auto token : tokens) {
    if (rep->is_token_type(token, HEADER)) {
      in_header = !in_header;
    }
    else if (!in_header) {
      fixed.push_back( token );
    }
  }
  std::cout << fixed.size() << " " << tokens.size() << std::endl;
  return fixed;
}

}
// END OF NAMESPACE