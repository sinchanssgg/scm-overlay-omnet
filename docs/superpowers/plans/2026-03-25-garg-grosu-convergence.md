# Garg-Grosu Beta-Convergence Detection — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Garg-Grosu nodes emit `nodeStableTime` with a discrete round count when beta stabilizes, enabling Figure 5 to show convergence comparison.

**Architecture:** Add `prevBeta`, `ggConverged`, `roundCounter` fields to `SCMNode`. At end of each stabilization tick, Garg-Grosu nodes compare beta vs previous round. On convergence, emit signal once. Guard other emission paths to prevent unit mixing. Fix downstream script to read signal value instead of timestamp.

**Tech Stack:** C++ (OMNeT++ 6.x), Python 3 (pandas)

**Spec:** `docs/superpowers/specs/2026-03-25-garg-grosu-convergence-design.md`

**Branch:** `fix/garg-grosu-convergence`

**Status:** Implementation partially done. Commit `909e64d` has the initial fields + convergence block. Spec review found 3 additional fixes needed (Rule 5 guard, FAULTY reset, script value-vs-timestamp). Those fixes are written but uncommitted.

---

### Task 1: Verify SCMNode.h fields match spec

**Files:**
- Verify: `omnetpp/simulations/src/SCMNode.h:48-52`

- [ ] **Step 1: Confirm 3 new fields exist**

Check that `SCMNode.h` contains:
```cpp
// Garg-Grosu convergence detection (compare beta across consecutive rounds)
double prevBeta;
bool ggConverged;
int roundCounter;
```

Run: `grep -n "prevBeta\|ggConverged\|roundCounter" omnetpp/simulations/src/SCMNode.h`
Expected: 3 matches at lines 49-51

- [ ] **Step 2: Confirm no other header changes needed**

The spec says only 3 fields. No new methods, no new includes. Verify the header is complete.

---

### Task 2: Verify SCMNode.cc initialization matches spec

**Files:**
- Verify: `omnetpp/simulations/src/SCMNode.cc` (initialize function)

- [ ] **Step 1: Confirm initialization**

Check that `initialize()` contains:
```cpp
// Garg-Grosu convergence state
prevBeta = NAN;
ggConverged = false;
roundCounter = 0;
```

Run: `grep -n "prevBeta\|ggConverged\|roundCounter" omnetpp/simulations/src/SCMNode.cc`
Expected: initialization lines + convergence block lines + FAULTY reset lines

---

### Task 3: Verify convergence block at end of handleStabilization()

**Files:**
- Verify: `omnetpp/simulations/src/SCMNode.cc` (end of handleStabilization)

- [ ] **Step 1: Confirm convergence block exists and matches spec pseudocode**

Should contain:
```cpp
// --- Garg-Grosu convergence detection ---
roundCounter++;
if (algorithmKind == AlgorithmKind::GARG_GROSU &&
    status == STABLE && !ggConverged) {
    if (!std::isnan(prevBeta) && fabs(beta - prevBeta) < 1e-9) {
        ggConverged = true;
        emit(stabilizationTimeSignal, (double)roundCounter);
    }
}
prevBeta = beta;
```

---

### Task 4: Verify Rule 5 dual-emission guard (spec review fix)

**Files:**
- Modify: `omnetpp/simulations/src/SCMNode.cc:250-260`

- [ ] **Step 1: Confirm Rule 5 emission is guarded for Garg-Grosu**

The rejoin path (Rule 5) must NOT emit `stabilizationTimeSignal` for Garg-Grosu. Verify the code reads:
```cpp
// Garg-Grosu uses beta-convergence detection (below), not fault-recovery timing
if (algorithmKind != AlgorithmKind::GARG_GROSU && lastFaultTime > 0) {
    emit(stabilizationTimeSignal, (simTime() - lastFaultTime).dbl());
}
```

Run: `grep -A2 "GARG_GROSU.*lastFaultTime" omnetpp/simulations/src/SCMNode.cc`
Expected: the guard line above

---

### Task 4b: Verify handleMessage emission guard (plan review fix)

**Files:**
- Modify: `omnetpp/simulations/src/SCMNode.cc` (handleMessage, lines 95-98)

- [ ] **Step 1: Confirm handleMessage emission is guarded for Garg-Grosu**

The per-message emission also uses seconds, so must be skipped for Garg-Grosu:
```cpp
// Garg-Grosu uses round-count emission in handleStabilization() instead
if (algorithmKind != AlgorithmKind::GARG_GROSU &&
    status == STABLE && lastFaultTime > 0) {
    emit(stabilizationTimeSignal, (simTime() - lastFaultTime).dbl());
}
```

Run: `grep -B1 -A2 "round-count emission" omnetpp/simulations/src/SCMNode.cc`
Expected: the guard block above

---

### Task 5: Verify FAULTY state resets convergence (spec review fix)

**Files:**
- Modify: `omnetpp/simulations/src/SCMNode.cc` (Rule 3 block AND handleFaultNotification)

