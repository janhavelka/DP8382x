# Prompt 19 sibling and integration review

Read-only source audit on 2026-11-10, using the existing agent model for the
bounded convention and ownership review. Only this report is owned by the
reviewer; the lead owns public declarations, core integration, shared contracts,
verification and Git. No sibling files or fixtures were changed or operated.

Execution branch: `promptset/dp8382x-implementation-20261028`.
Entry checkpoint: `8ee2efd77c27eb03dce069e1a0c98bd7fcb1440c`.
The recorded execution base remains `main` at
`5e148fffde074b85fadc15d8ff66fe2c77111036`.

## Fresh source identities

Paths in this report are relative to `C:/Users/Honza/Documents/Projects/`.
Read-only `git rev-parse HEAD` and `git status --short` established:

| Repository | HEAD | Working tree |
| --- | --- | --- |
| TMP1x2 | `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | Clean |
| INA228 | `73f954838d3fac8cce747d54d0ad738940d4c625` | Clean |
| FieldCore-node | `5ad9292dc8481cf08be9a61f662495a6474d1da0` | Clean |

The reviewer reopened root and parent AGENTS, COMMON, prompt 19, CONVENTIONS,
README, architecture, variants, reference index, API contracts and current
core sources. The sibling AGENTS files require application-owned serialization,
bounded callbacks, explicit configuration uncertainty and separate hardware
evidence. These match the DP8382x ownership boundary; their platform policies
do not turn this read-only comparison into sibling implementation or CI work.

## Declarations, behavior and actual callers

| Inspected paths | Observed behavior | Adopt or reject for link observation |
| --- | --- | --- |
| `TMP1x2/include/TMP1x2/TMP1x2.h`; `TMP1x2/src/TMP1x2Configuration.cpp`, `readSnapshot`; `TMP1x2/src/TMP1x2.cpp`, `getSettingsSnapshot` | A local RegisterSnapshot is assigned only after five successful reads and CONFIG comparison. Earlier format/configuration uncertainty is retained immediately. SettingsSnapshot merely copies cached state. The header explicitly says bookends do not detect unseen changes. | Adopt staged ordinary outputs and immediate independent evidence retention. Keep live acquisition and passive getters separate. Reject sensor-format or CONFIG agreement as proof of simultaneous PHY sampling. |
| `TMP1x2/test/helpers/test_helpers.cpp`, `registerSnapshots`, `snapshotFailureEvidence` | Verifies five reads, untouched sentinels at every failed read, retained uncertainty before later failure, and decoded-but-untrusted values. Register snapshot does not replace the separately acquired temperature sample. | Adopt failure at every phase and invalidity independent from numeric decode. Test link-current evidence separately from historical low events. |
| `TMP1x2/examples/common/Tmp1x2CliDiagnostics.cpp`, snapshot diagnostic | Prints raw words and explicit configuration/threshold/temperature trust. Acquisition happens in an explicit diagnostic phase. | Adopt validity-aware output. A cached-status example must print an already copied snapshot and perform no acquisition. |
| `INA228/include/INA228/INA228.h`, SettingsSnapshot, DiagAlert and readDiagAlert; `INA228/src/INA228.cpp`, getSettings, readDiagAlert, _readDiagAlertTracked, _captureDiagAlert | Settings reads are passive. readDiagAlert is an actual destructive register read and captures evidence before parsing ordinary output. The collector retains sticky event bits plus latest state. | Adopt a single capture path and distinct current versus consumed event evidence. Do not equate live readDiagAlert with a cached getter. |
| `INA228/test/test_basic.cpp`, destructive diagnostic tests around lines 2035-2088; `INA228/examples/esp_idf/basic/main/main.cpp`, printDiag | Tests demonstrate flags clearing in hardware while cached evidence persists; repeated raw reads differ. CLI explicitly identifies destructive acquisition. | Adopt destructive semantics in tests and explicit live-versus-cached routes. Do not copy INA228 clearing masks into PHY logic. |
| `FieldCore-node/src/devices/i2c/Ina228Module.cpp`, copyStatus; `FieldCore-node/test/native/test_component_ina228/test_ina228_module.cpp`, status freshness tests around lines 435-473 | copyStatus copies the retained aggregate. Repeated copies do not call the clock. refreshStatus is a separate owner action with an explicit supplied time; freshness boundaries are tested. | Adopt passive repeated readers and explicit age validity. Reject the peer's clearing numeric fields at expiry as a requirement: DP8382x retains historical values and old timestamps with explicit stale status. |
| `FieldCore-node/src/cli/components/Ina228Cli.cpp`, printPowerStatus | Receives a snapshot value, renders only fields with valid flags and otherwise prints unavailable. Application clock supplies rendering age. | Adopt unavailable rendering for unknown mode/pause and application-supplied time. Do not put a platform clock in the core getter. |

One source discrepancy is material to reuse: INA228's readDiagAlert Doxygen says
getDiagAlertSnapshot preserves the exact raw value, but `_captureDiagAlert`
ORs prior event bits into its raw snapshot. Its `diagnosticEvents.latestRaw`
retains the actual latest word. DP8382x must document raw samples and accumulated
ones/zeros as different evidence, rather than repeating that stronger wording.

## Python runners and workflow boundaries

Freshly inspected `TMP1x2/tools/hil_tmp1x2_runner.py` classifies incomplete
responses as INCONCLUSIVE, checks operation admission/result identity, requires
the expected diagnostic count and verified baseline restoration, and rejects
untrusted snapshot output. `TMP1x2/tools/test_hil_tmp1x2_runner.py` supplies
explicit firmware/library identity, bus and console records and snapshot trust
fixtures. These support finite correlated sessions, not PHY qualification.

`INA228/tools/run_i2c_hil.py` uses finite monotonic receive deadlines and
token/sequence-specific HIL frames. `INA228/tools/test_run_i2c_hil_parser.py`
rejects the wrong token, wrong sequence, truncated frame and an end marker before
its matching begin. Reuse the correlation principle in the eventual PHY runner;
do not infer passing link evidence merely from a prompt or a generic OK line.

`FieldCore-node/scripts/i2c_status_diagnostics.py` parses explicit status fields;
`FieldCore-node/scripts/hil_run_verdict.py` distinguishes observed failure,
blocked, missing scope and completed lifecycle. The actual
`FieldCore-node/test/python/test_hil_run_verdict.py` verifies failure outranks
blocked and missing scope cannot silently become PASS. Adopt that evidence
discipline while retaining this repository's own artifact contract.

Reopened workflow entry points were `TMP1x2/.github/workflows/ci.yml`,
`INA228/.github/workflows/ci.yml` and
`FieldCore-node/.github/workflows/verify.yml`: they select real native/target
checks and different repository-specific trigger policies. No workflow was
changed or dispatched. DP8382x's first-execution prompt-71 exception governs
this execution. Sibling policies do not override it.

## Integration recommendations at entry

These findings describe the entry source and the recommendations sent to the
lead; final implementation and acceptance are recorded by the stage report.

1. Extend `_invalidateEvidence()` with cached-link invalidation. Also handle
   `_invalidateResetEvidence(false)`: AN/circuit restart invalidates link even
   when persistent settings and identity remain intact. `_retainEvidence()`
   has special circuitry-restart and owned-profile-write branches that bypass
   `_invalidateEvidence()`; both need the same link staleness action. A semantic
   attempted or uncertain write must not leave an older snapshot apparently
   current while the operation continues.
2. Preserve OP-14 direct-call admission. Reads can run between operation polls
   and with a result pending. Reset WAIT, reentrancy and the hard-reset silence
   floor prohibit them. `_execute`'s existing `operation` argument bypasses some
   direct-call guards and enables reset admission; it must not be reused merely
   to label a link read as managed traffic. The lead retains normal direct-call
   execution and leaves managed-health implementation to its assigned stage.
3. When a RUNNING operation has attempted a semantic write, or temporary state
   may remain, publish only the current link indication and raw evidence. Mode
   and pause remain invalid. Do not use mutation completion as the only gate;
   OP-14 requires attempted-write suppression.
4. Preserve cached history on ordinary precondition/admission rejection where
   no refresh was submitted. A clock-contract failure invalidates freshness
   even before MDIO because age cannot be trusted. Once an acquisition callback
   was submitted, aggregate failure marks the old snapshot stale, preserves its
   old values/time and records the failure. Ordinary caller output stays intact.
5. The ten-read plan needs whole-query admission for both callback bounds,
   rather than ten times a frame bound or just admission of the first batch.
   The second callback keeps the original deadline. All destructive reads go
   through the existing collector, including partial prefixes on failure.
6. Prompt 19 specifies first-valid, changed, repeated, stale and regained-validity
   evidence but the entry API has no frozen change-counter shape. A minimal
   implementation can retain publication and semantic-change sequences plus
   an explicit latest outcome. Compare the validity-qualified semantic tuple,
   excluding timestamps and consumed raw event bits. Repeated identical fresh
   reads need no new semantic event; first stale and regained-validity edges do.
   Repeated failures while already stale must not repeatedly manufacture a loss
   transition. Saturation and multiple readers need explicit tests. Getters must
   never acknowledge evidence or call a clock.

The test acceptance focus is untouched ordinary output for each failed frame,
partial collector retention, no repeated-reader consumption, reset/configuration
invalidation, coherence failure, clock zero/regression/overflow, and OP-14 calls
before and after semantic writes. A down cable with clean callback receipts
remains successful transport. Raw forced PHYSTS bits cannot replace qualified
BMCR-based mode resolution.

## Evidence status

Final read-only implementation review checked `src/DP8382xLink.cpp`, shared
invalidation/collector paths, public API, `test/link.cpp`, the portable example
and CTest registration. No further blocking defect was found after integration.
One initial recommendation to preserve freshness on local budget rejection was
superseded by the frozen failed-refresh contract: frame/deadline/bound denial
marks prior history stale even without MDIO. This is documented and tested.

Source review: PASS for the paths and behavior identified above. Commands used
were read-only source searches/reads, Git identity/status inspection and SHA-256
file hashing. No sibling test, SDK build or hardware session was executed.
Host runtime, portable/package consumer, SDK link, remote CI and physical
qualification: NOT RUN by this reviewer; the lead owns stage verification and
the final synchronized checkpoint. This report supplies no new silicon facts
and does not claim independent datasheet or physical qualification.

## File identities

The following SHA-256 values identify the freshly inspected core peer paths;
repository commits above identify their remaining supporting files.

| Path | SHA-256 |
| --- | --- |
| `TMP1x2/AGENTS.md` | `a8ba69e5ee8b516b1b66555138a811e2cab1eecd1dbaa5d2c7d4c0fdbaa174a3` |
| `TMP1x2/include/TMP1x2/TMP1x2.h` | `66760185303f0814742d5b585fd98b76301ac0289879e14348ef7e733d9bef54` |
| `TMP1x2/src/TMP1x2Configuration.cpp` | `f69705b50cd29d280a10860bfe32d28ba5bbff7f17a258ab150f18b2c25dc635` |
| `TMP1x2/src/TMP1x2.cpp` | `9ded5af2f5dbcf5389917ef87ce5a67660c06cac8974daf9af7b85915f40c452` |
| `TMP1x2/test/helpers/test_helpers.cpp` | `89eb0e2023905176808dde3183d8acd539d9040874364659ba2a9b2dbb60fc90` |
| `TMP1x2/examples/common/Tmp1x2CliDiagnostics.cpp` | `9de1ea0ac501d15c5196049f04b8906df9ddc6b73fc32ec80c2312dbf725d71d` |
| `TMP1x2/tools/hil_tmp1x2_runner.py` | `09960aa61e5e5c709307913033c1ebf123548926b02a4f9f36f533e725bc5de7` |
| `TMP1x2/tools/test_hil_tmp1x2_runner.py` | `5ce59dad6476642bab3a4a49f8977c30b7949c33f0ff86fd2e1873fb58967c93` |
| `INA228/AGENTS.md` | `eca61d7b301ca1b5691c5b86db11bece0f4ab0249946d624417b159466297c43` |
| `INA228/include/INA228/INA228.h` | `cee47d13d9b0a32c06e9dcdebc0ff4becc26ccedc805bead9448fd26d52fd6a5` |
| `INA228/src/INA228.cpp` | `8f29c2721637da5690f82835594457248208d720d51d4339c427073b8f66c91d` |
| `INA228/test/test_basic.cpp` | `5446a8e8ce32aade3a0c85d60e05c0e05ea38f2e051e420a8b1b049daaf4b2af` |
| `INA228/examples/esp_idf/basic/main/main.cpp` | `fb771712a185270bef423b10c9d2de10bb906ae91619dde704d9a87e1f8bbf8f` |
| `INA228/tools/run_i2c_hil.py` | `860e3649886fb894ecebb91c49d223621d0e90fb15015e573443c5e4d5c7151f` |
| `INA228/tools/test_run_i2c_hil_parser.py` | `3a66356873cfb356cec068ae3610007f3a9c12cb1e8018276dce23343ddb8ddb` |
| `FieldCore-node/AGENTS.md` | `5c5936b19d911c8422fd07b0b856c85e0575551be431d7dc71dd82395cce40fe` |
| `FieldCore-node/src/devices/i2c/Ina228Module.cpp` | `b143d5cdd32b6435833ef1de5d308150def08a1eeab63030f45126a425e6ba24` |
| `FieldCore-node/test/native/test_component_ina228/test_ina228_module.cpp` | `b4f88ff341de2baf24a24d353ddc1a4d2e5cff6eac9c57d160fbf3d369cea69f` |
| `FieldCore-node/src/cli/components/Ina228Cli.cpp` | `28c8eb765751f692bea7ee9a8d0a8fcb2d2e6cf14be8d03406f48aafddfeccaa` |
| `FieldCore-node/scripts/i2c_status_diagnostics.py` | `6bf0c88b3a52660d7a400626708c4d80a195ee04bd82a3824f0f15eaed7b0d7c` |
| `FieldCore-node/scripts/hil_run_verdict.py` | `4677589c1eb826add505f8116befe3cc32bae30bdedc09405f9ecb2a16a5b8e3` |
| `FieldCore-node/test/python/test_hil_run_verdict.py` | `2c59cc7e90417c91433161e20ea5d36df264400f324526cffd22023337d192f2` |
