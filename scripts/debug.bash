#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

gdb ${BUILD}/bin/boomhs
