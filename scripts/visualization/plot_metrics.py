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

    def render_metric(ax, column, title, ylabel):
        sns.barplot(data=df, x="scenario", y=column, ax=ax)
        ax.set_title(title)
        ax.set_ylabel(ylabel)
        ax.tick_params(axis="x", rotation=45)
        ax.axhline(0.0, color="black", linewidth=0.8, alpha=0.5)

        series = df[column].fillna(0.0)
        if (series.abs() <= 1e-12).all():
            ax.set_ylim(0.0, 1.0)
            ax.text(
                0.5,
                0.5,
                "All values are 0.0",
                ha="center",
                va="center",
                transform=ax.transAxes,
                fontsize=10,
                color="dimgray",
            )

    # Plot 1: Maximum stabilization time per scenario
    render_metric(ax1, "time_max", "Maximum Stabilization Time", "Seconds")

    # Plot 2: Beta value standard deviation per scenario
    render_metric(ax2, "value_std", "Beta Value Standard Deviation", "Std Dev")

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
