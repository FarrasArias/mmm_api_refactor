#pragma once

#include <sstream>
#include "../enum/gm.h"
#include "midi.pb.h"

// START OF NAMESPACE
namespace mmm {

template <typename T>
void validate_protobuf_inner(const T &x, bool ignore_internal) {
  
  const google::protobuf::Reflection* reflection = x.GetReflection();
  const google::protobuf::Descriptor* descriptor = x.GetDescriptor();
  
  for (int i=0; i<descriptor->field_count(); i++) {
    const google::protobuf::FieldDescriptor *fd = descriptor->field(i);
    const google::protobuf::FieldOptions opt = fd->options();
    google::protobuf::FieldDescriptor::Type ft = fd->type();
    
    if ((fd->name().rfind("internal_", 0)) || (!ignore_internal)) {

      bool is_repeated = fd->is_repeated();
      int field_count = 1;
      if (is_repeated) {
        field_count = reflection->FieldSize(x, fd);
      }

      for (int index=0; index<field_count; index++) {

        //std::cout << "checking " << fd->name() << " at index " << index << std::endl;

        if (ft == google::protobuf::FieldDescriptor::Type::TYPE_FLOAT) {
          float minval = opt.GetExtension(midi::fminval);
          float maxval = opt.GetExtension(midi::fmaxval);
          float value;
          if (is_repeated) {
            value = reflection->GetRepeatedFloat(x,fd,index);
          }
          else {
             value = reflection->GetFloat(x,fd);
          }
          if ((value < minval) || (value > maxval)) {
            std::ostringstream buffer;
            buffer << "PROTOBUF ERROR : " << fd->name() << " not on range [" << minval << "," << maxval << ").";
            throw std::invalid_argument(buffer.str());
          }
        }
        else if (ft == google::protobuf::FieldDescriptor::Type::TYPE_INT32) {
          int minval = opt.GetExtension(midi::minval);
          int maxval = opt.GetExtension(midi::maxval);
          int value;
          if (is_repeated) {
            value = reflection->GetRepeatedInt32(x,fd,index);
          }
          else {
            value = reflection->GetInt32(x,fd);
          }
          if ((value < minval) || (value > maxval)) {
            std::ostringstream buffer;
            buffer << "PROTOBUF ERROR : " << fd->name() << " not on range [" << minval << "," << maxval << ").";
            throw std::invalid_argument(buffer.str());
          }
        }
        else if (ft == google::protobuf::FieldDescriptor::Type::TYPE_MESSAGE) {
          if (is_repeated) {
            validate_protobuf_inner(
              reflection->GetRepeatedMessage(x,fd,index), ignore_internal);
          }
          else {
            validate_protobuf_inner(
              reflection->GetMessage(x,fd), ignore_internal);
          }
        }
        /*
        else if (ft == google::protobuf::FieldDescriptor::Type::TYPE_ENUM) {
          // NOTE :: PROTOBUF CRASHES IF YOU TRY TO ADD ENUM OUT OF RANGE ANYWAYS
          const google::protobuf::EnumDescriptor *ed = fd->enum_type();
          int value = reflection->GetEnumValue(x,fd);
          if (ed->FindValueByNumber(value) == NULL) {
            std::ostringstream buffer;
            buffer << "PROTOBUF ERROR : " << fd->name() << " not member of enum";
            throw std::invalid_argument(buffer.str());
          }
        }
        */
      }
    }
  }
}


// this function is validating the range of each variable
// if the field has a min and max defined
template <typename T>
void validate_protobuf_field_ranges(T *x, bool ignore_internal=true) {
  validate_protobuf_inner(*x, ignore_internal);
}

bool operator< (const midi::Event &a, const midi::Event &b) {
  if (a.time() != b.time()) {
    return a.time() < b.time();
  }
  if (std::min(a.velocity(),1) != std::min(b.velocity(),1)) {
    return std::min(a.velocity(),1) < std::min(b.velocity(),1);
  }
  return a.pitch() < b.pitch();
}

void sort_piece_events(midi::Piece *p) {
  
  // find the re-indexing of the events using argsort
  std::vector<int> idx = arange(p->events_size());
  std::sort(idx.begin(), idx.end(),
    [&p](size_t i, size_t j) {return p->events(i) < p->events(j);});
  
  // make a map from old-index to new-index
  std::map<int,int> index_map;
  int count = 0;
  for (const auto i : idx) {
    index_map.insert(std::make_pair(i,count));
    count++;
  }

  // replace the events in piece->events
  midi::Piece orig(*p);
  p->clear_events();
  for (const auto i : idx) {
    midi::Event *e = p->add_events();
    e->CopyFrom(orig.events(i));
  }

  // replace indices in piece->tracks
  for (int track_num=0; track_num<p->tracks_size(); track_num++) {
    midi::Track *t = p->mutable_tracks(track_num);
    for (int bar_num=0; bar_num<t->bars_size(); bar_num++) {
      midi::Bar *b = t->mutable_bars(bar_num);
      b->clear_events();
      std::vector<int> bar_events;
      for (const auto e : orig.tracks(track_num).bars(bar_num).events()) {
        bar_events.push_back( index_map[e] );
      }
      std::sort(bar_events.begin(), bar_events.end());
      for (const auto e : bar_events) {
        b->add_events( e );
      }
    }
  }
}

// 1. check that each event is within the bar
void validate_events(midi::Piece *p) {
  for (const auto track : p->tracks()) {
    for (const auto bar : track.bars()) {
      int barlength = bar.internal_beat_length() * p->resolution();
      for (const auto index : bar.events()) {
        if ((index < 0) || (index >= p->events_size())) {
          throw std::invalid_argument("EVENT INDEX IN BAR IS OUT OF RANGE!");
        }
        int time = p->events(index).time();
        bool is_onset = (p->events(index).velocity()>0);
        if ((time < 0) || ((time >= barlength) && (is_onset)) || ((time > barlength) && (!is_onset))) {
          std::string event_type = "ONSET";
          if (!is_onset) {
            event_type = "OFFSET";
          }
          std::ostringstream buffer;
          buffer << "NOTE " << event_type << " TIME (" << time << ") IS BEYOND EXTENTS OF BAR (" << barlength << ")"; 
          throw std::invalid_argument(buffer.str());
        }
      }
    }
  }
}

void check_track_lengths(midi::Piece *x) {
  int num_tracks = x->tracks_size();
  if (num_tracks > 0) {
    int num_bars = x->tracks(0).bars_size();
    for (int track_num=1; track_num<num_tracks; track_num++) {
      if (num_bars != x->tracks(track_num).bars_size()) {
        throw std::invalid_argument("NUMBER OF BARS DIFFERS BETWEEN TRACKS!");
      }
    }
  }
}

void check_time_sigs(midi::Piece *x) {
  int track_num = 0;
  std::vector<int> numerators;
  std::vector<int> denominators;
  for (const auto track : x->tracks()) {
    int bar_num = 0;
    for (const auto bar : track.bars()) {
      if (track_num == 0) {
        numerators.push_back( bar.ts_numerator() );
        denominators.push_back( bar.ts_denominator() );
      }
      else {
        if ((numerators[bar_num] != bar.ts_numerator()) || (denominators[bar_num] != bar.ts_denominator())) {
          throw std::invalid_argument(
            "TIME SIGNATURES FOR EACH BAR MUST BE THE SAME ACROSS ALL TRACKS.");
        }
      }
      bar_num++;
    }
    track_num++;
  }
}

void set_beat_lengths(midi::Piece *x) {
  for (int track_num=0; track_num<x->tracks_size(); track_num++) {
    midi::Track *t = x->mutable_tracks(track_num);
    for (int bar_num=0; bar_num<t->bars_size(); bar_num++) {
      midi::Bar *b = t->mutable_bars(bar_num);
      b->set_internal_beat_length(
        (double)b->ts_numerator() / b->ts_denominator() * 4);
    }
  }
}


void validate_piece(midi::Piece *x) {

  if (!x) {
    throw std::invalid_argument("PIECE IS NULL. CANNOT VALIDATE!");
  }

  // check that piece has resolution
  // is there other metadata to check
  if (x->resolution() == 0) {
    throw std::invalid_argument("PIECE RESOLUTION CAN NOT BE 0");
  }

  // validate range of fields
  validate_protobuf_field_ranges(x);

  // set the beat length using the time signature information
  // make this is set first as we use this throughout
  set_beat_lengths(x);

  // to be kind we sort events
  sort_piece_events(x);

  // check that there are the same number of bars in each track
  check_track_lengths(x);

  // make sure time signatures are the same in each bar
  check_time_sigs(x);

  // check events are valid
  // event_index should reference valid event
  // event times should be within each bar
  validate_events(x);

}

// update has notes information
void prepare_piece(midi::Piece *x) {
  for (int track_num=0; track_num<x->tracks_size(); track_num++) {
    midi::Track *track = x->mutable_tracks(track_num);
    for (int bar_num=0; bar_num<track->bars_size(); bar_num++) {
      midi::Bar *bar = track->mutable_bars(bar_num);
      //bar->set_is_drum(track->is_drum());
    }
  }
}

template <typename T>
void print_set(std::set<T> &values) {
  std::cout << "{";
  for (const auto v : values) {
    std::cout << v << ",";
  }
  std::cout << "}";
}

template <typename T>
void check_range(T value, T minv, T maxv, const char *field) {
  if ((value < minv) || (value >= maxv)) {
    std::ostringstream buffer;
    buffer << field << " not on range [" << minv << "," << maxv << ").";
    throw std::invalid_argument(buffer.str());
  }
} 

template <typename T> 
void check_all_same(std::set<T> &values, const char *field) {
  if (values.size() != 1) {
    std::ostringstream buffer;
    buffer << field << " values must all be the same. {";
    for (const auto val : values) {
      buffer << val << ",";
    }
    buffer << "}";
    throw std::invalid_argument(buffer.str());
  }
}

template <typename T>
void check_all_different(std::set<T> &values, int n, const char *field) {
  if (values.size() != n) {
    std::ostringstream buffer;
    buffer << field << " values must all be different.";
    throw std::invalid_argument(buffer.str());
  }
}

template <typename T> 
void check_in_domain(T value, std::set<T> domain, const char *field) {
  if (domain.find( value ) == domain.end()) {
    std::ostringstream buffer;
    buffer << field << " not in domain.";
    throw std::invalid_argument(buffer.str());
  }
}

int count_selected_bars(const midi::StatusTrack &track) {
  int count = 0;
  for (const auto selected : track.selected_bars()) {
    count += (int)selected;
  }
  return count;
}

enum STATUS_TRACK_TYPE {
  CONDITION,
  RESAMPLE,
  INFILL
};

STATUS_TRACK_TYPE infer_track_type(const midi::StatusTrack &track) {
  int num_bars = track.selected_bars_size();
  int bar_count = count_selected_bars(track);
  if (bar_count == 0) {
    return CONDITION;
  }
  else if (bar_count != num_bars) {
    return INFILL;
  }
  return RESAMPLE;
}

void validate_param(midi::SampleParam *param) {

  validate_protobuf_field_ranges(param);
  
}

void validate_status(midi::Status *status, midi::Piece *piece, midi::SampleParam *param) {

  if ((!status) || (!piece)) {
    throw std::invalid_argument("PIECE OR STATUS IS NULL. CANNOT VALIDATE!");
  }

  if (status->tracks_size() == 0) {
    throw std::invalid_argument("STATUS IS EMPTY");
  }

  // validate range of fields
  validate_protobuf_field_ranges(status);

  int track_num = 0;
  for (const auto track : status->tracks()) {
    
    /*
    if (!track.has_track_id()) {
      throw std::invalid_argument("NO TRACK ID");
    }
    if (!track.has_track_type()) {
      throw std::invalid_argument("NO TRACK TYPE");
    }
    if (!track.has_instrument()) {
      throw std::invalid_argument("NO INSTRUMENT");
    }
    if (!track.has_density()) {
      throw std::invalid_argument("NO DENSITY");
    }
    */
    if (track.selected_bars_size() == 0) {
      throw std::invalid_argument("NO SELECTED BARS");
    }
    if (track.selected_bars_size() < param->model_dim()) {
      throw std::invalid_argument("SELECTED BARS MUST BE ATLEAST MODEL_DIM");
    }
    
    // if track is conditioning it must be within range
    STATUS_TRACK_TYPE tt = infer_track_type(track);
    if ((tt == CONDITION) || (tt == INFILL)) {
      check_range(track.track_id(), 0, piece->tracks_size(), "track_id");
      // check that the instruments match
      // this doesn't really matter
      // for track generation we simply ignore piece
      // and bar infilling we simply ignore status
      /*
      int piece_inst = piece->tracks(track.track_id()).instrument();
      int status_inst = GM[track.instrument()][0] % 128;
      if (piece_inst != status_inst) {
        std::ostringstream buffer;
        buffer << "instruments differ on track " << track.track_id() << "(" << piece_inst << " != " << status_inst << ")";
        throw std::invalid_argument(buffer.str());
      }
      */
    }
    
    // enum checking should be built into the automatic verification
    // its an enum now we shouldn't need to check
    //check_in_domain(track.instrument(), GM_KEYS, "instrument");
    //check_range(track.density(), -1, 10, "density");
    
    // FIX THIS : AS THIS ONLY MATTERS FOR SINGLE STEP
    //check_in_domain(
    //  track.selected_bars_size(), {4,8,16}, "sample_bars (length)");

    // check that if resample all the bars are selected
    if (track.autoregressive() == 1) {
      for (const auto bar : track.selected_bars()) {
        if (!bar) {
          throw std::invalid_argument("WHEN RESAMPLE IS ENABLED ALL BARS MUST BE SELECTED!");
        }
      }
    }

    // check that if ignore the mode is condition
    if ((track.ignore()) && ((tt == INFILL) || (tt == RESAMPLE))) {
      throw std::invalid_argument("CANNOT IGNORE TRACK WITH SELECTED BARS.");
    }
    
    track_num++;
  }

  // check track lengths and track_ids
  std::set<int> track_lengths;
  std::set<int> track_ids;
  for (const auto track : status->tracks()) {
    track_lengths.insert( track.selected_bars_size() );
    track_ids.insert( track.track_id() );
  }
  check_all_same(track_lengths, "sample_bars (length)");
  check_all_different(track_ids, status->tracks_size(), "track_id");

  // check that all tracks are referenced in status
  // we don't need to have this gauranteed .
  /*
  for (int track_num=0; track_num<piece->tracks_size(); track_num++) {
    if (track_ids.find(track_num) == track_ids.end()) {
      std::ostringstream buffer;
      buffer << "Track " << track_num << " is not found in status!";
      throw std::invalid_argument(buffer.str());
    }
  }
  */

  // check that instruments match between piece and status
}


void validate_inputs(midi::Piece *piece, midi::Status *status, midi::SampleParam *param) {
  validate_piece(piece);
  validate_status(status, piece, param);
  validate_param(param);
}



}
// END OF NAMESPACE