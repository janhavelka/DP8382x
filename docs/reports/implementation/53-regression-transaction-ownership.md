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

## Phase (b) implementation

The wrapper verified the assessment commit
`9f7829b65c54cffba6c5e715b49e0733fad03fb4` against `origin/main`.
The index and worktree were clean on entry. Both implementation agents used the
session's high-reasoning model because their tasks depend on the ordering of
deadline samples and SDK callbacks. One owned the portable implementation and
its four test files. The other owned the installed SDK/CLI tests in `cli44.cpp`.
The lead owned the private declarations, MAC fault injection, native timeout
suite, registration, integration, documentation and checked commit request.
Both agents finished. The live SDK and three sibling checkouts matched the
phase (a) revisions. The source decisions and test map above still apply.

| Regression | What the implementation executes |
| --- | --- |
| Reset and EEE admission | `_admitResetStep` handles init/replay, loopback restart and the calibration clear/HARD pair. It runs at the ordinary poll and fresh callback-entry samples. `_admitEeeStep` checks the 75/71 frames and 21/20 callbacks at callback entry. Both retain the original widened arithmetic, comparison order and error details. The public interface is unchanged. |
| Strict timing and indivisible batches | `reset_timing` tests HARD/RESTART at 2,028/1,028 µs in its fixture. `loopback_timing` tests 38 µs with the previous analog selection already applied. `electrical_calibration` tests 2,032 µs and separates a one-frame aggregate failure, a short-slice yield and a missing completion-read reserve. `eee_ownership_timing` tests initial enable and owned replay at 2,472/2,448 µs. At equality no callback starts. One more microsecond admits the expected literal batch. |
| Cross-entry ownership | `native_cli_cross_entry_boundaries` injects both task identities at each of 4 read, 4 write and 5 masked-update positions. SDK raw ioctls and mediators on both installed endpoints exercise a safe word, both indirect ports and a destructive counter. PHY speed, duplex, AN, restart, link and pause callbacks, a real CLI command and an application read run inside the holder's callback. The peer reaches its own terminal callback twice and each one-tick take fails. The holder records one take/give and exactly 4/4/5 physical-model frames. Both counter values survive. A peer read succeeds after release. |
| SDK effects independent of the bus lock | An identical PHY address succeeds, a different one is rejected, and neither issues a frame. The flow test exercises the typed MAC and PHY halves individually to avoid the pinned raw flow ioctl's incompatible pointer types. A rejected pause callback retains earlier accepted halves and records possible MAC change. Rejected restart and link callbacks retain the SDK stop obligation. |
| Uncertain writes | `native_raw_accepted_write_timeouts` applies the write before reporting `ESP_ERR_TIMEOUT` at each of the 11 write positions across the three transactions. Each fresh installed fixture compares the literal prefix, modeled selector, mode and target, attempted/completed counts, primary detail, release status and actual lock events. Address-only effects preserve verified settings. A possible final target write retains the desired profile and marks observed settings stale and dirty. Owner and peer are excluded during quarantine. After simulated external idle confirmation, one successful read reports the value while SDK agreement remains withdrawn. |
| Start after invalidation | `native_cli_restart_after_raw_observation` crosses AN/forced mode with an actual raw BMCR change or an observed PHYRCR HARD bit through the DEVAD 31 alias. A later successful read leaves trust dirty. The public SDK getter retains the old AN policy. A direct SDK start can post START in forced mode before failing. Owner stop accounts for that prefix. Explicit initialization replay and reconciliation restore supported start. |

The new cases passed with the existing native behavior and the extracted
portable admission checks.
The existing MMD failure, partial-notification and directional-pause suites
remain in the run. API, SDK, CLI, testing and feature coverage guides describe
the added evidence. The root README and CHANGELOG already describe these APIs
and user-visible behavior. The physical qualification and installed teardown
gates remain open.

### Local reproduction

Reuse the phase (a) Release/Ninja configure command above. For worktree and
firmware identity checks, include both the SDK and the actual repository in
the process-local Git safe directories. Add the exact SDK OpenThread submodule
path for its configure-time revision query. Run the same host filter with the
pinned public driver suites included:

```powershell
$env:GIT_CONFIG_COUNT = '3'
$env:GIT_CONFIG_KEY_0 = 'safe.directory'
$env:GIT_CONFIG_VALUE_0 = 'C:/esp/v6.1/esp-idf'
$env:GIT_CONFIG_KEY_1 = 'safe.directory'
$env:GIT_CONFIG_VALUE_1 = (Resolve-Path .).Path.Replace('\','/')
$env:GIT_CONFIG_KEY_2 = 'safe.directory'
$env:GIT_CONFIG_VALUE_2 = 'C:/esp/v6.1/esp-idf/components/openthread/openthread'
cmake --build build/p53-baseline --parallel 5
ctest --test-dir build/p53-baseline -R "($suites)|^pinned_driver_" --output-on-failure --no-tests=error -j 3
cmake -S test/consumer -B build/p53-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/p53-consumer --parallel 3
ctest --test-dir build/p53-consumer --output-on-failure --no-tests=error
```

