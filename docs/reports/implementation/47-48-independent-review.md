# Independent review of prompts 47 and 48

The review started on synchronized `main` at `683270be5755711d3f49613320b0e3979c0a153e`.
It read both prompts and checked their changes from `12085ea` against the
current producers, application owners, registered tests and runner. Three
agents reviewed native ownership and coverage, console loss and protocol
timing, and fixture process failure handling in parallel. They used the active
model for these state and timing reviews. The lead verified the findings and
owned the runner contract, documentation and Git integration.

The source comparison used clean checkouts of TMP1x2 at `afd50889312f`,
INA228 at `8e439d6c99ec`, TCA9548A at `4a244adabf38`, and 24Cxx at
`1bd9f969d1df`. The [native stage report](47-cli-native-integration.md)
records their console producers, admission tests and workflow definitions.
The [runner stage report](48-runner-protocol-artifacts.md) records the exact
framing, fixture and artifact routines that informed strict closure, separate
host provenance and exclusive reservation.

## Findings and corrections

- The basic application's STOP path could turn a zero caller deadline into a
  fresh teardown budget. START and STOP now reject zero or expired deadlines
  before touching native or application state. The native integration test
  calls the production route and verifies that traffic, SDK calls and stop
  accounting remain unchanged.
- The ESP-IDF event queue could fill after the UART pump sampled its occupancy
  and before it received events. A lost framing error let a shortened register
  command reach dispatch. The pump now drains a bounded event prefix and uses
  the driver's separate ISR error notification for framing, parity and FIFO
  overflow. It explicitly enables the framing interrupt, absent from the
  pinned driver's default mask. The decision follows
  `components/esp_driver_uart/src/uart.c` and both P4 revisions of
  `soc/uart_reg.h` in the installed SDK. The regression drives the production
  pump through dropped framing, break and buffer-full events, recovery and
  an initialization failure.
- Offline replay could pass a malformed trailing record, an admission reporting
  cleanup failure, contradictory cached health, or a device clock that returned
  to valid within one boot. It now rejects those receipts and deadlines attached
  to cached queries. Older schema-1 identities can omit the paired console and
  baud fields. An expectation for either field still needs the observed value.
- Fixture worker startup failures could leak a child or hide the original
  error. A late wake could also turn an expired closure into success. Teardown
  now handles started and unstarted workers, retains the bounded stdout prefix
  and records an unconfirmed reap separately. A late terminal after the last
  reply fails correlation during closure. Interrupting evidence reservation
  releases the new placeholder prior to transcript creation.

The [feature matrix](../../feature-coverage.md) resolves all 35 feature IDs,
52 procedure IDs and 283 register fields to their owners. The audited CLI
routes and host test names match the shared command tables and CMake
registration. The README already describes shared CLI use and offline evidence.
The CLI, runner and example guides and the changelog record the changed behavior.

## Verification

Windows, GNU 15.1.0, Python 3.12.10, and ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`.

| Layer | Result | Evidence |
| --- | --- | --- |
| Portable host | PASS | 262 Release CTests from `build/p46-release`, including `cli_hil_runner` with the compiled producer. |
| Native host | PASS | 372 Release CTests from `build/p45-native`, including the new UART regression and the basic and raw application integration cases. |
| Focused UBSan | PASS | 20 portable CLI and 15 native console and CLI CTests. |
| Portable consumer | PASS | Two separately linked tests-OFF consumer CTests. |
| Offline runner | PASS | 163 Python tests discovered, with the fixture-dependent test executed via CTest. The command-line help, dry-run, compiled fixture, transcript replay and artifact verification also ran directly. |
| P4 links | PASS | `scripts/idf.cmd` built `dp8382x_p4_ethernet.bin` and `dp8382x_p4_owner.bin` for `esp32p4`. Both ELF files contain the shared processor and place the callback in IRAM and its latch in DRAM. Both retain the default-closed board gate. |
| Documentation and tooling | PASS | 293 public Doxygen pages, two rejected negative fixtures and strict scoped CLI/platform Doxygen. Reference, ledger, version and diff checks passed. |
| Physical console and PHY, remote CI | NOT RUN | Physical behavior remains gated. The offline runner produces host protocol evidence. Serial work follows in prompt 49, and remote CI belongs to prompt 71. |

The reviewed offline unit was committed and synchronized at `28dc14a`.
