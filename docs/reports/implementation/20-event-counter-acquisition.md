# Prompt20 - complete event and counter acquisition

Date: 2026-11-10. Branch: `promptset/dp8382x-implementation-20261028`.
Execution base: `5e148fffde074b85fadc15d8ff66fe2c77111036`.
Entry/prerequisite checkpoint: `0d176b555c7fb35c1836041d9b7499ac5e9d5304`,
clean, fetched and equal to upstream. The same execution branch is retained;
final commit and upstream equality are recorded in the handoff without self-reference.

## Rules, owned procedure and behavior

Applied root/parent AGENTS, COMMON, prompt20, CONVENTIONS, EVIDENCE,
FULL_COVERAGE, ENGINEERING, SOURCE_MAP, VERIFICATION, OP-14 and the remaining
prompt audit. Reopened current entry implementation, README, architecture,
variants, reference index and ledger. Prerequisites are the single collector
from13, source research from4 and live link integration from19. Owned ledger
procedure: `collect_event_counters`. The lead owns public values, existing
driver/collector source, shared documentation, registration, integration and Git.
One application owner still serializes instance calls and all controller users.

Added typed `collectEvents`, `CounterSnapshot`, passive `counterSnapshot` and
bus/clock-silent `clearEvidence`. The live method delegates one fixed six-read
batch to the existing acquisition path: PHYSTS, FLDS, MISR1, MISR2, FCSCR, RECR.
It stages ordinary output while the collector immediately retains each valid
returned prefix. Partial, late, release and clock failures cannot manufacture
complete success or zero missing history. Later collector reads invalidate the
previous complete aggregate. No RXFS read is added to label MISR2's shared bit.

Per-source evidence now distinguishes raw/latest data, sourced event unions,
coalescing, sample-count saturation, hardware saturation, host sum overflow,
lower bounds and discontinuity. Only FCSCR is accumulated. The previous RECR
raw-word sum was removed because its effective width/saturation remain disputed.
Raw RECR/RCSR acquisition remains available with explicit interpretation gates.

Reset/external/configuration uncertainty retains old samples and sums as mixed
history with stale/discontinuous flags. Fresh latest samples can coexist with
that history; no per-reset event attribution is claimed. Explicit clear discards
local history only, refuses running work/callbacks, and preserves link, trust,
identity, transport statistics and operation results. Hardware can still return
events predating the clear. Cached readers and formatting never consume again.

## Fresh sources and delegation

[Independent source review](20-source-review.md) records current unchanged
SNLS647I bytes, original table images, TI RTOS main
`94bf4343004732983b0b8e1eec123d88ff70453b`, file hashes/licenses and independently
derived masks/boundaries. Generic/chip register printers are rejected as safe
dump designs. Vendor archives/hashes are unchanged; no comparison implementation
was copied. RECR arithmetic Q16 and RMII RC decoding Q17 remain unresolved;
RXFS SFD configuration-reference and selector-history ambiguity remain explicit.

[Peer review](20-peer-review.md) records clean current INA228
`73f954838d3fac8cce747d54d0ad738940d4c625`, TMP1x2
`db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` and PCA9555
`414e0f66b13082e974ec27a4a6e86985a65d83fd`. Exact paths include declarations,
capture/staging/cleanup implementations, executing tests and registration,
cached CLI rendering, Python runners and agent/workflow rules. Adopted staged
ordinary output, one destructive capture path, explicit partial evidence and
passive rendering; rejected sensor timing, I2C transaction semantics and a
sticky raw union presented as the latest exact device word.

Delegation selected gpt-6-astra/high for original-PDF destructive semantics and
independent correctness review; the current capable model handled sibling/API
review and disjoint tests/example/consumer work. Reviewers edited only their
reports; the test owner edited only assigned test/example paths. The lead
reviewed all contributions against actual source/diffs and owned all Git work.

Review found no remaining blocking correctness defect. Follow-up test gaps for
ANER/10BTSCR/RXFS event projections and host overflow were closed. The existing
small arithmetic was factored into private `DP8382xEvidence.h`, used by the
collector and direct literal limit tests. No public test hook, allocated event
framework, second collector, per-epoch archive or alternate transport was added.

## Verification

Environment: Windows/PowerShell, GNU C++15.1.0 WinLibs, Ninja, C++17 Release,
strict warnings including conversions as errors. Checks remain active under
NDEBUG. Controller/event behavior below is an identified host fake.

| Evidence layer | Result |
| --- | --- |
| Host Release | PASS:84 registered CTests, including five new event suites and eleven active negative Release probes. Five standalone public-header translation units compile. |
| UBSan trap | PASS:84 CTests in a separate build. Other sanitizers/second compiler NOT RUN locally under the Windows verification policy. |
| Portable consumer | PASS: tests-OFF real core link and1/1 executing consumer. Acquires once and formats two copied event views with no extra bus or clock calls. |
| Research/tool checks | PASS:102 Python tests; ledger97 registers/52 procedures/35 features; vendor hashes, reference/local links and diff checks. |
| Public documentation/version | PASS: Doxygen157 HTML pages, public coverage and two rejected negative fixtures; deterministic version check. Version remains0.1.0. |
| SDK/exported package | NOT RUN: SDK integration and release-package campaigns retain their later owners. Portable linkage is a separate layer. |
| Remote CI | NOT RUN: first execution reserves remote dispatch/status checks for prompt71; no workflow change. |
| Physical | NOT RUN: event/counter traces, invalid-symbol injection, error rates and Q16/Q17 require separately admitted identified fixtures. |

Commands:

```sh
cmake -S . -B build/events20-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/events20-release
ctest --test-dir build/events20-release --output-on-failure --no-tests=error
cmake -S . -B build/events20-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON "-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/events20-ubsan
ctest --test-dir build/events20-ubsan --output-on-failure --no-tests=error
cmake -S test/consumer -B build/events20-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/events20-consumer
ctest --test-dir build/events20-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/check_doxygen.py
python scripts/generate_version.py check
python tools/verify_references.py
git diff --check
```

Independent literals cover every MISR/FLDS event bit, auxiliary projections,
FCS saturation/threshold boundaries, raw RECR and no inferred RCSR faults.
Every read phase is failed both with and without possible hardware consumption.
Tests verify partial timestamp validity, aggregate sentinels, retained primary
versus release errors, late success, new events between reads, repeated captures,
local discard/reset, passive readers, buffer truncation and cross-PHY MMD
contention. Direct production arithmetic tests cover UINT32/UINT64 exact-limit,
overflow and sticky lower-bound behavior without billions of acquisitions.

## Documentation and remaining boundaries

Updated source Doxygen, API and architecture, ledger procedure/feature evidence,
device behavior, source catalogue, independent vectors, diagnostics/link guides,
README/CHANGELOG, test guide and progress. Added the event/counter user guide and
compiling example. Doxygen explicitly requires EvidenceSample/EvidenceSnapshot/
CounterSnapshot coverage. Reviewed variants/version/tool interfaces without
changing support or versions: only the declared exact primary part is admitted.

Global IRQ routing/selection and source-qualified session diagnostics retain
their later complete procedures. Managed health is prompt21; chip error counts
are not MDIO activity or application readiness. Source-blocked RECR arithmetic,
RMII RC polarity and physical verification remain named gates while independently
documented raw acquisition is complete. Prompt20 software is complete within
those explicit evidence boundaries.
