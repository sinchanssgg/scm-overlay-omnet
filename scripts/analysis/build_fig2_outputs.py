#!/usr/bin/env python3
"""Build Figure-2 per-depth outputs from simulation node-state exports.

Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
Modified By: Arannya Mukherjee <arannya@adhrith.ai>
"""
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


def analyze_depth_rep(baseline: pd.DataFrame, fault: pd.DataFrame, depth: int) -> dict | None:
    """Compare the corrupted node at a specific depth between baseline and fault runs."""
    base_at_depth = baseline[baseline["level"] == depth]
    fault_at_depth = fault[fault["level"] == depth]

    if base_at_depth.empty or fault_at_depth.empty:
        return None

    # Join on node_id to find matching nodes
    joined = base_at_depth.set_index("node_id").join(
        fault_at_depth.set_index("node_id")[["beta", "payment", "status", "proof_valid"]],
        how="inner",
        lsuffix="_base",
        rsuffix="_fault",
    )
    if joined.empty:
        return None

    # The corrupted node is the one with the largest absolute beta change
    joined["beta_delta"] = (joined["beta_fault"] - joined["beta_base"]).abs()
    corrupted = joined.loc[joined["beta_delta"].idxmax()]

    beta_base = float(corrupted["beta_base"])
    beta_fault = float(corrupted["beta_fault"])
    payment_base = float(corrupted["payment_base"])
    payment_fault = float(corrupted["payment_fault"])

    eps = 1e-9
    beta_pct = 0.0 if abs(beta_base) <= eps else ((beta_fault - beta_base) / abs(beta_base)) * 100.0
    payment_pct = 0.0 if abs(payment_base) <= eps else ((payment_fault - payment_base) / abs(payment_base)) * 100.0
    receiving_service = int(corrupted["status_fault"] == "STABLE" and int(corrupted["proof_valid_fault"]) == 1)

    return {
        "beta_pct_increase": beta_pct,
        "payment_pct_increase": payment_pct,
        "receiving_service": receiving_service,
    }


def build_topology_rows(topology: str, topo_dir: Path) -> list[dict]:
    """Build per-depth rows for one topology."""
    baseline_dir = topo_dir / "baseline"
    if not (baseline_dir / "mwe_node_state.csv").exists():
        print(f"WARN: No baseline for {topology} at {baseline_dir}", file=sys.stderr)
        return []

    baseline = load_node_state(baseline_dir)
    rows = []

    # Find all depth_N directories
    depth_dirs = sorted(
        [d for d in topo_dir.iterdir() if d.is_dir() and d.name.startswith("depth_")],
        key=lambda d: int(d.name.split("_")[1]),
    )

    for depth_dir in depth_dirs:
        depth = int(depth_dir.name.split("_")[1])
        rep_metrics = []

        # Find all rep_N directories
        rep_dirs = sorted(
            [d for d in depth_dir.iterdir() if d.is_dir() and d.name.startswith("rep_")],
            key=lambda d: int(d.name.split("_")[1]),
        )

        for rep_dir in rep_dirs:
            if not (rep_dir / "mwe_node_state.csv").exists():
                continue
            try:
                fault = load_node_state(rep_dir)
                result = analyze_depth_rep(baseline, fault, depth)
                if result:
                    rep_metrics.append(result)
            except Exception as e:
                print(f"WARN: {topology} depth {depth} {rep_dir.name}: {e}", file=sys.stderr)

        if not rep_metrics:
            print(f"WARN: No valid reps for {topology} depth {depth}", file=sys.stderr)
            continue

        avg_beta = sum(m["beta_pct_increase"] for m in rep_metrics) / len(rep_metrics)
        avg_payment = sum(m["payment_pct_increase"] for m in rep_metrics) / len(rep_metrics)
        avg_service = sum(m["receiving_service"] for m in rep_metrics) / len(rep_metrics)

        rows.append({
            "topology": topology,
            "depth": depth,
            "avg_beta_pct_increase": round(avg_beta, 6),
            "avg_payment_pct_increase": round(avg_payment, 6),
            "user_fraction_receiving_service": round(avg_service, 6),
        })

    return rows


def build_analysis(result_root: Path) -> pd.DataFrame:
    rows = []
    for topo in ("CBT", "ER", "Twitch"):
        topo_dir = result_root / topo
        if not topo_dir.is_dir():
            print(f"Skipping {topo}: directory not found at {topo_dir}", file=sys.stderr)
            continue
        rows.extend(build_topology_rows(topo, topo_dir))

    if not rows:
        raise ValueError(
            "No Figure-2 data found. Expected directories: CBT/, ER/, Twitch/ "
            "each containing baseline/ and depth_N/rep_N/ subdirectories."
        )

    return pd.DataFrame(rows)


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
    parser.add_argument("--result-root", required=True, help="Root containing CBT/, ER/, Twitch/ topology dirs")
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