The sandbox could read the pinned clangd archive but could not access its
installed executable. The SDK exporter checks this tool during activation.
The local build used that exact cached archive from `C:/esp/v6.1/tools/dist`,
SHA-256 `1a91c04bb570e0740eede529b20c98c2dd0be6750eb7d91a7226e19f3d88d490`.
Extract its `esp-clangd/bin` directory, including the DLLs, into
`build/p53-baseline/sdk-tools` and prepend the resulting bin path to this
process's PATH. The first Ninja build stopped progressing in ccache. A new
cache in the owned SDK build directory allowed compilation to proceed.

```powershell
$env:CCACHE_DIR = 'C:/esp/build/dp8382x-loop/53cache'
$projects = [ordered]@{l='test/esp_idf/adapter_link'; b='examples/esp_idf/basic'; o='examples/esp_idf/owner'}
foreach ($key in $projects.Keys) {
  $dir = "C:/esp/build/dp8382x-loop/53$key"
  scripts/idf.cmd -C $projects[$key] -B $dir -D IDF_TARGET=esp32p4 -D "SDKCONFIG=$dir/sdkconfig" build
  if ($LASTEXITCODE) { throw "SDK build failed for $key" }
}
```

### Phase (b) receipts

| Evidence | Result |
| --- | --- |
| Windows x86-64 host, GNU 15.1.0, CMake 4.0.1, Ninja, Release | **PASS**. The build verified 45 SDK inputs at `fff9895c82d744c7237be8847347bdd1b07c6643`. The final core, CLI and pinned-driver selection passed 167/167 in 0.94 s. Direct execution of the two new native CLI cases passed 2,606 and 84 checks, respectively. The portable agent also passed all 36 EEE, reset, operation, electrical and loopback registrations in an independent Release build. |
| Plain tests-OFF consumer | **PASS**, 2/2 in `build/p53-consumer`. Exported package consumer **NOT RUN**. |
| Python and source checks | **PASS**. Python 3.12.10 discovery ran 238 tests with 49 compiled-fixture skips. Doxygen checked 293 pages and two negative fixtures. Reference verification covered 29 payloads, 27 extracts and 2,234 local links. Ledger verification covered 97 registers, 52 procedures and 35 features. Version and whitespace checks passed. |
| ESP32-P4 SDK links | **PASS**, adapter link fixture, basic application and owner application. Each ELF contains `Phy::execute`, `_admitResetStep` and `_admitEeeStep`. Each firmware identity header records the phase (a) SHA and dirty worktree. Early builds with only the SDK safe directory produced unknown firmware identity. Relinking with the process-local repository entry above corrected all three. |
| Remote CI and physical evidence | Both **NOT RUN**. Prompt 71 owns first-execution CI. The host model ran offline and the three firmware images were linked only. |

| Image in `C:/esp/build/dp8382x-loop` | Bytes | SHA-256 |
| --- | ---: | --- |
| `53l/dp8382x_adapter_link.bin` | 387040 | `0779b3f699ab4845fee60fcb415a9264ee0be4b5b62bfccddd0a83b23883e8ed` |
| `53b/dp8382x_p4_ethernet.bin` | 682400 | `ade2f2e495900d604b4db91ce4f39f92ccc6c7deb58bc865c40837b1733dc8c7` |
| `53o/dp8382x_p4_owner.bin` | 560832 | `eb28551a608e566c80edcfb37257345de0a6586c7518b605720d9e97baa218b1` |

Phase (c) independently reviews the committed diff against the source vectors,
especially the two admission samples, partial effects and SDK cache behavior.
The wrapper owns the implementation commit and remote equality check.

## Phase (c) independent review and fix

The wrapper synchronized the 18-file implementation at
`61ef93f70906679be25135f4175fc73d9c496085`. Review started on clean
`main` at that SHA, zero ahead and behind `origin/main`. Two read-only
reviewers used the session's high-reasoning model in parallel. One traced
portable timing and effects through the diff from `9f7829b`. The other traced
installed callbacks and receipts through the pinned SDK. The lead reproduced
the SDK finding, fixed the native adapter and updated its test and contracts.
The SDK reviewer then rechecked the fix and the order of the deferred receipts.

The portable extraction preserves both clock samples, the ordinary batch
check before a short-slice yield, strict reset/calibration and EEE reserves,
and effects from earlier writes. The reviewer independently ran five Release
suites, totaling 37,407 checks. The SDK reviewer checked both task identities
at the 13 indirect frame boundaries, the 11 accepted-write timeout cuts,
the SDK AN cache, typed flow-control halves and separate release receipts.
The SDK and three clean sibling checkouts match the source decisions above.
The lead rechecked the sibling registrations, fault/output tests and CLI
boundaries, SNLS647I section 8.3.11.1 and the pinned SDK's start, stop, raw
ioctl, 802.3 and P4 controller paths.

