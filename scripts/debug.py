import mmm_api as mmm
import json

def protobuf(func):   
	def inner(x, *args, **kwargs):
		if isinstance(x,str):
			x = json.loads(x)
		x = func(x, *args, **kwargs)
		return json.dumps(x)   
	return inner

@protobuf
def override_param(param, force=False, **kwargs):
  for k,v in kwargs.items():
    if k in param or force:
      param[k] = v
    else:
      print("key {} not found".format(k))
  return param

@protobuf
def fix_status(status):
  for track in status["tracks"]:
    track["selectedBars"] = [bool(x) for x in track["selectedBars"]]
    track["resample"] = bool(track["resample"])
    track["ignore"] = bool(track["ignore"])
  return status

@protobuf
def fix_piece(piece):
  for track in piece["tracks"]:
    track["isDrum"] = bool(track["isDrum"])
  return piece


def load_json(path):
  with open(path,"r") as f:
    return json.dumps(json.load(f))

def dump_json(x, path):
  with open(path,"w") as f:
    json.dump(x,f,indent=4)

if __name__ == "__main__":

  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument('--folder', type=str, required=True)
  args = parser.parse_args()

  piece_path = "../debug/{}/piece.json".format(args.folder)
  status_path = "../debug/{}/status.json".format(args.folder)
  piece = fix_piece(load_json(piece_path))
  status = fix_status(load_json(status_path))
  param = mmm.default_sample_param()


  # fix the piece
  #x = json.loads(piece)
  #x["events"] = [{"velocity":e["velocity"],"pitch":e["pitch"],"time":48 if e["velocity"]==0 and e["time"]%48==0 and e["time"]>0 else e["time"]%48} for e in x["events"]]
  #dump_json(x, "../debug/piece_fixed.json")
  #piece = json.dumps(x)

  param = override_param(param, barsPerStep=1, tracksPerStep=1, ckpt="/users/jeff/CODE/MMM_API/paper_bar_4.pt", verbose=True, encoder="TRACK_BAR_FILL_DENSITY_ENCODER", modelDim=4, temperature=1, force=True, skip_preprocess=False)

  mmm.sample_multi_step(piece, status, param)