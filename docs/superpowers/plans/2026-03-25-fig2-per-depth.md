# Figure 2 Per-Depth Experiment — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Figure 2 produces per-depth curves (one line per topology) showing how beta/payment increase grows with depth, matching the paper.

**Architecture:** Add `campaignTargetDepth` parameter to SCMFaultInjector for single-depth targeting. Add `--fig2` flag to `run_experiments.sh` that loops over depths × reps. Rewrite `build_fig2_outputs.py` to read per-depth directories and compute per-corrupted-node metrics.

**Tech Stack:** C++ (OMNeT++ 6.x), Bash, Python 3 (pandas, matplotlib, seaborn)

**Spec:** `docs/superpowers/specs/2026-03-25-fig2-per-depth-design.md`

**Branch:** `fix/fig2-per-depth-grouping`

**Issue:** #59

---

### Task 1: Add `campaignTargetDepth` to SCMFaultInjector NED

**Files:**
- Modify: `omnetpp/simulations/networks/SCMFaultInjector.ned`

- [ ] **Step 1: Add the new parameter**

Add after `int maxCampaignDepth @default(-1);` (line 10):
```ned
        int campaignTargetDepth @default(-1); // >=0: corrupt only at this exact depth; -1: use maxCampaignDepth range
```

- [ ] **Step 2: Verify NED is valid**

Run: `grep "campaignTargetDepth" omnetpp/simulations/networks/SCMFaultInjector.ned`
Expected: one match with `@default(-1)`

- [ ] **Step 3: Commit**

```bash
git add omnetpp/simulations/networks/SCMFaultInjector.ned
git commit -m "Add campaignTargetDepth NED parameter to SCMFaultInjector

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>"
```

---

### Task 2: Add `campaignTargetDepth` to SCMFaultInjector C++

**Files:**
- Modify: `omnetpp/simulations/src/SCMFaultInjector.h`
- Modify: `omnetpp/simulations/src/SCMFaultInjector.cc`

- [ ] **Step 1: Add member variable to header**

In `SCMFaultInjector.h`, after `int maxCampaignDepth;` (line 24), add:
```cpp
    int campaignTargetDepth;
```

- [ ] **Step 2: Read parameter in initialize()**

In `SCMFaultInjector.cc`, after `maxCampaignDepth = par("maxCampaignDepth").intValue();` (line 101), add:
```cpp
    campaignTargetDepth = par("campaignTargetDepth").intValue();
```

- [ ] **Step 3: Fix buildDepthBuckets() for non-CBT networks**

The current implementation uses `computeCbtDepthFromIndex()` to size the bucket array and as a fallback for invalid levels. This produces wrong depth assignments for ER/Twitch. When `campaignTargetDepth >= 0`, use actual `getLevel()` values.

Replace `buildDepthBuckets()` (lines 22-35) with:
```cpp
void SCMFaultInjector::buildDepthBuckets(int numNodes)
{
    depthBuckets.clear();
    cModule *network = getParentModule();

    // First pass: find actual max depth from node levels
    int maxDepth = 0;
    for (int i = 0; i < numNodes; i++) {
        SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
        int depth = node->getLevel();
        if (depth >= 0 && depth < 1000000) {
            maxDepth = std::max(maxDepth, depth);
        }
    }

    // Fall back to CBT estimate if no valid levels found
    if (maxDepth == 0) {
        maxDepth = computeCbtDepthFromIndex(numNodes - 1);
    }

    depthBuckets.resize(maxDepth + 1);

    // Second pass: bucket nodes by their actual level
    for (int i = 0; i < numNodes; i++) {
        SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
        int depth = node->getLevel();
        if (depth < 0 || depth >= (int)depthBuckets.size()) {
            // Skip nodes with invalid/sentinel levels (e.g., FAULTY nodes at INT_MAX)
            continue;
        }
        depthBuckets[depth].push_back(i);
    }
}
```

- [ ] **Step 4: Modify selectDeterministicTargets() for single-depth mode**

