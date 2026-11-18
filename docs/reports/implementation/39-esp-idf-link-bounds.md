# Prompt 39: native notifications and timing conformance

Execution date: 2026-11-17. Entry was clean `main`, equal to `origin/main`, at
`456b9710095b4e1394cfb8d4acdeb90b722d981f` after fetch. Prompt 37's native
factory/lifecycle and prompt 38's configuration/raw arbitration, plus the portable
link/pause collector, are the concrete prerequisites. No ledger procedure has
implementationStage39: this stage closes adapter conformance and corrects defects
in the existing owner. No new chip procedure, base inheritance or lifecycle engine
is introduced. Remote CI remains prompt 71 only.

## Ownership, sources and review

Re-read COMMON, CONVENTIONS, ENGINEERING, VERIFICATION, parent/root rules,
README, architecture, variants, reference index, selected SDK catalogue, design
decisions 2-7 and OP1-17. Preserve C++17/core isolation, one owner/collector, complete
bus batches, finite terminal callbacks, explicit partial effects and separate
physical evidence. The lead owns production/header changes, shared guides,
progress, integration and Git. Sibling/SDK trees remain read-only.

Parallel assignments used gpt-6.1-sol/high for focused native tests and fresh
peer/source inventory, and gpt-6-astra/high for independent ownership/deadline
review. The stronger reviewer examined notification receipts, SDK field effects,
timer/start contention, teardown and clock rounding against actual code. The lead
verified findings against source/diff, integrated fixes and retained existing
regressions. Tests are owned under test/esp_idf; the source inventory owns only
[39-native-source-audit.md](39-native-source-audit.md). Separate agent build
folders prevent shared build mutation. Model choices follow COMMON/AGENTS.

The fresh audit records exact unchanged clean peer revisions and actual production
methods behind their native/error/lifecycle tests, CLI/runners and workflows.
I2C fake-bus/Arduino tests are not native MDIO execution evidence. The exact SDK
remains unmodified v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643` for P4; no
supported range is inferred. Compiler dependency checks verify 45 SDK blobs in
both 1000Hz and 100Hz host configurations. There is no repository workflow yet;
creation/remote execution belongs to 71.

## Confirmed findings and fixes

- Publication previously reacquired the instance gate after every synchronous
  mediator call. Legitimate callback/snapshot contention could lose accepted
  fields and leave notifying permanently true, blocking even stop/retire.
  One terminal receipt now keeps accepted fields, current event loss, uncertainty
  and error, then the next gated entry adopts it. P4 accepted LINK is never
  replayed, including after an event-post error. Generic MAC uncertainty still
  requires stop and retained terminal resources.
- Event-only retry previously returned OK for accepted posts beyond the absolute
  deadline. It now retains acceptance, clears eventLost and reports timeout;
  failed posts keep their original error. Mediator callbacks also detect an
  observed duration exceeding notificationUs even when aggregate time remains.
  This is detection after return, not software preemption.
- The SDK arms its periodic timer before start returns. A timer callback could
  win the gate and strand STARTING after successful SDK start. A terminal start
  receipt now records the result without that acquisition. Deferred START-event
  evidence is adopted before the phase transition so failed-start cleanup still
  repairs missing STOP after an unarmed timer. An existing regression caught
  and verified this ordering during integration.
- Final independent review also found that accepted STOP/fence-arm effects could
  be lost on a subsequent gate failure. The serialized lifecycle owner now saves
  those flags immediately and retains cleanup status through a terminal receipt.
  A retry cannot duplicate an accepted STOP or rearm the existing fence.
- Pinned SDK period conversion multiplies uint32 milliseconds by 1000 before
  widening. Factory validation now accepts only 1..4294967ms, rejecting overflow
  before allocation or mutation. The exact upper accepted value is exercised
  through the actual SDK driver timer call.
- P4 esp_timer divides counter ticks by 16, rounding down. Board POR, stable-clock,
  assertion and reset-release minima now use timestamp upper bounds with the
  declared clockLagUs. Tests cover early tick wakes and exact tick boundaries
  at 1000Hz and 100Hz; no frame occurs during shared silence.
- Added native cached linkSnapshot so acquired core link/full RX/TX pause evidence
  is observable independently from accepted SDK fields. No I/O, clock refresh,
  hidden collector or new polling owner is added.
- Corrected the timing guide: the separate 100ms MAC mutex is used for address
  filters, not MDIO or the selected link-notification methods. The full table
  covers all 17 vtable entries, local callbacks, rejected entries, actual batch
  counts, notification order, partial effects, output/errors and conditional bounds.

The pause table proves all four portable RX/TX outcomes versus the narrower SDK
Boolean peer-symmetric capability. Forced-mode stale partner bits cannot enable
peer pause; application MAC enable policy remains separate. Unresolved negotiation
retains a valid unresolved observation without fabricated speed/duplex or DOWN.
Changed UP tuples publish DOWN/SPEED/DUPLEX/PAUSE/UP; unchanged complete tuples
emit nothing. Stop/start resynchronizes publication and SDK caches.

## Verification

Host: Windows/PowerShell, GNU 15.1.0, CMake 4.0.1/Ninja, Python 3.12.10. Actual SDK:
side-by-side C:/esp/v6.1/esp-idf through scripts/idf.cmd only; P4 GCC 15.2.0
(esp-15.2.0_20251204), SDK-environment CMake 4.0.3, Ninja 1.12.1, esptool 5.6.0.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable host | PASS |235/235 Release CTests, assertions active |
| Portable consumer | PASS | Separate tests-OFF build,2/2 CTests; no SDK/tooling dependency |
| Native pinned-driver host | PASS | 55/55 Release and 55/55 UBSan trap CTests; seven driver, 47 native at 1kHz and one readiness case at 100Hz |
| Actual P4 SDK image | PASS | Fresh real image 377248 bytes (0x5c1a0), all native/actual MAC symbols linked; default guard remains false |
| Research/tooling | PASS |103 Python tests;97 registers/52 procedures/35 features; version check |
| Source docs/reference/diff | PASS | Doxygen 289 pages plus two rejected negative fixtures; 29 vendor artifacts/27 extracts/1882 links in 238 Markdown files; diff check |
| Installed/exported package | NOT RUN | This stage rebuilds the build-tree consumer; export/package stages 68-71 |
| Remote CI | NOT RUN | First-execution prompt 71 rule |
| Physical timing/link | NOT RUN | No flash or fixture operation; declared workload/controller bounds unmeasured |
| Reusable installed teardown | BLOCKED | native_rx_quiescence; retain all installed resources until restart |

Commands (repository root):

```sh
cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p39-agent-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p39-agent-native -j2
ctest --test-dir build/p39-agent-native --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p39-native-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf -DCMAKE_C_FLAGS="-fsanitize=undefined -fsanitize-undefined-trap-on-error" -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/p39-native-ubsan -j2
ctest --test-dir build/p39-native-ubsan --output-on-failure --no-tests=error
cmake -S test/consumer -B build/p39-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF -DDP8382X_BUILD_PINNED_IDF_TESTS=OFF
cmake --build build/p39-consumer -j2
ctest --test-dir build/p39-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_references.py
python tools/verify_ledger.py
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

