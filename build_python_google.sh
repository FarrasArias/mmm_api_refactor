#!/bin/bash
rm -rf python/src/mmm_api
rm -rf python/build
rm -rf python/mmm_api.egg-info
rm -rf python/dist

# copy srces
mkdir -p python/src
cp -a src/mmm_api python/src
cp -a midifile python/midifile

if [[ "$*" == *--torch* ]]
then
  cp CMakeLists_PY_TORCH.txt python/CMakeLists.txt 
else
  cp CMakeLists_PY.txt python/CMakeLists.txt
fi

if [[ "$*" == *--linux* ]]
then
  cp CMakeLists_PY_TORCH_LINUX.txt python/CMakeLists.txt
fi

if [[ "$*" == *--google* ]]
then
  cp CMakeLists_PY_TORCH_GOOGLE.txt python/CMakeLists.txt
fi

cd python

# generate protbuf
cd src/mmm_api/protobuf
protoc --cpp_out . *.proto
cd ../../..

# build extra code
cd src/mmm_api
python3 help.py 
cd ../..