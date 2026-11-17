# Independent audit of prompt 36

Fresh audit of the complete [original prompt](../../prompts/36-primary-feature-closure.md)
against current source, tests and the actual stage diff, starting from clean,
fetched `main` at `67e5e97cee016f57177d0496f1c47a51bd77821b`, equal to upstream.
Three confirmed behavior defects were fixed; the bounded audit is delivered.
**Primary-chip closure remains BLOCKED** by the required source branches already
recorded in the [single coverage matrix](../../feature-coverage.md).

## Rules, independent scope and evidence

Reread COMMON, repository/parent rules and relevant shared contracts. The lead
owns source/header integration, test changes, shared guides and Git. Keep one
application-controlled MDIO owner, terminal batches, shared collector and reset
restoration. Siblings and vendor payloads remain read-only. No new API, state
framework, retry, physical qualification or downstream stage execution was added.

Three parallel read-only reviewers used the current capable model. Disjoint
scopes concentrate reasoning on reset/effect lifetime, destructive observations,
and coverage/source claims rather than repeating the entire inventory:

- `audit36_reset`: own-poll and interleaved reset observations, partial batches,
  retained results, configuration/collision uncertainty, and removal of duplicate
  invalidation. Independently reproduced both reset bugs and rechecked the fix.
- `audit36_observation`: EEE/link quarantine, collector/IRQ partial evidence,
  sibling diagnostic contracts and regression quality. Independently reproduced
  the stale-link bug and approved the final source/test/Doxygen diff.
- `audit36_coverage`: complete prompt and actual stage diff, matrix/progress,
  gates, original-PDF samples, Release registration and sibling precedents.
  Found no additional unsupported coverage or completion claim.

