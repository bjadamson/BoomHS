#!/usr/bin/env bash
source "scripts/common.bash"

full_clean
mkdir -p ${BUILD}

echo "Configuring project ..."
source "scripts/helper_load_userargs.bash"

echo "DEBUG/RELEASE: $DEBUG_OR_RELEASE"
echo "Compiler: $COMPILER"
echo "cxxstdlibrary: $CXX_STD_LIBRARY"
echo "compiler specific flags: $COMPILER_SPECIFIC_FLAGS"

printf "Static Analysis: (ON|OFF): "
if [ -z "$STATIC_ANALYSIS_FLAGS" ]; then
  echo "OFF"
else
  echo "ON"
fi

printf "Build System: "
echo $BUILD_SYSTEM

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
## Setup variables to different paths; used while issung the build commands further below.
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXTERNAL_DIR ${PROJECT_DIR}/external)
set(TOOLS_DIRECTORY ${PROJECT_DIR}/tools)
set(TEST_DIRECTORY ${PROJECT_DIR}/test)
set(DEMO_DIRECTORY ${PROJECT_DIR}/demo)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

set(DEMO_DIRECTORY_INCLUDE_DIR ${PROJECT_DIR}/demo/include)
set(DEMO_DIRECTORY_SOURCE_DIR  ${PROJECT_DIR}/demo/source)

set(IMGUI_INCLUDE_DIR         "${EXTERNAL_DIR}/imgui/include/imgui")
set(CPPTOML_INCLUDE_DIR       "${EXTERNAL_DIR}/cpptoml/include")
set(STATIC_STRING_INCLUDE_DIR "${EXTERNAL_DIR}/static_string/include")
set(CMAKE_PREFIX_PATH         "${EXTERNAL_DIR}")

set(INCLUDE_DIRECTORY ${PROJECT_DIR}/include)
set(SOURCES_DIRECTORY ${PROJECT_DIR}/source)

set(BOOMHS_INCLUDES ${INCLUDE_DIRECTORY}/**/*.hpp)
set(BOOMHS_SOURCES  ${SOURCES_DIRECTORY}/**/*.cxx)
set(BOOMHS_CODE     ${BOOMHS_INCLUDES} ${BOOMHS_SOURCES})

## BFD_LIB and STACKTRACE_LIB are both needed for linux backtraces.
set(BFD_LIB        "bfd")
set(STACKTRACE_LIB "dw")

###################################################################################################
# Create lists of files to be used when issuing build commands further below.
file(GLOB         EXTERNAL_INCLUDE_DIRS
                  include external/**/include
                  external/boost/libs/**/include/)

file(GLOB_RECURSE EXTERNAL_SOURCES
        ${EXTERNAL_DIR}/backward/*.cxx
        ${EXTERNAL_DIR}/imgui/*.cxx
        ${EXTERNAL_DIR}/tinyobj/*.cxx
        ${EXTERNAL_DIR}/fastnoise/*.cxx)

file(GLOB         MAIN_SOURCE_FILE         ${PROJECT_DIR}/source/main.cxx)
file(GLOB_RECURSE SUBDIR_SOURCE_FILES      ${PROJECT_DIR}/source/**/*.cxx)
file(GLOB         DEMO_COMMON_SOURCE_FILES ${PROJECT_DIR}/demo/source/*.cxx)

set(SUBDIR_AND_EXTERNAL_SOURCE_FILES ${EXTERNAL_SOURCES}                 ${SUBDIR_SOURCE_FILES})
set(DEMO_AND_EXTERNAL_SOURCE_FILES   ${EXTERNAL_SOURCES}                 ${DEMO_COMMON_SOURCE_FILES})
set(ALL_SOURCE_FILES                 ${SUBDIR_AND_EXTERNAL_SOURCE_FILES} ${MAIN_SOURCE_FILE})
set(ALL_HEADER_FILES                 ${PROJECT_DIR}/include/**/*.hpp     ${EXTERNAL_DIR}/**/*.hpp)

###################################################################################################
## Manage External Dependencies
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${PROJECT_DIR}/external)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${PROJECT_DIR}/external)

find_package(BFD REQUIRED)
find_package(DL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SOIL REQUIRED)
find_package(ZLIB REQUIRED)

###################################################################################################
## Setup basic c++ flags.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

## Configure the "release" compiler settings
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++17")

## Compiler Flags
set(MY_EXTRA_COMPILER_FLAGS "                                                                      \
    -Wno-unused-variable                                                                           \
    -Wno-missing-braces                                                                            \
    -Wno-unused-parameter                                                                          \
    -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}                                                \
    STATIC_ANALYSIS_FLAGS_PLACEHOLDER                                                              \
    ${COMPILER_SPECIFIC_FLAGS}                                                                     \
    -Wall                                                                                          \
    -Wextra                                                                                        \
    -g -O0                                                                                         \
    ${MY_EXTRA_COMPILER_FLAGS}                                                                     \
    ")

## Linker Flags
## set(MY_EXTRA_LINKER_FLAGS "-static")
## set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MY_EXTRA_LINKER_FLAGS}" )

###################################################################################################
## Build-environment wide commands that need to be executed before the executables are declared
## below.
include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)


###################################################################################################
## Static Libraries
###################################################################################################

add_library(PROJECT_SOURCE_CODE     STATIC ${SUBDIR_AND_EXTERNAL_SOURCE_FILES})
add_library(DEMO_COMMON_SOURCE_CODE STATIC ${DEMO_AND_EXTERNAL_SOURCE_FILES})

