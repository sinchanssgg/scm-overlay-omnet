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
- Artifact evaluator playbook: `docs/artifact-evaluation.md`
- Submission/DOI packaging checklist: `docs/submission-packaging.md`
- Resource/headless operations runbook: `docs/operations-runbook.md`

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
./scripts/run_experiments.sh --mwe
MWE_NUM_NODES=4096 ./scripts/run_experiments.sh --mwe
```

Reproducibility knobs:

```bash
SCM_RANDOM_SEED=1337 ./scripts/run_experiments.sh --quick
uv run python scripts/analysis/validate_outputs.py /tmp/scm-results --require-non-empty
```

Artifact figure reproduction (Figure 2 style):

```bash
uv run python scripts/analysis/build_fig2_outputs.py results/fig2 --result-root results/latest
```

Artifact figure reproduction (Figure 3 style):

```bash
uv run python scripts/analysis/build_fig3_outputs.py results/fig3 \
  --cbt-state-dir results/latest/BaselineCBT \
  --twitch-state-dir results/latest/BaselineER \
  --twitch-nodes 50
```

Artifact figure reproduction (Figure 5 style):

```bash
uv run python scripts/analysis/build_fig5_outputs.py results/fig5 --result-root results/latest
```

Notes:
- `run_experiments.sh` bootstraps OMNeT++ environment automatically.
- If `results/` is not writable, output falls back to `$HOME/.local/state/scm-overlay-omnet/results/...`.

## Current Runtime Behavior

- Default active run matrix executes: `BaselineCBT`, `FaultDistance`, `BaselineER`, `FaultBeta`.
- `BaselineCBT`/`FaultDistance` run on `CompleteBinaryTree`.
- `BaselineER`/`FaultBeta` run on `ErdosRenyi`.
- `SCMNetwork` remains the global default for configs that do not override `network`.
- Preprocessing still generates `cbt_edges.txt` and `er_edges.txt`, but these files are not yet consumed by the active CBT/ER NED topologies.

See `docs/current-state.md` for known gaps and caveats.

## Expected Outputs

After a successful run, artifacts are written under `$RESULT_DIR`.

- Default local behavior: `RESULT_DIR=results/<timestamp>`
- Docker compose default in this repo: `RESULT_DIR=/workspace/results/latest` inside the container (volume-mapped to `results/latest` on the host)

Expected layout:
- `$RESULT_DIR/<ConfigName>/...` (OMNeT++ raw outputs such as `.vec`, `.sca`)
- `$RESULT_DIR/analysis.csv`
- `$RESULT_DIR/metrics_plot.png`

For Figure 2 artifact output:
- `results/fig2/analysis.csv`
- `results/fig2/metrics_plot.png`

For Figure 3 artifact output:
- `results/fig3/analysis.csv`
- `results/fig3/metrics_plot.png`

For Figure 5 artifact output:
- `results/fig5/analysis.csv`
- `results/fig5/metrics_plot.png`

`analysis.csv` numeric outputs are written with 6 decimal places, and plots are exported at 300 DPI.

For artifact Claim-1 MWE (`--mwe`), expected layout is:
- `$RESULT_DIR/mwe/analysis.csv`
- `$RESULT_DIR/mwe/metrics_plot.png`

`--mwe` defaults to `MWE_NUM_NODES=1024` and supports larger sizes (for example `2048` or `4096`) through environment override.

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
