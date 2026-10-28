# DP83826I register map guide

The authoritative numeric map is [dp83826i.json](registers/dp83826i.json), with
[schema](registers/README.md). This guide explains its use without maintaining a
second numeric register table. The [reference inventory](reference/dp83826i-feature-inventory.md)
remains a source-navigation checklist, not a competing writable register map.

The ledger contains all 79 summary-table entries, 17 additional vendor recipe
offsets and five separately recorded EEE printed addresses. Fifty-four register
records (20 core and 34 diagnostic) have complete bit partitions, known reset
masks, legal/prohibited encodings, access semantics, sources and feature ownership.
Three additional digital-loopback records have only exact recipe actions and
unknown fields/reset values. The remaining 39 entries belong to prompt 5;
pending does not mean writable. There are 28 ordered procedures and 226 fields.

| Core responsibility | Completed register records | Interpretation |
| --- | --- | --- |
| Exact identity and capabilities | PHYIDR1, PHYIDR2, BMSR | Preserve raw words and declared suffix separately; defaults do not license future revisions. |
| Control and reset | BMCR, PHYRCR | Distinct PCS reset, full reset, AN restart and circuitry restart; commands have neutral values. |
| Advertisement and exchange | ANAR, ALNPAR, ANER, ANNPTR, ANLNPTR | Local prohibitions, partner observations, page/ACK/toggle and destructive-event ownership. |
| Addressing | REGCR, ADDAR | Separate selector/control/data roles; complete transaction serialization. |
| Link and crossover | CR1, CR2, CR3, PHYSTS, PHYCR | Core fields established; diagnostic/interface/LED feature procedures retain prompt-4/5 ownership. |
| Required initialization | VOD_CFG3 | Exact 10BASE-Te settings use documented read/write fields; reserved bits remain protected. |
| Strap evidence | SOR1, SOR2 | Captured board configuration, not invariant defaults or current software settings. |

| Diagnostic responsibility | Researched records | Interpretation |
| --- | --- | --- |
| Loopback and BIST | BISCR, BICSR1, BICSR2 and three digital recipe offsets | Exact mode/speed scope, distinct read-counter and write-capture meaning; traffic alone is not a pass. |
| Cable capture | CDCR, CDSCR family, TDR controls, CDLRR/CDLAR banks | Segment-dependent controls, trigger freshness and supported raw results; reserved banks grant no decoded peak capability. |
| Events and counters | PHYSCR, MISR1/2, FCSCR, RECR, RCSR, 10BTSCR, RXFS | Single collector, cross-register clears, retained partial samples and explicit width/polarity gates. |
| Quality and rapid link loss | MSE_Val, FLDS, FLD_CFG1/2 | Raw quality evidence, MII-only activation, physical timing and conversion conflicts. |

Shared power, interface, transmit and wake fields are numerically partitioned
where their registers are shared with diagnostics. Their feature procedures and
implementation remain assigned to prompt 5 and later stages.

C22 and MMD are separate address spaces. The standard set has explicit vendor
indirect aliases except the two selector registers, which are direct-only.
Protocol-valid C22 holes still fail documented-access policy. An MMD offset in
range does not imply that it is implemented or safe to read/write. No contiguous
bank is inferred from two documented endpoints.

The EEE entries retain the printed namespace and unresolved translation gate.
They cannot be fed directly to a wire helper. The disputed MSE alternative stays
in its source gate instead of becoming an automatic alias. Recipe-only vendor
offsets confer permission only through the exact sourced procedure once audited.

`resetValue` represents only known bits under `resetKnownMask`; it is never a
reset-write template. Field tables take precedence over coarse hexadecimal
headings where they provide more precise known bits. Strap- and mode-dependent
bits remain unknown across unspecified boards. In particular, SOR printed
defaults and CR3's conflicting default descriptions must not become fixed
universal readback assertions.

Reserved fields are not writable even when their table says R/W. Several named
TDR result entries and ANA_LD_PROG_SL are entirely reserved in their detailed
tables. They remain inventoried, with no ordinary writable permission. Prompt 4
completes the TDR records; prompt 5 owns remaining entries. A fully reserved
record partitions all bits as reserved and has no fields. BICSR1's overlapping
write action is narrowly allowlisted without relabeling its read counter as RW.

Run `python tools/verify_ledger.py` after every ledger edit and the registered
tests with `python -m unittest discover -s test -p "test_*.py" -v`. The validator
rejects duplicate IDs/aliases, bad ranges, incomplete/overlapping partitions,
unknown reset bits promoted into values, invalid sources and missing ownership.
Its fixed inventory prevents accidental row deletion from masquerading as full
coverage. [Literal vectors](register-vectors.md) and independent PDF review supply
numeric evidence; neither structural acceptance nor source review proves a PHY.
