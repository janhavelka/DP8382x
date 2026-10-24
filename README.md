# DP8382x

Firmware library workspace for Texas Instruments DP8382x Ethernet PHYs, starting
with **DP83826IRHBR (DP83826I)** and integration with the ESP32-P4 Ethernet MAC.

**Status: documentation and repository foundation.** The official references,
scope and directory structure are ready. Driver implementation, build/package
metadata, examples and hardware qualification are future work.

The general library is intended to expose both **10 and 100 Mbit/s** operation.
The initial board firmware will use autonegotiation advertising **100 Mbit/s,
full duplex only**. That board policy belongs in the application configuration.

## Start here

- [Reference archive](docs/reference/README.md): downloaded TI datasheets,
  application notes, EVM guides and hardware collateral, with provenance.
- [Official source references](docs/reference/source-code.md): pinned TI and
  Espressif repositories for registers, PHY handling and SDK integration.
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
  examples/esp_idf/      future ESP32-P4 integration and diagnostic example
  test/                 verification guidance; future driver tests
  tools/                reference archive integrity checker
  docs/
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
