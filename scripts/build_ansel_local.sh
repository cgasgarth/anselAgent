#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd -P)
ANSEL_DIR="$REPO_ROOT/ansel"

BUILD_DIR="${BUILD_DIR:-$ANSEL_DIR/build-master}"
INSTALL_PREFIX="${INSTALL_PREFIX:-$ANSEL_DIR/.install-master}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_GENERATOR="${BUILD_GENERATOR:-Ninja}"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)}"
USE_CCACHE="${ANSEL_USE_CCACHE:-1}"
SKIP_CONFIG="${ANSEL_SKIP_CONFIG:-0}"
INSTALL_BUILD="${ANSEL_INSTALL:-1}"
MACOS_BUNDLE="${ANSEL_MACOS_BUNDLE:-0}"
SKIP_SYSTEM_LINKS="${ANSEL_SKIP_SYSTEM_LINKS:-1}"

if [[ "$MACOS_BUNDLE" == "1" ]]; then
  exec "$SCRIPT_DIR/build_ansel_macos_bundle.sh" "$@"
fi

build_args=()
cmake_args=()
saw_double_dash=0

for arg in "$@"; do
  if [[ "$saw_double_dash" == "0" ]]; then
    if [[ "$arg" == "--" ]]; then
      saw_double_dash=1
    else
      build_args+=("$arg")
    fi
    continue
  fi

  cmake_args+=("$arg")
done

configure_cmake_args=()

if ((${#cmake_args[@]} > 0)); then
  configure_cmake_args=("${cmake_args[@]}")
fi
build_tree_ready=0

if [[ -f "$BUILD_DIR/CMakeCache.txt" ]] && [[ -f "$BUILD_DIR/build.ninja" ]]; then
  build_tree_ready=1
fi

if [[ "$USE_CCACHE" == "1" ]] && command -v ccache >/dev/null 2>&1; then
  has_c_launcher=0
  has_cxx_launcher=0

  for arg in "${configure_cmake_args[@]}"; do
    case "$arg" in
      -DCMAKE_C_COMPILER_LAUNCHER=*|CMAKE_C_COMPILER_LAUNCHER=*)
        has_c_launcher=1
        ;;
      -DCMAKE_CXX_COMPILER_LAUNCHER=*|CMAKE_CXX_COMPILER_LAUNCHER=*)
        has_cxx_launcher=1
        ;;
    esac
  done

  if [[ "$has_c_launcher" == "0" ]]; then
    if [[ "$build_tree_ready" == "0" ]] || ! grep -Eq '^CMAKE_C_COMPILER_LAUNCHER(:[^=]+)?=ccache$' "$BUILD_DIR/CMakeCache.txt"; then
      configure_cmake_args+=("-DCMAKE_C_COMPILER_LAUNCHER=ccache")
    fi
  fi

  if [[ "$has_cxx_launcher" == "0" ]]; then
    if [[ "$build_tree_ready" == "0" ]] || ! grep -Eq '^CMAKE_CXX_COMPILER_LAUNCHER(:[^=]+)?=ccache$' "$BUILD_DIR/CMakeCache.txt"; then
      configure_cmake_args+=("-DCMAKE_CXX_COMPILER_LAUNCHER=ccache")
    fi
  fi
fi

if [[ "$SKIP_CONFIG" == "1" ]] && [[ "$build_tree_ready" == "1" ]] \
  && ((${#build_args[@]} == 0)) && ((${#configure_cmake_args[@]} == 0)); then
  if [[ "$INSTALL_BUILD" == "1" ]]; then
    cmake --build "$BUILD_DIR" -- -j"$JOBS"
    exec cmake --build "$BUILD_DIR" --target install -- -j"$JOBS"
  fi

  exec cmake --build "$BUILD_DIR" -- -j"$JOBS"
fi

script_args=(
  --build-generator "$BUILD_GENERATOR"
  --build-dir "$BUILD_DIR"
  --prefix "$INSTALL_PREFIX"
  --build-type "$BUILD_TYPE"
  -j "$JOBS"
)

if [[ "$INSTALL_BUILD" == "1" ]]; then
  script_args+=(--install)
fi

if [[ "$SKIP_SYSTEM_LINKS" == "1" ]]; then
  script_args+=(--skip-system-links)
fi

if ((${#build_args[@]} > 0)); then
  script_args+=("${build_args[@]}")
fi

if ((${#configure_cmake_args[@]} > 0)); then
  script_args+=(-- "${configure_cmake_args[@]}")
fi

exec "$ANSEL_DIR/build.sh" "${script_args[@]}"
