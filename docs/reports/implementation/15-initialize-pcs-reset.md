# Prompt 15 — initialization and PCS reset

Date: 2026-11-08. Execution branch:
`promptset/dp8382x-implementation-20261028`; original base
`5e148fffde074b85fadc15d8ff66fe2c77111036`. Clean synchronized entry/prerequisite
checkpoint: `f7165130b8f584aaf43fee997e4887aa4c3bc064` (prompt 14).
The final checked commit and upstream-equality result are recorded in the handoff.

## Rules, ownership and outcome

Followed repository/parent AGENTS, COMMON, CONVENTIONS, EVIDENCE, FULL_COVERAGE,
prompt 15 and the remaining-prompts audit. One application owner serializes the
instance; one terminal controller transaction excludes every bus participant.
Starts, cancellation, snapshots, take, abandon and teardown remain bus/clock
silent. No sibling edits, automatic retries, RTOS dependencies, allocations,
generic engine, fixture operation or remote CI were introduced.

Owned `include/DP8382x/DP8382x.h`, `src/DP8382x.cpp`, operation tests/registration,
portable example/consumer and affected guides/evidence. Ledger procedure scope
is `pcs_reset` and `initialize_10base`; reset family remains PARTIAL for prompt16.
Prerequisites 14 and 8 were verified against actual identity code and OP-1–17,
not inferred from earlier completion summaries.

Implemented `startInitialize`, `pollOperation`, `takeResult`, `cancel`,
`operationSnapshot`, `abandonCleanup`, `invalidateDeviceState` and `isInitialized`.
One explicit initialization phase switch uses fixed state and the existing
terminal batch path. Tokens are lifetime uint64 values without exhaustion policy.
Only INITIALIZE and NONE/PCS ship; hard-reset floors, link settings snapshots
and managed health arrive with their real consumers in prompts16,17–18,21.
Removed duplicated planned operation declarations from the guide in favor of
the shipped header and explicit future extensions.

The sequence accepts identity, captures raw straps, waits out observed reset,
optionally issues literal PCS reset and observes completion, refreshes identity/
straps, then always writes/verifies mandatory VOD. Only currently implemented
persistent owned state is restored. Raw LED/IRQ/WoL maintenance is not a desired
profile. RCSR replay and CDCR rescal remain their specified later procedures.
Initialization does not require link and does not expose managed-health stubs.

Short poll slices yield before a complete batch. Total frames/deadline, strict
bound admission and stalled polls are finite. Reset look-ahead is rechecked
at submission; waits use post-callback upper bounds. Cancellation freezes its
own result and possible effects. A later initialization has a separate token,
budget and retained result; success cannot replace the earlier failure.
Cleanup survives take and failed restoration until successful restoration or
explicit external disposition. Raw admission and end/rebind enforce those rules.

## Fresh sibling and original-source review

[Peer review](15-peer-source-review.md) records actual TMP1x2, PCA9555 and 24Cxx
headers, implementations, tests/registration, CLI, Python runners and rules:

- TMP1x2 `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb`.
- PCA9555 `414e0f66b13082e974ec27a4a6e86985a65d83fd`.
- 24Cxx `e4e1eeb41575df462e585c54989950323c43ab7a`.

Adopted retained effects, explicit result delivery and finite cleanup ownership.
Rejected clockful starts/snapshots, token exhaustion, cancellation that hides
cleanup behind the original result, EEPROM ACK polling and sensor/memory delays.
Chosen names follow the frozen DP8382x contracts, not sibling transport details.

Independent source audit at 2026-11-08 07:34 UTC fetched the
[current TI datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf): SNLS647I,
August 2025, byte-identical to the preserved archive. SHA-256:
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Original PDF pages15,17,54,86,89,90 were visually inspected. Table8-19 pp54–55
establishes self-clearing BMCR reset and vendor retention; §8.3.13 p40 and
table8-81 p86 establish mandatory VOD and stable mask; tables8-87/88 pp89–90
establish raw strap records. §7.6 timing/footnote2 and Figure7-1 p18 establish
external supply, clock and RESET_N readiness, not a software PCS completion
maximum. Final source review distinguished the figure's hold-low recommendation
from the footnote's additional-reset requirement; the application establishes
one valid readiness path before probing.

Current TI RTOS `main` was
`94bf4343004732983b0b8e1eec123d88ff70453b`:

