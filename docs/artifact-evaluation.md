# Artifact Evaluation Playbook (DSN 2026)

This guide is for AE reviewers and maintainers validating artifact readiness.

## 1) Kick-the-tires checklist

Run from repository root.

```bash
git submodule update --init --recursive
uv sync
```

If using native OMNeT++:

```bash
cd third_party/omnetpp
source setenv
./configure
make -j$(nproc)
cd ../..
```

## 2) Minimum working example

```bash
RESULT_DIR=/tmp/scm-mwe ./scripts/run_experiments.sh --mwe
```

Expected:
- `/tmp/scm-mwe/mwe/analysis.csv`
- `/tmp/scm-mwe/mwe/metrics_plot.png`
- `/tmp/scm-mwe/mwe/beta_increase_vs_level.png`
- `/tmp/scm-mwe/mwe/payment_increase_vs_level.png`
- `/tmp/scm-mwe/mwe/service_fraction_vs_level.png`

`analysis.csv` columns:
- `topology,corruption_level,avg_beta_pct_increase,avg_payment_pct_increase,user_service_fraction`

## 3) Figure reproduction commands

Figure 2:

```bash
uv run python scripts/analysis/build_fig2_outputs.py /tmp/scm-fig2 --result-root /tmp/scm-quick
```

Figure 3:

```bash
uv run python scripts/analysis/build_fig3_outputs.py /tmp/scm-fig3 \
  --cbt-state-dir /tmp/scm-quick/BaselineCBT \
  --twitch-state-dir /tmp/scm-quick/BaselineTwitch \
  --twitch-nodes 50
```

Figure 5:

```bash
uv run python scripts/analysis/build_fig5_outputs.py /tmp/scm-fig5 --result-root /tmp/scm-quick
```

Expected for each output directory:
- `analysis.csv`
- `metrics_plot.png`

Top-priority MWE plotting objective (`/tmp/scm-mwe/mwe/`):
- three separate line plots, each with one curve per topology (CBT, ER, Twitch)
- x-axis: SCM corruption level (1..9)
- y-axis metrics:
  - avg beta % increase
  - avg payment % increase
  - user service fraction [0,1]

## 4) Output contract checks

Quick pipeline output validation:

```bash
RESULT_DIR=/tmp/scm-quick SCM_RANDOM_SEED=1337 ./scripts/run_experiments.sh --skip-sim-build
uv run python scripts/analysis/validate_outputs.py /tmp/scm-quick --require-columns scenario,value_mean,value_std,value_count,time_max --require-non-empty
```

This default run now executes full topology x fault coverage (CBT/ER/Twitch with all three fault types), plus each topology baseline.

## 5) Environment/resource guidance

- OS: Linux (validated on Ubuntu-based environments)
- Python: managed by `uv` from `pyproject.toml` / `uv.lock`
- CPU/RAM: 4+ cores and 8GB RAM recommended for figure scripts; higher is better for larger simulation experiments
- Headless runs are supported (`matplotlib` uses non-interactive backend)

## 6) Common troubleshooting

- `uv: command not found`  
  Install uv, then run `uv sync`.

- `OMNeT++ not found`  
  Initialize submodule and ensure `third_party/omnetpp/setenv` exists.

- Missing expected result files  
  Re-run command with a clean `RESULT_DIR` and check command exit code.

- Validation command fails due to missing script  
  Ensure you are on the intended branch/PR head before running branch-specific validation.
