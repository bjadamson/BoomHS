#!/usr/bin/env bash
set -ex

ROOT="$(pwd)"
BUILD="${ROOT}/build-system"
BINARY="${BUILD}/bin/boomhs"

# Delete the binary produced by building this project.
#
## Does not require the project to be rebootstrapped.
function clean() {
  rm -f ${BINARY}
}

# Delete all the build artifacts from the build directory.
#
## Project must be re-bootstrapped after this function is run.
function full_clean() {
  rm -rf ${BUILD}/*
  rm -f "CMakeLists.txt"
}
