#!/usr/bin/env python3
import pandas as pd
import numpy as np
from pathlib import Path

def analyze_results(result_dir):
    # Load vector files
    vec_files = list(Path(result_dir).glob("*.vec"))
    
    dfs = []
    for vec in vec_files:
        df = pd.read_csv(vec, sep="\t", comment="#",
                        names=["time", "value", "attr"])
        df["scenario"] = vec.stem.split("_")[0]
        dfs.append(df)
    
    combined = pd.concat(dfs)
    
    # Calculate metrics
    metrics = combined.groupby("scenario").agg({
        "value": ["mean", "std", "count"],
        "time": ["max"]
    })
    
    # Save analysis
    metrics.to_csv(Path(result_dir) / "analysis.csv")
    print(f"Saved analysis to {result_dir}/analysis.csv")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("result_dir", help="Directory with .vec files")
    args = parser.parse_args()
    analyze_results(args.result_dir)