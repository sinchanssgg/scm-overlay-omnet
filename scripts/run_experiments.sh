#!/usr/bin/env bash
# run_experiments.sh — Master pipeline for SCM simulation experiments
# Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
# Modified By: Arannya Mukherjee <arannya@adhrith.ai>
set -euo pipefail

# Resolve project root relative to this script's location
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SKIP_SIM_BUILD=0
QUICK_MODE=0
MWE_MODE=0
CLAIM_A_MODE=0
CLAIM_B_MODE=0
FIG2_MODE=0
FIG5_MODE=0

usage() {
    cat <<'EOF'
Run SCM experiments locally (no Docker required)

Usage:
    scripts/run_experiments.sh [options]

Options:
    --quick              Run only BaselineCBT for a fast smoke test
    --mwe                Run only the artifact minimum working example
    --claim-a            Run claim-matrix scaffold scenarios (PR-A)
    --claim-b            Run claim-matrix algorithm comparison scenarios (PR-B)
    --fig2               Run strict Outcome-1 beta-vs-depth experiment
    --fig5               Run strict Outcome-2 convergence-vs-depth experiment
    --skip-sim-build     Skip simulation binary build step
    -h, --help           Show this help

Environment:
    RESULT_DIR           Custom result directory (default: results/<timestamp>)
    SCM_RANDOM_SEED      Seed for deterministic preprocessing/simulation (default: 1337)
    MWE_NUM_NODES        Node count for --mwe mode (default: 1023)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --quick)
            QUICK_MODE=1
            shift
            ;;
        --mwe)
            MWE_MODE=1
            shift
            ;;
        --claim-a)
            CLAIM_A_MODE=1
            shift
            ;;
        --claim-b)
            CLAIM_B_MODE=1
            shift
            ;;
        --fig2)
            FIG2_MODE=1
            shift
            ;;
        --fig5)
            FIG5_MODE=1
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

if [[ "$MWE_MODE" -eq 1 && ( "$CLAIM_A_MODE" -eq 1 || "$CLAIM_B_MODE" -eq 1 || "$FIG2_MODE" -eq 1 || "$FIG5_MODE" -eq 1 ) ]]; then
    echo "ERROR: --mwe cannot be combined with --claim-a/--claim-b/--fig2/--fig5" >&2
    exit 2
fi

if [[ "$CLAIM_A_MODE" -eq 1 && "$CLAIM_B_MODE" -eq 1 ]]; then
    echo "ERROR: --claim-a and --claim-b cannot be combined" >&2
    exit 2
fi

if [[ "$FIG2_MODE" -eq 1 && ( "$CLAIM_A_MODE" -eq 1 || "$CLAIM_B_MODE" -eq 1 || "$FIG5_MODE" -eq 1 ) ]]; then
    echo "ERROR: --fig2 cannot be combined with --claim-a/--claim-b/--fig5" >&2
    exit 2
fi

if [[ "$FIG5_MODE" -eq 1 && ( "$CLAIM_A_MODE" -eq 1 || "$CLAIM_B_MODE" -eq 1 || "$FIG2_MODE" -eq 1 ) ]]; then
    echo "ERROR: --fig5 cannot be combined with --claim-a/--claim-b/--fig2" >&2
    exit 2
fi

if ! command -v uv >/dev/null 2>&1; then
    echo "ERROR: uv is required but not found in PATH." >&2
    echo "Install uv first: https://docs.astral.sh/uv/getting-started/installation/" >&2
    exit 1
fi

SCM_RANDOM_SEED="${SCM_RANDOM_SEED:-1337}"
export SCM_RANDOM_SEED

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
export OMNETPP_RNGSEEDSET="$SCM_RANDOM_SEED"

# Generate topology files
CBT_NODES=31
ER_NODES=50
ER_PROB=0.2
if [[ "$MWE_MODE" -eq 1 || "$FIG2_MODE" -eq 1 || "$FIG5_MODE" -eq 1 ]]; then
    CBT_NODES="${MWE_NUM_NODES:-1023}"
    ER_NODES="${MWE_NUM_NODES:-1023}"
    ER_PROB=0.02