target_include_directories(DEMO_COMMON_SOURCE_CODE PUBLIC
  ${DEMO_DIRECTORY_INCLUDE_DIR}
  ${EXTERNAL_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIR}
  ${OPENGL_INDLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${SDL2IMAGE_INCLUDE_DIRS}
  ${SOIL_INCLUDE_DIR}
  ${STATIC_STRING_INCLUDE_DIR}
  )

target_include_directories(PROJECT_SOURCE_CODE PUBLIC
  ${EXTERNAL_INCLUDE_DIRS}
  ${CPPTOML_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIR}
  ${LIBAUDIO_INCLUDE_DIRS}
  ${OPENGL_INDLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${SDL2IMAGE_INCLUDE_DIRS}
  ${SOIL_INCLUDE_DIR}
  ${STATIC_STRING_INCLUDE_DIR}
  )

###################################################################################################
## Specify which static libraries depend on each-other (for the linker's sake).
##
## The base game is dependent on ALL other libraries. The rest of the libraries need to have
## specified which other libraries they will link too here.

## target_link_libraries(COMMON_LIB   External_LIB ${BFD_LIB} ${STACKTRACE_LIB})
## target_link_libraries(BoomHS_LIB External_LIB Opengl_LIB COMMON_LIB GL_SSTACKTRACE_LIB ${BFD_LIB} ${STACKTRACE_LIB})

## target_link_libraries(Opengl_LIB External_LIB COMMON_LIB ${BFD_LIB} ${STACKTRACE_LIB})
## target_link_libraries(GL_SSTACKTRACE_LIB External_LIB Opengl_LIB COMMON_LIB ${BFD_LIB} ${STACKTRACE_LIB})

###################################################################################################
## Executables
###################################################################################################
set(EXTERNAL_LIBS
  ${BFD_LIB}
  ${STACKTRACE_LIB}
  ${SDL2_LIBRARIES}
  ${SDL2IMAGE_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${SOIL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  )
set(SYSTEM_LIBS
  stdc++
  m
  audio
  pthread
  openal
  )

###################################################################################################
## COMPILE -- Post-Processing Application
##
## Application that does post-processing on the OpenGL shader code (after the main executable has
## been compiled). The scripts included with the project automatically execute this application on
## your behalf when starting the main executable.
add_executable(BUILD_POSTPROCESSING ${TOOLS_DIRECTORY}/build_postprocessing.cxx)
target_include_directories(BUILD_POSTPROCESSING PUBLIC ${EXTERNAL_INCLUDE_DIRS})
target_link_libraries(     BUILD_POSTPROCESSING stdc++ stdc++fs)

###################################################################################################
## COMPILE -- Multiple Viewports Mouse Selection Demo
##
## Application for developing/testing out multiple viewports using raycasting and mouse box selection.
add_executable(viewport_mouse_raycast_boxselection ${DEMO_DIRECTORY}/viewports_and_mouse_raycast_boxselection.cxx)

target_link_libraries(viewport_mouse_raycast_boxselection
  DEMO_COMMON_SOURCE_CODE
  PROJECT_SOURCE_CODE
  ${SYSTEM_LIBS}
  ${EXTERNAL_LIBS}
  )

target_include_directories(viewport_mouse_raycast_boxselection PUBLIC ${DEMO_DIRECTORY_INCLUDE_DIR})

###################################################################################################
## COMPILE -- Seperating Axis Theorem Demo
##
## Application for testing/visualing the implementation of the seperating axis theorem.
add_executable(seperating_axis_theorem ${DEMO_DIRECTORY}/seperating_axis_theorem.cxx)

target_link_libraries(seperating_axis_theorem
  DEMO_COMMON_SOURCE_CODE
  PROJECT_SOURCE_CODE
  ${SYSTEM_LIBS}
  ${EXTERNAL_LIBS}
  )

target_include_directories(seperating_axis_theorem PUBLIC ${DEMO_DIRECTORY_INCLUDE_DIR})

###################################################################################################
## COMPILE -- Memory BUG I want to make sure doesn't show up in future tests.
##
##            I'm PRETTY reasonably sure there is a bug, but not 100% sure.
##
add_executable(debug-membug ${TEST_DIRECTORY}/debug-membug.cxx)

target_link_libraries(debug-membug
  DEMO_COMMON_SOURCE_CODE
  PROJECT_SOURCE_CODE
  ${SYSTEM_LIBS}
  ${EXTERNAL_LIBS}
  )

target_include_directories(debug-membug PUBLIC)

###################################################################################################
## COMPILE -- Main Executable
add_executable(boomhs ${MAIN_SOURCE_FILE})

target_link_libraries(boomhs
  PROJECT_SOURCE_CODE
  ${SYSTEM_LIBS}
  ${EXTERNAL_LIBS}
  )
target_include_directories(boomhs PUBLIC)

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
glm/0.9.9.0@g-truc/stable

[generators]
cmake
EOF

cd ${BUILD}
echo $(pwd)
conan install --build missing                                                                      \
  -s compiler=${COMPILER}                                                                          \
  -s arch=x86_64                                                                                   \
  -s compiler.version=${COMPILER_VERSION}                                                          \
  -s compiler.libcxx=${CXX_STD_LIBRARY}                                                            \
  -s build_type=${DEBUG_OR_RELEASE}                                                                \
  .

cmake ..  -G "${BUILD_SYSTEM}"                                                                     \
  -DCMAKE_CXX_COMPILER=${COMPILER}                                                                 \
  -DCMAKE_BUILD_TYPE=${DEBUG_OR_RELEASE}                                                           \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd ..
