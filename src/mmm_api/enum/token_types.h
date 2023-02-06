#pragma once

namespace mmm {

enum TOKEN_TYPE {
  PIECE_START,
  NOTE_ONSET,
  NOTE_OFFSET,
  PITCH,
  NON_PITCH,
  VELOCITY,
  TIME_DELTA,
  TIME_ABSOLUTE,
  INSTRUMENT,
  BAR,
  BAR_END,
  TRACK,
  TRACK_END,
  DRUM_TRACK,
  FILL_IN,
  FILL_IN_PLACEHOLDER,
  FILL_IN_START,
  FILL_IN_END,
  HEADER,
  VELOCITY_LEVEL,
  GENRE,
  DENSITY_LEVEL,
  TIME_SIGNATURE,
  SEGMENT,
  SEGMENT_END,
  SEGMENT_FILL_IN,
  NOTE_DURATION,
  AV_POLYPHONY,
  MIN_POLYPHONY,
  MAX_POLYPHONY,
  MIN_NOTE_DURATION,
  MAX_NOTE_DURATION,
  NUM_BARS,
  NONE
};

inline const char* toString(mmm::TOKEN_TYPE tt) {
  switch (tt) {
    case PIECE_START: return "PIECE_START";
    case NOTE_ONSET: return "NOTE_ONSET";
    case NOTE_OFFSET: return "NOTE_OFFSET";
    case PITCH: return "PITCH";
    case NON_PITCH: return "NON_PITCH";
    case VELOCITY: return "VELOCITY";
    case TIME_DELTA: return "TIME_DELTA";
    case TIME_ABSOLUTE: return "TIME_ABSOLUTE";
    case INSTRUMENT: return "INSTRUMENT";
    case BAR: return "BAR";
    case BAR_END: return "BAR_END";
    case TRACK: return "TRACK";
    case TRACK_END: return "TRACK_END";
    case DRUM_TRACK: return "DRUM_TRACK";
    case FILL_IN: return "FILL_IN";
    case FILL_IN_PLACEHOLDER: return "FILL_IN_PLACEHOLDER";
    case FILL_IN_START: return "FILL_IN_START";
    case FILL_IN_END: return "FILL_IN_END";
    case HEADER: return "HEADER";
    case VELOCITY_LEVEL: return "VELOCITY_LEVEL";
    case GENRE: return "GENRE";
    case DENSITY_LEVEL: return "DENSITY_LEVEL";
    case TIME_SIGNATURE: return "TIME_SIGNATURE";
    case SEGMENT: return "SEGMENT";
    case SEGMENT_END: return "SEGMENT_END";
    case SEGMENT_FILL_IN: return "SEGMENT_FILL_IN";
    case NOTE_DURATION: return "NOTE_DURATION";
    case AV_POLYPHONY: return "AV_POLYPHONY";
    case MIN_POLYPHONY: return "MIN_POLYPHONY";
    case MAX_POLYPHONY: return "MAX_POLYPHONY";
    case MIN_NOTE_DURATION: return "MIN_NOTE_DURATION";
    case MAX_NOTE_DURATION: return "MAX_NOTE_DURATION";
    case NUM_BARS: return "NUM_BARS";
    case NONE: return "NONE";
  }
}

}