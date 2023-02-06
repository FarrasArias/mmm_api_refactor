import mmm_api as mmm
from utils import *
import json
import random

@protobuf
def update_genre(piece, genre):
  piece["genreData"] = {"discogs" : genre, "tagtraum" : genre, "lastfm" : genre}
  return piece

@protobuf
def update_opz(piece):
  for track in piece["tracks"]:
    track["trackType"] = random.choice(track["internalTrainTypes"])
  return piece

#enc = mmm.DensityGenreTagtraumEncoder()
#enc = mmm.TrackNoteDurationEncoder()
#enc = mmm.PolyphonyEncoder()
#enc = mmm.PolyphonyDurationEncoder()
#enc = mmm.NewDurationEncoder()
#enc = mmm.TrackBarFillDensityEncoder()
#enc = mmm.NewVelocityEncoder()
#enc = mmm.MultiLengthEncoder()
#enc = mmm.TeVelocityDurationPolyphonyEncoder()
enc = mmm.TeVelocityEncoder()

#print( enc.rep.encode_partial(mmm.mmm::TOKEN_TYPE.VELOCITY, 120) )
#exit()


nb = 8

try:
  midi_json = enc.midi_to_json("../test_midi/test_{}.mid".format(nb))
except:
  midi_json = enc.midi_to_json("test_midi/test_{}.mid".format(nb))

# for opz
midi_json = mmm.update_valid_segments(midi_json, nb, 1, True)
midi_json = update_opz(midi_json)

x = json.loads(midi_json)
x["events"] = []
print(json.dumps(x,indent=4))
 #midi_json = update_genre(midi_json, "spoken")

tokens = enc.json_to_tokens(midi_json)
for token in tokens:
  print(enc.rep.pretty(token))

print("STARTING ...")

rg = mmm.REP_GRAPH(mmm.ENCODER_TYPE.NEW_DURATION_ENCODER, mmm.MODEL_TYPE.TRACK_MODEL)
rg.validate_sequence(tokens)

#for token in tokens:
#  print(enc.rep.pretty(token), rg.get_mask(token))

enc.tokens_to_midi(tokens, "check.mid")

import pretty_midi

m = pretty_midi.PrettyMIDI("check.mid")
for instrument in m.instruments:
  for note in instrument.notes:
    print(note)