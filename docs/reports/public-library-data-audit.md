# Public-library data and prompt evidence audit

Reviewed on **2026-10-26**, starting from synchronized maintenance commit
`035a6c2e556bf3a704bfa45c99ef39581803593a` on `main`. This task enriches the
implementation plan; no implementation execution branch or stage has begun.

## Result and source coverage

The [catalogue index](../reference/library-evidence.md) leads to four detailed
references, totaling 12,069 whitespace-separated words at this audit:

| Reference | Extracted information |
| --- | --- |
| [TI core](../reference/library-ti-core.md) | Full named register/mask inventory from the inspected RTOS private/generic headers; exact-part identity, MDIO, reset, link, access effects, procedure ordering and source defects |
| [TI features](../reference/library-ti-features.md) | MCU+/Zephyr differences and exact TI tables for diagnostics, interrupts, power, EEE/LPI, wake, LED/GPIO, clocks, quality, analog tuning and compliance |
| [Family](../reference/library-family.md) | E/A/DP83825I/DP83822 identity, interface, fiber, wake, analog, TDR, reset and timing differences with exact-subtype conflicts |
| [SDK](../reference/library-sdk.md) | Actual ESP-IDF base/MAC/driver defaults, register access, loop intervals, cache behavior, callback ownership and integration hazards |

TI dedicated Ethernet RTOS, MCU+ ICSS-EMAC, combined Ethernet software and TI
Linux sources were inspected at immutable commits. Upstream Zephyr and ESP-IDF
provide primary implementation comparisons. The references record exact paths,
line anchors, licenses and, where collected, Git blob identities. Additional
TI SDK copies were checked: identical blobs repeat the same defects and are
not independent corroboration. Targeted NXP/ST searches and their limits are
recorded in the index. No search is claimed to exhaust all public material.

Exact archived TI datasheets/app notes were compared with source behavior;
ambiguous original PDF tables were visually inspected. The catalogues separate
code observations, exact-part corroboration, conflicts and unresolved meanings.
No copied GPL implementation, binary installer, vendor payload modification or
new firmware dependency is introduced. Actual code reuse still requires its
per-file license and notices.

## Concrete corrections and open facts

The references and affected prompts now include these regression inputs:

- TI RTOS passes reset mask `0x8000` as a register address instead of PHYRCR
  `0x1F`, ignores several configuration failures and can return successful
  link observation without assigning the output on a low first BMSR read.
- TI's MLEDCR mask `0x0038` truncates the documented `[6:3]` field (`0x0078`).
  RTOS/MCU+ LED1 and LED3 labels are swapped; LEDCFG2 includes reserved fields
  mislabeled as LED3 controls. MCU+ odd-nibble enable naming reverses the
  documented CR2 bit polarity.
- TI Zephyr's `(0x4000 & devad)` does not select indirect data mode. Its DAC
  binding units are not converted to the register encoding. Fixed busy waits
  and unbounded mutex acquisition are separate findings; no infinite polling
  loop is claimed where the inspected code has none.
- Pinned ESP-IDF indirect helpers write a control union without initializing
  its reserved bits. Defaults include no post-reset delay, GPIO 5 and a 2000 ms
  link-check period; none supplies a board declaration or TI readiness proof.
  Negotiation restart can return success after its polling limit without link
  completion. Raw mediator access and cached negotiation policy need one owner.
- DP83825I identity differs between SNLS638C (`0x2000A140`) and the Linux I
  label (`0x2000A150`). EEE printed-address translation, A-generation strap/EMC
  guidance and identified register-description conflicts remain explicit gates.
  These cannot be resolved by copying another driver or widening a match mask.

Independent review corrected this draft too: SNLS505H explicitly permits
DP83822 indirect access to standard registers except REGCR/ADDAR, so that alias
is not an unresolved source question. Its 120 ns latch, 64 ns output transition
and 1.5 s FLP values are TYP, not MAX. The DP83826 VOD citation was corrected to
8.3.13/page 40. Linux MAC-word packing remains an observed candidate requiring
independent exact-part byte-order derivation, not a self-validating oracle.

These are planning findings, with detailed source support in the catalogues.
No source review establishes physical behavior for this repository's hardware.

## Prompt integration and review

Sixty numbered prompts receive focused numeric inputs and failure cases. All
80 require [EVIDENCE](../prompts/EVIDENCE.md) through COMMON and root AGENTS:
reopen actual pinned code/helpers, confront exact-part tables and timing
columns, derive independent vectors, preserve unknowns, and correct all affected
prompts/contracts/code/tests/docs when stronger evidence disproves a claim.
Real hardware observations must identify the fixture and conditions; missing
measurements stay NOT RUN/BLOCKED and never become invented expected results.

