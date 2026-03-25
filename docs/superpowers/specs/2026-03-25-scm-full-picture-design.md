# SCM Overlay: The Full Picture

A comprehensive guide to the Self-Stabilizing Multicast Overlay (SCM) simulation project — what it does, how it works, and the state it's in.

Written for a tech-savvy general audience. No networking or simulation background assumed.

---

## Part 1 — The Story

### The Problem: Who Pays When Millions Watch?

Imagine a live football match being streamed to 10 million viewers. The broadcaster can't send a separate copy of the video stream to each viewer — that would require 10 million outgoing connections and melt any server on Earth.

Instead, the internet uses something called **multicast**: the broadcaster sends one copy, and it gets duplicated at various relay points along the way, forming a tree structure:

```
        [Broadcaster]           sends 1 copy
           /    \
      [Relay A]  [Relay B]     each duplicates and forwards
       / \        / \
     [V1][V2]  [V3][V4]       viewers receive the stream
```

Every relay node is doing work — forwarding data costs bandwidth, and bandwidth costs money. The fundamental question is: **how should the cost be shared fairly among all participants?**

This is not a hypothetical. Content delivery networks (CDNs), peer-to-peer streaming, and overlay networks all face this exact problem. If the cost-sharing is unfair, rational participants will leave the network. If a relay node crashes, the viewers below it lose their stream.

### The Solution: Self-Stabilizing Cost Sharing

The academic paper behind this project proposes an algorithm called **SCM (Self-stabilizing Multicast Cost Sharing)**. It does two things:

1. **Fair cost sharing**: Every node calculates how much it should pay based on its position in the tree, how many viewers it serves, and the cost of its network link. The algorithm ensures no node overpays — and proves it mathematically.

2. **Self-stabilization**: If anything goes wrong — a node crashes, a value gets corrupted, a link breaks — the system **automatically repairs itself** without any external intervention. No admin needs to restart anything. No monitoring system needs to detect the failure. The nodes themselves detect inconsistencies and reorganize.

Think of it like a self-healing bone. Break it, and given enough time, it repairs itself back to a correct state. The algorithm guarantees that no matter how badly the system is corrupted, it will always converge back to a legitimate, fair state.

### Why Simulate It?

You can't easily break a real internet infrastructure to test whether your algorithm recovers. So researchers use **simulation** — they build a virtual network, run the algorithm, deliberately inject faults, and measure whether and how fast the system recovers.

