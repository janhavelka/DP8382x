# Prompt 53 · Transaction ownership across entry points

## Phase (a) assessment

Planning started on clean `main` at `7c5f61495cbb6b85ed63b3853eb49efe3ee0d1fa`.
The wrapper verified the prompt 52 commit against `origin/main` and the entry
comparison was 0 ahead, 0 behind. The concrete prerequisites are the portable
transaction executor and cooperative operations, the installed owner and pinned
SDK harness from [prompt 38](38-esp-idf-state-arbitration.md), and the shared
processor and native application routes from [prompt 47](47-cli-native-integration.md).
The ledger assigns zero new procedures to prompt 53.

The lead traced both admission sites, checked the test registrations and ran
the baseline. Two read-only agents used the session's high-reasoning model to
inspect the independent deadline/effect and SDK ownership questions in parallel.
The [feature matrix](../../feature-coverage.md) remains the coverage record.
The following test map scopes its existing access, reset, EEE, calibration,
negotiation, link and collector rows for this regression task.

| Vector or failure boundary | Existing executing CTests | Phase (b) implication |
| --- | --- | --- |
| DEVAD 31 read/write/RMW. Registers 0x0D, 0x0E, 0x0D, 0x0E, then a dependent 0x0E write for RMW. Selector words 0x001F and 0x401F have bits 13:5 clear. | `mmd_traces`, `mmd_admission`, `mmd_failures`, `mmd_reports`, `mmd_ownership` | Keep the independent literals in `test/mmd.cpp`. The existing 4 + 4 + 5 positions exercise both possible write outcomes and 13 further late-completion cuts. They check one callback, attempted/completed counts, original detail, read sentinel, release and full readdress on the next explicit call. |
| Ordinary output and consumed evidence on failure. One logical transaction can launch several MDIO frames. | `identity_scan`, `identity_failures`, `link_failures`, `access_failures`, `health_accounting`, `native_discovery`, `native_discovery_accounting`, `native_cli_discovery`, `native_collector_partial_evidence` | Preserve ordinary caller outputs. Inspect the separate retained prefix, stale snapshot and primary/release status. The native CLI discovery case counts two callbacks, four attempted frames and three completed frames. |
| Complete batch ownership, zero or short frame allowance, failed take, release and quarantine. | `transport_ownership`, `operation_admission`, `mmd_admission`, `native_raw_lock_failures`, `native_raw_boundary_failures`, `native_p4_busy_before_command`, `native_cli_raw_access` | Compare controller take/give events, completed traces and logical operation counts. Preserve these cases for rejection before launch and a possible effect after launch. |
| Callback reentry via the SDK, application, CLI and another PHY. | `native_raw_read_arbitration`, `native_raw_mutation_arbitration`, `native_rejected_raw_reads`, `native_cli_raw_access`, `native_cli_shared_silence`, `cli_processor_routes`, `owner_cli_integration` | The read test probes the public raw ioctl at register 2 on four boundaries. The mutation test covers application and peer transport entry on four and five boundaries. The two-installed-owner fixture currently exercises reset silence. Combine the missing **routes at the boundaries** using the existing fixtures. |
| Partial configuration, raw BMCR, invalidation and restart, SDK AN agreement after forced stop/start. | `negotiation_failures`, `verify_interrupted_replay`, `verify_raw_reset_observation`, `native_uncertain_basic_control`, `native_raw_state_reconciliation`, `native_direct_start_invalidation`, `native_forced_lifecycle`, `native_cli_reconciliation` | Inspect the retained desired profile, observed trust, public `ETH_CMD_G_AUTONEGO`, forced-mode admission and prior SDK effects. Retest restart from actual raw/reset paths wherever coverage currently uses explicit invalidation. |
| Portable RX/TX pause, SDK Boolean, partial MAC/PHY updates and notification failures. | `link_pause`, `native_pause_projection`, `native_forced_partner_pause`, `native_notification_partial_receipts`, `native_flow_control`, `native_foreign_flow_effect` | Reuse the 16 local/partner combinations and all four notification positions. Preserve accepted speed/duplex/pause prefixes and the separate flow-control receipt when exercising a rejected callback. |
| Reset, loopback restart, calibration clear/HARD pair and the EEE capability/command floor. | `operation_timing`, `reset_traces`, `reset_floor`, `reset_timing`, `reset_failures`, `loopback_timing`, `electrical_calibration`, `electrical_timing`, `eee_recipe`, `eee_failures`, `eee_replay`, `eee_ownership_timing` | The PCS case changes time between the two admission samples. Extend that boundary coverage to the other procedure kinds. Keep all current regression assertions. |

