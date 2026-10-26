# Prompt correctness and simplicity audit

Date: 2026-10-26. Baseline: `5e27fb869516e6fdf86f535f7ae137dbac5865fd`.
This is a review and correction of implementation instructions, not driver
implementation or hardware qualification. All 34 stages remain NOT STARTED.

Three read-only reviewers examined primary silicon, family differences and
architecture/tooling. The silicon tasks used `gpt-6-astra` with high reasoning;
architecture used `gpt-6.1-sol` with high reasoning. The lead checked findings,
edited shared contracts/stages and owned Git. No sibling sources were changed.
The reviewers rechecked corrections; small follow-up wording issues were fixed.

## Branch rule

Every new promptset execution creates a new branch before its first stage and
records the base/name. Main and a previous set's branch are not reused. Every
stage, agent and resumed session stays on that branch. Resuming the same set
does not create another branch. Prompt-document maintenance does not start the
implementation set. This rule was committed and synced as `21a22c4` on main
before the remaining audit corrections; per-prompt/intermediate sync is retained.

## Confirmed defects and misleading instructions corrected

| Finding | Evidence and risk | Correction |
| --- | --- | --- |
| W0C was missing from access/ledger rules | SNLS647I tables 8-18/8-90: RXFCFG1 bit 11 acknowledges level WoL by writing zero. A generic command-bits-zero update can acknowledge it accidentally | FULL_COVERAGE, ENGINEERING, stages 02/06/14 and inventory include W0C/W1S, source-specific mixed-register writes, neutral command values and acknowledgement/update tests |
| Aggregate read staging could lose consumed events | SNLS647I tables 8-36 through 8-39: MISR/event/counter reads clear evidence before later reads can fail | Stages 06/10 and shared rules retain each valid sample in the collector; incomplete aggregate output stays stale; ambiguous read failure marks possible history loss |
| Remote-fault advertisement was requested despite the chip prohibition | ANAR bit 13 is labeled R/W but applications must not set it on DP83826, A-generation and DP83825I (SNLS647I table 8-23, SNLS783A table 8-24, SNLS638C table 7-7) | Stage 09/inventory reject the local advertisement before writes; partner/status observation remains. Full coverage obeys application-prohibited encodings |
| Active sleep was treated like an immediately requested sleep state | SNLS647I 8.3.6.1 enables automatic partner-dependent transitions; BISCR bit 8 observes state | Stage 14/inventory distinguish policy configuration from actual sleep/wake. Enabling can succeed while linked/awake; no fabricated partner-dependent completion bound |
| General MII-loopback prose could be copied to both speeds | SNLS647I 8.3.14.2 conflicts with table 8-19's explicit 100/10 Mbit companion settings | Stage 12/inventory name the discrepancy and use detailed speed-specific definitions with independent vectors, absent stronger exact-part evidence |
| P4 RGMII and fiber were grouped as impossible | SNLS505H separates MAC interface from MDI medium; IF/HF support 100BASE-FX, which has no autonegotiation | Stage 26 separates those axes, retains supported MAC-interface/optical-fixture qualification, and does not apply copper negotiation to fiber |
| Direct configuration could stale IDF's AN cache | Pinned `esp_eth.c` caches `auto_nego_en`; start/getters/forced admission use it | Stages 03/15/19/22 route SDK-mirrored settings through supported stopped-driver controls and require reconciliation after raw mutation. Other PHY settings remain shared procedures |
| PHY-method locks do not intercept raw SDK ioctls | `ETH_CMD_READ_PHY_REG`/`WRITE_PHY_REG` go directly through the mediator | Stages 03/15/22 require an actual owner-aware path or prohibit direct bypass. Public per-frame hooks do not replace whole-transaction ownership or justify recursive mediator calls |
| Full pause evidence was implicitly demanded from a narrower SDK notification | Pinned base maps nonzero advertised ability to both pause bits; MAC peer-pause notification is Boolean, not independent RX/TX resolution | Stages 03/15 preserve full portable configuration/evidence, document the SDK projection limit and override semantically incompatible base methods |

The review also added concrete family anchors: original/A model IDs overlap,
their documented revision defaults differ, and A repeats the same documented
10BASE-Te setting. DP83822 RGMII shifts are fixed 3.5 ns choices with opposite
RX/TX enable polarities. These clarify the design; they do not establish current
software or physical support.

## Simpler implementation and procedures

- Keep short reads/writes direct and bounded. Cooperative operations are for
  real scheduled waits/cleanup, using one active slot and simple token exhaustion.
- Keep operation admission across polls separate from the short bus transaction
  lock. Release the latter before wait intervals and console backpressure.
- Reuse base methods by semantic fit; one shared portable procedure can serve an
  overridden SDK callback. Do not create duplicate full standard-PHY drivers.
- Share source-proved register/field/procedure records across exact variants,
  with explicit applicability/differences and separate evidence per part.
- Validate one emitted package format against an exact content allowlist using
  standard parsing plus containment/bounds checks; no generic archive framework.
- Generate/check Doxygen in an owned temporary directory and propagate failure.
  Local validation needs no last-good-output or publishing subsystem.

These changes preserve complete documented feature scope, tests, source Doxygen,
local verification and first-set remote CI only at prompt 29.

## Source conflicts that remain open

**EEE address representation:** TI documents explicitly pair printed 1001/1014/
1016 and 203C/203D with MMD selectors, while DP83825I field descriptions also use
low offsets 0014/003C/003D. The exact DP83826 wire offsets or possible aliases
were not established. Inventory and stage 14 now distinguish printed addresses
from verified wire tuples. Neither stripping digits nor treating a translation
as proved is justified; obtain an exact applicable trace/clarification.

**A-generation EMC strapping:** SDAA067A section 2 allows Strap11 high or a CR3
setting, while section 3.2 requires the strap high. This remains a specific
hardware/EMC evidence conflict. Software FLD programming is not proof of measured
EMC performance. Existing PHYSTS, RECR, pin-map and other inventory conflicts
also remain visible; this audit does not silently resolve them.

## Primary evidence and verification

Original archived PDFs were read, with important tables visually checked:

- [DP83826 SNLS647I](../reference/source/TI-DP83826I-E-SNLS647I.pdf): pp27-29,
  41, 54, 57, 64, 66-69 and 91-93. The [official TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
  still returned revision I during this review.
- [A-generation SNLS783A](../reference/source/TI-DP83826AI-AE-SNLS783A.pdf):
  pp27, 41 and 58; [rollover SDAA067A](../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf): sections 2/3.2.
- [DP83825I SNLS638C](../reference/source/TI-DP83825I-SNLS638C.pdf): table 7-7
  and table 7-92; [DP83822 SNLS505H](../reference/source/TI-DP83822-SNLS505H.pdf):
  sections 7.4.4/7.4.6 and table 8-23.
- SNLA261A and SNLA328 from the [reference index](../reference/README.md):
  cross-part WoL recipe and printed EEE address comparisons, not replacements
  for exact-part definitions.
- ESP-IDF commit `fff9895c82d744c7237be8847347bdd1b07c6643`:
  [driver/cache/ioctls](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/esp_eth.c),
  [public register hooks](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/esp_eth_driver.h),
  [PHY base](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/phy/esp_eth_phy_802_3.c)
  and [MAC pause handling](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/mac/esp_eth_mac_esp.c).

Local documentation checks are recorded in [promptset validation](promptset-validation.md).
No firmware, driver test, SDK build, Doxygen build, remote CI or HIL ran for this
audit. Vendor payloads/provenance remain unchanged. Source review identifies
requirements and risks; later implementation still needs executing evidence.
