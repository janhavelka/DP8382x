# Prompt 16 — hard reset, restart and external reconciliation

Date: 2026-11-08. Branch: `promptset/dp8382x-implementation-20261028`.
Base: `5e148fffde074b85fadc15d8ff66fe2c77111036`.
Clean, fetched, synchronized entry/prerequisite checkpoint:
`5f05f97a24b929ad5ded85eb08112a4514208bdf` (prompt15).
The final checked commit and verified upstream equality are in the handoff,
avoiding a self-referencing commit.

## Rules, ownership and completed behavior

Read repository/parent AGENTS, COMMON, CONVENTIONS, ENGINEERING, EVIDENCE,
FULL_COVERAGE, SOURCE_MAP, VERIFICATION and the remaining-prompts audit index.
Reopened prompt15's actual state machine and original-ledger reset facts.
Owned procedure IDs: `hard_reset`, `restart`, `external_reset`. Owned paths are
the existing driver header/source, reset and operation tests/registration,
portable owner example/consumer, source/feature records and affected guides.
No sibling, SDK, board GPIO/power, fixture or remote CI changes were made.

`startInitialize({HARD})` and `startInitialize({RESTART})` extend the existing
INITIALIZE phase switch, lifetime token, aggregate budget and retained result.
There is no reset/recover alias or second register procedure. Commands are
literal PHYRCR writes, reserved bits neutral zero. Completion reads the actual
register and waits for both PHYRCR command bits clear; an overlapping command
cannot become readiness. PCS retains its own BMCR completion rule.

Every kind now prechecks both BMCR and PHYRCR. This closes NONE restoration over
an unfinished earlier hard reset. An observed configuration reset also requires
fresh identity/straps before issuing the requested command; changed declared-mode
evidence cannot authorize a write. Successful reset refreshes identity/straps and
uses the same unconditional VOD write/stable verification path. The current
persistent owned scope is VOD only; future validated link/feature settings extend
that path. Raw maintenance values and temporary diagnostics are not desired profiles.

HARD invalidates initialized/configuration/identity knowledge and may resample
eligible straps. RESTART retains registers and initialized/identity knowledge,
but makes live event/link observations historical. Its later mandatory VOD
mutation and identity/verification failures retain their normal invalidation
rules. At all times the ordering suffix is application-declared; no address scan
or implicit rebind follows changed reachability.

Every attempted or untrustworthily reported HARD command creates a persistent
floor at post-callback upper time plus max(2000 us, polling interval). OP-6 admits
command, four clock lags, wait and first read strictly before the deadline, then
rechecks at actual submission. Floor expiry is not reset completion or a proved
internal release time. The controller lock is released before waiting, while
the application keeps every bus participant silent.

The floor survives failure, cancellation, take, abandon, external invalidation
and new operation admission. Public snapshots expose the floor without clock
access; timed reads can expire it before MDIO. Invalid post-command clocks or
unrepresentable mandatory floors latch timed work closed. Original transport
errors remain primary; otherwise floor overflow reports BOUND_ADMISSION.
Known quarantine still takes precedence over later raw-access admission and
requires owner repair. Teardown remains local; the owner carries floor and
controller responsibilities across end/rebind.

External reset and POR use distinct EXTERNAL_RESET/EXTERNAL_POWER_ON arguments
to the existing bus-silent invalidation route. EXTERNAL_POWER_ON was already
declared in prompt13; this stage completes its reset-specific reconciliation
documentation/tests. Notifications invalidate current knowledge and clear current
cleanup by owner attestation, while preserving frozen results and the floor.
They do not operate pins, establish readiness or perform implicit recovery.

## Fresh evidence and delegation

[Peer review](16-peer-source-review.md) reopens actual lifecycle declarations,
implementations, tests, examples/CLI, runners and rules at clean local revisions:

- TMP1x2 `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb`.
- PCA9555 `414e0f66b13082e974ec27a4a6e86985a65d83fd`.
- TCA9548A `1997d3f9446057348adbc6b2b6872dd18c041de3`.

Adopted validated replay and retained uncertainty, separate result/cleanup
ownership, and bus-silent end. Rejected sensor/EEPROM delays, ACK polling,
I2C recovery aliases and mux GPIO reset callbacks as PHY behavior.

