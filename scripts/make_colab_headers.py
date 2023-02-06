

def clean_gm_name(x):
  x = x.lower()
  x = x.replace(" ", "_")
  x = x.replace("-", "_")
  x = x.replace("(", "")
  x = x.replace(")", "")
  x = x.replace("+", "")
  return x


INST_MAP = {'Acoustic Grand Piano': 0, 'Bright Acoustic Piano': 1, 'Electric Grand Piano': 2, 'Honky-tonk Piano': 3, 'Electric Piano 1': 4, 'Electric Piano 2': 5, 'Harpsichord': 6, 'Clavi': 7, 'Celesta': 8, 'Glockenspiel': 9, 'Music Box': 10, 'Vibraphone': 11, 'Marimba': 12, 'Xylophone': 13, 'Tubular Bells': 14, 'Dulcimer': 15, 'Drawbar Organ': 16, 'Percussive Organ': 17, 'Rock Organ': 18, 'Church Organ': 19, 'Reed Organ': 20, 'Accordion': 21, 'Harmonica': 22, 'Tango Accordion': 23, 'Acoustic Guitar (nylon)': 24, 'Acoustic Guitar (steel)': 25, 'Electric Guitar (jazz)': 26, 'Electric Guitar (clean)': 27, 'Electric Guitar (muted)': 28, 'Overdriven Guitar': 29, 'Distortion Guitar': 30, 'Guitar Harmonics': 31, 'Acoustic Bass': 32, 'Electric Bass (finger)': 33, 'Electric Bass (pick)': 34, 'Fretless Bass': 35, 'Slap Bass 1': 36, 'Slap Bass 2': 37, 'Synth Bass 1': 38, 'Synth Bass 2': 39, 'Violin': 40, 'Viola': 41, 'Cello': 42, 'Contrabass': 43, 'Tremolo Strings': 44, 'Pizzicato Strings': 45, 'Orchestral Harp': 46, 'Timpani': 47, 'String Ensemble 1': 48, 'String Ensemble 2': 49, 'Synth Strings 1': 50, 'Synth Strings 2': 51, 'Choir Aahs': 52, 'Voice Oohs': 53, 'Synth Voice': 54, 'Orchestra Hit': 55, 'Trumpet': 56, 'Trombone': 57, 'Tuba': 58, 'Muted Trumpet': 59, 'French Horn': 60, 'Brass Section': 61, 'Synth Brass 1': 62, 'Synth Brass 2': 63, 'Soprano Sax': 64, 'Alto Sax': 65, 'Tenor Sax': 66, 'Baritone Sax': 67, 'Oboe': 68, 'English Horn': 69, 'Bassoon': 70, 'Clarinet': 71, 'Piccolo': 72, 'Flute': 73, 'Recorder': 74, 'Pan Flute': 75, 'Blown bottle': 76, 'Shakuhachi': 77, 'Whistle': 78, 'Ocarina': 79, 'Lead 1 (square)': 80, 'Lead 2 (sawtooth)': 81, 'Lead 3 (calliope)': 82, 'Lead 4 (chiff)': 83, 'Lead 5 (charang)': 84, 'Lead 6 (voice)': 85, 'Lead 7 (fifths)': 86, 'Lead 8 (bass + lead)': 87, 'Pad 1 (new age)': 88, 'Pad 2 (warm)': 89, 'Pad 3 (polysynth)': 90, 'Pad 4 (choir)': 91, 'Pad 5 (bowed)': 92, 'Pad 6 (metallic)': 93, 'Pad 7 (halo)': 94, 'Pad 8 (sweep)': 95, 'FX 1 (rain)': 96, 'FX 2 (soundtrack)': 97, 'FX 3 (crystal)': 98, 'FX 4 (atmosphere)': 99, 'FX 5 (brightness)': 100, 'FX 6 (goblins)': 101, 'FX 7 (echoes)': 102, 'FX 8 (sci-fi)': 103, 'Sitar': 104, 'Banjo': 105, 'Shamisen': 106, 'Koto': 107, 'Kalimba': 108, 'Bag pipe': 109, 'Fiddle': 110, 'Shanai': 111, 'Tinkle Bell': 112, 'Agogo': 113, 'Steel Drums': 114, 'Woodblock': 115, 'Taiko Drum': 116, 'Melodic Tom': 117, 'Synth Drum': 118, 'Reverse Cymbal': 119, 'Guitar Fret Noise': 120, 'Breath Noise': 121, 'Seashore': 122, 'Bird Tweet': 123, 'Telephone Ring': 124, 'Helicopter': 125, 'Applause': 126, 'Gunshot': 127, 'drum_0': 0, 'drum_1': 1, 'drum_2': 2, 'drum_3': 3, 'drum_4': 4, 'drum_5': 5, 'drum_6': 6, 'drum_7': 7, 'drum_8': 8, 'drum_9': 9, 'drum_10': 10, 'drum_11': 11, 'drum_12': 12, 'drum_13': 13, 'drum_14': 14, 'drum_15': 15, 'drum_16': 16, 'drum_17': 17, 'drum_18': 18, 'drum_19': 19, 'drum_20': 20, 'drum_21': 21, 'drum_22': 22, 'drum_23': 23, 'drum_24': 24, 'drum_25': 25, 'drum_26': 26, 'drum_27': 27, 'drum_28': 28, 'drum_29': 29, 'drum_30': 30, 'drum_31': 31, 'drum_32': 32, 'drum_33': 33, 'drum_34': 34, 'drum_35': 35, 'drum_36': 36, 'drum_37': 37, 'drum_38': 38, 'drum_39': 39, 'drum_40': 40, 'drum_41': 41, 'drum_42': 42, 'drum_43': 43, 'drum_44': 44, 'drum_45': 45, 'drum_46': 46, 'drum_47': 47, 'drum_48': 48, 'drum_49': 49, 'drum_50': 50, 'drum_51': 51, 'drum_52': 52, 'drum_53': 53, 'drum_54': 54, 'drum_55': 55, 'drum_56': 56, 'drum_57': 57, 'drum_58': 58, 'drum_59': 59, 'drum_60': 60, 'drum_61': 61, 'drum_62': 62, 'drum_63': 63, 'drum_64': 64, 'drum_65': 65, 'drum_66': 66, 'drum_67': 67, 'drum_68': 68, 'drum_69': 69, 'drum_70': 70, 'drum_71': 71, 'drum_72': 72, 'drum_73': 73, 'drum_74': 74, 'drum_75': 75, 'drum_76': 76, 'drum_77': 77, 'drum_78': 78, 'drum_79': 79, 'drum_80': 80, 'drum_81': 81, 'drum_82': 82, 'drum_83': 83, 'drum_84': 84, 'drum_85': 85, 'drum_86': 86, 'drum_87': 87, 'drum_88': 88, 'drum_89': 89, 'drum_90': 90, 'drum_91': 91, 'drum_92': 92, 'drum_93': 93, 'drum_94': 94, 'drum_95': 95, 'drum_96': 96, 'drum_97': 97, 'drum_98': 98, 'drum_99': 99, 'drum_100': 100, 'drum_101': 101, 'drum_102': 102, 'drum_103': 103, 'drum_104': 104, 'drum_105': 105, 'drum_106': 106, 'drum_107': 107, 'drum_108': 108, 'drum_109': 109, 'drum_110': 110, 'drum_111': 111, 'drum_112': 112, 'drum_113': 113, 'drum_114': 114, 'drum_115': 115, 'drum_116': 116, 'drum_117': 117, 'drum_118': 118, 'drum_119': 119, 'drum_120': 120, 'drum_121': 121, 'drum_122': 122, 'drum_123': 123, 'drum_124': 124, 'drum_125': 125, 'drum_126': 126, 'drum_127': 127}

