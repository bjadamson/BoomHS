#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

cd ${BUILD}
make cpptidy
cd ..
