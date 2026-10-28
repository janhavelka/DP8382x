# Prompt 5 source and coverage review

Reviewed 2026-10-28 against prompt 5, COMMON, EVIDENCE and the current sibling
agent rules. The reviewer owns this report only; the lead owns the ledger,
schema, guides, tests and Git. The execution branch is
`promptset/dp8382x-implementation-20261028`; the entry checkpoint is
`2805cfdea3aacd1f870d27998cd99d0270cccb2a`. No sibling or vendor payload was
modified. This disjoint review uses the available current model for source
comparison and structural coverage. Separate silicon reviewers derive literals
from original PDFs; this report does not substitute source-code macros for that
review.

## Current sibling evidence and adopted conventions

The paths below are relative to the sibling repository named in the first
column. They were reopened, including the implementation and rejection tests,
not inferred from an earlier report. These are read-only source observations;
the sibling test suites were not executed during this review.

| Repository and current revision | Actual paths and observations | PHY adaptation |
| --- | --- | --- |
| TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9`, clean | `include/TMP1x2/Config.h`: `Model`, `isValidModel`, exact address bounds and `hasAlertOutput`; `src/TMP1x2Configuration.cpp`: `validateConfig` rejects a wrong model/address and unavailable physical ALERT before transport; `test/variants/test_variants.cpp`: all 128 addresses for each declared model, invalid enum, absent ALERT rejection without bus/GPIO calls, and retained internal thermostat capability | Adopt explicit declared part and capability checks before I/O. Adapt identification to the PHY's readable OUI/model/revision. An observed compatible PHY identity corroborates the declaration; it does not identify I/E temperature suffix. A missing board pin does not erase a documented register feature. |
| MB85RC `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`, clean | `include/MB85RC/CommandTable.h`: `VariantInfo`, `KNOWN_VARIANTS`, exact product-ID selection, address model and independent implemented/HS/sleep flags; `src/MB85RC.cpp`: capability queries and guards in `setHighSpeedMode`, `enterSleep`, `wake`; `test/test_basic.cpp`: explicit/AUTO mismatch, no-device-ID AUTO rejection, exact-known identity decode, address bounds, HS/sleep unsupported rejection with unchanged bus traffic | Adopt explicit capabilities and independently recorded support evidence. Reject permission based only on a family-looking name or bus response. PHY research completion is separate from implementation, host, SDK and physical qualification. |
| TUSB32x `e1e79e6fb440fe2c3eb56d2df215b46722dcded3` | `AGENTS.md`; `docs/prompts/2-family-capabilities.md`: exact part matrix, declared part versus ACK/signature/revision, electrical limits, shared-map differences, blocked future revisions and source provenance | Adopt the capability-planning distinction and explicit unresolved gates. This is not an audit or claim that the complete TUSB32x implementation exists. Shared signatures cannot establish ordering suffix, pin polarity or unproved capabilities. |

TUSB32x had unrelated uncommitted changes in
`tools/test_tusb32x_hil_artifacts.py` and `tools/tusb32x_hil_artifacts.py`.
They were preserved and are not evidence for this task. TMP1x2's address-select
model specifically describes its ADD0 package; the letters in an ordering code
are not a generic capability parser. MB85RC's sleep callback can leave state
UNKNOWN after an ambiguous error: adopt that uncertainty principle, but neither
its wake delay nor any sensor conversion delay specifies a PHY deadline.

Relevant surrounding contracts were also inspected:

- TMP1x2 `examples/common/Tmp1x2CliDiagnostics.cpp` reports discovery identity
  as unverified and distinguishes physical ALERT capability;
  `tools/hil_tmp1x2_runner.py` correlates selected BOM model/address with explicit
  firmware identity and requires an explicit hardware target.
- MB85RC `examples/common/DiagnosticCore.h` has typed variant/address-model
  formatting; `tools/hil_runner.py` records active variant, HS/sleep support and
  awake/asleep/waking/unknown state separately. Reuse these reporting principles,
  not the memory command syntax or timing.
- TMP1x2 `.github/workflows/ci.yml` registers CMake/CTest, contract, Doxygen,
  package and target builds. MB85RC `.github/workflows/ci.yml` registers native
  tests, parser tests, metadata/core timing checks, CLI/IDF contracts and package
  checks. TUSB32x has no `.github` directory at this checkpoint. These are source
  inspections, not new CI runs. DP8382x remote CI remains prompt 71 only.

## Current manufacturer source check

Repository metadata and current branch commits were fetched on 2026-10-28 using
the connected GitHub tool. The actual source files below were fetched at the
immutable commits and their per-file license headers inspected. The pinned
commits match the preceding audit; no newer relevant implementation was found
in these checked default branches. No implementation was copied into the MIT
project, and the archived source provenance/hashes remain unchanged.

| Repository and checked branch | Immutable commit / actual file | Per-file terms and relevant result |
| --- | --- | --- |
| TexasInstruments/mcupsdk-core, default branch `next` | [`4b9a66f4e18e02a834961f5f834155c846cab041`, `source/board/ethphy/icss_emac/ethphy_dp83826e.c`](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.c) | BSD-3-Clause header, TI copyright 2021. The odd-nibble enable helper ORs its disable mask; LED1/LED3 shifts remain swapped relative to SNLS647I. Preserve the catalogue defect and independent PDF polarity/placement tests. |
| TexasInstruments/ethernet-rtos-drivers, `main` | [`94bf4343004732983b0b8e1eec123d88ff70453b`, `rtos_drivers/src/dp83826.c`](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) | BSD-3-Clause header, TI copyright 2020–2025. Inspected reset, loopback and MII/RMII paths; these do not supply complete power/wake/pin coverage or settle EEE wire addressing. |
| TexasInstruments/ti-ethernet-software, `main` | [`450af1144b8caeb58724bcbae081b409f6bf5c3a`, `zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c`](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c) | SPDX Apache-2.0; per-file notices include NXP, TI and named contributors. The VOD helper inserts configuration values into field masks. Declaring VOD_CFG3 does not execute the exact DP83826I 10BASE-Te recipe. The A-family path is a comparison, not authority to enable I-part controls or derive electrical units. |
| Same repository and commit | [`linux_drivers/dp83822.c`](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/dp83822.c) | SPDX GPL-2.0. DP83826 entries share the DP8382X macro's DP83822 WoL callbacks, byte ordering and set-to-clear logic. This does not override DP83826's own MAC tables or W0C indication definition. Factual review only; no GPL implementation import. |

The MCU SDK branch is `next`, not `main`. The combined repository is TI's
published Linux/Zephyr material; it must not be described as the current
upstream Linux/Zephyr tree. The previous primary-upstream catalogue pins remain
separate. Driver names such as DP83826C/NC and DP83825S/CM/CS remain source labels,
not newly supported ordering codes.

The current [TI DP83826I product page](https://www.ti.com/product/DP83826I)
still lists revision I as its datasheet and links the newer Ax part separately.
Its documentation list includes the A-revision rollover guide and the 2026
EtherCAT troubleshooting notes. The observed listing supplies no new exact-part
correction resolving the ledger's EEE, GPIO/clock or reserved-field conflicts;
this observation is not a claim that no errata could exist. Applicable source
review and unresolved evidence requirements stay in the ledger gates. EVM or
EtherCAT examples do not prove a P4 RMII master configuration.

## Later exact-family boundary, checked against original PDFs

These are future research assignments. They do not expand DP83826I support or
perform the later complete register/procedure/implementation audits.

| Declared part | Original source reopened | Later research and identification boundary |
| --- | --- | --- |
| DP83826E | [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf), p1 and table 8-22 p57 | Share facts only where the I/E source proves them; preserve separate temperature/electrical qualification. BASIC/ENHANCED model and revision readback cannot establish I versus E. |
| DP83826AI / DP83826AE | [SNLS783A](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf), p1 and table 8-23 p58; [SDAA067A](../../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf), pp1–4 | Re-audit exact Ax defaults, revision, strap11, FLD and EMC before sharing procedures. The model fields are shared with the earlier generation; revision defaults distinguish documented revisions, not AI versus AE ordering suffix. The rollover's Ax-only recommendation must not become an I recipe. |
| DP83825I | [SNLS638C](../../reference/source/TI-DP83825I-SNLS638C.pdf), p1 and table 7-6 p42 | Independently research its RMII-only MAC interface, package/straps, identity and all power/wake controls. Resolve the datasheet model versus the Linux I-labelled ID discrepancy before accepting a new identity profile; no S/CM/CS aliases by analogy. |
| DP83822I / H / IF / HF | [SNLS505H](../../reference/source/TI-DP83822-SNLS505H.pdf), device comparison p3 and table 8-4 p55 | Keep each ordering suffix explicit: only IF/HF claim fiber; I/IF and H/HF have different temperature ranges. Shared identity is not suffix detection. Re-audit fiber state, auto-negotiation restrictions, MII/RMII/RGMII, electrical controls and access/acknowledgement differences independently. |

The original PDF tables, not the production ledger, supplied this boundary
check. Source revisions are SNLS647I August 2025, SNLS783A October 2025,
SDAA067A March 2026, SNLS638C April 2026 and SNLS505H July 2026. Their bytes and
hashes remain under the existing provenance manifest. This report deliberately
does not create a second numeric register map.

## Coverage findings and required closure checks

At entry the actual JSON had 96 register records: 79 detailed-map entries plus
17 feature/recipe addresses. There were 54 COMPLETE, three RECIPE and 39 PENDING
records. Five printed EEE address/selector associations were tracked separately.
The checker already independently transcribes the 79-entry map and explicit
recipe/printed inventories; retaining this small fixed inventory is clearer than
introducing a generic register framework.

The lead should close and test these observed gaps while integrating the two
silicon source reviews:

1. Require a researched disposition for every register and all 35 feature
   families. No PENDING entry may survive prompt 5. COMPLETE means known field
   partitions; recipe-only or source-blocked entries retain unknown reset and
   field bits. Source-blocked still requires source, owner, affected scope and
   exact next evidence. Unknown bits must not be relabelled reserved to satisfy
   the partition checker.
2. Keep the five printed EEE associations outside admitted wire addresses and
   aliases. Closing their research disposition must not authorize digit
   stripping, lower-offset substitution or indirect I/O. Test rejection of
   each unproved wire conversion and missing gate/source/owner.
3. Preserve source-backed exceptional writes separately from generic writable
   fields. For partial recipe knowledge, allow only explicitly sourced masks
   and values; no generic saved-word restoration or invented whole-register
   reset. COMPT/IOCTRL reserved conflicts need exact exceptional action records,
   not widened writable masks. Test an extra bit/value and an unrelated
   procedure attempting to borrow the exception.
4. Require a primary software implementation owner in prompts 11–36 and a
   corresponding accepted or explicitly blocked procedure. Later integration,
   CLI, fixture and family stages may remain additional owners. The
   `outside_phy` architectural scope is application-owned and is not a PHY
   software implementation assignment.
5. Correct existing `CR2.extended_duplex` ownership from negotiation (17) to
   crossover (18), with consistent feature/procedure backlinks. Next-page
   exchange already belongs to 17. BMCR isolation and collision-test fields
   already name mii (33) and compliance (35); complete their procedure and
   independent vector coverage, rather than treating ownership alone as the
   procedure. The extended full-duplex field is bit 5 only (`0x0020`);
   table 8-28 p61 reserves bit 6. Reject a widened mask or reassignment to
   prompt 17. An early reviewer message incorrectly grouped bits 6:5; the
   lead's original-PDF check corrected it before any ledger change.
6. Keep documented/implemented/host-tested/SDK-built/hardware-qualified evidence
   separate. Ledger schema tests and malformed temporary fixtures prove
   structure and rejection, not executing PHY behavior. Source closure with an
   unresolved gate is valid; reporting the affected feature implemented is not.

These recommendations were sent to the lead before integration. The main
prompt-5 report records their implemented outcome and final suite results. This
review adds no physical qualification, SDK build, fixture run or remote CI
evidence. Existing sibling source checks were inspected, not run; all changes
remain within this report's ownership.

The in-progress checker extension was independently reviewed against its actual
diff. Its fixed remaining inventory, no-PENDING closure, separate blocked printed
addresses and exact exceptional-action tuples fit the small explicit schema.
Two additional rejection gaps were sent to the lead: an unlisted procedure in
the same feature could borrow an exceptional action, and a consuming procedure
could omit its action's gate. Require the exact allowed action set for every
procedure (empty for unlisted procedures), and retain every action gate on the
consumer. Negative cases should cover both, missing action gates, widened
action bits/values, downgraded research, empty printed controls, unblocked printed
addresses and out-of-range primary implementation owners. This was a code review
while fragments were pending, not a passing full-ledger test claim.

After merge, a semantic JSON comparison against the entry commit retained all
existing register, procedure, feature and gate IDs. Among previously researched
registers only the intended CR2 field ownership changed; among existing
procedures only `configure_an` changed its ownership/delegation. The two action
loopholes were closed in the actual checker. The focused remaining-ledger suite
passed all eight tests at this review point. The lead is adding independent
numeric cases separately.

The final coverage pass found a further structural bypass: relabelling a
programmable feature HARDWARE_ONLY allowed its primary software owners outside
11–36. An in-memory `power` mutation to HARDWARE_ONLY with owner stage 80 was
accepted. The recommended small correction fixes the three non-software scopes
(`straps`, `outside_phy`, `absent`) and requires SOFTWARE for the remaining
required feature families, with a rejection regression. Stale prompt-5 research
wording in PHYCR LED configuration and VOD_CFG3 validity, plus an unbounded
test-helper subprocess, were also reported for correction. The led_configure
procedure and pin_fields gate already correctly preserve the undescribed
PHYCR field; this wording change does not broaden write permission.

Final verification checked those corrections in the actual merged files:
fixed feature scopes now reject the reproduced power/stage-80 mutation,
the temporary-file CLI helper has a 20-second timeout, and PHYCR/VOD_CFG3
validity text points to the completed procedures and remaining source gates.
All review findings above are resolved. The final semantic comparison shows
only the intended CR2 ownership and PHYCR/VOD_CFG3 wording changes among
previously researched registers; the original numeric core and diagnostic
facts remain intact. The old tests changed only to use researched fixtures
after PENDING rows disappeared and to limit the diagnostic action oracle to
its own four actions; the new suite independently covers the remaining actions.

The reviewer ran `python -m unittest discover -s test -p 'test_*.py'` on the
merged working tree: **74 tests PASS**, including 17 remaining-ledger tests.
The validator also passed with 96 registers, 52 procedures and 35 feature
families. There are 79 COMPLETE detailed registers, 17 RECIPE entries with
unknown fields/reset masks preserved, and five separately blocked printed
EEE associations. All 35 families have complete research dispositions; all
software features retain primary owners in prompts 11–36. No additional
coverage or regression finding remains from this review. These results are
host research-record validation only; firmware/SDK/physical evidence remains
separate and unclaimed.
