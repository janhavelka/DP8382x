# Prompt 17 - negotiated link settings and verified application

Date: 2026-11-08. Execution branch:
`promptset/dp8382x-implementation-20261028`.
Base: `5e148fffde074b85fadc15d8ff66fe2c77111036`.
Fetched, clean, synchronized entry/prerequisite checkpoint:
`1e0cec9067c498ac5bc7f2c03607395af73e8c2a` (prompt 16).
Final checked commit and verified upstream equality are reported in the handoff,
avoiding a self-referencing commit.

## Scope, rules and implementation

Reopened repository/parent AGENTS, COMMON, prompt 17, CONVENTIONS, ENGINEERING,
EVIDENCE, FULL_COVERAGE, SOURCE_MAP, VERIFICATION and the remaining-prompts audit.
Consumed the actual stage-16 operation slot, identity/reset/floor behavior,
stage-13 collector and stage-7 typed values. Owned ledger procedures are
`configure_an` and `next_page_exchange`. Owned paths comprise the driver header,
existing transport/lifecycle source, new configuration responsibility, focused
tests/registration, portable example/consumer and affected evidence/guides.
Siblings remained read-only; the lead alone integrated and operated Git.

Implemented `startConfigure` with default and explicit-options overloads,
`settingsSnapshot`, typed `NextPage`/`NextPageExchange`/`ConfigureOptions`, and
retained exchange results. Starts perform no bus/clock callbacks; accepted intent
becomes authoritative only after complete admission. All 15 nonempty technology
subsets and four pause choices work. Complete selector replacement, prohibited
local RF neutralization, exact reserved-bit preservation and command-neutral
composition are source-backed. Nondefault crossover/extended duplex and forced
settings reject before mutation until prompt 18. Configure adds no VOD write.

The existing operation slot, lifetime token, frame/time bounds, cancellation and
OP-9 restoration barrier remain authoritative. A new configuration translation
unit owns a concrete chip responsibility, not another job engine or transaction
mechanism. Atomic RMW uses the existing terminal controller callback across every
bus participant. No controller lock spans polls/waits. Initialization reuses the
same negotiated apply/verify path after mandatory VOD setup for every reset kind.
Replayed intent always has NP clear. A failed CONFIGURE can be retried through a
new admitted CONFIGURE; another operation's barrier cannot be bypassed that way.

Settings keep desired intent, raw readbacks, complete observed settings and trust
separate. Verification currently covers ANAR/BMCR plus previously established
mandatory VOD. A successful register application is not AN completion or link-up.
An explicit finite wait establishes BMSR AN-complete separately. The first stable
mismatch survives a subsequent transport/deadline failure; the later transport
status and full signed detail remain in `failingBatch`.

Clause 28 service drains prior page evidence, restarts with transient NP, then
distinguishes fresh BASE from NEXT events. It loads the first local page only
after bilateral BASE willingness and retains each consumed event/payload before
advancing. Toggle is hardware-owned; duplicate events consume finite storage but
cannot advance local content. Supported messages are null, remote fault, OUI and
PHY identifier with their defined unformatted pages. Null continuation is bounded.
Partner refusal is explicit, not an MDIO failure. Bilateral page termination still
requires AN-complete before clearing/verifying NP without another restart.
Failure/cancel ends immediately without cleanup writes and retains possible NP.
Public ANER collection is excluded while the exchange owns that destructive
source. Other admitted reads preserve the existing owner/quiet-floor rules.

## Fresh evidence and delegation

[Source review](17-source-review.md) records original TI/IEEE pages, unchanged
vendor hashes, current TI GitHub pins, per-file license scope, independent literal
vectors and targeted clarification-search limits. D8 supplies the software
protocol selection; Q10 remains physical qualification, not an implementation
block. No vendor payload changed or GPL implementation was copied.

