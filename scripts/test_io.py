import os
import glob
import random
import mmm_api as mmm

os.makedirs("test_quant", exist_ok=True)

paths = glob.glob("/users/jeff/DDD/**/*.mid", recursive=True)
random.shuffle(paths)

for i,path in enumerate(paths[:100]):
  try:
    enc = mmm.TrackUnquantizedEncoder()
    x = enc.midi_to_json(path)
    enc.json_to_midi(x, "test_quant/{}_unquant.mid".format(i))

    enc = mmm.TrackEncoder()
    x = enc.midi_to_json(path)
    enc.json_to_midi(x, "test_quant/{}_quant.mid".format(i))

    enc = mmm.AbsoluteEncoder()
    tokens = enc.midi_to_tokens(path)
    enc.tokens_to_midi(tokens, "test_quant/{}_absolute.mid".format(i))

    #for token in tokens:
    #  print( enc.rep.pretty(token) )

  except Exception as e:
    print(e)
