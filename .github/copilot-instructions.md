# Copilot Instructions for `scm-overlay-omnet`

## Build, test, and lint commands

### Environment/bootstrap
- Initialize submodules and Python deps:
  - `git submodule update --init --recursive`
  - `uv sync`

### Build (native)
- Install required system packages (Ubuntu/Debian):
  - `sudo apt-get install build-essential pkg-config bison flex libssl-dev`
- Build OMNeT++ (one-time or after OMNeT++ updates):
  - `cd third_party/omnetpp`
  - `source setenv`
  - `./configure WITH_QTENV=no WITH_OSG=no`
  - `make -j$(nproc)`
- Build SCM simulation binary:
  - `cd omnetpp/simulations`
  - `opp_makemake -f --deep -o scm-simulations -I/usr/include -lssl -lcrypto`
  - `make -j$(nproc)`
- Note: `WITH_QTENV=no WITH_OSG=no` skips GUI dependencies. This project runs headless via Cmdenv.

### Test/smoke run
- Quick smoke run (single scenario):
  - `./scripts/run_experiments.sh --quick`
- Full scenario matrix run (12 configs x 3 repeats):
  - `./scripts/run_experiments.sh`
- Minimum working example for artifact evaluation:
  - `./scripts/run_experiments.sh --mwe`
- Algorithm comparison (Figure 5 style):
  - `./scripts/run_experiments.sh --claim-b --quick`
- Claim-matrix scaffold scenarios:
  - `./scripts/run_experiments.sh --claim-a`
- Run a single scenario directly:
  - `cd omnetpp/simulations`
  - `./scm-simulations -u Cmdenv -c BaselineCBT -n networks --result-dir="$PWD/../../results/manual/BaselineCBT"`
- Docker (full pipeline, no native deps needed):
  - `cd docker && docker compose up --build`

### Analysis/plot only
- Aggregate results into CSV:
  - `uv run python scripts/analysis/process_results.py <RESULT_DIR>`
- Generate plot from results:
  - `uv run python scripts/visualization/plot_metrics.py <RESULT_DIR>`
- Figure-specific outputs:
  - `uv run python scripts/analysis/build_fig2_outputs.py results/fig2 --result-root <RESULT_DIR>`
  - `uv run python scripts/analysis/build_fig3_outputs.py results/fig3 --cbt-state-dir <RESULT_DIR>/BaselineCBT --twitch-state-dir <RESULT_DIR>/BaselineTwitch --twitch-nodes 50`
  - `uv run python scripts/analysis/build_fig5_outputs.py results/fig5 --result-root <RESULT_DIR>`

### Linting
- No repository-level lint command is currently defined in root tooling/docs.

## High-level architecture

This repo is a layered experiment pipeline:

1. **Simulation core (`omnetpp/simulations`)**
   - C++ OMNeT++ modules implement SCM protocol logic (`SCMNode`), fault injection (`SCMFaultInjector`), and topology initializers.
   - `SCMNode` supports three algorithm variants via the `algorithmVariant` NED parameter:
     - `"scm"` (default) — full self-stabilizing protocol with fault detection and cryptographic proofs
     - `"garg-grosu"` — baseline comparison; convergence detected when beta stabilizes across consecutive rounds
     - `"byrenheid"` — alternative comparison; uses level-based parent scoring
   - Scenario definitions are in `omnetpp.ini`.

2. **Topology/data preparation (`scripts/preprocess`)**
   - `generate_cbt.py` — generates complete binary tree edge lists
   - `generate_er.py` — generates Erdos-Renyi random graph edge lists
   - `process_twitch.py` — preprocesses the SNAP Twitch dataset (exists but not called by the pipeline; Twitch scenarios use a synthetic ER proxy instead — see #57)

3. **Orchestration (`scripts/run_experiments.sh`, `docker/`)**
   - Canonical execution path is `scripts/run_experiments.sh`.
   - Pipeline order: create result dir → generate topologies → (optional) build sim → run configs → analysis → plotting.
   - Docker compose runs the same script with `--skip-sim-build` and sets `RESULT_DIR=/workspace/results/latest`.
   - Flags: `--quick` (smoke test), `--mwe` (artifact MWE), `--claim-a` / `--claim-b` (paper figure pipelines), `--skip-sim-build` (Docker).

4. **Postprocessing (`scripts/analysis`, `scripts/visualization`)**
   - `.vec` files are parsed recursively into per-scenario aggregates in `analysis.csv`.
   - For top-priority MWE, `scripts/analysis/build_mwe_outputs.py` generates level-sweep artifacts under `<RESULT_DIR>/mwe/`:
     - `analysis.csv` with `topology,corruption_level,avg_beta_pct_increase,avg_payment_pct_increase,user_service_fraction`
     - `metrics_plot.png` (composite 3-panel line figure)
     - `beta_increase_vs_level.png`
     - `payment_increase_vs_level.png`
     - `service_fraction_vs_level.png`
   - Figure-specific scripts (`build_fig2_outputs.py`, `build_fig3_outputs.py`, `build_fig5_outputs.py`) produce paper-ready CSVs and plots.

## Key repository conventions

- Use `uv run python ...` for Python scripts to guarantee the managed environment from `pyproject.toml`/`uv.lock`.
- `scripts/run_experiments.sh` is the source of truth for runnable scenario selection:
  - `--quick` ⇒ `BaselineCBT` only
  - default ⇒ full 3×4 topology × fault matrix (12 configs):
    - baselines: `BaselineCBT`, `BaselineER`, `BaselineTwitch`
    - faults per topology: `FaultDistance*`, `FaultBeta*`, `FaultParent*`
  - `--claim-b` ⇒ algorithm comparison configs (SCM, Garg-Grosu, Byrenheid)
- The top-priority MWE objective is level-sweep parent-manipulation resilience at 1023 nodes across CBT/ER/Twitch:
  - x-axis: SCM corruption level (1..9)
  - y-axes: beta % increase, payment % increase, user service fraction [0,1]
  - one line per topology in each plot
- `omnetpp.ini` sets global `network = SCMNetwork`; active scenario configs override this with dedicated NED networks (`CompleteBinaryTree`, `ErdosRenyi`, `TwitchNetwork`).
- `RESULT_DIR` defaults to `results/<timestamp>`; if unwritable, script falls back to `${XDG_STATE_HOME:-$HOME/.local/state}/scm-overlay-omnet/results/<timestamp>`.
- Runtime outputs belong under top-level `results/`; `docs/results/` is for narrative documentation, not pipeline outputs.
- OMNeT++ is pinned as a Git submodule (`third_party/omnetpp`) for reproducible runs. Use `scripts/check_omnetpp_version.sh` to compare pinned state vs upstream tags.
- `.gitattributes` enforces LF line endings across all platforms.
- Garg-Grosu convergence is detected per-node by comparing beta values across consecutive stabilization rounds. Global convergence = all nodes locally converged. See `docs/superpowers/specs/2026-03-25-garg-grosu-convergence-design.md` for the full design.

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