Original SNLS647I tables 8-19/48 establish reset-in-progress and reset domains;
tables 8-37..39 retain MISR2/FCSCR semantics and the unresolved RECR contradiction.
Independent LED selector/placement and DPD samples agree with the recorded
dispositions. Source bytes/hashes and exact revision are unchanged from the
[stage source record](36-primary-feature-closure.md#source-and-sibling-evidence).
No production mask served as the expected reset value in the new tests.

Fresh sibling inspections rechecked the same clean revisions and paths recorded
there: TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` Config/Status,
operations/configuration, public snapshots, CLI/jobs/runner parser; PCA9555
`8b461b2afe9910b5b15fe32cc049d52e3b498970` cleanup/shadow source, ambiguity,
cancel and retained-result tests, cached health CLI/parser; INA228
`4de84f2564eb6382835ca9a72d8688377657dcb4` diagnostic declarations,
readDiagAlert/capture, destructive-read tests/registration and cached CLI/parser.
Retained effects and staged publication remain suitable precedents. I2C
ACK/pointer rules, sensor timing and automatic CLI recovery remain inapplicable.

## Findings, corrections and independent regressions

1. **Quarantined link refresh falsely remained fresh.** After a valid sample,
   malformed unrelated transport evidence could quarantine the controller. The
   next link refresh returned BUSY but left cached freshness/status unchanged.
   It now uses the existing failure path: preserve historical words/time, mark
   stale, retain exact status, and leave ordinary output unchanged. Repeated
   rejection adds no duplicate STALE transition, callback or health accounting.
   The new `link_failures` case failed six checks before the fix and passes now.
2. **Configuration could succeed after its own reset observation.** Shared
   retention invalidated identity, but preflight/RMW/readback could continue and
   restore initialized/VERIFIED state. Configuration now stops on the consumed
   reset before advancing. Primary transport/release/clock errors take precedence;
   otherwise CONFIGURE reports INVALID_CONFIG/DEVICE_INVALIDATED. VERIFY keeps
   BUSY/OPERATION_ACTIVE. Any write already accepted in the indivisible RMW keeps
   its effects and cleanup debt. New registered
   `verify_configuration_reset_observation` covers 26 combinations: negotiated
   and forced profiles, all three BMCR phases, success/release/deadline/clock
   outcomes and accepted-write timeout. It failed 48 checks before the fix.
3. **Collision entry/exit could hide reset in its RMW.** A valid BMCR reset read
   followed by a command-neutral write could produce successful later readback
   and verified controls despite invalidated identity. The collision phase now
   stops after that batch, preserving primary failure and temporary-state debt.
   Ten new cases in `electrical_collision` cover both entry/exit and the same
   failure classes. They failed nine checks before the fix and pass now.

Removed duplicate initialization and VERIFY reset-invalidation calls already
covered by shared retention. Actual command-effect handling and expected bounded
reset waits remain unchanged. Existing initialization/replay, pending-result,
power/interface and diagnostic tests exercise those distinctions. No broad
refactoring or second evidence path was introduced.

## Coverage and remaining gates

The previous matrix corrections are valid. Essential `power_deep` work remains
with prompt 28; pin conflicts with 32/33 and unresolved electrical encodings with
35 retain their exact next-evidence requirements. Prompt 55 still owns full row
closure. RECR, MSE, TDR calibration, strap and physical timing gates remain open.
No false-success issue was reproduced in sampled interface replay: its shared
restoration guard rejects invalidated state. No further confirmed defect was
found in the scoped collector/IRQ or other reset-guard review.

Reviewed README/CHANGELOG, public Doxygen, API/link/electrical/test guides,
stage report, matrix and progress. README needs no change: its bounded-review
and source-blocked claims remain accurate. Numeric ledger/vectors, architecture,
variants, licenses and version/manifests need no change; no encoding, supported
part, external dependency or source gate changed.

## Verification and checkpoint

Windows, GNU C++ 15.1.0, Ninja, C++17 Release, strict warnings. Rebuilt the host,
UBSan and tests-OFF portable consumer targets. Focused selection covers all
modified behavior, shared configuration users and representative restoration;
unchanged broad Python/package/SDK suites were not rerun.

- PASS: 96 selected registered CTests in `build/host` and the same 96 in
  `build/electrical35-ubsan` (undefined-behavior sanitizer/trap mode).
  Selection: `^(link_|verify_|reset_|operation_|negotiation_|forced_|electrical_|loopback_|bist_|health_|quality_ownership$|interface_replay$|eee_replay$|power_replay$|foundation_)`.
- PASS: both linked tests-OFF consumers in `build/electrical35-consumer`.
- PASS: all three affected Release failure-path probes, confirming assertions
  execute; Doxygen checks (264 pages, links/assets and two negative fixtures).
- PASS: reference/hash/local-link verification, ledger structural coverage
  (97 registers, 52 procedures, 35 features), generated version check and
  final whitespace/diff review.
- NOT RUN: unchanged Python suites, SDK/controller integration, installed/exported
  package validation, remote CI (reserved for 71), and physical campaigns. Host
  callbacks prove software evidence handling, not reset timing or traffic effects.

The final handoff records the checked commit and fetched upstream equality,
avoiding a self-referencing commit entry. Required source and physical gates
remain distinct from completion of this bounded audit.

## Current-code response — 2026-11-17

Revalidated all three behavior findings and every remaining-gate assertion above
against `main` entry `77c544f415edb26a0e23de76591a0163b9b73147`, prompt 36's
bounded integration scope, current source/tests and the maintained coverage matrix.

| Finding or proposal | Current verdict and evidence |
| --- | --- |
| Quarantined link refresh remained fresh | **Confirmed implemented.** `src/DP8382xLink.cpp::readLinkStatus` sends quarantine to the existing failure lambda; `_invalidateLink` records only one STALE transition. `test/link.cpp::failures` checks repeated rejection, original words/time, untouched ordinary output and no callback/clock/health changes. |
| Configuration continued after its own reset read | **Confirmed implemented.** `src/DP8382xConfiguration.cpp::_advanceConfiguration` retains read evidence, returns primary transport failure first, then stops on a valid BMCR reset before phase advancement/trust promotion. `test/verify.cpp::configurationResetObservation` covers negotiated/forced preflight, RMW and readback with success/release/deadline/clock/accepted-write errors. Already accepted RMW effects retain cleanup debt. |
| Collision RMW hid an observed reset | **Confirmed implemented.** `src/DP8382xElectrical.cpp::_advanceElectrical`, COLLISION_WRITE, checks the retained reset prefix before accepting later control proof and after primary-error handling. `test/electrical.cpp::collision` covers start/stop and all five outcome classes, retaining possible stimulus and cleanup. |
| Remove duplicate initialization/VERIFY invalidation | **Confirmed implemented.** The common collector now owns valid read-prefix invalidation; shared init phases inspect command completion without a second independent evidence path. `test/verify.cpp::resetObservationErrors`/`rawResetObservation` and reset suites retain domain distinctions and frozen results. |
| Claim primary closure from the bounded review | **Incorrect; closure still open.** Prompt 36 and `docs/feature-coverage.md` retain required missing DPD and unresolved pin/electrical branches; prompt 55 owns final row closure. `src/DP8382xPower.cpp::_validatePower` rejects DPD before I/O, which is safe rejection rather than a completed entry/exit implementation. |
| Interface replay false success or further collector/IRQ defect | **Not confirmed by this finding revalidation.** `_finishOperation` performs the shared final owned-control proof; `test/interface.cpp::replayReadiness`/`replay`, interrupt audit regressions and the new reset cases pass. This does not expand the historical sampled review into an unbounded correctness claim. |

Reopened original SNLS647I tables 8-19/48 and 8-37/38/39 directly from the
archived PDF. They support the distinct reset domains and retained consuming
evidence. RECR still states a 16-bit field with FF/7F saturation prose; no
arithmetic resolution is invented. Section 8.3.6.3 still provides DPD entry
without the full exit/preservation evidence required by `deep_power_exit`.
The pin, MSE, TDR and strap gates remain source/physical questions with the
owners and next-evidence requirements recorded in the maintained matrix.

Fresh host verification on Windows/GNU C++ 15.1/Ninja, C++17 Release:
`build/audit-current-response-silicon`, **PASS 236/236 CTests**, including all
named corrected suites. Package/SDK/CI/physical checks were **NOT RUN** by this
response review. No new production correction was confirmed; primary-chip
closure remains **BLOCKED**, independently of the completed review and passing
host evidence.
