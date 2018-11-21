#!/usr/bin/env bash
source "scripts/common.bash"

# https://gcc.gnu.org/onlinedocs/libstdc++/faq.html#faq.how_to_set_paths
LD_LIBRARY_PATH=/usr/local/lib64/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
