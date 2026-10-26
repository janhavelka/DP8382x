# 20 - Implement the bounded Python session and artifact runner

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-19.

## Fresh audit and owned work

Inspect current INA228 framing/parser tests, TCA9548A session/artifact behavior, TMP1x2
runner and 24Cxx provenance tests. Reconcile their actual grammar with their current
firmware code.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Implement tools/dp8382x_hil_runner.py with a small reusable session/parser structure
only where needed. Support explicit serial selection, dry-run, finite deadlines,
capability selection, expected firmware/board context and bounded artifacts. Serial
dependencies are optional for host tests; a dry-run opens no port and never claims
physical PASS.

Parse the frozen shared CLI protocol exactly. Correlate session/boot/request/operation
and terminal status; distinguish command failure from a test designed to expect
rejection. Require complete records and identity before evaluating a case. No runner
checkout-to-firmware identity inference.

Handle fragmented input, echo/ANSI/prompts, duplicates, stale/out-of-order results, boot
reset, missing fields, unexpected firmware, timeout, serial loss, output limits,
cancellation and cleanup failure. Define pass/fail/inconclusive exit codes, and preserve
not-run/blocked reasons. Never reset/flash/scan ports automatically.

Write unique bounded raw transcript and structured summary artifacts with command,
firmware/runner version, target/variant/interface/address, SDK, board/fixture settings,
times and per-case evidence. Use exclusive-create bundles, preserve prior runs, and
include artifact sizes/SHA-256 in a manifest. Report failed writes, flush/close and
incomplete manifests. Keep sensitive credentials outside transcripts and do not query
arbitrary host environment values. Use device monotonic time only for claims it actually
supports.

## Acceptance and handoff

Run Python parser/artifact unit tests including maliciously large and malformed input,
existing-output collisions, partial writes, failed flush/close, hashes and interrupted
sessions. Drive the compiled host CLI fixture to catch grammar drift. Test --help and
dry-run without pyserial or hardware. Explain that transcript tests verify the runner,
not a PHY.

Expected output: Executable runner with strict correlation, verdicts and durable
evidence. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
