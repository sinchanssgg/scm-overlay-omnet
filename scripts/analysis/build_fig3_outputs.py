#!/usr/bin/env python3
"""Build Figure-3 proof-size outputs from real simulation state exports."""
from __future__ import annotations

import argparse
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
    required = {"node_id", "level", "subtree_size", "beta", "payment", "proof_size_bytes", "proof_valid"}
    missing = required.difference(df.columns)
    if missing:
        raise ValueError(f"Missing required columns in {csv_path}: {sorted(missing)}")
    return df


def measured_scm_proof_size(df: pd.DataFrame) -> float:
    measured = df["proof_size_bytes"].astype(float)
    valid = measured[measured > 0]
    if valid.empty:
        raise ValueError("No non-zero proof_size_bytes found in node-state export")
    return float(valid.mean())


def estimate_garg_grosu_size(df: pd.DataFrame) -> float:
    depths = df["level"].astype(int).clip(lower=1)
    per_hop_sig = 72.0
    metadata = 64.0
    return float((depths * per_hop_sig + metadata).mean())


def build_analysis(cbt_dir: Path, twitch_dir: Path, twitch_nodes: int) -> pd.DataFrame:
    cbt = load_node_state(cbt_dir)
    twitch = load_node_state(twitch_dir)
    twitch = twitch.sort_values("node_id").head(twitch_nodes)

    rows = [
        {
            "topology": "CBT",
            "num_nodes": int(len(cbt)),
            "method": "SCM",
            "avg_proof_size_bytes": measured_scm_proof_size(cbt),
        },
        {
            "topology": "CBT",
            "num_nodes": int(len(cbt)),
            "method": "Garg-Grosu",
            "avg_proof_size_bytes": estimate_garg_grosu_size(cbt),
        },
        {
            "topology": "Twitch",
            "num_nodes": int(len(twitch)),
            "method": "SCM",
            "avg_proof_size_bytes": measured_scm_proof_size(twitch),
        },
        {
            "topology": "Twitch",
            "num_nodes": int(len(twitch)),
            "method": "Garg-Grosu",
            "avg_proof_size_bytes": estimate_garg_grosu_size(twitch),
        },
    ]
    df = pd.DataFrame(rows)
    df["avg_proof_size_bytes"] = df["avg_proof_size_bytes"].round(6)
    return df


def plot_fig3(df: pd.DataFrame, out_path: Path) -> None:
    plt.figure(figsize=(8, 6))
    sns.barplot(data=df, x="topology", y="avg_proof_size_bytes", hue="method")
    plt.title("Average Proof Size: SCM vs Garg-Grosu")
    plt.ylabel("Average proof size (bytes)")
    plt.xlabel("Topology")
    plt.grid(axis="y", alpha=0.25)
    plt.tight_layout()
    plt.savefig(out_path, dpi=300)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_dir", help="Directory for fig3 outputs")
    parser.add_argument("--cbt-state-dir", required=True, help="Dir containing cbt mwe_node_state.csv")
    parser.add_argument("--twitch-state-dir", required=True, help="Dir containing twitch mwe_node_state.csv")
    parser.add_argument("--twitch-nodes", type=int, default=1023)
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = build_analysis(Path(args.cbt_state_dir), Path(args.twitch_state_dir), args.twitch_nodes)
    out_csv = out_dir / "analysis.csv"
    out_png = out_dir / "metrics_plot.png"
    df.to_csv(out_csv, index=False, float_format="%.6f")
    plot_fig3(df, out_png)

    print(f"Saved analysis to {out_csv}")
    print(f"Saved plot to {out_png}")


if __name__ == "__main__":
    main()
