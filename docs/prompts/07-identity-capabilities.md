# 07 - Implement identity and supported-device selection

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-06.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Re-audit identity/capability validation in TMP1x2, MB85RC and relevant TUSB32x
implemented validators; distinguish declared model from observed hardware identity.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen
`MB85RC/src/MB85RC.cpp::_selectVariant`, `_validateActiveDeviceId`,
`_readDeviceIdBytesRaw` and `include/MB85RC/CommandTable.h` variant lookup.
Compare TMP1x2's declared-model validation and `probe()`: its configuration
plausibility check is explicitly not chip identification. Read
`PCA9555/test/test_basic.cpp::test_por_default_check_is_explicit_and_non_identity`
and `tools/test_run_i2c_hil_parser.py::test_address_scan_is_not_identity_proof`.
Adopt separation of response, observed identity and declared part; neither
sibling's presence protocol supplies an MDIO identity oracle.

Extend existing driver/config/capability values and the register source from 06;
split device selection only if the actual decoder/table warrants it. Keep raw ID
words, decoded model/revision and mode evidence with explicit validity. Do not
add future-variant factories, automatic discovery services or a registry. One
exact-part selection routine serves initialization, probe and later adapters.

Use the stage-04 identity/capability names. If missing, propose `DeviceIdentity`,
`readIdentity` and the passive capability query in `docs/api-contracts.md`, then
reconcile declarations and consumers before coding. Probe is live read-only MDIO;
`bind` and cached capability access are passive. A declaration is not observation.

## Decisions and independent test oracles

1. Reopen SNLS647I PHYID, BASIC/ENHANCED mode and strap sections and the stage-02
   conflict record. Record literal masks/IDs, revision policy and fields that
   cannot distinguish ordering suffix/temperature grade. Neither broad OUI match
   nor a vendor driver's literal revision check establishes exact support.
2. Decode into temporary storage and publish only a complete valid identity.
   Define partial diagnostic evidence after ID1 succeeds and ID2 fails without
   presenting it as usable identity. Classify all-zero/all-one responses using
   actual controller evidence, not guessed absence/ACK behavior. Preserve the
   transport Status and detail on failed reads. Use `DEVICE_MISMATCH` for a
   successfully read wrong part or unaccepted revision, with its code-specific
   reason. `DEVICE_NOT_FOUND` means no accepted/plausible identity under the
   documented rule, not proof of physical absence. `MDIO_ERROR`/`MDIO_TIMEOUT`
   describe transport failure; do not replace these with a guessed identity error.
3. Tie capabilities to exact supported part and valid mode. Distinguish silicon
   support, implemented API and board/fixture availability. Later family entries
   remain rejected until their stages; mismatch never changes address silently.
4. Specify identity invalidation after reset/rebind/external notification. A mode
   change that changes identity must update capability selection, not leave stale
   permissions. Reject contradictory part declaration before feature writes.
5. If 04 requires discovery now, accept an explicit finite range/address set and
   budget without implicit selection/binding. Otherwise deliver the exact-address
   probe and leave scanning with its owning later stage; avoid scope scaffolding.

Test literal BASIC/ENHANCED IDs, revision boundaries, wrong OUI/model, documented
aliases, unimplemented relatives and ambiguous subtype declarations. Include both
read failures with detail, sentinel outputs, zero/all-one data, stale identity
after rebind, changed mode and two instances. Local invalid input causes no
callbacks; unsupported capability causes no feature writes. Successful identity
must not mark link up, configuration verified or hardware qualified.
Assert the distinct mismatch/not-found/transport codes and reasons, not merely
that all negative cases return a non-OK Status.

## Documentation shipped in this stage

Doxygen each identity field/method with declaration versus observation, source
mask, validity, transaction bound and output-on-error. Update `docs/variants.md`,
API/device guides with an identity decision table and compiling probe example
that checks Status before using fields. README, CHANGELOG and ledger name exact
implemented selection and hardware limits. Run Doxygen/warning/link checks.
Keep one execution branch and coherent-unit/per-prompt commit+sync; remote CI
execution/status checking belongs only to 29 in this first set.

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
