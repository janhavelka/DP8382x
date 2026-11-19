# Prompt 42 · Bounded native MAC frame handoff

This stage began on synchronized `main` at
`20bcf9b33ca55cceacd9a899faa326b68c8cb3c0` on 2026-11-19. It extends
the existing owner, its pinned-driver host harness and its guides. It reuses
prompt 41's admission gate, prompt 37's retained lifetime and the existing
P4 board settings. No ledger procedure has this stage as implementation owner.
Separate prompt 40 review work in the checkout was outside the stage.

## Sources and decisions

| Source reopened | Decision |
| --- | --- |
| ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`, `components/esp_eth/include/esp_eth_driver.h`, `src/esp_eth.c` and `src/esp_eth_netif_glue.c` within that component | Install a single raw input path before start. The mediator transfers the received allocation to a registered input callback on every outcome. Netif attach would replace the input consumer. SDK transmit checks cached link and borrows caller storage through the MAC call. |
| Same SDK, `components/esp_eth/src/mac/esp_eth_mac_esp.c` functions `emac_esp32_rx_task` and `emac_esp32_transmit`, and `esp_eth_mac_esp_dma.c::emac_esp_dma_transmit_frame` | RX comes from the MAC task through the extended mediator entry. The P4 copies TX bytes into a finite descriptor chain, publishes descriptor ownership, and maps insufficient space to NO_MEM. Keep one sender with TX mutex off, no chunked cache writeback and log level at most INFO. No retry or wait for DMA completion. |
| FieldCore-node `c40ea8341edf8d665460b902cafefbe857a7f73f`, `src/i2c/I2cTask.cpp::submit`, `enqueueApplicationResult`, `src/devices/i2c/Ina228Module.cpp::takeResult`, `test/native/test_owner_i2c/test_i2c_task_fake.cpp` | Adapt bounded owner ingress, explicit overflow and separate result lifetime. The frame service copies bounded datagrams and preserves a generation. An I2C result slot supplies no MAC ownership evidence. |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`, `examples/common/ConsoleBuffer.h`, `test/console/test_console.cpp`, registered in `CMakeLists.txt` | Adopt whole-record overflow and bounded work per drain. A failed Ethernet submission drops one request. The console's partial-line continuation is specific to a byte stream. |
| SNLA344C §1/table 1, SNLA494 §§2.2/3.3 and SNLA495 §§2/3.1 in the [TI archive](../../reference/README.md) | These discuss slave-controller link and diagnostic behavior. C2000 ESC reset recipes and MII Fast Link Drop supply no P4 RMII timing or master-session guarantee. |

The available coding model handled two disjoint assignments for queue/test
implementation and native integration. An independent reviewer checked the
pinned sources, entry/exit races and final failure behavior. The lead owned
the public callback contract, pinned-driver integration tests, shared docs and
Git. Review caught a cached SDK DOWN that initially only updated a counter.
The shipped behavior withdraws the generation before another frame can submit.

## Executing evidence

Portable `frame_queue`, `frame_session` and `frame_load` cover fixed bounds,
copies, stale 64-bit tickets, overload and concurrent close/reopen. In the
real pinned driver, `native_application_frames` counts each actual callback
free, executes `esp_eth_transmit` against an instrumented MAC under the
admission gate, injects NO_MEM, cached DOWN and MAC errors, and resumes
captured RX after retirement failure. Separate raw and netif lifetime cases
exercise both consumers. The [owner guide](../../../examples/esp_idf/owner/README.md)
gives the P4 build command, limits and explicit single-frame opt-in.

Queue quiescence closes access to session data. A callback may still need to
free its original SDK allocation. The current SDK exposes no public fence
between a saved mediator and RX task entry. Installed retirement therefore
retains the original objects until restart. Resolving `native_rx_quiescence`
requires a source-proved lifetime fence. Physical traffic, peer capture,
power and cyclic timing were not exercised. First-set remote CI remains
assigned to prompt 71.

## Local verification

| Evidence on Windows | Result |
| --- | --- |
| Portable Release CMake build and `ctest --output-on-failure --no-tests=error` | 242/242 passed. The six owner and frame suites also passed with GNU UBSan. |
| Pinned ESP-IDF driver host build, CTest filter `^(native_\|pinned_driver_)` | 289/289 passed in Release and 289/289 with UBSan. The application frame test ran at 100, 333 and 1,000 Hz. The pinned source verifier checked 45 SDK files per configuration. |
| Separate tests-off portable consumer | 2/2 passed. Package publication was outside this stage. |
| Pinned P4 application build | Linked `dp8382x_p4_owner.bin`, 400,368 bytes, SHA-256 `ee6dc605113f2f5096cd5d245297e58d2affdbc8ae667f260b35d46c57ab9d97`. This precommit build identifies the entry SHA and dirty tree. Board and test-frame gates were closed. |
| Documentation and repository checks | 103 Python tests passed. `tools/check_doxygen.py` checked 291 generated pages. Reference hashes and links, ledger coverage, generated version and `git diff --check` passed. |
