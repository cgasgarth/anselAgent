#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)

if [[ "$(uname -s)" == "Darwin" ]]; then
  ANSEL_MACOS_BUNDLE=1 ANSEL_SKIP_CONFIG=1 "$SCRIPT_DIR/build_ansel_local.sh"
  ANSEL_MACOS_BUNDLE=1 "$SCRIPT_DIR/run_ansel_local.sh"
else
  ANSEL_SKIP_CONFIG=1 "$SCRIPT_DIR/build_ansel_local.sh"
  "$SCRIPT_DIR/run_ansel_local.sh"
fi

"$SCRIPT_DIR/run_server.sh"
