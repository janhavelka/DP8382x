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
