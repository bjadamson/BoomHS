#!/usr/bin/env bash
set -ex
source "scripts/common.bash"

# Allow core dumps
ulimit -c unlimited

# Address Sanitizer Runtime Flags
ADDRESS_SANITIZER_FLAGS="verbosity=1:detect_container_overflow=0:check_initialization_order=1:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1"
LEAK_DETECTOR_FLAGS="suppressions=external/llvm_leaks.supp"

RUN_TESTS_PROGRAM="${BUILD}/bin/raycast_test"
ASAN_OPTIONS=${ADDRESS_SANITIZER_FLAGS} LSAN_OPTIONS=${LEAK_DETECTOR_FLAGS} ${RUN_TESTS_PROGRAM}
