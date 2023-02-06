import json

def protobuf(func):   
	def inner(x, *args, **kwargs):
		if isinstance(x,str):
			x = json.loads(x)
		x = func(x, *args, **kwargs)
		return json.dumps(x)   
	return inner

def load_json(path):
	with open(path, "r") as f:
		return json.load(f)

def dump_json(x, path):
	with open(path, "w") as f:
		json.dump(x, f, indent=4)

def make_cpp_dict(d, name):
  content = ["\t{{{key},{value}}},".format(key=k,value=v) for k,v in d.items()]
  s = """const std::map<int,int> {name}  = {{
{content}
}};
""".format(name=name, content="\n".join(content))
  return s