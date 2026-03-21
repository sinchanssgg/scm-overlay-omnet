# Setup Guide

This document explains how to prepare the environment for this repository and how to start the project using either Docker or native OMNeT++.

## 1. What Is In This Repo

- `omnetpp/simulations`: SCM simulation code and NED network descriptions
- `scripts/`: preprocessing, analysis, and visualization scripts
- `third_party/omnetpp`: upstream OMNeT++ submodule pinned for reproducibility
- `docker/`: containerized build/run workflow

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
cd ../omnetpp/simulations
make
```

Important:
- The current repository has known build/wiring gaps. See `docs/current-state.md` for required fixes before this succeeds.

## 4. Docker Setup

### 4.1 Build and Run Compose Stack

```bash
cd docker
docker compose up --build
```

This starts:
- simulation service
- analysis service
- visualization service

Expected output location:
- `results/`

## 5. Pipeline Entry Points

- Main local pipeline script: `scripts/run_experiments.sh`
- Docker pipeline entrypoint: `docker/entrypoint.sh`
- Simulation config file: `omnetpp/simulations/omnetpp.ini`

## 6. Topology and Dataset Inputs

- Generated topologies:
	- CBT: `scripts/preprocess/generate_cbt.py`
	- ER: `scripts/preprocess/generate_er.py`
- Twitch preprocessing:
	- `scripts/preprocess/process_twitch.py`

The Twitch network mode expects an edge file path and large node count defaults; validate paths and memory constraints before running that scenario.

## 7. First Practical Bring-Up Order

1. Read `docs/current-state.md`
2. Apply listed build/wiring fixes
3. Build OMNeT++ if using native path
4. Build `omnetpp/simulations`
5. Run one baseline scenario first (CBT or ER)
6. Run full pipeline and confirm analysis/plot outputs

## 8. Version Awareness (Pinned vs Latest)

Use the helper script to check:

```bash
./scripts/check_omnetpp_version.sh
```

It reports:
- current submodule commit
- current tag (if exactly on a tag)
- latest upstream `omnetpp-*` tag

This keeps research runs reproducible while still monitoring new releases.
