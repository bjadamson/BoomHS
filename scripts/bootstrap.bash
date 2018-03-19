#!/usr/bin/env bash
source "scripts/common.bash"

# Control Begins Here !!
full_clean
mkdir -p ${BUILD}

STATIC_ANALYSIS_FLAGS=""
DEBUG_OR_RELEASE="Debug"

while getopts ":ahr" opt; do
  case ${opt} in
    a )
      export STATIC_ANALYSIS_FLAGS="-fsanitize=address"
      ;;
    r )
      export DEBUG_OR_RELEASE="Release"
      ;;
    \h )
      echo "Help options for bootstrapping process."
      echo "[-a] To enable Static Analysis."
      echo "[-r] To switch from Debug to Release mode."

      echo "[-h] See this message."
      echo "Please run again without the -h flag."
      echo "Quitting now."
      exit
      ;;
  esac
done
shift $((OPTIND -1))

echo "Configuring project ..."
echo "DEBUG/RELEASE: $DEBUG_OR_RELEASE"
printf "Static Analysis: (ON|OFF): "
if [ -z "$STATIC_ANALYSIS_FLAGS" ]; then
  echo "OFF"
else
  echo "ON"
fi

# Now let's rebuild everything.
cat > "${ROOT}/CMakeLists.txt" << "EOF"
project(BoomHS)
cmake_minimum_required(VERSION 3.4.3)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(MY_EXTRA_FLAGS "-Wno-unused-variable -Wno-missing-braces -Wno-unused-parameter")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} STATIC_ANALYSIS_FLAGS_PLACEHOLDER -fno-omit-frame-pointer -Wall -Wextra -g -O0 ${MY_EXTRA_FLAGS} ")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++17 -stdlib=libc++")


set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTERNAL_DIR ${PROJECT_DIR}/external)
set(TOOLS_DIRECTORY ${PROJECT_DIR}/tools/)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

set(IMGUI_INCLUDE_DIR "${EXTERNAL_DIR}/imgui/include/imgui")
set(CPPTOML_INCLUDE_DIR "${EXTERNAL_DIR}/cpptoml/include")

file(GLOB INTERNAL_INCLUDE_DIRS include external/**/include)

file(GLOB_RECURSE GLOBBED_SOURCES
  ${EXTERNAL_DIR}/**/*.cxx
  ${PROJECT_DIR}/source/*.cxx
  )

## Additional build targets CMake should generate.
set(BOOMHS_CODE ${PROJECT_DIR}/source/*.cxx ${PROJECT_DIR}/include/**/*.hpp)

add_custom_target(cppformat COMMAND clang-format -i ${BOOMHS_CODE})
add_custom_target(clangcheck COMMAND clang-check -analyze -p ${BUILD} -s ${GLOBBED_SOURCES_CLANG_TOOLS})

## Declare our executable and build it.
add_executable(build_postprocessing ${TOOLS_DIRECTORY}/build_postprocessing.cxx)
add_executable(boomhs ${GLOBBED_SOURCES})

find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)
find_package(BFD REQUIRED)
find_package(ZLIB REQUIRED)

## Build the application
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)

## We should get these through conan.io
target_include_directories(boomhs PUBLIC
  ${CPPTOML_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
  ${SDL2IMAGE_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIR}
  ${INTERNAL_INCLUDE_DIRS}
  ${OPENGL_INDLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${SOIL_INCLUDE_DIR})

target_link_libraries(boomhs
  ${SDL2_LIBRARIES}
  stdc++
  ${SDL2IMAGE_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${SOIL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  bfd ## BFD and dl are both needed for linux backtraces.
  dl
  pthread
  boost_system)

target_include_directories(build_postprocessing PUBLIC ${INTERNAL_INCLUDE_DIRS})
target_link_libraries(build_postprocessing stdc++ c++experimental)
EOF

# Overwrite the placeholders in CMakeLists.txt
sed -i "s|STATIC_ANALYSIS_FLAGS_PLACEHOLDER|${STATIC_ANALYSIS_FLAGS}|g" ${ROOT}/CMakeLists.txt

cat > "${BUILD}/conanfile.txt" << "EOF"
[requires]
glm/0.9.8.0@TimSimpson/testing
Boost/1.60.0/lasote/stable

[generators]
cmake
EOF

cd ${BUILD}
echo $(pwd)
conan install --build missing -s compiler=clang -s arch=x86_64 -s compiler.version=6.0 -s compiler.libcxx=libc++ -s build_type=${DEBUG_OR_RELEASE}
cmake .. -G "Unix Makefiles"              \
  -DCMAKE_BUILD_TYPE=${DEBUG_OR_RELEASE}  \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd ..
