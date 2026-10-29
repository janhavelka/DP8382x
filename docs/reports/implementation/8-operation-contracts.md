# Prompt 8: cooperative operation, time and cleanup contracts

Executed 2026-10-29 on `promptset/dp8382x-implementation-20261028` from clean,
fetched, synchronized `382462cdec356ae901c0a501f96435830518430f`.
Original base remains `5e148fffde074b85fadc15d8ff66fe2c77111036`; origin is
`https://github.com/janhavelka/DP8382x.git`. The final handoff records this stage's
commit and verified upstream equality. Lead alone owns integration and Git;
sibling repositories and vendor payloads are unchanged.

## Outcome, rules and owned work

Read prompt 8, COMMON, CONVENTIONS, ENGINEERING and relevant EVIDENCE/VERIFICATION
rules, using prompts 6–7's ownership/direct-call contracts and the reset/restore
ledger as prerequisites. Rechecked upcoming initialization/reset/recovery consumers
15/16/22. No workflows exist; remote CI dispatch/status/wait remains prompt 71.
Core still excludes SDK headers, heap, tasks, platform clocks and logging.

The [API contract](../../api-contracts.md) now specifies exact starts, polling,
snapshot, take, cancel and explicit owner quiescence/disposition. One slot and
fixed-size result retain primary/cleanup receipts, effects, identity/straps and
settings evidence. Lifetime tokens never wrap or reset on end/rebind. Reserved
absolute cleanup budgets and an owner-assisted retire-only path resolve the
broken-clock/owed-cleanup cycle without erasing evidence. The first real consumer
implements this state machine; no unused engine or placeholder facade was added.

One coordinated refinement to prompt 7 adds required `TransportBounds.clockLagUs`:
raw clock samples bound time below, while raw plus the proved lag bounds time
above at callback return. The lag includes quantization, read and scheduling age;
overhead separately covers later dispatch/publication. Minimum waits use a fresh
completion upper bound, readiness uses a lower sample, and uncertain deadline
crossing is distinguished from proved lateness. This corrects the concrete
tick-boundary hazard without another clock/transaction mechanism.

Lead-owned files include API, architecture/design links, feature map, API/example
plans, conventions/source notes, README/CHANGELOG, tests, reports and progress.
Numeric ledger, variants, board guidance, packaging and production-source locations
need no change. Existing EEE/compliance/forced-status/strap gates remain explicit.

## Current peer evidence

All revisions below were clean; source/test/example/runner and AGENTS paths were
reopened read-only. Paths are relative to `C:/Users/Honza/Documents/Projects`.

| Peer and revision | Actual behavior and selected adaptation |
| --- | --- |
| TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `include/TMP1x2/TMP1x2.h:182–207`, `src/TMP1x2Operations.cpp:7–50,97–124,153–170,192–250` define copied admission, lifetime token exhaustion, post-callback timing and retained results. Adopt these principles; reject `src/TMP1x2.cpp:19–69` invalid-bind/unbind erasure. `test/jobs/test_jobs.cpp:169–246,335–381,575–653` covers zero budgets, late calls, cancellation, frozen time and completion anchors; no explicit exhaustion test was found. |
| PCA9555 `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | `include/PCA9555/PCA9555.h:169–189`, `src/PCA9555.cpp:559–583,639–672,765–938` separate primary and finite one-attempt pointer cleanup. Adapt that distinction, but reject caller-ID reuse, stale entry time through callbacks and consumption while cleanup is still owed. `test/test_basic.cpp:620–817,2120–2190,2495–2596` exercises deadlines, primary/cleanup failures, callback allowances and teardown. |
| MB85RC `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | `include/MB85RC/MB85RC.h:165–198,860–869`, `src/MB85RC.cpp:1272–1331,1535–1562,1738–1813` preserve accepted/uncertain effects, copied result and failed-write status after verification. Reject wrapped automatic request IDs and callback-count-only time bounds. `test/test_basic.cpp:5455–5490,5677–5695,5880–5905,6010–6067,6235–6272` confirms retention and deliberate ID wrap. No EEPROM/wake delay is inherited. |
| FieldCore-node `5ad9292dc8481cf08be9a61f662495a6474d1da0` | `src/devices/i2c/Ina228Module.cpp:169–205,337–393,562–568,591–598,618–632,695–702,900–933` separates immutable owner deadline, actual callback completion, queued requests and cached publication. Adopt that boundary and no passive-health admission latch. Reject clearing active library identity after a failed take. `test/native/test_component_ina228/test_ina228_module.cpp:452–459,845–989` checks cache silence, effects, backpressure and explicit requests after fault. |

Consumer/tool paths also reopened:

- TMP1x2 `examples/common/{Tmp1x2CliDiagnostics.cpp,Tmp1x2CliOutput.cpp}` and
  `tools/{hil_tmp1x2_runner.py,test_hil_tmp1x2_runner.py}` preserve correlated
  admission/terminal records and full result after cancellation.
- PCA9555 `examples/espidf_basic/main/main.cpp:664–683` and
  `tools/{run_i2c_hil.py,test_run_i2c_hil_parser.py}` use finite owner/session
  bounds; reject example ID wrap and retain primary versus recovery results.
- MB85RC `examples/common/DiagnosticCore.h:143–171`, native
  `examples/espidf_basic/main/main.cpp:1137–1170`, and
  `tools/{hil_runner.py,test_hil_health_snapshot.py}`. Reject the helper's loss
  of full result/effects when reducing cancellation to a generic timeout.
- FieldCore-node `src/cli/components/Ina228Cli.cpp:147–180`,
  `scripts/hil_i2c_startup.py:68–86,115–133` and
  `test/python/test_hil_i2c_startup.py:111` distinguish cached status, owner
  requests, queued admission and actual correlated terminal completion.

