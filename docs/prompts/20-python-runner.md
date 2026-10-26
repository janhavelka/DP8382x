# 20 - Implement the bounded Python session and artifact runner

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-19.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

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

## Concrete parser, verdict and artifact responsibilities

Apply [CONVENTIONS](CONVENTIONS.md). Reopen these current source pairs, not just
their help pages:

- `INA228/tools/run_i2c_hil.py`: `hilrun_end_re`, `read_until_hilrun_end`,
  `strip_hilrun_frame`, `run_step`; tests for a full terminated trailer, short
  serial writes, stale input and exact frame identity in
  `tools/test_run_i2c_hil_parser.py`.
- `TMP1x2/tools/hil_tmp1x2_runner.py`: `one_record`, `operation_admission`,
  `response_complete`, `classify`, `EvidenceFiles`, `SerialSession`; its runner
  tests include a compiled CLI fixture rather than only invented transcripts.
- `24Cxx/tools/hil_identity.py`: `parse_identity`, `check_identity`; and
  `tools/hil_artifacts.py`: `collect_provenance`, `render_markdown`, with their
  malformed/unknown identity and exact-source-hash tests.

Use `tools/dp8382x_hil_runner.py` as the one entry point. Start with direct
functions plus small session/case/result values; split protocol or artifact
helpers only when their responsibilities are reused and independently tested.
Do not create a generic device/serial plugin framework or require optional
pyserial to import/test parsing. Freeze actual helper names in source/docstrings;
peer names above are evidence, not another public DP8382x API.

Process a run in this order:

1. Strictly validate positive finite timeouts/durations/counts, limits, target
   expectations, explicit port and fixture permissions before opening anything.
   Reject NaN/infinity, negative values, unknown suites and conflicting options.
   `--help` and dry-run require neither serial nor SDK tools.
2. Reserve a unique evidence bundle exclusively and write a provisional incomplete
   result before hardware access. Inspect DTR/RTS behavior before opening serial;
   configure a non-resetting mode where supported and report limitations rather
   than causing an implicit reset. No port guessing/reconnect/replay policy.
3. Collect complete reported firmware identity and session records; compare only
   explicit expectations. Host checkout revision/hashes are separate provenance.
   Unknown identity cannot satisfy a required known revision/target. Preserve
   observed abbreviations without manufacturing a full SHA.
4. Send one correlated command with a write deadline; a partial write fails that
   attempt and is not replayed. Read with a total deadline, maximum record size,
   total capture limit and bounded decoder state. Fragmentation cannot turn an
   unterminated numeric field into a complete result.
5. Require the exact expected session/request/operation and terminal fields.
   An admission record, prompt, substring OK, stale completion or skipped case
   is insufficient. A new boot invalidates pending correlation. Preserve
   unexpected/duplicate/conflicting evidence as a protocol failure.
6. Evaluate case assertions separately from command `Status`. Expected rejection
   passes only with its exact allowed error, no extra transport/cleanup failure
   and proved unchanged state/counters. Preserve the first failed case when later
   cleanup succeeds; stopped/incomplete/zero-executed campaigns cannot pass.
7. Perform only explicitly admitted cleanup with a separate finite budget. Close
   serial and artifact streams while preserving errors; finalize summary and
   byte-count/SHA-256 manifest only from actual stored bytes. An incomplete write,
   flush/close failure or incomplete manifest makes evidence unsuccessful.

Test every trailer split, echo/ANSI/prompt noise, blank/duplicate/missing fields,
wrong enum/value type, oversized records, stale operation, token rollover, new
boot, serial disappearance, cancellation and cleanup timeout. Inject existing
bundle collisions and permission/disk/short-write/flush/close failures. Assert no
port open on validation/dry-run failures and no hidden command retry. Drive the
compiled stage 18/19 fixture with bounded subprocess lifetime/output and assert
nonzero exit for false-PASS scenarios. Document precise exit-code meanings.

Python module/function docstrings must state accepted records, limits, time
domains, side effects, artifact finalization and error behavior. Update
`docs/hil-runner.md`, `docs/cli.md`, root README and CHANGELOG with working
invocations and a clearly labeled offline example. Doxygen remains current for
any changed C++ protocol producer. Parser and artifact fault tests are useful
disjoint agent assignments once the schema is frozen; use stronger review for
failure classification and durability. The lead owns schema and final verdicts.

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