## Source decisions

The [original DP83826I/E datasheet](../../reference/source/TI-DP83826I-E-SNLS647I.pdf),
SNLS647I section 8.3.11.1 and tables 8-31/32, supplies the MMD vectors above.
Tables 8-19 and 8-47/48 distinguish BMCR (0x00, PCS command 0x8000), CDCR
(0x1E, calibration bit 14) and PHYRCR (0x1F, HARD 0x8000 or restart 0x4000).
In TI RTOS commit `94bf4343004732983b0b8e1eec123d88ff70453b`,
`rtos_drivers/src/generic_phy.c` checks the four indirect stages.
`rtos_drivers/src/dp83826.c` ignores write errors in its extended RMW and
passes the reset mask as the register argument on completion. DP8382x stops at
the first failing frame and polls the literal register address. The pinned
files and table comparison are in the [core catalogue](../../reference/library-ti-core.md).

The clean ESP-IDF v6.1 checkout was `fff9895c82d744c7237be8847347bdd1b07c6643`.
In `components/esp_eth/src/esp_eth.c`, public raw ioctls obtain the installed
PHY address and invoke the mediator. The driver owns its AN cache. Its flow
ioctl reads one argument as both Boolean and 32-bit ability. The supported
owner calls the MAC and PHY with separately typed arguments.
`src/phy/esp_eth_phy_802_3.c` under the same component issues separate MMD
frames and leaves reserved bits of its local control union uninitialized.
`src/mac/esp_eth_mac_esp.c` distinguishes a busy controller before launch
from a timeout after issuing a frame. The production adapter installs two
unconditional raw-hook rejections and runs approved transactions under the
shared controller lock. [SDK evidence](../../reference/library-sdk.md)

The three read-only sibling checkouts were clean. Their `AGENTS.md` and
`.github/workflows/ci.yml` were inspected as source. TMP1x2 and 24Cxx
register host sources and CTests explicitly and build the SDK application
separately. DP8382x currently has no tracked workflow.

| Checkout | Concrete precedent and decision |
| --- | --- |
| TMP1x2 `c91bac63313ea725ce174ab98abc50705c365639` | `include/TMP1x2/BusOperations.h`, `src/BusOperations.cpp` and `test/bus/test_bus.cpp::transportFailureAndOutput` preserve output and backend detail and count calls. Use this pattern with MDIO frame receipts. Root CMake and `examples/esp_idf/basic/main/CMakeLists.txt` select production sources. `examples/common/Tmp1x2Cli.cpp`, `test/cli/test_cli.cpp` and `tools/hil_tmp1x2_runner.py` distinguish parser rejection from an owner result. |
| 24Cxx `0bb27f46beb4fbe907beb11d9134de4e1797347d` | `include/EEPROM24Cxx/EEPROM24Cxx.h`, `src/EEPROM24Cxx.cpp` and `test/test_transport/test_transport.cpp` retain indeterminate write commitment and check physical counts and lock release. Use separate effect and release receipts. Root and native main CMake select the shared `examples/common/Eeprom24CxxCli.cpp`. `test/test_cli/test_cli.cpp` checks rejected commands against transfer counts and `tools/test_cli_transcript.py` drives the compiled producer. EEPROM timing and I2C framing do not apply to MMD. |
| PCA9555 `b9297a45a6f18fa8251e95857aedea0588a32e27` | `include/PCA9555/PCA9555.h`, `src/PCA9555.cpp` and `test/test_basic.cpp::test_read_observed_state_failure_returns_only_current_partial_evidence` distinguish partial acquisition from ambiguous writes. `examples/espidf_basic/main/main.cpp` and `tools/test_run_i2c_hil_parser.py` keep that distinction at the CLI. Preserve earlier effects through a later successful diagnostic read. |

## Phase (b) brief

1. Extract the reset and EEE look-aheads from `DP8382x::_execute` into private
   procedure-specific admission functions. Trace `pollOperation` and
   `_execute` together in `src/DP8382x.cpp`, including initialization replay,
   loopback restart and the calibration clear/HARD pair. Keep the ordinary
   budget check at each existing sample, the short poll-slice yield, the
   guarded fresh sample immediately before callback admission, and the order of
   reset, calibration and EEE denials. Keep reset look-ahead at both existing
   sites. EEE currently checks the second sample. Preserve widened arithmetic,
   strict inequality, specific error details, accounting and retained effects.
