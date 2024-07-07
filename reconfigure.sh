set -e

cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=ninja -G Ninja -DCMAKE_CXX_COMPILER=clang++ -S . -B cmake_build_debug
