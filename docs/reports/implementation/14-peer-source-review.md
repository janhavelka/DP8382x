# Prompt 14: fresh sibling identity review

Reviewed 2026-11-07 for exact-address identity/capability implementation on the
recorded DP8382x execution branch. The lead owns implementation, shared headers,
integration and Git. This disjoint review used the current capable model for
actual sibling source/test inspection; stronger exact-silicon review is separate.
All three sibling worktrees were clean at inspection and remained read-only.

| Repository | Revision | Actual reviewed paths and anchors |
| --- | --- | --- |
| MB85RC | `1a45e882bb3b1d29f5a18538a7658b0ed0f8a424` | `include/MB85RC/CommandTable.h` VariantInfo/KNOWN_VARIANTS/findVariantByProductId (92–149); `include/MB85RC/MB85RC.h` DeviceId and read/decode declarations; `src/MB85RC.cpp` raw Device ID staging (1896–1950), `_selectVariant` (2017), `_validateActiveDeviceId` (2047); `test/test_basic.cpp` unknown/no-ID variants (1152–1179), reidentification (1182–1241), read/decode/error tests (3088 onward), registration (6373–6377, 6440–6443); `examples/01_basic_bringup_cli/main.cpp` explicit binding/identity check (1904), live read (1948), probe versus health (1112–1144). |
| TMP1x2 | `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | `include/TMP1x2/Config.h` model membership, exact address ranges and ALERT capability (22–38); `include/TMP1x2/TMP1x2.h` probe/config declarations; `src/TMP1x2.cpp` bind validation (31), probe (178), invalidation (187); `test/variants/test_variants.cpp` exhaustive model/address and capability negatives (45–105), `test/test_core.cpp` probe health neutrality (140–145), `test/jobs/test_jobs.cpp` busy probe (144), CMake variant registration (39–41); `examples/common/Tmp1x2Cli.cpp` cached display, selected-model meaning and live probe (286–397). |
| PCA9555 | `414e0f66b13082e974ec27a4a6e86985a65d83fd` | `test/test_basic.cpp::test_por_default_check_is_explicit_and_non_identity` (532), registered at 2682; `tools/test_run_i2c_hil_parser.py::test_address_scan_is_not_identity_proof` (79), registered at 827; `examples/01_basic_bringup_cli/main.cpp` probe health neutrality (1292–1305), explicit scan (2254) and live probe (2583). |

The scan test named by prompt 14 is in the Python runner tests, not the C++ test
file. TMP1x2's current tests are split into `test/variants`, `test/jobs` and other
directories; there is no `test/test_basic.cpp`. MB85RC's actual runner is
`tools/hil_runner.py`; the similarly named PCA runner must not be assumed there.

Read each repository's AGENTS.md and `.github/workflows/ci.yml`, plus the actual
Python session runners: MB85RC `tools/hil_runner.py`, TMP1x2
`tools/hil_tmp1x2_runner.py`, PCA9555 `tools/run_i2c_hil.py`. They retain finite
command/session deadlines and distinguish cached output from live probes. Their
remote-CI policy does not override DP8382x's prompt-71 exception. No sibling test,
remote CI, SDK build or physical session was run by this review.

## Adopted, adapted and rejected principles

- Adopt exact declared-model membership and local validation before I/O. TMP1x2
  exhausts model-specific address ranges rather than admitting their union; the
  PHY instead validates the five-bit protocol address separately from its straps.
- Adopt MB85RC's manufacturer/product checks and source-backed private variant
  metadata. Unknown identity is not a broad family match. Its `supportedByDriver`
  and per-feature attributes distinguish source capability from working software.
- Adapt MB85RC's reidentification invalidation: mode/revision/part rejection or
  changed identity must not preserve old permissions. Repeated accepted identity
  is not configuration verification and must not manufacture fresh link evidence.
- Adapt staged decoding: MB85RC decodes its raw buffer only after successful
  completion. The PHY's two-word terminal batch additionally retains independent
  valid prefix words when the second read fails; ordinary output remains unchanged.
- Reject MB85RC AUTO selection and I2C reserved-address Device ID byte/ACK rules.
  DP83826 uses exact configured PHY address and C22 ID registers; IDs do not prove
  the I/E ordering suffix or temperature grade.
- Reject TMP1x2 fixed-bit plausibility and PCA ACK/POR defaults as PHY identity
  oracles. PCA's tests explicitly state those limits. No scan is requested here.
- Adopt passive cached queries and explicit live-read commands. Probe success is
  separate from health, settings trust, initialization and hardware qualification.

## Contract findings for the lead

The frozen DP8382x API specifies one two-frame acquisition, per-word validity,
decoded validity, acceptance and observed time. Zero in either required word
precedes the all-one check and returns ZERO_ID; otherwise all-one in either word
returns ONES_ID. Other complete candidates reject in OUI, model, revision and
required-mode order. Callback failures retain their transport status/detail.
Partial identity remains UNASSESSED and cannot grant capabilities. Reset waits
and hard-reset floors belong to the shared OP-14 admission path when operation
machinery exists; this stage must not invent a second reset state engine.

Capabilities are the frozen foundational silicon facts for the accepted exact
declared part, not flags claiming future typed procedures are implemented. Board
wiring and fixture availability must not erase source-backed capabilities. Every
invalidation must make permissions unusable while preserving historical bytes.
