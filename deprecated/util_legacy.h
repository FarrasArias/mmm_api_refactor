
// make code for bar-major
// and track-major representations
// improved encoding for representations
// =======================================================================

void get_random_segment(midi::Piece *p, std::vector<int> &track_nums, std::vector<int> &bar_nums, EncoderConfig *e) {
  // select the section and the tracks

  if (e->seed >= 0) {
    srand(e->seed);
  }

  if (p->valid_segments_size() == 0) {
    std::cout << "WARNING : no valid segments" << std::endl;
    return; // track_nums and bar_nums are empty so nothing will happen
  }

  int index = e->segment_idx;
  if (index == -1) {
    index = rand();
  }
  index = index % p->valid_segments_size();
  
  for (int i=0; i<32; i++) {
    if (p->valid_tracks(index) & (1<<i)) {
      track_nums.push_back(i);
    }
  }

  int start_bar = p->valid_segments(index);
  for (int i=start_bar; i<start_bar+e->num_bars; i++) {
    bar_nums.push_back(i);
  }

  if (e->do_track_shuffle) {
    random_shuffle(track_nums.begin(), track_nums.end(), RNG());
  }
  int ntracks = std::min((int)track_nums.size(), e->max_tracks); // limit tracks
  track_nums.resize(ntracks); // remove extra tracks
}

void get_fill(midi::Piece *p, std::vector<int> &track_nums, std::vector<int> &bar_nums, EncoderConfig *e) {
  if ((e->do_multi_fill) && (e->multi_fill.size() == 0)) {
    assert(e->fill_percentage > 0);

    // pick a random percentange up to fill percentage and fill those
    int n_fill = rand() % (int)round(
      track_nums.size() * bar_nums.size() * e->fill_percentage);
    std::vector<std::tuple<int,int>> choices;
    for (const auto track_num : track_nums) {
      for (const auto bar_num : bar_nums) {
        choices.push_back(std::make_pair(track_num,bar_num));
      }
    }
    random_shuffle(choices.begin(), choices.end(), RNG());
    for (int i=0; i<n_fill; i++) {
      e->multi_fill.insert(choices[i]);
    }
  }
}

