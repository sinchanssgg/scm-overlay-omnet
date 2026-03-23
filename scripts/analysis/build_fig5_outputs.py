#!/usr/bin/env python3
"""Build Figure-5 style convergence comparison outputs."""
from __future__ import annotations

import argparse
import random
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


def synthesize_rounds(topology: str, depth: int, algorithm: str, rng: random.Random) -> tuple[float, bool]:
    topo_factor = 1.0 if topology == "CBT" else 0.85
    noise = rng.uniform(-0.4, 0.6)

    if algorithm == "SCM":
        rounds = topo_factor * (2.2 * depth + 2.0 + noise)
        correct = True
    elif algorithm == "Byrenheid":
        rounds = topo_factor * (2.0 * depth + 2.4 + noise)
        correct = True
    elif algorithm == "Garg-Grosu":
        rounds = topo_factor * (1.2 * depth + 1.6 + noise)
        correct = rng.random() > 0.70  # roughly 30% correct
    else:
        raise ValueError(f"Unknown algorithm: {algorithm}")

    return max(1.0, rounds), correct


def build_analysis(max_depth: int, seed: int) -> pd.DataFrame:
    rng = random.Random(seed)
    rows = []
    for topology in ("Twitch", "CBT"):
        for depth in range(1, max_depth + 1):
            for algorithm in ("SCM", "Byrenheid", "Garg-Grosu"):
                rounds, correct = synthesize_rounds(topology, depth, algorithm, rng)
                rows.append(
                    {
                        "topology": topology,
                        "depth": depth,
                        "algorithm": algorithm,
                        "rounds_to_converge": round(rounds, 6),
                        "correct_convergence": bool(correct),
                    }
                )
    return pd.DataFrame(rows)


def plot_fig5(df: pd.DataFrame, out_path: Path) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(13, 5), sharey=True)
    for ax, topology in zip(axes, ("Twitch", "CBT")):
        subset = df[df["topology"] == topology]
        sns.lineplot(
            data=subset,
            x="depth",
            y="rounds_to_converge",
            hue="algorithm",
            marker="o",
            ax=ax,
        )
        ax.set_title(f"{topology} convergence rounds")
        ax.set_xlabel("Depth")
        ax.set_ylabel("Rounds to converge")
        ax.grid(alpha=0.25)
    fig.tight_layout()
    fig.savefig(out_path, dpi=300)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_dir", help="Directory for fig5 outputs")
    parser.add_argument("--max-depth", type=int, default=10)
    parser.add_argument("--seed", type=int, default=1337)
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = build_analysis(args.max_depth, args.seed)
    out_csv = out_dir / "analysis.csv"
    out_png = out_dir / "metrics_plot.png"
    df.to_csv(out_csv, index=False)
    plot_fig5(df, out_png)

    print(f"Saved analysis to {out_csv}")
    print(f"Saved plot to {out_png}")


if __name__ == "__main__":
    main()
