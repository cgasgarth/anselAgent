#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
REPO_ROOT=$(cd "$SCRIPT_DIR/.." && pwd -P)
PLATFORM=$(uname -s)
ASSET_PATH="${ASSET_PATH:-$REPO_ROOT/assets/_DSC8809.ARW}"
HOST="${HOST:-${ANSEL_AGENT_SERVER_HOST:-127.0.0.1}}"
PYTHON_BIN="${PYTHON_BIN:-$REPO_ROOT/.venv/bin/python}"
AUTORUN_PROMPT="${AUTORUN_PROMPT:-${ANSEL_AGENT_TEST_AUTORUN_PROMPT:-Verify configured image settings.}}"
AUTORUN_QUIT_AFTER_MS="${AUTORUN_QUIT_AFTER_MS:-${ANSEL_AGENT_TEST_AUTORUN_QUIT_AFTER_MS:-5000}}"
KEEP_ARTIFACTS="${KEEP_ARTIFACTS:-0}"
SKIP_BUILD="${SKIP_BUILD:-0}"
SKIP_SERVER_TESTS="${SKIP_SERVER_TESTS:-0}"
CLEAN_RUNTIME="${CLEAN_RUNTIME:-0}"
RUNTIME_DIR="${RUNTIME_DIR:-$REPO_ROOT/.ansel-local}"
EXPECTED_REQUEST_COUNT="${EXPECTED_REQUEST_COUNT:-2}"
SETTINGS_JSON="${SETTINGS_JSON:-}"

if [[ -z "$SETTINGS_JSON" ]]; then
  SETTINGS_JSON=$(cat <<'EOF'
[
  {
    "selector": {
      "moduleIds": ["exposure"],
      "labelContains": "Exposure",
      "kind": "set-float"
    },
    "value": {
      "mode": "set",
      "number": 0.7
    }
  },
  {
    "selector": {
      "moduleIds": ["grain"],
      "labelContains": "strength",
      "kind": "set-float"
    },
    "value": {
      "mode": "set",
      "number": 40.0
    }
  },
  {
    "selector": {
      "moduleIds": ["colorbalancergb"],
      "labelContains": "contrast",
      "kind": "set-float"
    },
    "value": {
      "mode": "set",
      "number": 0.2
    }
  },
  {
    "selector": {
      "moduleIds": ["colorprimaries", "primaries"],
      "moduleLabelContains": "primaries",
      "labelContains": "red hue",
      "kind": "set-float"
    },
    "value": {
      "mode": "set",
      "number": 10.0
    }
  }
]
EOF
)
fi

if [[ -z "${ANSEL_TIMEOUT_SECONDS:-}" ]]; then
  ANSEL_TIMEOUT_SECONDS=600
fi

if [[ -z "${SERVER_TIMEOUT_SECONDS:-}" ]]; then
  SERVER_TIMEOUT_SECONDS=600
fi

cleanup() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi

  if [[ "$KEEP_ARTIFACTS" != "1" ]]; then
    rm -f "$REPORT_FILE" "$SERVER_LOG"
  fi

  if [[ "$CLEAN_RUNTIME" == "1" ]]; then
    rm -rf "$RUNTIME_DIR"
  fi
}
trap cleanup EXIT

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

if [[ ! -f "$ASSET_PATH" ]]; then
  echo "Missing RAW asset: $ASSET_PATH" >&2
  exit 1
fi

if [[ ! -x "$PYTHON_BIN" ]]; then
  PYTHON_BIN="python3"
fi

make_temp_file() {
  local stem="$1"
  local suffix="$2"

  "$PYTHON_BIN" - "$stem" "$suffix" <<'PY'
import pathlib
import sys
import tempfile

stem = sys.argv[1]
suffix = sys.argv[2]
tmpdir = pathlib.Path(tempfile.gettempdir())
fd, path = tempfile.mkstemp(prefix=f"{stem}.", suffix=suffix, dir=tmpdir)
pathlib.Path(path).unlink(missing_ok=True)
print(path)
PY
}

SERVER_LOG="${SERVER_LOG:-$(make_temp_file ansel-agent-settings-server .log)}"
REPORT_FILE="${REPORT_FILE:-$(make_temp_file ansel-agent-settings-report .ini)}"

if [[ -z "${PORT:-${ANSEL_AGENT_SERVER_PORT:-}}" ]]; then
  PORT="$((20000 + RANDOM % 20000))"
else
  PORT="${PORT:-${ANSEL_AGENT_SERVER_PORT}}"
fi

