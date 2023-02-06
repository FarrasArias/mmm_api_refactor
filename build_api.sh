cp CMakeLists_API.txt CMakeLists.txt

rm -rf build
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'` ..
cmake --build . --config Release
cd ..

cp CMakeLists_UNIT.txt CMakeLists.txt # replace the default one