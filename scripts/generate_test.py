import mmm_api as mmm
import glob
import json
import numpy as np

num_bars = 16

status = {
  "tracks":[
    {
      "trackId":0,
      "trackType":"STANDARD_DRUM_TRACK",
      "instrument":"any",
      "density":"DENSITY_FOUR",
      "selectedBars":[True for _ in range(num_bars)],
      "autoregressive":True,
      "ignore":False,
      "min_polyphony_q":"POLYPHONY_ANY",
      "max_polyphony_q":"POLYPHONY_ANY",
      "min_note_duration_q":"DURATION_ANY",
      "max_note_duration_q":"DURATION_ANY"
    }
  ]
}

param = {
  "tracks_per_step" : 4,
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

midi_json = {
  "tempo" : 120,
  "tracks" : {
    "bars" : [{"tsNumerator" : 4, "tsDenominator" : 4} for _ in range(num_bars)]
  }
}

piece = mmm.sample_multi_step(
  json.dumps(midi_json), json.dumps(status), json.dumps(param))

encoder = mmm.TrackDensityEncoder()
encoder.json_to_midi(piece, "gen.mid")