SERVER_URL="${ANSEL_AGENT_SERVER_URL:-http://$HOST:$PORT/v1/chat}"
HEALTH_URL="${HEALTH_URL:-http://$HOST:$PORT/health}"

cd "$REPO_ROOT"

if [[ "$SKIP_SERVER_TESTS" != "1" ]]; then
  echo "[settings-smoke] running server tests"
  "$PYTHON_BIN" -m pytest server/tests/test_mock_planner.py
fi

if [[ "$SKIP_BUILD" != "1" ]]; then
  echo "[settings-smoke] building ansel"
  "$SCRIPT_DIR/build_ansel_local.sh"
fi

echo "[settings-smoke] starting local server"
HOST="$HOST" \
  PORT="$PORT" \
  ANSEL_AGENT_USE_MOCK_RESPONSES=1 \
  ANSEL_AGENT_TEST_MOCK_OPERATIONS_JSON="$SETTINGS_JSON" \
  ANSEL_AGENT_OPENAI_TIMEOUT_SECONDS="$SERVER_TIMEOUT_SECONDS" \
  "$SCRIPT_DIR/run_server.sh" >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!

for _ in $(seq 1 40); do
  if curl -fsS "$HEALTH_URL" >/dev/null 2>&1; then
    break
  fi
  sleep 0.5
done

if ! curl -fsS "$HEALTH_URL" >/dev/null 2>&1; then
  echo "Server did not become healthy. See $SERVER_LOG" >&2
  exit 1
fi

echo "[settings-smoke] launching ansel autorun"

launcher=("$SCRIPT_DIR/run_ansel_local.sh" --foreground --disable-opencl "$ASSET_PATH")
if [[ "$PLATFORM" == "Linux" && -z "${DISPLAY:-}" ]]; then
  if command -v xvfb-run >/dev/null 2>&1; then
    launcher=(xvfb-run -a "${launcher[@]}")
  else
    echo "No DISPLAY set and xvfb-run is unavailable." >&2
    exit 1
  fi
fi

echo "Server: $SERVER_URL"
echo "Asset:  $ASSET_PATH"
echo "Report: $REPORT_FILE"
echo "Log:    $SERVER_LOG"

ANSEL_AGENT_SERVER_URL="$SERVER_URL" \
  ANSEL_AGENT_SERVER_TIMEOUT_SECONDS="$SERVER_TIMEOUT_SECONDS" \
  ANSEL_AGENT_TEST_AUTORUN_PROMPT="$AUTORUN_PROMPT" \
  ANSEL_AGENT_TEST_RESULT_FILE="$REPORT_FILE" \
  ANSEL_AGENT_TEST_AUTORUN_QUIT_AFTER_MS="$AUTORUN_QUIT_AFTER_MS" \
  ANSEL_AGENT_TEST_MULTI_TURN_ENABLED=1 \
  ANSEL_AGENT_TEST_MULTI_TURN_MAX_TURNS=2 \
  RUNTIME_DIR="$RUNTIME_DIR" \
  run_with_timeout "${ANSEL_TIMEOUT_SECONDS}s" "${launcher[@]}"

"$PYTHON_BIN" - "$SERVER_LOG" "$SETTINGS_JSON" "$EXPECTED_REQUEST_COUNT" <<'PY'
import json
import math
import sys

server_log_path = sys.argv[1]
settings = json.loads(sys.argv[2])
expected_request_count = int(sys.argv[3])

accepted_requests = []
fulfilled_requests = []
with open(server_log_path, "r", encoding="utf-8") as handle:
    for line in handle:
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            continue
        event = payload.get("event")
        if event == "accepted_request":
            accepted_requests.append(payload)
        elif event == "fulfilled_request":
            fulfilled_requests.append(payload)

if len(accepted_requests) != expected_request_count:
    raise SystemExit(
        f"Expected {expected_request_count} accepted_request entries, found {len(accepted_requests)}"
    )

if not fulfilled_requests:
    raise SystemExit("Expected at least one fulfilled_request entry")

first_fulfilled = fulfilled_requests[0]
if int(first_fulfilled.get("operationCount", 0)) != len(settings):
    raise SystemExit(
        f"Expected first fulfilled_request operationCount {len(settings)}, found "
        f"{first_fulfilled.get('operationCount', 0)}"
    )

first_snapshot = accepted_requests[0].get("imageSnapshot")
second_snapshot = accepted_requests[-1].get("imageSnapshot")
if not isinstance(first_snapshot, dict) or not isinstance(second_snapshot, dict):
    raise SystemExit("Expected imageSnapshot payloads in accepted_request logs")