The stronger `reset16_sources` reviewer (gpt-6-astra/high, selected under AGENTS)
independently reopened original PDF tables and current primary sources, then
reviewed the actual code. Current [TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
remains byte-identical SNLS647I, revised August2025: 3,366,322 bytes, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Original pages15,18,73 were rendered; pages17,48–51 and reset/strap definitions
were reopened. Table8-48 supplies the command/address distinction; table8-19
supplies PCS retention; §8.4.1 and strap tables retain the resampling ambiguity.

Current TI RTOS `main` remains `94bf4343004732983b0b8e1eec123d88ff70453b`:

| Inspected file | Git blob / finding |
| --- | --- |
| [dp83826.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) | `e0b64710350d764fe16ffd089fbdc5246e600039`; line349 still passes mask8000 as register address in completion polling. The core instead reads1F and tests command bits. |
| [dp83826_priv.h](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826_priv.h) | `37ab9b94d67687c569ac2e1e36b315fb90eb6749`; field names/macros compared with original tables. |

Per-file BSD-3-Clause notices were reread. No TI implementation was copied;
vendor payloads, hashes and original copyright notices are unchanged. A bounded
current TI product/support search did not resolve `hard_reset_straps`.

The reset-effects table in [device behavior](../../device-behavior.md#reset-domains-and-readiness)
records trigger, source terminology, waits/completion, retained settings,
identity/mode, strap exceptions and restoration ownership. Original timing
columns distinguish 25us MIN RESET_N pulse without debounce capacitors, 2ms MAX
reset-to-SMI and 50ms MAX POR-to-SMI readiness windows, and 1500ms NOM FLP.
The delayed-clock figure/footnote paths remain distinct. ENHANCED Strap1/6
retain POR values on reset; no BASIC POR-only rule is inferred. Q5 must establish
actual eligible-strap and internal-release behavior; physical evidence is NOT RUN.

`reset16_tests` used the current capable model for disjoint `test/reset.cpp` and
the authorized PHYRCR-precheck adjustments in `test/operations.cpp`.
`reset16_peers` used the current capable model for source review, then the assigned
portable example/consumer and read-only production review. Reviewers did not edit
Git/shared headers or siblings. The lead integrated/reviewed all contributions
and maintained source, architecture, progress and final documentation.

## Review findings, verification and limitations

Independent literal tests found an early published nextPollAtUs even though the
HARD floor correctly blocked frames; the published due time now respects the floor.
Independent production review found malformed successful command receipts could
omit every attempt flag after a physical write. Submitted semantic batches with
invalid reports now retain possible effects; HARD also retains its silence floor.
Well-formed zero-attempt BUSY/errors remain effect-free. Regression tests cover
both cases, command/VOD failures and other-PHY silence. The lead also tightened
identity reacceptance after an already-active configuration reset before mutation.

Windows PowerShell; GNU MinGW-w64 15.1.0 x86-64, CMake4.0.1, Python3.12.10,
Doxygen1.13.2. Strict C++17 Release builds enable conversion/sign warnings as
errors; registered negative probes prove executing checks remain active.

```sh
cmake -S . -B build/reset16-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/reset16-release
ctest --test-dir build/reset16-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/reset16-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/reset16-consumer
ctest --test-dir build/reset16-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/verify_references.py
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

| Evidence layer | Result |
| --- | --- |
| Portable host | PASS:47 registered Release CTests, including six reset suites (18,831 checks), existing operation suites (14,338 checks) and seven active-check negative probes. |
| Linked portable consumer | PASS: tests OFF; all four kinds (116 nominal frames), failed/cancelled HARD and shared owner floor through take/abandon/end. |
| Python research/tools | PASS:102 tests; structural and independent policy/source evidence only. |
| Ledger/reference | PASS:97 registers,283 fields,52 procedures,35 families; archived payloads/hashes and local links checked. |
| Doxygen | PASS:102 HTML pages and two rejected negative fixtures; operation/floor ownership documented. |
| Version/diff | PASS: unchanged generated version and clean diff. |
| Installed/exported package | NOT RUN: later package stages. |
| SDK link | NOT RUN: native adapter remains prompts37–39. |
| Remote CI | NOT RUN: reserved for first-set prompt71. |
| Physical | NOT RUN: Q5, actual part/address/straps, clock/supply/reset traces and fixture qualification remain open. |

Tests exercise every frame and release boundary of both reset kinds, accepted
write failures, exact waits, uint64 overflow, regressing/frozen clock, sticky
and overlapping commands, cancellation, zero budgets, partial restoration,
mode/strap observations, external-notification silence and two-instance exclusion.
NONE/PCS traces now include PHYRCR precheck (20/32 frames). No scope is claimed
for unimplemented desired link profiles or managed health.

Updated Doxygen, API/initialization/reset-effects/access/identity/variant guides,
vectors, source comparison, ledger/coverage, tests, README/CHANGELOG and progress.
Version and tooling commands remain unchanged; no package/version output changed.
Final maintainability review keeps one phase switch and shared bounded batches,
small explicit floor state and one existing restoration path. No generic engine,
per-PHY lock registry, native Clause45 route or hardware callback was added.
