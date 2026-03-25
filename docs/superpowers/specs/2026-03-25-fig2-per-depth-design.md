# Figure 2 Per-Depth Experiment — Design Spec

## Problem

`build_fig2_outputs.py` produces one data point per topology (a single average across all nodes). The paper's Figure 2 shows depth-varying curves — x-axis is depth, y-axis is beta/payment increase, one line per topology. The current code produces a flat chart.

## Sinchan's Clarifications

1. **Group by depth level** — each data point is about the specific corrupted node at that depth (Option A).
2. **Each depth is a separate run** — the tree starts fresh for each depth experiment.
3. **10 repetitions per depth** (configurable; paper uses 100).
4. **Single fault injection per run** — corrupt one node at exactly one depth, measure the effect.

## Spec Review Findings (Critical Corrections)

Three issues found during review that change the scope:

1. **`maxCampaignDepth` corrupts depths 1..D, not just depth D.** The `selectDeterministicTargets()` function iterates from depth 1 to `maxCampaignDepth`, picking one node at each depth. Setting `maxCampaignDepth=5` corrupts 5 nodes (one per depth 1-5), not 1 node at depth 5. A new parameter is needed.

2. **Deterministic campaign mode crashes on non-CBT networks.** `SCMFaultInjector.cc` checks `networkName == "CompleteBinaryTree"` and throws `cRuntimeError` for ER/Twitch when `strictDepthCampaign=true` (the default). The depth bucketing uses CBT-specific index math as fallback.

3. **Fault injector fires repeatedly (every 10s).** In a 30s simulation, faults fire at t=5, t=15, t=25 with potentially different targets each time. The paper requires a single-shot injection.

## Changes

### Layer 1: C++ — SCMFaultInjector

**New NED parameter:** `int campaignTargetDepth @default(-1)`
- When >= 0: corrupt exactly one node at this specific depth (not a range)
- When -1 (default): existing behavior (corrupt depths 1..maxCampaignDepth)

**Generalize for non-CBT networks:**
- In `buildDepthBuckets()`: always use `node->getLevel()` for depth bucketing. Remove the CBT-specific `computeCbtDepthFromIndex()` fallback for the `campaignTargetDepth` path. If `getLevel()` returns an invalid value (e.g., INT_MAX for a FAULTY node), skip that node.
- Remove the `strictDepthCampaign` crash for non-CBT when `campaignTargetDepth >= 0` is set — this mode is explicitly designed to work on any topology.

**Single-shot injection:** Handled via ini config — set `interval` to a value >= `sim-time-limit` (e.g., `interval = 999s`). No C++ change needed.

**Files:**
- `omnetpp/simulations/src/SCMFaultInjector.h` — add `int campaignTargetDepth` member
- `omnetpp/simulations/src/SCMFaultInjector.cc` — read new param in `initialize()`, add target-depth logic in `selectDeterministicTargets()`
- `omnetpp/simulations/networks/SCMFaultInjector.ned` — add `int campaignTargetDepth @default(-1)` parameter

### Layer 2: omnetpp.ini

Add fig2 configs. Each uses `campaignTargetDepth` (overridden at runtime) and single-shot injection:

```ini
[Config Fig2_CBT_FaultParent]
extends = BaselineCBT
repeat = 1
*.faultInjector.faultType = 2
*.faultInjector.campaignMode = 1
*.faultInjector.parentOffset = 1
*.faultInjector.sendFaultNotify = false
*.faultInjector.interval = 999s
*.faultInjector.campaignTargetDepth = -1  # overridden at runtime

[Config Fig2_ER_FaultParent]
extends = BaselineER
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
repeat = 1
*.faultInjector.faultType = 2
*.faultInjector.campaignMode = 1
*.faultInjector.parentOffset = 1
*.faultInjector.sendFaultNotify = false
*.faultInjector.interval = 999s
*.faultInjector.campaignTargetDepth = -1
*.faultInjector.strictDepthCampaign = false
```

Key settings:
- `repeat = 1` — shell loop handles repetitions (avoids conflict with global `repeat = 3`)
- `interval = 999s` — single-shot injection (fires once at t=5, never again within 30s sim-time)
- `strictDepthCampaign = false` for ER/Twitch — allows deterministic mode on non-CBT

### Layer 3: run_experiments.sh

Add `--fig2` flag. Mutually exclusive with `--mwe`, `--claim-a`, `--claim-b`.

```
FIG2_REPEATS=${FIG2_REPEATS:-10}

topologies=("CBT" "ER" "Twitch")
configs=("Fig2_CBT_FaultParent" "Fig2_ER_FaultParent" "Fig2_Twitch_FaultParent")
baselines=("BaselineCBT" "BaselineER" "BaselineTwitch")

for i in 0..2:
    topo = topologies[i]
    sim_root = $RESULT_DIR/fig2/$topo

    # Run baseline once
    run baseline config → $sim_root/baseline/

    # Read max_depth from baseline mwe_node_state.csv
    max_depth = $(uv run python -c "
        import pandas as pd
        df = pd.read_csv('$sim_root/baseline/mwe_node_state.csv')
        print(int(df['level'].max()))
    ")

    # Run per-depth fault experiments
    for depth in 1..max_depth:
        for rep in 0..FIG2_REPEATS-1:
            seed = base_seed + rep
            OMNETPP_RNGSEEDSET=$seed run fault config \
                --**.faultInjector.campaignTargetDepth=$depth \
                → $sim_root/depth_${depth}/rep_${rep}/
```

