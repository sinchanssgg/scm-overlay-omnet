#!/usr/bin/env python3
"""Build Figure-5 convergence outputs from simulation node-state + vectors."""
from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


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


def load_node_state(path: Path) -> pd.DataFrame:
    csv_path = path / "mwe_node_state.csv"
    if not csv_path.exists():
        raise FileNotFoundError(f"Expected node state export: {csv_path}")
    df = pd.read_csv(csv_path)
    required = {"level", "status", "proof_valid"}
    missing = required.difference(df.columns)
    if missing:
        raise ValueError(f"Missing required columns in {csv_path}: {sorted(missing)}")
    return df


def infer_topology(scenario: str) -> str:
    if "ER" in scenario or "Beta" in scenario:
        return "Twitch"
    return "CBT"


def infer_algorithm(scenario: str) -> str:
    if "Distance" in scenario:
        return "SCM"
    if "Beta" in scenario:
        return "Byrenheid"
    return "Garg-Grosu"


def latest_stabilization_time(scenario_dir: Path) -> float:
    times: list[float] = []
    for vec in scenario_dir.glob("*.vec"):
        df = parse_vec_file(vec)
        if df.empty:
            continue
        stable = df[df["signal"] == "nodeStableTime"]
        if stable.empty:
            times.append(float(df["time"].max()))
        else:
            times.append(float(stable["time"].max()))
    if not times:
        raise ValueError(f"No usable vector data found in {scenario_dir}")
    return float(sum(times) / len(times))


def build_analysis(result_root: Path) -> pd.DataFrame:
    rows = []
    scenario_dirs = [p for p in result_root.iterdir() if p.is_dir()]
    if not scenario_dirs:
        raise FileNotFoundError(f"No scenario directories found under {result_root}")
    for scenario_dir in scenario_dirs:
        scenario = scenario_dir.name
        state_path = scenario_dir / "mwe_node_state.csv"
        if not state_path.exists():
            continue
        state = load_node_state(scenario_dir)
        rounds = latest_stabilization_time(scenario_dir)
        depth = int(state["level"].max())
        correct = bool(((state["status"] == "STABLE") & (state["proof_valid"].astype(int) == 1)).all())

        rows.append(
            {
                "topology": infer_topology(scenario),
                "depth": depth,
                "algorithm": infer_algorithm(scenario),
                "rounds_to_converge": rounds,
                "correct_convergence": correct,
            }
        )

    if not rows:
        raise ValueError("Parsed vectors but no usable rows for convergence analysis")

    out = pd.DataFrame(rows)
    out = (
        out.groupby(["topology", "depth", "algorithm"], as_index=False)
        .agg(
            rounds_to_converge=("rounds_to_converge", "mean"),
            correct_convergence=("correct_convergence", "all"),
        )
    )
    out["rounds_to_converge"] = out["rounds_to_converge"].round(6)
    return out


def plot_fig5(df: pd.DataFrame, out_path: Path) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(13, 5), sharey=True)
    for ax, topology in zip(axes, ("Twitch", "CBT")):
        subset = df[df["topology"] == topology]
        if subset.empty:
            ax.set_title(f"{topology} convergence rounds")
            ax.text(0.5, 0.5, "No data", transform=ax.transAxes, ha="center", va="center")
            ax.axis("off")
            continue
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
    parser.add_argument("--result-root", required=True, help="Result root containing scenario subdirs and .vec files")
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = build_analysis(Path(args.result_root))
    out_csv = out_dir / "analysis.csv"
    out_png = out_dir / "metrics_plot.png"
    df.to_csv(out_csv, index=False)
    plot_fig5(df, out_png)

    print(f"Saved analysis to {out_csv}")
    print(f"Saved plot to {out_png}")


if __name__ == "__main__":
    main()
