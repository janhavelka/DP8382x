# DP83826I register map guide

The authoritative numeric map is [dp83826i.json](registers/dp83826i.json), with
[schema](registers/README.md). This guide explains its use without maintaining a
second numeric register table. The [reference inventory](reference/dp83826i-feature-inventory.md)
remains a source-navigation checklist, not a competing writable register map.

The ledger contains all 79 summary-table entries, 17 additional vendor recipe
offsets and five separately recorded EEE printed addresses. Twenty core register
records have complete bit partitions, known reset masks, legal/prohibited
encodings, access semantics, sources and feature ownership. Other entries remain
explicitly pending research by prompt 4 or 5; pending does not mean writable.

| Core responsibility | Completed register records | Interpretation |
| --- | --- | --- |
| Exact identity and capabilities | PHYIDR1, PHYIDR2, BMSR | Preserve raw words and declared suffix separately; defaults do not license future revisions. |
| Control and reset | BMCR, PHYRCR | Distinct PCS reset, full reset, AN restart and circuitry restart; commands have neutral values. |
| Advertisement and exchange | ANAR, ALNPAR, ANER, ANNPTR, ANLNPTR | Local prohibitions, partner observations, page/ACK/toggle and destructive-event ownership. |
| Addressing | REGCR, ADDAR | Separate selector/control/data roles; complete transaction serialization. |
| Link and crossover | CR1, CR2, CR3, PHYSTS, PHYCR | Core fields established; diagnostic/interface/LED feature procedures retain prompt-4/5 ownership. |
| Required initialization | VOD_CFG3 | Exact 10BASE-Te recipe exception; no general reserved/read-only tuning API. |
| Strap evidence | SOR1, SOR2 | Captured board configuration, not invariant defaults or current software settings. |

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
tables. They remain inventoried, with no ordinary writable permission. Prompts
4–5 complete their exact reset/access records without inventing functionality
from names. A future fully reserved record partitions all bits as reserved and
has no fields.

Run `python tools/verify_ledger.py` after every ledger edit and the registered
tests with `python -m unittest discover -s test -p "test_*.py" -v`. The validator
rejects duplicate IDs/aliases, bad ranges, incomplete/overlapping partitions,
unknown reset bits promoted into values, invalid sources and missing ownership.
Its fixed inventory prevents accidental row deletion from masquerading as full
coverage. [Literal vectors](register-vectors.md) and independent PDF review supply
numeric evidence; neither structural acceptance nor source review proves a PHY.
