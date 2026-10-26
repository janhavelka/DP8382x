# 10 - Implement link snapshots and change evidence

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-09.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Re-audit passive cached snapshots/freshness in INA228, SHT3x and TMP1x2; inspect
FieldCore's copyStatus without I/O. Do not substitute sensor validity for link
negotiation semantics.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Inspect
`TMP1x2/src/TMP1x2Configuration.cpp::readSnapshot`: staged output and CONFIG
bookends do not claim an atomic silicon snapshot. Reopen
`TMP1x2/include/TMP1x2/TMP1x2.h::RegisterSnapshot`, `SettingsSnapshot`,
`INA228/include/INA228/INA228.h::SettingsSnapshot`, `DiagAlert` and actual
`readDiagAlert` implementation for destructive-status behavior. Inspect CLI
rendering and native tests; cached display must not trigger a second read.
Peer validity/time naming is useful; sensor freshness is not PHY negotiation.

Use planned `readLinkStatus` for live work and `linkSnapshot` for passive
`LinkSnapshot` copies. Stage 04 fixes signatures/fields. Keep observation and
event consumption in the existing driver or one justified status source, such
as `src/DP8382xStatus.cpp`, not a class per event. The collector stores evidence
for future IDF/CLI consumers. No notification scheduler, SDK callback or logging
belongs in the core; the approved operation owner drives acquisition.

## Decisions and independent test oracles

1. Write a read plan with current/latch-low/read-clear/counter semantics, ordering
   and validity dependencies per register. Separate current link from consumed
   historical loss evidence. Define safe-dump reads and explicitly destructive
   acquisitions; do not make multiple independent consumers read the same latch.
2. Specify current/stale/unknown, link, forced/negotiated mode, speed/duplex,
   partner and resolved pause evidence with timestamp validity. Use `timestampMs`
   and `timestampValid` if selected in 04. Numeric defaults never imply a valid
   speed. Old values after failure retain old time and explicit stale/invalid
   state, not a refreshed timestamp claiming a successful observation.
3. Resolve from exact source, retaining raw evidence when forced-mode PHYSTS
   interpretation is gated. Pause resolution needs both advertisements and valid
   full-duplex negotiation; forced/half-duplex evidence cannot imply negotiated
   pause. Gate only unresolved claims, without dropping documented raw evidence.
4. Define consistency for a link transition during multi-register reads. Any
   bounded resample has an explicit maximum; otherwise publish uncertainty.
   Never retry until the fake stabilizes. Atomic software publication does not
   mean all registers were atomically sampled in silicon.
5. Define first-valid, changed, repeated, stale-failure and regained-validity
   evidence for stage 15 notifications. One collector serves polling and IRQ;
   one owner notifies the MAC. Specify event retention/consumption without an
   allocated event framework. Application policy does not modify PHY facts.

Script latch-low/current-up/down, transitions between every read, negotiation
pending/complete and forced/invalid resolution. Independently enumerate pause
truth-table cases rather than invoking production resolution for expected data.
Fail every read and assert no new fully valid snapshot. Test timestamp zero
versus unavailable time, age boundaries, stable repetitions, reset invalidation
and two cached readers. Cable-down reads count successful MDIO, not transport
failures. Getters/rendering do no I/O. A valid 10/half link may fail application
100/full policy while core health remains truthful.

## Documentation shipped in this stage

Doxygen snapshot fields and methods with units, validity groups, read effects,
update/notification ownership and maximum transaction count. Add a guide table
for down, negotiating, resolved, stale, MDIO failure and policy mismatch plus a
compiling cached-status example. Update README, CHANGELOG, API contracts and
coverage. Run Doxygen generation/warning/link checks and separate physical
partner qualification from host sequences. Keep the execution branch and
coherent-unit/per-prompt sync; this first set uses no remote CI here.

## Work

Implement explicit live link observations and passive cached LinkSnapshot. Handle
latch-low/current status using the exact required read sequence. Distinguish link down,
negotiation incomplete, resolved link, unknown/stale data and management failure.
Preserve old values with invalidity/age rather than publish partially read fields as
current.

Determine speed/duplex and partner/advertisement evidence according to mode and
documented source. Include resolved RX/TX pause evidence from the applicable
ANAR/ANLPAR/PHYCR semantics, valid only under documented full-duplex negotiation
conditions; don't infer it in half-duplex, forced or incomplete modes. Resolve or gate
the PHYSTS forced-mode ambiguity; don't choose a convenient bit interpretation. Define
what makes a snapshot coherent enough and expose uncertainty during an unstable
multi-register read rather than invent atomic silicon sampling.

Deliver change evidence for the adapter without duplicate notifications or polling. In
portable mode the application drives observation. In IDF mode stage 15 uses the approved
base/adapter path; there must be one collector and one MAC-notification owner. A cable
reconnection is not permission for core recovery policy.

Keep application policy evaluation separate: a valid link can fail a 100/full
requirement. IP address, DHCP, internet, EtherCAT state and servo availability are not
LinkSnapshot fields owned by the PHY. Document freshness and timestamps without implicit
platform clocks.

## Acceptance and handoff

Test link up/down/flapping, latched history vs current status, negotiation transitions,
forced modes, symmetric/asymmetric/no-pause partner combinations and half-duplex
invalidity, failures between reads, stale snapshots and changed/repeated notifications.
Assert cached views do no I/O and link-down does not increase MDIO failure counters.
Test a valid but application-incompatible link.

Expected output: Truthful passive/live link observations and adapter-ready change
evidence. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
