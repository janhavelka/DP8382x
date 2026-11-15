# Prompt 24 - BIST generator and checker sessions

Date: 2026-11-15. Branch: `main`. Entry checkpoint, clean/fetched/synchronized:
`d58a7da95b0f3e96c0a908b70eb2ecaa439b1377`. Final commit and verified upstream
equality are recorded in the handoff rather than a self-referencing commit.

## Rules, prerequisites and scope

Applied root/parent AGENTS, COMMON, CONVENTIONS, ENGINEERING, EVIDENCE,
FULL_COVERAGE, SOURCE_MAP, VERIFICATION and API OP-1 through OP-17. Reviewed
README, architecture, variants, reference index and current audit dispositions.
Prerequisites are the stage-23 loopback/restore owner, stage-20 collector and
stage-4 diagnostic ledger. Owned procedure IDs: `bist_run`, `bist_capture`.
Lead owns shared header, existing diagnostics/driver/configuration integration,
consumer, CMake, docs/ledger and Git. The test agent owns only `test/bist.cpp`.
One application owner serializes instance calls and every full bus transaction.
No locks span waits; starts/cancel/cached access are bus/clock silent. Siblings,
vendor bytes and remote CI are untouched; CI remains prompt 71.

Shipped BistOptions/Mode/CounterMode/Observation/Result/Snapshot and bounded
start/acquire/stop in the existing slot. All four PRBS encodings, every packet
and gap field code, wrap/saturation and source-backed loopback interactions are
represented. No new test engine or collector. Normal-line operation does not
pretend a loopback fixture exists. Optional digital/analog/external paths reuse
the stage-23 implementation; no alternate recipe or analog gain workaround.

Each capture has its own operation token and retains partial raw evidence before
later failures. An indivisible command/read pair cannot be split by a poll slice.
Observation spacing is owner policy anchored after callback completion; current
deadline, overflow and frozen-clock rules apply. BIST traffic is RAW, including
stop replay; managed health remains independent. Desired settings remain intact.
Stop disables/checks generation, invokes the same reset/profile restoration and
verifies saved IPG/packet. Explicit HARD uses the same tail. Failed restoration
keeps the barrier and measurements; no retry, automatic cleanup or fake rollback.

## Fresh source and peer review

