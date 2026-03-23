#!/usr/bin/env python3
"""Generate summary bar charts from analysis.csv."""
import pandas as pd
import matplotlib
matplotlib.use("Agg")  # Non-interactive backend (works headless / SSH)
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
import sys


def plot_metrics(result_dir):
    csv_path = Path(result_dir) / "analysis.csv"

    if not csv_path.exists():
        print(f"ERROR: {csv_path} not found. Run process_results.py first.", file=sys.stderr)
        sys.exit(1)

    df = pd.read_csv(csv_path)

    if df.empty:
        print("ERROR: analysis.csv is empty", file=sys.stderr)
        sys.exit(1)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6))

    # Plot 1: Maximum stabilization time per scenario
    sns.barplot(data=df, x="scenario", y="time_max", ax=ax1)
    ax1.set_title("Maximum Stabilization Time")
    ax1.set_ylabel("Seconds")
    ax1.tick_params(axis="x", rotation=45)

    # Plot 2: Beta value standard deviation per scenario
    sns.barplot(data=df, x="scenario", y="value_std", ax=ax2)
    ax2.set_title("Beta Value Standard Deviation")
    ax2.set_ylabel("Std Dev")
    ax2.tick_params(axis="x", rotation=45)

    plt.tight_layout()

    out_path = Path(result_dir) / "metrics_plot.png"
    plt.savefig(out_path, dpi=300)
    print(f"Saved plot to {out_path}")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("result_dir", help="Directory containing analysis.csv")
    args = parser.parse_args()
    plot_metrics(args.result_dir)