elif [[ "$CLAIM_A_MODE" -eq 1 ]]; then
    if [[ "$QUICK_MODE" -eq 1 ]]; then
        CBT_NODES=63
        ER_NODES=50
    else
        CBT_NODES=127
        ER_NODES=100
    fi
elif [[ "$CLAIM_B_MODE" -eq 1 ]]; then
    if [[ "$QUICK_MODE" -eq 1 ]]; then
        CBT_NODES=63
    else
        CBT_NODES=127
    fi
fi
uv run python "$SCRIPT_DIR/preprocess/generate_cbt.py" --nodes "$CBT_NODES" --output "$RESULT_DIR/cbt_edges.txt"
uv run python "$SCRIPT_DIR/preprocess/generate_er.py" --nodes "$ER_NODES" --prob "$ER_PROB" --seed "$SCM_RANDOM_SEED" --output "$RESULT_DIR/er_edges.txt"
uv run python "$SCRIPT_DIR/preprocess/generate_er.py" --nodes 256 --prob 0.02 --seed "$SCM_RANDOM_SEED" --output "$RESULT_DIR/twitch_edges.txt"

# Run simulations
SIM_DIR="$PROJECT_ROOT/omnetpp/simulations"

if [[ "$SKIP_SIM_BUILD" -eq 0 ]]; then
    echo "Building simulation binary..."
    (cd "$SIM_DIR" && \
     opp_makemake -f --deep -o scm-simulations -I/usr/include -lssl -lcrypto && \
     make -j"$(nproc)")
fi

cd "$SIM_DIR"

# --- Outcome 1: strict beta-vs-depth experiment ---
if [[ "$FIG2_MODE" -eq 1 ]]; then
    sim_root="$RESULT_DIR/fig2"
    FIG_DEPTH_MAX="${FIG_DEPTH_MAX:-9}"

    declare -a fig2_topos=("CBT" "ER" "Twitch")
    declare -a fig2_baselines=("BaselineCBT" "BaselineER" "BaselineTwitch")
    declare -a fig2_faults=("Fig2_CBT_FaultParent" "Fig2_ER_FaultParent" "Fig2_Twitch_FaultParent")

    for idx in 0 1 2; do
        topo="${fig2_topos[$idx]}"
        baseline_cfg="${fig2_baselines[$idx]}"
        fault_cfg="${fig2_faults[$idx]}"
        topo_root="$sim_root/$topo"

        echo "=== Fig2: $topo baseline ==="
        baseline_dir="$topo_root/baseline"
        mkdir -p "$baseline_dir"
        cp -f "$RESULT_DIR/cbt_edges.txt" "$baseline_dir/cbt_edges.txt"
        cp -f "$RESULT_DIR/er_edges.txt" "$baseline_dir/er_edges.txt"
        cp -f "$RESULT_DIR/twitch_edges.txt" "$baseline_dir/twitch_edges.txt"
        OMNETPP_RNGSEEDSET="$SCM_RANDOM_SEED" \
        ./scm-simulations -u Cmdenv -c "$baseline_cfg" -n networks \
            --result-dir="$baseline_dir" \
            --**.numNodes="${MWE_NUM_NODES:-1023}"

        for depth in $(seq 1 "$FIG_DEPTH_MAX"); do
            run_dir="$topo_root/depth_${depth}/run_0"
            mkdir -p "$run_dir"
            cp -f "$RESULT_DIR/cbt_edges.txt" "$run_dir/cbt_edges.txt"
            cp -f "$RESULT_DIR/er_edges.txt" "$run_dir/er_edges.txt"
            cp -f "$RESULT_DIR/twitch_edges.txt" "$run_dir/twitch_edges.txt"
            OMNETPP_RNGSEEDSET="$SCM_RANDOM_SEED" \
            ./scm-simulations -u Cmdenv -c "$fault_cfg" -n networks \
                --result-dir="$run_dir" \
                --**.numNodes="${MWE_NUM_NODES:-1023}" \
                --**.faultInjector.campaignTargetDepth=-1 \
                --**.faultInjector.maxCampaignDepth="$depth" \
                --**.faultInjector.campaignSeed="$SCM_RANDOM_SEED"
            echo "  $topo depth $depth: 1 deterministic run done"
        done
    done

    # Analysis + plot
    uv run python "$SCRIPT_DIR/analysis/build_fig2_outputs.py" "$sim_root" --result-root "$sim_root"
    echo "Outcome-1 experiment completed"
    exit 0
