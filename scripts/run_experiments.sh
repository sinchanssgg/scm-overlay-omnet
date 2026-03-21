#!/bin/bash
set -e

# Resolve project root relative to this script's location
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RESULT_DIR="${RESULT_DIR:-$PROJECT_ROOT/results/$(date +%Y%m%d_%H%M%S)}"
mkdir -p "$RESULT_DIR"

echo "Running experiments, results will be saved to $RESULT_DIR"

# Generate topology files
uv run python "$SCRIPT_DIR/preprocess/generate_cbt.py" --depth 5 --output "$RESULT_DIR/cbt_edges.txt"
uv run python "$SCRIPT_DIR/preprocess/generate_er.py" --nodes 50 --prob 0.2 --output "$RESULT_DIR/er_edges.txt"

# Run simulations
SIM_DIR="$PROJECT_ROOT/omnetpp/simulations"
cd "$SIM_DIR"
for config in BaselineCBT FaultDistance BaselineER FaultBeta; do
    echo "Running $config..."
    ./scm-simulations -u Cmdenv -c "$config" -n networks \
        --result-dir="$RESULT_DIR/$config"
done

# Process results
uv run python "$SCRIPT_DIR/analysis/process_results.py" "$RESULT_DIR"

# Generate visualizations
uv run python "$SCRIPT_DIR/visualization/plot_metrics.py" "$RESULT_DIR"

echo "Experiment pipeline completed"
