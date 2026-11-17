# Prompt 39: native notification and timing source audit

Source audit performed on 2026-11-17, entering clean `main` at
`456b9710095b4e1394cfb8d4acdeb90b722d981f`. The lead owns shared headers,
implementation, conformance tables, architecture, progress, integration and Git.
This delegated unit owns only this report. The current capable coding model is
used for the bounded source inventory and comparisons; independent ownership and
failure review is a separate lead-assigned outcome. No sibling, SDK or vendor
payload was edited. Concurrent prompt-39 implementation/test edits are preserved.

Reopened repository/parent AGENTS, README, architecture, variants, reference
index, COMMON, prompt 39, CONVENTIONS, ENGINEERING, VERIFICATION, SOURCE_MAP,
design decisions sections 2-7, the operation contract, the maintained SDK
catalogue and prompt-37/38 source audits. This is conformance closure of the
existing native path. No new silicon support, physical timing or reusable
installed teardown qualification follows from source inspection.

## Fresh peer inventory and changes since 37

Paths below are relative to `Projects/`. Fresh `git rev-parse HEAD` and
`git status --porcelain` found all four siblings clean at the same revisions
recorded by prompt 37. No relevant peer source or rule change was observed;
prompt 38's three-peer findings remain applicable and TMP1x2 was reopened too.

| Peer | Actual HEAD | Native production seam reopened |
| --- | --- | --- |
| INA228 | `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/Ina228IdfI2cTransport.h`, `.cpp`, `main.cpp` |
| PCA9555 | `0aec2212894910e0d872c3e511fbcd740837c5f7` | `examples/espidf_basic/main/main.cpp` |
| TCA9548A | `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/espidf_basic/main/main.cpp` |
| TMP1x2 | `afd50889312f6e218a6a34eca687e3e8547d8b40` | `examples/esp_idf/basic/main/main.cpp` |

For each peer, reopened `AGENTS.md`, public `include/<peer>/Status.h`,
`Config.h`, facade and relevant production source. Current rules keep core
framework independent, put transport/scheduling/serialization in the application,
retain backend detail and distinguish build evidence from hardware results.
Their `.github/workflows/ci.yml` files were reopened. Those workflow commands,
boards and SDK versions are peer evidence; they do not establish DP8382x support
or override the main-only / first-execution prompt-71 policy. DP8382x had no
workflow definitions at audit entry, as in prompts 37/38.

Concrete test-to-production comparisons:

- INA228 `test/test_basic.cpp`:
  `test_cancel_and_timeout_are_bus_silent_with_precise_effects` executes
  `src/INA228.cpp::_cancelJob`, `cancelJob`, `timeoutJob` and the invalidation
  path. It checks no extra frames, PARTIAL after earlier mutation and retained
  resynchronization requirements. Native `runOwnerJob` in `main.cpp` keeps the
  operation ID before requesting timeout and consumes that exact terminal result.
  `mapEspTransferErr`, `ina228IdfInitI2c`, `ina228IdfDeinitI2c` and native write/read
  callbacks preserve the original SDK detail and failed-removal handles. The
  similarly named `test_example_transport_*` cases execute Arduino `TwoWire`
  transport, not these native I2C callbacks; they are not native SDK error or
  scheduling execution evidence.
- PCA9555 `test/test_basic.cpp`:
  `test_transport_timeout_and_owner_deadline_remain_distinct`,
  `test_operation_callback_timeouts_share_remaining_deadline_budget` and
  `test_near_deadline_callback_count_respects_minimum_timeout` execute
  `src/PCA9555.cpp::pollOperation` with a fake bus. The production method divides
  the allowance among callbacks and retains separate pointer-cleanup evidence.
  It samples the supplied `nowMs` at poll entry; this does not prove an elapsed
  wall-clock bound across arbitrary callback preemption. Native `mapI2c`,
  `probeTargetAddress`, `ensureDevice`, `i2cWrite` and `i2cWriteRead` were reopened
  separately. `test/test_public_contracts.cpp` checks value/ownership contracts.
- TCA9548A `test/test_driver/test_driver.cpp`:
  `test_expired_owner_budget_performs_no_physical_io_or_cached_success`,
  `test_late_write_requires_a_new_cleanup_budget_and_failed_cleanup_stays_unknown`
  and `test_owner_clipping_preserves_specific_backend_errors` execute core
  `_writeControlByte` / `_readControlByte` through `DeadlineOwnerHarness`.
  The harness clips the callback allowance and models time/accepted effects;
  it does not execute native `i2c_master_receive`. Core failure invalidates mask
  observation; native `mapI2c`, `controllerReady`, `prepareAddress` and callbacks
  separately retain failed registration/rollback handles and retire receive-faulted
  controller state. The native fault policy is source based, not a measurement
  supplied by the core deadline tests.
