import mmm_api as mmm
import glob
import json
import numpy as np

paths = glob.glob("/users/jeff/DDD/**/*.mid", recursive=True)
path = np.random.choice(paths)

enc = mmm.EncoderVt()
midi_json = enc.midi_to_json(path)

midi_json = mmm.select_random_segment(midi_json, 4, 1, 4, False, np.random.randint(2**31))
enc.json_to_midi(midi_json, "test.mid")


midi_json = enc.midi_to_json("test.mid")

piece = json.loads(midi_json)
for track in piece["tracks"]:
  print(track["trackType"])
  f = track["internalFeatures"][0]
  f.pop("polyphonyDistribution")
  print(json.dumps(f,indent=4))


tokens = enc.json_to_tokens(midi_json)
for token in tokens:
  print(enc.rep.pretty(token))