This project uses **OMNeT++**, a discrete event simulation framework. Think of it as a flight simulator, but for computer networks. You define a network topology (which nodes exist and how they're connected), program the behavior of each node (the algorithm), and then run simulated time forward while measuring what happens.

### The Academic Context

This simulation is being prepared for submission to **DSN 2026** (Dependable Systems and Networks), a top-tier academic conference. DSN has a separate **artifact evaluation** track where reviewers clone your code, build it, run it, and verify that it produces the results claimed in the paper. The artifact needs to:

- Build from source (or Docker) without errors
- Run within ~4 hours of an evaluator's time
- Produce figures that match the paper's claims
- Be archived on Zenodo with a DOI

---

## Part 2 — The Science

### The 7 Rules

Every node in the network runs the same algorithm. On every "tick" of the simulation clock, each node checks a series of rules in order. The first rule that applies gets executed.

| Rule | Name | What it does | Analogy |
|------|------|-------------|---------|
| **1** | Root Init | Node 0 declares itself the root of the tree | "I'm the boss, I start everything" |
| **2** | Better Parent | A stable node checks if there's a better parent nearby (closer to the root) | "Is there a shorter path to the source?" |
| **3** | Error Detection | A node checks if its state is consistent with its parent's state | "Does my paycheck match what HR says?" |
| **4** | Recovery Start | A faulty node whose children are all aware of the problem begins recovering | "My team knows about the issue, I can start fixing things" |
| **5** | Rejoin | A recovering node finds a stable parent and rejoins the tree | "I found a new boss who's still working" |
| **6** | Sign State | A stable node cryptographically signs its state (subtree size and beta value) | "I'm putting my official signature on my report" |
| **7** | Build Proof | A stable node whose children all have proofs builds a cryptographic proof chain | "I'm assembling an audit trail from my entire department" |

Rules 1-5 handle **self-stabilization** (recovering from faults). Rules 6-7 handle **cryptographic auditing** (proving nobody is cheating on their payments).

### The Key Numbers: Alpha and Beta

**Alpha (subtree size)**: The total number of users in a node's subtree. Computed as the node's own `numUsers` plus the subtree sizes of all its children. For example, if a node has `numUsers = 3` and two children with subtree sizes of 5 each, its alpha is 3 + 5 + 5 = 13.

**Beta (cost share)**: How much a node pays per user. Calculated as: parent's beta + (link cost / subtree size). The deeper you are in the tree and the fewer users you serve, the higher your beta. This ensures costs are shared proportionally.

**Payment**: Simply beta times the number of local users. If your beta is $0.05 and you have 3 users, you pay $0.15.

### The Three Fault Types

The simulation deliberately breaks things to test whether the algorithm recovers:

| Fault | What it does | Real-world equivalent |
|-------|-------------|----------------------|
| **Distance Tamper** | Changes a node's level (distance from root) | A router lying about how far it is from the source |
| **Beta Modification** | Inflates a node's payment value by 1.5x | Corrupted billing data |
| **Parent Switch** | Changes who a node thinks its parent is | A router pointing to the wrong upstream |

### The Cryptographic Proof Chain

Why do nodes sign things? Because in a decentralized network, nodes might **lie** to reduce their payments. A node could claim a larger subtree size (to lower its beta) or report a lower payment.

The proof chain works like this:
1. Each node signs its subtree size and beta value using ECDSA (elliptic curve cryptography — the same math behind Bitcoin)
2. Each node collects its children's signatures
3. The root node can then verify the entire tree's accounting by checking the chain of signatures from leaf to root

If a node is caught cheating, it gets penalized — its payment is increased.

### What the Paper Claims

The paper makes several claims that the simulation tests:

- **Claim A (Figures 2-3)**: The algorithm correctly distributes costs and produces compact cryptographic proofs. Measured by: beta distribution before/after faults, proof sizes in bytes.
- **Claim B (Figure 5)**: The algorithm converges faster than competing algorithms (Garg-Grosu, Byrenheid). Measured by: time to reach stable state after faults.

---

## Part 3 — The Code Architecture

### The Four Layers

```
Layer 1: NED Files          "Blueprints"     What the network looks like
Layer 2: C++ Source          "Logic"          How nodes behave
Layer 3: Python Scripts      "Analysis"       How results become figures
Layer 4: Docker              "Packaging"      How to run it all reproducibly
```

### Layer 1: NED Files — The Blueprints

OMNeT++ uses a domain-specific language called NED to describe network topology. NED files are like architectural blueprints — they define what exists and how it's connected, but not how it behaves.

**Location:** `omnetpp/simulations/networks/`

| File | What it defines |
|------|----------------|
| `SCMNode.ned` | A single node — has an ID, number of users, link cost, and bidirectional ports |
| `SCMFaultInjector.ned` | The fault injection module — parameters for fault type, probability, interval |
| `SCMNetwork.ned` | The default network — N nodes with random 30% connections (development scaffold) |
| `CompleteBinaryTree.ned` | A perfect binary tree — every parent has exactly 2 children |
| `ErdosRenyi.ned` | A random graph — each pair of nodes has some probability of being connected |
| `TwitchNetwork.ned` | A network topology derived from Twitch streaming data. The original dataset has 168K nodes, but the active configs use a 256-node synthetic proxy for faster iteration. |
| `SimpleNetwork.ned` | A 2-node test network |
| `EdgeFileNetworkInitializer.ned` | A helper module that reads connections from a file at runtime |
| `TwitchNetworkInitializer.ned` | A helper module that reads Twitch edge data at runtime |

### Layer 2: C++ Source — The Logic

This is where the actual algorithm lives. OMNeT++ calls C++ code to handle events (messages arriving, timers firing, etc).

**Location:** `omnetpp/simulations/src/`

| File | What it does |
|------|-------------|
| `SCMNode.h / SCMNode.cc` | **The heart of the project.** ~880 lines combined implementing all 7 rules, cost calculations, cryptographic signing, and proof verification. Every node in the simulation is an instance of this class. |
| `SCMMessages.h / SCMMessages.cc` | Defines the message types nodes send to each other: ALPHA_UPDATE, BETA_UPDATE, FAULT_NOTIFY, PROOF_REQUEST, PROOF_RESPONSE. The `.cc` is just a one-line registration macro; all logic is in the header. |
| `SCMFaultInjector.h / SCMFaultInjector.cc` | The saboteur. Periodically corrupts random nodes to test self-stabilization. Supports probabilistic per-node injection and deterministic depth-based campaigns. |
| `TopologyBuilders.cc` | Three helper classes (CBTBuilder, ERBuilder, TwitchBuilder) that can programmatically build network topologies. Currently unused — topologies are built by NED files and EdgeFileNetworkInitializer instead. |
| `EdgeFileNetworkInitializer.cc` | Reads an edge-list file (pairs of node IDs) and creates bidirectional connections at simulation startup. Used by CompleteBinaryTree and ErdosRenyi configs. |
| `TwitchNetworkInitializer.cc` | Same concept but for the Twitch dataset format. Parses CSV-style edge files. |
| `Main.cc` | Empty placeholder. OMNeT++ provides its own main() function. |

### Layer 3: Python Scripts — The Analysis

Raw simulation output is a stream of timestamped events in OMNeT++ vector files (`.vec`). Python scripts process these into human-readable CSV tables and figures.

**Location:** `scripts/`

| File | What it does |
|------|-------------|
| `run_experiments.sh` | **The master pipeline.** Generates topologies, builds the simulation, runs all configs, processes results, generates plots. Supports `--quick` (smoke test), `--skip-sim-build` (Docker), `--mwe` (minimum working example), `--claim-a` and `--claim-b` (paper-specific figure pipelines). |
| `preprocess/generate_cbt.py` | Generates a complete binary tree as an edge-list file |
| `preprocess/generate_er.py` | Generates an Erdos-Renyi random graph as an edge-list file |
| `preprocess/process_twitch.py` | Preprocesses the Twitch social network dataset |
| `analysis/process_results.py` | Parses .vec files and produces a summary analysis.csv |
| `analysis/build_fig2_outputs.py` | Produces data for Figure 2 (beta/payment increase under faults) |
| `analysis/build_fig3_outputs.py` | Produces data for Figure 3 (proof sizes: SCM vs Garg-Grosu) |
| `analysis/build_fig5_outputs.py` | Produces data for Figure 5 (convergence time comparison) |
| `analysis/build_mwe_outputs.py` | Produces data for the Minimum Working Example |
| `analysis/validate_outputs.py` | Checks that output CSVs have expected columns and PNGs exist |
| `visualization/plot_metrics.py` | Generates bar charts and annotated plots from analysis CSVs |
| `analysis/run_experiments.py` | Empty stub file — not used. A leftover from early development. |

### Layer 4: Docker — The Packaging

**Location:** `docker/`

| File | What it does |
|------|-------------|
| `Dockerfile` | Multi-stage build: Stage 1 installs deps + compiles simulation. Stage 2 copies artifacts into a runtime image (same `omnetpp/omnetpp:u22.04-6.0` base). Uses `SHELL ["/bin/bash", "-c"]` to enable `source` in RUN commands. |
| `docker-compose.yml` | One-command execution: `docker compose up --build` runs the full pipeline. |

### How the Pieces Connect

```
omnetpp.ini          defines which network + parameters to use
    |
    v
*.ned files          define the network topology (nodes, connections)
    |
    v
*.cc files           implement node behavior (algorithm, messages, faults)
    |
    v
OMNeT++ runtime      executes the simulation, produces .vec/.sca output files
    |
    v
process_results.py   parses .vec files into analysis.csv
build_fig*.py        parses node_state.csv into figure-specific CSVs
    |
    v
plot_metrics.py      reads CSVs and generates PNG figures
```

### The Configuration File: omnetpp.ini

This is the control panel for all experiments. It defines:

- **[General]**: Default settings — which network, simulation time limit (30s), result recording
- **[Config BaselineCBT]**: 31-node binary tree, no faults (baseline measurement)
- **[Config FaultDistance]**: Same tree, 10% of nodes get distance-tampered every 10s
- **[Config BaselineER]**: 50-node random graph, no faults
- **[Config FaultBeta]**: Same graph, 15% of nodes get beta-modified
- **[Config BaselineTwitch]**: Twitch social network topology, no faults
- **[Config FaultParent]**: Same network, 5% of nodes get parent-switched

Each config runs 3 times (`repeat = 3`) for statistical reliability — OMNeT++ uses a different random number seed for each repetition so the results can be averaged to reduce noise.

### Testing and CI

The project has a CI workflow (`.github/workflows/ci-smoke.yml`) that runs on every PR and push to main. It does two things:

1. **Preprocessing unit tests** (`tests/test_preprocess.py`): Verifies graph generators produce correct output
2. **Docker smoke test**: Builds the Docker image, runs the pipeline, and validates that output CSVs exist with expected columns

**Known gap**: The tests verify file format but not scientific correctness. See issue #52 for details.

### The OMNeT++ Submodule

OMNeT++ itself (the simulation engine) is pulled in as a Git submodule at `third_party/omnetpp`, pinned to version 6.3.0. This means:
- Native builds compile OMNeT++ from source (one-time ~15 min build)
- Docker uses a pre-built OMNeT++ 6.0 image (different version — a known inconsistency)
- The submodule is ~500MB so `git clone --recurse-submodules` is slow

---

## Part 4 — The Pipeline

### What Happens When You Run It

**Docker path** (`docker compose up --build`):
```
1. Docker builds the image
   - Installs Python deps via uv
   - Compiles C++ simulation with opp_makemake

2. Container starts, runs run_experiments.sh --skip-sim-build
   - Generates topology edge files (CBT, ER, Twitch proxy)
   - Runs 12 configs x 3 repeats = 36 simulation runs
   - Each run: 30 simulated seconds, takes <1 real second
   - Processes results into analysis.csv
   - Generates metrics_plot.png

3. Results appear in results/ directory on host
```

**Native path** (`./scripts/run_experiments.sh`):
```
1. Script auto-detects OMNeT++ (submodule or Docker)
2. Sources setenv, builds simulation binary via opp_makemake
3. Same pipeline as Docker: generate → simulate → analyze → plot
```

**Quick smoke test** (`./scripts/run_experiments.sh --quick`):
```
Runs only BaselineCBT (1 config x 3 repeats) — completes in seconds
```

### The Experiment Matrix

The full run (non-quick mode) executes 12 configs across 3 topologies and 3 fault types:

| Config | Network | Nodes | Fault Type | Fault Prob |
|--------|---------|-------|------------|-----------|
| BaselineCBT | CompleteBinaryTree | 31 | None | 0 |
| FaultDistance | CompleteBinaryTree | 31 | Distance tamper | 10% |
| FaultBetaCBT | CompleteBinaryTree | 31 | Beta modification | 15% |
| FaultParentCBT | CompleteBinaryTree | 31 | Parent switch | 5% |
| BaselineER | ErdosRenyi | 50 | None | 0 |
| FaultDistanceER | ErdosRenyi | 50 | Distance tamper | 10% |
| FaultBeta | ErdosRenyi | 50 | Beta modification | 15% |
| FaultParentER | ErdosRenyi | 50 | Parent switch | 5% |
| BaselineTwitch | TwitchNetwork | 256 | None | 0 |
| FaultDistanceTwitch | TwitchNetwork | 256 | Distance tamper | 10% |
| FaultBetaTwitch | TwitchNetwork | 256 | Beta modification | 15% |
| FaultParent | TwitchNetwork | 256 | Parent switch | 5% |

Additional configs exist for algorithm comparisons (Garg-Grosu, Byrenheid) via `--claim-a` and `--claim-b` flags.

### From Raw Output to Paper Figure

```
Simulation run
    |
    v
.vec files (time-series: signal name, timestamp, value)
.sca files (scalar summaries)
mwe_node_state.csv (per-node state snapshot)
    |
    v
build_fig2_outputs.py  -->  fig2 analysis.csv  -->  Figure 2 (beta/payment % increase)
build_fig3_outputs.py  -->  fig3 analysis.csv  -->  Figure 3 (proof size comparison)
build_fig5_outputs.py  -->  fig5 analysis.csv  -->  Figure 5 (convergence comparison)
```

---

## Part 5 — The Bugs, The Fixes, The Remaining Issues

### What Was Broken at the Start

When this project was first examined, it could not compile. Here's what was wrong:

| Issue | What was broken | Why it happened |
|-------|----------------|-----------------|
| Makefile case mismatch | `src/main.cc` vs actual `src/Main.cc` | Linux is case-sensitive, macOS/Windows are not — code developed on one, deployed on the other |
| Missing NED file | `SCMFaultInjector.ned` was imported but never created | Forgotten during development |
| Gate name inconsistency | NED files said `in[]/out[]`, C++ said `port` | Two naming conventions used in parallel, never reconciled |
| Message API mismatch | Header declared methods that didn't exist in the implementation | Rapid prototyping — declarations written before implementations |
| ~20 missing method implementations | Header declared `calculateAlpha()`, `calculateBeta()`, etc. — none implemented | Same as above |
| `vector = nullptr` | C++ vectors can't be assigned null — they're not pointers | Common mistake when mixing C and C++ mental models |
| Empty requirements.txt | Python dependencies not listed | Dependencies installed manually, never recorded |
| Docker base image wrong | `omnetpp/omnetpp:6.0.1-ubuntu20.04` doesn't exist | Wrong tag format — actual format is `u22.04-6.0` |

### What Was Fixed

Over the course of this work:

1. **All 8 original compile blockers** were fixed (GitHub issues #1-#8, all closed)
2. **Docker pipeline** now works end-to-end: `docker compose up --build` produces results
3. **Native pipeline** works: `./scripts/run_experiments.sh` builds and runs on Ubuntu
4. **Python dependencies** managed via `uv` with pinned lockfile for reproducibility
5. **NED files** cleaned up: removed invalid imports, added package declarations, fixed gate naming
6. **OMNeT++ configuration** fixed: network-per-config, parameter defaults, unit annotations
7. **Result recording** enabled: `.vec` files now produced with vector+scalar recording
8. **Simulation time limit** added: 30s per config prevents infinite runs
9. **Documentation** updated: `current-state.md`, `setup.md`, `design.md` reflect reality

### What's Still Open

13 issues remain on GitHub (#40-#52), categorized by severity:

**Scientific Integrity (SEV-1) — Could cause DSN rejection:**
- **#40**: Figure 3 presents an analytical formula as if it were measured data (Garg-Grosu proof size)
- **#41**: Figure 5 uses different correctness criteria for SCM vs Garg-Grosu (biased comparison)
- **#42**: Missing stabilization signal falls back to sim-time-limit, inflating convergence times
- **#43**: Stabilization metric emitted with stale initial value, corrupting measurements
- **#52**: Test suite only checks file format, not scientific correctness — CI can pass with wrong results

**Memory Safety / Runtime (SEV-2):**
- **#44**: OpenSSL key creation not null-checked — crash if crypto init fails
- **#45**: ECDSA signature size assigned to unsigned int — crash on error
- **#46**: Node lookup doesn't null-check parent module — crash during teardown
- **#47**: Docker uses `uv:latest` floating tag — not reproducible over time
- **#48**: Twitch preprocessing has no default seed — non-deterministic
- **#49**: TwitchNetwork fault injector parameters not wired from parent NED

**Code Quality (SEV-3):**
- **#50**: Copy-paste duplication across topology builders, dead code, misleading column names
- **#51**: CI missing timeouts, Docker cache, artifact uploads, concurrency guards

---

## Part 6 — Quick Reference

### Commands Cheat Sheet

| I want to... | Command |
|---|---|
| Run everything in Docker | `cd docker && docker compose up --build` |
| Quick smoke test (native) | `./scripts/run_experiments.sh --quick` |
| Full experiment run (native) | `./scripts/run_experiments.sh` |
| Skip recompilation | `./scripts/run_experiments.sh --skip-sim-build` |
| Install Python deps | `uv sync` |
| Build OMNeT++ (first time) | `cd third_party/omnetpp && source setenv && ./configure WITH_QTENV=no WITH_OSG=no && make -j$(nproc)` |
| Run unit tests | `uv run python -m unittest tests/test_preprocess.py -v` |
| Check OMNeT++ version | `./scripts/check_omnetpp_version.sh` |

### Glossary

| Term | Meaning |
|------|---------|
| **OMNeT++** | Open-source discrete event simulation framework for networks. Think "flight simulator for networks." |
| **NED** | Network Description language — OMNeT++'s DSL for defining network topology |
| **Config** | A named experiment scenario in `omnetpp.ini` (e.g., `[Config BaselineCBT]`). Each config can override parameters from `[General]` or extend another config. |
| **Cmdenv** | Command-line execution mode for OMNeT++ (no GUI needed) |
| **Qtenv** | GUI execution mode for OMNeT++ (step-through debugging with visuals) |
| **SCM** | Self-stabilizing Multicast Cost Sharing — the algorithm this project implements |
| **Alpha** | Subtree size — total number of users served by a node and all its descendants |
| **Beta** | Per-user cost share — how much each user at a node should pay |
| **Self-stabilizing** | A system that automatically recovers from any fault to a correct state |
| **ECDSA** | Elliptic Curve Digital Signature Algorithm — crypto used for proof signing |
| **`.vec` file** | OMNeT++ vector output — time-series data (signal name, timestamp, value) |
| **`.sca` file** | OMNeT++ scalar output — single summary values per simulation run |
| **`opp_makemake`** | OMNeT++ build tool that generates Makefiles for simulation projects |
| **`uv`** | Fast Python package manager (Rust-based replacement for pip) |
| **DSN** | Dependable Systems and Networks — the academic conference this targets |
| **Artifact evaluation** | Conference process where reviewers verify that submitted code reproduces paper results |
| **Zenodo** | Academic archival platform for research artifacts (assigns DOIs) |
