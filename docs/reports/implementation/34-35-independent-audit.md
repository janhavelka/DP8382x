# Independent audit of prompts 34 and 35

Fresh audit of the complete [quality prompt](../../prompts/34-quality-link-loss.md)
and [electrical prompt](../../prompts/35-electrical-compliance-calibration.md),
starting on clean, fetched `main` at
`c292dc75658b01d224617548d896f88fb8c6ef71`, equal to `origin/main`.
The original reports describe historical checkpoints; this report records the
subsequent audit and corrections against current source and executing tests.

## Scope and independent review

Read COMMON, shared operation/convention/evidence/coverage contracts, repository
and parent instructions. The lead owns shared integration, guides, progress and
Git. One application owner still serializes instance access and complete MDIO
transactions; no lock spans a wait. Siblings and vendor payloads stayed read-only.
Remote CI remains reserved for prompt 71.

Three parallel agents used the current capable model for disjoint review:
`audit34_quality` reviewed quality fields, snapshots, sources and tests;
`audit35_electrical` reviewed electrical recipes, access and session effects;
`audit3435_lifecycle` reviewed shared replay, deadlines, cancellation and tests.
These scopes separate silicon interpretation from lifecycle integration. After
read-only review, narrowly assigned source/test fixes stayed in the corresponding
quality/electrical files; the lifecycle agent owned only `test/analog.cpp`.
Independent build directories avoided build collisions. The lead checked the
reported reproductions and actual diff, and made shared-source corrections.

Reopened original SNLS647I August 2025, SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
including section 8.3.17, quality tables 8-29/30/52/53/72, TX10/PHYSCR fields,
and electrical tables 8-19/47/48/50/73/76/79..83/86. Rechecked applicable
SNLA266B April 2025 page 13 tables 5-1/2/4/5, SHA256
`74af41cba6912b8734097332b7e766105feaa8fb015321f47d345b7596aa13d5`;
DP83822-only table 5-3 remains excluded. Independent literals matched the
implemented masks and applicable recipes. No numeric silicon correction or new
source permission was justified.

Fresh sibling inspections retained these clean revisions (paths relative to
Projects):

- INA228 `4de84f2564eb6382835ca9a72d8688377657dcb4`:
  `include/INA228/INA228.h`, `src/INA228.cpp` diagnostic validity/readDiagAlert,
  `test/test_basic.cpp`, `examples/esp_idf/basic/main/main.cpp` and
  `tools/test_run_i2c_hil_parser.py`.
- TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`:
  public snapshot/result declarations, `src/TMP1x2Configuration.cpp`,
  `src/TMP1x2Operations.cpp`, `test/test_core.cpp`, `test/jobs/test_jobs.cpp`,
  `test/cli/test_cli.cpp`, common CLI/diagnostic source and
  `tools/test_hil_tmp1x2_runner.py` with registrations.
- PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970`:
  `src/PCA9555.cpp`, registered primary/cleanup, ambiguous-write, cancellation
  and detach tests in `test/test_basic.cpp`, cached
  `examples/common/HealthDiag.h` and fixture/parser cases.
- LDC1614 `da104689de49ab8f492e7905f3eb7496f0866624`:
  public declarations, `src/LDC1614.cpp`, `test/test_basic.cpp`,
  `examples/common/Ldc1614Cli.cpp` and `tools/test_ldc1614_hil_runner.py`.

Adopt retained partial evidence, staged publication and passive rendering.
Sensor timing/scaling, I2C ACK/pointer cleanup and CONFIG bookends do not specify
PHY behavior or simultaneous sampling. No sibling implementation was copied.

## Confirmed findings and corrections