The lead independently read TMP admission/timing/retention and PCA poll/take
bodies, then reconciled the inventory against the exact contracts and examples.

## SDK and silicon evidence

Unchanged ESP-IDF v6.1 baseline `fff9895c82d744c7237be8847347bdd1b07c6643`:
original `components/esp_eth/src/phy/esp_eth_phy_802_3.c` and
`components/esp_eth/src/mac/esp_eth_mac_esp.c` were reopened outside the repository
and matched blobs `7d5d407d029c63494dffaaa3d82f8701b6b73790` and
`6448fcd6de4669ddf1e836c992ca8a1accc9dd96`.
BASE 133–149 writes reset even below the 10 ms poll quantum; 155–212 can return
success after AN expiry; 307–334 proves power-bit readback only; 485–506 includes
tick truncation. MAC 211–253 gives nominal polling iterations without a caller
deadline or proved wall-clock maximum. None supplies the selected aggregate bound.
`esp_eth_phy.h` confirms the native reset callback signature used in the design.

The source auditor reopened original SNLS647I pages 15,17,26–27,34 and SNLA266B
page 13, using original PDF coordinates for timing-column interpretation.
25 us assertion is MIN; 2 ms reset-release/50 ms POR SMI readiness occupy MAX
columns and become mandatory bus-silent waiting windows in 8.3.11. Delayed clock
adds the 100 us stability/reset condition. No software-reset maximum is inferred;
1500 ms FLP and EEE figures remain nominal. Clock-qualified board silence excludes
all bus participants, not just this address.

EEE's reset-bearing sequence includes unresolved printed wire/control/exit facts;
it rejects before the first reset until those gates close. SNLA266B table 5-5's
reset/forced-10M/COMPT stimulus is not arbitrary saved-word restoration authority.
Stop/reset primitives precede persistent replay and never recurse into the full
recipe or replay temporary stimuli. These conclusions preserve the ledger.

## Independent review and acceptance

Read-only model/path assignments:

- `p8_peer_audit`, gpt-6.1-sol/high: bounded current peer inventory and final
  requirement/vocabulary review.
- `p8_timing_audit`, gpt-6-astra/high: stronger SDK/silicon timing and restore
  evidence review.
- `p8_lifetime_review`, gpt-6-astra/xhigh: independent lifetime/deadline and
  clock-fault reasoning, then actual-diff review.

The lead alone wrote shared declarations. Review resolved minimum poll and cleanup
capacity before mutation, final-poll exhaustion, no-command applicability,
semantic target writes versus MMD selectors, initialized-at-admission behavior,
empty desired-profile recovery, first verification mismatch versus frozen primary,
retire-only bind rejection, next-poll validity and explicit error precedence.
Full native/plain-owner examples retain tokens on failed take. Independent
lifetime review closed with no blocking findings.
Final timing review also distinguished lower age bounds from maximum-age
freshness proof, including millisecond rounding; the arithmetic probe covers it.

The contract's exact call/status/state/effect recipes cover success/failure,
unplugged link, dead MDIO, wrong identity, zero/expired/wrapping/frozen clocks,
reset in flight, partial writes, cancellation, cleanup failure, late effects,
backpressure, exhaustion and end/rebind. Eight executable arithmetic probes
exercise synthetic strict-fit, whole-batch, poll-capacity, tick/lag, overflow,
finite-wait and reserved-cleanup cases. They are not a model PHY that reports
successful hardware behavior. First-consumer driver tests remain required at 15.

## Verification and documentation

Environment: Windows NT 10.0.26300.0, PowerShell 5.1.26100.9711, Python 3.12.10,
MinGW-w64 UCRT g++ 15.1.0.

| Evidence | Local result and scope |
| --- | --- |
| `python tools/verify_references.py` | PASS: 27 artifacts (43,240,385 bytes), 25 extracts and 1,303 local links in 150 Markdown files. |
| `python tools/verify_ledger.py` | PASS: 96 registers, 52 procedures, 35 features; structure/coverage only. |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS: 85 tests in 14.928 s (77 research tests plus eight arithmetic design probes). |
| Portable documentation C++17 syntax/static assertions | PASS: eight declaration/example blocks; no SDK includes or link claim. |
| `git diff --check` | PASS. |
| Native callback example | PASS source/signature review only; SDK compilation/link NOT RUN. |
| Host driver / portable-package consumer / physical qualification | NOT RUN: design stage, no working driver or fixture operation. |
| Remote CI | NOT RUN: reserved for prompt 71. |

After the final age-bound correction, the focused
`python -m unittest discover -s test -p test_operation_timing.py -v` run passed
all eight probes again; unchanged research tests did not need another run.

Portable documentation declarations/examples are extracted to one temporary
translation unit and checked with `g++ -std=c++17 -Wall -Wextra -Werror
-pedantic -fsyntax-only`, with ownership/token/default-state assertions. This
proves syntax only, not a linked portable consumer. The native block uses real
SDK signature/source review and explicit application helper declarations; it is
not included in the portable build and is not an SDK link result.

README/CHANGELOG, architecture/design, feature map, public-header plan, example
notes, conventions/source catalogue, test guide and progress were reconciled.
Feature-specific signatures still arrive with working behavior; no ledger or
physical-support claim changed. Existing prompt-37 lifecycle corrections and
silicon source gates remain implementation dependencies. The clock-fault cleanup
dependency from prompt 7 is resolved by this contract, not deferred again.

Host driver execution, portable/package consumer, SDK link and physical evidence
remain NOT RUN. Remote CI was not dispatched, queried or awaited; prompt 71 owns
it. No fixture action, release publication or sibling edit was performed.
