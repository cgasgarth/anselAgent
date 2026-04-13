#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST="${HOST:-${ANSEL_AGENT_SERVER_HOST:-127.0.0.1}}"
PORT="${PORT:-${ANSEL_AGENT_SERVER_PORT:-8001}}"
PYTHON_BIN="${PYTHON_BIN:-$ROOT_DIR/.venv/bin/python}"

if [[ ! -x "$PYTHON_BIN" ]]; then
  PYTHON_BIN="python3"
fi

cd "$ROOT_DIR"
exec "$PYTHON_BIN" -m uvicorn server.app:app --host "$HOST" --port "$PORT"
