#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
ANSEL_DIR=$(cd "$SCRIPT_DIR/../ansel" && pwd -P)
PACKAGING_DIR="$ANSEL_DIR/packaging/macosx"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "macOS bundle build is only supported on Darwin." >&2
  exit 1
fi

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required for the macOS bundle flow." >&2
  exit 1
fi

(
  cd "$PACKAGING_DIR"
  ./2_build_hb_ansel_custom.sh
  ./3_make_hb_ansel_package.sh
)