### Unreserved callbacks during SDK start

In `components/esp_eth/src/esp_eth.c`, `esp_eth_start` changes the SDK FSM,
optionally calls RESTART, posts START, calls `get_link`, then arms the timer.
The adapter previously checked the STARTING phase and used task identity to
select a gate wait. With the gate free, a competing task could repeat the
three-frame BMCR restart or perform the initial observation. The starting
task could also repeat a completed callback. With the gate occupied by the
genuine restart, a rejected link probe could incorrectly record a START post.
The reviewer reproduced a duplicate RESTART returning `ESP_OK` on the
unmodified pinned host image. The new CTest also failed against the old
adapter on its first competing RESTART during START delivery.

`Phy::start` now reserves the sequence selected by the reconciled AN cache.
The starting task consumes each expected callback before waiting for the gate.
A successful restart enables the initial link observation. A contending timer
can observe RUNNING after the start outcome is adopted. A genuine initial
link call that exhausts its gate attempts retains the START post for cleanup.
The SDK vtable identifies the task and callback kind only through the adapter's
task/sequence checks. Application calls use the owner API.

`native_start_callback_ownership` probes the installed callbacks during the
BMCR batch, START delivery and timer arming. It checks both cached modes, a
later timer observation, unchanged output on rejected restart, frame counts
and two independent event traces. A failed START post leaves an empty trace.
A genuine START followed by failed initial link admission produces exactly
one START and one STOP. The test runs at all three registered tick rates.

### Phase (c) verification

Windows x86-64, GNU 15.1.0, Ninja, CMake 4.0.1 and Python 3.12.10.
Use the phase (b) process-local Git, clangd and ccache settings above.

```powershell
cmake --build build/p53-baseline --parallel 5
$review = '^(native_|pinned_driver_|example_|owner_|transport_|mmd_|access_|identity_|operation_|reset_|electrical_|loopback_|eee_|negotiation_|verify_|link_|health_|cli_processor_)'
ctest --test-dir build/p53-baseline -R $review --output-on-failure --no-tests=error -j 4
$san = '-fsanitize=undefined -fsanitize-undefined-trap-on-error'
cmake -S test/esp_idf -B build/p53-review-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf "-DCMAKE_C_FLAGS=$san" "-DCMAKE_CXX_FLAGS=$san"
cmake --build build/p53-review-ubsan --target dp8382x_native_host_tests dp8382x_native_host_100hz_tests dp8382x_native_host_333hz_tests dp8382x_native_cli_tests --parallel 5
$nativeReview = '^native_(start_|raw_|direct_start_|forced_lifecycle|foreign_flow_effect|flow_control|notification_partial_receipts|cli_)'
ctest --test-dir build/p53-review-ubsan -R $nativeReview --output-on-failure --no-tests=error -j 4
```

| Evidence | Result |
| --- | --- |
| Release host and pinned public driver | **PASS**, 499/499 in 2.75 s. The build verified 45 real SDK sources/headers at `fff9895c82d744c7237be8847347bdd1b07c6643`. |
| Pinned host UBSan, C and C++ trap mode | **PASS**, 79/79 in 0.60 s, including the new regression at 1,000, 100 and 333 Hz. |
| Plain tests-OFF consumer | **PASS**, 2/2 using the phase (b) consumer commands. Exported package consumer **NOT RUN**. |
| Python and source checks | **PASS**. Python discovery ran 238 with 49 compiled-fixture skips. Doxygen checked 293 pages and two negative fixtures. Reference, ledger, version and whitespace checks passed. |
| ESP32-P4 SDK links | **PASS**. The phase (b) `scripts/idf.cmd` loop recompiled `Phy.cpp` and linked each ELF. All three firmware identities record `61ef93f` and a dirty worktree. Image hashes below. |
| Remote CI and physical measurements | Both **NOT RUN**. First-execution CI belongs to prompt 71. Reusable installed teardown and board qualification retain the gates in the native guide. |

| Binary | Bytes | SHA-256 |
| --- | ---: | --- |
| `53l/dp8382x_adapter_link.bin` | 387200 | `8a81ddb1f4caa3d89f7db69d35eb4f2ade5c8fd09a20ad50c340d28e4221d856` |
| `53b/dp8382x_p4_ethernet.bin` | 682544 | `fe020a0083204f0ac7b390f2d255b363bef44720325b6d1d0ea6c69c9d113ef3` |
| `53o/dp8382x_p4_owner.bin` | 560992 | `39c6dad9b2c24da567f0e51ee9d75c366efdb8e162f7613f594ebc27c29c376e` |

