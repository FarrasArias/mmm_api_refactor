#pragma once

#include "../encoder/encoder_all.h"
#include <string>

// START OF NAMESPACE
namespace mmm {

enum ENCODER_TYPE {
  TE_TRACK_DENSITY_ENCODER,
  TRACK_DENSITY_ENCODER_V2,
  TRACK_INTERLEAVED_ENCODER,
  TRACK_INTERLEAVED_W_HEADER_ENCODER,
  TRACK_ENCODER,
  TRACK_NO_INST_ENCODER,
  TRACK_UNQUANTIZED_ENCODER,
  TRACK_DENSITY_ENCODER,
  TRACK_BAR_FILL_DENSITY_ENCODER,
  TRACK_NOTE_DURATION_ENCODER,
  TRACK_NOTE_DURATION_CONT_ENCODER,
  TRACK_NOTE_DURATION_EMBED_ENCODER,
  DENSITY_GENRE_ENCODER,
  DENSITY_GENRE_TAGTRAUM_ENCODER,
  DENSITY_GENRE_LASTFM_ENCODER,
  POLYPHONY_ENCODER,
  POLYPHONY_DURATION_ENCODER,
  DURATION_ENCODER,
  NEW_DURATION_ENCODER,
  TE_ENCODER,
  NEW_VELOCITY_ENCODER,
  ABSOLUTE_ENCODER,
  MULTI_LENGTH_ENCODER,
  TE_VELOCITY_DURATION_POLYPHONY_ENCODER,
  TE_VELOCITY_ENCODER,
  EL_VELOCITY_DURATION_POLYPHONY_ENCODER,
  EL_VELOCITY_DURATION_POLYPHONY_YELLOW_ENCODER,
  EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER,
  NO_ENCODER
};

std::vector<std::string> ENCODER_TYPE_STRINGS = {
 "TE_TRACK_DENSITY_ENCODER",
 "TRACK_DENSITY_ENCODER_V2",
 "TRACK_INTERLEAVED_ENCODER",
 "TRACK_INTERLEAVED_W_HEADER_ENCODER",
 "TRACK_ENCODER",
 "TRACK_NO_INST_ENCODER",
 "TRACK_UNQUANTIZED_ENCODER",
 "TRACK_DENSITY_ENCODER",
 "TRACK_BAR_FILL_DENSITY_ENCODER",
 "TRACK_NOTE_DURATION_ENCODER",
 "TRACK_NOTE_DURATION_CONT_ENCODER",
 "TRACK_NOTE_DURATION_EMBED_ENCODER",
 "DENSITY_GENRE_ENCODER",
 "DENSITY_GENRE_TAGTRAUM_ENCODER",
 "DENSITY_GENRE_LASTFM_ENCODER",
 "POLYPHONY_ENCODER",
 "POLYPHONY_DURATION_ENCODER",
 "DURATION_ENCODER",
 "NEW_DURATION_ENCODER",
 "TE_ENCODER",
 "NEW_VELOCITY_ENCODER",
 "ABSOLUTE_ENCODER",
 "MULTI_LENGTH_ENCODER",
 "TE_VELOCITY_DURATION_POLYPHONY_ENCODER",
 "TE_VELOCITY_ENCODER",
 "EL_VELOCITY_DURATION_POLYPHONY_ENCODER",
 "EL_VELOCITY_DURATION_POLYPHONY_YELLOW_ENCODER",
 "EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER",
 "NO_ENCODER"
};

std::unique_ptr<ENCODER> getEncoder(ENCODER_TYPE et) {
  ENCODER *e;
  switch (et) {
    case TE_TRACK_DENSITY_ENCODER:
			e = new TeTrackDensityEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_DENSITY_ENCODER_V2:
			e = new TrackDensityEncoderV2();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_INTERLEAVED_ENCODER:
			e = new TrackInterleavedEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_INTERLEAVED_W_HEADER_ENCODER:
			e = new TrackInterleavedWHeaderEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_ENCODER:
			e = new TrackEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_NO_INST_ENCODER:
			e = new TrackNoInstEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_UNQUANTIZED_ENCODER:
			e = new TrackUnquantizedEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_DENSITY_ENCODER:
			e = new TrackDensityEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_BAR_FILL_DENSITY_ENCODER:
			e = new TrackBarFillDensityEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_NOTE_DURATION_ENCODER:
			e = new TrackNoteDurationEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_NOTE_DURATION_CONT_ENCODER:
			e = new TrackNoteDurationContEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TRACK_NOTE_DURATION_EMBED_ENCODER:
			e = new TrackNoteDurationEmbedEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case DENSITY_GENRE_ENCODER:
			e = new DensityGenreEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case DENSITY_GENRE_TAGTRAUM_ENCODER:
			e = new DensityGenreTagtraumEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case DENSITY_GENRE_LASTFM_ENCODER:
			e = new DensityGenreLastfmEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case POLYPHONY_ENCODER:
			e = new PolyphonyEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case POLYPHONY_DURATION_ENCODER:
			e = new PolyphonyDurationEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case DURATION_ENCODER:
			e = new DurationEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case NEW_DURATION_ENCODER:
			e = new NewDurationEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TE_ENCODER:
			e = new TeEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case NEW_VELOCITY_ENCODER:
			e = new NewVelocityEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case ABSOLUTE_ENCODER:
			e = new AbsoluteEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case MULTI_LENGTH_ENCODER:
			e = new MultiLengthEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TE_VELOCITY_DURATION_POLYPHONY_ENCODER:
			e = new TeVelocityDurationPolyphonyEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case TE_VELOCITY_ENCODER:
			e = new TeVelocityEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case EL_VELOCITY_DURATION_POLYPHONY_ENCODER:
			e = new ElVelocityDurationPolyphonyEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case EL_VELOCITY_DURATION_POLYPHONY_YELLOW_ENCODER:
			e = new ElVelocityDurationPolyphonyYellowEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER:
			e = new ElVelocityDurationPolyphonyYellowFixedEncoder();
			return std::unique_ptr<ENCODER>(e);
			break;
    case NO_ENCODER: return NULL;
  }
}

ENCODER_TYPE getEncoderType(const std::string &s) {
  if (s == "TE_TRACK_DENSITY_ENCODER") return TE_TRACK_DENSITY_ENCODER;
  if (s == "TRACK_DENSITY_ENCODER_V2") return TRACK_DENSITY_ENCODER_V2;
  if (s == "TRACK_INTERLEAVED_ENCODER") return TRACK_INTERLEAVED_ENCODER;
  if (s == "TRACK_INTERLEAVED_W_HEADER_ENCODER") return TRACK_INTERLEAVED_W_HEADER_ENCODER;
  if (s == "TRACK_ENCODER") return TRACK_ENCODER;
  if (s == "TRACK_NO_INST_ENCODER") return TRACK_NO_INST_ENCODER;
  if (s == "TRACK_UNQUANTIZED_ENCODER") return TRACK_UNQUANTIZED_ENCODER;
  if (s == "TRACK_DENSITY_ENCODER") return TRACK_DENSITY_ENCODER;
  if (s == "TRACK_BAR_FILL_DENSITY_ENCODER") return TRACK_BAR_FILL_DENSITY_ENCODER;
  if (s == "TRACK_NOTE_DURATION_ENCODER") return TRACK_NOTE_DURATION_ENCODER;
  if (s == "TRACK_NOTE_DURATION_CONT_ENCODER") return TRACK_NOTE_DURATION_CONT_ENCODER;
  if (s == "TRACK_NOTE_DURATION_EMBED_ENCODER") return TRACK_NOTE_DURATION_EMBED_ENCODER;
  if (s == "DENSITY_GENRE_ENCODER") return DENSITY_GENRE_ENCODER;
  if (s == "DENSITY_GENRE_TAGTRAUM_ENCODER") return DENSITY_GENRE_TAGTRAUM_ENCODER;
  if (s == "DENSITY_GENRE_LASTFM_ENCODER") return DENSITY_GENRE_LASTFM_ENCODER;
  if (s == "POLYPHONY_ENCODER") return POLYPHONY_ENCODER;
  if (s == "POLYPHONY_DURATION_ENCODER") return POLYPHONY_DURATION_ENCODER;
  if (s == "DURATION_ENCODER") return DURATION_ENCODER;
  if (s == "NEW_DURATION_ENCODER") return NEW_DURATION_ENCODER;
  if (s == "TE_ENCODER") return TE_ENCODER;
  if (s == "NEW_VELOCITY_ENCODER") return NEW_VELOCITY_ENCODER;
  if (s == "ABSOLUTE_ENCODER") return ABSOLUTE_ENCODER;
  if (s == "MULTI_LENGTH_ENCODER") return MULTI_LENGTH_ENCODER;
  if (s == "TE_VELOCITY_DURATION_POLYPHONY_ENCODER") return TE_VELOCITY_DURATION_POLYPHONY_ENCODER;
  if (s == "TE_VELOCITY_ENCODER") return TE_VELOCITY_ENCODER;
  if (s == "EL_VELOCITY_DURATION_POLYPHONY_ENCODER") return EL_VELOCITY_DURATION_POLYPHONY_ENCODER;
  if (s == "EL_VELOCITY_DURATION_POLYPHONY_YELLOW_ENCODER") return EL_VELOCITY_DURATION_POLYPHONY_YELLOW_ENCODER;
  if (s == "EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER") return EL_VELOCITY_DURATION_POLYPHONY_YELLOW_FIXED_ENCODER;
  return NO_ENCODER;
}

int getEncoderSize(ENCODER_TYPE et) {
  std::unique_ptr<ENCODER> encoder = getEncoder(et);
  if (!encoder) {
    return 0;
  }
  int size = encoder->rep->max_token();
  //delete encoder;
  return size;
}
}
// END OF NAMESPACE