Replace `selectDeterministicTargets()` (lines 37-54) with:
```cpp
std::vector<int> SCMFaultInjector::selectDeterministicTargets() const
{
    std::vector<int> targets;
    for (int depth = 0; depth < (int)depthBuckets.size(); depth++) {
        if (depth == 0) {
            continue;  // Never corrupt root
        }
        // campaignTargetDepth >= 0: corrupt ONLY at that exact depth
        if (campaignTargetDepth >= 0 && depth != campaignTargetDepth) {
            continue;
        }
        // campaignTargetDepth < 0: fall back to maxCampaignDepth range behavior
        if (campaignTargetDepth < 0 && maxCampaignDepth >= 0 && depth > maxCampaignDepth) {
            continue;
        }
        const auto &bucket = depthBuckets[depth];
        if (bucket.empty()) {
            continue;
        }
        int idx = (campaignSeed + campaignRound + depth) % (int)bucket.size();
        targets.push_back(bucket[idx]);
    }
    return targets;
}
```

- [ ] **Step 5: Allow non-CBT networks when campaignTargetDepth is set**

In `injectFault()` (lines 130-146), replace the CBT check block with:
```cpp
    if (campaignMode == DETERMINISTIC_ONE_NODE_PER_DEPTH) {
        const char *networkName = network->getNedTypeName();
        bool cbtLike = std::string(networkName) == "CompleteBinaryTree";
        // When campaignTargetDepth is set, allow any network type
        if (!cbtLike && campaignTargetDepth < 0) {
            if (strictDepthCampaign) {
                throw cRuntimeError("Deterministic depth campaign requires CompleteBinaryTree network; got %s", networkName);
            }
            EV_WARN << "Skipping deterministic depth campaign on non-CBT network " << networkName << endl;
            return;
        }
        buildDepthBuckets(numNodes);
        auto targets = selectDeterministicTargets();
        for (int nodeId : targets) {
            SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", nodeId));
            applyFaultToNode(node, numNodes);
        }
        return;
    }
```

- [ ] **Step 6: Verify compilation**

Run (on test machine):
```bash
cd omnetpp/simulations
opp_makemake -f --deep -o scm-simulations -I/usr/include -lssl -lcrypto
make -j$(nproc)
```
Expected: compiles with zero errors

- [ ] **Step 7: Commit**

```bash
git add omnetpp/simulations/src/SCMFaultInjector.h omnetpp/simulations/src/SCMFaultInjector.cc
git commit -m "Add campaignTargetDepth for single-depth fault targeting

When campaignTargetDepth >= 0, corrupt exactly one node at that
specific depth instead of the 1..maxCampaignDepth range. Fix
buildDepthBuckets to use actual node levels (not CBT index math)
so deterministic campaigns work on ER/Twitch networks.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>"
```

---

### Task 3: Add Fig2 configs to omnetpp.ini

**Files:**
- Modify: `omnetpp/simulations/omnetpp.ini`

- [ ] **Step 1: Add three Fig2 fault configs**

Append to `omnetpp.ini` after the existing Claim-A configs:
```ini
#---------------------------
# Figure 2: per-depth parent-manipulation experiment
#---------------------------

[Config Fig2_CBT_FaultParent]
extends = BaselineCBT
description = "Fig2 CBT: single-depth parent corruption"
repeat = 1
*.faultInjector.faultType = 2
*.faultInjector.campaignMode = 1
*.faultInjector.parentOffset = 1
*.faultInjector.sendFaultNotify = false
*.faultInjector.interval = 999s
*.faultInjector.campaignTargetDepth = -1

[Config Fig2_ER_FaultParent]
extends = BaselineER
description = "Fig2 ER: single-depth parent corruption"
repeat = 1
*.faultInjector.faultType = 2
*.faultInjector.campaignMode = 1
*.faultInjector.parentOffset = 1
*.faultInjector.sendFaultNotify = false
*.faultInjector.interval = 999s
*.faultInjector.campaignTargetDepth = -1
*.faultInjector.strictDepthCampaign = false

[Config Fig2_Twitch_FaultParent]
extends = BaselineTwitch
description = "Fig2 Twitch: single-depth parent corruption"
repeat = 1
*.faultInjector.faultType = 2
*.faultInjector.campaignMode = 1
*.faultInjector.parentOffset = 1
*.faultInjector.sendFaultNotify = false
*.faultInjector.interval = 999s
*.faultInjector.campaignTargetDepth = -1
*.faultInjector.strictDepthCampaign = false
```

- [ ] **Step 2: Verify configs are parseable**

Run: `grep "Config Fig2" omnetpp/simulations/omnetpp.ini`
Expected: 3 matches

- [ ] **Step 3: Commit**

```bash
git add omnetpp/simulations/omnetpp.ini
git commit -m "Add Fig2 per-depth configs with single-shot injection

repeat=1, interval=999s for single fault injection per run.
campaignTargetDepth overridden at runtime by run_experiments.sh.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>"
```

