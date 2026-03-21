#!/bin/bash
set -e

RESULTS_BASE=/workspace/results
mkdir -p "$RESULTS_BASE"

# Run experiments — each config writes to its own subdirectory
cd /workspace/omnetpp/simulations
for config in BaselineCBT FaultDistance BaselineER FaultBeta; do
    echo "=== Running $config ==="
    mkdir -p "$RESULTS_BASE/$config"
    ./scm-simulations -u Cmdenv -c "$config" -n ../networks \
        --result-dir="$RESULTS_BASE/$config"
done

# Create latest symlink pointing to the results root
# (all scenario subdirectories live directly under $RESULTS_BASE)
ln -sfn "$RESULTS_BASE" /workspace/results/latest

echo "=== All simulations complete ==="