- TMP1x2 `test/test_core.cpp` uses its `Bus` double for clock/protocol guards,
  format and partial-write matrices. `src/TMP1x2Operations.cpp::poll` checks the
  owner deadline again after terminal callbacks and retains transport outcome
  separately. `src/TMP1x2.cpp`, `TMP1x2Configuration.cpp`, native `mapError`,
  `finish`, precreated device handles and `initializeConsole` were reopened.
  These callbacks retain precise SDK detail without inventing a NACK phase;
  the core tests do not execute native console startup or deletion.

Reopened example/CLI and Python boundaries: INA228 native `main.cpp`,
`tools/check_owner_contract.py`, `tools/check_idf_example_contract.py`,
`tools/run_i2c_hil.py`, `tools/test_run_i2c_hil_parser.py`; PCA9555 native
`main.cpp`, `examples/common/CliShell.h`, `tools/check_idf_example_contract.py`,
`tools/run_i2c_hil.py`, `tools/test_run_i2c_hil_parser.py`; TCA9548A
`examples/common/Tca9548aCli.cpp`, `CliArguments.h`, native `main.cpp`,
`tools/check_idf_example_contract.py`, `tools/tca9548a_hil.py`,
`tools/test_tca9548a_hil.py`, `tools/test_hil_artifacts.py`; TMP1x2
`examples/common/Tmp1x2Cli.cpp`, `tools/test_idf_component_contract.py`,
`tools/hil_tmp1x2_runner.py`, `tools/test_hil_tmp1x2_runner.py`.
The INA228/PCA9555/TCA9548A component checkers are static framework/source checks;
TMP1x2's component-contract tests execute CMake registration and identity-refresh
targets with explicit registration substitutes, without an SDK compiler/link.
Runners use finite
monotonic sessions, correlated outcomes, firmware identity and retained cleanup
and artifact failures. No peer test or runner was executed for this audit.

Adopted: validate before mutation, one application owner, borrowed context
lifetimes, independent primary/cleanup evidence, exact backend detail, accepted
effects surviving late return, bounded sessions and honest evidence layers.
Adapted: peer owner-budget admission to complete controller-wide MDIO batches;
sensor dirty-state retention to per-field SDK publication and uncertain effects.
Rejected: I2C framing/NACK categories, sensor health admission, Arduino CLI types,
peer board/SDK ranges and source-text checks as proof of native executing bounds.
PCA9555 `ensureDevice` still ignores old-device removal failure; INA228 startup
can replace its initial creation failure with cleanup error. TMP1x2 startup also
discards some cleanup returns. None is a precedent for erasing primary failure
or releasing a possibly live Ethernet resource.

## Actual immutable SDK source

Checkout `C:\esp\v6.1\esp-idf` is at
`fff9895c82d744c7237be8847347bdd1b07c6643`, with clean tracked state.
Fresh `git hash-object` observations:

| SDK-relative file | Actual blob |
| --- | --- |
| `components/esp_eth/include/esp_eth_phy.h` | `a5a0d14e85dca35be5cb629054db9e4167d43c27` |
| `components/esp_eth/src/phy/esp_eth_phy_802_3.c` | `7d5d407d029c63494dffaaa3d82f8701b6b73790` |
| `components/esp_eth/src/esp_eth.c` | `76dab829d2099a574a07aeceeaa82491950fe07c` |
| `components/esp_eth/src/mac/esp_eth_mac_esp.c` | `6448fcd6de4669ddf1e836c992ca8a1accc9dd96` |
| `components/esp_hal_emac/emac_hal.c` | `e8be3015b1c6a372d1fdfccfd90c05fe23617d0f` |
| `components/esp_timer/src/esp_timer.c` | `cf8d8abd83c1a260c39e571c5fc4ff4dabd57854` |
| `components/esp_timer/src/esp_timer_impl_systimer.c` | `237a435061da654876f6f112dd050ddde1ffa887` |
| `components/esp_hw_support/port/esp32p4/systimer.c` | `fd5580d62b594df171314de2e148bcfad050a8de` |
| `components/freertos/FreeRTOS-Kernel/include/freertos/projdefs.h` | `2ec338490583778cf0ebfdb221481b38191ae25d` |

These bytes match the prior baseline. Original line numbers below refer to that
checkout, not the adapter being changed concurrently. No SDK patch or private
driver-layout access is proposed.

## Inputs for the lead's conformance and timing tables

