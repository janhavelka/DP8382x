# Prompt 19 - live and cached link observations

Date: 2026-11-10. Execution branch:
`promptset/dp8382x-implementation-20261028`.
Base: `5e148fffde074b85fadc15d8ff66fe2c77111036`.
Entry checkpoint: `8ee2efd77c27eb03dce069e1a0c98bd7fcb1440c`, clean, fetched and
equal to upstream after the independent prompts 13-18 audit. The final commit
and verified upstream equality are recorded in the handoff, avoiding self-reference.

## Scope, rules and outcome

Reopened root/parent AGENTS, COMMON, prompt19, relevant shared source/engineering/
verification contracts, OP-14, remaining-prompts audit, README, architecture,
variants and reference index. Prerequisites are the executing collector, exact
identity and initialization, and complete negotiated/forced profile paths from
13-18, with original diagnostic access facts from4. Owned ledger procedure:
`observe_link`. Names remain PascalCase values/camelCase methods in DP8382x.
The lead owns public driver headers, transport invalidation hooks, the cohesive
link source, shared guides/ledger/progress, integration and Git.

Implemented `readLinkStatus`, copied `linkSnapshot`, NegotiationState, LinkChange
and a fixed-size LinkSnapshot. Ten physical reads in two terminal callbacks
reserve their whole aggregate allowance before consuming history. Complete
bookend consistency gates ordinary publication; no retry or simultaneous-sample
claim. All destructive reads extend the existing collector. Successfully
consumed prefixes and possible lost history survive any later failure.

Forced resolution uses BMCR and the verified CR2 exception; qualified AN uses
PHYSTS. IEEE pause requires a highest-common full-duplex technology and a PHYCR
cross-check. Raw evidence survives invalidity. RUNNING semantic writes suppress
resolution; reset waits/quiet floors and active next-page ownership deny reads.
Cached readers and rendering perform no I/O or clock calls. Failure retains
old values/time as stale, including budget rejection before MDIO. Clock faults
from another API also invalidate use. The acquisition interval supports a
conservative application age policy, and saturating publication/change counters
retain first, changed, repeated, stale and regained evidence.

## Current sources, delegation and review

[Original-source review](19-source-review.md) pins fresh unchanged SNLS647I,
TI RTOS `94bf4343004732983b0b8e1eec123d88ff70453b`, selected ESP-IDF v6.1 and the
identified IEEE 2012 original-document mirror. It visually checks original
register tables and Table28B-3, with per-file licenses and independent literals.
Vendor payloads/hashes remain unchanged; no comparison implementation was copied.
TI first-low output omission and unqualified forced PHYSTS helpers were rejected.

[Sibling review](19-peer-review.md) records exact paths/hashes and clean current
TMP1x2 `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb`, INA228
`73f954838d3fac8cce747d54d0ad738940d4c625` and FieldCore-node
`5ad9292dc8481cf08be9a61f662495a6474d1da0`. Actual declarations, implementation,
tests, CLI rendering, runners and agent rules were reopened. Staged publication,
separate destructive capture and passive copying were adopted; sensor freshness,
CONFIG bookends and I2C assumptions were not used as PHY coherence evidence.
No workflow exists here yet; remote CI remains prompt71 and no workflow was added.

Delegation used gpt-6-astra/high for silicon, destructive semantics and independent
coherence review; the current capable model handled disjoint test/example work
and peer/integration review. Reviewers did not edit production, siblings or Git.
Test and lead verification used independent build directories. The lead reviewed
actual code/diffs and all contributions, rather than accepting report summaries.

Independent review found four concrete corrections: highest-common technology
for pause, acquisition-start time for conservative maximum age, stale cache on
failed budget admission, and invalidation on clock faults discovered elsewhere.
All were fixed with executing regressions. Final review found no remaining
blocking defect. The implementation is one explicit bounded read plan and small
value comparisons, reusing existing transport/collector hooks; no new registry,
event queue, notification scheduler, allocation or SDK callback layer.

## Verification

Environment: Windows PowerShell, GNU C++15.1.0 WinLibs, Ninja, C++17 Release,
strict warnings including conversion warnings as errors. Tests use active
CHECKs under NDEBUG. Hardware/controller behavior below is a host fake.

| Layer | Result and limit |
| --- | --- |
| Host Release | PASS: all78 registered CTests, including nine link suites and ten executing negative Release probes. Five public-header translation units compile independently. |
| UBSan trap | PASS: all78 CTests in a separate instrumented build. Second compiler and other sanitizers NOT RUN locally under the Windows verification policy. |
| Portable consumer | PASS: separate tests-OFF C++17 build/link,1/1 CTest, including live sampling and no-I/O cached policy plus existing owner examples. |
| Research/tools | PASS:102 Python tests; ledger97 registers/52 procedures/35 features; reference/vendor-hash/local-link and diff checks. |
| Public API/version | PASS: Doxygen155 generated HTML pages and two rejected negative fixtures; generated version read-only check. Version remains0.1.0. |
| SDK and release package | NOT RUN: adapter/SDK and exported-package campaigns retain later owners; portable linkage does not prove those layers. |
| Remote CI | NOT RUN: first-execution dispatch/status checking belongs only to71. |
| Physical | NOT RUN: Q11 forced/strap/extension behavior, actual partner agreement, cable transitions and MDIO timing need an identified admitted fixture. |

Commands executed from the repository root:

```sh
cmake -S . -B build/link19-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/link19-release
ctest --test-dir build/link19-release --output-on-failure --no-tests=error
cmake -S . -B build/link19-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON "-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/link19-ubsan
ctest --test-dir build/link19-ubsan --output-on-failure --no-tests=error
cmake -S test/consumer -B build/link19-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/link19-consumer
ctest --test-dir build/link19-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/check_doxygen.py
python scripts/generate_version.py check
python tools/verify_references.py
git diff --check
```

Independent test oracles cover the literal8+2 read plan, every failed frame,
both-direction transitions at every read boundary, latch-low/current distinction,
mode/AN bookend conflicts, all16 pause cells, highest-common priority and raw
cross-check disagreement, all forced speed/duplex and extension gates. They also
cover repeated readers, change/stale/regained evidence, resets/external changes,
zero/missing frame timestamps, frozen clock, lag-aware age, uint64 overflow and
clock regression through link and raw APIs. OP-14 running/result-pending routes,
next-page exclusion, every-frame two-PHY lock contention, release failures,
aggregate minimum admission and no retries are executing assertions.

## Documentation and remaining boundaries

Updated public Doxygen, API/architecture, authoritative procedure/feature
evidence, behavior/source catalogue, independent vectors, README/CHANGELOG,
test/consumer guide and progress. Added the [link guide](../../link-observation.md)
and linked portable example. Reviewed variants without changing support: only
accepted DP83826I is admitted; no new family/revision is claimed. Existing tool
interfaces are unchanged; Doxygen now explicitly requires LinkSnapshot coverage.

The `forced_physts` question remains a nonblocking raw-value gate; forced mode
does not decode those bits. Coherent host observations do not prove simultaneous
sampling or partner duplex in forced mode. Managed health, expanded event/counter
acquisition, SDK notification and application network readiness retain their
later owners. Prompt19 software is complete; physical qualification is NOT RUN.
