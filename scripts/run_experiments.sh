#!/bin/bash
set -e

RESULT_DIR="${RESULT_DIR:-results/$(date +%Y%m%d_%H%M%S)}"
mkdir -p "$RESULT_DIR"

echo "Running experiments, results will be saved to $RESULT_DIR"

# Generate topology files
uv run python scripts/preprocess/generate_cbt.py --depth 5 --output "$RESULT_DIR/cbt_edges.txt"
uv run python scripts/preprocess/generate_er.py --nodes 50 --prob 0.2 --output "$RESULT_DIR/er_edges.txt"

# Run simulations
cd omnetpp/simulations
for config in BaselineCBT FaultDistance BaselineER FaultBeta; do
    echo "Running $config..."
    ./scm-simulations -u Cmdenv -c $config -n ../networks \
        --result-dir="../$RESULT_DIR/$config"
done

# Process results
cd ../../scripts/analysis
uv run python process_results.py "../$RESULT_DIR"

# Generate visualizations
cd ../visualization
uv run python plot_metrics.py "../$RESULT_DIR"

echo "Experiment pipeline completed"