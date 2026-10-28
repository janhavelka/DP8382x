# DP8382x

General microcontroller library workspace for Texas Instruments DP8382x Ethernet
PHYs, starting with **DP83826IRHBR (DP83826I)**. The planned C++17 core is
framework-independent, with a native ESP-IDF adapter for the ESP32-P4 Ethernet MAC.

**Status: prompts 1 and 2 evidence audits complete; driver implementation not started.**
Official references and the 80-prompt implementation plan are available. The
[current source and tooling comparison](docs/library-comparison.md) records
the first two execution results. Driver code, build/package metadata, examples, CLI,
runners and hardware qualification are future work.

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
- [Device coverage](docs/variants.md): primary target and reference-only variants.
- [ESP32-P4 board context](docs/esp32p4-board.md): the starting hardware and link
  policy, kept separate from reusable driver defaults.
- [Verification](test/README.md): archive checks now and planned driver checks.

## Layout

```text
DP8382x/
  include/DP8382x/       future public interfaces
  src/                  future implementation
  examples/esp_idf/      future P4 networking and EtherCAT-owner examples
  test/                 verification guidance; future driver tests
  tools/                reference archive integrity checker
  docs/
    prompts/            80 implementation prompts and shared contracts
    reports/            planning audits; later implementation evidence
    reference/
      source/           unmodified TI PDFs/ZIPs and provenance.json
      text/             searchable text derived from archived PDFs
```

This follows the basic `Projects/` library layout. Build and package manifests
will be added with the implementation they describe. The family name is a scope
for development, not a claim that every DP8382x device is supported.

## Local verification

Python 3.10 or newer, standard library only:

```sh
python tools/verify_references.py
git diff --check
```

The first command checks archived payloads, SHA-256 hashes, ZIP integrity,
derived text and local Markdown links. It does not contact TI or test a PHY.

## Licensing

Original project material uses the [MIT license](LICENSE), following sibling
libraries. Downloaded TI documents retain their original copyrights and notices;
they are excluded from that license. Linked source code has its own per-file
license. See [third-party notices](docs/reference/THIRD_PARTY_NOTICES.md).