---

### Task 4: Add `--fig2` flag to run_experiments.sh

**Files:**
- Modify: `scripts/run_experiments.sh`

- [ ] **Step 1: Add FIG2_MODE variable and flag parsing**

Add `FIG2_MODE=0` after `CLAIM_B_MODE=0` (line 12).

Add to usage():
```
    --fig2               Run Figure-2 per-depth experiment (depth x rep loop)
```

Add to environment section:
```
    FIG2_REPEATS         Repetitions per depth for --fig2 (default: 10, paper: 100)
```

Add case in the while loop:
```bash
        --fig2)
            FIG2_MODE=1
            shift
            ;;
```

Add mutual exclusion check:
```bash
if [[ "$FIG2_MODE" -eq 1 && ( "$MWE_MODE" -eq 1 || "$CLAIM_A_MODE" -eq 1 || "$CLAIM_B_MODE" -eq 1 ) ]]; then
    echo "ERROR: --fig2 cannot be combined with --mwe, --claim-a, or --claim-b" >&2
    exit 2
fi
```

- [ ] **Step 2: Add the fig2 experiment loop**

Add after the existing `elif [[ "$CLAIM_B_MODE" -eq 1 ]]; then` block (before `elif [[ "$QUICK_MODE" -eq 1 ]]; then`):

```bash
elif [[ "$FIG2_MODE" -eq 1 ]]; then
    FIG2_REPEATS="${FIG2_REPEATS:-10}"
    sim_root="$RESULT_DIR/fig2"

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
        ./scm-simulations -u Cmdenv -c "$baseline_cfg" -n networks \
            --result-dir="$baseline_dir"

        # Extract max depth from baseline
        max_depth=$(uv run python -c "
import pandas as pd
df = pd.read_csv('$baseline_dir/mwe_node_state.csv')
levels = pd.to_numeric(df['level'], errors='coerce').dropna().astype(int)
valid = levels[(levels >= 0) & (levels < 1000000)]
print(int(valid.max())) if not valid.empty else print(0)
" 2>/dev/null)

        if [[ -z "$max_depth" || "$max_depth" -lt 1 ]]; then
            echo "WARN: Could not determine max_depth for $topo, skipping" >&2
            continue
        fi
        echo "  Max depth for $topo: $max_depth"

        for depth in $(seq 1 "$max_depth"); do
            for rep in $(seq 0 $(( FIG2_REPEATS - 1 ))); do
                rep_dir="$topo_root/depth_${depth}/rep_${rep}"
                mkdir -p "$rep_dir"
                cp -f "$RESULT_DIR/cbt_edges.txt" "$rep_dir/cbt_edges.txt"
                cp -f "$RESULT_DIR/er_edges.txt" "$rep_dir/er_edges.txt"
                cp -f "$RESULT_DIR/twitch_edges.txt" "$rep_dir/twitch_edges.txt"
                seed=$(( SCM_RANDOM_SEED + rep ))
                OMNETPP_RNGSEEDSET="$seed" \
                ./scm-simulations -u Cmdenv -c "$fault_cfg" -n networks \
                    --result-dir="$rep_dir" \
                    --**.faultInjector.campaignTargetDepth="$depth" \
                    --**.faultInjector.campaignSeed="$seed"
            done
            echo "  $topo depth $depth: $FIG2_REPEATS reps done"
        done
    done

    # Analysis + plot
    uv run python "$SCRIPT_DIR/analysis/build_fig2_outputs.py" "$sim_root" --result-root "$sim_root"
    echo "Fig2 experiment completed"
    exit 0
```

- [ ] **Step 3: Verify flag parses correctly**

Run: `./scripts/run_experiments.sh --fig2 --help` (should show error about mutual exclusion or just the help)
Run: `./scripts/run_experiments.sh --help` (should list --fig2)

- [ ] **Step 4: Commit**

```bash
git add scripts/run_experiments.sh
git commit -m "Add --fig2 flag with per-depth x repetition experiment loop

Runs baseline once per topology, then loops depth x FIG2_REPEATS
(default 10) fault runs with campaignTargetDepth override.
Extracts max_depth from baseline mwe_node_state.csv.

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>"
```

---

### Task 5: Rewrite build_fig2_outputs.py for per-depth analysis

**Files:**
- Modify: `scripts/analysis/build_fig2_outputs.py`

