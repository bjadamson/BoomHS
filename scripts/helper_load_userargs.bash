#!/usr/bin/env bash

# GLOBAL VARIABLES
STATIC_ANALYSIS_FLAGS=""
DEBUG_OR_RELEASE="Debug"
CXX_STD_LIBRARY="libc++"
COMPILER="clang"
COMPILER_VERSION="8.0"
COMPILER_SPECIFIC_FLAGS="-MJ"
BUILD_SYSTEM="Ninja"

# Load all (supported) user-arguments supported by the scripts.
while getopts ":aghmr" opt; do
  case ${opt} in
    a )
      export STATIC_ANALYSIS_FLAGS="-fsanitize=address"
      ;;
    g )
      export COMPILER="gcc"
      export COMPILER_VERSION="9.0"
      export CXX_STD_LIBRARY="libstdc++"
      export COMPILER_SPECIFIC_FLAGS="-M"
      ;;
    m )
      export BUILD_SYSTEM="Unix Makefiles"
      ;;
    r )
      export DEBUG_OR_RELEASE="Release"
      ;;
    \h )
      echo "Help options for bootstrapping process."
      echo "[-a] To enable Static Analysis."
      echo "[-g] To use the GCC compiler toolchain."
      echo "[-m] To to to use the default 'Make' build-system."
      echo "[-r] To switch from Debug to Release mode."

      echo "[-h] See this message."
      echo "Please run again without the -h flag."
      echo "Quitting now."
      exit
      ;;
  esac
done
shift $((OPTIND -1))
