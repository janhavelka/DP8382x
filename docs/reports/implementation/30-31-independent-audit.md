# Independent audit of prompts 30 and 31

The audit started on clean, fetched `main` at
`12a17284831dc1ae7a6b2e3584f551eed73f5f25`, equal to `origin/main`.
Both original prompts were read in full alongside COMMON, the shared evidence,
coverage, naming and OP contracts, repository guidance and current public API.
The actual stage commits, current source and registered tests were reviewed;
earlier completion summaries were not used as correctness evidence.

## Scope and independent review

Three read-only agents used the inherited capable reasoning model for separate
LPI, WoL and cross-feature ownership reviews. This kept silicon interpretation,
failure handling and tests independent while the lead owned all fixes and Git.
`audit30` built seven LPI suites in `build/audit30`; `audit31` reproduced route
conflicts; `audit_cross` built separate executable probes in `build/audit-cross`.
Agents edited no tracked files or siblings. No workflow exists yet; remote CI
remains prompt71. One MDIO owner, collector, operation slot and shared restoration
path remain unchanged.

Source review reopened original SNLS647I section8.3.4/page27, timing table7.6
and tables8-90 through8-97, plus the GPIO mux tables. The current
[official TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) still identifies
SNLS647I, August2025. Independent table checks found no numeric correction:
LPI owns only bit12; all four EEE timing values are nominal; WoL bit11 is W0C;
MAC/password packing and route masks agree with the existing independent literals.
Original vendor files and provenance were not changed.

Fresh read-only sibling checks included MB85RC `3ace168` sleep/wake uncertainty
in `src/MB85RC.cpp` and public SleepState; PCA9555 `8b461b2` cleanup/shadow
implementation and `test/test_basic.cpp` primary/cleanup cases and registration;
INA228 `4de84f2` readDiagAlert, retained capture and cached getter; and TMP1x2
`afd5088` operation/result and restoration source/tests. Retained uncertainty,
staged output and separate cleanup evidence remain useful precedents. I2C ACK,
memory wake delays and sensor timing remain inapplicable to the PHY.

## Findings and corrections

1. **A known WoL mismatch could precede one more mutation.** A caller could
   observe changed credentials between the final verification and enable polls.
   The mismatch was retained, but checked only after the next batch, allowing
   enable of a known-wrong match. The existing independent-observation stop rule
   now includes WoL. Regression tests cover all ten post-credential boundaries,
   require immediate retained failure and prove that later polls issue no frames.
   These tests failed before the correction. An independent probe and registered
   regression tests also verified
   that a raw read's release error or late completion remains its own error and
   leaves ordinary output unchanged while the operation retains the mismatch.

2. **Disabled detection incorrectly released route admission.** WoL still
   owned/restored its selected mux when disabled, but IRQ and EEE admission
   considered it free. Conflicting requests replaced desired intent, then failed
   during application and could prevent restoration. Route checks now apply
   independently of detector enable. Disabled STATUS_ONLY has no route
   requirement; a selected INTERRUPT route keeps its WAKE delivery contract.
   Explicit STATUS_ONLY releases the requirement without claiming to disconnect
   an old physical mux. Tests cover enabled/disabled direct pins, all four IRQ
   delivery dependencies and LED1/EEE conflict, with unchanged intent/token and
   zero callbacks on rejection. These tests failed before the correction.

3. **Failed operation reads could retain current-valid flags.** After an earlier
   successful configuration, a failed requested control, credential or mux read
   left its old validity flag set. Aggregate staleness was correct, but per-word
   validity differed from live acquisition. Operations now clear the failed
   requested word's validity and keep its raw history. Valid consumed prefixes
   survive later write/release errors. Twelve regressions cover all four indirect
   phases for control, credential and pin reads; all failed before the fix.

No confirmed LPI defect was found. Review covered nine-frame/two-callback bounds,
cached admission, no-wait cancellation semantics, bit12-only stop verification,
uncertain accepted writes, unrelated restoration debt, reset reconciliation,
quiet floors, link invalidation and passive snapshots. Q2 remains physical work.

## Documentation and verification

