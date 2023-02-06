template = """void protobuf_prune_{field}({input} *x) {{
  int keep = 0;
  for (int i=0; i<x->{field}_size(); i++) {{
    ////cout << x->{field}(i).should_prune() << endl;
    if (!x->{field}(i).should_prune()) {{
      if (keep<i) {{
        x->mutable_{field}()->SwapElements(i, keep);
      }}
      ++keep;
    }}
  }}
  x->mutable_{field}()->DeleteSubrange(keep, x->{field}_size() - keep);
}}

"""


fields = ["events", "tracks", "bars"]
inputs = ["midi::Piece", "midi::Piece", "midi::Track"]

with open("prune_protobuf.h", "w") as f:
  f.write('#include "protobuf/midi.pb.h"\n\n')
  for field,i in zip(fields,inputs):
    f.write(template.format(field=field,input=i))
  