#!/usr/bin/env bash
set -e

ROOT="$(pwd)"
BUILD="${ROOT}/build-system"
BINARY="${BUILD}/bin/boomhs"
SOURCE="${ROOT}/source"

# Delete the binary produced by building this project.
#
## Does not require the project to be rebootstrapped.
function clean() {
  rm -f ${BINARY}
  rm -rf ${BUILD}/CMakeFiles
}

# Delete all the build artifacts from the build directory.
#
## Project must be re-bootstrapped after this function is run.
function full_clean() {
  rm -rf ${BUILD}
  rm -f "CMakeLists.txt"
}
