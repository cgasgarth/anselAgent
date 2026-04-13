# Tracking Upstream Ansel

This repository does not need to be a GitHub fork of `aurelienpierreeng/ansel` to stay aligned with upstream. The source of truth for the vendored Ansel base lives in `ansel-upstream.json`.

## Current baseline

- Upstream repository: `https://github.com/aurelienpierreeng/ansel.git`
- Original fork/base tag: `release-4.0.0`
- Current matched upstream tag: `master`
- Vendored source path in this repo: `ansel/`

## How to check our downstream patch surface

Run:

```bash
npm run ansel:upstream-status
```

That command downloads the tracked upstream source archive into a temporary directory, compares it to `ansel/`, ignores generated/build metadata, and reports whether the local tree matches upstream plus our expected downstream patch set.

The status output intentionally distinguishes between:

- `Original base tag`: the upstream release this fork started from
- `Tracked current-match tag`: the upstream release the current vendored tree matches now

You can also compare against a newer tag before attempting a sync:

```bash
python3 scripts/ansel_upstream.py status --tag master
```

Swap in a newer upstream ref when Ansel advances.

## How to track new upstream releases

- `ansel-upstream.json` records both the original fork base and the currently matched upstream ref, plus the expected downstream patch surface.

## Recommended sync workflow

1. Create a branch like `sync-ansel-<ref>`.
2. Run `python3 scripts/ansel_upstream.py status --tag <ref>` to see the delta from the new upstream ref.
3. Update the vendored `ansel/` tree to the new upstream ref.
4. Reapply or adapt only the downstream patch surface recorded in `ansel-upstream.json`.
5. Update `ansel-upstream.json` to the new tracked ref.
6. Run the repo validation/build flow before opening the sync PR.

## Why this workflow

- No GitHub fork relationship is required.
- No persistent git remote is required for day-to-day tracking.
- The upstream baseline stays explicit and reviewable in the repo.
- Future sync PRs can focus on the small set of files where anselAgent intentionally diverges from upstream.
