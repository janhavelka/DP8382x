# Prompt 54 · Lifecycle, deadlines and retained evidence

## Phase (a) assessment

The board reset path can miss a backward clock step between two phases.
The basic network example also discards the direct result of a link-event
retry. Phase (b) owns these corrections and the test gaps below. Prompt 54
remains in progress.

The assessment used clean `main` at `f3f369c0e1070b29cdc9e886cb802369f3fcb609`.
The wrapper verified the prompt 53 push. Local HEAD and `origin/main` matched
with zero commits ahead or behind. The prerequisites are the installed-owner
and callback regressions from [53](53-regression-transaction-ownership.md),
the offline campaign and interruption evidence from [52](52-hil-soak-interruption.md),
and the shipped lifecycle, link, collector and health APIs. The DP83826I
ledger assigns zero of its 52 procedures to prompt 54. The existing
[feature matrix](../../feature-coverage.md) continues to own feature coverage.

Two read-only agents used the session's high-reasoning model for the ordering
of portable clock/effect observations and native callback/teardown effects.
The lead checked their source anchors, traced CLI scheduling and replay,
inspected live peer code, and exercised the compiled baseline.

## Sources and decisions

- **DP83826I.** SNLS647I §7.6 gives a 25 µs minimum reset pulse without
  debounce capacitors and a 2 ms maximum reset-to-SMI-ready interval. §8.3.11
  requires bus silence during readiness. The 1,500 ms FLP value is nominal.
  Use the reset-specific command semantics in tables 8-19 and 8-48 and the
  §8.3.13 requirement to write VOD_CFG3 (MMD 31, 0x030E) to 0x4A40. Check
  latch-low, remote-fault, interrupt and counter consumption against tables
  8-20 and 8-36 through 8-39. Preserve the open RECR interpretation and
  the documented EEE timing qualifiers. Original PDF SHA-256 is
  `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
  [TI datasheet](../../reference/source/TI-DP83826I-E-SNLS647I.pdf)
- **ESP-IDF.** The clean v6.1 checkout is `fff9895c82d744c7237be8847347bdd1b07c6643`.
  In `components/esp_eth/src/phy/esp_eth_phy_802_3.c`, a reset budget below
  10 ms permits the write and zero completion polls. AN restart can return
  success with negotiation incomplete. In `src/mac/esp_eth_mac_esp.c` under
  the same component, polling proceeds in 100 µs delays up to a nominal
  accumulated 1,000 µs. Record actual modeled elapsed time, launch and
  completion independently. P4 wall time still needs measurement.
  `esp_eth.c` changes framework state before all start/stop effects complete.
  `esp_eth_netif_glue.c::eth_action_stop` calls the void
  `esp_netif_action_stop` in `components/esp_netif/esp_netif_handlers.c`.
  It exposes no acknowledgment of session closure to the application.
  [Pinned SDK evidence](../../reference/library-sdk.md)
- **TMP1x2**, clean `c91bac63313ea725ce174ab98abc50705c365639`.
  `include/TMP1x2/TMP1x2.h`, `src/TMP1x2Operations.cpp` and
  `test/jobs/test_jobs.cpp::{externalClockConversions,cancellationAndTeardown}`
  retain token and effect evidence, record the transfer clock, and count
  calls across cancel. Root CMake lists the operation translation unit and
  registers the jobs executable. `examples/common/Tmp1x2CliDiagnostics.cpp`
  takes an owner result, and `tools/hil_tmp1x2_runner.py` distinguishes
  operation admission from completion. Adopt the independent timestamps and
  explicit registration. DP8382x uses nonwrapping uint64 microseconds and
  accounts for each complete MDIO batch.
- **24Cxx**, clean `0bb27f46beb4fbe907beb11d9134de4e1797347d`.
  `include/EEPROM24Cxx/EEPROM24Cxx.h` and `src/EEPROM24Cxx.cpp` retain
  accepted prefixes across cancellation. `examples/common/Eeprom24CxxCli.cpp`,
  `test/test_cli/test_cli.cpp`, `tools/test_cli_transcript.py` and
  `tools/test_hil_runner.py` check cancellation, partial terminals and
  restored state through the compiled producer. Adopt that test boundary
  with DP8382x boot/request/token correlation and separate restoration.
  EEPROM write-cycle delays and CLI markers are specific to the sibling.
  Both peers' AGENTS files, CMake registrations and CI workflows were
  inspected. DP8382x has no tracked workflow at this checkpoint.

## Coverage already registered

| Prompt or boundary | Existing named tests and what to retain |
| --- | --- |
| 43 · CLI | `cli_processor_grammar`, `cli_processor_loss`, `cli_processor_cached`, `cli_processor_retention`, `cli_console_line_boundaries`, `cli_console_atomic_records`, `cli_console_writer_backpressure`, `cli_fixture_transcript`. Current `cli_native_console_input_gap`, `cli_native_console_retention` and `cli_processor_operation_edges` cover fragment loss, a full queue, slow draining and denied result delivery. |
| 48 · Protocol and files | `cli_hil_runner`, `test_every_split_and_one_byte_reads`, `test_live_duplicate_during_closure_invalidates_pass`, `test_verify_rejects_tampered_manifest_and_transcript`. Replay checks malformed/trailing records and corruption after publication. |
| 49 and 52 · Serial and campaigns | `cli_hil_serial` runs `test_preopen_provisional_and_fragmented_pass_replay`, `test_timeout_never_promotes_late_fragment_and_recovers_once` and a compiled-producer case. `cli_hil_campaign`, `cli_hil_soak` and `cli_hil_interruptions` cover zero/skip-only plans, interrupted cleanup, output stalls and incomplete receipts. |
| 53 · Ownership | `native_cli_cross_entry_boundaries`, `native_raw_accepted_write_timeouts`, `native_cli_restart_after_raw_observation`, `native_start_callback_ownership`; also retain `operation_timing`, `reset_floor` and `eee_ownership_timing`. Instance admission, controller ownership, possible write effects and callback release have distinct receipts. |
| Lifecycle, timing, reset | `operation_admission`, `operation_timing`, `operation_failures`, `operation_lifecycle`, `reset_traces`, `reset_floor`, `reset_timing`, `verify_barrier`, `verify_comparisons`, `verify_interrupted_replay`. These cover zero and exhausted budgets, frozen and regressed clocks, overflow, illegal callback progress, accepted writes, repeated bind/end, stale tokens, separate restoration and HARD silence. `test/reset.cpp` checks the literal VOD write then readback. A failed stable comparison leaves restoration unproved. |
| Passive observations | `events_failures`, `events_history`, `events_ownership`, `link_transitions`, `link_coherence`, `link_failures`, `health_raw_local`, `health_configuration`, `health_isolation`, `health_saturation`, `owner_policy`. Preserve acquired prefixes and explicit possible loss, valid zero timestamps, passive/invalid-call counts and independent instance state. Keep cable loss, negotiation pending, stale link, policy mismatch, unverified settings and transport fault distinguishable. |
| Installed owner and frame lifetime | `pinned_driver_install_failures`, `pinned_driver_glue_rx_lifetime`, `native_partial_start_fence`, `native_stop_failure_receipts`, `native_uninstall_gate_contention`, `native_timer_publication_during_stop`, `native_live_tuple_changes`, `native_notification_partial_receipts`, `native_incomplete_negotiation`, `frame_session`, `native_application_frames`. The instance gate also protects snapshots. Queued RX and TX carry the admission generation. Receipt of a late callback cannot establish when the frame arrived on the wire. |

## Phase (b) implementation brief

1. **Portable polling and allocations.** Extend the real `startInitialize` /
   `pollOperation` fixture in `test/operations.cpp`. Poll every 100 µs
   through a 2,000 µs wait on a 1 µs clock. Check every early call, the
   first eligible read and eventual completion, using the command completion
   sample plus lag as the anchor. On a 100 Hz floor clock with 10,000 µs lag
   and a 5,000 µs interval, a post-write raw sample of zero gives a 15,000 µs
   due time. Make repeated polls within ticks zero and 10,000 with the actual
   default `maxStalledPolls=100000`. At 20,000 expect the first eligible
   read. Count frames and stalled polls independently. Preserve the existing
   frozen-clock and zero-I/O yield cases. For the four-frame MMD step after
   identity, allow a one-frame slice while the batch fits. At
   `raw + 1 + 8 + 4*4 == deadline` expect terminal BOUND_ADMISSION with zero
   additional calls. Keep the separate expired zero-slice DEADLINE case.
   Add a measured post-setup allocation check with a positive control, a
   fixed-storage fixture and real polling, result delivery, destructive
   acquisition and passive projections. Record which allocator entry points
   the host test observes. The native fake's allocation tracker counts only
   its own allocations. [Operation contract](../../api-contracts.md)
2. **Native board clock and budgets.** In `src/esp_idf/Phy.cpp::prepareBoard`
   track the last observed sample from entry through controller drain,
   each wake, both sides of both pin callbacks, and final readiness. Exercise
   backwards time before assertion, during the hold, before deassertion and
   after release. A regression closes admission even before first pin use.
   After any attempted assertion make exactly one release attempt and retain
   primary and cleanup failures independently. Check zero MDIO and the actual
   shared bus state. Extend `native_board_reset_window` with one readiness
   delay already consumed, lag 1 µs, hold 101 µs and post-release quiet
   2,001 µs. At 100/333 Hz total `maxPolls` 5 denies before the pins and 6
   admits. At 1,000 Hz use 7 and 8. Check elapsed intervals and delay counts.
   Retain `native_board_tick_readiness` and pre-GPIO recoverable budget
   denials. Use `InstrumentedMac::elapsedUs` for the nominal 100/1,000 µs
   frame cases, testing a late accepted write and the remaining aggregate
   deadline at the next transfer. Compare sub-10-ms reset acceptance and
   subsequent expiry to exact frame traces. Keep AN control completion and
   eventual resolution as separate observations.
3. **Basic application receipts.** In `examples/esp_idf/basic/main/main.cpp`
   retain and expose the direct `retryLinkEvent` result alongside the
   original failure and subsequent journal. In `test/esp_idf/host/example.cpp`,
   extend `example_last_event_retry` and `example_last_event_retry_on_time`
   so the accepted eleventh event attempt reports TIMEOUT or OK as sampled,
   event loss clears and the retry adds no MAC UP. For the pending-result
   oracle, freeze MDIO_ERROR with a distinct detail, make the next poll gate
   return BUSY, allow the snapshot and deny the bounded takes. Check the
   reported frozen status/detail and token, cleanup BUSY, CONTROL phase
   and exactly one later delivery. Start with the six existing start, stop,
   STOP-event, start-timer, stop-timer and start-MDIO fault cases. Inventory
   the remaining injection sites in the same file, including
   `terminalDeliveryFailure`, `runningStopRequired`, `runningFailures`,
   `failedCallWithoutJournal` and `cliNetwork`. For each injected start,
   stop, timer or MDIO failure, prove the injection fired and check both
   error domains, phase, timer and event state, live resources and the
   permitted next owner action. Reuse `example_attach_partial_failure` and
   `example_board_release_failure`.
4. **Retirement and late RX.** Deliver START, pause RX after the MAC loads
   the mediator, stop and retire the basic app while STOP is still queued.
   Verify the post, handler count, expected STOP and retained object graph.
   Resume RX on the retained graph with STOP withheld and with later STOP
   delivery. Verify buffer ownership and the glue's observable action in
   each ordering. Record actual netif session closure as unresolved.
   Reusable installed teardown remains blocked on `native_rx_quiescence`.
5. **CLI cross-entry test.** A phase (a) probe of the compiled producer
   used the following input with `--trace-resets`. It observed one BMCR write
   (`00 8000`), a passive nonce on request 2 while token 2 was running,
   request 1's later CANCELLED result with accepted mutation and cleanup
   owed, and successful restoration under token 3. Use this transcript in
   the owning fixture. Add fragmented input across cancellation and bounded
   output stalls. Check original and control request IDs, unchanged transfer
   counts for the nonce, one terminal per admitted operation, and fair owner
   service while output is retained. Preserve the serial rule for partial
   writes, which makes framing uncertain. [CLI session contract](../../cli.md#session-handshake)

```text
$pause
recovery reset pcs
$tick 8
session P54A
owner status
owner cancel 2
$resume
owner result 2
recovery reinitialize none
```

Give one implementer the portable fixture and allocation tests and the other
the native board implementation and its host vectors in
`test/esp_idf/host/native.cpp`. The lead owns shared headers, registrations,
application/CLI work, integration and the commit request. Extend existing
C++17 interfaces only where an observable receipt requires it. Keep borrowed
context alive through retained results and cleanup. Serialize all driver and
snapshot calls under the application owner. Release the bus lock before
waiting or printing. Keep separate operation tokens and finite budgets for
restoration. Phase (c) should independently review clock/admission ordering
and the installed-owner/result lifetimes against the implementation diff.

Review public Doxygen, `docs/api-contracts.md`, `docs/esp-idf.md`, the basic
example guide, `test/README.md` and affected matrix/CLI rows alongside the
changes. README and CHANGELOG were reviewed at entry. Phase (a) changes
only this report and the progress row. Phase (b) should run focused Release
and UBSan trap suites, the tests-OFF consumer and source checks, and link the
adapter fixture and both P4 applications through `scripts/idf.cmd` when the
adapter changes.

## Phase (a) verification

On Windows x86-64 with GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1 and Python
3.12.10, the existing pinned Release build had tests enabled. Rebuilding it
verified 45 SDK inputs at the v6.1 pin. The first CTest selection passed
475/475 in 158.84 s. The second passed 8/8 in 12.86 s. These runs include
all 53 CTest names in the coverage table. Its five named Python cases also
resolve to actual tests. The baseline and nonce probe exercise the entry
commit. The new phase (b) scenarios still need implementation.

```powershell
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'safe.directory'
$env:GIT_CONFIG_VALUE_0 = 'C:/esp/v6.1/esp-idf'
cmake --build build/p53-baseline --parallel 4
$selected = '^(operation_|reset_|electrical_|events_|health_|verify_|link_|negotiation_|transport_reports$|cli_(processor_|native_console_|console_|fixture_transcript$|hil_(runner|serial|campaign|interruptions)$)|native_|example_|owner_|pinned_driver_)'
ctest --test-dir build/p53-baseline -R $selected --output-on-failure --no-tests=error -j 1
$extra = '^(frame_|forced_replay$|transport_(timing|ownership)$|eee_ownership_timing$|cli_hil_soak$)'
ctest --test-dir build/p53-baseline -R $extra --output-on-failure --no-tests=error -j 1
python -m unittest discover -s test -p "test_*.py" -v
python tools/check_doxygen.py
python tools/verify_references.py
python tools/verify_ledger.py
python scripts/generate_version.py check
git diff --check
```

| Evidence layer | Phase (a) result |
| --- | --- |
| Host, CLI and pinned SDK driver | **PASS**, 483 selected registrations, plus the real-core nonce probe. Python discovery ran 238 tests, 189 passed and 49 compiled-fixture cases skipped in standalone discovery. |
| Source and documentation | **PASS**, 293 Doxygen pages and two negative fixtures, 29 reference payloads and 27 extracts, ledger checks for 97 registers, 52 procedures and 35 features, version and whitespace checks. |
| Portable and exported package consumer | **NOT RUN** in this planning phase. |
| P4 SDK link | **NOT RUN** in this planning phase. Prompt 53 records the three earlier image links and their tested SHA. |
| Remote CI | **NOT RUN**. Prompt 71 owns the first-execution checkpoint. |
| Physical evidence | **NOT RUN**. Board/PHY qualification needs the identified, authorized fixture. Reusable installed teardown remains **BLOCKED** on RX quiescence. |

Phase (b) can proceed after the wrapper verifies this report and progress
checkpoint against the configured upstream.

## Phase (b) implementation

The wrapper verified `main` and `origin/main` at
`a741d0d406660f8aafd3265ccf36310f941946fc`. The index and worktree were
clean. Both implementers used the session's high-reasoning model for clock
ordering and partial effects. One owned the portable fixture. The other
owned the board procedure, native fixture and clock hook. The lead integrated
the shared declarations, registrations, basic application, CLI fixture and
guides. Both implementers finished. Prompt 54 remains in progress.

The peer paths in phase (a) were reopened at the same clean revisions.
TMP1x2's clock/cancellation tests and 24Cxx's accepted-prefix and CLI tests
support retaining frame and token receipts. All timing oracles here use the
DP83826I and SDK sources listed above. In the pinned SDK, the P4 MAC polling
loop and generic PHY reset/AN loops use different intervals. In
`components/esp_timer/src/esp_timer.c`, the initialized timer service
removes an armed timer under its list lock. The host's injected failing stop
that leaves it armed exercises a stronger fault model.

| Boundary | Implementation and observed result |
| --- | --- |
| OP-5 polling and short slices | `operation_poll_schedules` issued no read on 20 early fine-clock polls or 19,974 early quantized polls. After the PCS write, the first eligible BMCR read was frame 14 in both cases. Both operations completed. `operation_slice_bound` yielded at raw 174 µs and froze BOUND_ADMISSION at 175 µs with the same one-frame slice and zero additional transfers. The existing zero-slice DEADLINE and frozen-clock suites also ran. |
| Hot-path allocation | After binding fixed caller storage, `operation_allocations` ran reset initialization, polling, delivery, destructive acquisition of six words and cached views. Thirteen batches and 38 frames completed with zero calls to ordinary global scalar or array C++ allocation. Both positive controls fired in Release. Direct C allocation, over-aligned allocation and SDK allocators are outside this result. |
| Board preparation | `Phy::prepareBoard` checks one sequence of samples through drain, wakes, pin callbacks and final readiness. `native_board_clock_regression` injects nine cuts and checks FAILED_CLOSED, zero MDIO and zero pin calls before assertion or exactly one compensating release after assertion. The SDK entry returns INVALID_ARG for a primary clock fault. The journal records INVALID_CONFIG/CLOCK_CONTRACT, and retains a separate failed release. The reverse error order is also tested. After one readiness wake, the `native_board_reset_window` hold/quiet reservation rejects 5 polls and admits 6 at 100 and 333 Hz. At 1,000 Hz it rejects 7 and admits 8. |
| Controller and negotiation | `native_mdio_elapsed_deadlines` gives PCS reset 9,000 µs. Twelve prerequisite frames cost 100 µs each, and the accepted reset costs 1,000 µs. Under three cuts, the exact 13-frame trace ends with that write. The next poll rejects at bound equality or expiry, or the callback returns late. Each result retains the accepted write and restoration debt. `native_incomplete_negotiation` checks the literal restart read/write/read before subsequent resolution. |
| Application failures | `example_start_failure`, `example_stop_failure`, `example_stop_event_failure`, the two timer faults and `example_start_mdio_failure` check injection consumption, both error domains, installed graph, phase, active timer, posted events and subsequent owner admission. The same checks cover running MDIO faults, an unavailable journal, failed CLI stop and denied result delivery. The synthetic active-timer fault enters STOPPING before retirement. Its callback in STOPPED raises unexpected-start cleanup, the owner rejects restart before a second periodic arm, and retirement posts a second STOP after one START. In retained maintenance the active model timer issues zero frames. |
| Retry and result delivery | `example_last_event_retry` and its on-time counterpart observe the eleventh CONNECTED attempt accepted after ten failures. The direct return is TIMEOUT or OK respectively. Both clear event loss, preserve the original SDK error and issue only one MAC UP before the next start. The application retains and prints the last direct retry receipt. In `example_terminal_snapshot_delivery` the poll gate returns BUSY, the snapshot identifies MDIO_ERROR/264, and all bounded takes fail. The example prints the frozen status and retains the token in CONTROL with cleanup BUSY. One later take succeeds and the duplicate fails. |
| Late RX | `example_retired_rx_before_stop_delivery` and `example_retired_rx_after_stop_delivery` deliver START, pause RX with a captured mediator and retire with STOP queued. One expected STOP remains until the handlers run. Both orderings reach the retained netif and free the one buffer through the glue. The void netif stop action supplies no acknowledgment of actual session closure. |
| CLI cancellation | The compiled transcript feeds a partial cancel line, steps the owner, and completes the line while output is stalled. The passive nonce leaves frame/batch counts unchanged. The original request freezes CANCELLED under token 2 after one PCS write. The control request identifies that origin, later result lookup agrees and token 3 restores persistent settings. Consult the [fixture controls](../../../test/cli/README.md). |

### Phase (b) local checks

Windows x86-64, GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1 and Python 3.12.10.
The pinned build verified 45 SDK sources/headers at `fff9895c`. Use the
process-local safe directories, cached clangd path, `CCACHE_DIR` and
three-project `scripts/idf.cmd` loop from the
[preceding build receipt](53-regression-transaction-ownership.md#phase-b-implementation).
The selected Release suites include the compiled Python serial, campaign,
soak and interruption fixtures. The standalone native UBSan build instruments
both C and C++. The separate portable UBSan build exercises the operation
suite with the same trap flags.

```powershell
cmake --build build/p53-baseline --parallel 5
$selection = '^(operation_|reset_|electrical_|events_|health_|verify_|link_|negotiation_|transport_|eee_ownership_timing$|frame_|forced_replay$|cli_(processor_|native_console_|console_|fixture_transcript$|hil_(runner|serial|campaign|soak|interruptions)$)|native_|example_|owner_|pinned_driver_)'
ctest --test-dir build/p53-baseline -R $selection --no-tests=error --output-on-failure -j 1
cmake --build build/p53-review-ubsan --parallel 4
ctest --test-dir build/p53-review-ubsan -R '^(example_|native_(board|mdio_elapsed|incomplete_negotiation|cli_link_events$))' --no-tests=error --output-on-failure -j 2
ctest --test-dir build/p54-portable-ubsan -R '^(dp8382x_operation_tests_failure_path|operation_)' --no-tests=error --output-on-failure
cmake --build build/p53-consumer --clean-first --parallel 4
ctest --test-dir build/p53-consumer --no-tests=error --output-on-failure
```

| Evidence layer | Phase (b) result |
| --- | --- |
| Host | **PASS**. 497 selected Release registrations in 171.27 s, 104 native UBSan registrations and ten portable UBSan registrations. The dedicated native selection at three tick rates also passed 302/302. After the pre-retirement assertions were added, all 70 application CTests passed again in Release (0.49 s) and UBSan (0.44 s). The initial timer-fault expectation of STOPPED was corrected to STOPPING after tracing the active fake timer. The initial MAC-deletion expectation was also corrected to retain the failed deletion receipt and its modeled effect. |
| Portable consumer | **PASS**, both clean-rebuilt tests-OFF consumers. Exported package **NOT RUN**. |
| Source and documentation | **PASS**, Python discovery ran 238 with 49 fixture-dependent skips. Doxygen checked 293 pages and rejected both negative fixtures. Reference, ledger, version and whitespace checks passed. |
| P4 SDK link | **PASS**, three ELF/binary pairs from the adapter fixture, basic application and owner application. Each identity header records `a741d0d` with a dirty worktree. |
| Remote CI and physical | Both **NOT RUN**. Prompt 71 owns CI. Board qualification requires an authorized fixture. Reusable installed teardown remains **BLOCKED** on public RX quiescence. |

| ESP32-P4 binary | Bytes | SHA-256 |
| --- | ---: | --- |
| `53l/dp8382x_adapter_link.bin` | 387504 | `e29cf482ea7f3e66d99390bf15288292e13661492d32edf291795ccf7f044e7d` |
| `53b/dp8382x_p4_ethernet.bin` | 682976 | `369ee8943bfd108495400cdb0900138bd471701fc4dcefff84d9dc5d578ef189` |
| `53o/dp8382x_p4_owner.bin` | 561296 | `f8793c5afcfdaeec60e65e1470e83bf1a25ab68178959dbaa0bd569073025284` |

In phase (c), review the actual implementation commit against `a741d0d`.
Independently trace the sample order, remaining poll allowance and write
effects, and the callback, event, result and borrowed-context lifetimes.

## Phase (c) independent review

The wrapper verified the implementation commit `bafaeddffdaf276339e8b369652b5ddde9448b28`
on `main` and `origin/main`. The index and worktree were clean, 0 ahead and
0 behind. Two read-only reviewers independently traced the diff from
`a741d0d406660f8aafd3265ccf36310f941946fc`. Both used the current
high-reasoning model to trace clock, callback and borrowed-object lifetimes.
The lead integrated the corrections below. The private reproductions are in
`build/p54-c-timing` and `build/p54-c-retirement`.

| Boundary | Observation at the implementation commit | Correction and regression |
| --- | --- | --- |
| Deadline at RESET_N release entry | The hold completed before expiry. The next sample reached 160001 µs. A failing release became the primary SDK error. An overlong successful release lost its cleanup error. | Freeze DEADLINE at that entry sample, then attempt release once. `native_board_release_at_deadline` tests successful, failed and 1001 µs releases with a 1000 µs callback bound. It checks both domains, the shared quiet floor, FAILED_CLOSED and zero MDIO at all three tick rates. |
| MAC lifetime after pre-install cleanup | The host MAC modeled deletion before returning ESP_FAIL. A second cleanup called its deleter again. The pinned P4 `emac_esp32_del` frees the object and returns ESP_OK. | The basic example relinquishes its callable pointer on the first deletion and reports the exact unresolved deletion error. `example_mac_cleanup_failure` repeats shutdown and counts one deletion. `example_live_cleanup_denied` retains the separate retry when a live PHY still borrows the MAC. |
| STOP queued by retirement | Directly retiring the active application queued STOP with zero expected deliveries. Later dispatch incorrectly requested an unexpected shutdown. | Reserve the active session's STOP before retirement can post. The two `example_retired_rx_*` registrations run the demo through two sessions, then use direct retirement or an explicit stop. Dispatch consumes one expected STOP in both orderings. Both late RX calls reach retained glue and release one buffer. |
| Owed result across tokens | After delivery of token 3's MDIO_ERROR/264, gate contention throughout token 4's wait made shutdown print the old error for token 4. | Clear the owed status at delivery and associate each observation with its token. `example_terminal_snapshot_delivery` reads both diagnostic lines through the actual console queue. It checks later delivery, duplicate rejection and zero additional frames. The C++ test now checks output and exit status together. |
| CLI result delivery during output stall | A deliberately delayed producer that took CANCELLED only on the first resumed tick still passed. That tick was exactly 4000 µs after cancellation. | Require the original result timestamp to fall before the first resumed tick. The real producer passes. The delayed executable fails the revised assertion at exactly 4000 µs. |

The clock reviewer also verified the fine and coarse OP-5 loops, short-slice
admission equality, the 13-frame PCS trace and the 38-frame allocation
measurement with both positive controls. The application reviewer checked
the direct event-retry code and post count, including a late accepted retry,
and the stricter injected timer-stop failure. STOP posting, delivery to the
glue and netif session closure remain separate observations.

The reviewers and lead reopened the source paths listed under **Sources and
decisions** above. TMP1x2 and 24Cxx remained clean at the recorded revisions.
The shared convention is to retain accepted effects and correlate terminal
results with their owner. Clock bounds still come from the exact device and
SDK. The P4 MAC deleter in `components/esp_eth/src/mac/esp_eth_mac_esp.c`,
framework stop in `components/esp_eth/src/esp_eth.c` and glue in
`components/esp_eth/src/esp_eth_netif_glue.c` were checked at `fff9895c`.
The review changed no silicon values or procedure ownership. README and
the feature matrix still describe the supported scope. The changelog,
board API, native guide, basic example guide and test guide record the
corrected behavior.

### Phase (c) verification

Both reviewers rechecked the fixes. The timing reviewer linked the private
reproduction against the corrected adapter. All three releases gave direct
ESP_ERR_TIMEOUT and primary DEADLINE. Cleanup was respectively OK, SDK
ESP_FAIL and core BOUND_ADMISSION. The application reviewer repeated the MAC,
STOP, two-token and delayed-output probes. All five findings are closed.

An initial regression run exposed an early wake at 333 Hz. From assertion at
60001 µs the first wake at 60061 µs is short of the 60102 µs hold floor.
The corrected test counts the first sample meeting the floor, then injects
expiry on the subsequent release-entry sample. Both final selections passed.

Windows x86-64, GNU 15.1.0, Ninja, CMake 4.0.1 and Python 3.12.10. Use the
same process-local Git settings and ESP-IDF environment as phase (b).

```powershell
cmake --build build/p53-baseline --parallel 4
$release = '^(example_|native_|owner_|pinned_driver_|operation_|reset_|cli_fixture_transcript$|cli_processor_|cli_native_console_)'
ctest --test-dir build/p53-baseline -R $release --output-on-failure --no-tests=error -j 3
cmake --build build/p53-review-ubsan --parallel 4
$native = '^(example_|native_(board|mdio_elapsed|incomplete_negotiation|cli_link_events$))'
ctest --test-dir build/p53-review-ubsan -R $native --output-on-failure --no-tests=error -j 3
ctest --test-dir build/p54-portable-ubsan -R '^(dp8382x_operation_tests_failure_path|operation_)' --output-on-failure --no-tests=error
cmake --build build/p53-consumer --clean-first --parallel 4
ctest --test-dir build/p53-consumer --output-on-failure --no-tests=error
```

| Evidence layer | Stage (c) result |
| --- | --- |
| Host | **PASS**. 426/426 Release CTests in 3.65 s, 107/107 native UBSan in 1.42 s and 10/10 portable UBSan in 0.21 s. The builds verified 45 pinned SDK inputs. Standalone Python discovery ran 238 tests with 49 fixture-dependent skips. The Release transcript registration executes the compiled producer. |
| Portable consumer | **PASS**. Both separate tests-OFF consumers rebuilt and passed. Exported package **NOT RUN**. |
| Source and documentation | **PASS**. Doxygen verified 293 generated pages and rejected both negative fixtures. The phase (a) reference, ledger, version and whitespace commands passed. |
| ESP32-P4 SDK link | **PASS**. The phase (b) three-project `scripts/idf.cmd` loop produced an ELF and binary for each target. All firmware identity headers record `bafaeddffdaf276339e8b369652b5ddde9448b28` and a dirty tree. |
| Remote CI and physical | **NOT RUN**. First-execution CI belongs to prompt 71. Physical qualification needs an authorized, identified fixture. Reusable installed teardown remains **BLOCKED** on RX quiescence. Netif session closure remains unproved after STOP dispatch. |

| Binary | Bytes | SHA-256 |
| --- | ---: | --- |
| `53l/dp8382x_adapter_link.bin` | 387520 | `796ae6f82b9a0aa5f575346bf456884e05888354270ccbc67135a1bdd1837baa` |
| `53b/dp8382x_p4_ethernet.bin` | 683248 | `f869e7b7c2097ce94e8ec12596fbafdb196dea3ddfbb43180547e4f2d3659eae` |
| `53o/dp8382x_p4_owner.bin` | 561328 | `e06e07ee5de7e434711d8b58779c34c03647704f6fd596ec955a252a0b4c172e` |

Prompt 54 remains in progress. Stage (d) starts from the wrapper-verified
review commit and runs final local checks on that checkpoint.

### Phase (c) checkpoint recovery

The earlier wrapper stopped before committing the review. On recovery, all
13 modified paths matched its saved request on `bafaedd`, with an empty index.
Two read-only reviewers rechecked the saved fixes. The board API comment and
native guide now specify that an assertion or hold error keeps precedence
when release starts at expiry. The compiled CLI fixture passed again and
the deliberately delayed producer failed at the 4,000 µs boundary.

The clean TMP1x2 and 24Cxx checkouts had advanced to
`0791429f8fce1170e2082f1b011456fc461318b8` and
`139e76cd38ecdeec40be5fdb1ce5e633158ea59a`, respectively. Only peer
documentation changed since the source review above.

Recovery passed all 426 focused Release tests, 107 native UBSan tests,
ten portable UBSan tests and two tests-OFF consumer tests.
Python discovery ran 238 tests with 49 fixture-dependent skips.
Doxygen, reference, ledger, version and whitespace checks passed. Rebuilding
the three ESP32-P4 targets reproduced every image hash in the phase (c) table.

## Phase (d) final verification

Prompt 54 is complete for the DP83826I software paths. Verification ran on
2026-11-22 on clean `main` at `7fbdab2ad0a9c1c216b2c7cf4f2cef7c7087c044`.
The wrapper had committed all 13 reviewed paths and verified equality with
`origin/main`. The index and worktree were clean, zero ahead and behind.

The lead checked the Release registrations against the phase (a) coverage
table and ran the full suite and source checks. Two agents used the session
model to verify the instrumented builds and the firmware links in separate
build directories. The lead checked the image hashes and embedded Git SHA.
The TMP1x2 and 24Cxx headers, operation sources, CLI, tests, runners,
registrations and agent rules listed above matched the clean phase (c)
recovery revisions. The tests retain peer conventions for token identity and
accepted effects. The reset and controller timing expectations use the
archived SNLS647I and pinned SDK sources.

| Evidence | Result at the review commit |
| --- | --- |
| Host Release | **PASS**, 662/662 CTests in 256.63 s on Windows x86-64, GNU 15.1.0, CMake 4.0.1 and Ninja 1.12.1. The build verified 45 SDK files at `fff9895c82d744c7237be8847347bdd1b07c6643`. All 53 baseline registrations and all 15 added registrations executed. |
| Lifecycle and deadline regressions | **PASS**. The real driver exercised fine and coarse polling, exact slice admission and zero ordinary C++ allocations in the measured 38-frame interval. At 1,000, 100 and 333 Hz the native tests exercised clock regression, reset release at expiry and MDIO elapsed deadlines. The example tested retained terminal status and late RX on both sides of STOP delivery. While output was stalled, the compiled CLI resynchronized the nonce and retained cancellation for token 2. After output resumed, restoration completed under token 3. The offline interruption suite replayed 2,927 prefixes and rejected 436,290 field mutations. |
| UBSan trap | **PASS**, 107/107 native C/C++ tests in 0.72 s and 10/10 portable C++ tests in 0.20 s. |
| Portable build-tree consumer | **PASS**, 2/2 after a clean Release rebuild with library tests OFF. |
| Python and source checks | **PASS**. Python 3.12.10 discovery ran 238 tests in 48.21 s, with 189 passes and 49 fixture-dependent skips. The Release suite also ran the compiled CLI and offline runner fixtures. Doxygen 1.13.2 verified 293 pages and rejected both negative fixtures. References verified 29 payloads, 27 extracts and 2,246 links. The ledger verified 97 registers, 52 procedures and 35 features. Generated version and whitespace checks passed. |
| ESP32-P4 SDK link | **PASS**, three ELF files and application binaries linked at ESP-IDF v6.1. Each build's identity header recorded the clean review commit, and all six outputs contained its full SHA. |
| Exported package consumer | **NOT RUN**. Clean export and relocation verification belongs to prompt 70. |
| Remote CI | **NOT RUN**. Prompt 71 owns first-execution CI. |
| Physical | **NOT RUN**. Physical reset, MDIO timing and network qualification require an identified, authorized fixture. Reusable installed teardown remains **BLOCKED** on public RX quiescence. STOP handler delivery provides no receipt for actual netif session closure. |

| Image under `C:/esp/build/dp8382x-loop` | Bytes | SHA-256 |
| --- | ---: | --- |
| `53l/dp8382x_adapter_link.bin` | 387520 | `e965e9595d7023b3ed4109964b592fd06d74e661b325d7fae98b9e7f40513b23` |
| `53b/dp8382x_p4_ethernet.bin` | 683248 | `2ad760b00509235a6f8b331713d4d465054b571ccef755b457e7642ed606f6fd` |
| `53o/dp8382x_p4_owner.bin` | 561328 | `85b1c6ead155b95f927ae4609eddc2e3bc4839570fb6039aebf0bf37d39427c5` |

Use the process-local SDK Git settings from phase (a) for the Release build.

```powershell
cmake -S . -B build/p53-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p53-baseline --parallel 4
ctest --test-dir build/p53-baseline --output-on-failure --no-tests=error -j 1
```

For UBSan and the tests-OFF consumer, repeat the phase (c) commands, building
`build/p54-portable-ubsan` before its CTest. For the P4 links, use the
three-project `scripts/idf.cmd` loop referenced in phase (b). The phase (a)
Python and source commands reproduce the documentation checks. Whitespace
checks covered `f3f369c..7fbdab2`, the working diff and the index.

The implementation and review commits are `bafaedd` and `7fbdab2`. The
completion request covers this report and the progress row. The wrapper
records the completion commit and its upstream verification.
