import mmm_api as mmm
import glob
import json
import numpy as np

def dump_json(path, x):
  with open(path,"w") as f:
    json.dump(x, f, indent=4)

seed = 8123713
np.random.seed(seed)

paths = glob.glob("/users/jeff/DDD/**/*.mid", recursive=True)
path = np.random.choice(paths)

encoder = mmm.TrackDensityEncoder()
midi_json = encoder.midi_to_json(path)

seed = np.random.randint(2**31)
midi_json = mmm.select_random_segment(midi_json, 4, 4, 4, False, seed)
status = mmm.status_from_piece(midi_json)

# adjust some of the bars to true
""""
status = {
  "tracks":[
    {
      "trackId":0,
      "trackType":"STANDARD_TRACK",
      "instrument":"flute",
      "density":"DENSITY_ANY",
      "selectedBars":[False,True,False,False],
      "autoregressive":False,
      "ignore":False,
      "min_polyphony_q":"POLYPHONY_ANY",
      "max_polyphony_q":"POLYPHONY_ANY",
      "min_note_duration_q":"DURATION_ANY",
      "max_note_duration_q":"DURATION_ANY"
    },
    {
      "trackId":1,
      "trackType":"STANDARD_TRACK",
      "instrument":"acoustic_grand_piano",
      "density":"DENSITY_ANY",
      "selectedBars":[False,False,True,True],
      "autoregressive":False,
      "ignore":False,
      "min_polyphony_q":"POLYPHONY_ANY",
      "max_polyphony_q":"POLYPHONY_ANY",
      "min_note_duration_q":"DURATION_ANY",
      "max_note_duration_q":"DURATION_ANY"
    },
    {
      "trackId":2,
      "trackType":"STANDARD_TRACK",
      "instrument":"flute",
      "density":"DENSITY_ANY",
      "selectedBars":[False,True,False,True],
      "autoregressive":False,
      "ignore":False,
      "min_polyphony_q":"POLYPHONY_ANY",
      "max_polyphony_q":"POLYPHONY_ANY",
      "min_note_duration_q":"DURATION_ANY",
      "max_note_duration_q":"DURATION_ANY"
    },
    {
      "trackId":3,
      "trackType":"STANDARD_TRACK",
      "instrument":"flute",
      "density":"DENSITY_ANY",
      "selectedBars":[True,False,False,False],
      "autoregressive":False,
      "ignore":False,
      "min_polyphony_q":"POLYPHONY_ANY",
      "max_polyphony_q":"POLYPHONY_ANY",
      "min_note_duration_q":"DURATION_ANY",
      "max_note_duration_q":"DURATION_ANY"
    }
  ]
}
"""

param = {
  "tracks_per_step" : 4,
  "bars_per_step" : 4,
  "model_dim" : 4,
  "percentage" : 100,
  "batch_size" : 1,
  "temperature" : 1.,
  "max_steps" : 0,
  "shuffle" : False,
  "verbose" : True,
  "ckpt" : "/users/jeff/CODE/MMM_TRAINING/models/el_yellow_ts.pt"
}

print(json.dumps(param))


dump_json("doc/piece_example.json", json.loads(midi_json))
dump_json("doc/status_example.json", json.loads(status))
dump_json("doc/param_example.json", param)

print( mmm.blank(midi_json, status, json.dumps(param)) )

