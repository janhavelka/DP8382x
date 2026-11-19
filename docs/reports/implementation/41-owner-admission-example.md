# Prompt 41 · ESP32-P4 application admission and maintenance

Work started on 2026-11-19 from synchronized `main` at
`c2023d91c4984ee2b1c72b96d863a5bf87791f32`. The concurrent review of the
ordinary Ethernet example has its own changes. This stage owns the new
[owner application](../../../examples/esp_idf/owner/README.md), the native
admission callback, their tests and guides. It consumes the existing verified
link, configuration, operation and health contracts. The register ledger
requires no new silicon procedure.

## Evidence and ownership

| Source reopened | Application decision |
| --- | --- |
| FieldCore-node `c40ea8341edf8d665460b902cafefbe857a7f73f`, `src/devices/i2c/Ina228Module.h`, `.cpp`, and `test/native/test_owner_i2c/test_i2c_task_fake.cpp` | Its bind/start/poll/takeResult, refreshStatus/snapshot/copyStatus and mapLibraryStatus paths place execution with one owner and deliver a retained terminal result. Adapt validated start, bounded service and cached display. Ethernet admission also needs agreement with the running MAC and a current session generation. |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`, `examples/common/Tmp1x2Cli.cpp`, `test/jobs/test_jobs.cpp`, `tools/test_hil_tmp1x2_runner.py`; PCA9555 `0aec2212894910e0d872c3e511fbcd740837c5f7`, `examples/espidf_basic/main/main.cpp`, `examples/common/HealthDiag.h`, `tools/test_run_i2c_hil_parser.py` | Preserve outstanding tokens, failure details and the original receipt across restoration. Render cached facts. Sensor delays, I2C transactions and fixture results supply no Ethernet timing bounds. |
| ESP-IDF v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`, `components/esp_eth/include/esp_eth_driver.h` and `components/esp_eth/src/esp_eth.c`, with prompt 39's collector and prompt 40's board inputs | The SDK default poll period is 2,000 ms. The application explicitly requests 1,000 ms. One SDK timer collects. One application task checks the gated evidence, owns the generation and grants maintenance. |
| SNLA344C (Oct 2023), SNLA494 and SNLA495 (Feb 2026), [SDK and TI evidence](../../reference/library-sdk.md) | ESC and C2000 integration examples have different MAC roles. MII Fast Link Drop does not establish a deadline for a P4 RMII master. |

Two disjoint agents implemented the portable policy/tests and native example.
A third independently reviewed gate, timing and result semantics against the
sources. The available coding model was used for implementation and independent
review. The lead owned the shared adapter, integrated the pinned-driver test,
reviewed changes and ran verification. The [integration guide](../../ethercat-integration.md)
records the application decisions and physical scope.

## Acceptance

The owner checks the oldest time in the sample, permits up to 3 s of age,
targets 100 ms service and withdraws an active session after a 250 ms check
gap. A regressing clock, missed evidence or changed link generation withdraws
permission. The adapter checks core and published MAC evidence in a single
bounded callback. Healthy negotiation may continue across framework ticks.
The application reuses the framework start/stop path and retains installed
resources under the existing RX-lifetime restriction.

Portable Release tests cover profile rejection before I/O, policy and freshness
boundaries, stale generations, release, quiescence and result backpressure.
Pinned-driver `application_recovery` exercises admission, real framework
DOWN/UP, cancellation after the literal PCS reset write and a separate verified
restoration before readmission. `observation_cable_flap` and
`incomplete_negotiation` exercise contention, a still-published older UP and
repeated unresolved AN. All run with assertions enabled.

The local checks use the repository CMake Release builds, CTest, a separate
tests-off portable consumer, GNU undefined-behavior sanitizer, the pinned
SDK verification manifest, Doxygen, ledger, version, reference and Python
checks. The [example README](../../../examples/esp_idf/owner/README.md)
gives the real P4 link command. The build leaves board admission closed.
No physical trigger, link timing, traffic or scheduling measurement was run.
Raw frame integration belongs to prompt 42. First-set remote CI belongs to 71.

| Local evidence on Windows | Result |
| --- | --- |
| Portable Release CTest, including three owner suites | 239/239 passed. Owner suites also passed 3/3 with UBSan. |
| Pinned driver CTest, `-R '^native_'`, including 100, 333 and 1,000 Hz builds | 279/279 passed in Release and 279/279 with UBSan. Each build checked 45 actual SDK sources/headers. |
| Tests-off portable consumer | Both registered consumers passed. Packaging is unchanged. |
| Pinned P4 link | `dp8382x_p4_owner.bin`, 397,136 bytes, SHA-256 `976d11df7086d50a475fb93a87f485333960d74f902c864ef2ca9f797c05189e`. The precommit image records the entry SHA and dirty tree. |
| Source and documentation | 103 Python tests passed. Doxygen checked 291 pages. The reference, ledger, generated version and whitespace checks passed. |
| Physical, raw traffic and remote CI | NOT RUN. Board power remains unconfirmed. No physical link-loss, traffic, EtherCAT or cyclic timing result is claimed. |