### Layer 4: build_fig2_outputs.py

Rewrite `build_topology_row()` → `build_topology_rows()`:

**Input:** Topology directory with `baseline/` and `depth_D/rep_R/` subdirectories.

**Logic per depth D:**
1. Load baseline `mwe_node_state.csv`, index by `node_id`
2. For each rep directory at this depth:
   - Load fault `mwe_node_state.csv`, index by `node_id`
   - Join on `node_id`
   - Filter to nodes where `level == D`
   - Find the node with max `abs(beta_fault - beta_base)` — the corrupted node
   - Compute its beta % increase, payment % increase, and service status
3. Average metrics across all reps for this depth
4. Return one row: `{topology, depth, avg_beta_pct_increase, avg_payment_pct_increase, user_fraction_receiving_service}`

**Output CSV (same columns, new row semantics):**
```
topology, depth, avg_beta_pct_increase, avg_payment_pct_increase, user_fraction_receiving_service
CBT, 1, 2.1, 1.8, 1.0
CBT, 2, 5.4, 4.9, 0.98
...
CBT, 10, 63.2, 58.7, 0.42
ER, 1, 1.3, 1.1, 1.0
...
```

### plot_fig2()

No changes needed. `sns.lineplot(x="depth", y=metric, hue="topology")` draws curves once there are multiple depth rows.

## Result Directory Structure

```
$RESULT_DIR/fig2/
  CBT/
    baseline/mwe_node_state.csv
    depth_1/rep_0/mwe_node_state.csv
    depth_1/rep_1/mwe_node_state.csv
    ...
    depth_10/rep_9/mwe_node_state.csv
  ER/
    baseline/mwe_node_state.csv
    depth_1/rep_0/mwe_node_state.csv
    ...
  Twitch/
    baseline/mwe_node_state.csv
    depth_1/rep_0/mwe_node_state.csv
    ...
  analysis.csv
  metrics_plot.png
```

## Error Handling

- Missing depth directory → skip that depth, warn on stderr
- Zero successful reps for a depth → skip, warn
- All topologies empty → raise ValueError
- No node at depth D with significant beta change → use the node at depth D with max abs delta, warn
- Non-CBT topology with no nodes at a given depth → skip that depth

## Edge Cases

- **Root node (depth 0):** Never corrupted. Depth loop starts at 1.
- **ER/Twitch depth distribution:** May be uneven. Some depths may have few or no nodes. Skip empty depths.
- **Repetition seeding:** Shell loop varies `OMNETPP_RNGSEEDSET` per rep. Config sets `repeat = 1` to avoid OMNeT++ internal repetition.
- **Baseline max_depth extraction:** Requires `mwe_node_state.csv` to exist after baseline run. The baseline simulation must complete before the depth loop starts (sequential in the shell script).

## Files Modified

| File | Change |
|---|---|
| `omnetpp/simulations/src/SCMFaultInjector.h` | Add `campaignTargetDepth` member |
| `omnetpp/simulations/src/SCMFaultInjector.cc` | Read new param, add single-depth targeting logic |
| `omnetpp/simulations/networks/SCMFaultInjector.ned` | Add `campaignTargetDepth` NED parameter |
| `omnetpp/simulations/omnetpp.ini` | Add Fig2 configs (3 fault + reuse existing baselines) |
| `scripts/run_experiments.sh` | Add `--fig2` flag with depth×rep loop |
| `scripts/analysis/build_fig2_outputs.py` | Rewrite to per-depth analysis from directory structure |

## What This Does NOT Change

- SCMNode (no algorithm changes)
- Figure 3 or Figure 5 scripts
- The default 12-config pipeline
- Docker, validate_outputs.py
- CSV column names (only row count changes)

## Configurable Parameters

| Parameter | Default | Paper | Env var |
|---|---|---|---|
| Repetitions per depth | 10 | 100 | `FIG2_REPEATS` |
| CBT node count | 1023 | 1023 | `CBT_NODES` |
| ER node count | 1023 | 1023 | `ER_NODES` |
| Twitch node count | 1023 | 1023 | via Twitch subsample |
| Sim time limit | 30s | — | via ini |
| Fault injection delay | 5s | — | via ini `initialDelay` |

## Estimated Run Time

- ~10 depths × 10 reps × 3 topologies = 300 simulation runs + 3 baselines
- Each run <1 second at 1023 nodes with 30s sim-time-limit
- Total: ~5 minutes (default 10 reps), ~50 minutes (paper's 100 reps)
- Well within DSN's 2-hour evaluation budget

## References

- Sinchan's overview doc: Claim 2 section
- Spec review: identified 3 critical corrections (maxCampaignDepth semantics, non-CBT support, repeated injection)