first_settings = first_snapshot.get("editableSettings")
second_settings = second_snapshot.get("editableSettings")
if not isinstance(first_settings, list) or not isinstance(second_settings, list):
    raise SystemExit("Expected editableSettings arrays in accepted_request logs")


def matches_selector(setting: dict, selector: dict) -> bool:
    module_ids = selector.get("moduleIds")
    if isinstance(module_ids, list) and module_ids:
        valid_module_ids = {value for value in module_ids if isinstance(value, str)}
        if setting.get("moduleId") not in valid_module_ids:
            return False

    module_label_contains = selector.get("moduleLabelContains")
    if isinstance(module_label_contains, str) and module_label_contains:
        module_label = setting.get("moduleLabel")
        if not isinstance(module_label, str) or module_label_contains.lower() not in module_label.lower():
            return False

    label_contains = selector.get("labelContains")
    if isinstance(label_contains, str) and label_contains:
        label = setting.get("label")
        if not isinstance(label, str) or label_contains.lower() not in label.lower():
            return False

    action_path = selector.get("actionPath")
    if isinstance(action_path, str) and action_path:
        if setting.get("actionPath") != action_path:
            return False

    kind = selector.get("kind")
    if isinstance(kind, str) and kind:
        if setting.get("kind") != kind:
            return False

    return True


def find_setting(rows: list[dict], selector: dict) -> dict:
    for row in rows:
        if matches_selector(row, selector):
            return row
    raise SystemExit(f"No editable setting matched selector {selector}")


def assert_float_setting(first: dict, second: dict, value: dict) -> None:
    expected = value.get("number")
    mode = value.get("mode")
    if not isinstance(expected, (int, float)):
        raise SystemExit(f"Expected numeric value for setting {second.get('actionPath')}")
    first_value = first.get("currentNumber")
    second_value = second.get("currentNumber")
    if not isinstance(second_value, (int, float)):
        raise SystemExit(f"Missing currentNumber for setting {second.get('actionPath')}")
    if mode == "set":
        if math.fabs(float(second_value) - float(expected)) > 0.05:
            raise SystemExit(
                f"Expected {second.get('actionPath')} to be {expected}, found {second_value}"
            )
        return
    if mode == "delta":
        if not isinstance(first_value, (int, float)):
            raise SystemExit(f"Missing initial currentNumber for setting {second.get('actionPath')}")
        actual_delta = float(second_value) - float(first_value)
        if math.fabs(actual_delta - float(expected)) > 0.05:
            raise SystemExit(
                f"Expected {second.get('actionPath')} delta {expected}, found {actual_delta}"
            )
        return
    raise SystemExit(f"Unsupported float mode {mode!r} for {second.get('actionPath')}")


def assert_choice_setting(second: dict, value: dict) -> None:
    if "choiceValue" in value:
        if second.get("currentChoiceValue") != value["choiceValue"]:
            raise SystemExit(
                f"Expected {second.get('actionPath')} choiceValue {value['choiceValue']}, found "
                f"{second.get('currentChoiceValue')}"
            )
        return
    if "choiceId" in value:
        if second.get("currentChoiceId") != value["choiceId"]:
            raise SystemExit(
                f"Expected {second.get('actionPath')} choiceId {value['choiceId']!r}, found "
                f"{second.get('currentChoiceId')!r}"
            )
        return
    raise SystemExit(f"Expected choiceValue or choiceId for {second.get('actionPath')}")


def assert_bool_setting(second: dict, value: dict) -> None:
    expected = value.get("bool")
    if not isinstance(expected, bool):
        raise SystemExit(f"Expected bool value for setting {second.get('actionPath')}")
    if second.get("currentBool") is not expected:
        raise SystemExit(
            f"Expected {second.get('actionPath')} bool {expected}, found {second.get('currentBool')}"
        )


for item in settings:
    selector = item.get("selector")
    value = item.get("value")
    if not isinstance(selector, dict) or not isinstance(value, dict):
        raise SystemExit("Each settings entry must contain selector and value objects")
    first_setting = find_setting(first_settings, selector)
    second_setting = find_setting(second_settings, selector)
    kind = second_setting.get("kind")
    if kind == "set-float":
        assert_float_setting(first_setting, second_setting, value)
        continue
    if kind == "set-choice":
        assert_choice_setting(second_setting, value)
        continue
    if kind == "set-bool":
        assert_bool_setting(second_setting, value)
        continue
    raise SystemExit(f"Unsupported setting kind {kind!r} for selector {selector}")

print(f"Settings smoke test passed for {len(settings)} configurable settings.")
PY
