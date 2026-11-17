# Prompt correctness and simplicity audit

Historical numbering: DP8382x stage numbers in this report refer to the former
34-stage plan. Use the [current 80-prompt index](../prompts/README.md) and
[old-to-new mapping](prompt-workload-audit.md) for execution; its sole first-set
remote-CI checkpoint is now 71. The original findings below retain their dated
scope and are not current execution instructions.

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

## 2026-11-17 Current-code response

Reviewed every table finding, simpler-procedure recommendation and open source
conflict against DP8382x `77c544f415edb26a0e23de76591a0163b9b73147`,
current public/core/native code, tests, ledger and owning prompts. Reopened
original archived SNLS647I, SNLS783A, SNLS638C, SNLS505H and SDAA067A rather
than using the old summaries as authority. SNLS647I tables8-19/22/23/39/49/85/90,
SNLS783A8-23/24, SNLS638C7-6/7/26/92 and SNLS505H8-23 were checked directly.
SNLS647I pages68/91 and SNLS505H pages11/69 were rendered for access/timing
columns. Current official TI endpoints still return the cited revisions;
SDAA067A section2/3.2 retains its conditional-register versus mandatory-strap
conflict. Selected local SDK source remains unmodified v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`.

| Original finding / recommendation | Current response and evidence | Disposition |
| --- | --- | --- |
| Branch-per-new-execution rule | Root AGENTS/COMMON now explicitly require main until version0; the old execution branch was integrated. Neither old commit21a22c4 nor these historical instructions authorizes a new branch today. | SUPERSEDED |
| W0C omitted from access/ledger rules | Original table8-90 explicitly says RXFCFG1[11] clears on zero. `CommandTable.h::composeWakeControl` uses neutral one and explicit zero acknowledgement; `DP8382xWake.cpp` shares it. `test/access.cpp` and `wol.cpp` check literal neutral/acknowledge and failure effects. Generic command-zero RMW is not used. | IMPLEMENTED |
| Aggregate staging lost consumed events | `_retainEvidence` merges each successful destructive prefix before reporting aggregate failure; `test/access.cpp`, `events.cpp`, `interrupts.cpp` check later failure, release/clock error and possible lost-history flags. Fresh public aggregate outputs remain staged. | IMPLEMENTED |
| Prohibited local ANAR remote fault | Original exact-part ANAR tables still prohibit bit13 despite R/W labeling. Current Advertisement exposes legal technologies/pause only, and register composition/validators reject prohibited mutation. `test/negotiation.cpp`/access policy cover it. Future56-61 retain exact-part applicability; partner/status fault observation remains allowed. | IMPLEMENTED primary; future variants OPEN |
| Active sleep policy mistaken for observed sleep | Original8.3.6.1/2 describes automatic partner-dependent sleep. Current `PowerSettings`/`PowerSnapshot`, `DP8382xPower.cpp` and `test/power.cpp` distinguish policy, software intent, observed BISCR state and physical wake. No partner-arrival bound is fabricated. | IMPLEMENTED |
| General MII-loopback prose applied to both speeds | Original BMCR table8-19 supplies speed-specific companion fields. Current diagnostics uses different 10/100 selections and shared restoration; `test/loopback.cpp` contains independent mode/speed vectors. Source discrepancy remains documented; physical packet/loopback qualification is not implied. | IMPLEMENTED software selection; physical OPEN |
| P4 RGMII/fiber wrongly treated as one impossible combination | `MacInterface` and future medium/subtype policy remain separate. Current DP83826I rejects RGMII; future822 generic capability is not constrained by P4. Current62/64/65 specifically record FX+RGMII as unstated/not excluded with strap encoding and physical NOT RUN, rather than asserting proven compatibility. FX has no copper AN. | CORRECTED planning; future family/physical OPEN |
| Direct configuration stales SDK AN cache | Native `PhyControl.cpp` reconciles through supported stopped SDK calls after shared-core configuration; local pinned `esp_eth.c` still caches AN. `test/esp_idf/host/native.cpp` executes actual driver cache transitions, partial failures and start admission. | IMPLEMENTED |
| Per-method locks miss raw SDK ioctl access | Current raw mediator hooks unconditionally reject bypass; owner raw/discovery/collector routes use complete shared-controller batches. `test/esp_idf` exercises actual raw ioctl routing and ownership. No recursive mediator/second collector is introduced. | IMPLEMENTED |
| SDK Boolean pause does not carry full RX/TX resolution | Current portable `LinkSnapshot` retains full pause resolution. Native projection uses the narrower peer-symmetric signal, and MAC enable policy is separate. Native pause/notification tests cover all portable RX/TX outcomes, forced stale-partner rejection and partial SDK publication. | IMPLEMENTED |
| Original/A model overlap, revision defaults and common VOD setting | Original model tables confirm11/13 with default revisions1/4; both8.3.13 notes require030E=4A40. Current identity policy accepts primary only; prompts56-59 parameterize exact sourced differences rather than infer suffix/revision compatibility from shared ID. | CONFIRMED source facts; future A implementation OPEN |
| DP83822 fixed3.5ns opposite-polarity shifts | Rendered original table8-23 confirms RX1 enables, TX0 enables. Current64 retains independent literals0800/1800/0000/1000. That claim is specifically RCSR RGMII; MII TXCPSR is separately assigned with its wording gate. | CONFIRMED correction; future implementation OPEN |
| Keep short calls direct, real waits cooperative, one slot | Shipped core has bounded direct C22/MMD/pin/interface/LPI calls and one operation slot for scheduled procedures. OP1-17 supersede early token-exhaustion and cleanup proposals. Admission/cancel/snapshots are local; stop/restoration uses a separately admitted finite operation where stimulus spans owner decisions. | IMPLEMENTED current contract |
| Separate long admission from short controller lock | Complete batches own controller execution; waits, dwell and output do not hold it. `test/mmd.cpp`, `operations.cpp`, `reset.cpp` and native readiness tests cover exclusion and shared silence. Controller return/clock errors retain independent effects. | IMPLEMENTED |
| Reuse compatible base behavior without duplicate PHY engines | Prompt6 selected a public PHY wrapper over the same core, rather than generic-base inheritance. `src/esp_idf/Phy.cpp`/`PhyControl.cpp` translate SDK entry points; all chip procedures are shared. Pinned base remains comparison evidence only. | IMPLEMENTED selected design |
| Share source-proved records with explicit per-part deltas | One primary ledger and future part-keyed contracts in56-67 retain shared map/procedures and exact changes. These are future acceptance, not existing family support; only declared DP83826I is currently admitted. | IMPLEMENTED primary structure; family OPEN |
| One simple bounded allowlisted package format and extracted consumer | Prompts69-70 require these checks. Current tests-OFF consumer proves build-tree use only; no release archive/install-export proof exists yet. | OPEN future implementation |
| Doxygen generated in owned temporary dir with failure propagation | `tools/check_doxygen.py` uses bounded temporary output and verifies actual generated API pages/negative fixtures. No last-good publishing subsystem is introduced. Stage76 still owns final source/API completeness review. | IMPLEMENTED checker; final review OPEN |
| EEE address representation unresolved | Original SNLS647I printed forms and cross-part low offsets remain conflicting. WP3 D2 selected exactly five source-implied low tuples with fail-closed capability/advertisement readbacks, recorded as inference. `CommandTable.h`, EEE core/tests and `docs/eee.md` implement it; Q1 remains physically unqualified. The old absolute prohibition on implementing any selected mapping is superseded, not silently treated as measured fact. | SUPERSEDED software deferral; Q1 OPEN |
| A-generation EMC Strap11 conflict | Original SDAA067A p3 allows register substitution if FLD is used; p7 says Strap11 must be high. Current variants/catalogue/57-59 preserve that material conflict and scoped CR3 ownership; neither register programming nor shared I/A IDs prove EMC. | OPEN source/physical evidence |
| PHYSTS, RECR and pin conflicts remain explicit | D4 uses BMCR for forced interpretation with extended-duplex uncertainty; raw PHYSTS remains. RECR exposes raw16-bit values without invented saturation totals. Unsupported pin routes/read-only conflicts remain gated. Current link/events/pins tests verify these fail-closed boundaries, not the unresolved physics. | IMPLEMENTED bounded selections; named gates OPEN |
| Historical test/review status,34 stages and sole CI29 | Those are dated records. Current plan has80 prompts and current progress through admitted primary/native work; first-set CI belongs71. This response does not reattribute earlier counts or turn original NOT RUN into new PASS. | HISTORICAL; numbering/status SUPERSEDED |

Additional future-family sanity checks requested during this review reopened
original SNLS638C table7-26 and SNLS505H7.4.2.1/6/7, table7-10 and8-60..64.
Prompts60-61 correctly treat825 role as declared/observed strap evidence, retain
consuming RCSR samples and mark inferred clock-loss consequences. Prompts62-67
retain the822 function10 contradiction, source-selected01 access, seven-bit
amplitudes, literal RMII forward recipe and explicitly inferred reverse/field
restoration with physical evidence still open. These are acceptance instructions,
not implemented or physically qualified family claims.

No production code or vendor payload was changed by this report response, and no
new driver, SDK, package, remote-CI or HIL result is claimed. Existing per-stage
reports retain their source/build identities; the lead records this maintenance
unit's final checks and synchronized commit separately.
