# Design Deep Dive

This document describes how the SCM overlay project is structured and how the major components are intended to work together.

## 1. System Layers

The repository is organized as a layered simulation workflow:

1. Simulation core
	- OMNeT++ modules, NED networks, and scenario configurations
2. Input/topology preparation
	- Scripts that generate or preprocess graph edge lists
3. Execution orchestration
	- Shell scripts and Docker compose flow
4. Result postprocessing
	- Analysis and visualization scripts

## 2. Simulation Core

Location: `omnetpp/simulations`

### 2.1 C++ Modules

- `SCMNode`: protocol logic, local state, stabilization handling, and proof/signature flow
- `SCMFaultInjector`: fault scenario injection
- `TwitchNetworkInitializer`: runtime connection construction from edge files
- `SCMControlMessage`: control-message abstraction

### 2.2 NED Network Definitions

Location: `omnetpp/simulations/networks`

Declared topologies include:
- `SCMNetwork`
- `CompleteBinaryTree`
- `ErdosRenyi`
- `TwitchNetwork`
- `SimpleNetwork`

### 2.3 Scenario Configuration

Location: `omnetpp/simulations/omnetpp.ini`

Config groups include:
- Baseline scenarios (`BaselineCBT`, `BaselineER`, `BaselineTwitch`)
- Fault scenarios (`FaultDistance`, `FaultBeta`, `FaultParent`)
- Comparison placeholders (`GargGrosu`, `Byrenheid`)

## 3. Data and Topology Preparation

Location: `scripts/preprocess`

- `generate_cbt.py`: deterministic complete binary tree edge list generation
- `generate_er.py`: probabilistic Erdos-Renyi edge list generation
- `process_twitch.py`: CSV edge list preprocessing and optional sampling

These scripts are intended to create topology inputs before simulation runs.

## 4. Experiment Orchestration

### 4.1 Local Script Path

`scripts/run_experiments.sh` performs:

1. result directory setup
2. topology generation
3. scenario execution loop
4. analysis
5. plotting

### 4.2 Docker Path

`docker/docker-compose.yml` defines service stages:

1. `simulation`
2. `analysis`
3. `visualization`

`docker/Dockerfile` builds and packages the simulation environment.

## 5. Result Processing

Location:
- `scripts/analysis/process_results.py`
- `scripts/visualization/plot_metrics.py`

Intended flow:

1. Parse OMNeT++ vector output
2. Aggregate per-scenario metrics
3. Export CSV summary
4. Render summary plots

## 6. OMNeT++ Upstream Submodule

Location: `third_party/omnetpp`

Purpose:
- Uses upstream OMNeT++ directly while pinning a commit/tag for reproducibility
- Keeps project ownership boundaries clear: SCM code here, OMNeT++ maintained upstream

Expected bootstrap:

```bash
source setenv
./configure
make
```

## 7. Design Intent vs Current Implementation

The design intent is coherent: generate topology -> run scenario matrix -> analyze and plot results.

However, the current implementation has unresolved integration issues (build rules, NED/module consistency, message API mismatches, and incomplete methods). Those are tracked in `docs/current-state.md`.
