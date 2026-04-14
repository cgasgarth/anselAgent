#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd -P)

BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/ansel/build-master}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$REPO_ROOT/ansel/.install-master}"
BUNDLE_APP="${BUNDLE_APP:-$REPO_ROOT/ansel/install/package/Ansel.app}"
ANSEL_AGENT_SERVER_TIMEOUT_SECONDS="${ANSEL_AGENT_SERVER_TIMEOUT_SECONDS:-600}"
FOREGROUND="${ANSEL_FOREGROUND:-0}"
RUN_FROM_BUILD_DIR="${ANSEL_RUN_FROM_BUILD_DIR:-0}"
MACOS_BUNDLE="${ANSEL_MACOS_BUNDLE:-0}"
ISOLATE_RUNTIME="${ANSEL_ISOLATE_RUNTIME:-1}"
FRESH_RUNTIME="${ANSEL_FRESH_RUNTIME:-0}"
RUNTIME_NAME="${ANSEL_RUNTIME_NAME:-}"

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

ANSEL_ROOT="$INSTALL_PREFIX"
ANSEL_BIN="$ANSEL_ROOT/bin/ansel"

if [[ "$MACOS_BUNDLE" == "1" ]]; then
  ANSEL_BIN="$BUNDLE_APP/Contents/MacOS/ansel"
  if [[ ! -x "$ANSEL_BIN" ]]; then
    echo "Missing macOS app bundle executable: $ANSEL_BIN" >&2
    echo "Run 'npm run ansel:build:macos-bundle' first." >&2
    exit 1
  fi
fi

if [[ "$RUN_FROM_BUILD_DIR" == "1" ]]; then
  ANSEL_BIN="$BUILD_DIR/src/ansel"
elif [[ ! -x "$ANSEL_BIN" ]] && [[ -x "$BUILD_DIR/src/ansel" ]]; then
  ANSEL_BIN="$BUILD_DIR/src/ansel"
fi

if [[ -z "$RUNTIME_NAME" ]]; then
  runtime_tag="default"
  if [[ -e "$ANSEL_BIN" ]]; then
    if [[ "$(uname -s)" == "Darwin" ]]; then
      runtime_tag="$(basename "$ANSEL_BIN").$(stat -f '%m' "$ANSEL_BIN")"
    else
      runtime_tag="$(basename "$ANSEL_BIN").$(stat -c '%Y' "$ANSEL_BIN")"
    fi
  fi
  RUNTIME_NAME="$runtime_tag"
fi

if [[ -z "${RUNTIME_DIR:-}" ]]; then
  if [[ "$ISOLATE_RUNTIME" == "1" ]]; then
    RUNTIME_DIR="$REPO_ROOT/.ansel-local/$RUNTIME_NAME"
  else
    RUNTIME_DIR="$REPO_ROOT/.ansel-local"
  fi
fi

CONFIG_DIR="${CONFIG_DIR:-$RUNTIME_DIR/config}"
CACHE_DIR="${CACHE_DIR:-$RUNTIME_DIR/cache}"
ANSEL_LIBRARY_FILE="${ANSEL_LIBRARY_FILE:-$RUNTIME_DIR/library.db}"
ANSEL_LOG_FILE="${ANSEL_LOG_FILE:-$RUNTIME_DIR/ansel.log}"

if [[ "$FRESH_RUNTIME" == "1" ]]; then
  rm -rf "$RUNTIME_DIR"
fi

mkdir -p "$CONFIG_DIR" "$CACHE_DIR"

cmd=(
  "$ANSEL_BIN"
  --conf "plugins/ai/agent/timeout_seconds=$ANSEL_AGENT_SERVER_TIMEOUT_SECONDS"
  --configdir "$CONFIG_DIR"
  --cachedir "$CACHE_DIR"
  --library "$ANSEL_LIBRARY_FILE"
  ${args[@]+"${args[@]}"}
)

if [[ "$(uname -s)" == "Darwin" ]]; then
  brew_prefix="$(brew --prefix 2>/dev/null || true)"
  gtk_prefix="$(brew --prefix gtk+3 2>/dev/null || true)"
  glib_schemas_dir=""

  if [[ -n "$brew_prefix" && -d "$brew_prefix/share/glib-2.0/schemas" ]]; then
    glib_schemas_dir="$brew_prefix/share/glib-2.0/schemas"
  else
    glib_schemas_dir="$(pkg-config --variable=schemasdir gio-2.0 2>/dev/null || true)"
  fi

  if [[ -n "$glib_schemas_dir" && -d "$glib_schemas_dir" ]]; then
    export GSETTINGS_SCHEMA_DIR="$glib_schemas_dir"
  fi

  if [[ -n "$gtk_prefix" && -d "$gtk_prefix/share" ]]; then
    export XDG_DATA_DIRS="$gtk_prefix/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
  fi

  if [[ -n "$brew_prefix" && -d "$brew_prefix/share" ]] && [[ ":${XDG_DATA_DIRS:-}:" != *":$brew_prefix/share:"* ]]; then
    export XDG_DATA_DIRS="$brew_prefix/share${XDG_DATA_DIRS:+:$XDG_DATA_DIRS}"
  fi
fi

if [[ "$ANSEL_BIN" == "$BUILD_DIR/src/ansel" ]]; then
  export DYLD_LIBRARY_PATH="$BUILD_DIR/src${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
fi

if [[ "$MACOS_BUNDLE" == "1" ]] && [[ "$(uname -s)" == "Darwin" ]]; then
  cmd=(
    open
    -a "$BUNDLE_APP"
    --args
    --conf "plugins/ai/agent/timeout_seconds=$ANSEL_AGENT_SERVER_TIMEOUT_SECONDS"
    --configdir "$CONFIG_DIR"
    --cachedir "$CACHE_DIR"
    --library "$ANSEL_LIBRARY_FILE"
    ${args[@]+"${args[@]}"}
  )
fi

if [[ "$FOREGROUND" == "1" ]]; then
  exec "${cmd[@]}"
fi

nohup "${cmd[@]}" >"$ANSEL_LOG_FILE" 2>&1 < /dev/null &
echo "Started ansel (PID $!)"
echo "Log: $ANSEL_LOG_FILE"
