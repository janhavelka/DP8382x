# 08 - Implement lifecycle, reset and bounded restoration

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-07.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Inspect TMP1x2/MB85RC lifecycle/cancellation, PCA9555 primary-vs-cleanup error
precedence and TCA9548A bus-silent end. Reuse semantic patterns, not their reset
procedures.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen
`TMP1x2/include/TMP1x2/TMP1x2.h::OperationResult`, `OperationSnapshot`,
`PollResult` and `src/TMP1x2Operations.cpp::admit`, `poll`, `finishJob`, `cancel`,
`takeResult`; compare PCA9555 owner implementation and tests
`test_input_cancel_runs_required_pointer_park_before_terminal_result`,
`test_detach_refuses_to_abandon_required_pointer_cleanup` and
`test_operation_callback_timeouts_share_remaining_deadline_budget` in
`PCA9555/test/test_basic.cpp`. Reopen `24Cxx/src/EEPROM24Cxx.cpp::cancel`,
`takeResult`, `poll` for retained effects across a device wait. Reject EEPROM
write-cycle/ACK-poll rules and TMP1x2 conversion timings for PHY reset.

Extend the existing driver and one operation slot, in a justified
`src/DP8382xOperations.cpp` if needed. Use one phase switch and small named
procedures shared by convenience/cooperative entry points. No generic scheduler
or duplicate reset code in the future IDF adapter. Planned names are `bind`,
`end`, `startInitialize`, `startReset`, `pollOperation`, `takeResult`, `cancel`
and `getOperationSnapshot`; stage 04 freezes signatures and admission semantics.
Do not copy all begin/init/startInit/pollJob/tick compatibility aliases.

## Decisions and independent test oracles

1. Make a reset-effects table: trigger, minimum wait, completion evidence,
   retained/invalidated fields, strap sampling, identity/mode effects and restore
   responsibility. Separate BMCR PCS reset, PHYRCR reset/restart and external
   reset/POR. Use exact document terminology where hard reset is ambiguous.
2. Admission validates binding/state, token/result availability, time range and
   reset kind bus-silently. Use 04's chosen acceptance Status; acceptance never
   means reset complete. Pending results cannot be overwritten by a new request.
3. Bound each phase/wait. Anchor minimum silicon waits after callback completion;
   with caller-only poll time use a conservative documented anchor. Account for
   quantization, wrap limits and frozen-clock transfer bounds. Poll returns to
   the owner instead of spinning until the chip clears a bit.
4. Accepted reset invalidates appropriate link/configuration/identity evidence
   even when completion polling fails. Restore validated owned desired settings
   for implemented behavior; their current hardware image need not be trusted.
   Follow the reset-specific order; compare stable fields separately from command
   and self-clearing status. Later stages extend this same restore procedure.
5. Specify cancel before transfer/after reset write, wait timeout, cleanup
   admitted/denied and teardown with unresolved effects. Cleanup has the finite
   budget fixed in 04 and cannot replace the primary error. Local end cannot
   claim hardware stopped or erase unresolved effects. Context outlives callbacks
   and adapter users; external-reset notification performs no pin operation.

Create independent literal traces per reset kind and time tables for exact
expiry, wrap, no clock, callback overrun and never-clearing reset. Fail each write,
completion read, restore and readback phase. Test zero poll budget, cancellation
at every effect boundary, stale/wrong token, repeated take/end, pending-result
rebind and reuse after consumption. Assert initialize succeeds with cable down.
Small poll budgets cannot split MMD restoration transactions or create a second
polling owner. Check primary and cleanup Status/detail independently.

## Documentation shipped in this stage

Doxygen lifecycle/operation methods and fields with states, ownership, per-poll
bound, units, deadline origin, token lifetime, consumption and cleanup. Add a
compiling portable owner loop, reset-effects table and failed-cleanup example
to the lifecycle guide. Reconcile architecture/API contracts, README, CHANGELOG,
coverage and validation. Run Doxygen generation/warning/link checks now. Keep
one execution branch and coherent-unit/per-prompt commit+sync; this first set
does not dispatch/poll/wait for remote CI here.

## Work

Implement bind/initialize/end and the minimal cooperative operation mechanism specified
at stage 04. Keep local bind/end/cancel/result access bus-silent and document any
explicit live compatibility convenience. A pending callback or IDF user cannot outlive
its context. No heap, platform delay or internal task in core.

Implement the distinct BMCR PCS reset, PHYRCR hard reset, PHYRCR restart and externally
signaled hardware-reset effects according to the evidence. Hardware GPIO/power
sequencing stays with the application. Record what is invalidated, retained and
resampled for each kind, including POR-only straps. Do not use the vendor source's bit
mask as a register address.

Bound reset completion using aggregate deadlines and caller scheduling; no full-timeout
refresh per read. Cancellation reports effects already accepted. Reset/reinitialization
restores validated owned desired configuration with source-defined waits and readback
that re-establishes trust. Current hardware uncertainty must not prevent explicit
recovery from a valid desired profile. At this stage restore only functionality
implemented so far; later config/features extend
the same path.

Retain terminal operation evidence until consumed, reject conflicting work and preserve
primary failure if cleanup also fails. A bus-silent cancellation request cannot itself
restore registers: implement the stage 04 follow-up polling/cleanup-budget contract and
report remaining effects if cleanup is abandoned or impossible. Initialization success
must not require an Ethernet cable unless the caller explicitly requested link waiting.
Reset success is not proof that link, configuration or electrical timing is qualified.

## Acceptance and handoff

Test reset-kind differences, sticky bits, never-completing reset, clock boundary cases,
failures before/after writes, cancellation and repeated teardown/rebind. Verify no-I/O
accessors and active-result protection. Test a reset when a second owner attempts
indirect access. Add independent literal sequences and bounded call-count assertions.

Expected output: Usable lifecycle/reset operations with explicit effects and time
bounds. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
