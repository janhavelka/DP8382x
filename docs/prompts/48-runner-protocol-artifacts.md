# 48 - Implement the offline protocol verifier and durable evidence records

Prerequisite: shared producer, schema and compiled fixture from
[43](43-cli-foundation.md) through [46](46-cli-diagnostics-controls.md).
SDK integration at47 does not block offline runner work48-52. Serial follows49.

## Current evidence and ownership

Reopen INA228's `tools/run_i2c_hil.py` framing/read helpers and
`tools/test_run_i2c_hil_parser.py` terminated-trailer, short-write,
stale-input and identity tests. Inspect TMP1x2's `one_record`,
`operation_admission`, `response_complete`, `classify`, `EvidenceFiles`,
`SerialSession` and compiled fixture tests. Compare 24Cxx
`tools/hil_identity.py` (`parse_identity`, `check_identity`) and
`tools/hil_artifacts.py` provenance/rendering with malformed identity/hash
tests. Record revisions/paths and adopted/adapted/rejected choices.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Implement

Create the single canonical `tools/dp8382x_hil_runner.py` entry point with
`--help`, dry-run and explicit offline-transcript verification. Use direct
functions and small session/case/result values; split helpers only for a real
independently tested responsibility. Import/testing requires no optional
serial dependency, SDK or hardware. Offline results are labeled host protocol
evidence and cannot claim physical PASS. Do not create a generic serial/device
framework.

Validate positive finite deadlines/durations/counts, limits, known suites and
explicit target expectations; reject NaN/infinity, negatives and conflicts.
Implement strict bounded record decoding and correlation for schema,
boot/session, request/operation, identity, terminal status and typed fields.
Handle echo, ANSI/prompts, fragmentation, unterminated records,
missing/duplicate/conflicting fields, stale/out-of-order records, rollover,
reboot, wrong firmware and oversized input. Validate integer types without
accepting booleans as numbers, enforce ranges before arithmetic and bound
decoder carry-over across fragments. Do not strip arbitrary ANSI sequences or
repair malformed machine records into success. A duplicate/conflicting
terminal discovered after the first result invalidates the run; consume
trailing complete records within the defined finite session closure window
before finalizing its verdict. An admission, prompt, substring OK or skip
never satisfies completion.

Separate command Status from case assertions: exact expected rejection
requires proved unchanged state/counters and no extra transport/cleanup
failure. Preserve primary failure after cleanup success. Unknown firmware
identity cannot meet a known expectation; runner checkout identity is
separate, and observed short SHAs remain short. Freeze full-revision versus
prefix-match policy: prefix agreement cannot prove a longer unseen revision.
Record exactly the identity strength verified. Keep host UTC and device
monotonic time domains distinct.

Keep exactly two files per run: `<run>.txt` transcript and `<run>.json` with
verdict, cases, identity, fixture, UTC times, commands, restoration, transcript
SHA-256/length and truncation. Exclusive create reserves both with an INCONCLUSIVE
placeholder. Close transcript first; write JSON via temp file, flush, fsync and
os.replace. Offline verify checks schema/hash/length and aggregate worst verdict:
FAIL, then INCONCLUSIVE/BLOCKED/NOT RUN, then PASS. Zero cases, skipped-only runs,
failed restoration and incomplete/corrupt evidence never pass. Preserve prior runs.

## Acceptance

Test every record split, malformed type/enum, reboot/stale/duplicate result
and false-PASS condition. Inject file collisions and write/flush/close/hash
errors. Drive the compiled CLI fixture with bounded subprocess lifetime, stdin
writes, stdout/stderr drains and termination escalation; reap it and preserve
failures. Verify nonzero exits for all unsuccessful evidence and interrupted
execution. Define pass/fail/inconclusive and offline/dry-run meanings;
zero-executed, skipped-only and incomplete runs cannot pass.

Document Python protocol/artifact functions and `docs/hil-runner.md`: finite
limits, JSON/transcript verification, exit codes and explicitly offline examples.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
