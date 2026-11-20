# Independent review of prompts 45 and 46

The review started on synchronized `main` at
`d0d018b2334187cc3b9fa12b561697505f7d5eda`. It checked the original
[link and lifecycle prompt](../../prompts/45-cli-link-lifecycle.md), the
[typed diagnostics prompt](../../prompts/46-cli-diagnostics-controls.md),
and the implementation diff from `110b6b0` through `d0d018b`.

Three read-only agents reviewed the code in parallel using the active model.
The processor reviewer traced deadlines, terminal records, and stalls. The
owner reviewer used the pinned SDK host target to test authorization and
partial effects. The feature reviewer checked typed settings, literal register
vectors, and registration against the coverage matrix. These reviews used
separate build directories. The lead checked the findings against the working
tree and integrated the fixes.

The sibling source check reopened TMP1x2's shared diagnostic/output jobs and
CMake registrations, INA228's native owner jobs and framed parser tests,
TCA9548A's cleanup handling and native CLI tests, and MB85RC's
`tools/hil_runner.py::classify` and restoration tests. The clean revisions
match the stage reports. The shared CLI keeps bounded per-tick work, strict
request/token correlation and separate restoration evidence. The fixes reuse
the existing DP83826I reset, link and event procedures and their register
evidence. [Prompt 45 sources](45-cli-link-lifecycle.md#source-audit),
[prompt 46 sources](46-cli-diagnostics-controls.md#current-source-audit)

## Findings and corrections

| Finding | Correction and executing evidence |
| --- | --- |
| A native poll refusal could keep the core slot running past the CLI deadline. The CLI also rejected cancellation while waiting to take an operation that was still running. | A transient refusal leaves the active token available for cancellation. At the exclusive deadline the CLI requests local cancellation without advancing the hardware and retains the result through output stalls. The record carries the outer timeout and the core status and effects. Actual pinned SDK START refusal and exact expiry are covered by `native_cli_blocked_poll`. The stalled writer, explicit cancel, and retry of an already frozen result are covered by `cli_processor_operation_edges`. |
| A successful core result whose feature fields could not be formatted appeared as a failure in the terminal record but as success in `owner result`. | The retained summary is assigned after field validation. The processor test checks the matching terminal outcome and preserved core status. |
| Passive runs could finish successfully after a clock regression detected while recording their last sample. Selftest accepted a successful reply without complete identity/frame evidence. | The completed run checks for a clock fault already detected during sampling. Selftest verifies the two-ID or single-PHYRCR batch before treating it as successful. `cli_processor_run_edges` checks both paths. `cli_selftest_transcript` exercises the real core evidence and partial second-ID failure. |
| Live typed reads could transfer with a missing or rejected identity. | The portable bridge and native gate require current accepted identity. They keep a current identity rejection separate from missing proof and permit a successful reprobe. `cli_control_transcript` and `native_cli_feature_admission` check zero frames on denial and successful acquisition after reprobe. |
| The native generic feature template accepted link configuration directly and bypassed the dedicated native check for extended forced half duplex. | The templates accept only the specific diagnostic and feature methods they serve. A compiler probe rejects the previous call through `startFeature`; the native host suites exercise the approved routes. |
| BIST narrowed a legal 64-bit observation to 32 bits. The guide also treated every `UNSUPPORTED` as an established silicon exclusion. | The CLI parses a positive uint64 interval and leaves activation-time overflow admission with the core. `cli_diagnostic_transcript` exercises an interval above UINT32_MAX and rejects a token above UINT64_MAX. The guide documents the existing status/detail distinction for source and board prerequisites. |

The [coverage matrix](../../feature-coverage.md) retains all 35 feature IDs,
52 procedures and 283 fields and cites the added regressions. The review
rechecked LED0 mode 8, reserved LED3 drive, odd-nibble polarity, WoL status
versus acknowledgement, and byte order through the existing literal tests.

## Local verification

Windows host with GNU 15.1.0, CMake 4.0.1, Ninja, Python 3.12.10 and
Doxygen 1.13.2. Pinned ESP-IDF v6.1 at
`fff9895c82d744c7237be8847347bdd1b07c6643` verified 45 SDK sources and
headers for the host build.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable host | PASS | 257 Release CTests including the compiled CLI and real-core transcripts. |
| Pinned SDK host | PASS | 368 Release CTests including 11 native CLI suites. |
| Focused UBSan | PASS | 22 portable CLI, reset and transport checks and 11 native CLI checks. |
| Tests-OFF consumer | PASS | 2 linked consumer CTests. |
| P4 firmware links | PASS | Basic and owner examples built through `scripts/idf.cmd`; images are 515,792 and 401,632 bytes. |
| Source and tools | PASS | Public Doxygen generated 293 pages and rejected both negative fixtures. Strict CLI and bridge Doxygen, 107 Python tests, local references, ledger, version and diff checks passed. |
| UART, physical PHY and remote CI | NOT RUN | Firmware console attachment is prompt 47. Physical qualification and first-execution remote CI have separate stages. |

The host builds used `cmake --build` in `build/p46-release`,
`build/p45-native`, `build/p43-ubsan`, `build/p44-native-ubsan` and
`build/audit43-44-consumer`, followed by CTest with `--output-on-failure`
and `--no-tests=error`. The P4 builds used the `examples/esp_idf/basic` and
`examples/esp_idf/owner` projects with their separate SDK build directories.
