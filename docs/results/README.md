# Results Documentation

This directory is reserved for documentation artifacts related to experimental outputs.

## Runtime output locations

The execution pipeline writes raw artifacts under `$RESULT_DIR`.

- Local default: `RESULT_DIR=results/<timestamp>`
- Docker compose in this repo: `RESULT_DIR=/workspace/results/latest` in-container (mapped to `results/latest` on the host)
- If the default `results/` location is not writable, the run script falls back to `${XDG_STATE_HOME:-$HOME/.local/state}/scm-overlay-omnet/results/<timestamp>`

Expected layout under `$RESULT_DIR`:
- `$RESULT_DIR/<ConfigName>/...` (OMNeT++ run outputs such as `.vec`/`.sca`)
- `$RESULT_DIR/analysis.csv`
- `$RESULT_DIR/metrics_plot.png`
- For MWE path (`$RESULT_DIR/mwe/`):
  - `analysis.csv`
  - `metrics_plot.png`
  - `beta_increase_vs_level.png`
  - `payment_increase_vs_level.png`
  - `service_fraction_vs_level.png`

## `analysis.csv` schema

`scripts/analysis/process_results.py` (default non-MWE path) writes one row per scenario with:

- `scenario`: Scenario name inferred from result subdirectory
- `value_mean`: Mean across parsed vector values
- `value_std`: Standard deviation across parsed vector values
- `value_count`: Number of parsed vector rows
- `time_max`: Maximum simulation time observed in parsed vectors

For MWE, `scripts/analysis/build_mwe_outputs.py` writes:

- `topology`
- `corruption_level`
- `avg_beta_pct_increase`
- `avg_payment_pct_increase`
- `user_service_fraction`

## `metrics_plot.png`

`scripts/visualization/plot_metrics.py` renders default non-MWE summary plots.

For MWE, `scripts/analysis/build_mwe_outputs.py` renders the required level-sweep line plots for:

- Beta % increase vs corruption level
- Payment % increase vs corruption level
- User service fraction vs corruption level

## Suggested usage

Raw outputs should continue to live in the runtime result directory (`$RESULT_DIR`), while this folder is for narrative documentation.

Useful artifacts to store here:
- Methodology notes for experiment batches
- Metric interpretation notes
- Versioned summaries of major runs
- Figure captions and reproducibility notes
