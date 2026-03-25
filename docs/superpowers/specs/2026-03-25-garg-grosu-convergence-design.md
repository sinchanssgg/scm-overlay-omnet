# Garg-Grosu Beta-Convergence Detection — Design Spec

## Problem

Figure 5 in the paper compares convergence time between SCM, Garg-Grosu, and Byrenheid algorithms. The simulation currently produces no convergence data for Garg-Grosu because the algorithm variant never emits the `nodeStableTime` signal. `build_fig5_outputs.py` returns NaN for Garg-Grosu scenarios, leaving Figure 5 incomplete.

## Root Cause

SCM detects faults via `notLocallyConsistent()` and emits `nodeStableTime` when a node recovers from FAULTY to STABLE. Garg-Grosu's `notLocallyConsistent()` is hardcoded to `return false` for structural consistency (it only checks level, not beta), so nodes never go FAULTY from inconsistency detection, `lastFaultTime` is never set, and the signal is never emitted.

## Sinchan's Clarification (from issue #55)

Four key responses from the paper's author:

1. **What variables define convergence?**
   > "It only checks the Beta value for comparison in two consecutive rounds, i.e. the payment that needs to be made by that node. None of the other variables are checked."

2. **Is convergence local or global?**
   > "Convergence detection is local for a node. When a node x checks that Beta is consistent (same) for two consecutive rounds, x decides that it has converged. But, the convergence may not be global for other nodes. Other nodes will converge at their own pace, and the global convergence is achieved only when all nodes locally decide convergence."

3. **What unit does Figure 5 measure?**
   > "The y-axis measures the number of discrete rounds until Beta stabilizes. It is not a wall-clock simulation because the algorithm is distributed and asynchronous."

4. **Are SCM and Garg-Grosu convergence times comparable on the same axis?**
   > "Those two measurements mean the same to us for the purpose of experiments. We are comparing two algorithms: one that is non self-stabilizing and one that is self-stabilizing. Hence, we have to make some suitable assumptions."

## Design

### New State Variables

Three new member variables in `SCMNode`:

| Variable | Type | Purpose |
|----------|------|---------|
| `prevBeta` | `double` | Beta value from the previous round, for comparison |
| `ggConverged` | `bool` | Whether this node has locally declared convergence (emit signal only once) |
| `roundCounter` | `int` | Discrete round count (incremented each `handleStabilization()` tick) |

### Initialization

In `SCMNode::initialize()`:
- `prevBeta = NAN` (no previous value on first round)
- `ggConverged = false`
- `roundCounter = 0`

### Convergence Check

Added at the end of `handleStabilization()`, after all 7 rules have executed:

```
roundCounter++
if (GARG_GROSU && STABLE && !ggConverged):
    if (prevBeta is not NAN && |beta - prevBeta| < 1e-9):
        ggConverged = true
        emit(stabilizationTimeSignal, roundCounter)
prevBeta = beta
```

Key design decisions:
- **End of tick, not beginning** — ensures beta has been recalculated by Rules 2-5 before comparison
- **Epsilon 1e-9** — tighter than the 1e-6 used in `notLocallyConsistent()` because convergence compares the same computation's output across rounds (should be near-identical), while consistency checks compare against a parent's independently computed value
- **Emit once per node** — `ggConverged` flag prevents re-emitting every subsequent tick. Flag is **reset to false** when the node enters FAULTY state (via `lostStableSupport()`), so re-convergence after recovery is tracked correctly
- **NAN check on first round** — `prevBeta` starts as NAN, so the first round always sets `prevBeta = beta` without triggering convergence
- **All algorithm variants increment roundCounter** — but only Garg-Grosu uses it for emission. This is harmless overhead (one integer increment per tick) and avoids conditional logic

### Signal Semantics

| Algorithm | Signal value | Unit |
|-----------|-------------|------|
| SCM | `simTime() - lastFaultTime` | Seconds (simulated wall-clock duration from fault to recovery) |
| Garg-Grosu | `roundCounter` | Discrete rounds (ticks until beta stabilizes) |
| Byrenheid | `simTime() - lastFaultTime` | Same as SCM |

Since each tick is 1 simulated second (`scheduleAt(simTime() + 1.0, msg)`), rounds and seconds are numerically close on the same axis (offset by initial jitter of `uniform(0, 0.1)`). This matches Sinchan's statement that the measurements "mean the same to us for the purpose of experiments."

**Dual-emission guard**: Garg-Grosu nodes can also enter FAULTY via `lostStableSupport()` and rejoin via Rule 5 (line 257). The Rule 5 rejoin emission is **skipped** for Garg-Grosu to prevent mixing seconds (rejoin path) with rounds (convergence path) on the same signal.

### Downstream Impact

- **`build_fig5_outputs.py`**: Changed to read signal **value** (`stable["value"].max()`) instead of emission timestamp (`stable["time"].max()`). The value carries the semantically correct metric: round count for Garg-Grosu, seconds for SCM. Also changed from mean to max across `.vec` files (global convergence = last node to converge).
- **`process_results.py`**: No changes needed. It aggregates all signals generically.
- **`plot_metrics.py`**: No changes needed. It does not consume Figure 5 data.
- **`validate_outputs.py`**: No changes needed. It checks column presence, not values.

### Edge Cases

- **Root node** (id=0): Beta is always 0. Converges trivially at round 2 (first round `prevBeta=NAN`, second round `prevBeta=0, beta=0`). This is correct — the root has no cost to share.
- **Beta oscillation**: If a node's beta oscillates between two values and never settles, it never declares convergence and never emits the signal. `build_fig5_outputs.py` handles this gracefully via the NaN fallback.
- **Re-convergence after fault**: `ggConverged` and `prevBeta` are reset when entering FAULTY state, so the node re-tracks convergence from scratch after recovery.

### Files Modified

| File | Change |
|------|--------|
| `omnetpp/simulations/src/SCMNode.h` | Add 3 member variables (`prevBeta`, `ggConverged`, `roundCounter`) |
| `omnetpp/simulations/src/SCMNode.cc` | Initialize 3 variables in `initialize()`, add convergence block at end of `handleStabilization()`, guard Rule 5 emission for GG, reset convergence state on FAULTY transition |
| `scripts/analysis/build_fig5_outputs.py` | Read signal value instead of timestamp, use max instead of mean |
| `.gitignore` | Add `/.claude/` (unrelated cleanup, bundled for convenience) |

### What This Does NOT Change

- SCM convergence behavior (unchanged — uses fault detection + recovery)
- Byrenheid convergence behavior (unchanged — uses SCM's mechanism)
- Any Python analysis scripts
- Any NED files or omnetpp.ini configs
- The NaN fallback in `build_fig5_outputs.py` (still useful as a safety net for edge cases)

## Alternatives Considered

**A. Track only `prevBeta` with no round counter** — Would work but would emit `simTime()` instead of discrete rounds, mismatching the paper's Figure 5 y-axis definition.

**B. Generalized convergence checker per algorithm** — Over-engineered. Byrenheid uses SCM's mechanism, so only Garg-Grosu needs special handling. One `if` block is simpler than an abstraction.

## References

- GitHub issue: #55
- PR comment with Sinchan's convergence definition: PR #39
