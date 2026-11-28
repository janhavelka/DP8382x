# DP8382x

General microcontroller library workspace for Texas Instruments DP8382x Ethernet
PHYs, starting with **DP83826IRHBR (DP83826I)**. The C++17 core is
framework-independent, with a native ESP-IDF adapter for the ESP32-P4 Ethernet MAC.

The core and native adapter also accept declared **DP83826E**. Its original
revision-1 BASIC and ENHANCED IDs are shared with I. The board supplies the
ordering grade. The implemented procedures and source-gated features apply to
both declarations. DP83826AI and DP83826AE accept revision 4 for initialization,
link negotiation and observation, MII/RMII, clocks, fast link drop and recovery.
Both A grades also have typed diagnostics, IRQ and wake routes, LED and GPIO
profiles, power controls and the 27-write EEE recipe. The [coverage matrix](docs/feature-coverage.md#dp83826ai-and-dp83826ae) records the supported paths and open silicon gates.
[Device coverage](docs/variants.md)

The portable core can probe revision-0 DP83822I, H, IF and HF on a
board-declared copper interface. The returned identity retains the declared
suffix. After a successful probe, the owner can capture both strap latches
in one bounded MDIO batch and read CR2 to observe the active medium.
Copper startup requires a board declaration for LED_1 strap mode 1 or 4.
Boards can declare the expected boot interface separately from the intended
MAC wiring. Startup reports when the target still needs a software transition.
On initialized copper, `readDp83822Interface` samples the live MAC interface
and retains the FIFO flags consumed by the same read. On a strapped RGMII
board, `configureInterface` programs and verifies independent fixed 3.5 ns
RX and TX shifts.
[DP83822 boot declaration](docs/interface-clock.md#dp83822-boot-interface)
The portable and native paths support copper reset, 10/100 configuration,
link observation and interrupt service. The portable PHY supports MII, RMII
and RGMII. The ESP32-P4 adapter accepts MII and RMII. [Identity guide](docs/identity.md)

DP83825I accepts the documented revision-0 ID and requires an explicit RMII
leader or follower declaration. Initialization checks both clock straps, then
restores owned 10/100 copper settings. Its default mode needs no VOD recipe.
The exact-part feature paths include functional mode selection, EEE, power,
wake, LED0/LED2, interrupts, diagnostics and electrical controls. Software
readback and simulated tests are recorded separately from board measurements.
See [DP83825I coverage and limits](docs/feature-coverage.md#dp83825i).

**Status: prompts 1-35 complete for supported procedures; prompts 37-39 add native ESP-IDF lifecycle, SDK state arbitration and notification conformance. Prompt 40 adds a native P4 Ethernet/IP application with a default-closed board gate. Deep-power-down and full primary closure remain source-blocked; hardware qualification remains future work.**
The [P4 basic example](examples/esp_idf/basic/README.md) builds against the
pinned SDK, but its saved board netlist leaves PHY `3V3` without a demonstrated
supply source. Resolve that physical gate before flashing or interpreting link,
DHCP or reachability results.
The [hardware validation guide](docs/hardware-validation.md) defines the
observation, configuration/reset, link, diagnostic and opt-in control campaigns
and their physical gates.
The runner guide includes a two-iteration compiled example with independent
restoration receipts and offline replay. [Finite soak](docs/hil-runner.md#finite-soak)
The [P4 owner example](examples/esp_idf/owner/README.md) adds explicit session
admission on fresh 100/full evidence, bounded raw RX/TX queues and maintenance
after release. An SDK transmit reporting cached link-down requires verified
framework recovery and fresh admission. Test transmission is disabled by default.
The [native ESP-IDF guide](docs/esp-idf.md) covers explicit board readiness,
AN/forced configuration, public SDK reconciliation, link publication, owner-routed
raw/collector access and discovery, and bounded framework start/stop. The real pinned SDK driver has executing host tests
and the ESP32-P4 fixture links. Reusable installed teardown remains blocked by
`native_rx_quiescence`: resources stay alive until system restart. Flow policy uses
separately typed MAC/PHY calls and preserves partial effects. Acquired link evidence
and accepted SDK publication are separate cached views. Notification/timing
regressions include 100 Hz ticks; physical wall-time bounds remain unqualified.
The [native correction review](docs/reports/implementation/37-39-native-fixes-2026-11-18.md)
records all 17 auditor findings, lifecycle retry/error fixes and additional timing,
failure and shared-controller regressions. Installed resources remain retained.
The [second-round review](docs/reports/implementation/37-39-native-fixes-round2-2026-11-18.md)
adds recoverable transient link misses, continued hardware cleanup after event
failure or forbidden uninstall, and stricter admission/deadline evidence.
The [third-round review](docs/reports/implementation/37-39-native-fixes-round3-2026-11-18.md)
corrects immediate trust-loss handling, pre-GPIO admission recovery and P4
controller-busy quarantine, with separate deadline and poll-limit receipts.
[EEE negotiation and status](docs/eee.md) now provide explicit reset-bearing setup,
verified restoration and bounded retained observations; physical Q1 remains pending.

The DP83826I coverage matrix maps all 35 features and 52 procedures to their
command routes, host tests and physical evidence requirements. The source
gates identified in the primary integration review still limit which
procedures can execute. [Coverage matrix](docs/feature-coverage.md) ·
[Primary integration review](docs/reports/implementation/36-primary-feature-closure.md)

[Power policy and observations](docs/power.md) now share explicit software intent,
finite PLL/IEEE transitions and reset replay. Deep-power-down exit remains gated;
physical clock and power qualification remain unmeasured.
[Interrupt policy and deferred service](docs/interrupts.md) now provide all source
enables, selector-aware retained evidence, explicit software-test stop and shared
reset replay. Pin delivery and electrical qualification remain unmeasured.
Official references and the 80-prompt implementation plan are available. The
[current source and tooling comparison](docs/library-comparison.md) records
the first two execution results. The [primary silicon ledger](docs/register-map.md)
closes primary-part source coverage, including unresolved gates and offline validation.
The C++17 Status, configuration values, pure validators, selected register masks,
host tests, source-document checker and deterministic release-version tooling now
exist. The [direct-access guide](docs/direct-access.md) covers binding, safe raw
reads, atomic MMD read/write, stable-field RMW, the restricted LEDCR maintenance
subset and retained failure evidence. The [side-effect access guide](docs/side-effect-access.md)
covers the complete named catalog, safe dump limits, retained partial samples,
interrupt enables and WoL W0C acknowledgement. Linked portable examples exercise
direct/indirect transactions, explicit evidence acquisition and
[exact-address identity probing](docs/identity.md). Identity acceptance is separate
from initialized settings, link-up and physical qualification.
[Initialization](docs/initialization.md) now provides caller-driven NONE/PCS/HARD/RESTART,
part-specific startup verification, retained results, shared-bus hard-reset silence and explicit bounded restoration.
[Live and cached link observations](docs/link-observation.md) now provide bounded
current/history evidence, forced-mode qualification and negotiated pause.
[Typed event/counter acquisition](docs/event-counters.md) adds complete snapshots,
partial evidence, saturation/loss metadata and bus-silent host-history clearing.
[Passive health](docs/health.md) counts managed transactions and retains errors
without blocking explicit attempts or performing recovery.
[Explicit verification](docs/reconciliation.md) checks identity and stable owned
settings; restoration reuses initialization with retained independent results.
[Loopback sessions](docs/loopback.md) now provide all documented modes with
separate bounded starts/stops, retained temporary state and verified restoration.
[BIST sessions](docs/bist.md) provide generator/checker controls, bounded raw
captures and verified stop, with inconclusive outcomes where source evidence is missing.
The [portable example CLI](docs/cli.md) provides local and cached views, exact
identity and discovery, typed profile apply/verify, guarded register access,
explicit link/event observations, cooperative recovery, and typed diagnostics
and opt-in controls under application admission.
Live typed reads require current exact-part identity. An admitted operation
keeps its original deadline and retains its outcome through console stalls.
Both P4 applications and the real-core host fixture use the same processor.
The P4 console uses UART0 at 115200 bit/s with a separate bounded I/O task.
Explicit application stop opens maintenance and verified settings reconcile
through public SDK controls before start. Physical console and PHY qualification
remain open. [CLI and console guide](docs/cli.md), [host fixture](test/cli/README.md)
The [console runner](docs/hil-runner.md) verifies compiled-fixture and recorded
CLI sessions and can attach to an explicitly qualified running serial console.
It preserves raw transcripts with replayable JSON evidence.

Prompt 6 selects one shared C++17 chip implementation, a terminal batch MDIO
boundary and explicit native lifecycle/cache/collector ownership in the
[design decisions](docs/design-decisions.md). The selected native design uses public APIs on unmodified ESP-IDF v6.1;
its adapter has host and real SDK-link evidence. Source/lifetime limitations
remain in the design record and physical checks remain later work.

Prompt 7 freezes [foundational API contracts](docs/api-contracts.md): values,
Status/details, batch transport/budgets, identity admission, passive health,
snapshot validity and direct calls. Prompt 8 completes cooperative operations,
finite primary/cleanup budgets, retained results, lifetime tokens and explicit
owner-assisted teardown. Executable timing design probes complement the
ledger and source-policy tests. Prompt 9 ships the passive subset in `include/DP8382x/`;
later feature declarations remain contracts; stage 15 ships the initialization operation.

The [fresh prompts 4–5 audit](docs/reports/implementation/4-5-silicon-ledger-audit.md)
records independent PDF review, corrected split-field transition ordering and
stronger semantic field/ownership regressions.

The general library is intended to expose both **10 and 100 Mbit/s** operation.
The initial board firmware will use autonegotiation advertising **100 Mbit/s,
full duplex only**. That board policy belongs in the application configuration.

The implementation target is the **complete publicly documented feature set**
of each supported chip, including features unused by that first board. Runtime
opt-in diagnostics, EEE/WoL, PHY low-power, interrupts, clocks and pin controls
remain library scope. See [full-coverage requirements](docs/prompts/FULL_COVERAGE.md),
[public-source research](docs/reference/public-research.md) and the
[DP83826I register/feature inventory](docs/reference/dp83826i-feature-inventory.md).

The [current independent-audit dispositions](docs/reports/promptset-audit-dispositions.md)
check each historical finding against the present implementation and future
acceptance requirements. Active-profile application rejects an existing BMCR
power-down indication and never copies a later power-pin assertion into software
intent. Native reusable teardown remains gated on RX lifetime safety.

## Start here

- [Implementation promptset](docs/prompts/README.md): complete ordered prompts,
  shared contracts and copyable execution instructions for an AI agent.
- [Concrete conventions](docs/prompts/CONVENTIONS.md): selected Status/health and
  method names, source layout, readability and Doxygen rules fitted from peers.
- [Planning audits](docs/reports/README.md): actual sibling driver, FieldCore-node,
  tooling and PHY findings behind the plan.
- [Implementation progress](docs/implementation-progress.md): execution branch,
  completed evidence stages and remaining work.
- [Open questions](docs/open-questions.md) for product requirements and the
  first physical fixtures, with examples and tradeoffs.
- [Reference archive](docs/reference/README.md): downloaded TI datasheets,
  application notes, EVM guides and hardware collateral, with provenance.
- [Official source references](docs/reference/source-code.md): pinned TI and
  Espressif repositories for registers, PHY handling and SDK integration.
- [Extracted library data](docs/reference/library-evidence.md): concrete register,
  mask, sequence and timing tables, source defects and required verification rules.
- [Scope and architecture](docs/architecture.md): PHY component responsibilities
  and the boundary with the MAC, network stack and board firmware.
- [Ownership design](docs/design-decisions.md): complete portable/native entry
  matrix, transaction exclusion, SDK cache/lifetime and acceptance gates.
- [API contracts](docs/api-contracts.md): exact foundational declarations,
  failure/output rules and complete typed feature responsibilities.
- [Device coverage](docs/variants.md): declared I/E and AI/AE parts, implemented procedures and qualification status.
- [ESP32-P4 board context](docs/esp32p4-board.md): the starting hardware and link
  policy, kept separate from reusable driver defaults.
- [Verification](test/README.md) covers portable driver tests, the pinned SDK host build,
  CLI and runner regressions, the build-tree consumer, and source checks.
- [Version tooling](scripts/README.md): release-version source, sync/check and
  firmware Git-identity boundary.
- [Core silicon behavior](docs/device-behavior.md), [independent vectors](docs/register-vectors.md)
  and [feature coverage](docs/feature-coverage.md): established facts, open gates
  and explicit implementation/source gates.

## Layout

```text
DP8382x/
  include/DP8382x/       public driver, passive values, Status and register constants
  scripts/              release-version sync/check
  src/                  portable bounded direct-access implementation
  examples/portable/    compiling application-owned direct-read example
  examples/esp_idf/      future P4 networking and EtherCAT-owner examples
  test/                 host driver/foundation, consumer and research tests
  tools/                source-document, reference and ledger checks
  docs/
    prompts/            80 implementation prompts and shared contracts
    reports/            planning audits; later implementation evidence
    registers/          machine-readable ledger and documented schema
    reference/
      source/           unmodified TI PDFs/ZIPs and provenance.json
      text/             searchable text derived from archived PDFs
```

This follows the basic `Projects/` library layout. Package manifests arrive
with their supported build paths. The family name is a scope
for development, not a claim that every DP8382x device is supported.

## Local verification

Python 3.10 or newer, standard library only:

```sh
python tools/verify_references.py
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
git diff --check
```

The first command checks archived payloads, SHA-256 hashes, ZIP integrity,
derived text and local Markdown links. Ledger checks validate research structure,
coverage and independent literal values. These commands do not contact TI,
execute a driver or test a physical PHY.

After editing `library.json`, run `python scripts/generate_version.py sync` and
commit its changed `Version.h`; use `python scripts/generate_version.py check`
to verify it without writes. Ordinary CMake consumers use the committed header
without Python or Git. The [tool guide](scripts/README.md) defines supported
version syntax and failure behavior.

The portable target needs CMake 3.16 or newer and a C++17 compiler. These
verification commands use CTest 3.26 or newer so zero registered tests fail.
The build-tree consumer needs no Python, Git, Doxygen, SDK or sibling sources:

```sh
cmake -S . -B build/host -DDP8382X_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
cmake -S test/consumer -B build/consumer
cmake --build build/consumer
ctest --test-dir build/consumer --output-on-failure --no-tests=error
```

If Doxygen is installed, `python tools/check_doxygen.py` checks generated public
pages, member comments and links, and exercises two rejected documentation
fixtures. Its output is temporary. See [verification](test/README.md).

## Licensing

Original project material uses the [MIT license](LICENSE), following sibling
libraries. Downloaded TI documents retain their original copyrights and notices;
they are excluded from that license. Linked source code has its own per-file
license. See [third-party notices](docs/reference/THIRD_PARTY_NOTICES.md).

[Negotiated configuration](docs/negotiation.md) uses the existing caller-driven
operation slot. Generic and application 100/full examples compile and execute
against the portable core; AN application, completion and link remain distinct.

[Forced and coupled routing](docs/forced-settings.md) supports all documented
10/100 half/full settings, robust/manual crossover, coupled mirror and extension.
Host tests do not establish partner compatibility or electrical qualification.

The [independent prompts 13-18 audit](docs/reports/implementation/13-18-independent-audit.md)
corrects failure/effect reporting and extends next-page boundary coverage.

[Manual cable capture](docs/cable.md) provides the exact SNLA428 six-run recipe,
typed raw A/B controls, per-word evidence and separate HARD restoration. The
source-implied distance helper is not physically qualified. Explicit autorun policy
and bounded single-run raw acquisition share the same owner and restoration path.

[Forced TX LPI](docs/lpi.md) provides bounded opt-in entry/exit with retained
uncertainty and application-owned MAC coordination. Q1/Q2 physical qualification
remains NOT RUN; readback does not prove partner readiness.

[Wake-on-LAN](docs/wake-on-lan.md) adds exact MAC/Secure-ON controls, CRC and
pulse/level indication, retained status and explicit acknowledgement, with bounded
configuration and shared restoration. PHY indication is not proof of host wake.

[LED/GPIO profiles](docs/led-gpio.md) add all admitted sources, force/polarity,
shared blink/stretch and pin routing with conflict validation and verified replay.
Reserved or contradictory routes remain gated; physical Q12 is NOT RUN.

[Clock and interface profiles](docs/interface-clock.md) add explicit isolation,
MII/RMII compatibility, reference direction and repeater-side configuration.
Application-owned clocks/wiring and per-PHY results remain separate from working
MAC or two-PHY qualification.

[Quality controls and raw MSE](docs/quality.md) add bounded TX10/TX100 and MII
Fast Link Drop profiles, retained collector evidence and shared reset restoration.

[Electrical controls](docs/electrical.md) add persistent analog/slew tuning, finite
compliance and collision sessions, and conditional calibration recovery.
Source-gated branches reject before I/O; electrical qualification remains NOT RUN.
