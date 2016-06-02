#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

# Control Begins Here !!
unlink ./bb.bash
unlink ./bbc.bash
unlink ./bbr.bash
rm -rf ${BUILD}
rm -f "CMakeLists.txt"

# Now let's rebuild everything.
mkdir -p ${BUILD}

cat > "${ROOT}/CMakeLists.txt" << "EOF"
project(BoomHS)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(boomhs main.cxx)
target_link_libraries(boomhs ${CONAN_LIBS})
EOF

cat > "${BUILD}/conanfile.txt" << "EOF"
[generators]
cmake
EOF

cd ${BUILD}
echo $(pwd)
conan install -s compiler=clang -s arch=x86 -s compiler.version=3.8
cmake .. -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++
cd ..

# Usage is "bb", "bbc", and "bbr"
ln -s scripts/bb.bash ./bb.bash
ln -s scripts/bbc.bash ./bbc.bash
ln -s scripts/bbr.bash ./bbr.bash
