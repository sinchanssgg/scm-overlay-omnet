#!/usr/bin/env python3
"""Build strict Outcome-2 plot: rounds-to-converge vs depth (SCM/Garg-Grosu, CBT)."""
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


DEPTH_RE = re.compile(r"^depth_(\d+)$")


def parse_vec_file(vec_path: Path) -> pd.DataFrame:
    vectors = {}
    rows = []
    with vec_path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("vector "):
                parts = line.split()
                if len(parts) >= 4:
                    vec_id = parts[1]
                    signal = parts[3].split(":")[0]
                    vectors[vec_id] = signal
                continue
            if line.startswith(("version", "run", "attr", "param", "itervar")):
                continue
            parts = line.split()
            if len(parts) >= 4 and parts[0] in vectors:
                rows.append(
                    {
                        "signal": vectors[parts[0]],
                        "time": float(parts[2]),
                        "value": float(parts[3]),
                    }
                )
    return pd.DataFrame(rows)


def latest_stabilization_value(scenario_dir: Path) -> float:
    values: list[float] = []
    for vec in scenario_dir.glob("*.vec"):
        df = parse_vec_file(vec)
        if df.empty:
            continue
        stable = df[df["signal"] == "nodeStableTime"]
        if not stable.empty:
            values.append(float(stable["value"].max()))
    if not values:
        raise ValueError(f"No nodeStableTime values found in {scenario_dir}")
    return float(max(values))


def collect_rows(result_root: Path) -> pd.DataFrame:
    base = result_root / "CBT"
    if not base.is_dir():
        raise FileNotFoundError(f"Missing CBT directory: {base}")

    rows: list[dict[str, float | int | str]] = []
    missing_by_alg: dict[str, list[int]] = {"SCM": [], "Garg-Grosu": []}
    for depth_dir in sorted(base.glob("depth_*")):
        m = DEPTH_RE.match(depth_dir.name)
        if not m:
            continue
        depth = int(m.group(1))

        for alg_dir, alg_label in (("SCM", "SCM"), ("GargGrosu", "Garg-Grosu")):
            run_dir = depth_dir / alg_dir
            if not run_dir.is_dir():
                print(f"WARN: missing {alg_label} run dir at depth {depth}", file=sys.stderr)
                continue
            try:
                rounds = latest_stabilization_value(run_dir)
            except Exception as exc:
                print(f"WARN: {alg_label} depth {depth}: {exc}", file=sys.stderr)
                missing_by_alg[alg_label].append(depth)
                continue
            rows.append(
                {
                    "topology": "CBT",
                    "algorithm": alg_label,
                    "depth_original": depth,
                    "rounds_to_converge": rounds,
                }
            )

    if not rows:
        raise ValueError("No valid rows found for Outcome-2 analysis")

    df = pd.DataFrame(rows)
    scm_depths = set(df[df["algorithm"] == "SCM"]["depth_original"].astype(int).tolist())
    garg_depths = set(df[df["algorithm"] == "Garg-Grosu"]["depth_original"].astype(int).tolist())
    common_depths = sorted(scm_depths.intersection(garg_depths))
    if not common_depths:
        if missing_by_alg["Garg-Grosu"]:
            raise ValueError(
                "No common depths between SCM and Garg-Grosu runs: "
                "Garg-Grosu produced no nodeStableTime values. "
                "Current model does not emit a reliable global-round convergence metric for Garg-Grosu. "
                "Re-run with a supported comparator metric or updated Garg instrumentation."
            )
        raise ValueError("No common depths between SCM and Garg-Grosu runs")

    dropped_scm = sorted(scm_depths - set(common_depths))
    dropped_garg = sorted(garg_depths - set(common_depths))
    if dropped_scm:
        print(f"WARN: dropped SCM non-common depths: {dropped_scm}", file=sys.stderr)
    if dropped_garg:
        print(f"WARN: dropped Garg-Grosu non-common depths: {dropped_garg}", file=sys.stderr)

    depth_map = {d: i + 1 for i, d in enumerate(common_depths)}
    df = df[df["depth_original"].isin(common_depths)].copy()
    df["depth_plot"] = df["depth_original"].map(depth_map)
    return df.sort_values(["depth_plot", "algorithm"]).reset_index(drop=True)


def plot_outcome2(df: pd.DataFrame, out_path: Path) -> None:
    plt.figure(figsize=(8.5, 6))
    sns.set_theme(style="whitegrid")
    sns.lineplot(
        data=df,
        x="depth_plot",
        y="rounds_to_converge",
        hue="algorithm",
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
    plt.ylabel("Rounds to Converge")
    plt.title("Rounds to Converge vs. Tree Depth")
    plt.grid(alpha=0.25)
    plt.tight_layout()
    plt.savefig(out_path, dpi=300)
    plt.close()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_dir", help="Directory for fig5 outputs")
    parser.add_argument("--result-root", required=True, help="Result root containing CBT/depth_*/{SCM,GargGrosu}")
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = collect_rows(Path(args.result_root))
    out_csv = out_dir / "analysis.csv"
    out_png = out_dir / "rounds_to_converge_vs_depth.png"
    out_legacy_png = out_dir / "metrics_plot.png"
    df.to_csv(out_csv, index=False, float_format="%.6f")
    plot_outcome2(df, out_png)
    plot_outcome2(df, out_legacy_png)
    print(f"Saved analysis to {out_csv}")
    print(f"Saved plot to {out_png}")
    print(f"Saved plot to {out_legacy_png}")


if __name__ == "__main__":
    main()
