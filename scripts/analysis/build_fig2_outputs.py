#!/usr/bin/env python3
"""Build strict Outcome-1 plot: avg beta % increase vs depth (CBT/ER/Twitch)."""
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
    required = {"beta"}
    missing = required.difference(df.columns)
    if missing:
        raise ValueError(f"Missing required columns in {csv_path}: {sorted(missing)}")
    return df


def baseline_beta_mean(topo_dir: Path) -> float:
    baseline = load_node_state(topo_dir / "baseline")
    return float(pd.to_numeric(baseline["beta"], errors="coerce").dropna().mean())


def depth_run_beta_mean(topo_dir: Path, depth: int) -> float | None:
    run_dir = topo_dir / f"depth_{depth}" / "run_0"
    csv_path = run_dir / "mwe_node_state.csv"
    if not csv_path.exists():
        return None
    df = load_node_state(run_dir)
    return float(pd.to_numeric(df["beta"], errors="coerce").dropna().mean())


def discover_depths(topo_dir: Path) -> set[int]:
    depths: set[int] = set()
    for depth_dir in topo_dir.glob("depth_*"):
        if not depth_dir.is_dir():
            continue
        try:
            depth = int(depth_dir.name.split("_", 1)[1])
        except Exception:
            continue
        if (depth_dir / "run_0" / "mwe_node_state.csv").exists():
            depths.add(depth)
    return depths


def build_analysis(result_root: Path) -> pd.DataFrame:
    topologies = ("CBT", "ER", "Twitch")
    topo_dirs = {topo: result_root / topo for topo in topologies}
    for topo, path in topo_dirs.items():
        if not path.is_dir():
            raise FileNotFoundError(f"Missing topology directory for {topo}: {path}")

    depth_sets = {topo: discover_depths(path) for topo, path in topo_dirs.items()}
    common_depths = set.intersection(*depth_sets.values()) if depth_sets else set()
    if not common_depths:
        raise ValueError("No common depths across CBT/ER/Twitch run outputs")

    dropped = {topo: sorted(depth_sets[topo] - common_depths) for topo in topologies}
    for topo, vals in dropped.items():
        if vals:
            print(f"WARN: dropped non-common depths for {topo}: {vals}", file=sys.stderr)

    ordered_common = sorted(common_depths)
    relabel = {d: i + 1 for i, d in enumerate(ordered_common)}

    rows: list[dict[str, float | int | str]] = []
    for topo in topologies:
        base_mean = baseline_beta_mean(topo_dirs[topo])
        for depth in ordered_common:
            attack_mean = depth_run_beta_mean(topo_dirs[topo], depth)
            if attack_mean is None:
                print(f"WARN: missing run for {topo} depth {depth}; skipping", file=sys.stderr)
                continue
            if abs(base_mean) <= 1e-12:
                raw_beta_pct = 0.0
            else:
                raw_beta_pct = ((attack_mean - base_mean) / abs(base_mean)) * 100.0
            beta_pct = max(0.0, raw_beta_pct)
            rows.append(
                {
                    "topology": topo,
                    "depth_original": depth,
                    "depth_plot": relabel[depth],
                    "baseline_beta_mean": base_mean,
                    "attack_beta_mean": attack_mean,
                    "raw_beta_pct_change": raw_beta_pct,
                    "avg_beta_pct_increase": beta_pct,
                }
            )

    if not rows:
        raise ValueError("No rows produced for Outcome-1 analysis")

    return pd.DataFrame(rows).sort_values(["depth_plot", "topology"]).reset_index(drop=True)


def plot_outcome1(df: pd.DataFrame, out_path: Path) -> None:
    plt.figure(figsize=(8.5, 6))
    sns.set_theme(style="whitegrid")
    sns.lineplot(
        data=df,
        x="depth_plot",
        y="avg_beta_pct_increase",
        hue="topology",
        marker="o",
        linewidth=2.2,
    )
    xticks = sorted(df["depth_plot"].unique().tolist())
    labels = (
        df[["depth_plot", "depth_original"]]
        .drop_duplicates()
        .sort_values("depth_plot")
    )
    plt.xticks(xticks, labels["depth_original"].astype(int).astype(str).tolist())
    plt.xlabel("Tree Depth")
    plt.ylabel("Avg. β % Increase")
    plt.title("Avg. β % Increase vs. Tree Depth")
    plt.grid(alpha=0.25)
    plt.tight_layout()
    plt.savefig(out_path, dpi=300)
    plt.close()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_dir", help="Directory for fig2 outputs")
    parser.add_argument("--result-root", required=True, help="Root containing CBT/, ER/, Twitch/ topology dirs")
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = build_analysis(Path(args.result_root))
    out_csv = out_dir / "analysis.csv"
    out_png = out_dir / "beta_increase_vs_depth.png"
    out_legacy_png = out_dir / "metrics_plot.png"
    df.to_csv(out_csv, index=False, float_format="%.6f")
    plot_outcome1(df, out_png)
    plot_outcome1(df, out_legacy_png)
    print(f"Saved analysis to {out_csv}")
    print(f"Saved plot to {out_png}")
    print(f"Saved plot to {out_legacy_png}")


if __name__ == "__main__":
    main()
