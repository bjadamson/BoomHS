#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

CWD=$(pwd)
function cleanup {
  echo "Removing ./temp/"
  cd $CWD
  rm  -rf ./temp/
}

# Logic starts here.
mkdir temp
cd temp
trap cleanup EXIT

# We need python to get started
sudo apt-get install python-setuptools python-dev build-essential
sudo apt-get install python-pip

# Use pip to install conan (c++ library/package manager)
pip install conan

# Install basic dependencies
apt-get install libc++-dev

# Required libraries for GLEW
apt-get install libXmu-dev libXi-dev libgl-dev dos2unix git wget

# Install sdl-2.0
apt-get install libsdl2-dev #(TODO: confirm more libraries aren't needed)

# Install libSOIL
git clone https://github.com/kbranigan/Simple-OpenGL-Image-Library.git
cd Simple-OpenGL-Image-Library
make
make install
