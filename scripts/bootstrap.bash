#!/usr/bin/env bash
source "scripts/common.bash"

# Control Begins Here !!
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

# Detect and add SFML
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
find_package(SFML 2.0 REQUIRED system window graphics network audio)
target_link_libraries(boomhs ${CONAN_LIBS})
target_link_libraries(boomhs ${SFML_LIBRARIES})

EOF

cat > "${BUILD}/conanfile.txt" << "EOF"
[generators]
cmake
EOF

cd ${BUILD}
echo $(pwd)
conan install -s compiler=clang -s arch=x86 -s compiler.version=3.9
cmake .. -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=/usr/local/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/local/bin/clang++
cd ..

# Usage is "bb", "bbc", and "bbr"
ln -s scripts/bb.bash ./bb.bash
ln -s scripts/bbc.bash ./bbc.bash
ln -s scripts/bbr.bash ./bbr.bash
