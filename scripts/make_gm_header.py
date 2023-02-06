import numpy as np

GENERAL_MIDI_MAP_FULL = {
  "opz_sample" : list(range(96,104)) + list(range(120,128)),
  "opz_bass" : list(range(32,40)),
  "any" : list(range(256)),
  "piano" : list(range(8)),
  "chromatic_perc" : list(range(9,16)),
  "organ" : list(range(16,24)),
  "guitar" : list(range(24,32)),
  "bass" : list(range(32,40)),
  "strings" : list(range(40,48)),
  "ensemble" : list(range(48,56)),
  "brass" : list(range(56,64)),
  "reed" : list(range(64,72)),
  "pipe" : list(range(72,80)),
  "synth_lead" : list(range(80,88)),
  "synth_pad" : list(range(88,96)),
  "synth_effects" : list(range(96,104)),
  "ethnic" : list(range(104,112)),
  "percussive" : list(range(112,120)),
  "sound_fx" : list(range(120,128)),
  "no_drums" : list(range(128)),
  "drums" : list(range(128,256)),
  "Acoustic Grand Piano" : [0],
  "Bright Acoustic Piano" : [1],
  "Electric Grand Piano" : [2],
  "Honky-tonk Piano" : [3],
  "Electric Piano 1" : [4],
  "Electric Piano 2" : [5],
  "Harpsichord" : [6],
  "Clavi" : [7],
  "Celesta" : [8],
  "Glockenspiel" : [9],
  "Music Box" : [10],
  "Vibraphone" : [11],
  "Marimba" : [12],
  "Xylophone" : [13],
  "Tubular Bells" : [14],
  "Dulcimer" : [15],
  "Drawbar Organ" : [16],
  "Percussive Organ" : [17],
  "Rock Organ" : [18],
  "Church Organ" : [19],
  "Reed Organ" : [20],
  "Accordion" : [21],
  "Harmonica" : [22],
  "Tango Accordion" : [23],
  "Acoustic Guitar (nylon)" : [24],
  "Acoustic Guitar (steel)" : [25],
  "Electric Guitar (jazz)" : [26],
  "Electric Guitar (clean)" : [27],
  "Electric Guitar (muted)" : [28],
  "Overdriven Guitar" : [29],
  "Distortion Guitar" : [30],
  "Guitar Harmonics" : [31],
  "Acoustic Bass" : [32],
  "Electric Bass (finger)" : [33],
  "Electric Bass (pick)" : [34],
  "Fretless Bass" : [35],
  "Slap Bass 1" : [36],
  "Slap Bass 2" : [37],
  "Synth Bass 1" : [38],
  "Synth Bass 2" : [39],
  "Violin" : [40],
  "Viola" : [41],
  "Cello" : [42],
  "Contrabass" : [43],
  "Tremolo Strings" : [44],
  "Pizzicato Strings" : [45],
  "Orchestral Harp" : [46],
  "Timpani" : [47],
  "String Ensemble 1" : [48],
  "String Ensemble 2" : [49],
  "Synth Strings 1" : [50],
  "Synth Strings 2" : [51],
  "Choir Aahs" : [52],
  "Voice Oohs" : [53],
  "Synth Voice" : [54],
  "Orchestra Hit" : [55],
  "Trumpet" : [56],
  "Trombone" : [57],
  "Tuba" : [58],
  "Muted Trumpet" : [59],
  "French Horn" : [60],
  "Brass Section" : [61],
  "Synth Brass 1" : [62],
  "Synth Brass 2" : [63],
  "Soprano Sax" : [64],
  "Alto Sax" : [65],
  "Tenor Sax" : [66],
  "Baritone Sax" : [67],
  "Oboe" : [68],
  "English Horn" : [69],
  "Bassoon" : [70],
  "Clarinet" : [71],
  "Piccolo" : [72],
  "Flute" : [73],
  "Recorder" : [74],
  "Pan Flute" : [75],
  "Blown bottle" : [76],
  "Shakuhachi" : [77],
  "Whistle" : [78],
  "Ocarina" : [79],
  "Lead 1 (square)" : [80],
  "Lead 2 (sawtooth)" : [81],
  "Lead 3 (calliope)" : [82],
  "Lead 4 (chiff)" : [83],
  "Lead 5 (charang)" : [84],
  "Lead 6 (voice)" : [85],
  "Lead 7 (fifths)" : [86],
  "Lead 8 (bass + lead)" : [87],
  "Pad 1 (new age)" : [88],
  "Pad 2 (warm)" : [89],
  "Pad 3 (polysynth)" : [90],
  "Pad 4 (choir)" : [91],
  "Pad 5 (bowed)" : [92],
  "Pad 6 (metallic)" : [93],
  "Pad 7 (halo)" : [94],
  "Pad 8 (sweep)" : [95],
  "FX 1 (rain)" : [96],
  "FX 2 (soundtrack)" : [97],
  "FX 3 (crystal)" : [98],
  "FX 4 (atmosphere)" : [99],
  "FX 5 (brightness)" : [100],
  "FX 6 (goblins)" : [101],
  "FX 7 (echoes)" : [102],
  "FX 8 (sci-fi)" : [103],
  "Sitar" : [104],
  "Banjo" : [105],
  "Shamisen" : [106],
  "Koto" : [107],
  "Kalimba" : [108],
  "Bag pipe" : [109],
  "Fiddle" : [110],
  "Shanai" : [111],
  "Tinkle Bell" : [112],
  "Agogo" : [113],
  "Steel Drums" : [114],
  "Woodblock" : [115],
  "Taiko Drum" : [116],
  "Melodic Tom" : [117],
  "Synth Drum" : [118],
  "Reverse Cymbal" : [119],
  "Guitar Fret Noise" : [120],
  "Breath Noise" : [121],
  "Seashore" : [122],
  "Bird Tweet" : [123],
  "Telephone Ring" : [124],
  "Helicopter" : [125],
  "Applause" : [126],
  "Gunshot" : [127],

  "drum_0" : [128],
  "drum_1" : [129],
  "drum_2" : [130],
  "drum_3" : [131],
  "drum_4" : [132],
  "drum_5" : [133],
  "drum_6" : [134],
  "drum_7" : [135],
  "drum_8" : [136],
  "drum_9" : [137],
  "drum_10" : [138],
  "drum_11" : [139],
  "drum_12" : [140],
  "drum_13" : [141],
  "drum_14" : [142],
  "drum_15" : [143],
  "drum_16" : [144],
  "drum_17" : [145],
  "drum_18" : [146],
  "drum_19" : [147],
  "drum_20" : [148],
  "drum_21" : [149],
  "drum_22" : [150],
  "drum_23" : [151],
  "drum_24" : [152],
  "drum_25" : [153],
  "drum_26" : [154],
  "drum_27" : [155],
  "drum_28" : [156],
  "drum_29" : [157],
  "drum_30" : [158],
  "drum_31" : [159],
  "drum_32" : [160],
  "drum_33" : [161],
  "drum_34" : [162],
  "drum_35" : [163],
  "drum_36" : [164],
  "drum_37" : [165],
  "drum_38" : [166],
  "drum_39" : [167],
  "drum_40" : [168],
  "drum_41" : [169],
  "drum_42" : [170],
  "drum_43" : [171],
  "drum_44" : [172],
  "drum_45" : [173],
  "drum_46" : [174],
  "drum_47" : [175],
  "drum_48" : [176],
  "drum_49" : [177],
  "drum_50" : [178],
  "drum_51" : [179],
  "drum_52" : [180],
  "drum_53" : [181],
  "drum_54" : [182],
  "drum_55" : [183],
  "drum_56" : [184],
  "drum_57" : [185],
  "drum_58" : [186],
  "drum_59" : [187],
  "drum_60" : [188],
  "drum_61" : [189],
  "drum_62" : [190],
  "drum_63" : [191],
  "drum_64" : [192],
  "drum_65" : [193],
  "drum_66" : [194],
  "drum_67" : [195],
  "drum_68" : [196],
  "drum_69" : [197],
  "drum_70" : [198],
  "drum_71" : [199],
  "drum_72" : [200],
  "drum_73" : [201],
  "drum_74" : [202],
  "drum_75" : [203],
  "drum_76" : [204],
  "drum_77" : [205],
  "drum_78" : [206],
  "drum_79" : [207],
  "drum_80" : [208],
  "drum_81" : [209],
  "drum_82" : [210],
  "drum_83" : [211],
  "drum_84" : [212],
  "drum_85" : [213],
  "drum_86" : [214],
  "drum_87" : [215],
  "drum_88" : [216],
  "drum_89" : [217],
  "drum_90" : [218],
  "drum_91" : [219],
  "drum_92" : [220],
  "drum_93" : [221],
  "drum_94" : [222],
  "drum_95" : [223],
  "drum_96" : [224],
  "drum_97" : [225],
  "drum_98" : [226],
  "drum_99" : [227],
  "drum_100" : [228],
  "drum_101" : [229],
  "drum_102" : [230],
  "drum_103" : [231],
  "drum_104" : [232],
  "drum_105" : [233],
  "drum_106" : [234],
  "drum_107" : [235],
  "drum_108" : [236],
  "drum_109" : [237],
  "drum_110" : [238],
  "drum_111" : [239],
  "drum_112" : [240],
  "drum_113" : [241],
  "drum_114" : [242],
  "drum_115" : [243],
  "drum_116" : [244],
  "drum_117" : [245],
  "drum_118" : [246],
  "drum_119" : [247],
  "drum_120" : [248],
  "drum_121" : [249],
  "drum_122" : [250],
  "drum_123" : [251],
  "drum_124" : [252],
  "drum_125" : [253],
  "drum_126" : [254],
  "drum_127" : [255],
}

