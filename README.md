# anselAgent

anselAgent is an AI-assisted editing workflow built on top of a vendored copy of [Ansel](https://github.com/aurelienpierreeng/ansel). It combines an Ansel darkroom chat interface, a local Python backend, and a structured edit protocol so edit requests can be translated into supported Ansel operations.

Ansel remains the source of truth for image state and rendering. The backend plans edits; Ansel validates and applies them through supported controls.

## Features

- Integrated darkroom chat UI
- Single-turn planning and live iterative edit sessions
- Structured operation protocol between UI and backend
- Streaming progress during live runs
- Deterministic smoke-test path using mock responses

## Architecture

- `ansel/` contains the vendored Ansel source tree and the darkroom UI integration port
- `server/` contains the FastAPI backend and Codex bridge
- `shared/` contains the protocol models and schema

Request flow:

1. Ansel captures the current image state, editable settings, preview, and session context.
2. The backend sends that context to the planner.
3. The planner returns structured edit operations.
4. Ansel validates and applies those operations through supported controls.

In live mode, the backend can stage multiple edit batches, refresh state and preview, and continue refining within the same run.

## Setup

Prerequisites:

- macOS or Linux
- `python3` 3.14+
- `uv`
- `codex` CLI installed and authenticated
- macOS: Homebrew
- Linux: Ansel build dependencies for your distribution
- local CLI tools used by the build and test scripts: `ninja`, `cmake`, `curl`
- optional: `xvfb-run` for headless smoke tests (Linux)
- macOS smoke runs require an active logged-in GUI session; they do not use `xvfb-run`

Install all dependencies (Homebrew packages on macOS, Python packages on all platforms):

```bash
npm run bootstrap
```

Build Ansel:

```bash
npm run ansel:build
```

Start the backend:

```bash
npm run server:start
```

Start Ansel:

```bash
npm run ansel:start
```

By default, the backend runs locally on `127.0.0.1:8001`.

## macOS App

Build the native macOS app bundle:

```bash
npm run ansel:build:macos-bundle
```

This produces:

```text
ansel/install/package/Ansel.app
```

Install it into `/Applications` so it appears in Finder, Launchpad, and Spotlight:

```bash
cp -R "./ansel/install/package/Ansel.app" /Applications/
```

If `Ansel.app` already exists in `/Applications`, replace it after rebuilding:

```bash
rm -rf /Applications/Ansel.app
cp -R "./ansel/install/package/Ansel.app" /Applications/
```

Typical update flow after making native Ansel changes:

```bash
npm run ansel:build:macos-bundle
rm -rf /Applications/Ansel.app
cp -R "./ansel/install/package/Ansel.app" /Applications/
```

Open the installed app with Finder or:

```bash
open -a /Applications/Ansel.app
```

The installed app starts the local AI backend automatically. Server and launcher
logs are written to `~/Library/Logs/AnselAgent/`.

## Testing

Run the Python test suite:

```bash
uv run pytest server/tests
```

Run Python type checking:

```bash
uv run ty check
```

Run local pre-commit checks:

```bash
uvx pre-commit run --all-files
```

Run the evaluation harness against the built-in golden corpus:

```bash
npm run agent:eval
```

Run the deterministic smoke test:

```bash
npm run agent:smoke
```

Run the deterministic multi-turn smoke test:

```bash
npm run agent:smoke:multi-turn
```

On Linux, the smoke script can run headlessly with `xvfb-run`.
On macOS, run it from a logged-in desktop session so Ansel can open normally.

## Protocol

Protocol details are documented in `docs/protocol-v1.md`.

## Evaluation Harness

Evaluation harness details are documented in `docs/evaluation-harness.md`.

## Upstream Ansel tracking

Upstream tracking details are documented in `docs/upstream-ansel.md`.

This repo tracks Ansel through a git remote named `upstream`:

```bash
git remote add upstream https://github.com/aurelienpierreeng/ansel.git
git fetch upstream
```

Review the current delta from upstream with:

```bash
git log --oneline --left-right --cherry upstream/master...HEAD
git diff upstream/master...HEAD
```

Check the vendored Ansel tree against the tracked upstream release with:

```bash
npm run ansel:upstream-status
```

The upstream metadata distinguishes between Ansel's original fork base and the current upstream release our vendored tree matches.