PHY-H exposes seventeen entries: `set_mediator`, `reset`, `reset_hw`, `init`,
`deinit`, `autonego_ctrl`, `get_link`, `set_link`, `pwrctl`, `set_addr`, `get_addr`,
`advertise_pause_ability`, `loopback`, `set_speed`, `set_duplex`, `custom_ioctl`,
`del`. Include an explicit optional-null `custom_ioctl` row; the wrapper assigns
the other sixteen and inherits no IEEE base method. Source/text inventories
alone do not establish that the final method-by-method tests pass.

| Source | Observed behavior | Required bound/ownership interpretation |
| --- | --- | --- |
| MAC 41, 211-253 | Frame begins only if not busy; polls in 100 us busy-delay steps to nominal 1000 us accumulated delay; no mutex or caller deadline | At most ten delayed polls per attempted frame. Include HAL/instruction/logging/interrupt/preemption costs; 1000 us is not a wall-clock maximum. A whole indirect sequence needs external serialization. |
| MAC 91, 185-193, 379-397 | 100 ms multi-register mutex used by add/remove destination-address filters | Separate non-MDIO API wait. The actual callers are filter operations, not SPEED/DUPLEX/PAUSE/LINK notifications. Do not assign this wait to every notification or mistake it for the management-bus lock. |
| MAC 651-667 | Init resets MAC, then checks completion for `sw_reset_timeout_ms/10` iterations, sleeping `pdMS_TO_TICKS(10)` after unsuccessful checks | Include install-time MAC reset, floor iteration count, tick phase and scheduler latency. Below 10 ms, reset is issued and no completion check runs. This is distinct from PHY reset. |
| BASE 133-212 | PHY reset uses 10 ms sleeps; AN restart uses 100 ms sleeps; AN expiry need not produce timeout | Comparison only: these base procedures do not execute in the selected wrapper. Do not add their loops or false success semantics to our core callback bounds. |
| MAC 42, 727-742; HAL 656-685 | Stop attempts HAL stop up to 100 times with 25 us delay after failure, nominal 2500 us accumulated busy delay | Add HAL/instruction/scheduling cost. HAL stop can mutate before failure; its final FIFO flush has a finite 1000-read loop whose error is ignored. Direct stop observes only the result the SDK exposes. |
| DRIVER 187-192, 300-318; DRIVER-H 190-200 | Start invokes initial link synchronously; running timer calls `get_link` and discards its return; default period 2000 ms | Owner-selected `check_link_period_ms` is observation scheduling, not detection latency proof. Include timer-task workload, delayed callbacks, admission failures and observation execution. |
| DRIVER 323-344; TIMER 260-280, 524-527 | Stop changes FSM first and nonblocking timer stop does not drain an already captured callback | Fence/drain outside callback-required locks, finite deadline and poll cap. Callback-count zero alone is insufficient. |
| FreeRTOS `projdefs.h` 42-46 | Default `pdMS_TO_TICKS` multiplies then divides with truncation | At 100 Hz, 2 ms converts to zero ticks. Readiness must compare high-resolution elapsed time after reset release; tick sleeps only schedule those comparisons. |

The exact formulas use caller-qualified values, not guessed numeric allowances:
let `F` include complete batch acquisition/release and fixed overhead, `W` each
actual terminal frame's scheduling-inclusive bound, `L` clock lag, and `N` a
complete mediator notification's scheduling-inclusive bound. Four-frame MMD
read/write requires `F + 4W + L`; five-frame MMD RMW requires `F + 5W + L`.
The live collector in `src/DP8382xLink.cpp` uses two batches of eight and two
reads, so reserve both batch fixed contributions, ten frame contributions and
clock/admission overhead. A changed UP publication can add five notifications
DOWN/SPEED/DUPLEX/PAUSE/UP. The unchanged complete publication emits none.
Admission also needs core/adapter execution and owner scheduling overhead;
these formulas are components, not universal elapsed-time guarantees.

Controller acquisition is distinct from the immediate instance-gate admission.
The fixed declaration must cover the configured acquisition allowance and
quantization; no controller lock survives settling waits, future polls or output.
Cooperative operations additionally include finite poll intervals/counts, exact
chip waits, independent cleanup decisions and finite stop/fence work. Bounds
require an identified maximum interrupt/preemption load, owner service interval,
timer-task backlog and callback execution time. Arbitrary task starvation has
no finite supported wall-clock bound. A finite loop or a late-return detector
cannot preempt a blocked SDK call or prove these workload assumptions.