| Finding | Correction and regression evidence |
| --- | --- |
| Quality preflight observed active reset but retained accepted identity/readiness. | Shared evidence retention now includes quality operations before later frame/release failure. Tests retain the reset observation under successful read, later read failure and release failure. |
| Quality preflight could observe BMCR contradicting a verified persistent mode. | Route the observation through existing owned-mode comparison; reject before mutation and preserve the mismatch and dirty trust. |
| Quarantined-controller MSE rejection left the old cached sample apparently fresh. | Mark it stale with the rejection status, retaining its raw word and original time. Caller output and bus/clock counts remain unchanged. |
| Completed analog replay remained selected while later owned restoration was pending. | Clear its dispatcher flag on success before shared handoff. Test pins, WoL, interrupts and power individually and together, read-only verification, every downstream frame failure and cancellation around handoff. |
| External reset/POR cleared generic cleanup but left current electrical session/HARD-debt flags. | Clear current electrical obligations under the existing external-reset contract; preserve frozen historical results. Test both reasons and calibration/compliance/collision states with zero callbacks. |
| Initial compliance HARD alone was classified as active temporary stimulus. | Only attempted non-reset companion/profile/stimulus writes activate that evidence. Literal trace tests retain reset effects, floor and cleanup without inventing stimulus, including timeout/cancellation and subsequent NONE reconciliation. |
| An interleaved admitted raw reset observation could leave a maintenance operation running from pre-reset snapshots. | Freeze the interrupted operation after retaining the valid observation. Quality and analog tests prove no subsequent dependent writes, with earlier effects and cleanup retained. |

The last issue was independently reproduced after the first fixes: BMCR `9000`
observed between analog polls invalidated initialization but allowed three more
analog writes. The corrected path returns the raw word successfully while the
operation retains a separate `DEVICE_INVALIDATED` result. This distinction
preserves transport accounting and does not reinterpret a good read as failure.

## Coverage and maintainability

Rechecked all five quality procedures (`configure_fast_drop`,
`signal_quality_sample`, `tx100_configure`, `tx10_configure`,
`link_recovery_configure`) and all four electrical procedures
(`analog_configure`, `collision_test`, `compliance_session`,
`calibration_recovery`) against their source restrictions, current source,
registered failure/boundary suites and portable examples. Existing literal
field/extrema, legal split-field transition, collector, reset, stop and finite
budget tests remain active. New tests extend existing suites and do not derive
numeric expectations from production masks.

Corrections use existing evidence helpers and the shared operation switch. No
public API, allocation, retry, extra controller path, generic engine or new
production file was introduced. Reviewed the final diff for scope, retained
primary/effect evidence, command neutrality and bus-silent cached behavior.
Guides, API contract notes, CHANGELOG, test guide and progress were updated.
README needs no change: implemented stage/feature availability is unchanged.
Public Doxygen signatures and source citations remain accurate; examples are
rebuilt through the consumer. Ledger, source vectors and coverage ownership need
no factual changes because no fields, recipes or support gates changed. Version,
manifests, licenses and workflow policy are unchanged.

## Verification and remaining limits

Final checks on Windows with GNU C++ 15.1.0, Ninja and C++17 Release:

- PASS: `cmake --build build/host` and all 229 registered CTests, including
  active Release assertion probes and the new regressions.
- PASS: separate `build/electrical35-ubsan` build and all 229 CTests with
  `-fsanitize=undefined -fsanitize-undefined-trap-on-error`.
- PASS: tests-OFF `build/electrical35-consumer` build and both registered
  portable consumers, including the compiling quality/electrical examples.
- PASS: 103 Python unit tests; ledger validation of 97 registers, 52 procedures
  and 35 features (structural evidence, not silicon execution).
- PASS: reference/vendor hash/local-link checks, deterministic version check
  and `git diff --check`.
- PASS: Doxygen, 264 generated HTML pages, local links/assets and two rejected
  negative fixtures.

Source gates `mse_address`, `fld_reset_defaults`, `fld_threshold_recipe`,
`ioctrl_example`, `analog_rx_gain`, `compt_reserved` and `hard_reset_straps`
remain open. Host protocol tests do not establish physical latency, MSE scaling,
electrical compliance, COL timing or resistor calibration. SDK/controller link,
physical measurements and remote CI are NOT RUN in this audit. No fixture was
operated. The final handoff records the verified commit and upstream equality,
avoiding a self-referencing commit in this report.

## Current-code response — 2026-11-17

