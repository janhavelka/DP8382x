# 48 - Implement the offline protocol verifier and durable evidence bundles

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisite: the actual shared producer, schema and compiled fixture from
[47](47-cli-native-integration.md). This task delivers a working offline tool;
physical serial sessions follow in task 49.

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
Handle echo, ANSI/prompts, fragmentation, unterminated trailers,
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

Reserve exclusive-create evidence bundles with provisional incomplete results.
Save bounded timestamped raw bytes, commands, identities, context, start/end,
case dispositions and cleanup outcome. Finalize closed payload files before
hashing their actual bytes and lengths. Write one manifest last, excluding
itself from its payload/hash list, using exclusive bundle ownership and atomic
replacement within that bundle. A valid complete manifest is the completion
marker; provisional or abandoned temporary files cannot indicate PASS. Verify
manifests offline against stored payloads, including missing, truncated or
changed files. Define required payload membership, unique normalized bundle-
relative paths, schema/version and agreement between manifest verdict and case
results. Reject empty/omitted mandatory payloads, duplicate/escaping paths and
verdict disagreement even when every listed hash matches.
Existing bundles, partial writes, permissions, disk errors,
flush/close failures and incomplete manifests are unsuccessful evidence.
Preserve prior runs and never query arbitrary environment secrets.

## Acceptance

Test every trailer split, malformed type/enum, reboot/stale/duplicate result
and false-PASS condition. Inject bundle collisions and write/flush/close/hash
errors. Drive the compiled CLI fixture with bounded subprocess lifetime, stdin
writes, stdout/stderr drains and termination escalation; reap it and preserve
failures. Verify nonzero exits for all unsuccessful evidence and interrupted
execution. Define pass/fail/inconclusive and offline/dry-run meanings;
zero-executed, skipped-only and incomplete runs cannot pass.

Document Python protocol/bundle functions and `docs/hil-runner.md`: finite
limits, manifest verification, exit codes and explicitly offline examples.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