- [ ] **Step 1: Rewrite the analysis logic**

Replace the entire file content with:
```python
#!/usr/bin/env python3
"""Build Figure-2 per-depth outputs from simulation node-state exports."""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns


def load_node_state(path: Path) -> pd.DataFrame:
    csv_path = path / "mwe_node_state.csv"
    if not csv_path.exists():
        raise FileNotFoundError(f"Expected node state export: {csv_path}")
    df = pd.read_csv(csv_path)
    required = {"node_id", "level", "beta", "payment", "status", "proof_valid"}
    missing = required.difference(df.columns)
    if missing:
        raise ValueError(f"Missing required columns in {csv_path}: {sorted(missing)}")
    return df


def analyze_depth_rep(baseline: pd.DataFrame, fault: pd.DataFrame, depth: int) -> dict | None:
    """Compare the corrupted node at a specific depth between baseline and fault runs."""
    base_at_depth = baseline[baseline["level"] == depth]
    fault_at_depth = fault[fault["level"] == depth]

    if base_at_depth.empty or fault_at_depth.empty:
        return None

    # Join on node_id to find matching nodes
    joined = base_at_depth.set_index("node_id").join(
        fault_at_depth.set_index("node_id")[["beta", "payment", "status", "proof_valid"]],
        how="inner",
        lsuffix="_base",
        rsuffix="_fault",
    )
    if joined.empty:
        return None

    # The corrupted node is the one with the largest absolute beta change
    joined["beta_delta"] = (joined["beta_fault"] - joined["beta_base"]).abs()
    corrupted = joined.loc[joined["beta_delta"].idxmax()]

    beta_base = float(corrupted["beta_base"])
    beta_fault = float(corrupted["beta_fault"])
    payment_base = float(corrupted["payment_base"])
    payment_fault = float(corrupted["payment_fault"])

    eps = 1e-9
    beta_pct = 0.0 if abs(beta_base) <= eps else ((beta_fault - beta_base) / abs(beta_base)) * 100.0
    payment_pct = 0.0 if abs(payment_base) <= eps else ((payment_fault - payment_base) / abs(payment_base)) * 100.0
    receiving_service = int(corrupted["status_fault"] == "STABLE" and int(corrupted["proof_valid_fault"]) == 1)

    return {
        "beta_pct_increase": beta_pct,
        "payment_pct_increase": payment_pct,
        "receiving_service": receiving_service,
    }


def build_topology_rows(topology: str, topo_dir: Path) -> list[dict]:
    """Build per-depth rows for one topology."""
    baseline_dir = topo_dir / "baseline"
    if not (baseline_dir / "mwe_node_state.csv").exists():
        print(f"WARN: No baseline for {topology} at {baseline_dir}", file=sys.stderr)
        return []

    baseline = load_node_state(baseline_dir)
    rows = []

    # Find all depth_N directories
    depth_dirs = sorted(
        [d for d in topo_dir.iterdir() if d.is_dir() and d.name.startswith("depth_")],
        key=lambda d: int(d.name.split("_")[1]),
    )

    for depth_dir in depth_dirs:
        depth = int(depth_dir.name.split("_")[1])
        rep_metrics = []

        # Find all rep_N directories
        rep_dirs = sorted(
            [d for d in depth_dir.iterdir() if d.is_dir() and d.name.startswith("rep_")],
            key=lambda d: int(d.name.split("_")[1]),
        )

        for rep_dir in rep_dirs:
            if not (rep_dir / "mwe_node_state.csv").exists():
                continue
            try:
                fault = load_node_state(rep_dir)
                result = analyze_depth_rep(baseline, fault, depth)
                if result:
                    rep_metrics.append(result)
            except Exception as e:
                print(f"WARN: {topology} depth {depth} {rep_dir.name}: {e}", file=sys.stderr)

        if not rep_metrics:
            print(f"WARN: No valid reps for {topology} depth {depth}", file=sys.stderr)
            continue

        avg_beta = sum(m["beta_pct_increase"] for m in rep_metrics) / len(rep_metrics)
        avg_payment = sum(m["payment_pct_increase"] for m in rep_metrics) / len(rep_metrics)
        avg_service = sum(m["receiving_service"] for m in rep_metrics) / len(rep_metrics)

        rows.append({
            "topology": topology,
            "depth": depth,
            "avg_beta_pct_increase": round(avg_beta, 6),
            "avg_payment_pct_increase": round(avg_payment, 6),
            "user_fraction_receiving_service": round(avg_service, 6),
        })

    return rows


def build_analysis(result_root: Path) -> pd.DataFrame:
    rows = []
    for topo in ("CBT", "ER", "Twitch"):
        topo_dir = result_root / topo
        if not topo_dir.is_dir():
            print(f"Skipping {topo}: directory not found at {topo_dir}", file=sys.stderr)
            continue
        rows.extend(build_topology_rows(topo, topo_dir))

    if not rows:
        raise ValueError(
            "No Figure-2 data found. Expected directories: CBT/, ER/, Twitch/ "
            "each containing baseline/ and depth_N/rep_N/ subdirectories."
        )

    return pd.DataFrame(rows)


def plot_fig2(df: pd.DataFrame, out_path: Path) -> None:
    fig, axes = plt.subplots(1, 3, figsize=(16, 5))

    sns.lineplot(data=df, x="depth", y="avg_beta_pct_increase", hue="topology", marker="o", ax=axes[0])
    axes[0].set_title("Beta increase vs depth")
    axes[0].set_ylabel("Average % increase")

    sns.lineplot(data=df, x="depth", y="avg_payment_pct_increase", hue="topology", marker="o", ax=axes[1], legend=False)
    axes[1].set_title("Payment increase vs depth")
    axes[1].set_ylabel("Average % increase")

    sns.lineplot(data=df, x="depth", y="user_fraction_receiving_service", hue="topology", marker="o", ax=axes[2], legend=False)
    axes[2].set_title("Users receiving service vs depth")
    axes[2].set_ylabel("Fraction")
    axes[2].set_ylim(0, 1.05)

    for ax in axes:
        ax.set_xlabel("Depth")
        ax.grid(alpha=0.2)
    fig.tight_layout()
    fig.savefig(out_path, dpi=300)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output_dir", help="Directory for fig2 outputs")
    parser.add_argument("--result-root", required=True, help="Root containing CBT/, ER/, Twitch/ topology dirs")
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = build_analysis(Path(args.result_root))
    out_csv = out_dir / "analysis.csv"
    out_png = out_dir / "metrics_plot.png"
    df.to_csv(out_csv, index=False, float_format="%.6f")
    plot_fig2(df, out_png)
    print(f"Saved analysis to {out_csv}")
    print(f"Saved plot to {out_png}")


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Verify script parses without syntax errors**

Run: `uv run python -c "import scripts.analysis.build_fig2_outputs"` or `uv run python scripts/analysis/build_fig2_outputs.py --help`
Expected: no import errors, shows help

- [ ] **Step 3: Commit**

```bash
git add scripts/analysis/build_fig2_outputs.py
git commit -m "Rewrite build_fig2_outputs.py for per-depth directory analysis

