#include <iostream>
#include <vector>
#include <tuple>
#include <map>
#include <set>

#include "../../midifile/include/MidiFile.h"
#include "lz4.h"
#include "midi.pb.h"

// need this for json out
#include <google/protobuf/util/json_util.h>

#include "encoder.h"
#include "encoder_types.h"

#include "skipgram_bloomfilter.h"

;

//cd src/dataset_builder_2; protoc --cpp_out . midi.proto; cd ../..; python3 setup.py install

// installing protobuf
// brew install protobuf ...
// export PKG_CONFIG_PATH=/usr/local/Cellar/protobuf/3.12.2/lib/pkgconfig
// pkg-config --cflags --libs protobuf
// protoc --cpp_out . midi.proto
// protoc --cpp_out src/dataset_builder_2 src/dataset_builder_2/midi.proto

//g++ token_bold.cpp ../../midifile/src/Binasc.cpp ../../midifile/src/MidiEvent.cpp ../../midifile/src/MidiEventList.cpp ../../midifile/src/MidiFile.cpp ../../midifile/src/MidiMessage.cpp midi.pb.cc lz4.c -I../../midifile/include -pthread -I/usr/local/Cellar/protobuf/3.12.2/include -L/usr/local/Cellar/protobuf/3.12.2/lib -lprotobuf --std=c++11; ./a.out