fi

# --- Outcome 2: strict convergence-vs-depth experiment ---
if [[ "$FIG5_MODE" -eq 1 ]]; then
    sim_root="$RESULT_DIR/fig5"
    FIG_DEPTH_MAX="${FIG_DEPTH_MAX:-9}"
    mkdir -p "$sim_root"

    for depth in $(seq 1 "$FIG_DEPTH_MAX"); do
        for alg in SCM GargGrosu; do
            run_dir="$sim_root/CBT/depth_${depth}/${alg}"
            mkdir -p "$run_dir"
            cp -f "$RESULT_DIR/cbt_edges.txt" "$run_dir/cbt_edges.txt"
            cp -f "$RESULT_DIR/er_edges.txt" "$run_dir/er_edges.txt"
            cp -f "$RESULT_DIR/twitch_edges.txt" "$run_dir/twitch_edges.txt"

            if [[ "$alg" == "SCM" ]]; then
                variant="scm"
                global_round_mode="true"
            else
                variant="garg-grosu"
                global_round_mode="true"
            fi

            OMNETPP_RNGSEEDSET="$SCM_RANDOM_SEED" \
            ./scm-simulations -u Cmdenv -c Fig2_CBT_FaultParent -n networks \
                --result-dir="$run_dir" \
                --**.numNodes="${MWE_NUM_NODES:-1023}" \
                --**.node[*].algorithmVariant="\"$variant\"" \
                --**.node[*].globalRoundMode="$global_round_mode" \
                --**.faultInjector.sendFaultNotify=true \
                --**.faultInjector.campaignTargetDepth=-1 \
                --**.faultInjector.maxCampaignDepth="$depth" \
                --**.faultInjector.campaignSeed="$SCM_RANDOM_SEED"
        done
        echo "  Outcome2 depth $depth: SCM + Garg-Grosu done"
    done

    uv run python "$SCRIPT_DIR/analysis/build_fig5_outputs.py" "$sim_root" --result-root "$sim_root"
    echo "Outcome-2 experiment completed"
    exit 0
fi

if [[ "$MWE_MODE" -eq 1 ]]; then
    MWE_NUM_NODES="${MWE_NUM_NODES:-1023}"
    if ! [[ "$MWE_NUM_NODES" =~ ^[0-9]+$ ]] || [[ "$MWE_NUM_NODES" -lt 8 ]]; then
        echo "ERROR: MWE_NUM_NODES must be an integer >= 8 (got: $MWE_NUM_NODES)" >&2
        exit 2
    fi
    export MWE_NUM_NODES
    configs=(
        MWE_CBT_Baseline
        MWE_ER_Baseline
        MWE_Twitch_Baseline
        MWE_CBT_L1 MWE_CBT_L2 MWE_CBT_L3 MWE_CBT_L4 MWE_CBT_L5 MWE_CBT_L6 MWE_CBT_L7 MWE_CBT_L8 MWE_CBT_L9
        MWE_ER_L1 MWE_ER_L2 MWE_ER_L3 MWE_ER_L4 MWE_ER_L5 MWE_ER_L6 MWE_ER_L7 MWE_ER_L8 MWE_ER_L9
        MWE_Twitch_L1 MWE_Twitch_L2 MWE_Twitch_L3 MWE_Twitch_L4 MWE_Twitch_L5 MWE_Twitch_L6 MWE_Twitch_L7 MWE_Twitch_L8 MWE_Twitch_L9
    )
