# 13 - Implement supported cable diagnostics

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-12.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Revisit peer long-operation deadlines, cancellation/result ownership and
fixture-specific Python verdicts; inspect TMP1x2 and TCA9548A examples where external
state cannot be inferred.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen
`TMP1x2/src/TMP1x2Operations.cpp::poll`, `finishJob`, `takeResult` and PCA9555
native timeout/cancel/cleanup assertions. Read fixture/result rules in
`TMP1x2/tools/hil_tmp1x2_runner.py` and parser tests, plus
`24Cxx/tools/test_hil_identity.py`: host configuration cannot substitute for
observed firmware/device evidence. Adopt correlation, bounds and invalid-result
handling; memory/sensor conversion formulas have no cable-diagnostic authority.

Extend the diagnostics source and shared operation/result mechanism from 12.
Use compact exact-part cable config/results with raw observations and validity,
named/frozen in 04. A pure conversion helper needs an authoritative formula and
current caller. No TDR service, extra scheduler, new result-consumption API or
always-successful unavailable-feature implementation.

## Decisions and independent test oracles

1. Reconcile all SNLS647I cable rows: manual start, hardware autorun on link down,
   A/B transmit/capture, same/cross-pair evidence, segments, averaging, DC removal
   and documented threshold/pulse/shadow controls. TDR terminology does not
   authorize copying SNLA253 DP83822 or SNLA330A DP83825I sequences. Unresolved
   distance calibration does not excuse missing documented raw controls/results.
2. Separate finite manual acquisition from explicit silicon autorun configuration.
   Autorun is not an undocumented software recovery action. Define acquisition
   through the one collector, interaction with manual sessions and evidence when
   another hardware event arrives. No independent diagnostic polling owner.
3. Validate part/mode, peer/link conditions, maintenance and complete settings
   before I/O. Derive waits, done/fail, clear/freeze from exact source. Preserve
   raw evidence and gate unresolved interpretations individually; uncertainty
   cannot become a convenient decoded success or unsupported-silicon claim.
4. Retain documented peak position/amplitude/polarity/cross evidence. Derived
   fault/distance needs supported interpretation, units/range and calibration.
   Unknown/saturated data cannot mean zero metres or good cable. A requested
   fixture label is not proof the wire/termination actually has that condition.
5. Start/wait/acquire/stop/restore use the existing bounded owner engine. Record
   partial validity, accepted effects and primary/cleanup Status. Cancel itself
   performs no MDIO; later poll completes admitted finite cleanup or leaves
   unresolved effects visible. Never hide failed restoration in a cable verdict.
6. Write an exact fixture recipe for known-good/open/short/length references,
   termination, peer disconnects, calibration limits and manual interventions.
   Specify future runner inputs and inconclusive cases; writing a plan neither
   operates the fixture nor establishes its availability.

Derive literal traces/raw decoding vectors independently, including extrema,
invalid settings and all documented capture combinations. Test done, failed,
never-done, missing/corrupt peaks, saturation, stale autorun data, cancel and each
acquire/restore failure. Check sentinel outputs and validity so a partial result
cannot become PASS or measured distance. Test status-collector contention,
already-active diagnostics, total transfer bounds and exactly-once results.
Source-proved absent capability gets zero-I/O rejection tests; unresolved source
interpretation stays an open gate without removing documented functionality.

## Documentation shipped in this stage

Doxygen raw/derived fields with units, validity/source, manual/autorun behavior
and operation bounds. Extend the diagnostics guide and fixture plan with
calibration, unknown results and failed-cleanup examples. Update API contracts,
README, CHANGELOG, ledger and validation. Runner cases remain planned until
implemented; physical accuracy is NOT RUN/BLOCKED until measured against known
cables. Compile the API example and run Doxygen/warning/link checks. Keep the
execution branch and coherent-unit/per-prompt sync; no first-set remote CI here.

## Work

Determine the exact DP83826I-supported cable diagnostic mechanism and its required
link/mode conditions from the datasheet and applicable TI notes. Implement documented
behavior; do not copy DP83822 TDR register sequences into DP83826I by family name. If
evidence rules out a capability, return unsupported and document the source rather than
create a stub.

Model start, progress, completion, timeout, cancellation and restoration with bounded
owner-driven work. Do not run cable tests on an active EtherCAT session or automatically
during recovery. Preserve configuration uncertainty if test cancellation or cleanup
cannot restore it.

Represent open/short/normal/unknown and estimated distance only where the device
actually reports them. Include units, range, quantization, calibration/fixture limits
and invalid/inconclusive conditions. No fabricated numeric cable length from unspecified
counter math.

Add a fixture procedure covering known-good/open/short/length cases and required peer
disconnects. Map it to a future runner suite while keeping physical manipulations
explicit. Coordinate destructive reads with the diagnostic collector. Update
capabilities and docs to distinguish proven protocol implementation from unperformed
physical qualification.

## Acceptance and handoff

Use independent literal vectors and simulated documented outcomes, unsupported
mode/variant rejection, timeout and every partial failure/cleanup boundary. Check
invalid results cannot be rendered as a measured length or passed cable test. If silicon
evidence is unresolved, block only that capability and retain a visible completion
issue.

Expected output: Exact supported cable diagnostics and a reproducible fixture plan.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
