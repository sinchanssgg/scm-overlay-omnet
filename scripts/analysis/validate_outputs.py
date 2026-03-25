#!/usr/bin/env python3
"""Validate artifact output contracts for analysis CSV and metrics plot.

Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
"""
from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("result_dir", help="Directory containing analysis.csv and metrics_plot.png")
    parser.add_argument(
        "--require-columns",
        default="scenario,value_mean,value_std,value_count,time_max",
        help="Comma-separated list of required CSV columns",
    )
    parser.add_argument(
        "--require-non-empty",
        action="store_true",
        help="Fail if analysis.csv has no data rows",
    )
    parser.add_argument(
        "--require-files",
        default="",
        help="Comma-separated list of additional required files in result_dir",
    )
    return parser.parse_args()


def validate_csv(path: Path, required_columns: list[str], require_non_empty: bool) -> None:
    if not path.exists():
        raise ValueError(f"Missing required file: {path}")

    with path.open("r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        actual = reader.fieldnames or []
        missing = [c for c in required_columns if c not in actual]
        if missing:
            raise ValueError(f"{path} is missing required columns: {missing}")
        rows = list(reader)
        if require_non_empty and not rows:
            raise ValueError(f"{path} is empty")


def validate_png(path: Path) -> None:
    if not path.exists():
        raise ValueError(f"Missing required file: {path}")
    with path.open("rb") as f:
        sig = f.read(8)
    if sig != PNG_SIGNATURE:
        raise ValueError(f"{path} is not a valid PNG file")


def main() -> None:
    args = parse_args()
    result_dir = Path(args.result_dir)
    required_columns = [c.strip() for c in args.require_columns.split(",") if c.strip()]
    required_files = [c.strip() for c in args.require_files.split(",") if c.strip()]

    try:
        validate_csv(result_dir / "analysis.csv", required_columns, args.require_non_empty)
        validate_png(result_dir / "metrics_plot.png")
        for rel in required_files:
            path = result_dir / rel
            if not path.exists():
                raise ValueError(f"Missing required file: {path}")
            if path.suffix.lower() == ".png":
                validate_png(path)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)

    print(f"Validated outputs in {result_dir}")


if __name__ == "__main__":
    main()
