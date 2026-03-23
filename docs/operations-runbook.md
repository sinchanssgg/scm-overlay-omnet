# Operations Runbook (Resource Sizing + Headless-first)

This runbook provides practical guidance for safe execution sizing and headless operation.

## 1) Scenario profiles

### Small profile (first success / CI-like)
- Command:
  - `RESULT_DIR=/tmp/scm-small SCM_RANDOM_SEED=1337 ./scripts/run_experiments.sh --quick`
- Intended scope:
  - compile + one baseline scenario + analysis + plotting
- Recommended machine:
  - 2+ CPU cores, 4GB RAM
- Expected behavior:
  - fastest validation path for environment sanity

### Medium profile (local engineering validation)
- Command:
  - `RESULT_DIR=/tmp/scm-medium SCM_RANDOM_SEED=1337 ./scripts/run_experiments.sh`
- Intended scope:
  - default scenario matrix (`BaselineCBT`, `FaultDistance`, `BaselineER`, `FaultBeta`)
- Recommended machine:
  - 4+ CPU cores, 8GB RAM

### Large profile (artifact-style focused runs)
- MWE large:
  - `RESULT_DIR=/tmp/scm-mwe MWE_NUM_NODES=4096 ./scripts/run_experiments.sh --mwe`
- Figure generation:
  - `uv run python scripts/analysis/build_fig2_outputs.py /tmp/scm-fig2 --num-nodes 1023 --max-depth 10 --seed 1337`
  - `uv run python scripts/analysis/build_fig3_outputs.py /tmp/scm-fig3 --cbt-nodes 16383 --twitch-nodes 1023 --seed 1337`
  - `uv run python scripts/analysis/build_fig5_outputs.py /tmp/scm-fig5 --max-depth 10 --seed 1337`
- Recommended machine:
  - 8+ CPU cores, 16GB RAM

## 2) Headless-first operating mode (recommended)

Use command-line mode for SSH/CI and reproducibility:
- `./scm-simulations -u Cmdenv ...`
- Pipeline script already uses Cmdenv and non-interactive plotting backend.

Recommended baseline:

```bash
RESULT_DIR=/tmp/scm-headless SCM_RANDOM_SEED=1337 ./scripts/run_experiments.sh --quick
```

## 3) Optional GUI path

GUI is optional and not required for artifact checks. Prefer headless unless visual debugging is explicitly needed.

## 4) Default-safe bring-up sequence

1. `git submodule update --init --recursive`
2. `uv sync`
3. Build OMNeT++ if needed
4. `./scripts/run_experiments.sh --quick`
5. Validate outputs:
   - `uv run python scripts/analysis/validate_outputs.py <RESULT_DIR> --require-non-empty`

## 5) Troubleshooting matrix

| Symptom | Likely cause | Action |
|---|---|---|
| `uv: command not found` | uv not installed | Install uv, then run `uv sync` |
| `OMNeT++ not found` | Submodule/environment not initialized | Run submodule init and ensure `third_party/omnetpp/setenv` exists |
| `.vec` not found in result dir | Scenario output path mismatch or run failure | Re-run with clean `RESULT_DIR`, verify non-zero simulation output |
| Plot appears blank in quick run | Metric values may be all zero in smoke scenario | Confirm `analysis.csv`; quick-run plots include explicit zero annotations |
| OOM / very slow run | Profile too large for machine | Use small/medium profile or lower node counts first |

## 6) Runtime safety notes

- Prefer explicit `RESULT_DIR` to avoid mixing runs.
- Keep `SCM_RANDOM_SEED` fixed for reproducibility when comparing outputs.
- Start with small profile after environment changes before running large profiles.
