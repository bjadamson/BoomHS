#!/usr/bin/env bash
set -ex
source "scripts/common.bash"


kill -9 $(ps aux | grep -i build-system/bin/boomhs | head -n1 | cut -d " " -f3)