type_map = {
  (True,True) : "STANDARD_BOTH",
  (True,False) : "STANDARD_TRACK",
  (False,True) : "STANDARD_DRUM_TRACK"
}

def clean_gm_name(x):
  x = x.lower()
  x = x.replace(" ", "_")
  x = x.replace("-", "_")
  x = x.replace("(", "")
  x = x.replace(")", "")
  x = x.replace("+", "")
  return x

GENERAL_MIDI_MAP_FULL = {clean_gm_name(k):v for k,v in GENERAL_MIDI_MAP_FULL.items()}

TRACK_TYPE_MAP = {k:type_map[(np.any(np.array(v)<128),np.any(np.array(v)>=128))] for k,v in GENERAL_MIDI_MAP_FULL.items()}




header_path = "../src/mmm_api/enum/gm.h"

proto_path = "../src/mmm_api/protobuf/enum.proto"

with open(proto_path, "w") as f:
  f.write("""syntax = "proto2";

package midi;

""")

  f.write("enum GM_TYPE {\n");
  for ii,(k,v) in enumerate(GENERAL_MIDI_MAP_FULL.items()):
    f.write('\t{} = {};\n'.format(k,ii))
  f.write("};\n\n")

with open(header_path, "w") as f:
  f.write("""#pragma once

#include <string>
#include <map>
#include <vector>
#include "track_type.h"
#include "../protobuf/midi.pb.h"

// START OF NAMESPACE
namespace mmm {

""")

  f.write("std::map<midi::GM_TYPE,std::vector<int>> GM = {\n")
  for k,v in GENERAL_MIDI_MAP_FULL.items():
    f.write('\t{{midi::GM_TYPE::{},{{{}}}}},\n'.format(k,",".join([str(_) for _ in v])))
  f.write("};\n\n")

  f.write("std::map<midi::GM_TYPE,std::vector<int>> GM_MOD = {\n")
  for k,v in GENERAL_MIDI_MAP_FULL.items():
    f.write('\t{{midi::GM_TYPE::{},{{{}}}}},\n'.format(k,",".join([str(_%128) for _ in v])))
  f.write("};\n\n")

  f.write("std::map<int,midi::GM_TYPE> GM_REV = {\n")
  for k,v in GENERAL_MIDI_MAP_FULL.items():
    if len(v) == 1:
      f.write('\t{{{},midi::GM_TYPE::{}}},\n'.format(v[0],k))
  f.write("};\n\n")

  #f.write("std::set<std::string> GM_KEYS = {\n")
  #for k,v in GENERAL_MIDI_MAP_FULL.items():
  #  f.write('\t"{}",\n'.format(k))
  #f.write("};\n\n")

  f.write("std::map<midi::GM_TYPE,midi::TRACK_TYPE> GM_TO_TRACK_TYPE = {\n")
  for k,v in TRACK_TYPE_MAP.items():
    f.write('\t{{midi::GM_TYPE::{},midi::TRACK_TYPE::{}}},\n'.format(k,v))
  f.write("};")

  f.write("""

}
// END OF NAMESPACE
""")

