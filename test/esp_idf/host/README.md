# Pinned driver host tests

The optional target compiles the supplied, unmodified `esp_eth.c` and
`esp_eth_netif_glue.c`, the native adapter and the portable core. SDK headers
come from the actual pinned checkout. No Ethernet, timer, event, heap, logging
or netif public header is replaced.

```powershell
cmake -S . -B build/pinned-idf -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/pinned-idf
ctest --test-dir build/pinned-idf --output-on-failure --no-tests=error
```

`test/esp_idf` is also a standalone CMake entry for an independent build.
GNU or Clang is required for the unmodified SDK C17 source. Consumers leave
`DP8382X_BUILD_PINNED_IDF_TESTS` off and need no SDK or Python.

`verify_sdk.py` checks the exact SDK commit, asks the compiler for every
compiled source's header dependencies for 1000Hz, 100Hz and 333Hz configurations,
checks every selected SDK file against
the pinned Git blob and compares the complete closure with `sdk-files.json`.
Only Windows CRLF expansion is normalized. Configuration and every build
run the check. A missing, altered or unrecorded SDK file fails the build.

The platform directory supplies the explicit host configuration, the small
FreeRTOS scheduling/semaphore boundary, Windows endian definitions and the
host's missing `__containerof` macro. The host configuration selects OPENETH
only to expose the real public MAC vtable; no OpenETH controller executes.
Real P4 SoC headers remain in the dependency closure. This configuration is
separate from the ESP32-P4 firmware fixture.

Fakes implement the actual public timer/event/netif/allocator functions.
Timers run only when explicitly advanced; an unarmed stop returns
`ESP_ERR_INVALID_STATE`. One-shot fences dispatch after task delay, and can
be held to test finite deadlines without rearming. Events retain the actual
driver handle payload and bounded wait. The real netif glue installs its
real input callback into the real driver; only the receiving network stack
is fake. Allocation wrappers record driver/glue lifetime without editing SDK
source. `Host::reset` models system restart, not reusable production teardown.

The instrumented MAC can pause after loading its mediator and before entering
the application callback, matching the lifetime window in the pinned MAC
source. Raw and real-glue tests keep the driver, MAC, PHY, callback context,
glue and netif alive through stop/retire and resumed RX while the application
RX count was zero. The native adapter continues to reject destructive
teardown under `native_rx_quiescence`.

These tests establish driver order, callback admission, raw routing,
notification behavior, failure journals and the modeled lifetime window.
They do not establish real timer concurrency, MAC hardware waits, RX task
scheduling, FIFO flush completion, physical PHY behavior or reusable teardown.
The real SDK image link and physical qualification remain separate evidence.

Prompt 38 extends this target with AN-to-forced 10/100 half/full lifecycle and
SDK getter-cache scenarios, explicit stopped reconciliation, raw BMCR accepted
and uncertain effects, direct framework-start rejection for both cached AN
states, and supported flow-control calls. Flow-control uses a Boolean MAC
argument and a separate 32-bit PHY argument; the defective SDK flow-control
ioctl is never given a small Boolean. MAC acceptance followed by PHY failure
remains visible and requires a later explicit successful owner call.

The semaphore fake records each acquire/release, can fail a selected controller
boundary, and exposes held state to the test harness. MAC frame callbacks try
same-instance reentry, a second owner and raw ioctls at all four MMD boundaries;
the five-frame LED update is tested similarly. Failure cases cover each frame,
failed acquisition, failed release, quarantine and collector partial evidence.
These are deterministic boundary tests, not a claim about FreeRTOS scheduling.
Prompt 38 registered 43 cases: the original 24 and 19 configuration/arbitration
regressions. The latter include shared silence during hard reset, preserved MAC
receipts after gate contention, a foreign flow-control effect during the PHY
callback window, stale partner pause bits in forced mode, and a queued link poll
during the stop fence. The foreign-flow case invokes the correctly typed public
MAC and PHY callbacks separately, without executing the defective ioctl cast.
Failed instance-gate acquisition after a direct framework start retains both the
SDK FSM change and any already posted START event for the owner stop cleanup.

A separate GNU undefined-behavior trap build runs the same registered cases:

```powershell
cmake -S test/esp_idf -B build/p38-native-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf "-DCMAKE_C_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error" "-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/p38-native-ubsan
ctest --test-dir build/p38-native-ubsan --output-on-failure --no-tests=error
```

Prompt 39 registers 55 cases in total. Eleven additional native cases exercise
live speed, duplex and peer-pause changes; unchanged tuples and stop/start
resynchronization; unresolved negotiation; full portable RX/TX pause evidence
against the narrower SDK Boolean; accepted notification prefixes; per-callback
and aggregate deadlines; generic uncertain publication versus attested P4 lost
events; and retained publication/start/stop receipts after callback gate
contention, including exactly one accepted STOP event and fence arm after retry.
The maximum accepted link poll period is passed through the actual SDK start
implementation and checked at the timer boundary.

The readiness case also runs against separately compiled copies of the same
SDK, adapter and fake sources at 100 Hz. The FreeRTOS boundary derives tick
conversion from that selected rate and models the earliest tick-edge wake:
a one-tick delay started just before the boundary can last one microsecond.
Both an early wake and an exactly aligned release retain the full 2 ms PHY
readiness floor plus the one-microsecond floor-clock uncertainty before bus
admission opens. These tests model quantization, not arbitrary preemption or a
physical timing measurement. Both configurations verify the same 45-file real
SDK dependency closure; the pinned manifest is unchanged.

The independent prompts 37-39 audit brings the total to 63 cases. New cases prevent
board preparation from replacing another PHY's initialization reservation, keep
failed board readiness closed after adapter deletion, and reject other owners
throughout GPIO callbacks. Discovery now tests the production core accounting,
partial receipts, clock faults and ordinary-output preservation. Control and
flow-policy callbacks retain accepted effects when their individual bound is
exceeded. Positive controller waits return MDIO_TIMEOUT; zero-tick contention
returns BUSY at both 1000Hz and 100Hz. The timer fake now reports a dispatched
callback as active until it returns, matching the pinned public timer behavior.

The [2026-11-18 corrections](../../../docs/reports/implementation/37-39-native-fixes-2026-11-18.md)
bring the registered total to 144: 68 native cases at each of 1000Hz and 100Hz,
seven actual SDK driver cases, and a 333Hz rational-wait case. All suites retain
assertions in Release. They add failed/cancelled initialization/configuration,
retryable retirement, independent failed-start cleanup, discarded-timer admission,
first/latest native cleanup domains, DOWN-before-STOP repair, lost observations,
P4 no-command busy rejection, full pause resolution and dependent callback bounds.

Shared-controller tests now use a non-installed address-1 participant on the
same actual MAC object and SharedBus as the installed owner. Every peer obeys
logical silence/quarantine and the whole-batch mutex; no second P4 MAC is installed.
MAC installation is tested under exclusive logical silence with no mutex over its
waits. Fakes serialize callbacks on one ESP_TIMER_TASK, so a live timer callback
cannot be bypassed by a fence fired recursively from an injected task delay.