P4 `esp_timer_impl_systimer.c` lines 68-75 convert the counter through
`ticks_to_us` and alias `esp_timer_get_time` to that implementation. Its early
initialization lines 159-164 install the port conversion functions. The actual
P4 `systimer.c` lines 10-25 describe the 16 MHz counter and implement integer
`ticks / 16` and `us * 16`. Thus reported whole microseconds are floor-rounded;
the getter's comment hoping for execution below 1 us is not a source guarantee
of scheduling-inclusive return lag. Clock declarations must cover quantization
(at least 1 us for this conversion alone), sampling/return delay and identified
preemption conditions. A timestamp observed after completion needs that
conservative lag before it becomes an upper completion anchor.

The prompt-39 `prepareBoard` compares `esp_timer_get_time()` against the observed
reset-release time plus 2000 us plus the declared clock lag, waits with
`vTaskDelay(1)`, and keeps shared-controller admission
closed. This avoids the zero-tick 2 ms shortcut at 100 Hz; late scheduling can
extend the silence and consume the aggregate deadline. It is software source
evidence; the actual reset waveform, clock continuity and wire silence still
need the admitted fixture measurement. Factory/fixture allowances remain
explicit unmeasured application assumptions until measured/qualified.

## Notification and pause dispositions

DRIVER 149-176 calls MAC first, records LINK before event posting, and stores
speed/duplex only after their MAC callbacks return success. Successful earlier
fields cannot be rolled back by a later failure. MAC 401-420 returns success for
valid UP/DOWN while dropping internal start/stop returns; MAC start 715-724
resets DMA descriptors. On this exact P4 policy, a returned valid LINK error
therefore means lost event after accepted MAC/cache publication, and event-only
retry must not replay UP. Another MAC can return after uncertain effects;
do not infer which layer failed from the shared `esp_err_t` value. Factory
selection of the policy and retained accepted/uncertain receipts are required.

BASE 232-300 observes only link transitions and updates its whole link cache at
the end, after SPEED/DUPLEX/PAUSE/LINK callbacks. Reject that strategy for
same-link changes and partial publication. The adapter needs acquired evidence
separate from each accepted field, complete-state equality for duplicate
suppression, explicit DOWN/UP resynchronization and stop/start interleavings.
Down/incomplete negotiation cannot manufacture resolved speed/duplex.

PHY-H 184-195 declares a `uint32_t ability` but specifies no 0..3 bitmask
encoding. BASE 357-371 maps any nonzero value to both ANAR pause bits; it does
not implement the portable four-combination contract. DRIVER 491-494 applies
MAC flow policy first, then passes the same object as `bool*` and `uint32_t*`.
Keep the owner's separately typed calls and exact acknowledged 0..3 portable
mask; rejected PHY acknowledgement can follow an accepted MAC policy effect.

BASE 289-297 uses full duplex plus partner symmetric bit as Boolean peer ability;
MAC 493-513 combines that Boolean with separately enabled local MAC policy.
This is narrower than negotiated RX/TX resolution and may be true when local
portable pause resolves to neither direction. Preserve the complete portable
IEEE matrix and PHYCR cross-check in `src/DP8382xLink.cpp`,
`docs/register-vectors.md` and `docs/link-observation.md`. Native projection must
remain qualified by completed AN/full duplex/AN-capable partner, with zero for
forced mode despite stale ANLPAR bits. No new register or IEEE fact is introduced
by this audit. Reject forced 100/half plus extended-full-duplex configuration
before mutation because that native resolved tuple cannot be published.

## Independent P4 link build environment

The source-audit task was extended to build the real fixture in the independent
short folder `C:\esp\build\dp8382x-p39`. Commands run from the repository root:

```powershell
.\scripts\idf.cmd -C test\esp_idf\adapter_link -B C:\esp\build\dp8382x-p39 set-target esp32p4
.\scripts\idf.cmd -C test\esp_idf\adapter_link -B C:\esp\build\dp8382x-p39 build
.\scripts\idf.cmd --version
```

Actual observed tools: ESP-IDF **v6.1** at the full commit above, Python
**3.12.10**, `riscv32-esp-elf-gcc` **15.2.0**, crosstool-NG
**esp-15.2.0_20251204**, CMake **4.0.3**, Ninja **1.12.1**, and esptool
**5.6.0**. Versions were read from the invoked tools, not inferred from sibling
support ranges. Configuration selects `esp32p4`, `CONFIG_FREERTOS_HZ=1000` and
minimum chip revision **3.1** (301), maximum **3.99**. The 100 Hz readiness case
belongs to host contract tests; this image uses 1000 Hz ticks.

