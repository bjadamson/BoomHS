#!/usr/bin/env bash
source "scripts/common.bash"

# Control Begins Here !!
full_clean
mkdir -p ${BUILD}

STATIC_ANALYSIS_FLAGS=""
DEBUG_OR_RELEASE="Debug"
CXX_STD_LIBRARY="libc++"

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

# Generate a CMakeLists.txt file for CMake to use when executing a build.
cat > "${ROOT}/CMakeLists.txt" << "EOF"
###################################################################################################
###################################################################################################
## GENERATED FILE.
##
## Modifying this script by hand is not recommend. Modify script that generates this file instead
## (scripts/boostrap).
##
###################################################################################################
###################################################################################################

###################################################################################################
## SCRIPT BEGIN
project(BoomHS CXX)
cmake_minimum_required(VERSION 3.4.3)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

###################################################################################################
## Setup basic c++ flags.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

## Configure the "release" compiler settings
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++17 -stdlib=STDLIB_PLACEHOLDER")

## "Extra" flags
set(MY_EXTRA_FLAGS "-Wno-unused-variable -Wno-missing-braces -Wno-unused-parameter -fno-omit-frame-pointer")

## Configure the "default" compiler settings
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} STATIC_ANALYSIS_FLAGS_PLACEHOLDER -MJ -Wall -Wextra -g -O0 ${MY_EXTRA_FLAGS} ")


###################################################################################################
## Setup variables to different paths; used while issung the build commands further below.
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTERNAL_DIR ${PROJECT_DIR}/external)
set(TOOLS_DIRECTORY ${PROJECT_DIR}/tools/)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

set(IMGUI_INCLUDE_DIR "${EXTERNAL_DIR}/imgui/include/imgui")
set(CPPTOML_INCLUDE_DIR "${EXTERNAL_DIR}/cpptoml/include")

set(BOOMHS_INCLUDES ${PROJECT_DIR}/include/**/*.hpp)
set(BOOMHS_CODE     ${BOOMHS_INCLUDES} ${BOOMHS_SOURCES})

## BFD and dl are both needed for linux backtraces.
## BFD_LIB => Backtrace Libraries
set(BFD_LIB "bfd")
set(STACKTRACE_LIB "dw")

###################################################################################################
# Create lists of files to be used when issuing build commands further below.
##file(GLOB         EXTERNAL_INCLUDE_DIRS include ${EXTERNAL_DIR}/**/include)
##file(GLOB_RECURSE EXTERNAL_SOURCES              ${EXTERNAL_DIR}/**/source/*.cxx)

file(GLOB         EXTERNAL_INCLUDE_DIRS include external/**/include)
file(GLOB_RECURSE EXTERNAL_SOURCES      ${EXTERNAL_DIR}/**/*.cxx)