- [ ] **Step 1: Confirm convergence state reset in Rule 3**

When a node enters FAULTY, `ggConverged` and `prevBeta` must be reset. Verify:
```cpp
// Reset Garg-Grosu convergence so re-convergence is tracked after recovery
ggConverged = false;
prevBeta = NAN;
```

Run: `grep -B2 -A2 "Reset Garg-Grosu" omnetpp/simulations/src/SCMNode.cc`
Expected: TWO matches — one in Rule 3 block, one in handleFaultNotification

- [ ] **Step 2: Confirm convergence state reset in handleFaultNotification**

Nodes can also enter FAULTY via fault notification messages (not just Rule 3). Verify `handleFaultNotification()` also resets convergence state:
```cpp
// Reset Garg-Grosu convergence so re-convergence is tracked after recovery
ggConverged = false;
prevBeta = NAN;
```

Run: `grep -c "Reset Garg-Grosu" omnetpp/simulations/src/SCMNode.cc`
Expected: `2` (one in Rule 3, one in handleFaultNotification)

---

### Task 6: Verify build_fig5_outputs.py reads value not timestamp (spec review fix)

**Files:**
- Modify: `scripts/analysis/build_fig5_outputs.py:68-89`

- [ ] **Step 1: Confirm script reads signal value column**

The function `latest_stabilization_time()` must read `stable["value"].max()` (the emitted round count / seconds) NOT `stable["time"].max()` (the emission sim-timestamp).

Run: `grep "stable\[" scripts/analysis/build_fig5_outputs.py`
Expected: `values.append(float(stable["value"].max()))` — NOT `stable["time"]`

- [ ] **Step 2: Confirm function uses max not mean**

Run: `grep "return float" scripts/analysis/build_fig5_outputs.py`
Expected: `return float(max(values))` — NOT `sum(values) / len(values)`

---

### Task 7: Commit spec review fixes

**Files:**
- Stage: `omnetpp/simulations/src/SCMNode.cc`, `scripts/analysis/build_fig5_outputs.py`, `docs/superpowers/specs/2026-03-25-garg-grosu-convergence-design.md`, `docs/superpowers/plans/2026-03-25-garg-grosu-convergence.md`

- [ ] **Step 1: Stage all changes**

```bash
git add omnetpp/simulations/src/SCMNode.cc scripts/analysis/build_fig5_outputs.py docs/superpowers/specs/2026-03-25-garg-grosu-convergence-design.md docs/superpowers/plans/2026-03-25-garg-grosu-convergence.md
```

- [ ] **Step 2: Commit**

```
Fix spec review issues: Rule 5 emission guard, FAULTY reset, value-vs-timestamp

- Guard Rule 5 and handleMessage stabilizationTimeSignal emission to skip
  Garg-Grosu (prevents mixing seconds and round-count units on same signal)
- Reset ggConverged and prevBeta in both Rule 3 and handleFaultNotification
  when entering FAULTY state (enables re-convergence tracking after recovery)
- Fix build_fig5_outputs.py to read signal value (round count / seconds)
  instead of emission timestamp, and use max instead of mean
- Add design spec and implementation plan documents

Co-Authored-By: Claude Opus 4.6 (1M context) <noreply@anthropic.com>
```

---

### Task 8: Push and create PR

- [ ] **Step 1: Push branch**

```bash
git push -u origin fix/garg-grosu-convergence
```

- [ ] **Step 2: Create PR**

Title: `Implement Garg-Grosu beta-convergence detection for Figure 5`
Labels: `bug`
Body: include test plan with actionable commands (already drafted in previous session)

---

### Task 9: Verify end-to-end on test machine

- [ ] **Step 1: Run claim-b quick pipeline**

Note: run on the Docker test machine, not locally.

```bash
./scripts/run_experiments.sh --claim-b --quick
```

Expected: 3 configs complete — `ClaimA_CBT_FaultParent_D5`, `ClaimA_CBT_FaultParent_D5_GargStub`, `ClaimA_CBT_FaultParent_D5_ByrenheidStub` (the `--claim-b --quick` combination runs these 3 per `run_experiments.sh` lines 199-204)

- [ ] **Step 2: Check GargStub .vec files have nodeStableTime**

```bash
grep "nodeStableTime" results/*/claim-b/ClaimA_CBT_FaultParent_D5_GargStub/*.vec
```

Expected: Multiple lines with nodeStableTime data (previously zero)

- [ ] **Step 3: Run Figure 5 analysis**

```bash
uv run python scripts/analysis/build_fig5_outputs.py /tmp/fig5-test --result-root results/*/claim-b
cat /tmp/fig5-test/analysis.csv
```

Expected: `rounds_to_converge` column has numeric values for Garg-Grosu (not NaN)

- [ ] **Step 4: Docker smoke test**

```bash
cd docker && docker compose up --build
```

Expected: Pipeline completes, results produced