elif [[ "$CLAIM_A_MODE" -eq 1 ]]; then
    if [[ "$QUICK_MODE" -eq 1 ]]; then
        configs=(
            ClaimA_CBT_Baseline_D5
            ClaimA_CBT_FaultParent_D5
            ClaimA_CBT_FaultParent_D5_GargStub
            ClaimA_CBT_FaultParent_D5_ByrenheidStub
        )
    else
        configs=(
            ClaimA_CBT_Baseline_D4
            ClaimA_CBT_FaultParent_D4
            ClaimA_CBT_Baseline_D5
            ClaimA_CBT_FaultParent_D5
            ClaimA_CBT_Baseline_D6
            ClaimA_CBT_FaultParent_D6
            ClaimA_ER_Baseline_N50
            ClaimA_ER_FaultBeta_N50
            ClaimA_ER_Baseline_N100
            ClaimA_ER_FaultBeta_N100
            ClaimA_CBT_FaultParent_D5_GargStub
            ClaimA_CBT_FaultParent_D5_ByrenheidStub
            ClaimA_Twitch_FaultParent_N256
        )
    fi
elif [[ "$CLAIM_B_MODE" -eq 1 ]]; then
    if [[ "$QUICK_MODE" -eq 1 ]]; then
        configs=(
            ClaimA_CBT_FaultParent_D5
            ClaimA_CBT_FaultParent_D5_GargStub
            ClaimA_CBT_FaultParent_D5_ByrenheidStub
        )
    else
        configs=(
            ClaimA_CBT_FaultParent_D4
            ClaimA_CBT_FaultParent_D5
            ClaimA_CBT_FaultParent_D6
            ClaimA_CBT_FaultParent_D5_GargStub
            ClaimA_CBT_FaultParent_D5_ByrenheidStub
            ClaimA_Twitch_FaultParent_N256
        )
    fi
elif [[ "$QUICK_MODE" -eq 1 ]]; then
    configs=(BaselineCBT)
else
    configs=(
        BaselineCBT
        FaultDistance
        FaultBetaCBT
        FaultParentCBT
        BaselineER
        FaultDistanceER
        FaultBeta
        FaultParentER
        BaselineTwitch
        FaultDistanceTwitch
        FaultBetaTwitch
        FaultParent
    )
fi

if [[ "$CLAIM_A_MODE" -eq 1 ]]; then
    sim_root="$RESULT_DIR/claim-a"
elif [[ "$CLAIM_B_MODE" -eq 1 ]]; then
    sim_root="$RESULT_DIR/claim-b"
else
    sim_root="$RESULT_DIR"
fi

for config in "${configs[@]}"; do
    echo "Running $config..."
    config_result_dir="$sim_root/$config"
    mkdir -p "$config_result_dir"
    cp -f "$RESULT_DIR/cbt_edges.txt" "$config_result_dir/cbt_edges.txt"
    cp -f "$RESULT_DIR/er_edges.txt" "$config_result_dir/er_edges.txt"
    cp -f "$RESULT_DIR/twitch_edges.txt" "$config_result_dir/twitch_edges.txt"
    if [[ "$MWE_MODE" -eq 1 ]]; then
        ./scm-simulations -u Cmdenv -c "$config" -n networks \
            --result-dir="$config_result_dir" \
            --**.numNodes="$MWE_NUM_NODES"
    else
        ./scm-simulations -u Cmdenv -c "$config" -n networks \
            --result-dir="$config_result_dir"
    fi
done

if [[ "$MWE_MODE" -eq 1 ]]; then
    mwe_root="$RESULT_DIR/mwe"
    mkdir -p "$mwe_root"
    uv run python "$SCRIPT_DIR/analysis/build_mwe_outputs.py" "$RESULT_DIR" "$mwe_root"
else
    # Process results
    uv run python "$SCRIPT_DIR/analysis/process_results.py" "$sim_root"

    # Generate visualizations
    uv run python "$SCRIPT_DIR/visualization/plot_metrics.py" "$sim_root"
fi

echo "Experiment pipeline completed"