```bat
scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-p39 set-target esp32p4
scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-p39 build
```

The precommit image at C:/esp/build/dp8382x-p39/dp8382x_adapter_link.bin has
SHA256 `5f00a4ac84188d511ca66627850ce2e5d774caa9948372b91679321b8850ce40`.
Its embedded identity is entry 456b971 plus dirty tree. After the final production
edit, an incremental wrapper build recompiled Phy.cpp/header and relinked; source,
object and final-link timestamps and actual native/MAC symbols were checked.
The postcommit clean identity/image is refreshed separately for the final handoff.

New registered cases cover live_tuple_changes, incomplete_negotiation,
pause_projection, notification_partial_receipts, notification_late_retry,
notification_completion_contention, notification_late_event_retry,
board_tick_readiness (1000Hz and 100Hz), poll_period_bounds,
start_completion_contention and stop_completion_contention. Existing raw/typed,
flow, error, stop/start/delete and RX-retention regressions all remain enabled.
The final independent reviewer confirmed the stop/fence receipt correction and
reported no remaining confirmed defect. Unsupported physical wall-time and RX
teardown claims remain gated rather than relabeled as passing tests.

The host driver/glue use original SDK code and public headers, with instrumented
MAC and small platform fakes. These execute deterministic ordering/partial-effect
cases; they do not execute P4 controller waits or establish real scheduler races.
The independently linked P4 image supplies that integration layer without claiming
physical timing. Final clean image identity and synchronized commit are recorded
in the handoff, avoiding a self-referencing commit loop.

## Documentation and retained limits

Updated public Doxygen, native entry/notification/timing tables, architecture and
design decisions, feature map, example plans, test guides, README/CHANGELOG and
progress. API OP1-17/core declarations and variant/ledger facts are unchanged;
no new silicon feature, facade stub, SDK pin/range or version change is needed.
The fixture source, build wrapper and component manifests remain appropriate;
rebuild verifies the changed production adapter.

The native_wall_time qualification gate explicitly requires identified scheduler,
interrupt, logging, event/timer-service, controller and clock-lag bounds. Nominal
1000us/frame, 2500us MAC-stop and 10ms MAC-init loops are source components, not
wall-clock promises. No guaranteed completion is claimed for arbitrary starvation
or stopped owner polling. A late return preserves effects but cannot interrupt the
backend. Physical qualification, hidden MAC/HAL errors and native_rx_quiescence
remain separate limits; no mock or private API closes them.
