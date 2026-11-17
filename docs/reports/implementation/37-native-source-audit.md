# Prompt 37: native source and peer audit

This is the source inventory and design disposition for prompt 37. It does not
claim completed integration tests, a linked firmware image or physical results.
The final implementation report owns commands/results and the commit/sync
handoff. Work began on `main` at
`cb5ffe5b5d21e700c3e9fbef434e192a50b1aeff`; Git integration belongs to the lead.

## Scope, delegation and preserved inputs

Read repository/parent AGENTS, README, architecture, variants, reference index,
prompt 37, COMMON, CONVENTIONS, ENGINEERING, VERIFICATION, EVIDENCE, source map,
design sections 2-7, and OP-1 through OP-17. Prompt 36 supplies the implemented
core procedures, retaining its explicit source-gated gaps. Native integration
does not close those silicon gates or physical qualification.

The lead owns public/shared contracts, core/native integration, architecture,
progress, final report, verification integration and Git. Disjoint agents own
the hash-checked real-driver host harness, peer inventory/link fixture and
independent SDK ownership review. `p37_host_tests` and `p37_peer_fixture` use
gpt-6.1-sol/high for bounded harness/fixture implementation and peer inventory.
`p37_sdk_review` uses gpt-6-astra/high for stronger independent ownership/deadline
reasoning; it also implemented the narrowly assigned shared restart procedure
and its tests before returning to read-only review. These are actual selected
models, not inherited labels from earlier reports. Review work is read-only;
fixture and harness edits have separate assigned paths and build directories.

Sibling repositories, SDK checkout, FieldCore-node and vendor payloads were not
modified. No remote CI dispatch/status/waiting, flashing, fixture operation or
publication belongs to this stage. Current automatic workflow definitions do
not yet exist in DP8382x; prompt 71 owns their first creation/execution.

## Fresh peer inventory

All paths in this section are relative to `Projects/`. `git rev-parse HEAD` and
`git status --porcelain` were rerun while preparing this report; all four peers
were clean at these exact revisions:

| Repository | Revision | Relevant native path |
| --- | --- | --- |
| INA228 | `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/Ina228IdfI2cTransport.cpp` |
| PCA9555 | `0aec2212894910e0d872c3e511fbcd740837c5f7` | `examples/espidf_basic/main/main.cpp` |
| TCA9548A | `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/espidf_basic/main/main.cpp` |
| TMP1x2 | `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/esp_idf/basic/main/main.cpp` |

For each peer, reopened `AGENTS.md`, root `CMakeLists.txt`, `idf_component.yml`,
`.github/workflows/ci.yml`, public `include/<name>/Status.h`, `Config.h`, facade
and relevant production source. Names and terminal Status shape are precedents;
their different operation vocabularies, health admission and I2C framing are
not interchangeable PHY contracts.

- INA228: transport context validation, `mapEspTransferErr`, separate probe
  mapping, bounded integer timeout conversion, `ina228IdfInitI2c`,
  `ina228IdfDeinitI2c`, and temporary-handle cleanup retention. Related inputs:
  `src/INA228.cpp`, `test/test_basic.cpp`, `tools/check_owner_contract.py`,
  `tools/check_idf_example_contract.py`, `tools/run_i2c_hil.py`,
  `tools/test_run_i2c_hil_parser.py` and shared CLI sources.
- PCA9555: `mapI2c`, `ensureDevice`, `probeTargetAddress`, `i2cWrite` and
  `i2cWriteRead`, including NOT_ATTEMPTED versus MAY_HAVE_COMMITTED. Related
  inputs: `src/PCA9555.cpp`, `test/test_basic.cpp`,
  `test/test_public_contracts.cpp`, `tools/check_idf_example_contract.py`,
  `tools/run_i2c_hil.py`, `tools/test_run_i2c_hil_parser.py` and shared CLI.
- TCA9548A: `mapI2c`, `controllerReady`, `prepareAddress`, receive-fault retirement
  and retained replacement-device ownership. Related inputs:
  `src/TCA9548A.cpp`, `test/test_driver/test_driver.cpp`,
  `examples/common/Tca9548aCli.cpp`, `tools/check_idf_example_contract.py`,
  `tools/tca9548a_hil.py` and `tools/test_tca9548a_hil.py`.
