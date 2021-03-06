#!/usr/bin/env bash
set -ex
source "scripts/common-build.bash"

# Allow core dumps
ulimit -c unlimited

# Address Sanitizer Runtime Flags
export ADDRESS_SANITIZER_FLAGS="verbosity=1:detect_container_overflow=0:check_initialization_order=1:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"
export LEAK_DETECTOR_FLAGS="suppressions=external/llvm_leaks.supp"

export ASAN_OPTIONS=${ADDRESS_SANITIZER_FLAGS}
export LSAN_OPTIONS=${LEAK_DETECTOR_FLAGS}
