# 13 - Implement supported cable diagnostics

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-12.

## Fresh audit and owned work

Revisit peer long-operation deadlines, cancellation/result ownership and
fixture-specific Python verdicts; inspect TMP1x2 and TCA9548A examples where external
state cannot be inferred.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

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