file(GLOB MAIN_SOURCE      ${PROJECT_DIR}/source/main.cxx)
file(GLOB BOOMHS_SOURCES   ${PROJECT_DIR}/source/boomhs/*.cxx)
file(GLOB OPENGL_SOURCES   ${PROJECT_DIR}/source/opengl/*.cxx)
file(GLOB STLW_SOURCES     ${PROJECT_DIR}/source/stlw/*.cxx)
file(GLOB WINDOW_SOURCES   ${PROJECT_DIR}/source/window/*.cxx)
file(GLOB GL_SDL_SOURCES   ${PROJECT_DIR}/source/gl_sdl/*.cxx)


###################################################################################################
## Manage External Dependencies
find_package(BFD REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(DL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)
find_package(ZLIB REQUIRED)

###################################################################################################
## Build-environment wide commands that need to be executed before the executables are declared
## below.
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)


###################################################################################################
## Static Libraries
###################################################################################################

## External_LIB
add_library(External_LIB STATIC ${EXTERNAL_SOURCES})
target_include_directories(External_LIB PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIR}
  ${SDL2_INCLUDE_DIRS}
  )

## Stlw_LIB
add_library(Stlw_LIB   STATIC ${STLW_SOURCES})
target_include_directories(Stlw_LIB PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  )

## BoomHS_LIB
add_library(BoomHS_LIB STATIC ${BOOMHS_SOURCES})
target_include_directories(BoomHS_LIB PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  )

## Opengl_LIB
add_library(Opengl_LIB STATIC ${OPENGL_SOURCES})
target_include_directories(Opengl_LIB PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  ${OPENGL_INDLUDE_DIRS}
  )

## Window_LIB
add_library(Window_LIB STATIC ${WINDOW_SOURCES})
target_include_directories(Window_LIB PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  )

## GL_SSTACKTRACE_LIB
add_library(GL_SSTACKTRACE_LIB STATIC ${GL_SDL_SOURCES})
target_include_directories(GL_SSTACKTRACE_LIB PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${OPENGL_INDLUDE_DIRS}
  )

###################################################################################################
## Specify which static libraries depend on each-other (for the linker's sake).
##
## The base game is dependent on ALL other libraries. The rest of the libraries need to have
## specified which other libraries they will link too here.
target_link_libraries(Stlw_LIB   External_LIB ${BFD_LIB} ${STACKTRACE_LIB})
target_link_libraries(BoomHS_LIB External_LIB Opengl_LIB Stlw_LIB Window_LIB GL_SSTACKTRACE_LIB ${BFD_LIB}
    ${STACKTRACE_LIB})

target_link_libraries(Opengl_LIB External_LIB Stlw_LIB ${BFD_LIB} ${STACKTRACE_LIB})
target_link_libraries(Window_LIB External_LIB Opengl_LIB Stlw_LIB ${BFD_LIB} ${STACKTRACE_LIB})
target_link_libraries(GL_SSTACKTRACE_LIB External_LIB Opengl_LIB Stlw_LIB Window_LIB ${BFD_LIB} ${STACKTRACE_LIB})

###################################################################################################
## Executables
###################################################################################################

###################################################################################################
## **1** Build Post-Processing Application
##
## Application that does post-processing on the OpenGL shader code (after the main executable has
## been compiled). The scripts included with the project automatically execute this application on
## your behalf when starting the main executable.
add_executable(BUILD_POSTPROCESSING ${TOOLS_DIRECTORY}/build_postprocessing.cxx)
target_include_directories(BUILD_POSTPROCESSING PUBLIC ${EXTERNAL_INCLUDE_DIRS})
target_link_libraries(     BUILD_POSTPROCESSING stdc++ c++experimental)

###################################################################################################
## **2** Main Executable
add_executable(boomhs ${MAIN_SOURCE})

target_link_libraries(boomhs
  ## External Directory
  External_LIB

  ## Internal Project Libraries
  BoomHS_LIB
  Stlw_LIB

  Opengl_LIB
  Window_LIB
  GL_SSTACKTRACE_LIB

  ## System Libraries
  stdc++
  audio
  pthread
  boost_system
  openal

  ## External Libraries
  ${BFD_LIB}
  ${STACKTRACE_LIB}
  ${SDL2_LIBRARIES}
  ${SDL2IMAGE_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${SOIL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  )

target_include_directories(boomhs PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  ${CPPTOML_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIR}
  ${LIBAUDIO_INCLUDE_DIRS}
  ${OPENGL_INDLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${SDL2IMAGE_INCLUDE_DIRS}
  ${SOIL_INCLUDE_DIR}
  )

## Additional build targets CMake should generate. These are useful for debugging or
## code-maintenance.
add_custom_target(cppformat  COMMAND clang-format -i                     ${BOOMHS_CODE})
add_custom_target(cpptidy    COMMAND clang-tidy                          ${BOOMHS_SOURCES})
add_custom_target(clangcheck COMMAND clang-check -analyze -p ${BUILD} -s ${GLOBBED_SOURCES_CLANG_TOOLS})

EOF

# Overwrite the placeholders in CMakeLists.txt
sed -i "s|STATIC_ANALYSIS_FLAGS_PLACEHOLDER|${STATIC_ANALYSIS_FLAGS}|g" ${ROOT}/CMakeLists.txt
sed -i "s|STDLIB_PLACEHOLDER|${CXX_STD_LIBRARY}|g"                      ${ROOT}/CMakeLists.txt


cat > "${BUILD}/conanfile.txt" << "EOF"
[requires]
glm/0.9.8.0@TimSimpson/testing
Boost/1.60.0/lasote/stable

[generators]
cmake
EOF

cd ${BUILD}
echo $(pwd)
conan install --build missing                                                                      \
  -s compiler=clang                                                                                \
  -s arch=x86_64                                                                                   \
  -s compiler.version=6.0                                                                          \
  -s compiler.libcxx=${CXX_STD_LIBRARY}                                                            \
  -s build_type=${DEBUG_OR_RELEASE}

cmake .. -G "Unix Makefiles"                                                                       \
  -DCMAKE_BUILD_TYPE=${DEBUG_OR_RELEASE}                                                           \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd ..
