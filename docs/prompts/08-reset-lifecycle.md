# 08 - Implement lifecycle, reset and bounded restoration

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-07.

## Fresh audit and owned work

Inspect TMP1x2/MB85RC lifecycle/cancellation, PCA9555 primary-vs-cleanup error
precedence and TCA9548A bus-silent end. Reuse semantic patterns, not their reset
procedures.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

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
restores only verified owned configuration with source-defined waits and readback. At
this stage restore only functionality implemented so far; later config/features extend
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
