data = """piece_start -> global_param
global_param -> track
track -> track_param
track_param -> bar
bar -> bar_content
bar_content -> bar_end
bar_end -> bar
bar_end -> track_end
track_end -> track
track_end -> drum_track
track_end -> piece_end
track_end -> fill_in_start
"""

data = """drum_track -> drum_track_param
drum_track_param -> drum_bar
drum_bar -> drum_bar_content
drum_bar_content -> drum_bar_end
drum_bar_end -> drum_bar
drum_bar_end -> drum_track_end
drum_track_end -> drum_track
drum_track_end -> track
drum_track_end -> piece_end
drum_track_end -> fill_in_start
"""

data = """fill_in_start -> fill_bar_content
fill_bar_content -> fill_in_end
fill_in_end -> fill_in_start
fill_in_end -> piece_end"""

data = """{NOTE_ONSET, TIME_DELTA},
  {NOTE_ONSET, NOTE_ONSET},
  {NOTE_ONSET, BAR_END},
  {NOTE_ONSET, FILL_IN_END},
  {NOTE_OFFSET, TIME_DELTA},
  {NOTE_OFFSET, NOTE_ONSET},
  {NOTE_OFFSET, NOTE_OFFSET},
  {NOTE_OFFSET, BAR_END},
  {NOTE_OFFSET, FILL_IN_END},
  {TIME_DELTA, TIME_DELTA},
  {TIME_DELTA, NOTE_ONSET},
  {TIME_DELTA, NOTE_OFFSET},
  {TIME_DELTA, BAR_END},
  {TIME_DELTA, FILL_IN_END},"""

for row in data.splitlines():
  u,v,*_ = row.replace("{","").replace("}","").split(",")
  prefix = "" #"DRUM_"
  u = prefix + u.strip().upper()
  v = prefix + v.strip().upper()
  print( '{{"{}","{}"}},'.format(u,v) )
