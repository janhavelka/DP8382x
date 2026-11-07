# 57 - Establish exact AI/AE source and identity policy

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Inputs
are the frozen variant/operation contracts and original-generation ledger and
procedures, including usable [56](56-e-support.md) outputs. An unrelated E
evidence gate does not prevent independent A-generation research. This prompt
establishes researched contracts; it does not declare A-generation software or
hardware support.

## Current evidence and ownership

Reopen `../MB85RC/include/MB85RC/CommandTable.h::VariantInfo/KNOWN_VARIANTS`,
actual identity dispatch/public-contract tests,
`../TMP1x2/test/variants/test_variants.cpp` and
`../TUSB32x/include/TUSB32x/Config.h` (`Variant`, `DriverConfig`,
`DeviceProfile`, `validateProfile`) with actual validators/tests. Inspect
relevant CLI/runner/check registration and AGENTS. Record revisions, dirty
files, exact paths and adopted/adapted/rejected conventions; unfinished peer
code is comparison material rather than a support claim.

The lead owns shared contracts, source dispositions and integration.

## Concrete source checks

Start with [the immutable family
evidence](../reference/library-family.md#identity-and-a-generation-comparison):
combined TI Linux IDs `0x2000A114/0x2000A134` corroborate A BASIC/ENHANCED,
whereas its matching mask `0xFFFFFFF0` ignores revisions. TI Zephyr's NC
`0x2000A131` label conflicts with Linux NC `0x2000A110`; do not turn either
label into an ordering-code rule. Reopen actual source, source license and
SNLS783A table 8-23 before defining admission. Resolve reset domains from the
datasheet and specify complete indirect-selector phases with independent
literals. Record discrepancies and required regression cases in the evidence
ledger.

## Required original/A deltas

AI/AE share one cited map/procedure set with temperature-only suffix differences.
Seed and independently check these SNLS783A deltas:

- LEDCFG table 8-86 reset `0x5565`: LED3 [11:8], LED2 [7:4], LED1 [3:0]; preserve
  reserved [15:12]. LEDCFG2 table 8-90 bits 10/9/8 are LED3 polarity/value/enable.
- Mux `0x0303[2:0]` and `0x0306[2:0]` change R to R/W; `0x0304[2:0]` is RH/W,
  strap default, header `000Xh`; `0x0305[2:0]` is RH/W with strap default.
  IO_CFG1 bit 6 becomes reserved R; PHYCR[5] becomes reserved.
- SOR1[11] is Strap11, pin 19. FLD_CFG2 reset `0x2284` becomes `0x0288`,
  [15:6] `0x8A` becomes `0x0A`, threshold [5:0] 4 becomes 8. CR1[5]/CR3[10]
  are RH/W X, BASIC 0, ENHANCED strap. PHYSCR[1:0] are RH/W X, ENHANCED 0,
  BASIC 1; register reset `010Xh`.
- RXFCFG1 reset `108Xh` becomes `10X1h`; bit 7 is RH/W X, BASIC strap,
  ENHANCED 1; bit 0 table reset 1 with strap wording. FLDS RC becomes RCH,
  retaining consuming handling. FLD tables 8-5/8-6 add Strap11 and dependent
  BASIC mechanisms; section 8.3.17 requires MLT3, low SNR and descrambler with
  energy loss or RX error.
- EEE section 8.3.3.1 has 27 ordered rows versus 18: drop `04D3=4F12`; change
  `0416` from `1F30` to `1830` and `0466` from `FE00` to `FD00`; add
  `0126=5D23,0132=0183,04D4=3322,0310=76E1,0311=01FC,0313=06E3,033A=579B,
  033C=EC00,0317=1000` and a second `04F5=2864`. `0466` moves last after
  `0000=3300`. Preserve ordering and duplicate writes.

Assign identity, CR1, CR3, PHYSCR, FLD_CFG2, FLDS, FLD note and SOR1 to 58;
LEDCFG/2, mux, IO_CFG1, PHYCR[5], RXFCFG1 and EEE to 59. Require literal field,
unknown-default and complete ordered-recipe tests.

## Work and acceptance

Read exact SNLS783A and SDAA067A sections and tables, current TI A-generation
troubleshooting guidance, relevant manufacturer repositories/SDKs and
independent primary implementations. Pin inspected revisions, paths/sections,
licenses and inspection depth. Inventory every register, non-reserved field and
documented procedure, including recipe-only addresses and runtime opt-in
controls. Compare AI and AE individually with original I/E:
identity/modes/revisions, reset values and domains, resampled straps,
clocks/interfaces, startup/10BASE-Te, FLD, EMC, status/counters/interrupts,
power and diagnostics. Matching mnemonics or a rollover checklist alone cannot
prove complete shared behavior.

Original and A-generation share BASIC/ENHANCED model values 0x11/0x13;
documented revision defaults are 1 and 4. Define an evidence-backed
accepted-revision policy, keeping declared part and observed identity distinct.
Do not invent generation model IDs or infer I/E suffix. SNLS783A 8.3.13 repeats
`0x030E=0x4A40` for 10BASE-Te: determine shared applicability, reset and
ordering requirements rather than inventing a different procedure. Preserve
prohibited ANAR remote-fault bit 13 advertisement and supported remote-fault
observations.

Record Strap11 guidance **if FLD is used** and distinguish programmable FLD
from EMC qualification. SDAA067A table 3-3 option 5 says Strap8 HIGH while its
Figure 3-2 and SNLS783A table 8-5 say LOW; the datasheet governs. Software
writes only CR3 mask `0x040F`, preserving bits 6:5, and reports SOR1[11]. Do not
claim register-only EMC equivalence.

Assign every changed/inherited field and procedure to [58](58-a-core-link.md) or
[59](59-a-feature-closure.md), with source-backed applicability and independent
test requirements. Acceptance is an exhaustive reconciled comparison, explicit
source conflicts and usable implementation inputs, not support by declaration.
Update variants/reference/architecture guidance with the researched comparison.
Check ledger consistency and local references/links; verify any changed public
contract documentation. Record assigned implementation inputs and open source
issues, then complete COMMON's report and checkpoint.
