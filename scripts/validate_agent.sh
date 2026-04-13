#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd -P)
PYTHON_BIN="${PYTHON_BIN:-$REPO_ROOT/.venv/bin/python}"

if [[ ! -x "$PYTHON_BIN" ]]; then
  PYTHON_BIN="python3"
fi

run_with_timeout() {
  local duration="$1"
  shift

  if command -v timeout >/dev/null 2>&1; then
    timeout "$duration" "$@"
    return
  fi

  if command -v gtimeout >/dev/null 2>&1; then
    gtimeout "$duration" "$@"
    return
  fi

  "$PYTHON_BIN" - "$duration" "$@" <<'PY'
import subprocess
import sys

duration = sys.argv[1]
command = sys.argv[2:]

if duration.endswith("s"):
    timeout_seconds = float(duration[:-1])
else:
    timeout_seconds = float(duration)

result = subprocess.run(command, timeout=timeout_seconds, check=False)
raise SystemExit(result.returncode)
PY
}

cd "$REPO_ROOT"

run_with_timeout 300s "$PYTHON_BIN" -m pytest \
  server/tests/test_protocol.py \
  server/tests/test_api.py \
  server/tests/test_codex_app_server.py \
  server/tests/test_smoke.py

run_with_timeout 120s uv run ty check

run_with_timeout 3600s "$SCRIPT_DIR/build_ansel_local.sh"

run_with_timeout 900s env \
  SKIP_BUILD=1 \
  SKIP_SERVER_TESTS=1 \
  ANSEL_TIMEOUT_SECONDS="${ANSEL_TIMEOUT_SECONDS:-600}" \
  SERVER_TIMEOUT_SECONDS="${SERVER_TIMEOUT_SECONDS:-600}" \
  "$SCRIPT_DIR/agent_exposure_smoke.sh"

run_with_timeout 900s env \
  SKIP_BUILD=1 \
  SKIP_SERVER_TESTS=1 \
  MULTI_TURN_ENABLED=1 \
  EXPECTED_DELTA= \
  EXPECTED_FINAL_EXPOSURE=1.4 \
  EXPECTED_MIN_REFINEMENT_PASSES=2 \
  EXPECTED_MAX_REFINEMENT_PASSES=15 \
  ANSEL_AGENT_TEST_AUTORUN_QUIT_AFTER_MS="${ANSEL_AGENT_TEST_AUTORUN_QUIT_AFTER_MS:-5000}" \
  ANSEL_TIMEOUT_SECONDS="${ANSEL_TIMEOUT_SECONDS:-600}" \
  SERVER_TIMEOUT_SECONDS="${SERVER_TIMEOUT_SECONDS:-600}" \
  "$SCRIPT_DIR/agent_exposure_smoke.sh"
