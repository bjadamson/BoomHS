#!/usr/bin/env bash
source "scripts/common-static-analysis.bash"

# First, run the build post-processing script
${BUILD}/bin/BUILD_POSTPROCESSING

${BINARY}
