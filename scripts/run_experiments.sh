#!/bin/bash
set -e

RESULT_DIR="${RESULT_DIR:-results/$(date +%Y%m%d_%H%M%S)}"
mkdir -p "$RESULT_DIR"

echo "Running experiments, results will be saved to $RESULT_DIR"

# Generate topology files
python3 scripts/preprocess/generate_cbt.py --depth 5 --output "$RESULT_DIR/cbt_edges.txt"
python3 scripts/preprocess/generate_er.py --nodes 50 --prob 0.2 --output "$RESULT_DIR/er_edges.txt"

# Run simulations
cd omnetpp/simulations
for config in BaselineCBT FaultDistance BaselineER FaultBeta; do
    echo "Running $config..."
    ./scm-simulations -u Cmdenv -c $config -n ../networks \
        --result-dir="../$RESULT_DIR/$config"
done

# Process results
cd ../../scripts/analysis
python3 process_results.py "../$RESULT_DIR"

# Generate visualizations
cd ../visualization
python3 plot_metrics.py "../$RESULT_DIR"

echo "Experiment pipeline completed"