The 80 task boundaries, natural execution order, per-prompt documentation,
subagent rules and commit/sync checkpoints remain intact. Numbered files now
range from 412 to 775 words, median 547, using whitespace-separated counts.
Detailed tables are linked accompanying inputs, rather than repeated wholesale
inside each task. Source catalogues feed the one future implementation ledger;
they are not a second production map or a replacement for full feature coverage.

Three agents handled disjoint core, feature and family evidence. Stronger
reasoning was selected for register/timing semantics; a capable coding model
handled the bounded family comparison. They then reviewed one another's data.
The lead reviewed contributions, investigated SDK ownership, integrated shared
rules and retained exclusive Git ownership. No sibling project was modified.

## Verification and scope

`python tools/verify_references.py` passed for 27 vendor artifacts
(43,240,385 bytes), 25 text extracts and 1160 local links in 124 Markdown files.
One-off checks passed for natural 1-80 numbering, index/COMMON/progress links
and all 80 NOT STARTED rows, plus 45 local heading links in changed files.
Final formatting acceptance is UTF-8/LF, one final newline and Git whitespace
validation. The original vendor artifacts and derived text remain unchanged.

Driver/host tests, SDK builds, remote CI and physical experiments are NOT RUN:
this change contains planning documents only. The first execution's sole remote
CI checkpoint remains prompt 71. No board was flashed or operated, no servo was
moved, and no driver or package support is claimed by this research.

## 2026-11-17 Current-code response

