#!/usr/bin/env python3
"""Build Claim-1 MWE outputs from per-node simulation state export.

Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd


def load_node_state(mwe_run_dir: Path) -> pd.DataFrame:
    csv_path = mwe_run_dir / "mwe_node_state.csv"
    if not csv_path.exists():
        print(f"ERROR: expected node-state export at {csv_path}", file=sys.stderr)
        sys.exit(1)

    df = pd.read_csv(csv_path)
    required = [
        "node_id",
        "num_users",
        "subtree_size",
        "beta",
        "payment",
        "status",
        "proof_valid",
    ]
    missing = [c for c in required if c not in df.columns]
    if missing:
        print(f"ERROR: missing columns in {csv_path}: {missing}", file=sys.stderr)
        sys.exit(1)
    return df[required].copy()


def export_analysis(df: pd.DataFrame, out_dir: Path) -> None:
    out_csv = out_dir / "analysis.csv"
    df.sort_values("node_id").to_csv(out_csv, index=False)
    print(f"Saved analysis to {out_csv}")


def build_tree_plot(df: pd.DataFrame, out_dir: Path) -> None:
    nodes = sorted(df["node_id"].astype(int).tolist())
    if not nodes:
        print("ERROR: no nodes found in MWE analysis data", file=sys.stderr)
        sys.exit(1)

    pos = {}
    max_level = max((node.bit_length() for node in nodes), default=1) - 1
    level_counts: dict[int, int] = {}
    for node in nodes:
        lvl = (node + 1).bit_length() - 1
        idx = level_counts.get(lvl, 0)
        level_counts[lvl] = idx + 1
        width = 2 ** lvl
        x = (idx + 0.5) / width
        y = 1.0 - (lvl / max(max_level, 1))
        pos[node] = (x, y)

    payment_sum = float(df["payment"].sum())
    total_link_cost = float(max(len(nodes) - 1, 0))
    budget_gap = payment_sum - total_link_cost

    fig, ax = plt.subplots(figsize=(14, 8))
    draw_labels = len(nodes) <= 256
    draw_scatter = len(nodes) <= 4096
    # Draw edges
    for node_id in nodes:
        if node_id == 0:
            continue
        parent = (node_id - 1) // 2
        x1, y1 = pos[parent]
        x2, y2 = pos[node_id]
        ax.plot([x1, x2], [y1, y2], color="#9e9e9e", linewidth=1.0, alpha=0.7, zorder=1)

    # Draw nodes
    if draw_scatter:
        xs = [pos[n][0] for n in nodes]
        ys = [pos[n][1] for n in nodes]
        node_colors = []
        for node_id in nodes:
            row = df.loc[df["node_id"] == node_id].iloc[0]
            node_colors.append("#4caf50" if int(row["proof_valid"]) == 1 else "#ef5350")
        marker_size = 850 if len(nodes) <= 256 else 80
        ax.scatter(xs, ys, s=marker_size, c=node_colors, edgecolors="black", linewidths=0.4, zorder=2)

    # Draw labels
    if draw_labels:
        for _, row in df.sort_values("node_id").iterrows():
            node_id = int(row["node_id"])
            label = (
                f"{node_id}\n"
                f"β={float(row['beta']):.3f}\n"
                f"p={float(row['payment']):.3f}"
            )
            x, y = pos[node_id]
            ax.text(x, y, label, fontsize=7, ha="center", va="center", zorder=3)

    node_count = len(nodes)
    if draw_labels:
        title_suffix = "annotated per-node beta/payment"
    elif draw_scatter:
        title_suffix = "large-scale node map (labels suppressed)"
    else:
        title_suffix = "edge-only topology view"
    ax.set_title(f"SCM MWE Tree ({node_count} nodes): {title_suffix}")
    ax.axis("off")
    fig.text(
        0.02,
        0.02,
        (
            f"Budget check: sum(payments)={payment_sum:.6f}, "
            f"total_link_cost={total_link_cost:.6f}, gap={budget_gap:.6f}"
        ),
        fontsize=10,
    )

    out_png = out_dir / "metrics_plot.png"
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(out_png, dpi=300)
    print(f"Saved plot to {out_png}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("mwe_run_dir", help="Simulation result dir for MWE config")
    parser.add_argument("out_dir", help="Target results/mwe directory")
    args = parser.parse_args()

    run_dir = Path(args.mwe_run_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = load_node_state(run_dir)
    export_analysis(df, out_dir)
    build_tree_plot(df, out_dir)


if __name__ == "__main__":
    main()
