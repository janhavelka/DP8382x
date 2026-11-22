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