2. Add independent fresh-sample tests in the owning suites. With the
   `test/eee.cpp` transport bounds of 8 µs fixed, 4 µs per frame and 1 µs
   clock lag, and the 2,000 µs HARD wait, the two strict lower bounds are
   **2,472 µs** (75 frames, 21 callbacks) and **2,448 µs** (71 frames, 20
   callbacks after the capability read). Set the first sample early enough to
   admit, then test equality and one extra microsecond at the fresh sample.
   Exercise 74 versus 75 remaining frames at capability admission. Check that
   a denial launches zero additional frames and a short cooperative slice
   cannot split the calibration pair or any MMD transaction.
3. Extend the pinned SDK fixture in `test/esp_idf/host/cli44.cpp`. Use two
   installed endpoints at distinct PHY addresses on one `SharedBus`. At the
   four read, four write and five RMW frame positions, probe the installed PHY
   callbacks and mediator, public read/write register ioctls, production CLI
   owner route, application API and second adapter. Exercise both the current
   owner task without a permit and a competing task. Include raw 0x0D/0x0E,
   a destructive source, same/foreign PHY address and flow control. Expect
   idempotent address setting to succeed without frames. An unadmitted pause
   acknowledgement can record an uncertain earlier MAC effect. Assert each
   route's documented result, sentinel, bounded return and both frame traces.
   Distinguish denied lock takes from the holder's single successful take/give.
4. The instrumented MAC returns a scripted write error before it applies
   a modeled write. Add targeted post-effect timeout injection for the
   uncovered installed-adapter address/data cases. Compare the modeled target
   and exact trace with the owner's uncertainty, original backend detail,
   release result, quarantine and subsequent admission. Use a fresh fixture
   per timeout. Check that explicit successful observation alone cannot restore
   trust. Fix any reproduced defect in its owning module.
5. The lead owns `include/DP8382x/DP8382x.h`, integration, registrations,
   coverage, guides and the commit request. Separate native fixture work from
   portable admission/tests. Maintain the existing one-owner API. Update
   procedure comments, public Doxygen, `test/README.md`, the feature matrix
   and affected API/SDK/CLI guides against actual changes. Review README and
   CHANGELOG alongside them. Phase (c) should compare the resulting diff and
   original sources independently for deadline order and cross-entry effects.

## Baseline receipt

Windows x86-64, GNU 15.1.0, CMake 4.0.1, Ninja, Python 3.12.10, Release.
The SDK verifier checked 45 sources and headers at the pin for the host build.
The first configure and build attempts encountered Git's dubious-ownership
check in the SDK verifier. The following process-local settings allowed its
read-only Git queries in both steps.

```powershell
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'safe.directory'
$env:GIT_CONFIG_VALUE_0 = 'C:/esp/v6.1/esp-idf'
cmake -S . -B build/p53-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p53-baseline --parallel 6
$suites = '^(transport_|mmd_|access_|identity_|operation_|reset_|electrical_(calibration|timing)$|loopback_timing$|eee_|negotiation_failures$|verify_|link_|health_accounting$|native_(cli_|raw_|rejected_raw_reads$|forced_lifecycle$|direct_start_invalidation$|flow_|foreign_flow_effect$|uncertain_basic_control$|pause_projection$|forced_partner_pause$|notification_partial_receipts$|discovery|p4_busy_before_command$|collector_partial_evidence$|hard_reset_silence$)|cli_processor_|owner_cli_integration$|example_cli_network$)'
ctest --test-dir build/p53-baseline -R $suites --output-on-failure --no-tests=error -j 3
```

Host core, CLI and pinned-driver baseline **PASS, 155/155**. These results
precede the planned extraction and additional tests. Portable tests-OFF and
exported package consumer, P4 SDK link, remote CI and physical evidence were
**NOT RUN** in phase (a). For the implementation, rerun the affected suites
and new registrations, rebuild `test/consumer` separately with tests OFF and
run its two CTests. Link the affected P4 fixture and applications using
`scripts/idf.cmd` and separate short directories under
`C:/esp/build/dp8382x-loop`. The physical readiness and reusable native
teardown gates remain as recorded in the [native guide](../../esp-idf.md).
Prompt 71 owns remote CI. Phase (b) is ready.
