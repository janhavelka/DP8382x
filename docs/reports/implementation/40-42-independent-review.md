# Independent review of the ESP32-P4 examples

Reviewed prompts 40, 41 and 42 on Windows against `main` at
`9c04541331c9e516ce6801228adbb8396b74ce29`, including the outstanding
prompt 40 working-tree corrections. Three parallel reviewers took the basic
example lifecycle, application admission and raw frame ownership respectively.
Each used the available coding model to inspect the source and exercise its
failure paths. The lead checked their findings against the code, the actual
SDK and the final diff, integrated the MAC readback and ran the combined suites.

## Findings and regressions

| Area | Finding, correction and executing evidence |
| --- | --- |
| Basic example stack and results | The compiled operation-delivery function alone used 3,680 bytes with a configured 3,584-byte main stack. The last delivered result now lives in static application storage. The default stack is 16 KiB. The final receipt includes the full token, possible write and cleanup obligation. `example_operation_busy_retry` and `example_operation_result_owed` exercise contention and undelivered results. |
| Startup and netif attachment | The real glue ignores both its EMAC address read and netif address assignment results. Read the EMAC before attachment, compare the attached netif address, and preserve the failing stage or mismatch before PHY configuration and DHCP start. The three address-error cases require zero PHY frames, a retained installed driver and no framework start. |
| Ordinary Ethernet negotiation | An unresolved healthy link during startup could end the demonstration. The basic example now enables the adapter's wait-for-negotiation policy. `example_negotiating_start` runs the framework from pending AN to resolved UP. |
| Event and stop handling | The last allowed event retry could deliver the event just after its deadline and still end the session. Check the receipt on the next service turn. An unexpected STOP during the final interval or between owner stop and restart now ends service. Tests exercise last-retry delivery, permanent event loss, delayed expected STOP and an additional STOP before restart. |
| Lifecycle reporting and cleanup | Integrated and rechecked the existing corrections for stop obligations, bounded BUSY polling and result delivery, pre-install dependency order, mutex quarantine, typed first errors, cleanup allowance and retention of the full operational path in a gate-closed image. The six start/stop failure injections now check exact primary and cleanup causes and the retained phase. Reset release failure after successful assertion leaves management closed. Freshness also checks both ends of the acquisition interval. |
| Session continuity | A transmit can discover a cached SDK DOWN while the adapter still holds a published UP. A later identical PHY observation skips republication. The owner now latches the contradiction through release and later PHY UP. The pinned regression exercises failed readmission, stop, verified restoration, restart and successful submit with a new generation. The final transmit timestamp also participates in clock-regression detection without extending the service-gap allowance. |
| Raw frame bounds | A malformed descriptor could pass a null handle or excessive length to the SDK. The final boundary checks the handle and 14–1,518 byte range. Tests cover lengths 0, 13 and 1,519 and a null handle with an active session. All four retain the prior TX receipt and make zero SDK transmit calls. |
| Host verification | The optimized example harness could discard references to the SDK netif defaults, hiding a sanitizer link error. The fake netif now defines its host-side defaults using the pinned header and checks the selected Ethernet configuration. Firmware still links the actual ESP-IDF definitions. |

## Scope and sources

The source of the MAC check is ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`,
`components/esp_eth/src/esp_eth_netif_glue.c::esp_eth_post_attach` and
`components/esp_netif/lwip/esp_netif_lwip.c::esp_netif_get_mac`.
Session tests run `components/esp_eth/src/esp_eth.c::esp_eth_transmit`
through the same native admission gate as the example. Reusable installed
teardown still requires a public fence for RX paused before callback entry.
The host tests exercise late RX against retained storage.

Rechecked the board source at `ee16c0e`, FieldCore-node's
`Ina228Module.cpp` and `I2cTask.cpp` at `c40ea834`, the TMP1x2 native
example and console tests at `afd5088` and INA228's native example at
`8e439d6`. The source decisions and exact paths are in the
[board audit](40-board-source-audit.md), [admission report](41-owner-admission-example.md)
and [raw frame report](42-owner-frame-boundary.md). Application service uses
one owner, one framework link poller and the existing board mapping. No PHY
register or TI timing assumption changed. The basic example uses DHCP and
documents a separately requested, peer-initiated local reachability check.

## Verification

| Layer | Result |
| --- | --- |
| Portable Release and consumer | 242/242 host CTests and 2/2 tests-off consumer CTests passed. All six portable owner/frame cases also passed with GNU UBSan. |
| Pinned-driver host | 338/338 Release and 338/338 UBSan CTests, including 49 basic-example cases and native owner/frame cases at 100, 333 and 1,000 Hz. The verifier checked 45 SDK files per tick configuration. |
| P4 link | Both examples linked through `scripts/idf.cmd` with ESP-IDF v6.1. ELF inspection confirmed the basic image contains the EMAC, driver, glue and owner, and the raw image contains the callback and SDK TX path. Both board gates and the raw traffic option were closed. |
| Repository checks | 103 Python tests passed. Reference hashes, local links, the 97-register ledger, generated version and Git whitespace checks passed. Doxygen checked 291 HTML pages and rejected both negative fixtures. |
| Physical, package publication and remote CI | NOT RUN. PHY supply connectivity and actual stack high-water, link, DHCP, frame delivery and timing require an authorized fixture. First-set remote CI belongs to prompt 71. |

The precommit firmware identified `9c04541` with a dirty working tree.
The basic image was 513,824 bytes with SHA-256
`0aca53efefcedfc26d3c9096ba418ef614b3b11722be16ce1a03bc0c5d78ff81`.
The raw owner image was 400,528 bytes with SHA-256
`297dc5ef948a52d4216b878f0567afcf6925ded7151e0e2e95f356a96e507b44`.
