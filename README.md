# Self-stabilizing Multicast Overlay (SCM) Implementation

OMNeT++ implementation of the self-stabilizing multicast overlay algorithm from:

> "On Self-stabilizing Sharing of Multicast Transmission in Overlays"

This repository contains:
- A custom SCM simulation project under `omnetpp/simulations`
- Data preprocessing, analysis, and visualization scripts under `scripts/`
- An OMNeT++ upstream Git submodule under `third_party/omnetpp`
- Docker assets for reproducible execution under `docker/`

## Documentation Map

- Setup and dependencies: `docs/setup.md`
- Architecture and design walkthrough: `docs/design.md`
- Verified current-state audit: `docs/current-state.md`
- Result schema notes: `docs/results/README.md`

## First-time Setup

```bash
git submodule update --init --recursive
# Install uv first if missing: https://docs.astral.sh/uv/getting-started/installation/
uv sync
```

## Quick Start

### Path A: Docker-first

```bash
cd docker
docker compose up --build
```

### Path B: Native build + local pipeline

Build OMNeT++ once:

```bash
cd third_party/omnetpp
source setenv
./configure
make -j$(nproc)
```

Then run the project pipeline from repository root:

```bash
./scripts/run_experiments.sh --quick
./scripts/run_experiments.sh
```

Optional:

```bash
./scripts/run_experiments.sh --skip-sim-build
RESULT_DIR=/tmp/scm-results ./scripts/run_experiments.sh --quick
```

Notes:
- `run_experiments.sh` bootstraps OMNeT++ environment automatically.
- If `results/` is not writable, output falls back to `$HOME/.local/state/scm-overlay-omnet/results/...`.

## Current Runtime Behavior

- Default active run matrix executes: `BaselineCBT`, `FaultDistance`, `BaselineER`, `FaultBeta`.
- All active configs currently use `network = SCMNetwork` from `omnetpp/simulations/omnetpp.ini`.
- Additional topology/network modules (`CompleteBinaryTree`, `ErdosRenyi`, `TwitchNetwork`) exist in code but are not selected by the default run matrix.

See `docs/current-state.md` for known gaps and caveats.

## Expected Outputs

After a successful run, artifacts are written under `$RESULT_DIR`.

- Default local behavior: `RESULT_DIR=results/<timestamp>`
- Docker compose default in this repo: `RESULT_DIR=/workspace/results/latest` inside the container (volume-mapped to `results/latest` on the host)

Expected layout:
- `$RESULT_DIR/<ConfigName>/...` (OMNeT++ raw outputs such as `.vec`, `.sca`)
- `$RESULT_DIR/analysis.csv`
- `$RESULT_DIR/metrics_plot.png`

## Repository Layout

```text
docker/                  # Container build and compose orchestration
docs/                    # Project docs (setup, design, status)
omnetpp/simulations/     # SCM simulation model, NED files, ini configs
third_party/omnetpp/     # OMNeT++ upstream submodule (pinned commit in gitlink)
scripts/preprocess/      # Topology/data preprocessing
scripts/analysis/        # Result aggregation
scripts/visualization/   # Plot generation
```

## Notes

- Python dependencies are managed via `uv` and pinned in `uv.lock`.
- `docs/results/` stores narrative/result documentation, while runtime artifacts are written under `$RESULT_DIR`.
- Run `scripts/check_omnetpp_version.sh` to compare the pinned submodule state against latest upstream tags.
