#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

# Allow core dumps
ulimit -c unlimited

${BINARY}