- TMP1x2: the specifically requested `mapError`, `finish`, `initializeConsole`,
  precreated native device handles and startup unwind. Related inputs:
  `src/TMP1x2.cpp`, `src/TMP1x2Operations.cpp`,
  `src/TMP1x2Configuration.cpp`, `test/test_core.cpp`,
  `tools/hil_tmp1x2_runner.py`, `tools/test_hil_tmp1x2_runner.py`,
  `tools/test_idf_component_contract.py` and shared CLI. TMP1x2 has no
  `tools/check_idf_example_contract.py`; its actual component-contract and
  general contract checks provide that boundary evidence.

Adopted: precise backend detail, argument validation before transport, accounting
only actual attempted transfers, explicit primary/cleanup evidence, retained
ownership when deletion fails, native framework separation, real linked-image
evidence, firmware identity, finite runner sessions and preserved cleanup verdicts.

Adapted: TCA9548A's source-driven retired-controller model to the Ethernet RX
lifetime gate, and retained handles to the complete driver/glue/netif/MAC/PHY
resource set. TMP1x2 console installation/error handling is useful for a later
native CLI; this small link fixture needs no custom console driver.

Rejected: PCA9555 `ensureDevice` ignoring removal failure and retargeting within
transfer callbacks; INA228 startup paths allowing a cleanup error to replace
the original creation failure; I2C NACK/probe categories as Ethernet transport
facts; sensor timing and bus framing; implicit recovery; peer S2/S3 matrices,
pins and IDF 5.3/5.4-to-5.x support ranges as DP8382x support claims.

## Actual SDK and tools

The side-by-side checkout is `C:\esp\v6.1\esp-idf`, clean at
`fff9895c82d744c7237be8847347bdd1b07c6643`. The repository
`scripts\idf.cmd --version` completed successfully and reported ESP-IDF v6.1;
activation reported Python **3.12.10**. No user/machine environment setting was
changed. The actual P4 compiler executable is
`C:\esp\v6.1\tools\tools\riscv32-esp-elf\esp-15.2.0_20251204\riscv32-esp-elf\bin\riscv32-esp-elf-gcc.exe`;
`--version` reports **15.2.0**, crosstool-NG **esp-15.2.0_20251204**.
These read-only identity checks are PASS. They do not replace the final image
build or report tool versions that were not observed.

The actual SDK files reopened for lifecycle/control/readiness include:

| SDK-relative source | Pinned Git blob / relevant scope |
| --- | --- |
| `components/esp_eth/src/esp_eth.c` | `76dab829d2099a574a07aeceeaa82491950fe07c`; installation, start/stop, uninstall, raw ioctl and notification dispatch |
| `components/esp_eth/src/mac/esp_eth_mac_esp.c` | `6448fcd6de4669ddf1e836c992ca8a1accc9dd96`; mediator storage, MDIO waits, MAC controls, RX handoff and deletion |
| `components/esp_eth/src/phy/esp_eth_phy_802_3.c` | `7d5d407d029c63494dffaaa3d82f8701b6b73790`; comparison only, not a production dependency |
| `components/esp_eth/src/esp_eth_netif_glue.c` | `9dec04bc104dde182906bc0626a452fee761f592`; retained driver/netif references and event handlers |
| `components/esp_timer/src/esp_timer.c` | `cf8d8abd83c1a260c39e571c5fc4ff4dabd57854`; TASK callback ordering and active/executing state |
| `components/esp_eth/include/esp_eth_driver.h` | `405138e4e63dd93a21896bb970b8ad19d68127f4`; public installer/ioctl/lifecycle contract |
| `components/esp_eth/include/esp_eth_phy.h` | `a5a0d14e85dca35be5cb629054db9e4167d43c27`; complete PHY vtable |