Reads baseline/ and depth_N/rep_N/ directories per topology.
Identifies corrupted node as max-beta-delta node at target depth.
Averages metrics across reps. Produces one row per (topology, depth).

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>"
```

---

### Task 6: End-to-end verification

- [ ] **Step 1: Run Fig2 experiment (on test machine)**

Note: must build OMNeT++ first if not already built.

```bash
FIG2_REPEATS=2 ./scripts/run_experiments.sh --fig2 --skip-sim-build
```

Expected: 3 topologies × baselines + depth loops complete. Output in `results/<timestamp>/fig2/`.

- [ ] **Step 2: Verify directory structure**

```bash
ls results/*/fig2/CBT/
```

Expected: `baseline/`, `depth_1/`, `depth_2/`, ..., `depth_N/`

```bash
ls results/*/fig2/CBT/depth_1/
```

Expected: `rep_0/`, `rep_1/`

- [ ] **Step 3: Verify analysis.csv has per-depth rows**

```bash
cat results/*/fig2/analysis.csv
```

Expected: multiple rows per topology with varying depth values, e.g.:
```
topology,depth,avg_beta_pct_increase,avg_payment_pct_increase,user_fraction_receiving_service
CBT,1,...
CBT,2,...
...
ER,1,...
```

- [ ] **Step 4: Verify plot shows curves, not dots**

Check `results/*/fig2/metrics_plot.png` — should show lines with multiple points, not single dots.

- [ ] **Step 5: Commit the plan document**

```bash
git add docs/superpowers/plans/2026-03-25-fig2-per-depth.md
git commit -m "Add Figure 2 per-depth implementation plan

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>"
```

- [ ] **Step 6: Push and update PR**

```bash
git push
```
