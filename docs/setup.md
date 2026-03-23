# Setup Guide

This document explains how to prepare the environment for this repository and how to start the project using either Docker or native OMNeT++.

## 1. What Is In This Repo

- `omnetpp/simulations`: SCM simulation code and NED network descriptions
- `scripts/`: preprocessing, analysis, and visualization scripts
- `third_party/omnetpp`: upstream OMNeT++ submodule pinned for reproducibility
- `docker/`: containerized build/run workflow

## 1.1 First-time Repository Bootstrap

From repository root:

```bash
git submodule update --init --recursive
uv sync
```

## 2. Dependency Overview

You need two dependency sets:

1. OMNeT++ toolchain dependencies (C/C++ build, simulation runtime)
2. Python dependencies (preprocessing and postprocessing)

### 2.1 OMNeT++ Dependencies (Ubuntu)

Based on OMNeT++ install guide sources in the submodule (`third_party/omnetpp/doc/...`):

```bash
sudo apt-get update
sudo apt-get install build-essential clang lld gdb bison flex perl \
		python3 python3-pip qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
		libqt5opengl5-dev libxml2-dev zlib1g-dev doxygen graphviz \
		libwebkit2gtk-4.0-37 xdg-utils
```

Optional:

```bash
sudo apt-get install mpi-default-dev
```

Project-specific C++ dependency:

```bash
sudo apt-get install libssl-dev
```

### 2.2 Python Dependencies

Managed via [uv](https://docs.astral.sh/uv/). Dependencies are declared in `pyproject.toml` and pinned in `uv.lock`.

Install uv (if not already installed):

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

Install dependencies:

```bash
uv sync
```

Run any Python script through the managed environment:

```bash
uv run python scripts/analysis/process_results.py results/latest
```

## 3. Native OMNeT++ Setup

### 3.1 Build OMNeT++

```bash
cd third_party/omnetpp
source setenv
./configure
make -j$(nproc)
```

Notes:
- You must source `setenv` (do not execute it directly).
- If Qt/GUI dependencies are unavailable, OMNeT++ can be configured for command-line-only workflows by disabling Qtenv in `configure.user` and re-running configure.

### 3.2 Build SCM Simulation Project

```bash
cd ../../omnetpp/simulations
make
```

## 4. Docker Setup

### 4.1 Build and Run Compose Stack

```bash
cd docker
docker compose up --build
```

Current compose setup starts one `simulation` service that runs the full pipeline (`run_experiments.sh`), including simulation, analysis, and plotting.

Expected output location:
- `results/latest/` (mapped from `/workspace/results/latest` in container)
- plus timestamped run directories under `results/` if configured

## 5. Pipeline Entry Points

- Main local pipeline script: `scripts/run_experiments.sh`
- Docker compose command: `scripts/run_experiments.sh --skip-sim-build`
- Simulation config file: `omnetpp/simulations/omnetpp.ini`

## 6. Topology and Dataset Inputs

- Generated topologies:
	- CBT: `scripts/preprocess/generate_cbt.py`
	- ER: `scripts/preprocess/generate_er.py`
- Twitch preprocessing:
	- `scripts/preprocess/process_twitch.py`

The Twitch network mode expects an edge file path and large node count defaults; validate paths and memory constraints before running that scenario.

## 7. First Practical Bring-Up Order

1. Initialize submodules and Python dependencies
2. Build OMNeT++ if using native path
3. Run one baseline scenario first: `./scripts/run_experiments.sh --quick`
4. Run full pipeline: `./scripts/run_experiments.sh`
5. Confirm outputs (`analysis.csv` and `metrics_plot.png`) exist in the result directory (`$RESULT_DIR`; see below)

## 8. Expected Outputs and Validation

On success, the pipeline writes artifacts under `$RESULT_DIR`.
By default this is a timestamped folder such as `results/<timestamp>`.
In docker compose for this repository, `RESULT_DIR` is set to `results/latest`.

Expected layout:
- Per-scenario simulator outputs under `$RESULT_DIR/<ConfigName>/`
- Aggregated CSV: `$RESULT_DIR/analysis.csv`
- Plot image: `$RESULT_DIR/metrics_plot.png`

Helpful log markers:
- `Found <N> .vec file(s)` (analysis step)
- `Saved analysis to .../analysis.csv`
- `Saved plot to .../metrics_plot.png`

## 9. Version Awareness (Pinned vs Latest)

Use the helper script to check:

```bash
./scripts/check_omnetpp_version.sh
```

It reports:
- current submodule commit
- current tag (if exactly on a tag)
- latest upstream `omnetpp-*` tag

This keeps research runs reproducible while still monitoring new releases.
