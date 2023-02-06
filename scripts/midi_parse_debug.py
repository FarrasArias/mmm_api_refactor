import mmm_api as mmm

encoder = mmm.TrackDensityEncoder()
x = encoder.midi_to_json("../test_midi/9_apollo_seeds.midi")
print(x)