#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

function cleanup {
  echo "Removing ./temp/"
  rm  -rf ./temp/
}

# Logic starts here.
mkdir temp
trap cleanup EXIT

# Install libc++dev
sudo apt-get install libc++dev

# Install sdl-2.0
sudo apt-get install libsdl2-dev #(TODO: confirm more libraries aren't needed)

# Install libSOIL
cd temp
git clone https://github.com/kbranigan/Simple-OpenGL-Image-Library.git
cd Simple-OpenGL-Image-Library
make
make install