Reviewed all original findings, proposed corrections and scope claims against
DP8382x `77c544f415edb26a0e23de76591a0163b9b73147`. The four source
catalogues remain dated comparison inputs; `docs/registers/dp83826i.json` is the
single primary implementation ledger. Current original PDFs, pinned TI source
and local unmodified ESP-IDF v6.1 source were reopened. The official TI endpoints
still identify SNLS647I, SNLS638C, SNLS505H, SNLS783A and SDAA067A at their
archived revisions on this review; the A symlink failed in the web tool, so its
[official GPN endpoint](https://www.ti.com/lit/gpn/DP83826AI) was checked instead.
This does not establish that every external source or support clarification has
been exhaustively searched.

| Original finding / recommendation | Current response and code/test/plan evidence | Disposition |
| --- | --- | --- |
| Catalogue completeness/word counts/source scope | Four linked catalogues still exist, with source identities and comparison dispositions. The old 12,069-word/60-enriched-prompt counts describe that maintenance checkpoint, not a current quality quota or exhaustive public-code inventory. Identical vendor blobs are one inherited implementation, not independent corroboration. | HISTORICAL counts; IMPLEMENTED evidence structure |
| RTOS reset mask used as register address | Reopened pinned `ethernet-rtos-drivers` source at `94bf4343004732983b0b8e1eec123d88ff70453b`: `Dp83826_isResetComplete` still reads the mask symbol. Current core uses PHYRCR address 001F and rejects C22 address 8000 before callbacks. `test/reset.cpp` and `transport.cpp` contain independent address/bounds cases. | CONFIRMED upstream defect; IMPLEMENTED local avoidance |
| Ignored configuration errors and low-first-BMSR output not assigned | Pinned TI generic helper/source behavior remains a rejected comparison. DP8382x batches stop at first failure and stage public output; every consumed valid prefix is retained. `DP8382xLink.cpp`, `test/link.cpp`, `transport.cpp` and `mmd.cpp` cover low-first history, later failure and output sentinels. | IMPLEMENTED local correction |
| MLEDCR source mask 0038 truncates bits6:3 | Pinned private header still contains 0038. Reopened original SNLS647I table 8-49; current `CommandTable.h` has 0078 and `test/access.cpp`/`pins.cpp` test all legal source placements independently. | CONFIRMED upstream defect; IMPLEMENTED |
| LED1/LED3 swaps and reserved LEDCFG2 controls | Original tables 8-85/89 govern DP83826I: current masks are LED1 00F0, LED3 F000 and only documented LED1/2 drive fields. `DP8382xPins.cpp`, `test/access.cpp`/`pins.cpp` avoid peer mislabels/reserved writes. A-generation moved fields remain separate future deltas in prompts 57/59. | IMPLEMENTED primary correction; A deltas OPEN |
| MCU odd-nibble label reverses CR2 bit polarity | Current `MASK_CR2_ODD_NIBBLE_DISABLE=0002` agrees with the detailed table. Interface settings use disable semantics rather than inheriting the misleading upstream enable label; `test/interface.cpp` supplies literal vectors. | IMPLEMENTED |
| TI Zephyr data-mode AND, DAC units, fixed waits/unbounded mutex | Reopened pinned `450af1144b8caeb58724bcbae081b409f6bf5c3a` file: extended helpers use AND and lock calls use K_FOREVER. Current core constructs 4000 OR DEVAD, typed analog codes use source-bounded conversion, and terminal batches carry aggregate deadlines. `test/mmd.cpp`, `analog.cpp`, `operations.cpp` exercise actual code. The audit does not invent an infinite loop where source is finite. | CONFIRMED rejected source behavior; IMPLEMENTED local alternatives |
| ESP-IDF uninitialized selector union and unsuitable board defaults | Local pinned base source remains a comparison, not the native transaction executor. Current core constructs whole selector words; native application configuration requires explicit address/board readiness and selected poll period. `test/sdk_mmd_poison.c` is a bounded probe, while `test/mmd.cpp` covers production frames. GPIO5/zero reset delay/2000ms defaults are not TI readiness proof. | IMPLEMENTED avoidance |
| SDK restart success without negotiation completion | Native SDK RESTART uses the shared core terminal restart/readback; `protocolComplete` and observed negotiation completion remain separate. `test/native_restart.cpp` and native tests cover the actual route. No wait-loop completion or link-up is fabricated. | IMPLEMENTED semantic correction |
| SDK AN cache/raw-ioctl ownership | Current `Phy.cpp`/`PhyControl.cpp` reject direct raw hooks and route stopped settings through the shared core plus supported SDK cache reconciliation. Native tests compile actual pinned `esp_eth.c`, including partial cache/publication cases. | IMPLEMENTED |
| DP83825I identity discrepancy | Reopened SNLS638C table7-6 and pinned Linux ID declarations: A140 versus Linux I A150 remains. Prompt60 uses exact datasheet policy with raw mismatch retention, not a wider match mask. No DP83825I code/support exists yet. | OPEN future exact-part work; discrepancy retained |
| EEE printed-address translation | WP3 D2 now selects exactly five source-implied DEVAD3/7 low tuples with fail-closed whole-word capability/advertisement checks. `CommandTable.h`, `DP8382xEee.cpp`, `test/eee.cpp` and `docs/eee.md` implement that bounded selection. It is not a physically proved general digit-stripping rule or a vendor31 alias. Q1 remains pending. | SUPERSEDED absolute software deferral; physical gate OPEN |
| A strap/EMC and other register-description conflicts | Original SDAA067A section2 says register substitute if FLD is used, while3.2 requires Strap11. Current ledger/variants and prompts57-59 retain that discrepancy; software CR3 ownership preserves unowned pair/polarity bits and proves no EMC performance. RECR/PHYSTS/pin and analog conflicts remain specifically bounded, rather than silently resolved. | OPEN exact source/physical gates |
| DP83822 standard indirect aliases are documented | Reopened SNLS505H7.4.2.1: standard aliases except direct-only selectors are explicit. Prompt62 retains that fact, and separately records its contradictory function10 text versus examples/table. No-post-increment01 is sufficient; no assumption of increment10 is necessary. | CONFIRMED correction; future implementation OPEN |
| DP83822 120 ns/64 ns/1.5 s are TYP | Original PDF page11 was rendered: all three occupy TYP, not MAX. Current family catalogue/prompts preserve timing qualifiers. These values cannot become hard timeout guarantees. | CONFIRMED correction |
| VOD citation and Linux MAC-word packing | Original SNLS647I8.3.13/page40 confirms the citation. Wake code/tests derive MAC/SecureON words from exact-part byte labels, not Linux as an oracle; W0C is composed separately. `test/wol.cpp` and `access.cpp` cover asymmetric literals and neutral acknowledgement. | IMPLEMENTED |
| EVIDENCE integration, one ledger and all-required feature scope | COMMON/EVIDENCE/FULL_COVERAGE remain mandatory and source catalogues feed one ledger. Current feature matrix distinguishes admitted implementation from required gated branches. Archive absence/vendor-API omission does not mark a documented control unsupported. | IMPLEMENTED contract; complete closure OPEN |
| All80 NOT STARTED / no implementation branch and original test counts | Those statements were true of that dated preparation. Current headers/core/native implementation and progress supersede them; root AGENTS requires main until version0. Old documentation counts/test numbers are historical and are not rerun results for this maintenance. | HISTORICAL; current-status interpretation SUPERSEDED |
| Per-stage docs/checkpoints, bounded task boundaries and review roles | Current COMMON/progress retain scoped complete units and lead integration/Git. Prompt-workload mapping remains valid; future CLI/runner/package/family/physical/CI work is not retroactively complete because the plan contains it. | IMPLEMENTED rules; future acceptance OPEN |
| Licenses, original vendor payloads, no external dependencies or hardware claims | No source code/vendor bytes are imported or modified by these report responses. GPL factual references stay distinct from MIT implementation. Packaging must enforce exclusions at69-70. Physical and remote-CI results remain NOT RUN here. | IMPLEMENTED scope boundary; package/physical gates OPEN |

The source references above were reopened at their pinned identities; the
[PHY response](phy-implementation-audit.md) and [correctness response](prompt-correctness-audit.md)
record original-table checks. Source review does not create fresh driver/SDK/HIL
PASS evidence. Lead-owned task verification and final synchronization remain the
completion record for this maintenance unit.