void piece_header(midi::Piece *p, REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {

  tokens.push_back( rep->encode({{PIECE_START,0}}) );

  // GENRE HEADER
  // NOTE : data must be formated to always have two fields
  if (e->genre_header) {
    if (e->genre_tags.size()) {
      for (const auto tag : e->genre_tags) {
        tokens.push_back( rep->encode({{GENRE,genre_maps["msd_cd2"][tag]}}) );
      }
    }
    else {
      for (const auto tag : p->msd_cd1()) {
        tokens.push_back( rep->encode({{GENRE,genre_maps["msd_cd2"][tag]}}) );
      }
    }
  }

  // INSTRUMENT HEADER
  /*
  if (e->instrument_header) {
    tokens.push_back( rep->encode({{HEADER,0}}) );
    for (int i=0; i<(int)vtracks.size(); i++) {
      int track = get_track_type(p,e,vtracks[i]);
      tokens.push_back( rep->encode({{TRACK,track}}) );
      int inst = p->tracks(vtracks[i]).instrument();
      tokens.push_back( rep->encode({{INSTRUMENT,inst}}) );
    }
    tokens.push_back( rep->encode({{HEADER,1}}) );
  }
  */
}

void segment_header(REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  tokens.push_back( rep->encode({{SEGMENT,0}}) );
}

void segment_footer(REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  tokens.push_back( rep->encode({{SEGMENT_END,0}}) );
}

void track_header(const midi::Track *track, REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  // determine track type
  int track_type = 0;
  if (track->is_drum()) { 
    track_type = 1;
  }
  else if (e->mark_polyphony) {
    if (track->av_polyphony() < 1.1) { 
      track_type = 0; 
    }
    else {
      track_type = 2; 
    }
  }
  tokens.push_back( rep->encode({{TRACK,track_type}}) );
  // specify instrument for track
  if (e->force_instrument) {
    int inst = track->instrument();
    tokens.push_back( rep->encode({{INSTRUMENT,inst}}) );
  }
  // specify average density for track
  if (e->mark_density) {
    int density_level = track->note_density_v2();
    tokens.push_back( rep->encode({{DENSITY_LEVEL,density_level}}) );
  }
}

void track_footer(const midi::Track *track, REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  tokens.push_back( rep->encode({{TRACK_END,0}}) );
}

void bar_header(const midi::Bar *bar, REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  tokens.push_back( rep->encode({{BAR,0}}) );
  if (e->mark_time_sigs) {
    int ts = time_sig_map[round(bar->internal_beat_length())];
    tokens.push_back( rep->encode({{TIME_SIGNATURE,ts}}) );
  }
}

void bar_footer(const midi::Bar *bar, REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  tokens.push_back( rep->encode({{BAR_END,0}}) );
}

void encode_bar(midi::Piece *p, int track_num, int bar_num, REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  /*
  if ((e->do_fill) && (fill_track==i) && (fill_bar==j)) {
    tokens.push_back( rep->encode({{FILL_IN,0}}) );
  }
  */
  if ((e->do_multi_fill) && (e->multi_fill.find(std::make_pair(track_num,bar_num)) != e->multi_fill.end())) {
    tokens.push_back( rep->encode({{FILL_IN,0}}) );
  }
  else {
    midi::Bar bar = p->tracks(track_num).bars(bar_num);
    int cur_transpose = e->transpose;
    if (p->tracks(track_num).is_drum()) {
      cur_transpose = 0;
    }
    std::vector<int> bar_tokens = to_performance(&bar, p, rep, cur_transpose, e);
    tokens.insert(tokens.end(), bar_tokens.begin(), bar_tokens.end());
  }
}

void fill_footer(midi::Piece *p, REPRESENTATION *rep, EncoderConfig *e, std::vector<int> &tokens) {
  if (e->do_multi_fill) {
    for (const auto track_bar : e->multi_fill) {
      int fill_track = std::get<0>(track_bar);
      int fill_bar = std::get<1>(track_bar);

      midi::Bar bar = p->tracks(fill_track).bars(fill_bar);
      int cur_transpose = e->transpose;
      if (p->tracks(fill_track).is_drum()) {
        cur_transpose = 0;
      }
      tokens.push_back( rep->encode({{FILL_IN,1}}) ); // begin fill-in
      std::vector<int> bar_tokens = to_performance(
        &bar, p, rep, cur_transpose, e);
      tokens.insert(tokens.end(), bar_tokens.begin(), bar_tokens.end());
      tokens.push_back( rep->encode({{FILL_IN,2}}) ); // end fill-in
    }
  }
}

std::vector<int> main_enc(midi::Piece *p, REPRESENTATION *rep, EncoderConfig *e) {

  std::vector<int> tokens;
  
  // determine the track_nums and bar_nums
  std::vector<int> track_nums;
  std::vector<int> bar_nums;
  get_random_segment(p, track_nums, bar_nums, e);
  
  std::vector<std::vector<int>> split_bar_nums(4,std::vector<int>());
  int local_idx, context_tracks;
  if (e->segment_mode) {
    local_idx = rand() % 4;
    context_tracks = track_nums.size() - (rand() % (track_nums.size()/2));
    for (int i=0; i<bar_nums.size(); i++) {
      split_bar_nums[i/4].push_back(bar_nums[i]);
    }
    get_fill(p, track_nums, split_bar_nums[local_idx], e);
  }
  get_fill(p, track_nums, bar_nums, e);

  if (track_nums.size() && bar_nums.size()) {
    if (e->piece_header) {
      piece_header(p, rep, e, tokens); // in either case we need piece header
    }
    if (e->segment_mode) {
      bool multi_fill_status = e->do_multi_fill;
      e->do_multi_fill = false;
      for (int seg_num=0; seg_num<4; seg_num++) {
        segment_header(rep, e, tokens);
        if (seg_num != local_idx) {
          int track_count = 0;
          for (const int track_num : track_nums) {
            const midi::Track track = p->tracks(track_num);
            track_header(&track, rep, e, tokens);
            for (int bar_num : split_bar_nums[seg_num]) {
              const midi::Bar bar = track.bars(bar_num);
              bar_header(&bar, rep, e, tokens);
              encode_bar(p, track_num, bar_num, rep, e, tokens);
              bar_footer(&bar, rep, e, tokens);
            }
            track_footer(&track, rep, e, tokens);
            track_count++;
            if ((track_count) >= context_tracks) {
              break; // no more tracks
            }
          }
        }
        else {
          tokens.push_back( rep->encode({{SEGMENT_FILL_IN,0}}) );
        }
        segment_footer(rep, e, tokens);
      }
      e->do_multi_fill = multi_fill_status;

      // fill in remaining segment
      tokens.push_back( rep->encode({{SEGMENT_FILL_IN,1}}) );

      for (const int track_num : track_nums) {
        const midi::Track track = p->tracks(track_num);
        track_header(&track, rep, e, tokens);
        for (int bar_num : bar_nums) {
          const midi::Bar bar = track.bars(bar_num);
          bar_header(&bar, rep, e, tokens);
          encode_bar(p, track_num, bar_num, rep, e, tokens);
          bar_footer(&bar, rep, e, tokens);
        }
        track_footer(&track, rep, e, tokens);
      }
      fill_footer(p, rep, e, tokens); // for bar fill in ...
      tokens.push_back( rep->encode({{SEGMENT_FILL_IN,2}}) );
    }
    else if (!e->bar_major) {
      for (const int track_num : track_nums) {
        const midi::Track track = p->tracks(track_num);
        track_header(&track, rep, e, tokens);
        for (int bar_num : bar_nums) {
          const midi::Bar bar = track.bars(bar_num);
          bar_header(&bar, rep, e, tokens);
          encode_bar(p, track_num, bar_num, rep, e, tokens);
          bar_footer(&bar, rep, e, tokens);
        }
        track_footer(&track, rep, e, tokens);
      }
    }
    else {
      for (const int bar_num : bar_nums) {
        const midi::Bar bar = p->tracks(track_nums[0]).bars(bar_num);
        bar_header(&bar, rep, e, tokens);
        for (const int track_num : track_nums) {
          const midi::Track track = p->tracks(track_num);
          track_header(&track, rep, e, tokens);
          encode_bar(p, track_num, bar_num, rep, e, tokens);
          track_footer(&track, rep, e, tokens);
        }
        bar_footer(&bar, rep, e, tokens);
      }
    }
    //fill_footer(p, rep, e, tokens);
  }
  return tokens;
}

void main_dec(std::vector<int> &tokens, midi::Piece *p, REPRESENTATION *rep, EncoderConfig *ec) {
  p->set_tempo(ec->default_tempo);
  p->set_resolution(ec->resolution);

  midi::Event *e = NULL;
  midi::Track *t = NULL;
  midi::Bar *b = NULL;
  int curr_time, current_instrument, current_track, current_velocity, beat_length;
  int bar_time = 0;
  int track_count = 0;
  int bar_count = 0;

  // this is flexible enough to handle bar-major or track-major

  for (const auto token : tokens) {
    if (rep->is_token_type(token, TRACK)) {
      if (ec->bar_major) {
        curr_time = bar_time;
      }
      else {
        // track major
        bar_time = 0; // restart bar_time each track
        bar_count = 0; // reset bar_count each track
        curr_time = 0; // restart track_time
      }
      current_instrument = 0; // reset instrument once per track
      current_track = SAFE_TRACK_MAP[track_count];

      // add or get track
      if (track_count >= p->tracks_size()) {
        t = p->add_tracks();
      }
      else {
        t = p->mutable_tracks(track_count);
      }
      if (rep->decode(token,TRACK)==1) {
        current_track = 9; // its a drum channel
        t->set_is_drum(true); // by default its false
      }
      else {
        t->set_is_drum(false);
      }
    }
    else if (rep->is_token_type(token, TRACK_END)) {
      track_count++;
    }
    else if (rep->is_token_type(token, BAR)) {
      if (ec->bar_major) {
        track_count = 0; // reset track_count each bar
      }
      curr_time = bar_time; // can set this either way
      beat_length = 4; // default value optionally overidden with TIME_SIGNATURE
    }
    else if (rep->is_token_type(token, TIME_SIGNATURE)) {
      beat_length = rev_time_sig_map[rep->decode(token, TIME_SIGNATURE)];
    }
    else if (rep->is_token_type(token, BAR_END)) {
      bar_time += (ec->resolution * beat_length);
      bar_count++;
    }
    else if (rep->is_token_type(token, TIME_DELTA)) {
      curr_time += (rep->decode(token, TIME_DELTA) + 1);
    }
    else if (rep->is_token_type(token, INSTRUMENT)) {
      current_instrument = rep->decode(token, INSTRUMENT);
      t->set_instrument( current_instrument );
    }
    else if (rep->is_token_type(token, VELOCITY_LEVEL)) {
      current_velocity = rep->decode(token, VELOCITY_LEVEL);
    }
    else if (rep->is_token_type(token, PITCH)) {

      int current_note_index = p->events_size();
      e = p->add_events();
      e->set_pitch( rep->decode(token, PITCH) );
      if ((!ec->use_velocity_levels) || (rep->decode(token, VELOCITY)==0)) {
        e->set_velocity( rep->decode(token, VELOCITY) );
      }
      else {
        e->set_velocity( current_velocity );
      }
      e->set_time( curr_time );
      e->set_qtime( curr_time - bar_time );
      e->set_instrument( current_instrument );
      e->set_track( current_track );
      e->set_is_drum( current_track == 9 );

      // get or add bar
      if (bar_count >= t->bars_size()) {
        b = t->add_bars();
      }
      else {
        b = t->mutable_bars(bar_count);
      }
      b->set_is_four_four(true);
      b->set_internal_has_notes(false); // no notes yet
      b->set_time( bar_time );


      b->add_events(current_note_index);
      b->set_internal_has_notes(true);
    }
  }
  p->add_valid_segments(0);
  p->add_valid_tracks((1<<p->tracks_size())-1);
}

// ====================================================================


/*
std::vector<int> to_performance_w_tracks_and_bars(midi::Piece *p, REPRESENTATION *rep, int transpose, bool ordered, int n_to_fill) {

  // allow for random order of tracks
  // and random order of bars
  int n_bars = 4;
  //srand(time(NULL));

  std::vector<int> tokens;
  if (p->valid_segments_size() == 0) {
    std::cout << "WARNING : no valid segments" << std::endl;
    return tokens; // return empty sequence
  }

  int ii = rand() % p->valid_segments_size();
  int start = p->valid_segments(ii);

  // select subset of valid tracks
  std::vector<int> vtracks;
  for (int i=0; i<32; i++) {
    if (p->valid_tracks(ii) & (1<<i)) {
      vtracks.push_back(i);
    }
  }

  random_shuffle(vtracks.begin(), vtracks.end());
  int ntracks = std::min((int)vtracks.size(), 12); // no more than 12 tracks
  vtracks.resize(ntracks); // remove extra tracks

  // random order of tracks and bars ...
  std::vector<std::pair<int,int>> track_bar_order;
  for (int i=0; i<(int)vtracks.size(); i++) {
    for (int j=0; j<n_bars; j++) {
      track_bar_order.push_back( std::make_pair(i,j) );
    }
  }
  random_shuffle(track_bar_order.begin(), track_bar_order.end());

  int split = 0;
  if (ordered) {
    if (n_to_fill == 0) {
      split = rand() % track_bar_order.size();
    }
    else {
      split = (int)track_bar_order.size() - n_to_fill;
    }
    std::sort(track_bar_order.begin(), track_bar_order.begin() + split);
    std::sort(track_bar_order.begin() + split, track_bar_order.end());
  }

  int count = 0;
  int track, bar, cur_transpose;
  tokens.push_back( rep->encode({{PIECE_START,0}}) );
  for (const auto track_bar : track_bar_order) {
    track = std::get<0>(track_bar);
    bar = std::get<1>(track_bar);
    cur_transpose = transpose;

    if ((ordered) && (count == split)) {
      tokens.push_back( rep->encode({{FILL_IN,0}}) );
    }

    tokens.push_back( rep->encode({{TRACK,track}}) );
    if (p->tracks(vtracks[track]).is_drum()) { 
      tokens.push_back( rep->encode({{DRUM_TRACK,0}}) );
      cur_transpose = 0; // // AVOID DRUM TRANSPOSTION
    }
    tokens.push_back( rep->encode({{BAR,bar}}) );
    midi::Bar b = p->tracks(vtracks[track]).bars(start+bar);
    std::vector<int> bar = to_performance(&b, p, rep, cur_transpose);
    tokens.insert(tokens.end(), bar.begin(), bar.end());
    tokens.push_back( rep->encode({{BAR_END,0}}) );
    count++;
  }
  return tokens;
}
*/

/*
midi::Piece *decode_track_bar(REPRESENTATION *rep, std::vector<int> &tokens) {
  midi::Piece *p = new midi::Piece;
  p->set_tempo(104);
  p->set_resolution(12);

  midi::Event *e = NULL;
  int current_time, current_instrument, current_track, current_bar;
  std::vector<int> track_map = {0,1,2,3,4,5,6,7,8,10,11,12,13,14,15};

  for (const auto token : tokens) {
    if (rep->is_token_type(token, TRACK)) {
      current_time = 0; // restart the time
      current_instrument = 0; // reset instrument
      // make sure to avoid drum channel
      current_track = track_map[rep->decode(token, TRACK)];      
    }
    if (rep->is_token_type(token, DRUM_TRACK)) {
      current_track = 9; // switch to drum channel
    }
    else if (rep->is_token_type(token, BAR)) {
      current_bar = rep->decode(token, BAR);
      current_time = 48 * current_bar;
    }
    else if (rep->is_token_type(token, TIME_DELTA)) {
      current_time += (rep->decode(token, TIME_DELTA) + 1);
    }
    else if (rep->is_token_type(token, INSTRUMENT)) {
      current_instrument = rep->decode(token, INSTRUMENT);
    }
    else if (rep->is_token_type(token, PITCH)) {
      e = p->add_events();
      e->set_pitch( rep->decode(token, PITCH) );
      e->set_velocity( rep->decode(token, VELOCITY) );
      e->set_time( current_time );
      e->set_instrument( current_instrument );
      e->set_track( current_track );
      e->set_is_drum( current_track == 9 );
    }
  }
  return p;
}
*/