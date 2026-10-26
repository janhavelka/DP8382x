# Core prompt convention follow-up

Observed 2026-10-26. This is a source audit and maintenance of implementation
prompts 06-14, not driver implementation or a sibling test run. Siblings were
read-only. The implementation stages remain NOT STARTED. Paths below are
relative to `Projects/`; current sources must be reopened when a stage executes.

## Revisions and inspection scope

| Repository | Observed HEAD | Worktree at inspection |
| --- | --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | Clean |
| INA228 | `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b` | Clean |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | Clean |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | Clean |
| 24Cxx | `936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e` | Clean |

Read parent and local AGENTS, each listed sibling AGENTS, baseline architecture,
variants, reference index and shared prompt contracts. Actual implementation,
declarations and selected native assertions were inspected, with focused CLI,
Python and CMake/workflow source review. No remote CI results were requested.
This was not an exhaustive code review of five repositories. SCD41, RV3032-C7,
LDC1614 and other stage-specific references remain explicit future live-audit
targets; their behavior is not asserted from a filename or historical report.

## Observed behavior and selected adaptations

| Actual path and symbols | Observed fact | DP8382x adaptation or rejection |
| --- | --- | --- |
| `TMP1x2/include/TMP1x2/Status.h`: `Err`, `Status`, `errorName` | Status contains code/detail/msg; ok/is/inProgress/explicit bool and Ok/Error helpers; unknown enum prints UNKNOWN | Adopt shape/helper names and static message lifetime. Use selected MDIO errors; no I2C NACK enum or copied ordinals/aliases |
| `TMP1x2/include/TMP1x2/Config.h`: callback aliases, `Model`, `isValidModel`, `isValidAddress` | Terminal borrowed callbacks; declared model and pure range checks; no readable sensor chip ID | Adopt pure validation and explicit declaration/observation distinction. Reject sensor address rules and treating probe as PHY identity proof |
| `TMP1x2/src/TMP1x2.cpp`: `read`, `write`, `track`, `healthSnapshot`, `markConfigurationDirty` | Reads stage output; wrappers attribute health; dirty state is separate; health has explicit time-validity flags and retained last error | Preserve output-on-error, passive health and configuration uncertainty. MDIO whole-transaction/frame attribution must be specified independently |
| `TMP1x2/include/TMP1x2/TMP1x2.h`: `HealthSnapshot`, `DriverState` | UNINIT/READY/DEGRADED/OFFLINE; state, initialized, consecutiveFailures, totalFailures, totalSuccess, lastOkMs, lastErrorMs, time-validity flags, lastError | Use meaningful exact field names; READY never means link/IP readiness. Avoid parallel state()/driverState() aliases |
| `TMP1x2/src/TMP1x2Operations.cpp`: `admit`, `poll`, `finishJob`, `cancel`, `takeResult` | One retained token/result, bounded poll work, callback allowance, no-I/O cancellation; late completion cannot publish success | Adapt to PHY resets/diagnostics with finite deferred cleanup. Select pollOperation plus takeResult/cancel deliberately, rather than shipping all peer vocabularies |
| `TMP1x2/src/TMP1x2Configuration.cpp`: `verify`, `beginApply`, `stepApply`, `readSnapshot` | Owned expected state and partial mismatch persist; CONFIG bookends do not establish atomic sampling | Adapt desired/observed/verified profile and bounded multi-register acquisition; reject sensor-format timing as PHY authority |
| `PCA9555/src/PCA9555.cpp`: `_mapTransportResult`, `_i2cWriteReadRaw`, `_i2cWriteTracked` | Completion counts and write effects are normalized conservatively; short successful completion becomes error | Adapt effect uncertainty and contract-violation handling to frame/transaction semantics; do not copy byte-count or ACK assumptions |
| `PCA9555/src/PCA9555.cpp`: `_i2cWriteCleanupTracked`, `_updateHealth` | Cleanup failures affect health; successful pointer-park cleanup does not clear a preceding failure. Its DriverState has three states | Preserve primary-error visibility and successful-cleanup neutrality for managed recovery. All-source MDIO diagnostics still count actual cleanup frames. Do not call the three-state peer a four-state precedent |
| `PCA9555/src/PCA9555.cpp`: `_invalidateShadowPair`, `_establishShadowPair` | Shadow validity and uncertainty are explicit, separately from observed values | Adapt selective loss of owned configuration knowledge; reject expander pair grouping or safe-direction sequences as PHY behavior |
| `INA228/include/INA228/INA228.h`, `src/INA228.cpp`: `SettingsSnapshot`, `DiagAlert`, `_readDiagAlertTracked`, `_captureDiagAlert` | Cached settings differ from live destructive acquisition; event evidence is preserved in snapshots | Adapt single collection and cached consumer views with PHY-specific latch/counter semantics |
| `INA228/src/INA228.cpp`: `_updateHealth`, `_ensureNormalI2cAllowed` | LATCH_OFFLINE policy can gate ordinary access, and initialization affects accounting | Reject automatic import of offline admission policy. DP8382x OFFLINE is passive; specify its initialization accounting rather than pretending peers agree |
| `MB85RC/src/MB85RC.cpp`: `_selectVariant`, `_validateActiveDeviceId`, `_mapTransportResult` | Exact manufacturer/product selection, variant-dependent state invalidation, terminal counts/effects | Adapt exact supported selection and honest uncertain writes; no memory geometry or arbitrary family mask acceptance |
| `MB85RC/src/MB85RC.cpp`: `enterSleep`, `wake`, `_ensureAwakeForI2c` | Capability/state checks and UNKNOWN effects are distinct; wake with no time hook delegates recovery wait to caller | Adapt explicit transition uncertainty; do not inherit no-clock wake semantics without satisfying the PHY bounded-wait contract |
| `24Cxx/src/EEPROM24Cxx.cpp`: `cancel`, `takeResult`, `poll`; public `Types.h` | Terminal result is retained; known EEPROM busy polling is distinguished from transport fault | Adapt result/effect ownership; reject EEPROM ACK-poll timing, memory buffers and write-cycle barriers as MDIO protocol rules |

