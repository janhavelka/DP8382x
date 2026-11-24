# Device coverage

The core and native adapter accept application-declared DP83826I and DP83826E
in BASIC or ENHANCED mode at revision 1. They use the same register map and
procedures. The temperature range and the electrical limits at those
temperatures come from the ordering grade. Both parts retain the open
procedure gates in the primary ledger. Physical qualification is pending for
each part. [Shared silicon record](reports/implementation/56-e-support.md#shared-silicon-record)
and [initialization](initialization.md).

The primary DP83826I [ledger](registers/dp83826i.json) and
[behavior contract](device-behavior.md) now close primary-part source research.
The diagnostic masks, procedures and source gates are exact-part evidence;
they do not qualify other family members or promise calibrated cable/SNR results.
The accepted
documented revision is 1, with BASIC/ENHANCED observable model IDs kept separate
from the declared I/E suffix. Other revisions require an explicit acceptance
decision; shared model values do not establish A-generation support. PHYSTS
forced-mode interpretation and software-hard-reset strap resampling retain named
gates. Later variants must qualify these independently.

| Device | Role here | Specification | Boundaries to preserve |
| --- | --- | --- | --- |
| DP83826I, including DP83826IRHBR | First implementation target | [SNLS647I, Aug 2025](reference/source/TI-DP83826I-E-SNLS647I.pdf) | Verify selected mode, PHY identity, straps, reset and 10BASE-Te setup |
| DP83826E | Declared original extended-temperature grade | SNLS647I above | Same core procedures as I. Declared-E host, SDK and physical evidence are recorded separately in the [qualification report](reports/implementation/56-e-support.md) |
| DP83826AI / DP83826AE | Revision-4 core/link, diagnostics and optional controls | [SNLS783A, Oct 2025](reference/source/TI-DP83826AI-AE-SNLS783A.pdf) | Shared revision-4 policy and procedures. AI is rated −40 to +85 °C, AE −40 to +105 °C. Each grade needs its own qualification. |
| DP83825I | Family expansion reference | [SNLS638C, Apr 2026](reference/source/TI-DP83825I-SNLS638C.pdf) | RMII-only device; do not expose unsupported MII/fiber features |
| DP83822I / H / IF / HF | Family expansion reference | [SNLS505H, Jul 2026](reference/source/TI-DP83822-SNLS505H.pdf) | Interface, temperature and fiber capabilities differ by exact variant |

The DP8382x name follows the family grouping used in TI's
[compliance/debug guide](reference/source/TI-DP8382x-Compliance-SNLA266B.pdf).
It does not imply compatibility with every chip whose name starts with DP8382.

The [implemented variant decision table](identity.md) separates declared part,
observed ID/mode, implemented admission and hardware qualification. Both modes
admit MII and RMII, without claiming the observed interface strap matches wiring.

## Identifiers appearing in software

TI's Linux source contains DP83825S, DP83825CM and DP83825CS identifiers. Official
product, GPN and datasheet-symlink checks for those names returned HTTP 404 during
this review. No current public standalone datasheet was located. Treat those
names as source-code references until their exact silicon documentation and
supported ordering codes are established.

Similarly, Linux's DP83826C/DP83826NC labels must not be mapped directly to TI
orderable suffixes. Resolve their operating-mode/compatibility meaning from the
exact datasheet and observed device ID. See [the pinned source](reference/source-code.md).

## A-generation software scope

Declare `Variant::DP83826AI` or `Variant::DP83826AE` in the binding. Identity,
reset, startup VOD, link configuration and observation, fast link drop, recovery
and interface/clock control use the shared driver. The diagnostic owners
also support A-grade loopback, BIST, TDR, counters, IRQ, wake, power, EEE,
LPI and electrical controls. Reversible deep power and the COUNT100 pattern
remain gated at 0428. Pin 31 retains the figure/table conflict, and the raw
MSE sample has no qualified conversion. Each grade needs its own fixture
record. [Exact-part coverage](feature-coverage.md#dp83826ai-and-dp83826ae)

For either A declaration, the accepted BASIC identity is `2000 A114` and
ENHANCED is `2000 A134`. Only revision 4 has an accepted A source record.
The application supplies the suffix. The register comparison identifies each
field by its original semantic ID and A table, including reserved bits, strap
defaults and OTP-loaded values. The procedure comparison assigns core/link,
FLD and clock work to 58 and diagnostic/optional controls to 59. Both sets of
tests apply separately to AI and AE. [Register comparison](registers/dp83826a.json)
and [procedure comparison](registers/dp83826a-procedures.json).

The [family source catalogue](reference/library-family.md) adds the 2026-10-26
numeric comparison. DP83825I identity remains a specific conflict: SNLS638C
table 7-6 gives `0x2000A140`, while the pinned TI Linux driver labels
`0x2000A150` as I and `0x2000A140` as S. Implement the exact datasheet's documented
admission policy and tests, retaining that discrepancy. Qualify actual observed
identity separately; accepting an alternate ID needs exact-part/revision evidence.

Use these source anchors when establishing support:

- Original DP83826 and A-generation use model values 0x11 BASIC / 0x13 ENHANCED.
  Their documented revision defaults differ (1 / 4); these are not a future-proof
  revision acceptance policy or an I/E suffix detector. See SNLS647I table 8-22
  and SNLS783A table 8-23.
- SNLS783A 8.3.13 repeats the 10BASE-Te `0x030E=0x4A40` setting. Share proved
  common behavior and qualify reset/order differences rather than duplicating it.
- DP83826, A-generation and DP83825I prohibit local ANAR remote-fault bit 13
  despite its R/W label (SNLS647I table 8-23, SNLS783A table 8-24, SNLS638C table
  7-7). Partner/status observation is separate; other parts require exact evidence.
- DP83822 IF/HF fiber parts are identified in SNLS505H section 4; MII/RMII
  operation is described in 7.4.1.1/7.4.1.2. FX+RGMII is not explicitly stated;
  there is no documented exclusion and table 7-10 RX_ER mode 3 can encode it,
  but physical evidence is NOT RUN. 100BASE-FX has no autonegotiation (7.4.6).
  RGMII shifts are fixed 3.5 ns with opposite RX/TX enable polarities
  (table 8-23), not a programmable tap range.
- SDAA067A sections 2 and 3.2 disagree on whether an A-generation register
  setting can substitute for Strap11 in EMC guidance. Keep that specific source
  conflict open and separate software FLD control from measured EMC qualification.

The [prompt-5 source review](reports/implementation/5-coverage-source-review.md)
reopened each original family identity/device-comparison table and the rollover
guide. Keep these later research boundaries:

| Stages | Exact declared parts | Required independent work |
| --- | --- | --- |
| 56 | DP83826E | SNLS647I shared software records and procedures; qualify the I/E temperature range and temperature-dependent electrical limits. No suffix-conditioned reset/optional-control behavior or suffix detection from shared ID. |
| 57–59 | DP83826AI, DP83826AE | SNLS783A/SDAA067A defaults, revision acceptance, strap 11/FLD/EMC conflict and complete diagnostics/optional controls. |
| 60–61 | DP83825I | SNLS638C RMII-only topology, identity discrepancy, package/straps and complete power/wake/EEE differences; SNLA328 remains exact-part input here. |
| 62–67 | DP83822I, H, IF, HF | SNLS505H per-subtype media/temperature, fiber and negotiation, interfaces/fixed shifts, reset, diagnostics, electrical and WoL access differences. |

Source-proved shared facts should reference one record with explicit differences.
Neither shared model IDs nor revision defaults detect an ordering suffix.
Undocumented Linux suffixes remain reference-only. These assignments do not
constitute another family's completed implementation audit or support claim.

For each additional device, document its identity/mode behavior, register map,
reset defaults, available interfaces, feature differences and initialization
requirements. Add tests for the actual differences and record hardware evidence
separately. Unsupported feature calls should fail explicitly.

No dedicated silicon-errata PDF was found among the reviewed TI product
documentation. This is a bounded search result, not a statement that no errata
or silicon issues exist. Recheck product documentation and TI support information
when implementing and before hardware qualification.
