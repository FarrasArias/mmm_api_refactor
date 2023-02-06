import mmm_api as mmm
import json
import numpy as np

num_bars = 16

param = {
  "tracks_per_step" : 1,
  "bars_per_step" : 2,
  "model_dim" : 4,
  "percentage" : 100,
  "batch_size" : 1,
  "temperature" : 1.,
  "max_steps" : 0,
  "polyphony_hard_limit" : 6,
  "shuffle" : False,
  "verbose" : False,
  "ckpt" : "/users/jeff/CODE/MMM_TRAINING/models/el_yellow_ts.pt",
}

encoder = mmm.ElVelocityDurationPolyphonyYellowEncoder()
midi_json = encoder.midi_to_json("/users/jeff/multitrack.midi")

#piece = json.loads(midi_json)
#print(piece["tracks"][0])
#for bar in piece["tracks"][0]["bars"]:
#  for ii in bar.get("events",[]):
#    print(piece["events"][ii])

tokens = encoder.midi_to_tokens("/users/jeff/multitrack.midi")
#for token in tokens:
#  print(encoder.rep.pretty(token))


piece = json.loads(encoder.tokens_to_json(tokens))
for bar in piece["tracks"][0]["bars"]:
  for ii in bar.get("events",[]):
    print(piece["events"][ii])
#exit()

status = mmm.piece_to_status(midi_json)
# modify 
status = json.loads(status)
status["tracks"][3]["selectedBars"] = [False,False,False,False,True,True,False,False]
status = json.dumps(status)

param["verbose"] = True

piece = mmm.sample_multi_step(
  midi_json, status, json.dumps(param))

piece = json.loads(piece)

for bar in piece["tracks"][0]["bars"]:
  for ii in bar.get("events",[]):
    print(ii, piece["events"][ii])