Original [SNLS647I](https://www.ti.com/lit/ds/symlink/dp83826i.pdf), August 2025:
section 8.3.15 p42, table 8-18 p54, table 8-40 pp68-69, 8-45 p72 and 8-46 p73.
Reopened [SNLA423A](https://www.ti.com/lit/an/snla423a/snla423a.pdf), June 2025,
2.5.3 pp19-20. Fresh official searches did not find exact-part latch clarification.
Vendor payloads/provenance/hashes remain unchanged. Independent literals include
capture807D, saturatedFF7D, packet05EE, IPG7D*4=500 and analog status3B08.
The note's3B04/status-address17/reverse20 examples do not override exact tables.
RH sync-loss is hardware-controlled, not RC. `030B=3380` stays rejected.

Manufacturer main remains `94bf4343004732983b0b8e1eec123d88ff70453b` in
TexasInstruments/ethernet-rtos-drivers. `rtos_drivers/src/dp83826.c` and
`dp83826_priv.h` are BSD-3-Clause; they do not implement complete BIST or resolve
capture lifecycle. No source implementation was copied.

Both sibling worktrees were clean and their agent rules reopened:

| Read-only source / revision | Actual paths and adaptation |
| --- | --- |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | `include/PCA9555/PCA9555.h` OperationResult, `src/PCA9555.cpp` cleanup, `test/test_basic.cpp:2120` primary/cleanup failure and registration2729. Retain distinct primary/cleanup evidence; reject automatic I2C pointer parking as PHY restoration. |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/common/Tmp1x2CliDiagnostics.cpp:84/150/157` result/restore handling; `tools/test_hil_tmp1x2_runner.py:326/1429/1450` token/partial-write/cleanup cases and CMake registration58. Adopt correlated retained results and cached rendering. Reject automatic restore/rebind and sensor timing for DP OP-9. |

`bist_result_semantics` still gates exact counter intervals, repeat-latch release,
continuous lock history and PASS. Explicit capture/read yields raw evidence;
writing zero to unlatch is not invented. Complete zero-error captures remain
INCONCLUSIVE, even with apparent activity/lock. Nonzero captures and separately
correlated single packets have distinct evidence outcomes. Saturation is a lower
bound and wrapping cannot establish totals. All packet codes are documented raw
settings, not proof of Ethernet-valid frame lengths. Physical BIST qualification remains NOT RUN.

## Delegation and review corrections

Three inherited capable-model agents were selected for current code, independent
silicon reasoning and focused tests: `bist24_sources` read-only source/peer audit,
`bist24_contract` read-only lifecycle/ownership review, `bist24_tests` independent
literal/failure tests only. Separate builds: `build/bist24-review` and
`build/bist24-tests`. No subagent edited siblings or Git. Lead reviewed findings
against actual code and reviewed test contribution and final diff.

Confirmed and fixed findings:

- Save IPG/packet before any loopback mutation; failed digital setup can need
  HARD and must not lose the original controls to reset defaults.
- Require generator-inactive evidence on start and bounded stop, not mode bits
  alone. A sticky active flag cannot reach restoration success.
- Retire pending-capture state on reset/POR/bind/end so a fresh collector epoch
  cannot acquire false lost-history evidence from an older failed capture.
- Use explicit unsigned promotion for mode/counter shifts under strict sanitizer
  builds; no warning suppression or weakened test was introduced.

Tests retain mismatches even with later release/deadline error, inject every frame
and release failure and every cancellation boundary, prove no retries and exact
bounds, preserve measurements through failed stop/HARD, and exercise repeated
sessions, stale tokens, clock/budget edges, cached readers and cross-PHY contention.

## Documentation and verification

[BIST guide](../../bist.md) records the full matrix, procedure, partial-evidence
and source-gated outcome contracts. Public Doxygen, API, architecture, collector,
health, initialization, loopback, device behavior, vectors, feature/ledger owners,
README/CHANGELOG, test guide and progress are updated. Version remains0.1.0;
exact supported variant and SDK/tool/CLI plans have no new implementation claims.
The tests-OFF portable consumer compiles the bounded owner example, acquires once,
copies two cached views without I/O, then separately stops.

Environment: Windows, GNU C++15.1, Ninja, C++17, Release, active CHECK assertions.

| Evidence | Result |
| --- | --- |
| Host CTest, `build/bist24` | PASS,127/127 |
| UBSan trap build/CTest, `build/bist24-ubsan` | PASS,127/127 |
| Focused BIST suites/failure probe | PASS,9/9; eight suites528,009 checks |
| Independent reviewer build | PASS,127/127 before final review fixes; final fixes covered by lead/focused suites |
| Separate portable consumer, `build/bist24-consumer` | PASS,1/1 |
| Python research/tool tests | PASS,103/103 |
| Doxygen | PASS,175 generated pages and two rejected negative fixtures |
| Ledger and version checks | PASS,97 registers/52 procedures/35 features; generated version current |
| References and diff | PASS,29 vendor artifacts/27 extracts, local links and whitespace |
| SDK/controller link, remote CI | NOT RUN; no SDK implementation change, first CI at71 |
| Physical BIST qualification, routing/packet/termination/electrical qualification | NOT RUN; source gates explicit |

Maintainability review: one diagnostics source, existing loopback/profile/reset
procedures, one collector, fixed values and explicit phase switches. No allocation,
platform clocks, scheduler, second transaction mechanism or unbounded wait added.
Source-backed control/raw-acquisition scope is complete; PASS/rate/interval claims
remain deliberately unavailable pending the named exact-part evidence.
