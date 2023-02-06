import networkx as nx
import mmm_api as mmm
from matplotlib import pyplot as plt

edges = [
  ("PIECE_START", "TRACK"),
  ("TRACK", "INSTRUMENT"),
  ("INSTRUMENT", "BAR"),
  ("BAR", "TIME_DELTA"),
  ("BAR", "NOTE_ONSET"),
  ("BAR", "FILL_PLACEHOLDER"),

  ("FILL_PLACEHOLDER", "BAR_END"),
  
  ("NOTE_ONSET","TIME_DELTA"),
  ("NOTE_ONSET","NOTE_ONSET"),
  
  ("NOTE_OFFSET", "NOTE_OFFSET"),
  ("NOTE_OFFSET", "NOTE_ONSET"),
  ("NOTE_OFFSET", "TIME_DELTA"),

  ("TIME_DELTA", "TIME_DELTA"),
  ("TIME_DELTA", "NOTE_OFFSET"),
  ("TIME_DELTA", "NOTE_ONSET"),

  ("TIME_DELTA", "BAR_END"),
  ("NOTE_OFFSET", "BAR_END"),
  ("NOTE_ONSET", "BAR_END"),

  ("BAR_END", "BAR"),
  ("BAR_END", "TRACK_END"),

  ("TRACK_END", "TRACK"),
  ("TRACK_END", "FILL_IN_START"),

  ("FILL_IN_START", "FILL_IN_END")

]

edges = [
  ("PIECE_START", "TRACK"),
  ("TRACK", "INSTRUMENT"),
  ("INSTRUMENT", "BAR"),

  ("BAR", "BAR_END"), # EMPTY BAR
  ("BAR", "TIME_DELTA"),
  ("BAR", "NOTE_ONSET"),
  ("BAR", "FILL_PLACEHOLDER"),

  ("FILL_PLACEHOLDER", "BAR_END"),
  
  ("NOTE_ONSET","NOTE_DURATION"),
  
  ("NOTE_DURATION", "NOTE_ONSET"),
  ("NOTE_DURATION", "BAR_END"),
  ("NOTE_DURATION", "TIME_DELTA"),

  ("TIME_DELTA", "TIME_DELTA"),
  ("TIME_DELTA", "NOTE_ONSET"),
  ("TIME_DELTA", "BAR_END"),

  ("BAR_END", "BAR"),
  ("BAR_END", "TRACK_END"),

  ("TRACK_END", "TRACK"),
  ("TRACK_END", "FILL_IN_START"),

  ("FILL_IN_START", "FILL_IN_END")

]

g = nx.DiGraph()
for u,v in edges:
  g.add_edge(u,v)
  #g.add_node(u, label=u)
  #g.add_node(v, label=v)

#pos = nx.spring_layout(g, scale=.5)
pos = nx.nx_pydot.pydot_layout(g)
plt.figure(figsize=(10,10))

nx.draw(g, with_labels=True, pos=pos, node_color="white")
plt.tight_layout()
plt.show()