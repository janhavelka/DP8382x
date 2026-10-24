# Device coverage

Reviewed on 2026-10-24. **No variant has an implemented or hardware-qualified
driver in this workspace yet.** The following describes documentation coverage
and intended development order.

| Device | Role here | Specification | Boundaries to preserve |
| --- | --- | --- | --- |
| DP83826I, including DP83826IRHBR | First implementation target | [SNLS647I, Aug 2025](reference/source/TI-DP83826I-E-SNLS647I.pdf) | Verify selected mode, PHY identity, straps, reset and 10BASE-Te setup |
| DP83826E | Same datasheet, secondary qualification target | SNLS647I above | Shared documentation does not establish tested support |
| DP83826AI / DP83826AE | A-generation reference | [SNLS783A, Oct 2025](reference/source/TI-DP83826AI-AE-SNLS783A.pdf) | Review [rollover changes](reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf) before reuse |
| DP83825I | Family expansion reference | [SNLS638C, Apr 2026](reference/source/TI-DP83825I-SNLS638C.pdf) | RMII-only device; do not expose unsupported MII/fiber features |
| DP83822I / H / IF / HF | Family expansion reference | [SNLS505H, Jul 2026](reference/source/TI-DP83822-SNLS505H.pdf) | Interface, temperature and fiber capabilities differ by exact variant |

The DP8382x name follows the family grouping used in TI's
[compliance/debug guide](reference/source/TI-DP8382x-Compliance-SNLA266B.pdf).
It does not imply compatibility with every chip whose name starts with DP8382.

## Identifiers appearing in software

TI's Linux source contains DP83825S, DP83825CM and DP83825CS identifiers. Official
product, GPN and datasheet-symlink checks for those names returned HTTP 404 during
this review. No current public standalone datasheet was located. Treat those
names as source-code references until their exact silicon documentation and
supported ordering codes are established.

Similarly, Linux's DP83826C/DP83826NC labels must not be mapped directly to TI
orderable suffixes. Resolve their operating-mode/compatibility meaning from the
exact datasheet and observed device ID. See [the pinned source](reference/source-code.md).

## Before extending support

For each additional device, document its identity/mode behavior, register map,
reset defaults, available interfaces, feature differences and initialization
requirements. Add tests for the actual differences and record hardware evidence
separately. Unsupported feature calls should fail explicitly.

No dedicated silicon-errata PDF was found among the reviewed TI product
documentation. This is a bounded search result, not a statement that no errata
or silicon issues exist. Recheck product documentation and TI support information
when implementing and before hardware qualification.
