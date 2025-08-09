#!/bin/bash
set -e

# Initialize results directory
mkdir -p /workspace/results

# Run experiments
cd /workspace/omnetpp/simulations
for config in BaselineCBT FaultDistance BaselineER FaultBeta; do
    ./scm-simulations -u Cmdenv -c $config -n ../networks \
        --result-dir=/workspace/results/$config
done

# Create latest symlink
ln -sfn $(date +%Y%m%d_%H%M%S) /workspace/results/latest