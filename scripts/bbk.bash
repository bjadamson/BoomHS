#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

PROCESS_ID=$(ps aux | grep -i build-system/bin/boomhs | head -n1 | cut -d " " -f2)
kill -9 $PROCESS_ID
