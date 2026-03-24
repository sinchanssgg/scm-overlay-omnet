# Copilot Instructions for `scm-overlay-omnet`

## Build, test, and lint commands

### Environment/bootstrap
- Initialize submodules and Python deps:
  - `git submodule update --init --recursive`
  - `uv sync`

### Build (native)
- Build OMNeT++ (one-time or after OMNeT++ updates):
  - `cd third_party/omnetpp`
  - `source setenv`
  - `./configure`
  - `make -j$(nproc)`
- Build SCM simulation binary:
  - `cd omnetpp/simulations`
  - `opp_makemake -f --deep -o scm-simulations -I/usr/include -lssl -lcrypto`
  - `make -j$(nproc)`

### Test/smoke run
- Quick smoke run (single scenario path):
  - `./scripts/run_experiments.sh --quick`
- Full scenario matrix run:
  - `./scripts/run_experiments.sh`
- Run a single scenario directly (closest equivalent to a single test):
  - `cd omnetpp/simulations`
  - `./scm-simulations -u Cmdenv -c BaselineCBT -n networks --result-dir="$PWD/../../results/manual/BaselineCBT"`

### Analysis/plot only
- Aggregate results into CSV:
  - `uv run python scripts/analysis/process_results.py <RESULT_DIR>`
- Generate plot from `analysis.csv`:
  - `uv run python scripts/visualization/plot_metrics.py <RESULT_DIR>`

### Linting
- No repository-level lint command is currently defined in root tooling/docs.

## High-level architecture

This repo is a layered experiment pipeline:

1. **Simulation core (`omnetpp/simulations`)**
   - C++ OMNeT++ modules implement SCM protocol logic (`SCMNode`), fault injection (`SCMFaultInjector`), and topology initializers/builders.
   - Scenario definitions are in `omnetpp.ini`.

2. **Topology/data preparation (`scripts/preprocess`)**
   - `generate_cbt.py`, `generate_er.py`, and `process_twitch.py` prepare edge-list inputs.

3. **Orchestration (`scripts/run_experiments.sh`, `docker/`)**
   - Canonical execution path is `scripts/run_experiments.sh`.
   - Pipeline order: create result dir -> generate topologies -> (optional) build sim -> run configs -> analysis -> plotting.
   - Docker compose runs the same script with `--skip-sim-build` and sets `RESULT_DIR=/workspace/results/latest`.

4. **Postprocessing (`scripts/analysis`, `scripts/visualization`)**
    - `.vec` files are parsed recursively into per-scenario aggregates in `analysis.csv`.
    - For top-priority MWE, `scripts/analysis/build_mwe_outputs.py` generates level-sweep artifacts under `<RESULT_DIR>/mwe/`:
      - `analysis.csv` with `topology,corruption_level,avg_beta_pct_increase,avg_payment_pct_increase,user_service_fraction`
      - `metrics_plot.png` (composite 3-panel line figure)
      - `beta_increase_vs_level.png`
      - `payment_increase_vs_level.png`
      - `service_fraction_vs_level.png`

## Key repository conventions

- Use `uv run python ...` for Python scripts to guarantee the managed environment from `pyproject.toml`/`uv.lock`.
- `scripts/run_experiments.sh` is the source of truth for runnable scenario selection:
  - `--quick` => `BaselineCBT`
  - default => full 3x3 topology x fault matrix across CBT/ER/Twitch:
    - baselines: `BaselineCBT`, `BaselineER`, `BaselineTwitch`
    - faults per topology: `FaultDistance*`, `FaultBeta*`, `FaultParent*`
- The top-priority MWE objective is level-sweep parent-manipulation resilience at 1023 nodes across CBT/ER/Twitch:
  - x-axis: SCM corruption level (1..9)
  - y-axes: beta % increase, payment % increase, user service fraction [0,1]
  - one line per topology in each plot
- `omnetpp.ini` sets global `network = SCMNetwork`; active scenario configs in the default run matrix explicitly override this with dedicated NED networks (`CompleteBinaryTree`, `ErdosRenyi`, `TwitchNetwork`).
- `RESULT_DIR` defaults to `results/<timestamp>`; if unwritable, script falls back to `${XDG_STATE_HOME:-$HOME/.local/state}/scm-overlay-omnet/results/<timestamp>`.
- Runtime outputs belong under top-level `results/`; `docs/results/` is for narrative documentation, not pipeline outputs.
- OMNeT++ is pinned as a Git submodule (`third_party/omnetpp`) for reproducible runs. Use `scripts/check_omnetpp_version.sh` to compare pinned state vs upstream tags.

## Priority guardrails (artifact goal: do not deviate)

- Core model intent:
  - Root node is provider; downstream nodes are subscribers in a spanning-tree relationship.
  - Malicious nodes can manipulate parent pointers (and, separately, beta), but the current top-priority MWE focuses on parent-pointer manipulation effects.

- Top-priority MWE requirements (strict):
  - Run all three topologies (`CBT`, `ER`, `Twitch`) at `1023` nodes.
  - Use deterministic one-node-per-level corruption for parent-pointer manipulation.
  - Use SCM tree level from algorithm state as the x-axis level definition.
  - Produce three separate line plots, each containing three topology curves:
    1. `avg_beta_pct_increase` vs corruption level
    2. `avg_payment_pct_increase` vs corruption level
    3. `user_service_fraction` (0..1) vs corruption level

- Forbidden plotting patterns:
  - Do not use baseline values as y-axis output for these adversary-response plots.
  - Do not mix topology labels with attack-type labels as if they are the same plotting dimension.
  - Do not emit flat/constant lines that contradict expected variation under level-specific manipulation (unless raw computed data truly supports it, which should be treated as a likely logic/data issue and investigated).

- Engineering behavior:
  - Ask clarifying questions before changing semantics when requirements are ambiguous.
  - Do not introduce alternate objectives or extra plot families without explicit user approval.
