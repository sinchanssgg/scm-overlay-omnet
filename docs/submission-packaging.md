# Artifact Submission Packaging (Issue #21)

This document tracks what to package for DSN artifact submission and how to prepare archival metadata.

## 1) Required submission channel

For DSN 2026 artifact track submission, archive the artifact on:
- Zenodo, or
- Figshare

Use a persistent identifier (DOI) from the archival platform.

## 2) Package contents checklist

- Source snapshot (tagged release)
- `README.md` with execution instructions
- `docs/setup.md`
- `docs/design.md`
- `docs/current-state.md`
- `docs/artifact-evaluation.md`
- Analysis/plot scripts under `scripts/analysis`
- Reproducibility controls (`SCM_RANDOM_SEED`, validation script)
- License (`LICENSE`)

## 3) Claim-to-command mapping

- Claim 1 (MWE): `./scripts/run_experiments.sh --mwe`
- Topology x fault matrix (all 3 faults on each topology): `./scripts/run_experiments.sh`
- Figure 2: `uv run python scripts/analysis/build_fig2_outputs.py ...`
- Figure 3: `uv run python scripts/analysis/build_fig3_outputs.py ...`
- Figure 5: `uv run python scripts/analysis/build_fig5_outputs.py ...`

Each path must produce:
- `analysis.csv`
- `metrics_plot.png`
- For MWE path: `beta_increase_vs_level.png`, `payment_increase_vs_level.png`, `service_fraction_vs_level.png`

## 4) Metadata checklist for DOI deposit

- Title and version aligned with release tag
- Author list
- Repository URL
- License (MIT)
- Short artifact abstract
- Reproduction steps (kick-the-tires + figure commands)

## 5) Pre-deposit dry-run

Before archive upload:

```bash
git submodule update --init --recursive
uv sync
RESULT_DIR=/tmp/scm-quick SCM_RANDOM_SEED=1337 ./scripts/run_experiments.sh --skip-sim-build
uv run python scripts/analysis/validate_outputs.py /tmp/scm-quick --require-columns scenario,value_mean,value_std,value_count,time_max --require-non-empty
```

And run figure scripts:

```bash
uv run python scripts/analysis/build_fig2_outputs.py /tmp/scm-fig2 --result-root /tmp/scm-quick
uv run python scripts/analysis/build_fig3_outputs.py /tmp/scm-fig3 \
  --cbt-state-dir /tmp/scm-quick/BaselineCBT \
  --twitch-state-dir /tmp/scm-quick/BaselineTwitch \
  --twitch-nodes 50
uv run python scripts/analysis/build_fig5_outputs.py /tmp/scm-fig5 --result-root /tmp/scm-quick
```

## 6) Release checklist

- [ ] Merge required PRs to `main`
- [ ] Tag release used for deposit
- [ ] Create Zenodo/Figshare artifact and mint DOI
- [ ] Verify DOI landing page contains required metadata
- [ ] Update repo docs/issue tracker with DOI link
