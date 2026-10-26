# 07 - Implement identity and supported-device selection

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-06.

## Fresh audit and owned work

Re-audit identity/capability validation in TMP1x2, MB85RC and relevant TUSB32x
implemented validators; distinguish declared model from observed hardware identity.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Implement DP83826I identification using exact PHYID fields and mode behavior. Preserve
raw IDs, decoded revision and observed mode. Match only the documented supported
identities with explicit policy for unknown revisions. Device ordering
suffix/temperature grade may require caller declaration; do not infer them from shared
IDs.

Separate a bounded explicit read-only probe/discovery from normal initialization. A
valid management response, all-zero/all-one read and unsupported PHY must have distinct
outcomes where evidence allows. Never use the generic base's ID1-only discovery as proof
of correct DP83826I. No automatic address scanning or successful binding to the first
responding device.

Expose capability queries based on the selected supported exact part and mode.
Unsupported features fail before I/O. Future family members remain documented and
rejected until their implementation stages, not enabled by accepting a broad OUI/model
mask.

Document detection's transport/health attribution and configuration effects. Binding can
remain bus-silent while probe/initialize are explicitly live. Multiple instances and PHY
addresses share the application bus under the transaction contract.

## Acceptance and handoff

Test enhanced/basic identities, masked revision fields, wrong OUI/model, unknown
revision policy, partial ID read, all-zero/all-one responses, ambiguous declared subtype
and wrong requested address. Assert unsupported capability checks are bus-silent and
preserve outputs. Cross-check literal IDs against the PDF.

Expected output: Exact DP83826I identity and honest capability evidence. Include source
documentation, relevant guides, feature/evidence matrix and progress updates. Apply
COMMON's completion rules; report actual commands, results and remaining gates. Continue
only when the concrete required inputs exist; COMMON explains how unrelated gates stay
open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
