#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd -P)

BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/ansel/build-master}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$REPO_ROOT/ansel/.install-master}"
RUNTIME_DIR="${RUNTIME_DIR:-$REPO_ROOT/.ansel-local}"
CONFIG_DIR="${CONFIG_DIR:-$RUNTIME_DIR/config}"
CACHE_DIR="${CACHE_DIR:-$RUNTIME_DIR/cache}"
ANSEL_LIBRARY_FILE="${ANSEL_LIBRARY_FILE:-$RUNTIME_DIR/library.db}"
ANSEL_AGENT_SERVER_TIMEOUT_SECONDS="${ANSEL_AGENT_SERVER_TIMEOUT_SECONDS:-600}"
ANSEL_LOG_FILE="${ANSEL_LOG_FILE:-$RUNTIME_DIR/ansel.log}"
FOREGROUND="${ANSEL_FOREGROUND:-0}"
RUN_FROM_BUILD_DIR="${ANSEL_RUN_FROM_BUILD_DIR:-0}"

args=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --foreground)
      FOREGROUND=1
      shift
      ;;
    --detached)
      FOREGROUND=0
      shift
      ;;
    *)
      args+=("$1")
      shift
      ;;
  esac
done

mkdir -p "$CONFIG_DIR" "$CACHE_DIR"

ANSEL_ROOT="$INSTALL_PREFIX"
ANSEL_BIN="$ANSEL_ROOT/bin/ansel"

if [[ "$RUN_FROM_BUILD_DIR" == "1" ]]; then
  ANSEL_BIN="$BUILD_DIR/src/ansel"
elif [[ ! -x "$ANSEL_BIN" ]] && [[ -x "$BUILD_DIR/src/ansel" ]]; then
  ANSEL_BIN="$BUILD_DIR/src/ansel"
fi

cmd=(
  "$ANSEL_BIN"
  --conf "plugins/ai/agent/timeout_seconds=$ANSEL_AGENT_SERVER_TIMEOUT_SECONDS"
  --configdir "$CONFIG_DIR"
  --cachedir "$CACHE_DIR"
  --library "$ANSEL_LIBRARY_FILE"
  ${args[@]+"${args[@]}"}
)

if [[ "$(uname -s)" == "Darwin" ]]; then
  glib_schemas_dir="$(pkg-config --variable=schemasdir gio-2.0 2>/dev/null || true)"
  gtk_prefix="$(pkg-config --variable=prefix gtk+-3.0 2>/dev/null || true)"

  if [[ -n "$glib_schemas_dir" && -d "$glib_schemas_dir" ]]; then
    export GSETTINGS_SCHEMA_DIR="$glib_schemas_dir"
  fi

  if [[ -n "$gtk_prefix" && -d "$gtk_prefix/share" ]]; then
    export XDG_DATA_DIRS="$gtk_prefix/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
  fi
fi

if [[ "$ANSEL_BIN" == "$BUILD_DIR/src/ansel" ]]; then
  export DYLD_LIBRARY_PATH="$BUILD_DIR/src${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
fi

if [[ "$FOREGROUND" == "1" ]]; then
  exec "${cmd[@]}"
fi

nohup "${cmd[@]}" >"$ANSEL_LOG_FILE" 2>&1 < /dev/null &
echo "Started ansel (PID $!)"
echo "Log: $ANSEL_LOG_FILE"
