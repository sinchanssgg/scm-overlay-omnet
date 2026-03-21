#!/usr/bin/env bash
set -euo pipefail

# Resolve project root relative to this script's location
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SKIP_SIM_BUILD=0
QUICK_MODE=0

usage() {
    cat <<'EOF'
Run SCM experiments locally (no Docker required)

Usage:
    scripts/run_experiments.sh [options]

Options:
    --quick              Run only BaselineCBT for a fast smoke test
    --skip-sim-build     Skip simulation binary build step
    -h, --help           Show this help

Environment:
    RESULT_DIR           Custom result directory (default: results/<timestamp>)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --quick)
            QUICK_MODE=1
            shift
            ;;
        --skip-sim-build)
            SKIP_SIM_BUILD=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage
            exit 2
            ;;
    esac
done

if ! command -v uv >/dev/null 2>&1; then
    echo "ERROR: uv is required but not found in PATH." >&2
    echo "Install uv first: https://docs.astral.sh/uv/getting-started/installation/" >&2
    exit 1
fi

timestamp="$(date +%Y%m%d_%H%M%S)"
DEFAULT_RESULT_DIR="$PROJECT_ROOT/results/$timestamp"
RESULT_DIR="${RESULT_DIR:-$DEFAULT_RESULT_DIR}"

if ! mkdir -p "$RESULT_DIR" 2>/dev/null; then
    fallback_root="${XDG_STATE_HOME:-$HOME/.local/state}/scm-overlay-omnet/results"
    fallback_result_dir="$fallback_root/$timestamp"
    mkdir -p "$fallback_result_dir"
    echo "Warning: cannot write to $RESULT_DIR" >&2
    echo "Using fallback result directory: $fallback_result_dir" >&2
    RESULT_DIR="$fallback_result_dir"
fi

echo "Running experiments, results will be saved to $RESULT_DIR"

# Find OMNeT++ — Docker image has it at /root/omnetpp, native uses submodule
if [[ -f "/root/omnetpp/setenv" ]]; then
    export OMNETPP_ROOT="/root/omnetpp"
elif [[ -f "$PROJECT_ROOT/third_party/omnetpp/setenv" ]]; then
    export OMNETPP_ROOT="$PROJECT_ROOT/third_party/omnetpp"
else
    echo "ERROR: OMNeT++ not found" >&2
    echo "Docker: expected /root/omnetpp" >&2
    echo "Native: run 'git submodule update --init --recursive'" >&2
    exit 1
fi

# OMNeT++ setenv requires configure.user to exist.
if [[ ! -f "$OMNETPP_ROOT/configure.user" ]]; then
    cp "$OMNETPP_ROOT/configure.user.dist" "$OMNETPP_ROOT/configure.user"
fi

# OMNeT++ setenv references optional env vars that may be unset.
set +u
# shellcheck disable=SC1090
source "$OMNETPP_ROOT/setenv" -q
set -u
export OMNETPP_ROOT

# Generate topology files
uv run python "$SCRIPT_DIR/preprocess/generate_cbt.py" --depth 5 --output "$RESULT_DIR/cbt_edges.txt"
uv run python "$SCRIPT_DIR/preprocess/generate_er.py" --nodes 50 --prob 0.2 --output "$RESULT_DIR/er_edges.txt"

# Run simulations
SIM_DIR="$PROJECT_ROOT/omnetpp/simulations"

if [[ "$SKIP_SIM_BUILD" -eq 0 ]]; then
    echo "Building simulation binary..."
    (cd "$SIM_DIR" && \
     opp_makemake -f --deep -o scm-simulations -I/usr/include -lssl -lcrypto && \
     make -j"$(nproc)")
fi

cd "$SIM_DIR"
if [[ "$QUICK_MODE" -eq 1 ]]; then
    configs=(BaselineCBT)
else
    configs=(BaselineCBT FaultDistance BaselineER FaultBeta)
fi

for config in "${configs[@]}"; do
    echo "Running $config..."
    ./scm-simulations -u Cmdenv -c "$config" -n networks \
        --result-dir="$RESULT_DIR/$config"
done

# Process results
uv run python "$SCRIPT_DIR/analysis/process_results.py" "$RESULT_DIR"

# Generate visualizations
uv run python "$SCRIPT_DIR/visualization/plot_metrics.py" "$RESULT_DIR"

echo "Experiment pipeline completed"
