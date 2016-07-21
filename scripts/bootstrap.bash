#!/usr/bin/env bash
source "scripts/common.bash"

# Control Begins Here !!
full_clean

# Now let's rebuild everything.
mkdir -p ${BUILD}

cat > "${ROOT}/CMakeLists.txt" << "EOF"
project(BoomHS)
cmake_minimum_required(VERSION 2.8.12)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_COMPILER "clang++")

add_executable(boomhs main.cxx)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
target_link_libraries(boomhs ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})

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

function link_script() {
  ln -fs scripts/$1 ./$2
}

# Usage is "bb", "bbc", and "bbr"
link_script bb.bash
link_script bbc.bash
link_script bbr.bash