The public Doxygen, native guide, API contract, coverage matrix, test guide
and CHANGELOG describe the corrected ownership boundary. The README's owner
entry point still applies. Phase (d) owns final verification and completion
after the wrapper confirms the review commit and remote equality.

## Phase (d) final verification

Prompt 53 is complete for the portable core, CLI and pinned ESP-IDF adapter.
On 2026-11-22, verification ran on clean `main` at
`65ec3096028a77c3b364cdc45e55380549f6460d`. The wrapper had verified
equality with `origin/main` at that SHA. The review commit contained all 11
requested files, and the entry comparison was zero ahead and behind.
The three sibling checkouts matched the clean revisions in the source table.
Their fault, output, CLI and registration conventions still apply.

The lead ran the full Release suite, the portable admission suites under UBSan
and the source checks. Two agents used the session model to verify independent
builds in parallel. One linked the P4 images and checked their embedded
identity. The other verified the native UBSan tests and rebuilt the standalone
consumer. Both finished before completion.

| Evidence layer | Result |
| --- | --- |
| Host Release | **PASS**, 647/647 CTests in 179.91 s on Windows x86-64 with GNU 15.1.0, Ninja 1.12.1 and CMake 4.0.1. The build verified 45 real SDK inputs at `fff9895c82d744c7237be8847347bdd1b07c6643`. All 52 baseline test names and the four added regression cases ran. |
| Transaction and callback coverage | **PASS**. The installed CLI reentry case passed 2,606 checks across 13 frame boundaries, and restart after raw observation passed 84 checks. The 11 accepted-write timeouts and start callback ownership ran at 1,000, 100 and 333 Hz. |
| Windows UBSan trap | **PASS**, 36/36 portable reset, loopback, calibration, operation and EEE tests in 0.58 s, and 79/79 pinned native tests in 0.43 s. |
| Plain build-tree consumer | **PASS**, 2/2 after a clean Release rebuild with library tests OFF. |
| Python and source checks | **PASS**. Python 3.12.10 discovery ran 238 tests, with 189 passes and 49 compiled-fixture skips. CTest ran the compiled fixture. Doxygen checked 293 pages and rejected two negative fixtures. The reference check verified 29 payloads, 27 extracts and 2,234 links. The ledger verified 97 registers, 52 procedures and 35 features. Version and whitespace checks passed. |
| ESP32-P4 SDK links | **PASS**, 3/3 using ESP-IDF v6.1 at the pin above. Each ELF and application image contained the full review SHA and clean worktree identity. |
| Exported package consumer | **NOT RUN**. Clean export and relocation checks belong to prompt 70. |
| Remote CI | **NOT RUN**. Prompt 71 owns the first-execution CI checkpoint. |
| Physical | **NOT RUN**. Board power, wiring and timing qualification require an identified fixture and authorization. Reusable installed teardown remains **BLOCKED** on SDK RX quiescence. |

| P4 image under `C:/esp/build/dp8382x-loop` | Bytes | SHA-256 |
| --- | ---: | --- |
| `53l/dp8382x_adapter_link.bin` | 387200 | `80a740753098d8abde7840e9fce4409eaf203d967da144ee075de19e0ac7ff83` |
| `53b/dp8382x_p4_ethernet.bin` | 682544 | `dec7068e559b676dd0e1f972667d2b23e3b6fce096231f8866c021730c06bea0` |
| `53o/dp8382x_p4_owner.bin` | 560992 | `3773d2833080ca747464789f0495595f74e06080e0f938c0202d8e5d8474b838` |

To reproduce the host result, use the phase (a) SDK environment and Release
configure command, then run the commands below. For native UBSan, use the
phase (c) configuration, targets and filter. For the consumer, use the phase
(b) commands with `--clean-first` on the build. The P4 links used the phase
(b) environment and three-project `scripts/idf.cmd` loop.

```powershell
cmake --build build/p53-baseline --parallel 5
ctest --test-dir build/p53-baseline --output-on-failure --no-tests=error -j 1
cmake -S . -B build/p53-final-core-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON '-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error'
cmake --build build/p53-final-core-ubsan --target dp8382x_reset_tests dp8382x_loopback_tests dp8382x_electrical_tests dp8382x_operation_tests dp8382x_eee_tests --parallel 4
ctest --test-dir build/p53-final-core-ubsan -R '^(reset_|loopback_|electrical_|operation_|eee_)' --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/check_doxygen.py
python tools/verify_ledger.py
python tools/verify_references.py
python scripts/generate_version.py check
git diff --check 7c5f614..65ec309
git diff --check
git diff --cached --check
```

The implementation and regression tests are in `61ef93f` and `65ec309`.
The completion request covers this report and the progress row. The wrapper
records the completion commit and verifies its upstream SHA.
