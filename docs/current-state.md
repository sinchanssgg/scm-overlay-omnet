# Current State and Known Gaps

This file captures the verified repository state as of the latest documentation pass.

## 1. What Is Working Structurally

- Repository layout clearly separates simulation, preprocessing, orchestration, and analysis.
- OMNeT++ dependency is now sourced from upstream via submodule at `third_party/omnetpp`.
- Docker and native workflows are both represented.
- Scenario naming and intent are visible in `omnetpp/simulations/omnetpp.ini`.

### 1.1 Version Tracking Policy

- Keep OMNeT++ pinned to a known tag/commit for reproducible runs.
- Monitor upstream releases separately and upgrade intentionally.
- Use `scripts/check_omnetpp_version.sh` for quick pinned-vs-latest visibility.

## 2. Blocking Gaps Before End-to-End Execution

### 2.1 Build Rule Mismatch

- `omnetpp/simulations/Makefile` expects `src/main.cc`
- Actual file is `src/Main.cc`
- Result: `make` fails immediately

### 2.2 Missing NED File

- Several network NED files import `SCMFaultInjector.ned`
- That file is not present in `omnetpp/simulations/networks`

### 2.3 Gate Name Inconsistency

- `SCMNode.ned` declares gates `in[]` and `out[]`
- Other network definitions and C++ logic use `port` gate naming

### 2.4 Message API Inconsistency

- `SCMMessages.h` defines a limited enum set
- `SCMNode.cc` references additional message types not defined there
- `SCMNode.h` references `SCMControlMessage::MsgType`, which does not match current enum naming

### 2.5 Incomplete C++ Implementations

- `SCMNode.h` declares many methods not implemented in `SCMNode.cc`
- This prevents successful linking/compilation

### 2.6 Potential Duplicate Module Definition

- `SCMNode` module definition appears in both `Main.cc` and `SCMNode.cc`

### 2.7 Runtime Path/Output Assumption Issues

- Simulation execution uses `-n ../networks` while networks are in the local `networks/` directory under `omnetpp/simulations`
- Result processing currently scans only top-level `*.vec` files while run scripts emit per-scenario subdirectories

## 3. Documentation Quality Status

- `README.md`, `docs/setup.md`, and `docs/design.md` are now structured and populated.
- `requirements.txt` remains empty and should be aligned with actual Python imports in a later cleanup.

## 4. Recommended Bring-Up Sequence

1. Fix simulation build and NED consistency issues listed above
2. Validate a single baseline scenario from CLI
3. Validate analysis and visualization outputs for that run
4. Then execute full scenario matrix locally or via Docker

## 5. Scope Note

This file is intentionally focused on objective current-state behavior and blockers, not on algorithmic correctness claims.