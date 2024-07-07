set -e

clang-format -i src/*.cpp
clang-format -i src/*.h

cmake --build cmake_build_debug --target unknown_wm -j 10