Revalidated all seven defects and retained source gates against `main` entry
`77c544f415edb26a0e23de76591a0163b9b73147`, current prompts 34/35, OP-9/OP-13
and the actual source/tests. No historical pass count is reused as a fresh result.

| Finding | Current verdict and evidence |
| --- | --- |
| Quality reset preflight retained identity/readiness | **Confirmed implemented.** `src/DP8382x.cpp::_retainEvidence` invalidates a completed BMCR reset observation before later failure, for all owners. `test/quality.cpp::prerequisiteEvidence` checks successful read, later frame failure and release failure, with zero quality writes. |
| Quality observed mode contradiction remained trusted | **Confirmed implemented.** The same collector calls `_retainOwnedMode` for quality/analog/electrical work; `src/DP8382xConfiguration.cpp::_retainOwnedMode` records mismatch and dirty trust. `prerequisiteEvidence` verifies the BMCR 3100 mask, pre-mutation rejection and retained intent. |
| Quarantined MSE rejection left cached freshness | **Confirmed implemented.** `src/DP8382xQuality.cpp::readSignalQuality` routes quarantine through its failure helper, preserving word/time but setting stale/status. `test/quality.cpp::raw` verifies unchanged ordinary output and zero callback/clock activity. |
| Completed analog pass captured later dispatch | **Confirmed implemented.** `src/DP8382x.cpp::pollOperation` clears `_analogWorking` on successful analog completion before `_finishOperation` selects the next owned tail. `test/analog.cpp::downstreamReplay` covers each later owner/all together, VERIFY without writes, every downstream frame failure and cancellation at the handoff. |
| External reset/POR retained electrical session/HARD debt | **Confirmed implemented.** `invalidateDeviceState` clears current electrical/calibration/session obligations for the two explicit reset reasons. `test/electrical.cpp::externalResetReconciliation` checks bus-silent state reconciliation and immutable previously frozen results. The caller still owns truthful external-reset/readiness evidence. |
| Initial compliance HARD incorrectly counted as stimulus | **Confirmed implemented.** `src/DP8382xElectrical.cpp::_retainElectricalEffects` requires an attempted non-PHYRCR companion/profile/stimulus write before marking temporary activity. `test/electrical.cpp::resetBeforeStimulus` retains reset effects, quiet floor and cleanup while excluding stimulus, and exercises later NONE reconciliation. |
| Interleaved raw reset let maintenance use old snapshots | **Confirmed implemented.** `_retainEvidence` freezes a running operation after an independently observed reset, while the raw call retains its own transport result. `test/quality.cpp::ownership` and `test/analog.cpp::ownership` cover pre/post-effect boundaries with no later dependent writes. |
| Broader electrical/quality scope and unsupported encodings | **Implemented for admitted procedures; named gates still open.** `DP8382xQuality.cpp`, `DP8382xAnalog.cpp`, `DP8382xElectrical.cpp` and their vectors/admission/replay suites implement all nine procedures listed above within the ledger restrictions. `mse_address`, `fld_reset_defaults`, `fld_threshold_recipe`, `ioctrl_example`, `analog_rx_gain`, `compt_reserved` and `hard_reset_straps` are not closed by tests or source-gated rejection. |

Reopened original SNLS647I tables 8-19/47/48/52/53/79/80/81 and SNLA266B
page 13 tables 5-1/2/4/5. The calibration text requires CDCR 14 deassertion
immediately followed by HARD; it does not report calibration success. The
DP83822-only 7108 recipe remains excluded; exact DP83825/6 table 5-4 uses 7101.
The existing source-qualified composition and split-field ranges remain justified.
No new silicon value or generic reserved-write permission is needed.

Fresh verification: Windows/GNU C++ 15.1/Ninja C++17 Release,
`build/audit-current-response-silicon`, **PASS 236/236 CTests**, including every
quality/analog/electrical suite and Release probes. SDK/package/remote CI and
physical measurements were **NOT RUN** by this review. Electrical compliance,
COL timing, calibration accuracy and MSE scaling remain unmeasured. No further
production defect was confirmed in this finding set.