The initial complete build compiled the actual adapter at steps 1023/1026 of
1130, then linked the real P4 MAC and produced a checked image. A post-link
timestamp check detected subsequent concurrent adapter/header changes, so an
incremental wrapper rebuild was required before recording final image evidence.
This is a source-freshness check, not repeated verification of unchanged inputs.

The incremental rebuild completed successfully, including actual native C++
compilation, archive/link, image generation and partition-size checks. At the
post-build check on **2026-11-17 11:23:53 UTC**, the source hashes were:

| Repository source | SHA-256 |
| --- | --- |
| `src/esp_idf/Phy.cpp` | `c4467f687c28399da1c4140c5fea1655f8e025ca07bbeec70dd93952cd699d33` |
| `src/esp_idf/PhyControl.cpp` | `4866c0b30f68f7257e47ccc0b0a2734383abe087c875c1d633de3f5f709b7ac4` |
| `include/DP8382x/esp_idf/Phy.h` | `b5d55d92768471c30c995530ce5b38977a7ed71efb4f823a2cb2c36abc9e97a9` |

`Phy.cpp` and the header were last written at **11:22:47 UTC**; the actual
`Phy.cpp.obj` was compiled at **11:23:16.435 UTC**, SHA-256
`9baf6a71ded10d06e827e64db734e6c61a3fcb0625cfcd17be4952881a4a9086`.
`PhyControl.cpp.obj` was rebuilt at **11:23:15.934 UTC**. Both entries appear in
the actual compile database and link map. Demangled ELF symbols include
`Phy::prepareBoard`, `execute`, `getLink`, `publish`, `start`, `stop`,
`setFlowControl`, real `emac_esp32_read_phy_reg`, `emac_esp32_write_phy_reg`,
`emac_esp32_set_link`, `esp_eth_start` and `esp_eth_stop`. This verifies the real
adapter/MAC route was linked; no successful production stub replaced it.

Final outputs in `C:\esp\build\dp8382x-p39`:

| Artifact | Bytes | SHA-256 |
| --- | --- | --- |
| `dp8382x_adapter_link.bin` | 377248 (`0x5c1a0`) | `5f00a4ac84188d511ca66627850ce2e5d774caa9948372b91679321b8850ce40` |
| `dp8382x_adapter_link.elf` | 7212068 | `12cb2432a4b0d6af10a2f2635aa8419ad4a466b8e20097441b6c53b92e357eed` |
| `bootloader/bootloader.bin` | 23952 (`0x5d90`) | `dcb119d596fe23a3e4199fcffc298a874c65cb2417baca414c82495690f22ce5` |

ELF and BIN were written at **11:23:30.247 / 11:23:30.690 UTC**. The application
fits the `0x100000` partition with `0xa3e60` bytes (64%) free; bootloader size
check reports `0x270` bytes (3%) free. Read-only `esptool --chip esp32p4
image-info` reports a valid checksum and validation hash, IDF v6.1, application
version 0.1.0, seven image segments, DIO/80 MHz/2 MB flash and the revision range
above. The cached SDK application descriptor's compile time is
`Nov 17 2026 12:21:10`; it is not the latest source-build timestamp. The explicit
source/object/link observations establish freshness for this dirty-tree image.

Generated `adapter_identity/FixtureIdentity.h` records
`456b9710095b4e1394cfb8d4acdeb90b722d981f` / `dirty`; the matching strings were
also found in the final BIN. This is **PASS for an actual SDK link of these
working-tree sources**, not evidence for a later clean commit. The lead owns
post-commit identity refresh and the final commit/sync evidence. SDK tracked state
remained clean. No `flash`, serial fixture, hardware operation or remote CI ran.

## Preserved gates and evidence boundary

The `native_rx_quiescence` source gate remains intentional. The MAC RX task can
capture the old mediator before application callback entry. STOP, timer fence,
zero application callback count and mediator replacement do not prove safe
reusable teardown. Keep installed driver/glue/netif/MAC/PHY, synchronization and
all borrowed contexts alive until system restart; physical Q13 cannot override
this known software lifetime boundary. Hidden HAL FIFO-flush errors remain a
separate observation limit.

Fresh peer/SDK identity and source checks: **PASS**, read-only commands above.
Host contract and portable consumer execution: **NOT RUN by this delegated
audit**; the lead's integrated report owns their actual results. SDK image link:
**PASS** for the identified dirty-tree sources and image above. Remote CI:
**NOT RUN**, first-execution prompt 71. Physical timing and
fixture operation: **NOT RUN**. Reusable installed teardown: **BLOCKED** by
`native_rx_quiescence`. Scheduler-inclusive numeric bounds remain conditional
on identified workload and measured/qualified declarations. No stage completion,
Git commit or upstream synchronization is claimed by this delegated report.