[Peer review](17-peer-source-review.md) records fresh TMP1x2 and PCA9555 paths,
clean revisions, implementations, tests, CLI rendering, Doxygen and runners.
Desired adoption follows this project's admission-time contract rather than
TMP1x2's first-write timing. Sensor waits and I2C ACK semantics were rejected.

Three parallel assignments used disjoint paths: gpt-6-astra/high for original
silicon/IEEE reasoning and the configuration implementation; the current capable
model for independent literal tests; and the current capable model for peer
audit, examples/docs and independent implementation review. The source author
does not count as an independent reviewer of its own code. Lead reviewed the
actual contributions and integrated shared headers/lifecycle/Git.

Review fixed stale ledger wording that treated Q10 as a software gate or implied
persistent NP replay. It also ensured the exchange-complete flag requires actual
AN completion. Independent implementation review found no remaining confirmed
defect and requested added late-write/BMCR-mismatch cases; these now execute.
Final simplicity review retained fixed values and explicit phases, shared replay,
one collector and one transport owner; no discovery/factory/retry framework.

## Verification

Environment: Windows, PowerShell, GNU C++ 15.1.0 (WinLibs), Ninja, C++17 Release,
strict warnings including conversion warnings treated as errors. Check macros
remain active with NDEBUG. The part/controller in these tests is simulated;
literal vectors were independently derived from original sources.

| Evidence layer | Result and scope |
| --- | --- |
| Host implementation | PASS: 59 registered CTests, including 11 negotiation suites and eight executing negative Release probes. Negotiation suites execute 18,544 checks. |
| Portable consumer | PASS: tests-OFF linked consumer executes generic and 100/full owner examples, plus existing transport/identity/reset examples; no SDK dependency. |
| Research/tools | PASS: ledger validates 97 register records, 52 procedures, 35 features; 102 Python tests; reference/hash/local-link checks and diff whitespace checks. |
| Doxygen/version | PASS: generated API/links and two negative fixtures; deterministic version read-only check. Version remains 0.1.0. |
| SDK/package release | NOT RUN: SDK integration and clean release-package campaigns retain their later stages; no SDK support inferred from a portable link. |
| Remote CI | NOT RUN: first-set execution reserves dispatch/status checks for prompt 71. |
| Physical | NOT RUN: Q10 handshake, actual advertisement/link readbacks, MDC timing and MAC pause behavior require identified admitted fixtures. |

Commands:

```sh
cmake -S . -B build/an17-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/an17-release
ctest --test-dir build/an17-release --output-on-failure
cmake -S test/consumer -B build/an17-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/an17-consumer
ctest --test-dir build/an17-consumer --output-on-failure
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
python tools/check_doxygen.py
python scripts/generate_version.py check
python tools/verify_references.py
git diff --check
```

Independent tests cover literal masks/traces/counts, all legal advertisement/pause
combinations, no-I/O rejection, every baseline/next-page frame failure and cancel
boundary, accepted/late writes, prior mismatch plus later failure, sentinel output,
retained consumed pages, missing/duplicate events, partner refusal, null service,
page limits, reset replay including 10 Mbit/s, two owners, collector exclusion,
short slices, raw expiry, frozen-clock stalls and clock regression. Existing
overflow/quantization/reset-floor suites continue to pass against the shared core.

## Documentation and remaining boundaries

Updated public Doxygen, API/architecture, initialization and access guides,
negotiation guide, ledger/feature coverage, independent vectors, test commands,
README/CHANGELOG and progress. Version metadata, variants and tool CLI needed no
behavior change: no release-version change, relative support or new runner is
claimed. Doxygen checks now require the new public types. Future CLI/SDK routes
must call this owner rather than create another register procedure.

Stage 17 software is complete. Forced/crossover profiles remain prompt 18;
resolved link/pause observation, managed health, SDK projection and physical Q10
remain their assigned later work. Initialization/application may succeed with
cable down. No source review or host fake establishes electrical qualification.
