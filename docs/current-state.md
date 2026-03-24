# Current State and Known Gaps

This file captures the verified repository state from a source-level documentation audit.

## 1. Verified Working Structure

- Repository layout clearly separates simulation, preprocessing, orchestration, and analysis concerns.
- OMNeT++ is consumed via Git submodule at `third_party/omnetpp`.
- `scripts/run_experiments.sh` provides the canonical end-to-end path: topology preprocessing, simulation runs, analysis, and plotting.
- Docker compose uses the same script-based workflow (`--skip-sim-build`), keeping execution paths aligned.

## 2. Verified Gaps (Current)

### 2.1 Runtime topology x fault matrix selection

- Default `run_experiments.sh` now executes all three fault scenarios on each topology:
  - CBT: `FaultDistance`, `FaultBetaCBT`, `FaultParentCBT`
  - ER: `FaultDistanceER`, `FaultBeta`, `FaultParentER`
  - Twitch: `FaultDistanceTwitch`, `FaultBetaTwitch`, `FaultParent`
- Baseline runs for each topology are included in default mode (`BaselineCBT`, `BaselineER`, `BaselineTwitch`).
- `SCMNetwork` remains the global default for configs that do not set `network` in scenario config.

### 2.2 Edge-file wiring status

- `run_experiments.sh` generates `cbt_edges.txt` and `er_edges.txt` per run.
- `CompleteBinaryTree` and `ErdosRenyi` now consume these files via runtime initializer modules.
- Invalid/missing edge files fail fast during network initialization.

### 2.3 Twitch path in default pipeline

- Twitch baseline/fault scenarios are now part of the default matrix.
- `run_experiments.sh` generates a deterministic `twitch_edges.txt` input and copies it to each scenario result directory.
- `BaselineTwitch` consumes `${resultdir}/twitch_edges.txt` consistently.

### 2.5 Figure-5 algorithm comparison path

- `--claim-b` now provides a dedicated algorithm comparison matrix (SCM/Garg-Grosu/Byrenheid) without changing default quick/full behavior.
- `build_fig5_outputs.py` now reads `algorithm` and `topology` directly from simulation exports, removing scenario-name heuristic labeling.

### 2.6 MWE metrics plot semantics

- `--mwe` now targets the strict parent-manipulation level-sweep objective:
  - all three topologies at 1023 nodes
  - deterministic one-node-per-level parent corruption (levels 1..9)
  - x-axis uses SCM corruption level from algorithm state
- `scripts/analysis/build_mwe_outputs.py` writes:
  - `analysis.csv` (`topology,corruption_level,avg_beta_pct_increase,avg_payment_pct_increase,user_service_fraction`)
  - `metrics_plot.png` (composite 3-panel figure)
  - `beta_increase_vs_level.png`
  - `payment_increase_vs_level.png`
  - `service_fraction_vs_level.png`

### 2.4 Legacy Docker entrypoint drift

- `docker/entrypoint.sh` is not the path used by current compose flow.
- It contains path assumptions (`-n ../networks`) that differ from active `run_experiments.sh` usage.

## 3. Documentation Quality Status

- `README.md`, `docs/setup.md`, `docs/design.md`, and this file were aligned to current executable behavior in the latest pass.
- `docs/results/README.md` now documents concrete output artifacts and CSV schema.

## 4. Recommended Bring-up Sequence

1. `git submodule update --init --recursive`
2. `uv sync`
3. Build OMNeT++ (native path) if needed
4. Run `./scripts/run_experiments.sh --quick`
5. Run `./scripts/run_experiments.sh`
6. Verify output artifacts (`analysis.csv`, `metrics_plot.png`)

## 5. Scope Note

This document tracks integration and execution-state observations, not algorithmic proof/correctness claims.
