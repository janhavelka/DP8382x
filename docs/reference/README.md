# Official reference archive

Collected from Texas Instruments on **2026-10-24**: **25 PDFs and two ZIPs**,
covering the primary DP83826I and documented DP83822, DP83825I and DP83826A
relatives. All vendor payloads are unmodified. This is a technical reference
collection for the planned PHY component, not a statement of implemented support.

[Provenance](source/provenance.json) records the requested and resolved official
URL, document revision/date, retrieval timestamp, byte count and SHA-256 for each
payload. Dates below come from the PDF itself; TI product-page dates sometimes
differ. The ZIPs retain their original internal notices and member names.

The `text/` directory contains searchable UTF-8 PDF extracts, with hashes in the
manifest. Extraction can alter table layout and symbols: inspect the original
PDF when interpreting a register definition, bit field or timing requirement.
Unmapped NUL glyphs are replaced with the Unicode replacement character so
ordinary text-search tools can read the extracts; the PDFs remain unchanged.
See [third-party notices](THIRD_PARTY_NOTICES.md) for licensing boundaries.

The 2026-10-26 follow-up adds a [full DP83826I feature/register inventory](dp83826i-feature-inventory.md)
and [expanded public-source research](public-research.md), including additional
TI GitHub/SDK and primary upstream comparisons. These derived planning guides
do not change the archived payloads or establish implemented chip support.

## Reading order

1. DP83826I/E datasheet and DP83826 troubleshooting guide: identity, modes,
   clocks/reset, standard and extended registers, 10/100 operation.
2. MDIO configuration guide and the [official source references](source-code.md).
3. DP83826EVM guide, schematic/checklist packages, and compliance/debug guide
   when bringing up hardware or testing diagnostics.
4. Feature-specific notes for the functionality being implemented. Keep
   EtherCAT ESC/C2000 examples distinct from an ESP32-P4 EtherCAT master.
5. Other family datasheets and the 26-to-26A rollover note before extending support.

## Datasheets

