#!/usr/bin/env python3
"""Parse OMNeT++ vector output files and produce per-scenario metrics."""
import pandas as pd
import numpy as np
from pathlib import Path
import re
import sys


def parse_vec_file(vec_path):
    """Parse an OMNeT++ .vec file into a DataFrame.

    OMNeT++ vector files contain:
      - Header lines: version, run, attr, vector definitions
      - Data lines: vectorId  eventNumber  simtime  value
    """
    vectors = {}   # vectorId -> (module, signalName)
    rows = []

    with open(vec_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            # Vector definition: vector <id> <module> <signal>:<type> <columns>
            if line.startswith("vector "):
                parts = line.split()
                if len(parts) >= 4:
                    vec_id = parts[1]
                    module = parts[2]
                    signal = parts[3].split(":")[0]  # strip :vector suffix
                    vectors[vec_id] = (module, signal)
                continue

            # Skip other header lines
            if line.startswith(("version", "run", "attr", "param", "itervar")):
                continue

            # Data line: vectorId  eventNumber  simtime  value
            parts = line.split()
            if len(parts) >= 4:
                vec_id = parts[0]
                if vec_id in vectors:
                    module, signal = vectors[vec_id]
                    try:
                        rows.append({
                            "module": module,
                            "signal": signal,
                            "time": float(parts[2]),
                            "value": float(parts[3]),
                        })
                    except ValueError:
                        continue

    return pd.DataFrame(rows) if rows else pd.DataFrame(
        columns=["module", "signal", "time", "value"]
    )


def analyze_results(result_dir):
    result_path = Path(result_dir)

    # Search recursively for .vec files across per-scenario subdirectories
    vec_files = list(result_path.rglob("*.vec"))

    if not vec_files:
        print(f"ERROR: No .vec files found under {result_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(vec_files)} .vec file(s)")

    dfs = []
    for vec in vec_files:
        df = parse_vec_file(vec)
        if df.empty:
            print(f"  WARN: {vec.name} contained no data rows, skipping")
            continue

        # Derive scenario name from parent directory or filename
        # run_experiments.sh puts files in results/<ConfigName>/
        scenario = vec.parent.name if vec.parent != result_path else vec.stem.split("-")[0]
        df["scenario"] = scenario
        dfs.append(df)
        print(f"  Parsed {vec.name}: {len(df)} data points, scenario={scenario}")

    if not dfs:
        print("ERROR: All .vec files were empty", file=sys.stderr)
        sys.exit(1)

    combined = pd.concat(dfs, ignore_index=True)

    # Calculate per-scenario metrics
    metrics = combined.groupby("scenario").agg(
        value_mean=("value", "mean"),
        value_std=("value", "std"),
        value_count=("value", "count"),
        time_max=("time", "max"),
    ).reset_index()

    # Save with flat column names so downstream tools can read it trivially
    out_path = result_path / "analysis.csv"
    metrics.to_csv(out_path, index=False)
    print(f"Saved analysis to {out_path}")
    print(metrics.to_string(index=False))


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("result_dir", help="Root results directory (searched recursively)")
    args = parser.parse_args()
    analyze_results(args.result_dir)
