#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path

def plot_metrics(result_dir):
    df = pd.read_csv(Path(result_dir) / "analysis.csv")
    
    plt.figure(figsize=(12, 6))
    
    # Plot stabilization time
    ax1 = plt.subplot(121)
    sns.barplot(data=df, x="scenario", y=("time", "max"), ax=ax1)
    ax1.set_title("Maximum Stabilization Time")
    ax1.set_ylabel("Seconds")
    ax1.tick_params(axis='x', rotation=45)
    
    # Plot beta variance
    ax2 = plt.subplot(122)
    sns.barplot(data=df, x="scenario", y=("value", "std"), ax=ax2)
    ax2.set_title("Beta Value Standard Deviation")
    ax2.tick_params(axis='x', rotation=45)
    
    plt.tight_layout()
    plt.savefig(Path(result_dir) / "metrics_plot.png")
    print(f"Saved plot to {result_dir}/metrics_plot.png")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("result_dir", help="Directory with analysis.csv")
    args = parser.parse_args()
    plot_metrics(args.result_dir)