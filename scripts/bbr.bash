#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

# Allow core dumps
ulimit -c unlimited

${BUILD}/bin/shader_loader
${BUILD}/bin/log_mover
${BINARY}
