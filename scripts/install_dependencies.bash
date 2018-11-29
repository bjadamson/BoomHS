#!/usr/bin/env bash
set -ex
source "scripts/common.bash"
source "scripts/helper_load_userargs.bash"

# GLOBAL VARIABLES
CWD=$(pwd)

function cleanup {
  echo "Removing ./temp/"
  cd $CWD
  rm  -rf ./temp/
}

function install_cmake() {
  wget http://www.cmake.org/files/v3.4/cmake-3.4.3.tar.gz
  tar -xvzf cmake-3.4.3.tar.gz
  cd cmake-3.4.3
  ./configure
  make
  sudo make install
  cd ../
}

function install_clang() {
  svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
  cd llvm/tools
  svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
  cd ../..
  cd llvm/tools/clang/tools
  svn co http://llvm.org/svn/llvm-project/clang-tools-extra/trunk extra
  cd ../../../..
  cd llvm/projects
  svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
  cd ../..
  mkdir build
  cd build
  cmake ../llvm -G "${BUILD_SYSTEM}"                                                                       \
      -DCMAKE_BUILD_TYPE=${DEBUG_OR_RELEASE}                                                       \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
  make
  sudo make install
}

# Logic starts here.

mkdir temp
cd temp
trap cleanup EXIT


# We need cmake
apt-get install git wget

read -r -p "Do you want to install cmake 3.4.3 from the cmake official website? [y/N] " response
case "$response" in
    [yY][eE][sS]|[yY])
        install_cmake
        ;;
    *)
        ;;
esac

read -r -p "Do you want to install clang from source (follows the instructions from clang website automatially website? [y/N] " response
case "$response" in
    [yY][eE][sS]|[yY])
        install_clang
        ;;
    *)
        ;;
esac

# We need python to get started
apt-get install python-setuptools python-dev build-essential
apt-get install python3-pip3

# Use pip to install conan (c++ library/package manager)
echo "If this next command fails. Try creating a new shell instance and re-running."
pip3 install conan

# Tell conan about the "bincrafters" remote (where from we get out dependencies)
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan

# Install basic dependencies
apt-get install libc++-dev
apt-get install libasound-dev
apt-get install portaudio19-dev


# dev-version of standard libaries (for GDB)
# The following versions were in use on my dev pc, so I grabbed dev versions of them all
# `dpkg --list | grep libstdc++`
apt-get install libstdc++6-4.8-dbg
apt-get install libstdc++-4.8-dev

# Stacktrace
apt-get install libdw-dev

# Required libraries for GLEW
apt-get install libXmu-dev libXi-dev libgl-dev dos2unix

# Install sdl-2.0
apt-get install libsdl2-dev #(TODO: confirm more libraries aren't needed)
apt-get install libaudio-dev

# Install libSOIL
git clone https://github.com/kbranigan/Simple-OpenGL-Image-Library.git
cd Simple-OpenGL-Image-Library
make
make install