Updated the public route contract, WoL guide, README, CHANGELOG and progress.
Reviewed API, LPI, architecture, feature and source guides: no new APIs, fields,
numeric interpretations or feature ownership require changes there. The ledger
and independent source vectors remain valid. The fixes reuse existing guards
and fixed storage; no additional manager, state machine or retry was introduced.

Verification on Windows, GNU C++17 Release/Ninja with active assertions:

- PASS: 187 registered host CTests, including the LPI/WoL suites and deliberate
  Release failure probes.
- PASS: separately configured tests-OFF portable consumer, two CTests.
- PASS: 103 Python tests; ledger and reference/local-link validation; generated
  Doxygen and its negative fixtures; version check; `git diff --check`.
- NOT RUN: SDK/controller, installed package and physical qualification.
  Physical W0C/RH-bit behavior, live traffic races, pulse routing, partner wake
  and actual application wake remain Q2/Q3 evidence, not host-test conclusions.
- NOT RUN: remote CI, reserved for prompt71.

The final checked commit and verified upstream equality are recorded in the
handoff, avoiding a self-referencing commit in this report.

## Current-code response — 2026-11-17

Revalidated all three findings and the LPI no-defect conclusion against `main`
entry `77c544f415edb26a0e23de76591a0163b9b73147`, current prompts 30/31, source
and registered tests. The stage checkpoint above remains historical.

| Finding or proposal | Current verdict and evidence |
| --- | --- |
| Known credential mismatch permitted another mutation | **Confirmed implemented.** `src/DP8382x.cpp::_retainEvidence` includes `_wakeWorking` in the independent-observation freeze guard. `src/DP8382xWake.cpp::_retainWakeWord` keeps the first mismatch. `test/wol.cpp::ownership` covers all ten post-credential boundaries and raw-read release/deadline errors, with no subsequent enable/frame. |
| Disabled detector released its still-owned route | **Confirmed implemented.** `_validateInterruptSettings` checks the retained WoL route independently of enabled; EEE admission also rejects a conflicting selected route. `_validateWake` keeps STATUS_ONLY distinct from selected physical routing. `test/wol.cpp::routing`/`routeChanges` exercise disabled direct routes, IRQ delivery dependencies and EEE/LED1 conflict without intent/token mutation. |
| Failed requested reads reused old validity | **Confirmed implemented.** `_advanceWake` clears control/credential/pin validity for an incomplete target-read prefix, retains historical words, then preserves any valid prefix before later error handling. `test/wol.cpp::failures` covers all four indirect failure positions for these words. |
| LPI bounds, cancellation, bit12 stop and unrelated restoration debt | **Confirmed protected, no additional correction justified.** `src/DP8382xEee.cpp` implements `startLpi`, `stopLpi` and the shared LPI phases; `test/lpi.cpp` traces/admission/failures/recovery/ownership/timing suites retain accepted uncertainty, finite two-callback/nine-frame work and passive cached state. Prompt 30 leaves MAC drain, dwell and post-exit policy with the application. |
| Use nominal EEE timings as a completion/readiness maximum | **Incorrect.** The original rendered SNLS647I page 15 places 210 us, 20 ms, 36 us and 200 us in NOM, not MAX. Section 8.3.4 gives bit12 entry/exit and says the PHY has no buffering; it does not supply a readiness event or guaranteed partner wake deadline. |
| Replace exact-part W0C or change MAC/password order using a sibling recipe | **Incorrect/unsupported.** Reopened original tables 8-90 through 8-97 retain W0C bit11 and the distinct byte labels; current prompt 31 and `test/wol.cpp::vectors` use asymmetric literal wire words. `_wakeControlBatch` in `src/DP8382x.cpp` writes one as neutral and zero only for acknowledgement after collector harvesting. |

The archived original PDF sections/tables above were read directly, including
rendered timing columns. No changed silicon constant, source permission or vendor
payload is warranted. Physical W0C/RH behavior and indication delivery remain Q3;
LPI traffic/wake timing remains Q2.

Fresh verification: Windows/GNU C++ 15.1/Ninja C++17 Release,
`build/audit-current-response-silicon`, **PASS 236/236 CTests**, including all
LPI/WoL suites. SDK/package/CI/physical campaigns were **NOT RUN** by this review.
No new production gap was confirmed in this finding set.