Also inspected `esp_eth_mac.h`, `esp_eth_mac_esp.h`, `esp_eth_com.h`,
`components/esp_eth/CMakeLists.txt`, `Kconfig`,
`components/esp_hal_emac/esp32p4/emac_periph.c` and relevant public dependencies.
The P4 GPIO table admits REF_CLK44, TX_EN49, TXD0=41, TXD1=42, CRS_DV51,
RXD0=52 and RXD1=53; SMI GPIO40/39 remains explicit application wiring.
The [maintained SDK catalogue](../../reference/library-sdk.md) owns the broader
source/timing inventory. No SDK file was copied into MIT production source.

## Source-derived decisions and limitations

Installer lines 200-271 ignore mediator/reset_hw results. Those adapter entries
must therefore be local/infallible for admitted install; board reset failure is
handled before install, when no PHY frames have run. SDK defaults address -1,
GPIO 5, reset timeout 100 ms, AN timeout 4000 ms, assertion 100 us and no post-reset wait
are SDK policy, not selected board wiring or exact-chip readiness. SNLS647I
sections 7.6 and 8.3.11 require the clock-qualified reset/POR path, 2 ms reset-release
SMI silence and 50 ms POR readiness. The [initialization guide](../../initialization.md)
retains the exact distinction between the delayed-clock alternatives.

Driver start lines 300-321 changes FSM before AN, START, initial link and timer
arming. Stop lines 323-344 changes FSM before timer stop; unarmed stop returns
INVALID_STATE and omits DOWN/STOP. The owner must preserve those partial effects,
fence once, repair missing events and observe actual MAC stop. A resumed fence
does not prove an SDK STOP event happened. MAC LINK lines 401-419 ignores actual
start/stop errors and returns success for legal values; accepted LINK/cache
publication therefore survives a later event-post error on this exact MAC.
Event retry must never replay UP and reset DMA descriptors.

MDIO methods use 100 us steps and a nominal 1000 us polling allowance without a bus
mutex. The separate MAC mutex can wait 100 ms. MAC stop has nominal 2500 us polling.
These are source-loop contributions; scheduler-inclusive bounds need a qualified
owner declaration. `notificationUs` is a positive per-mediator-call declaration,
and all calls must fit the aggregate callback allowance. The fixture uses
unmeasured conditional values only behind its disabled execution guard.

The native_rx_quiescence gate remains open. RX can capture mediator at the MAC
handoff before the application callback, while uninstall frees driver storage
before MAC deletion terminates RX. STOP, timer fence, callback count zero and
unsynchronized mediator swap do not close this race. No safe reusable public-API
quiescence route was derived. Retain installed driver/glue/netif/MAC/PHY and
contexts with terminal admission closure until system restart; hardware Q13
cannot authorize a source-known dangling reference. HAL FIFO-flush error loss
remains an independent SDK observation limit.

The opt-in host harness compiles actual driver/glue sources and native adapter,
with small platform fakes and instrumented MACs. Its compiler-derived SDK file
closure is recorded and Git-blob checked by
[verify_sdk.py](../../../test/esp_idf/host/verify_sdk.py) against
[sdk-files.json](../../../test/esp_idf/host/sdk-files.json); no fixed guessed
header count is claimed. Raw and netif RX captures must be tested separately.
This proves the modeled source race and owner decisions, not real RX task timing.

Independent review raised STOP-resume event evidence, lost-event replacement,
quarantine receipt classification, DOWN phase admission, aggregate notification
timing and raw-uninstall partial effects. The lead owns corrections and the
executing regression evidence; this source inventory does not mark those tests
PASS. The public [native guide](../../esp-idf.md) describes the intended corrected
contract and separates unavailable later-stage routes.

## Evidence handoff

Peer/SDK revisions and tool identity: **PASS**, read-only checks above. Application
Git identity generator: **PASS** for actual dirty-tree commit, unchanged-file
timestamp preservation on repetition, and archive `unknown` fallback.
Portable host/consumer, real pinned-driver host and actual SDK image-link results
must be recorded in the final implementation report after integration. Remote CI:
**NOT RUN**, first-execution prompt 71 policy. Physical fixture/Q13: **NOT RUN**.
Reusable installed teardown: **BLOCKED**, native_rx_quiescence source gate.
No Git synchronization or completion status is claimed by this delegated audit.
