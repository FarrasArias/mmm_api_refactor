import glob
import random
import mmm_api as mmm
from tqdm import tqdm

# load a bunch of midis
# encode them and check that validation is successful
# to ensure our encoding matches the graph

# ARE THERE edges which are never used in the graph

def check(path):
  et = mmm.ENCODER_TYPE.NEW_DURATION_ENCODER
  enc = mmm.getEncoder(et)
  rg = mmm.CONSTRAINT_REP_GRAPH(et)
  try:
    tokens = enc.midi_to_tokens(path)
  except:
    return
  rg.validate_sequence(tokens)

paths = glob.glob("/users/jeff/DDD/**/*.mid", recursive=True)
random.shuffle(paths)
for path in tqdm(paths[:10000]):
  check(path)

exit()

et = mmm.ENCODER_TYPE.TRACK_BAR_FILL_DENSITY_ENCODER
et = mmm.ENCODER_TYPE.NEW_DURATION_ENCODER

enc = mmm.getEncoder(et)
rg = mmm.CONSTRAINT_REP_GRAPH(et)


for _ in tqdm(range(1000)):

  tokens = rg.generate_random()
  for token in tokens:
    print( enc.rep.pretty(token) )
  enc.tokens_to_midi(tokens, "test.mid")