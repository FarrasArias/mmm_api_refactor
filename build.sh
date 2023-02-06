cd src/mmm_api/protobuf
protoc --cpp_out . *.proto
cd ../../..
cd src/mmm_api
python3 help.py
cd ../..
rm -rf build
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` ..
cmake --build . --config Release
cd ..