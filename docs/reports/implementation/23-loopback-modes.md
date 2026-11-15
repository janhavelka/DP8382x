# Prompt 23 - bounded loopback sessions

The subsequent [independent audit](23-24-independent-audit.md) corrects MII
line-transmit transition ordering and records integrated restoration regressions.

Date: 2026-11-15. Execution branch: `main`. Clean, fetched and synchronized
entry checkpoint: `9dee869f7e43beeea9f3c1837bf111029e29a74b`. Final commit and
upstream equality are recorded in the handoff, avoiding a self-referencing commit.

## Rules, ownership and delivered scope

Applied root/parent AGENTS, COMMON, prompt 23, shared conventions/evidence/full
coverage rules and OP-1 through OP-17. Prerequisites 4, 20 and 22 exist in the
ledger, single collector and existing operation/reconciliation implementation.
Owned ledger procedures: `loopback_near`, `loopback_digital`, `loopback_reverse`
and `loopback_external`.
The lead owns the public header, operation/configuration integration, new cohesive
diagnostics source, consumer, guides, ledger, report and Git. Tests have a disjoint
agent owner. Siblings and vendor files remain read-only. One application owner
serializes instance calls and complete bus transactions; locks never span waits.
Remote CI remains prompt 71. No SDK or fixture action was performed.

Implemented MII, PCS input/output, digital, analog, reverse and external sessions
with separate bounded start/stop tokens and retained results. Starts are bus/clock
silent. The application owns dwell and traffic; control verification is not packet
delivery evidence. Temporary overrides reuse the forced-profile implementation
without replacing desired settings. Stop uses existing initialization/profile
replay followed by saved ordinary BISCR/PHYSCR verification. Digital's opaque
companions use the exact allowlisted recipe and an explicitly admitted HARD exit;
arbitrary opaque values are never read or replayed. Explicit HARD initialization
uses the same restoration tail. No second MMD implementation or job engine exists.

The first attempted semantic mutation retains temporary state and cleanup debt,
including effect-free failure. Selector-only prefixes do not imply target writes.
Failure/cancel freezes the result; a later stop has its own token and budget.
Take/abandon do not erase temporary knowledge. Raw mutation, configuration and
verification cannot bypass the barrier. External reset/POR notification remains
bus-silent. Session callbacks count as raw transport, including stop replay;
explicit initialization remains managed. Cached readers perform no I/O.

## Source and sibling audit

Reopened original SNLS647I August 2025 section 8.3.14 and tables 8-19/35/40/48,
and SNLA423A June 2025 pages 18-20. The current official datasheet remains
[SNLS647I](https://www.ti.com/lit/ds/symlink/dp83826i.pdf). Detailed BMCR definitions
select the speed-specific MII companion. PCS input is 10 only; digital is 100
only. Analog requires termination; reverse requires compatible link and bounded
relink. Selected note scripts add reverse/analog restart. The reserved reverse
example and out-of-range analog gain recipe are rejected. No newer exact-part
clarification closes `loopback_recipe`; Q8 remains physical evidence work.

Fresh manufacturer main check: TexasInstruments/ethernet-rtos-drivers
`94bf4343004732983b0b8e1eec123d88ff70453b`,
`rtos_drivers/src/dp83826.c` (BSD-3-Clause). Near-loop routing disables Auto-MDIX;
unchecked helper completion is not accepted as a complete procedure. No source
implementation was copied. Vendor bytes, licenses and hashes are unchanged.

| Read-only sibling / revision | Reopened paths and decision |
| --- | --- |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | `include/PCA9555/PCA9555.h` result/cleanup types, `src/PCA9555.cpp`, `test/test_basic.cpp::test_failed_cooperative_input_read_preserves_primary_and_cleanup_evidence` and registration. Adopt independent primary/cleanup evidence; adapt cleanup to DP's explicit subsequent operation. Reject I2C pointer parking as PHY cleanup. |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/common/Tmp1x2CliDiagnostics.cpp` restore/result/cancel, `tools/test_hil_tmp1x2_runner.py` cleanup/reboot cases, source/operation declarations and agent rules. Adopt retained failure reporting and application ownership. Reject sensor timing and CLI rebind as a PHY restoration recipe. |

Both sibling worktrees were clean. Existing source policy, exact DP83826I variant
boundary and unrelated gates remain unchanged. EEE overlap at 0123 is explicit;
EEE is not implemented or implicitly enabled. BIST generator/checker work is 24.

## Independent review and fixes

Three capable inherited-model agents worked in parallel: `loop23_sources` for
original-source/sibling review, `loop23_contract` for ownership/deadline/restoration
review, and `loop23_tests` for `test/loopback.cpp` only. The source and contract
reviewers were read-only; no subagent edited siblings or Git. This separates
literal silicon review from production recipes and test implementation. Independent
builds used `build/loop23-review` and `build/loop23-tests`.

The lead checked findings against actual code and integrated these corrections:

- Preserve valid readback mismatch before a later release/deadline error.
- Withdraw cached activation proof when stopping or invalidating state.
- Reject reverse relink at a different negotiated speed.
- Restore BMCR ordinary extras after HARD may reload BASIC isolation straps;
  keep initialized false until the whole restoration tail passes.
- Preserve loopback mode/speed in explicit HARD-exit results.

Tests independently assert literal sequences, every mode/speed/interface,
semantic write order, exact frame/callback totals, every frame/release failure,
accepted-write timeout, cancellation boundaries, saved-field preservation,
manual MDIX/robust/mirror restoration, stale tokens, repeated sessions, clock
regression/overflow/frozen clocks and indivisible MMD admission. Cross-PHY
contention uses the existing transaction owner. Release assertions remain active.

## Documentation and verification

The [loopback guide](../../loopback.md) gives the complete matrix, prerequisites,
temporary-state barrier, bounds, exit policy and physical limitations. Updated
public Doxygen/API, initialization/reconciliation/health/access guides, architecture,
README/CHANGELOG, vectors, feature coverage, four ledger procedures, test guide
and progress. The portable example runs a session, retains a failed restoration,
then explicitly retries stop. Version stays 0.1.0; variant/tool/SDK plans need no
new implementation claims. No hardware support is inferred from host success.

Environment: Windows, Ninja, GNU C++ 15.1, C++17, Release.

| Evidence | Result |
| --- | --- |
| Host build/CTest, `build/loop23` | PASS, 118/118 registered tests |
| Undefined-behavior sanitizer trap build/CTest, `build/loop23-ubsan` | PASS, 118/118 |
| Independent review Release build | PASS, 118/118 |
| Focused loopback suites plus Release failure probe | PASS, 9/9; eight suites execute 791,325 checks |
| Separate tests-OFF portable consumer, `build/loop23-consumer` | PASS, 1/1; linked owner/session and failed-stop example |
| Python suites | PASS, 103/103; updated literal procedure-ownership regression for the newly implemented temporary profile and saved FIFO |
| Reference integrity | PASS, 29 vendor artifacts / 27 extracts; local Markdown links checked |
| Ledger structure/coverage | PASS, 97 registers / 52 procedures / 35 features |
| Doxygen | PASS, 165 generated pages and two rejected negative fixtures |
| Generated version and diff whitespace | PASS |
| SDK/controller link and remote CI | NOT RUN; no SDK change, first CI at 71 |
| Physical routing/termination/packet delivery | NOT RUN; Q8 plus existing Q5/Q11 remain open |

Source/host completion covers control procedures and restoration only. Opaque
digital exit can reset unowned hardware state; the owner explicitly admits that
reset. No fixture, electrical timing, MAC interoperability or BIST pass is claimed.