## Tests, examples and tooling inspected

`PCA9555/test/test_basic.cpp` was inspected at the actual assertions for:

- short successful transport preserving sentinel read outputs;
- transport timeout versus owner deadline, with zero transfer at expiry;
- callback timeout sums bounded by remaining operation budget;
- failed input read preserving primary/cleanup error details;
- successful cleanup preserving DEGRADED and failure count.

Its registered test names also identify useful future live review cases for
ambiguous writes, mismatch reconciliation, cleanup on cancellation and pending
result protection. They are not a substitute for new independent TI register
oracles. Merely finding a test name was not recorded as executing that test.

`TMP1x2/examples/common/Tmp1x2CliDiagnostics.cpp::observedStatus`,
`startOperation`, `pollOperation`, `finishOperation` show explicit configuration
uncertainty, owner admission and token-checked terminal consumption. These
examples support stage-specific result documentation; they are not core CLI
dependencies. `PCA9555/examples/common/HealthDiag.h` was inspected for field/
error presentation, not adopted as portable code: it uses Serial and treats
zero timestamp as a special display case that DP8382x validity flags avoid.

`24Cxx/tools/test_hil_identity.py` checks observed firmware fields, unknown
identity, duplicates/malformed records and explicit mismatch. TMP1x2 runner and
parser source distinguish bounded host CLI simulation from hardware and retain
restoration/terminal evidence. PCA9555 parser tests explicitly distinguish
address response from identity and fail closed on wrong expected rejection.
These are future runner acceptance references, not proof of an implemented
DP8382x runner or physical fixture.

`TMP1x2/CMakeLists.txt` has explicit source lists, tests OFF by default, real
add_test registration and timeouts. Its `.github/workflows/ci.yml` supplies local
native/Doxygen commands, pinned actions/dependencies and separate SDK targets.
Only source was inspected. The S2/S3 sensor build matrix is not a PHY/MAC support
claim and its remote CI cadence does not override DP8382x's first-set exception.
`TMP1x2/tools/check_doxygen.py` checks generated HTML links, XML public API
visibility and intended documentation inputs; plain comment grep is weaker.
Page existence alone still does not prove useful declaration comments. Follow
CONVENTIONS' additional warning and missing-documentation negative-fixture
requirements instead of assuming the peer checker proves complete Doxygen.

## Prompt changes and limits

Prompts 06-14 now name exact source functions/types to reopen, fitted planned
PHY spellings, real file responsibilities, ordered design decisions, independent
failure oracles and per-stage Doxygen/guides/README/CHANGELOG obligations. They
retain full documented feature coverage and explicitly distinguish missing
source evidence from absent silicon and unavailable hardware. Stage 14 includes
concrete interrupt, power, EEE/LPI, WoL, mux/clock and remaining-feature work
packages; it is not narrowed to the first board's connected pins.

Names follow [CONVENTIONS](../prompts/CONVENTIONS.md); stage 04 owns signatures.
The source excerpts above informed conventions, not copied implementation.
No new silicon facts were inferred from sibling behavior. No driver, SDK adapter,
firmware, package or hardware support was implemented by this maintenance.
Validation at this handoff: `python tools/verify_references.py` PASS (27 vendor
artifacts, 25 text extracts, 315 local links in 69 Markdown files);
`git diff --check` PASS. These are planning checks, not host behavior/SDK/physical
tests. The lead repeats affected checks after integration and records the final
authorized commit/sync evidence; this subagent performed no Git mutations.
