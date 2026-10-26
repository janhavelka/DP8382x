# 11 - Complete health, invalidation and recovery primitives

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-10.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Compare INA228, SHT3x, PCA9555 and TCA9548A actual health accounting; read TMP1x2
config-trust implementation and FieldCore lifecycle/presence/health separation.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen
`TMP1x2/src/TMP1x2.cpp::track`, `healthSnapshot`, `invalidateDeviceState` and
`TMP1x2/include/TMP1x2/TMP1x2.h::HealthSnapshot`; compare
`PCA9555/src/PCA9555.cpp::_updateHealth`, `_i2cWriteCleanupTracked` and
`test_input_read_failures_survive_successful_cleanup_in_health_and_validity`
in its tests. Inspect INA228's actual health-policy branches rather than assume
its OFFLINE admission policy matches TMP1x2. 24Cxx expected busy-poll accounting
illustrates progress versus transport failure; it supplies no MDIO NACK rule.

Use `DriverState::{UNINIT, READY, DEGRADED, OFFLINE}`, `healthSnapshot` and
`driverState` from the planned baseline. Prefer TMP1x2 field names `initialized`,
`consecutiveFailures`, `totalFailures`, `totalSuccess`, `lastOkMs`, `lastErrorMs`,
`lastOkTimeValid`, `lastErrorTimeValid`, `lastError`; 04 fixes widths/thresholds.
Do not add equivalent getHealth/state/healthState/online APIs. READY expresses
documented driver health, never link/IP readiness. PCA9555's actual three-state
model is a deliberate difference, not evidence all siblings share four states.

Accounting stays at 06's transport boundary; projection extends existing status
and lifecycle sources. `startRecover` shares reset/configuration verification.
No recovery manager, hidden retry timer or autonomous application policy.

## Decisions and independent test oracles

1. Make a transition/accounting table for bind, initialize, local rejection,
   tracked success/failure, raw/probe diagnostics, cleanup, cancel, reset,
   invalidation and end. Define initialized separately from health. OFFLINE is
   passive evidence; it must not silently suppress owner-requested MDIO.
2. Define counting units. Several successful frames followed by one failure in
   an indirect operation must not produce contradictory logical outcomes. Keep
   all attempted frames and transactions observable separately from managed
   health; local BUSY/IN_PROGRESS and no link are not transport faults.
3. Adapt the PCA9555 cleanup lesson: successful cleanup must not hide the primary
   failure by clearing consecutiveFailures, while cleanup failure remains visible.
   All-source activity still records actual cleanup frames. Document managed
   attribution explicitly rather than copying I2C wrapper names/counting units.
4. Preserve last error and saturate counts. If 04 selected resetStatistics,
   counter reset must not erase active failures, uncertain configuration, pending
   result or timestamp evidence. Zero is a valid time; absence has a validity flag.
5. Reconcile stable owned bits with explicit proof. Unrelated read/raw success
   cannot restore trust. Raw write, external writer/reset and ambiguous managed
   writes invalidate appropriate knowledge, without automatically being bus faults.
6. Recovery is an admitted bounded request through existing verify/reapply/reset
   procedures. Choose required steps from source/current evidence, preserving
   partial effects and cleanup errors. Cable reconnect does not schedule recovery
   or select forced speed. Application scheduling/retries remain outside core.

Table-driven sequences cover every transition/threshold, literal multi-frame
counts, saturation at each actual width, timestamp zero/missing/wrap and retained
last error after success. Test primary failure plus successful/failed cleanup,
two instances, zero-I/O views and local-invalid input. Link down, negotiating
and policy mismatch change no transport-failure counters. Recover after raw
write and external reset; fail every verification/apply phase and prove that
successful MDIO alone does not validate the desired configuration.

## Documentation shipped in this stage

Doxygen each health field/state/counter/getter and recovery method with counting
unit, saturation, validity, passive behavior and error retention. Maintain the
transition table and a compiling bounded recovery example in the health/config
guide. Explain mapping to application readiness without hidden I/O. Update
README, CHANGELOG, API contracts, ledger and validation; run Doxygen/link checks.
Use the same branch and coherent-unit/per-prompt commit+sync. Remote CI dispatch,
polling and waiting stay at 29 for the first set.

## Work

Implement passive HealthSnapshot over the transport/effect accounting established
earlier. Use the chosen four-state vocabulary with defined transitions, saturating
counters, consecutive failures, last-success and last-error validity. Document any
difference from peers. Do not erase the last error on unrelated success or turn raw
diagnostic traffic into managed recovery.

Keep counters for logical operations, MDIO frames, driver health and chip link/error
events separately named. Count actual attempted work once at its true boundary; local
errors, no-link observations and pending negotiation are not transport failures. Define
how skipped/offline admission is reported without hidden blocking policy.

Implement explicit invalidation after raw writes, external resets/writers and uncertain
application. A healthy management read cannot automatically revalidate desired
configuration. Reconciliation reads stable owned bits and restores evidence only when
proved. End/reset/cancel semantics preserve hardware effects that may still matter.

Provide caller-invoked recover/reinitialize primitives using the existing bounded
procedures. No retry loop, backoff task, automatic power cycle, machine recovery or
forced link policy. Examples/consumer owners decide when recovery is allowed. Expose
operation/result state so applications can schedule it fairly and map it to their own
health contract.

## Acceptance and handoff

Test all health transitions, counter saturation, invalid timestamps, diagnostic vs
managed accounting, no-I/O getters, raw/external invalidation, partial apply and
recovery cleanup failure. Cable disconnect must not increment transport-failure or
consecutive-failure counters; actual successful MDIO reads still increment their
success/activity counters. Rejected local/CLI input changes no transport counters. Test
two instances without cross-contamination.

Expected output: Passive health and trustworthy configuration/recovery evidence. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
