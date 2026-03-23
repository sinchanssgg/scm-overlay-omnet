#!/usr/bin/env python3
"""Build Figure-2 outputs from simulation node-state exports."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


def load_node_state(path: Path) -> pd.DataFrame:
    csv_path = path / "mwe_node_state.csv"
    if not csv_path.exists():
        raise FileNotFoundError(f"Expected node state export: {csv_path}")
    df = pd.read_csv(csv_path)
    required = {"node_id", "level", "beta", "payment", "status", "proof_valid"}
    missing = required.difference(df.columns)
    if missing:
        raise ValueError(f"Missing required columns in {csv_path}: {sorted(missing)}")
    return df


def percent_increase_series(baseline: pd.Series, fault: pd.Series) -> pd.Series:
    eps = 1e-9
    mask = baseline.abs() > eps
    if not mask.any():
        return pd.Series([0.0])
    return ((fault[mask] - baseline[mask]) / baseline[mask].abs()) * 100.0


def build_topology_row(topology: str, baseline_dir: Path, fault_dir: Path) -> dict[str, float]:
    base = load_node_state(baseline_dir).set_index("node_id")
    fault = load_node_state(fault_dir).set_index("node_id")

    joined = base.join(
        fault[["beta", "payment", "status", "proof_valid"]],
        how="inner",
        lsuffix="_base",
        rsuffix="_fault",
    )
    if joined.empty:
        raise ValueError(f"No overlapping node_ids between {baseline_dir} and {fault_dir}")

    beta_delta = percent_increase_series(joined["beta_base"], joined["beta_fault"])
    payment_delta = percent_increase_series(joined["payment_base"], joined["payment_fault"])
    service_fraction = (
        (joined["status_fault"] == "STABLE") & (joined["proof_valid_fault"].astype(int) == 1)
    ).mean()
    depth = int(base["level"].max())
    return {
        "topology": topology,
        "depth": depth,
        "avg_beta_pct_increase": float(beta_delta.mean()),
        "avg_payment_pct_increase": float(payment_delta.mean()),
        "user_fraction_receiving_service": float(service_fraction),
    }


def build_analysis(result_root: Path) -> pd.DataFrame:
    rows = []
    pair_specs = [
        ("CBT", result_root / "BaselineCBT", result_root / "FaultDistance"),
        ("ER", result_root / "BaselineER", result_root / "FaultBeta"),
    ]
    for topology, baseline_dir, fault_dir in pair_specs:
        if not (baseline_dir / "mwe_node_state.csv").exists() or not (fault_dir / "mwe_node_state.csv").exists():
            print(
                f"Skipping {topology}: missing state export in {baseline_dir} or {fault_dir}",
                file=sys.stderr,
            )
            continue
        rows.append(build_topology_row(topology, baseline_dir, fault_dir))

    if not rows:
        raise ValueError(
            "No Figure-2 scenario pairs found. Expected mwe_node_state.csv under "
            "BaselineCBT/FaultDistance and/or BaselineER/FaultBeta."
        )

    out = pd.DataFrame(rows)
    for col in ("avg_beta_pct_increase", "avg_payment_pct_increase", "user_fraction_receiving_service"):
        out[col] = out[col].round(6)
    return out


def plot_fig2(df: pd.DataFrame, out_path: Path) -> None:
    fig, axes = plt.subplots(1, 3, figsize=(16, 5))
    sns.lineplot(data=df, x="depth", y="avg_beta_pct_increase", hue="topology", marker="o", ax=axes[0])
    axes[0].set_title("Beta increase vs depth")
    axes[0].set_ylabel("Average % increase")

    sns.lineplot(data=df, x="depth", y="avg_payment_pct_increase", hue="topology", marker="o", ax=axes[1], legend=False)
    axes[1].set_title("Payment increase vs depth")
    axes[1].set_ylabel("Average % increase")

    sns.lineplot(data=df, x="depth", y="user_fraction_receiving_service", hue="topology", marker="o", ax=axes[2], legend=False)
    axes[2].set_title("Users receiving service vs depth")
    axes[2].set_ylabel("Fraction")
    axes[2].set_ylim(0, 1.05)

    for ax in axes:
        ax.set_xlabel("Depth")
        ax.grid(alpha=0.2)
    fig.tight_layout()
    fig.savefig(out_path, dpi=300)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_dir", help="Directory for fig2 outputs")
    parser.add_argument("--result-root", required=True, help="Result root containing scenario .vec files")
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = build_analysis(Path(args.result_root))
    out_csv = out_dir / "analysis.csv"
    out_png = out_dir / "metrics_plot.png"
    df.to_csv(out_csv, index=False, float_format="%.6f")
    plot_fig2(df, out_png)
    print(f"Saved analysis to {out_csv}")
    print(f"Saved plot to {out_png}")


if __name__ == "__main__":
    main()
