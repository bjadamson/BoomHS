#!/usr/bin/env bash

# GLOBAL VARIABLES
STATIC_ANALYSIS_FLAGS=""
DEBUG_OR_RELEASE="Debug"
CXX_STD_LIBRARY="libc++"
BUILD_SYSTEM="Ninja"

# Load all (supported) user-arguments supported by the scripts.
while getopts ":ahmr" opt; do
  case ${opt} in
    a )
      export STATIC_ANALYSIS_FLAGS="-fsanitize=address"
      ;;
    r )
      export DEBUG_OR_RELEASE="Release"
      ;;
    m )
      export BUILD_SYSTEM="Unix Makefiles"
      ;;
    \h )
      echo "Help options for bootstrapping process."
      echo "[-a] To enable Static Analysis."
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
