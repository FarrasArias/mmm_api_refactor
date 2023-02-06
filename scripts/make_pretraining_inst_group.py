# we use this script to create instrument
# groupings that the model is actually trained on
# we need a map that we can pass into a TOKEN_DOMAIN
import numpy as np
from utils import *
from scipy.stats import rankdata

gm = {int(k):v for k,v in load_json("gm.json").items()}

# note these are 1-indexed
groups = [
  [1,2,3], # pianos
  [5,6], # electric pianos
  [17,18,19], # organs
  [20,21], # church organs
  [34,35], # electric bass
  [37,38], # slap bass
  [39,40], # synth bass 
  [49,50], # string ensemble
  [51,52], # synth strings
  [63,64], # synth brass
  [89,90,91,92,93,94,95,96], # pads
]

# then create a map from [0-127) --> new instrument numbers
group_min = {i:i for i in range(128)}
for ii,group in enumerate(groups):
  for value in group:
    group_min[value-1] = int(np.min(group)-1)
print(make_cpp_dict(group_min, "PRETRAIN_GROUPING_V2"))
exit()

used = set()
mapping = {}
pretty_mapping = {}
for i in range(128):
  used.add(group_min[i])
  mapping[i] = len(used)-1
  pretty_mapping[gm[i]] = mapping[i]

# save the mapping
dump_json(mapping, "pretrain_group.json")
dump_json(pretty_mapping, "pretrain_group_pretty.json")

# make the cpp map and save in header
print(make_cpp_dict(mapping, "PRETRAIN_GROUPING"))