CMAKE_MINIMUM_REQUIRED(VERSION 3.8.2 FATAL_ERROR)
PROJECT(mmm_api)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

INCLUDE_DIRECTORIES(midifile/include)
SET(SRCS
    midifile/src/Options.cpp
    midifile/src/Binasc.cpp
    midifile/src/MidiEvent.cpp
    midifile/src/MidiEventList.cpp
    midifile/src/MidiFile.cpp
    midifile/src/MidiMessage.cpp
)

SET(HDRS
    midifile/include/Binasc.h
    midifile/include/MidiEvent.h
    midifile/include/MidiEventList.h
    midifile/include/MidiFile.h
    midifile/include/MidiMessage.h
    midifile/include/Options.h
)

ADD_LIBRARY(midifile STATIC ${SRCS} ${HDRS})

FIND_PACKAGE(Protobuf REQUIRED)
INCLUDE_DIRECTORIES(${Protobuf_INCLUDE_DIRS})
FILE(GLOB PROTO_DEF "src/mmm_api/protobuf/*.proto")
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER ${PROTO_DEF})
ADD_LIBRARY(proto ${PROTO_HEADER} ${PROTO_SRC})

FIND_PACKAGE(Torch REQUIRED)

add_executable(mmm_api 
  src/mmm_api/api.cpp 
  src/mmm_api/protobuf/midi.pb.cc)
TARGET_LINK_LIBRARIES(
  mmm_api PUBLIC midifile proto ${Protobuf_LIBRARIES} ${TORCH_LIBRARIES})