| Local document | Revision / date | Applicability and use | Official download |
| --- | --- | --- | --- |
| [DP83822 Robust, Low Power 10/100 Mbps Ethernet Physical Layer Transceiver](source/TI-DP83822-SNLS505H.pdf) | SNLS505H; July 2026 | DP83822 family. Family comparison and register map | [TI](https://www.ti.com/lit/gpn/DP83822I) |
| [DP83825I Low-Power 10/100-Mbps Ethernet Physical Layer Transceiver](source/TI-DP83825I-SNLS638C.pdf) | SNLS638C; April 2026 | DP83825I. Family comparison and register map | [TI](https://www.ti.com/lit/gpn/DP83825I) |
| [DP83826Ax Low-Latency, Low-Power, 10/100 Mbps, Industrial Ethernet PHY](source/TI-DP83826AI-AE-SNLS783A.pdf) | SNLS783A; October 2025 | DP83826AI / DP83826AE. A-generation comparison; not assumed software-identical | [TI](https://www.ti.com/lit/ds/symlink/dp83826ai.pdf) |
| [DP83826 Deterministic, Low-Latency, Low-Power, 10/100 Mbps, Industrial Ethernet PHY](source/TI-DP83826I-E-SNLS647I.pdf) | SNLS647I; August 2025 | DP83826I / DP83826E. Primary device specification and register map | [TI](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) |

## Bring-up and hardware references

| Local document | Revision / date | Applicability and use | Official download |
| --- | --- | --- | --- |
| [DP83822 Evaluation Module User Guide](source/TI-DP83822EVM-SNLU179.pdf) | SNLU179; August 2016 | DP83822 family. Family reference hardware | [TI](https://www.ti.com/lit/pdf/SNLU179) |
| [DP83825 Evaluation Module](source/TI-DP83825EVM-SNLU239A.pdf) | SNLU239A; December 2023 | DP83825I. Family reference hardware and RMII setup | [TI](https://www.ti.com/lit/pdf/SNLU239) |
| [DP83826AEVM User Guide](source/TI-DP83826AEVM-SNLU357.pdf) | SNLU357; April 2025 | DP83826AI / DP83826AE. A-generation reference hardware | [TI](https://www.ti.com/lit/pdf/SNLU357) |
| [DP83826EVM User Guide](source/TI-DP83826EVM-SNLU262A.pdf) | SNLU262A; June 2023 | DP83826EVM. Reference hardware and MDIO bring-up | [TI](https://www.ti.com/lit/pdf/snlu262) |
| [DP83822 Troubleshooting Guide](source/TI-DP83822-Troubleshooting-SNLA437A.pdf) | SNLA437A; October 2025 | DP83822 family. Family expansion bring-up reference | [TI](https://www.ti.com/lit/pdf/SNLA437) |
| [DP83825 Troubleshooting Guide](source/TI-DP83825-Troubleshooting-SDAA081.pdf) | SDAA081; September 2025 | DP83825I. Family expansion bring-up reference | [TI](https://www.ti.com/lit/pdf/SDAA081) |
| [DP83826 Troubleshooting Guide](source/TI-DP83826-Troubleshooting-SNLA423A.pdf) | SNLA423A; June 2025 | DP83826E / DP83826I. Link, clocks, reset, registers and diagnostic procedures | [TI](https://www.ti.com/lit/pdf/snla423) |
| [DP83826A Troubleshooting Guide](source/TI-DP83826A-Troubleshooting-SNLA491.pdf) | SNLA491; August 2025 | DP83826AI / DP83826AE. A-generation reference; not an original-DP83826 procedure | [TI](https://www.ti.com/lit/pdf/SNLA491) |
| [DP83826 Schematic Design Review Checklist v0.3](source/TI-DP83826-Checklist-SNLR050C.zip) | SNLR050C; Archive; date not inferred | DP83826. Schematic and layout review checklist | [TI](https://www.ti.com/lit/zip/snlr050) |
| [DP83826EVM Schematic and Assembly Files](source/TI-DP83826EVM-Design-SNLR047.zip) | SNLR047; Archive; date not inferred | DP83826EVM. Reference schematics, assembly and PCB files | [TI](https://www.ti.com/lit/zip/snlr047) |

## Management, diagnostics and integration

| Local document | Revision / date | Applicability and use | Official download |
| --- | --- | --- | --- |
| [Solving Cable Faults Challenges with TI Ethernet PHYs](source/TI-Cable-Diagnostics-SNLA330A.pdf) | SNLA330A; January 2020 | Ethernet PHY diagnostics. Diagnostic background; exact sequences remain device-specific | [TI](https://www.ti.com/lit/pdf/SNLA330) |
| [How to use the TDR Feature of DP83822](source/TI-DP83822-TDR-SNLA253.pdf) | SNLA253; August 2016 | DP83822. Variant-specific cable diagnostics | [TI](https://www.ti.com/lit/pdf/SNLA253) |
| [F28P65 + DP83826 EtherCAT Frame Error Investigation Methods and Application Recommendations](source/TI-DP83826-EtherCAT-Frame-Errors-SNLA494.pdf) | SNLA494; February 2026 | C2000 F28P65 + DP83826. Secondary diagnostic example; not ESP32-P4 configuration | [TI](https://www.ti.com/lit/pdf/SNLA494) |
| [F28P65 + DP83826 EtherCAT Link Down Fault Troubleshooting Methods and Case](source/TI-DP83826-EtherCAT-Link-Down-SNLA495.pdf) | SNLA495; February 2026 | C2000 F28P65 + DP83826. Secondary diagnostic example; not ESP32-P4 configuration | [TI](https://www.ti.com/lit/pdf/SNLA495) |
| [How and Why to Use the DP83826 for EtherCAT Applications](source/TI-DP83826-EtherCAT-SNLA344C.pdf) | SNLA344C; October 2023 | DP83826. Protocol-specific context; distinguish master and slave roles | [TI](https://www.ti.com/lit/pdf/SNLA344) |
| [DP8382x IEEE 802.3u Compliance and Debug](source/TI-DP8382x-Compliance-SNLA266B.pdf) | SNLA266B; April 2025 | DP83822 / DP83825 / DP83826. Test modes and electrical debugging | [TI](https://www.ti.com/lit/pdf/SNLA266) |
| [How to Use Energy Efficient Ethernet (IEEE 802.3az) With Texas Instruments Ethernet PHYs](source/TI-Energy-Efficient-Ethernet-SNLA328.pdf) | SNLA328; October 2019 | Ethernet PHY low-power features. Optional feature background; not a board default | [TI](https://www.ti.com/lit/pdf/SNLA328) |
| [Optimizing EMC Performance in Industrial Ethernet Application](source/TI-Industrial-Ethernet-EMC-SNLA466A.pdf) | SNLA466A; October 2024 | Industrial Ethernet hardware. Secondary physical bring-up and EMC reference | [TI](https://www.ti.com/lit/pdf/SNLA466) |
| [Ethernet PHY Configuration Using MDIO for Industrial Applications](source/TI-MDIO-Configuration-SPRACC8A.pdf) | SPRACC8A; May 2021 | Ethernet PHY management. Management-interface and industrial configuration reference | [TI](https://www.ti.com/lit/pdf/SPRACC8) |
| [How and When to Use Ethernet PHYs in PROFINET Systems](source/TI-PROFINET-SNLA438.pdf) | SNLA438; September 2023 | Industrial Ethernet PHY configuration. Secondary protocol-specific configuration context | [TI](https://www.ti.com/lit/pdf/SNLA438) |
| [DP838xx Wake-on-LAN](source/TI-Wake-on-LAN-SNLA261A.pdf) | SNLA261A; March 2024 | DP8382x feature reference. Future wake feature; confirm exact variant support | [TI](https://www.ti.com/lit/pdf/SNLA261) |
| [DP83826 to DP83826A Hardware Rollover Document](source/TI-DP83826-to-DP83826A-SDAA067A.pdf) | SDAA067A; March 2026 | DP83826 versus DP83826A. Silicon-generation differences | [TI](https://www.ti.com/lit/pdf/SDAA067) |
| [DP83848 to DP83826 Hardware Rollover Document](source/TI-DP83848-to-DP83826-SNLA456.pdf) | SNLA456; April 2024 | DP83848 versus DP83826. Pin, feature and configuration differences | [TI](https://www.ti.com/lit/pdf/SNLA456) |

## Additional official entry points

- [DP83826I product documentation](https://www.ti.com/product/DP83826I)
  and [DP83826EVM](https://www.ti.com/tool/DP83826EVM).
- [DP83826AI](https://www.ti.com/product/DP83826AI),
  [DP83825I](https://www.ti.com/product/DP83825I),
  and [DP83822I](https://www.ti.com/product/DP83822I) product documentation.
- [TI Debug Interface for Ethernet PHYs](https://www.ti.com/tool/DIEP)
  and [USB-2-MDIO](https://www.ti.com/tool/USB-2-MDIO): optional bench tools;
  installers are not part of this source archive.
- [DP83826 IBIS model](https://www.ti.com/lit/zip/SNLM241): optional signal-integrity
  simulation input, linked rather than required for firmware implementation.

Marketing articles, installers, unrelated board components and entire SDK
archives are outside this firmware-reference collection. The official Git links
provide register/source references without importing third-party driver code.
See [device coverage](../variants.md) for unresolved DP83825 software identifiers
and the limited errata-search finding.

## Updating and checking

Use an official product/document URL to retrieve any update. Confirm the actual
payload format and document ID, inspect its revision history, and preserve the
original bytes. Update the index, metadata, hashes and derived text together.
Do not silently assume that an old product-page date is the PDF revision date.

From the repository root:

```sh
python tools/verify_references.py
rg -n "VOD_CFG3|REGCR|ADDAR|PHYRCR" docs/reference/text
```

The checker is offline. Its success proves integrity against this manifest,
not that these documents remain the latest revision or that the hardware works.
