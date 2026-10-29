# DP8382x

General microcontroller library workspace for Texas Instruments DP8382x Ethernet
PHYs, starting with **DP83826IRHBR (DP83826I)**. The planned C++17 core is
framework-independent, with a native ESP-IDF adapter for the ESP32-P4 Ethernet MAC.

**Status: prompts 1–10 complete. Portable values and version tooling are implemented; device and hardware behavior are not.**
Official references and the 80-prompt implementation plan are available. The
[current source and tooling comparison](docs/library-comparison.md) records
the first two execution results. The [primary silicon ledger](docs/register-map.md)
closes primary-part source coverage, including unresolved gates and offline validation.
The C++17 Status, configuration values, pure validators, selected register masks,
host tests, source-document checker and deterministic release-version tooling now
exist. Device code, SDK integration,
examples, CLI, runners and hardware qualification are future work.

Prompt 6 selects one shared C++17 chip implementation, a terminal batch MDIO
boundary and explicit native lifecycle/cache/collector ownership in the
[design decisions](docs/design-decisions.md). The pinned ESP-IDF baseline needs
the specified lifecycle corrections before native support can be claimed.

Prompt 7 freezes [foundational API contracts](docs/api-contracts.md): values,
Status/details, batch transport/budgets, identity admission, passive health,
snapshot validity and direct calls. Prompt 8 completes cooperative operations,
finite primary/cleanup budgets, retained results, lifetime tokens and explicit
owner-assisted teardown. Eight executable timing design probes complement the
77 research tests. Prompt 9 ships the passive subset in `include/DP8382x/`;
device and operation declarations remain contracts, not working stubs.

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

## Start here

- [Implementation promptset](docs/prompts/README.md): complete ordered prompts,
  shared contracts and copyable execution instructions for an AI agent.
- [Concrete conventions](docs/prompts/CONVENTIONS.md): selected Status/health and
  method names, source layout, readability and Doxygen rules fitted from peers.
- [Planning audits](docs/reports/README.md): actual sibling driver, FieldCore-node,
  tooling and PHY findings behind the plan.
- [Implementation progress](docs/implementation-progress.md): execution branch,
  completed evidence stages and remaining work.
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
- [Device coverage](docs/variants.md): primary target and reference-only variants.
- [ESP32-P4 board context](docs/esp32p4-board.md): the starting hardware and link
  policy, kept separate from reusable driver defaults.
- [Verification](test/README.md): portable host, version and archive checks,
  with driver checks planned.
- [Version tooling](scripts/README.md): release-version source, sync/check and
  firmware Git-identity boundary.
- [Core silicon behavior](docs/device-behavior.md), [independent vectors](docs/register-vectors.md)
  and [feature coverage](docs/feature-coverage.md): established facts, open gates
  and explicit implementation/source gates.

## Layout

```text
DP8382x/
  include/DP8382x/       public passive values, Status and register constants
  scripts/              release-version sync/check
  src/                  future device implementation
  examples/esp_idf/      future P4 networking and EtherCAT-owner examples
  test/                 host foundation and research tests; future driver tests
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