REV_INST_MAP = {0: 'Acoustic Grand Piano', 1: 'Bright Acoustic Piano', 2: 'Electric Grand Piano', 3: 'Honky-tonk Piano', 4: 'Electric Piano 1', 5: 'Electric Piano 2', 6: 'Harpsichord', 7: 'Clavi', 8: 'Celesta', 9: 'Glockenspiel', 10: 'Music Box', 11: 'Vibraphone', 12: 'Marimba', 13: 'Xylophone', 14: 'Tubular Bells', 15: 'Dulcimer', 16: 'Drawbar Organ', 17: 'Percussive Organ', 18: 'Rock Organ', 19: 'Church Organ', 20: 'Reed Organ', 21: 'Accordion', 22: 'Harmonica', 23: 'Tango Accordion', 24: 'Acoustic Guitar (nylon)', 25: 'Acoustic Guitar (steel)', 26: 'Electric Guitar (jazz)', 27: 'Electric Guitar (clean)', 28: 'Electric Guitar (muted)', 29: 'Overdriven Guitar', 30: 'Distortion Guitar', 31: 'Guitar Harmonics', 32: 'Acoustic Bass', 33: 'Electric Bass (finger)', 34: 'Electric Bass (pick)', 35: 'Fretless Bass', 36: 'Slap Bass 1', 37: 'Slap Bass 2', 38: 'Synth Bass 1', 39: 'Synth Bass 2', 40: 'Violin', 41: 'Viola', 42: 'Cello', 43: 'Contrabass', 44: 'Tremolo Strings', 45: 'Pizzicato Strings', 46: 'Orchestral Harp', 47: 'Timpani', 48: 'String Ensemble 1', 49: 'String Ensemble 2', 50: 'Synth Strings 1', 51: 'Synth Strings 2', 52: 'Choir Aahs', 53: 'Voice Oohs', 54: 'Synth Voice', 55: 'Orchestra Hit', 56: 'Trumpet', 57: 'Trombone', 58: 'Tuba', 59: 'Muted Trumpet', 60: 'French Horn', 61: 'Brass Section', 62: 'Synth Brass 1', 63: 'Synth Brass 2', 64: 'Soprano Sax', 65: 'Alto Sax', 66: 'Tenor Sax', 67: 'Baritone Sax', 68: 'Oboe', 69: 'English Horn', 70: 'Bassoon', 71: 'Clarinet', 72: 'Piccolo', 73: 'Flute', 74: 'Recorder', 75: 'Pan Flute', 76: 'Blown bottle', 77: 'Shakuhachi', 78: 'Whistle', 79: 'Ocarina', 80: 'Lead 1 (square)', 81: 'Lead 2 (sawtooth)', 82: 'Lead 3 (calliope)', 83: 'Lead 4 (chiff)', 84: 'Lead 5 (charang)', 85: 'Lead 6 (voice)', 86: 'Lead 7 (fifths)', 87: 'Lead 8 (bass + lead)', 88: 'Pad 1 (new age)', 89: 'Pad 2 (warm)', 90: 'Pad 3 (polysynth)', 91: 'Pad 4 (choir)', 92: 'Pad 5 (bowed)', 93: 'Pad 6 (metallic)', 94: 'Pad 7 (halo)', 95: 'Pad 8 (sweep)', 96: 'FX 1 (rain)', 97: 'FX 2 (soundtrack)', 98: 'FX 3 (crystal)', 99: 'FX 4 (atmosphere)', 100: 'FX 5 (brightness)', 101: 'FX 6 (goblins)', 102: 'FX 7 (echoes)', 103: 'FX 8 (sci-fi)', 104: 'Sitar', 105: 'Banjo', 106: 'Shamisen', 107: 'Koto', 108: 'Kalimba', 109: 'Bag pipe', 110: 'Fiddle', 111: 'Shanai', 112: 'Tinkle Bell', 113: 'Agogo', 114: 'Steel Drums', 115: 'Woodblock', 116: 'Taiko Drum', 117: 'Melodic Tom', 118: 'Synth Drum', 119: 'Reverse Cymbal', 120: 'Guitar Fret Noise', 121: 'Breath Noise', 122: 'Seashore', 123: 'Bird Tweet', 124: 'Telephone Ring', 125: 'Helicopter', 126: 'Applause', 127: 'Gunshot', 128: 'drum_0', 129: 'drum_1', 130: 'drum_2', 131: 'drum_3', 132: 'drum_4', 133: 'drum_5', 134: 'drum_6', 135: 'drum_7', 136: 'drum_8', 137: 'drum_9', 138: 'drum_10', 139: 'drum_11', 140: 'drum_12', 141: 'drum_13', 142: 'drum_14', 143: 'drum_15', 144: 'drum_16', 145: 'drum_17', 146: 'drum_18', 147: 'drum_19', 148: 'drum_20', 149: 'drum_21', 150: 'drum_22', 151: 'drum_23', 152: 'drum_24', 153: 'drum_25', 154: 'drum_26', 155: 'drum_27', 156: 'drum_28', 157: 'drum_29', 158: 'drum_30', 159: 'drum_31', 160: 'drum_32', 161: 'drum_33', 162: 'drum_34', 163: 'drum_35', 164: 'drum_36', 165: 'drum_37', 166: 'drum_38', 167: 'drum_39', 168: 'drum_40', 169: 'drum_41', 170: 'drum_42', 171: 'drum_43', 172: 'drum_44', 173: 'drum_45', 174: 'drum_46', 175: 'drum_47', 176: 'drum_48', 177: 'drum_49', 178: 'drum_50', 179: 'drum_51', 180: 'drum_52', 181: 'drum_53', 182: 'drum_54', 183: 'drum_55', 184: 'drum_56', 185: 'drum_57', 186: 'drum_58', 187: 'drum_59', 188: 'drum_60', 189: 'drum_61', 190: 'drum_62', 191: 'drum_63', 192: 'drum_64', 193: 'drum_65', 194: 'drum_66', 195: 'drum_67', 196: 'drum_68', 197: 'drum_69', 198: 'drum_70', 199: 'drum_71', 200: 'drum_72', 201: 'drum_73', 202: 'drum_74', 203: 'drum_75', 204: 'drum_76', 205: 'drum_77', 206: 'drum_78', 207: 'drum_79', 208: 'drum_80', 209: 'drum_81', 210: 'drum_82', 211: 'drum_83', 212: 'drum_84', 213: 'drum_85', 214: 'drum_86', 215: 'drum_87', 216: 'drum_88', 217: 'drum_89', 218: 'drum_90', 219: 'drum_91', 220: 'drum_92', 221: 'drum_93', 222: 'drum_94', 223: 'drum_95', 224: 'drum_96', 225: 'drum_97', 226: 'drum_98', 227: 'drum_99', 228: 'drum_100', 229: 'drum_101', 230: 'drum_102', 231: 'drum_103', 232: 'drum_104', 233: 'drum_105', 234: 'drum_106', 235: 'drum_107', 236: 'drum_108', 237: 'drum_109', 238: 'drum_110', 239: 'drum_111', 240: 'drum_112', 241: 'drum_113', 242: 'drum_114', 243: 'drum_115', 244: 'drum_116', 245: 'drum_117', 246: 'drum_118', 247: 'drum_119', 248: 'drum_120', 249: 'drum_121', 250: 'drum_122', 251: 'drum_123', 252: 'drum_124', 253: 'drum_125', 254: 'drum_126', 255: 'drum_127'}

print({clean_gm_name(k):v for k,v in INST_MAP.items()})

print({k:clean_gm_name(v) for k,v in REV_INST_MAP.items()})