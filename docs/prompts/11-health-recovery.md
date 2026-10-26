# 11 - Complete health, invalidation and recovery primitives

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-10.

## Fresh audit and owned work

Compare INA228, SHT3x, PCA9555 and TCA9548A actual health accounting; read TMP1x2
config-trust implementation and FieldCore lifecycle/presence/health separation.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

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
