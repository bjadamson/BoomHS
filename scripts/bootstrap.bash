#!/usr/bin/env bash
source "scripts/common.bash"

# Control Begins Here !!
full_clean

# Now let's rebuild everything.
mkdir -p ${BUILD}

cat > "${ROOT}/CMakeLists.txt" << "EOF"
project(BoomHS)
cmake_minimum_required(VERSION 3.4.3)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g -O0")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++17 -stdlib=libc++")
set(TOOLS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

set(ASSIMP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/assimp/include")
set(ASSIMP_LIBRARY_SO "${CMAKE_CURRENT_SOURCE_DIR}/external/assimp/lib/libassimp.so")

## DEFINITIONS
file(GLOB INTERNAL_INCLUDE_DIRS include
  external/assimp/include
  external/backward/include
  external/compact_optional/include
  external/expected/include
  external/hana/include
  external/ecst/include
  external/ecst/extlibs/vrm_core/include
  external/ecst/extlibs/vrm_pp/include
  external/fmt/include
  external/spdlog/include
  external/tinyobj/include)

file(GLOB_RECURSE GLOBBED_SOURCES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/external/backward/source/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cxx
  )

## Gather the source files in such a way that we can pass them to clang-format and other clang
file(GLOB_RECURSE GLOBBED_SOURCES_CLANG_TOOLS *.cxx *.hpp)
set (EXCLUDE_DIR "expected/")
foreach (TMP_PATH ${GLOBBED_SOURCES_CLANG_TOOLS})
    string (FIND ${TMP_PATH} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
        list (REMOVE_ITEM GLOBBED_SOURCES_CLANG_TOOLS ${TMP_PATH})
    endif ()
endforeach(TMP_PATH)

## Additional build targets CMake should generate.
add_custom_target(cppformat COMMAND clang-format -i ${GLOBBED_SOURCES_CLANG_TOOLS})
add_custom_target(clangcheck COMMAND clang-check -analyze -p ${BUILD} -s ${GLOBBED_SOURCES_CLANG_TOOLS})

## Declare our executable and build it.
add_executable(shader_loader ${TOOLS_DIRECTORY}/main_shaderloader.cxx)
add_executable(boomhs ${GLOBBED_SOURCES})

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)
find_package(BFD REQUIRED)

## Build the application
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)

## We should get these through conan.io
target_include_directories(boomhs PUBLIC
  ${ASSIMP_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
  ${SDL2IMAGE_INCLUDE_DIRS}
  ${INTERNAL_INCLUDE_DIRS}
  ${OPENGL_INDLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${SOIL_INCLUDE_DIR})

target_link_libraries(boomhs
  ${SDL2_LIBRARIES}
  stdc++
  ${ASSIMP_LIBRARY_SO}
  ${SDL2IMAGE_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${SOIL_LIBRARIES}
  bfd ## BFD and dl are both needed for linux backtraces.
  dl
  pthread
  boost_system)

target_include_directories(shader_loader PUBLIC ${INTERNAL_INCLUDE_DIRS})
target_link_libraries(shader_loader stdc++ c++experimental)
EOF

cat > "${BUILD}/conanfile.txt" << "EOF"
[requires]
glm/0.9.8.0@TimSimpson/testing
Boost/1.60.0/lasote/stable

[generators]
cmake
EOF

cd ${BUILD}
echo $(pwd)
conan install --build missing -s compiler=clang -s arch=x86 -s compiler.version=4.0 -s compiler.libcxx=libc++ -s build_type=Debug
cmake .. -G "Unix Makefiles"          \
  -DCMAKE_BUILD_TYPE=Debug            \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd ..

function link_script() {
  ln -fs scripts/$1 ./$2
}

# Usage is "bb", "bbc", and "bbr"
link_script bb.bash
link_script bbc.bash
link_script cbb.bash
link_script bbr.bash
