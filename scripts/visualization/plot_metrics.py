#!/usr/bin/env python3
"""Generate parent-attack topology line plots from per-scenario node-state exports."""
import pandas as pd
import matplotlib
matplotlib.use("Agg")  # Non-interactive backend (works headless / SSH)
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
import sys


def _load_avg_beta(result_dir: Path, scenario: str) -> float:
    csv_path = result_dir / scenario / "mwe_node_state.csv"
    if not csv_path.exists():
        raise FileNotFoundError(f"Missing node-state export: {csv_path}")
    df = pd.read_csv(csv_path)
    if "beta" not in df.columns:
        raise ValueError(f"Missing beta column in {csv_path}")
    if df.empty:
        raise ValueError(f"Empty node-state export: {csv_path}")
    return float(df["beta"].astype(float).mean())


def _build_parent_attack_df(result_dir: Path) -> pd.DataFrame:
    topology_pairs = [
        ("CBT", "BaselineCBT", "FaultParentCBT"),
        ("ER", "BaselineER", "FaultParentER"),
        ("Twitch", "BaselineTwitch", "FaultParent"),
    ]
    rows = []
    missing = []
    for topology, baseline_cfg, attack_cfg in topology_pairs:
        try:
            baseline_beta = _load_avg_beta(result_dir, baseline_cfg)
            attack_beta = _load_avg_beta(result_dir, attack_cfg)
        except (FileNotFoundError, ValueError):
            missing.append(topology)
            continue

        pct = 0.0 if abs(baseline_beta) <= 1e-12 else ((attack_beta - baseline_beta) / abs(baseline_beta)) * 100.0
        rows.append(
            {
                "topology": topology,
                "baseline_beta": baseline_beta,
                "parent_attack_beta": attack_beta,
                "beta_pct_increase": pct,
            }
        )

    if not rows:
        print(
            "WARN: No complete baseline/parent-attack topology pairs found. "
            "Expected BaselineCBT/FaultParentCBT, BaselineER/FaultParentER, BaselineTwitch/FaultParent.",
            file=sys.stderr,
        )
        return pd.DataFrame(
            columns=["topology", "baseline_beta", "parent_attack_beta", "beta_pct_increase"]
        )

    if missing:
        print(f"WARN: Missing complete parent-attack pairs for: {', '.join(missing)}", file=sys.stderr)

    return pd.DataFrame(rows)


def _plot_parent_attack_lines(df: pd.DataFrame, out_path: Path) -> None:
    if df.empty:
        plt.figure(figsize=(8.5, 6))
        plt.title("Average Beta: Baseline vs Parent Attack by Topology")
        plt.text(
            0.5,
            0.5,
            "No complete baseline/parent-attack topology pairs found",
            ha="center",
            va="center",
            transform=plt.gca().transAxes,
        )
        plt.axis("off")
        plt.tight_layout()
        plt.savefig(out_path, dpi=300)
        return

    long_df = pd.concat(
        [
            df[["topology", "baseline_beta"]].rename(columns={"baseline_beta": "avg_beta"}).assign(stage="Baseline"),
            df[["topology", "parent_attack_beta"]].rename(columns={"parent_attack_beta": "avg_beta"}).assign(stage="Parent attack"),
        ],
        ignore_index=True,
    )
    long_df["stage"] = pd.Categorical(long_df["stage"], categories=["Baseline", "Parent attack"], ordered=True)
    long_df = long_df.sort_values(["topology", "stage"])

    plt.figure(figsize=(8.5, 6))
    sns.lineplot(data=long_df, x="stage", y="avg_beta", hue="topology", marker="o", linewidth=2.2)
    plt.title("Average Beta: Baseline vs Parent Attack by Topology")
    plt.xlabel("Scenario")
    plt.ylabel("Average beta")
    plt.grid(alpha=0.25)

    pct_lookup = {r["topology"]: r["beta_pct_increase"] for _, r in df.iterrows()}
    leg = plt.legend(title="Topology")
    for text in leg.get_texts():
        topo = text.get_text()
        if topo in pct_lookup:
            text.set_text(f"{topo} ({pct_lookup[topo]:+.2f}%)")

    plt.tight_layout()
    plt.savefig(out_path, dpi=300)


def plot_metrics(result_dir):
    result_path = Path(result_dir)
    df = _build_parent_attack_df(result_path)

    summary_out = result_path / "parent_attack_beta_summary.csv"
    df.round(6).to_csv(summary_out, index=False, float_format="%.6f")

    out_path = result_path / "metrics_plot.png"
    _plot_parent_attack_lines(df, out_path)
    print(f"Saved plot to {out_path}")
    print(f"Saved parent-attack summary to {summary_out}")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("result_dir", help="Directory containing analysis.csv")
    args = parser.parse_args()
    plot_metrics(args.result_dir)
