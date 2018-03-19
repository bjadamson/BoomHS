#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

cd ${BUILD}
cmake --build .
cd ..
${BUILD}/bin/build_postprocessing