| File | Git blob | Comparison |
| --- | --- | --- |
| [generic_phy.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c) | `39414bd3b1004cdf6075bf0f4be30180babdc84d` | Reset RMW line159 rejected for this procedure; completion read/test lines190–195 compared. |
| [dp83826.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) | `e0b64710350d764fe16ffd089fbdc5246e600039` | Mandatory initialization compared; hard-reset address/mask defect at349 remains in catalogue, outside PCS scope. |
| [dp83826_priv.h](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826_priv.h) | `37ab9b94d67687c569ac2e1e36b315fb90eb6749` | Names/masks compared with exact tables; macros are not independent authority. |

These TI files retain BSD-3-Clause notices at lines1–31 and their repository
license. No implementation was copied. Original vendor payloads/provenance and
hashes remain unchanged; PDFs retain TI notices.

## Independent review and corrections

`init15_sources` used gpt-6-astra/high for independent original-PDF arithmetic,
reset/access review and final production timing/effect review. Expected literals
were derived from original tables, not ledger or production masks.
`init15_tests` used the current capable model for disjoint `test/operations.cpp`;
`init15_peers` used the current capable model for sibling research, read-only
lifecycle/maintainability review and then the separately assigned example/consumer.
No subagent edited Git or siblings. The lead owned shared code/docs and reviewed
each contribution; reviewers used separate build directories.

Confirmed findings were fixed and independently rechecked:

- Replaced saturating-statistic callback detection with an explicit invocation
  flag, so frame budgets and identity never depend on statistics saturation.
- Retained completed strap reads before terminal release/deadline failure with
  raw validity and conservative stale/time flags.
- Used actual normalized release classification in the frozen batch summary.
- Marked running-operation identity stale on explicit or contradictory live
  invalidation; preserved already frozen historical results.
- Preserved initialized state for a proved effect-free failed NONE reapply;
  clarified the historical configurationDirty latch and current readiness.

Final review found no remaining blocker. One private phase switch, shared batch
builders and transport normalization remain; no alternate selector implementation,
hidden work, extra register framework or speculative settings replay was added.

## Verification and documentation

Environment: Windows PowerShell, GNU MinGW-w6415.1.0 x86-64, CMake4.0.1,
Python3.12.10, Doxygen1.13.2. Reproduction:

```sh
cmake -S . -B build/init15-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/init15-release
ctest --test-dir build/init15-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/init15-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/init15-consumer
ctest --test-dir build/init15-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/verify_references.py
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

| Layer | Result |
| --- | --- |
| Portable host | PASS:40 registered Release CTests, including six operation suites (13,056 active checks) and six negative assertion probes. |
| Portable consumer | PASS: tests OFF; linked NONE/PCS owner loops, original failure plus separately failed restoration, explicit abandon and retained results. |
| Python research/tools | PASS:102 tests; independent silicon/policy vectors remain separate from driver evidence. |
| Ledger | PASS:97 registers,283 fields,52 procedures,35 feature families; reset PARTIAL, mandatory initialization COMPLETE. |
| Doxygen | PASS:102 HTML pages, public operation/evidence docs and two rejected negative fixtures. |
| Reference/version/diff | PASS: archived hashes/local links, unchanged generated version and whitespace. |
| Installed/exported package | NOT RUN: later package stages. |
| SDK link | NOT RUN: native adapter remains prompts37–39. |
| Remote CI | NOT RUN: first-set checkpoint reserved for prompt71. |
| Physical | NOT RUN: no fixture, actual part/strap/supply measurements or MDIO/reset traces operated. |

Tests inject every one of the 50 NONE/PCS trace frame failures, every release
phase, accepted-before-error writes, sticky reset, output/partial validity and
malformed receipts. They cover OP-6 equality/100 Hz/preemption, quantization,
overflow, raw expiry, regression, frozen clocks, slices, cancellation, cleanup,
token reuse/staleness, two-instance exclusion and complete MMD restoration.

Updated public Doxygen, initialization/API/user/access/variant guides, independent
vectors, source comparison, ledger/coverage, test guide, README/CHANGELOG and
progress. Version/tool commands are unchanged because no version/package output
changed; the Doxygen checker now requires actual operation types. SDK/CLI/runner
implementations remain later work; their source/qualification gates are not closed
by this portable completion. Future link profiles extend this initialization
path when implemented. Source reset-domain ambiguity remains conservatively
handled and still requires exact physical/TI evidence for broader claims.
