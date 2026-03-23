# Results Documentation

This directory is reserved for documentation artifacts related to experimental outputs.

## Runtime output locations

The execution pipeline writes raw artifacts under top-level `results/`:

- `results/<timestamp>/<ConfigName>/...` (OMNeT++ run outputs such as `.vec`/`.sca`)
- `results/<timestamp>/analysis.csv`
- `results/<timestamp>/metrics_plot.png`

When `RESULT_DIR` is set, the same structure is created under that directory.

## `analysis.csv` schema

`scripts/analysis/process_results.py` currently writes one row per scenario with:

- `scenario`: Scenario name inferred from result subdirectory
- `value_mean`: Mean across parsed vector values
- `value_std`: Standard deviation across parsed vector values
- `value_count`: Number of parsed vector rows
- `time_max`: Maximum simulation time observed in parsed vectors

## `metrics_plot.png`

`scripts/visualization/plot_metrics.py` renders two bar charts:

- Maximum stabilization time per scenario (`time_max`)
- Standard deviation of aggregated vector values per scenario (`value_std`, currently stabilization-time values from `nodeStableTime`)

## Suggested usage

Raw outputs should continue to live under top-level `results/` (runtime generated), while this folder is for narrative documentation.

Useful artifacts to store here:
- Methodology notes for experiment batches
- Metric interpretation notes
- Versioned summaries of major runs
- Figure captions and reproducibility notes
