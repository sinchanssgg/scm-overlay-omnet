# Self-stabilizing Multicast Overlay (SCM) Implementation

OMNeT++ implementation of the self-stabilizing multicast overlay algorithm from:

> "On Self-stabilizing Sharing of Multicast Transmission in Overlays"

This repository currently contains:
- A custom SCM simulation project under `omnetpp/simulations`
- Data preprocessing, analysis, and visualization scripts under `scripts/`
- An OMNeT++ upstream Git submodule under `third_party/omnetpp` (currently pinned to `omnetpp-6.3.0`)
- Docker assets for reproducible execution under `docker/`

## Documentation Map

- Setup and dependencies: `docs/setup.md`
- Architecture and design walkthrough: `docs/design.md`
- Verified current-state audit and known blockers: `docs/current-state.md`

## Features (Target Design)

- SCM stabilization logic across seven protocol rules
- Multiple topology modes:
  - Complete Binary Tree (CBT)
  - Erdos-Renyi (ER)
  - Twitch-derived graph
- Fault injection scenarios
- Result processing and metric plotting pipeline
- Dockerized execution path

## Current Status

The project structure and intent are clear, but there are known wiring/build gaps in the current codebase.

Before expecting full end-to-end runs, read:
- `docs/current-state.md`

## Quick Start Paths

### Path A: Docker-first

Use this if you want a controlled environment for dependencies.

```bash
cd docker
docker compose up --build
```

### Path B: Native OMNeT++ build

Use this if you want direct local development in OMNeT++.

```bash
cd third_party/omnetpp
source setenv
./configure
make -j$(nproc)
```

Then build and run simulations from `omnetpp/simulations` after applying the fixes listed in `docs/current-state.md`.

### Path C: Fast Local Pipeline (No Docker Rebuild)

Use this for day-to-day debugging.

Quick smoke test without Docker (single scenario):

```bash
./scripts/run_experiments.sh --quick
```

Full local pipeline (all configured scenarios):

```bash
./scripts/run_experiments.sh
```

Optional:

```bash
./scripts/run_experiments.sh --skip-sim-build
RESULT_DIR=/tmp/scm-results ./scripts/run_experiments.sh --quick
```

Notes:
- `run_experiments.sh` now bootstraps OMNeT++ environment automatically, so you do not need to manually run `source third_party/omnetpp/setenv` first.
- On first local run it auto-creates `third_party/omnetpp/configure.user` from `configure.user.dist` if missing.
- If `results/` is not writable (for example after Docker runs as root), it falls back to `$HOME/.local/state/scm-overlay-omnet/results/...`.

Notes:
- Keep Docker for final reproducibility checks, and use the local pipeline for rapid iteration.

## Repository Layout

```text
docker/                  # Container build and compose orchestration
docs/                    # Project docs (setup, design, status)
omnetpp/simulations/     # SCM simulation model, NED files, ini configs
third_party/omnetpp/     # OMNeT++ upstream submodule (pinned version)
scripts/preprocess/      # Topology/data preprocessing
scripts/analysis/        # Result aggregation
scripts/visualization/   # Plot generation
tests/                   # Unit/integration placeholders
```

## Notes

- Python dependencies are managed via `uv` and pinned in `uv.lock`. Run `uv sync` to install.
- `docs/results/` is reserved for result-oriented documentation artifacts.
- Run `scripts/check_omnetpp_version.sh` to compare your pinned submodule version against latest upstream tags.