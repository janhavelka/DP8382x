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
compiled source's header dependencies, checks every selected SDK file against
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
