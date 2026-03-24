#!/usr/bin/env python3
"""Build top-priority MWE outputs (3 topology line plots across corruption levels)."""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

LEVEL_RE = re.compile(r"^MWE_(CBT|ER|Twitch)_L([1-9])$")


def _parse_topology_level(name: str) -> tuple[str, int] | None:
    m = LEVEL_RE.match(name)
    if not m:
        return None
    return m.group(1), int(m.group(2))


def build_mwe_metrics(result_root: Path) -> pd.DataFrame:
    rows: list[dict[str, float | int | str]] = []

    for scen_dir in sorted(p for p in result_root.iterdir() if p.is_dir()):
        parsed = _parse_topology_level(scen_dir.name)
        if not parsed:
            continue
        topology, level = parsed
        sample_csv = scen_dir / "fault_samples.csv"
        if not sample_csv.exists():
            raise FileNotFoundError(f"Missing fault sample export: {sample_csv}")
        samples = pd.read_csv(sample_csv)
        required = {
            "corruption_level",
            "beta_pct_increase",
            "payment_pct_increase",
            "service_fraction",
        }
        missing = required.difference(samples.columns)
        if missing:
            raise ValueError(f"Missing required columns in {sample_csv}: {sorted(missing)}")
        rows_for_level = samples[samples["corruption_level"].astype(int) == level]
        if rows_for_level.empty:
            raise ValueError(f"No sample rows for corruption level {level} in {sample_csv}")

        rows.append(
            {
                "topology": topology,
                "corruption_level": level,
                "avg_beta_pct_increase": float(rows_for_level["beta_pct_increase"].astype(float).mean()),
                "avg_payment_pct_increase": float(rows_for_level["payment_pct_increase"].astype(float).mean()),
                "user_service_fraction": float(rows_for_level["service_fraction"].astype(float).mean()),
            }
        )

    if not rows:
        raise ValueError(
            "No MWE level scenarios found. Expected directories named MWE_<Topology>_L<level> under result root."
        )

    out = pd.DataFrame(rows).sort_values(["topology", "corruption_level"]).reset_index(drop=True)
    return out


def _lineplot(df: pd.DataFrame, y_col: str, title: str, y_label: str, out_path: Path, ylim: tuple[float, float] | None = None) -> None:
    plt.figure(figsize=(8.5, 6))
    sns.lineplot(data=df, x="corruption_level", y=y_col, hue="topology", marker="o", linewidth=2.2)
    plt.title(title)
    plt.xlabel("Corruption level (SCM level of manipulated node)")
    plt.ylabel(y_label)
    plt.xticks(list(range(1, 10)))
    if ylim is not None:
        plt.ylim(*ylim)
    plt.grid(alpha=0.25)
    plt.tight_layout()
    plt.savefig(out_path, dpi=300)
    plt.close()


def _composite_plot(df: pd.DataFrame, out_path: Path) -> None:
    fig, axes = plt.subplots(1, 3, figsize=(17, 5))
    sns.lineplot(data=df, x="corruption_level", y="avg_beta_pct_increase", hue="topology", marker="o", ax=axes[0])
    axes[0].set_title("Avg Beta % Increase vs Corruption Level")
    axes[0].set_xlabel("Corruption level")
    axes[0].set_ylabel("Avg beta % increase")
    axes[0].set_xticks(list(range(1, 10)))
    axes[0].grid(alpha=0.25)

    sns.lineplot(data=df, x="corruption_level", y="avg_payment_pct_increase", hue="topology", marker="o", ax=axes[1], legend=False)
    axes[1].set_title("Avg Payment % Increase vs Corruption Level")
    axes[1].set_xlabel("Corruption level")
    axes[1].set_ylabel("Avg payment % increase")
    axes[1].set_xticks(list(range(1, 10)))
    axes[1].grid(alpha=0.25)

    sns.lineplot(data=df, x="corruption_level", y="user_service_fraction", hue="topology", marker="o", ax=axes[2], legend=False)
    axes[2].set_title("User Service Fraction vs Corruption Level")
    axes[2].set_xlabel("Corruption level")
    axes[2].set_ylabel("Service fraction [0,1]")
    axes[2].set_xticks(list(range(1, 10)))
    axes[2].set_ylim(0.0, 1.05)
    axes[2].grid(alpha=0.25)

    fig.tight_layout()
    fig.savefig(out_path, dpi=300)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("result_root", help="Root containing per-config result directories")
    parser.add_argument("out_dir", help="Output directory for MWE artifacts")
    args = parser.parse_args()

    result_root = Path(args.result_root)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    try:
        metrics = build_mwe_metrics(result_root)
    except (FileNotFoundError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)

    analysis_csv = out_dir / "analysis.csv"
    metrics.to_csv(analysis_csv, index=False, float_format="%.6f")
    print(f"Saved analysis to {analysis_csv}")

    _lineplot(
        metrics,
        "avg_beta_pct_increase",
        "Average Beta % Increase vs Corruption Level",
        "Avg beta % increase",
        out_dir / "beta_increase_vs_level.png",
    )
    _lineplot(
        metrics,
        "avg_payment_pct_increase",
        "Average Payment % Increase vs Corruption Level",
        "Avg payment % increase",
        out_dir / "payment_increase_vs_level.png",
    )
    _lineplot(
        metrics,
        "user_service_fraction",
        "User Service Fraction vs Corruption Level",
        "Service fraction [0,1]",
        out_dir / "service_fraction_vs_level.png",
        ylim=(0.0, 1.05),
    )
    _composite_plot(metrics, out_dir / "metrics_plot.png")

    print(f"Saved plot to {out_dir / 'metrics_plot.png'}")
    print(f"Saved plot to {out_dir / 'beta_increase_vs_level.png'}")
    print(f"Saved plot to {out_dir / 'payment_increase_vs_level.png'}")
    print(f"Saved plot to {out_dir / 'service_fraction_vs_level.png'}")


if __name__ == "__main__":
    main()
