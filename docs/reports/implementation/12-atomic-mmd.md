# Prompt 12: atomic MMD transactions

Executed 2026-11-01 on `promptset/dp8382x-implementation-20261028`, unchanged
from the recorded execution. Clean synchronized entry after fetch:
`c678f59b3e74bac359acee1d247b43a1b84c2dea`. Original base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`. Final commit and verified upstream
equality are recorded in the handoff, avoiding a self-referencing commit.

## Rules, prerequisites and delivered behavior

Followed COMMON and the shared conventions, evidence and full-coverage contracts:
namespace/class `DP8382x`, PascalCase types, camelCase methods/fields, borrowed
terminal callbacks, one application instance owner and controller-wide batch
exclusion. Root/parent guidance remains applicable; siblings/vendor payloads are
read-only and the lead alone integrates and performs Git mutations. No root
`.github` workflow exists or was changed; remote CI remains prompt 71 only.

Concrete inputs were prompt 11's usable bounded direct driver, tests and consumer;
prompt 3's exact REGCR/ADDAR/alias/access ledger; and prompt 6's selected one-batch
controller ownership. Prompts 7–8 supply signed Status, frame-prefix, release,
clock/budget and lifetime contracts. Unrelated silicon/SDK gates remain open.

Implemented `readMmd`, `writeMmd` and stable `modifyMmd` in the existing facade
and source. One `_execute` boundary now validates and accounts for direct and
multi-frame results; no second transaction API, native Clause 45 claim, SDK
bitfields, selector cache, per-PHY lock registry or retained caller buffer.
Four-frame read/write and five-frame RMW are each one indivisible callback.
Full admission includes every frame plus acquisition/release/overhead and clock
lag. Fewer frames or insufficient aggregate time reject before the callback;
actual in-flight failure terminates/releases rather than suspending a sequence.

One canonical target policy serves direct registers and vendor aliases. Thirty-
eight original-table nonconsuming extended words join the existing direct read
subset. All-reserved banks, destructive RXFS, holes, indirect selectors and
procedure-only offsets remain denied. DEVAD3/7 and opaque EEE mappings are
explicit SOURCE_GATE failures. Raw TDR/MSE/straps do not establish freshness,
calibration, SNR, interpreted mode or successful resampling.

Mutation remains the already accepted LEDCR maintenance subset at vendor alias
31:0018, with the same exact-part/address/electrical owner prerequisites. RMW
requires a nonzero legal stable mask and a replacement inside it. It preserves
only other documented stable bits and forces reserved bits zero. No additional
pre-initialization feature exception or arbitrary raw-write bypass was added.
Mixed command/status and destructive composition remain task 13's typed policy.

Each next explicit call fully reselects, regardless of the previous outcome.
Snapshot slots distinguish selector/address frames from target read/write and
the dependent RMW write. Primary/release errors and possible effects survive;
a later read cannot retrospectively make an uncertain write successful.
Malformed prefix continuation, timestamps, statuses or extra slots reject output
and stop the local instance under the existing shared-quarantine contract.

## Independent source and live sibling evidence

`mmd_source_review` used available `gpt-6-astra` with high reasoning for original
PDF protocol, access, ownership and final code review. It was read-only.
`mmd_peer_tests` used the current capable coding model for fresh sibling review
and disjoint `test/mmd.cpp` implementation, deriving literal expectations from
the specification/contracts rather than production constants. Neither edited
siblings or Git. The lead reviewed actual code/tests/diffs and owns shared guides.

Original archived SNLS647I pages 35–36, 63 and 70 establish the protocol, reserved
control bits and LEDCR composition. Pages 74–93 establish the extended read
allowlist from detailed field tables. Independent derivation from
8.3.11.1/.4/.5: in function01 the selected address remains unchanged through a
read followed by a write. Therefore a complete stable RMW takes **five** frames,
not the early project's eight-frame reselection policy. Updated API/design
contracts and timing probes together. Other research procedure counts remain
conservative finite upper bounds, not permission to split ownership.

The [transport guide](../../direct-access.md#complete-indirect-transactions)
contains complete independent read/write/RMW traces. Control words are full
literal 001F/401F, with reserved bits zero; no poisoned SDK storage can enter
production construction. Independent examples include prior FFFF, mask0012,
value0002 yielding0682, and prior0492/mask0600/value0200 yielding0292. The
[vector guide](../../register-vectors.md#atomic-stable-mmd-modification) records
their source and limitations. Initialization-owned VOD writes remain a separate
procedure; the raw API cannot bypass that ownership.

Current manufacturer checks:

- [TI's current DP83826I datasheet](https://www.ti.com/lit/gpn/DP83826I) still
  resolves to SNLS647I. Original archived bytes/hashes were preserved.
- GitHub connector main for `TexasInstruments/ethernet-rtos-drivers` remains
  `94bf4343004732983b0b8e1eec123d88ff70453b`. Reopened
  [`rtos_drivers/src/dp83826.c`](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c)
  (16,010 bytes, Git blob `e0b64710350d764fe16ffd089fbdc5246e600039`) and
  [`generic_phy.c`](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c)
  (33,761 bytes, blob `39414bd3b1004cdf6075bf0f4be30180babdc84d`). Their per-file
  BSD-3-Clause notices apply. Vendor read/write selects four frames; the six-frame
  RMW's unchecked write results and absent complete owner are rejected, as the
  [catalogue](../../reference/library-ti-core.md#transport-and-mmd-procedure-audit)
  records. No implementation was copied.
- GitHub connector ESP-IDF v6.1 remains pinned at
  `fff9895c82d744c7237be8847347bdd1b07c6643`. Reopened
  [`esp_eth_phy_802_3.c`](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/phy/esp_eth_phy_802_3.c)
  (28,023 bytes, Git blob `7d5d407d029c63494dffaaa3d82f8701b6b73790`, Apache-2.0).
  Uninitialized control declarations at 609/628/648/669 and separate mediator
  frames remain the [pinned comparison](../../reference/library-sdk.md#indirect-management-and-validation-defects).
  A backend must execute inside its already acquired scope, not recursively lock
  through those helpers. No SDK code or dependency enters this portable target.

Fresh clean sibling revisions and paths:

| Reference | Revision | Reopened implementation/tests |
| --- | --- | --- |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | `src/PCA9555.cpp` mapping/raw/tracked/cleanup wrappers at149–268; `include/PCA9555/Config.h`; `test/test_basic.cpp` pointer cleanup/primary retention at2044/2120/2194, short read2461, aggregate grants2495 and RUN_TEST registration. |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | `src/MB85RC.cpp::_mapTransportResult` at1449; `test/test_basic.cpp` completion/ambiguous failure cases5426/5841 and registrations; `examples/common/I2cTransport.h` scoped timeout/readiness/reset safety. |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `src/TMP1x2.cpp` read/write/track; `src/TMP1x2Operations.cpp` timing allowances; `test/jobs/test_jobs.cpp`228 expiry/callback duration; `test/test_core.cpp`, `CMakeLists.txt` active test registration. |

Also reopened each AGENTS/workflow and relevant example adapters/runners
(`tools/run_i2c_hil.py`, `tools/hil_runner.py`, `tools/hil_tmp1x2_runner.py`).
Adopted terminal accounting, staged outputs, retained uncertainty and separate
cleanup evidence. Rejected I2C byte/ACK completion proofs, sensor wrap-ms timing,
per-frame lock scope and pointer-cleanup writes after an MMD failure. Here the
next explicit operation reselects; cleanup does not mutate a potentially stale
target. Sibling tests were NOT RUN and no sibling was changed.

## Verification and maintainability

Environment: Windows PowerShell, GNU C++15.1.0 WinLibs, CMake/CTest4.0.1,
Python3.12.10 and installed Doxygen. Commands:

```sh
cmake -S . -B build/mmd-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/mmd-release
ctest --test-dir build/mmd-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/mmd-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/mmd-consumer
ctest --test-dir build/mmd-consumer --output-on-failure --no-tests=error
python tools/check_doxygen.py
python scripts/generate_version.py check
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_references.py
git diff --check
```

Host PASS: 14 registered Release CTest suites and five standalone public header
compilations with strict warnings. Five new MMD suites execute 10,776 checks,
including every selector/address/data error, accepted writes before failure,
deadlines between frames, complete next-call reselection, original allowance
forwarding, minimum full budget, stable masks, all canonical aliases, extended
read policy, gates, malformed prefixes/timestamps, release/quarantine and
competing accesses after every frame across two PHY addresses. The explicit
failure probe exits1 with NDEBUG, proving checks remain active.

Portable build-tree consumer PASS: tests OFF, real library linked, direct/MMD
read/write/RMW examples execute using an explicitly synthetic controller. No
Python/Git/Doxygen/SDK is needed by that consumer. SDK link, installed/exported
package qualification, physical bus timing and hardware qualification are NOT RUN.
Remote CI is NOT RUN by the first-execution prompt-71 policy. Host simulated
success cannot establish controller timing, electrical traces or real PHY behavior.

Doxygen PASS: 53 generated pages, member/link coverage and both rejected negative
fixtures. All 98 Python tests PASS. Version generation check, ledger validation
(96 registers, 52 procedures, 35 features), references (27 vendor artifacts,
25 extracts, 1,359 local links in 158 Markdown files) and diff whitespace checks
PASS. Vendor archive inventory and numeric silicon facts are preserved;
access evidence remains PARTIAL because task13 and EEE gates still apply.

Final maintainability review keeps one explicit access policy, one batch builder
and one common transport normalization path. No new generic framework or source
split is justified. Reviewed/updated README, CHANGELOG, public Doxygen, API/design,
architecture, transport/example, test, vector, variant, coverage and progress
guides. Version tooling behavior and SDK/tool commands require no change; no
nonexistent native/CLI/hardware operation is advertised.

The independent final acceptance review found no blocking defect or scope gap.
Its minor transport-guide report-link finding was corrected. Callback Doxygen
also states stop-on-first-failure, successful-read dependency and whole-batch
admission explicitly; Doxygen checks passed after that clarification.
