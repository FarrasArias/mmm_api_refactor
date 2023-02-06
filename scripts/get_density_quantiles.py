import os
import json
import glob
import random
import numpy as np
from tqdm import tqdm
from multiprocessing import Pool
import mmm_api as mmm

def num_notes_per_bar(midi_json):
  output = []
  for track in midi_json.get("tracks",[]):
    track_inst = track["instrument"]
    for bar in track.get("bars",[]):
      num_notes = 0
      for event_id in bar.get("events",[]):
        if midi_json["events"][event_id]["velocity"]:
          num_notes += 1
      if num_notes:
        for track_type in track["trainTypes"]:
          output.append( (track_type,track_inst,num_notes) )
  return output

def worker(path):
  try:
    enc = mmm.TeTrackDensityEncoder()
    data = json.loads(enc.midi_to_json(path))
    return num_notes_per_bar(data)
  except:
    return []

def compute_stat_on_dataset(output_file):
  if os.path.exists(output_file):
    return np.load(output_file, allow_pickle=True)["data"]
  stat = []
  paths = glob.glob("/Users/Jeff/DDD_split/**/*.mid", recursive=True)
  random.shuffle(paths)
  p = Pool(8)
  count = 0
  for out in tqdm(p.imap_unordered(worker, paths),total=len(paths)):
    stat.extend(out)
    count += 1
    if count % 10000 == 0:
      np.savez_compressed(os.path.splitext(output_file)[0] + "_{}.npz".format(count), data=stat)
    if count >= 100000:
      break

if __name__ == "__main__":
  compute_stat_on_dataset("OPZ_DENSITY.npz")