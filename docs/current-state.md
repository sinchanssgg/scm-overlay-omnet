# Current State and Known Gaps

This file captures the verified repository state from a source-level documentation audit.

## 1. Verified Working Structure

- Repository layout clearly separates simulation, preprocessing, orchestration, and analysis concerns.
- OMNeT++ is consumed via Git submodule at `third_party/omnetpp`.
- `scripts/run_experiments.sh` provides the canonical end-to-end path: topology preprocessing, simulation runs, analysis, and plotting.
- Docker compose uses the same script-based workflow (`--skip-sim-build`), keeping execution paths aligned.

## 2. Verified Gaps (Current)

### 2.1 Baseline runtime network selection

- `BaselineCBT` and `FaultDistance` explicitly run on `CompleteBinaryTree`.
- `BaselineER` and `FaultBeta` explicitly run on `ErdosRenyi`.
- `SCMNetwork` remains the global default for configs that do not set `network` in scenario config.

### 2.2 Preprocessing artifacts are not fully wired into active topology behavior

- `run_experiments.sh` generates `cbt_edges.txt` and `er_edges.txt`.
- The active `SCMNetwork` currently creates random links directly in NED and does not consume those generated edge files.

### 2.3 Twitch scenario is not part of default pipeline

- Twitch configs exist in `omnetpp.ini`, and Twitch initializer code exists.
- Default `run_experiments.sh` matrix does not execute Twitch scenarios.
- `BaselineTwitch` expects `twitch_edges.txt` in result directory, but default pipeline does not generate it.
- `scripts/preprocess/process_twitch.py` defaults to `twitch_processed.txt`; use `--output twitch_edges.txt` or adjust `omnetpp.ini` to avoid filename mismatch.

### 2.5 Figure-5 algorithm comparison path

- `--claim-b` now provides a dedicated algorithm comparison matrix (SCM/Garg-Grosu/Byrenheid) without changing default quick/full behavior.
- `build_fig5_outputs.py` now reads `algorithm` and `topology` directly from simulation exports, removing scenario-name heuristic labeling.

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
