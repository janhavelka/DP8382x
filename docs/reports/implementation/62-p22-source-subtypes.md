# Prompt 62 · DP83822 subtype and feature evidence

## Phase (a) assessment and brief

**IN PROGRESS.** The research can proceed for all four ordering parts. Prompt 61's
open DP83825I interrupt, calibration and DSP questions stay with that part.
The worktree and index were clean on `main` at
`e8cf6277e245d99f3299b7c6ddf33d37934dfb16`. The wrapper verified that
`origin/main` matched, and the local tracking comparison was 0 ahead, 0 behind.
The existing four register JSON files have no procedure assigned to 62.
[Prompt 62](../../prompts/62-p22-source-subtypes.md) ·
[Prompt 61 disposition](61-p25-feature-closure.md#phase-d-final-disposition)

The frozen inputs are the application-declared part, separate MAC interface
and medium, one owner for selector transactions and destructive reads, and
the finite operation and restoration contracts. The private DP83822 row uses
`Variant::UNKNOWN`, model 0x24, revision mask 0x0001 and zero feature bits.
`findPart` excludes UNKNOWN. All four suffixes need separate admission cases
in later implementation. README and CHANGELOG describe shipped parts. This
phase adds a research brief and a progress entry.
[Part table](../../../src/PartTable.h) · [Identity](../../identity.md) ·
[Interface and clock](../../interface-clock.md) · [API contracts](../../api-contracts.md)

### Source inventory and subtype declarations

The lead counted the headings in the original archived SNLS505H (July 2026,
SHA-256 `d73d83c018e6dab3f935ea45ca1771bbc34c030b6b24de45fccd1dfa6998a2d3`).
Tables 8-1 through 8-142 span PDF pp. 53–102, including continuations.
They give **142 distinct printed addresses**. Of these, 30 are standard, 106
are vendor extended, four are MMD 3 and two are MMD 7. This is the starting
count for registers. The field count and the procedure count require separate
reconciliation in phase (b). In particular, a timing footnote on p. 14 names
a register absent from those 142 headings.
[Archived datasheet](../../reference/source/TI-DP83822-SNLS505H.pdf) ·
[Original payload and extract hashes](../../reference/source/provenance.json)

Two table gaps already affect the checker design. Table 8-12 skips CR3
000B[11], and table 8-37 ends at DSPCR1 0101[7], leaving [6:0] undescribed.
Mark those bits unlisted and exclude them from writable masks. Table 8-39
explicitly marks all of 0107 reserved. Preserve that distinct disposition
and retain its address in the inventory. Reassemble wrapped access cells
from the original columns, especially MISR and RXFS in table 8-88.

| Declared suffix | Ambient range | Copper | 100BASE-FX | Admission and physical evidence to keep separately |
| --- | --- | --- | --- | --- |
| DP83822I | −40 to +85 °C | Yes | No | I declaration and copper fixture |
| DP83822H | −40 to +125 °C | Yes | No | H declaration and copper fixture |
| DP83822IF | −40 to +85 °C | Yes | Yes | IF declaration, copper and identified optical fixture |
| DP83822HF | −40 to +125 °C | Yes | Yes | HF declaration, copper and identified optical fixture |

Section 4 and the ordering addendum identify the 32-pin RHB VQFN grades.
Tables 8-3/4 give the same two ID words, `2000 A240`, for each declaration
(OUI `080028`, model `24` hex, revision 0). Preserve the supplied suffix and
keep `DeviceIdentity.mode` UNKNOWN. The register observation cannot establish
temperature grade, board wiring or installed optics. Test all four declarations
against the same words, other revisions, wrong model/OUI, all-zero and
all-one IDs. Separate local rejection from a mismatch after bounded ID reads.
[SNLS505H, pp. 3, 55 and ordering addendum](../../reference/source/TI-DP83822-SNLS505H.pdf)

### Assignments and independent acceptance cases

The following assignments set the boundaries for phase (b). Its ledger must
give each individual field and recipe a named procedure and owner, including
fields in shared registers. The later prompts own the executable traces.

| Owner | Source and decisions phase (b) must record | Independent cases for the owner |
| --- | --- | --- |
| 63 · copper core | Identity, straps, management, reset, copper AN/forced modes, link, IRQs and counters. Section 7.4.2.1 permits standard register aliases through vendor DEVAD 31 except direct-only REGCR/ADDAR. It rejects function 10 while table 8-13 and §§7.4.2.6/.7 describe it. Use address mode 00 then data mode 01. Section 7.4.2.9 maps printed IEEE addresses by DEVAD, including 703D → 7/003D. | For 703D write 0D=0007, 0E=003D, 0D=4007, then read 0E. Check the vendor alias separately and deny indirect 0D/0E and post-increment. Compare each reset literal below, both collector reads and partial acquisition. |
| 64 · MAC interface and timing | MII, RMII and RGMII, clock role and mux ownership. Table 8-23 gives fixed 3.5 ns RGMII shifts with opposite polarities. Table 8-36 separates TXCPSR 0042 bit 4 (W1S) and bits 3:0 (MII phase value), while the enable description says 4:0. Sections 6.20/.21 footnote 2 qualify RGMII FAST_RXDV timing with 0009=0002 and 0457=0410. Inventory that exceptional word with its exact timing scope. | For RX/TX off/off, on/off, off/on, on/on expect 0800, 1800, 0000, 1000 under mask 1800. Test RMII software entry with 0462=4300 then 001F=4000, strap entry separately, and pin-owner conflicts. Record inferred restoration/reverse transition and unresolved follower clock behavior. Establish width before allowing any wider MII phase value. |
| 65 · fiber | Sections 4, 7.4.1.1/.2 and 7.4.6 establish IF/HF, MII/RMII and forced 100BASE-FX. Table 7-10 RX_ER mode 3 can encode FX+RGMII. The text leaves its applicability open. Distinguish CR2[14] medium, SOR1 signal-detect straps and 0465[0] polarity. Include far-end fault, LDCTRL[11:8] and fiber link refresh. | Require positioned mask/value 4000 for FX enable. Cross the SOR1[11:10] and [9:8] selections. Cover both duplex choices, I/H rejects and absence of copper advertisement, MDIX, TDR and 10BASE-Te in FX. For explicit refresh, compare SNLA437A §2.3.4 with tables 8-2/16/83, retain both BMSR reads and keep physical Q14 open. |
| 66 · diagnostics and electrical controls | Reconcile loopback, BIST, PRBS, quality, TDR, ALCD, analog, repeater and compliance recipes to their actual fields. SNLA253 §2.3 and tables 8-55..65 distinguish peak mapping, 8-bit location, 7-bit amplitude and polarity. CDCR Done resets to one. SNLA266B tables 5-3/4 distinguish the DP83822 analog loopback word 7108 from the DP83825/26 example. | Exercise manual start clearing **and** Done set, failure, invalid samples and location 33 hex (about 36.31 m under the guide's conversion). Preserve raw amplitude and the unresolved physical scale. Verify exact LDCTRL codes and fractional termination values from tables 8-68/80. |
| 67 · power, wake, pins and closure | IEEE power-down precedes deep entry 0428[2]. Include exit timing, EEE/LPI, LED/GPIO ownership, PTPPSEL 003E, PTPCFG 003F and the SFD sequence with exceptional 0456=000A. Tables 8-87..131 and SNLA261A table 2-4 cover Magic, Secure-ON and pattern filters. Keep RXFCFG1[6] semantics open. | Test pattern bytes 0 and 63, all four mask words (one ignores), offset default 12 and CRC gating. Ordinary writes use bit 11 zero, acknowledgement sets 0800 with owned configuration. Track RXFS[12] and MISR2[9]. Verify exceptional SFD/IPG and optional WoL effects independently. Reconcile all four subtype rows. |

For 63, distinguish five reset mechanisms. POR/RESET_N load straps and reset
all. PHYRCR 001F=8000 is pin-equivalent, with strap reload selected despite
table 8-1's narrower statement. PHYRCR 001F=4000 restarts digital logic and
retains registers. Interpreting “soft reset” in §7.4.4.2 as this literal is
inferred. BMCR[15] resets PCS while retaining vendor registers and straps.
The unspecified PCS membership requires rereading owned direct fields.
Printed 3000[15] addresses MMD 3 offset 0000 and resets MMD 3, MMD 7 and
PCS while retaining vendor registers. Record actual readiness and timing
qualifiers from §§6.6–6.8 separately from link acquisition. Seek TI
clarification or later identified fixture traces for disputed domains,
function 10, MII phase width and FX link behavior. The SDK's P4 interface
restrictions and the unmeasured FX+RGMII path each have their own disposition.
[Family source comparison](../../reference/library-family.md) ·
[Troubleshooting guide](../../reference/source/TI-DP83822-Troubleshooting-SNLA437A.pdf)

### Implementation comparisons

| Source inspected | Scope and consequence |
| --- | --- |
| TI RTOS, BSD-3-Clause, revision `94bf4343`, version and MAC predicates and `Dp83822_config` | The driver tests OUI/model/revision, admits MII/RMII/RGMII and only configures MDIX in its chip-specific configuration path. The Processor SDK distribution at `dd5febde` carries the same source blob. Count these as one implementation. TI's AM243x 12.01 SDK guide lists RMII integration. [RTOS source](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83822.c) · [SDK copy](https://github.com/TexasInstruments/Processor-SDK-TDA/blob/dd5febde39f8f32d053a1aa03651d1f5dd225f69/mcu_plus_sdk/source/board/ethphy/enet/rtos_drivers/src/dp83822.c) · [SDK guide](https://software-dl.ti.com/mcu-plus-sdk/esd/AM243X/latest/exports/docs/api_guide_am243x/enet_supported_phy_guide.html) |
| TI Linux and TI combined Linux, GPL-2.0, WoL, reset and FX configuration | The comparison pins expose Magic/Secure-ON. Their FX masked write supplies 1 for bit 14. The TI Linux update at `7bd96b21` retains that write. The two drivers also give different bits the software-reset label. Use the positioned mask and literal reset domains above, and reconcile the complete wake pattern engine. [Catalogue pin](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c) · [Updated source](https://github.com/TexasInstruments/ti-linux-kernel/blob/7bd96b217efb4bd1b17a8e053dacae92b0621179/drivers/net/phy/dp83822.c#L584-L585) · [Combined source](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/dp83822.c#L424-L425) |
| Oryx CycloneTCP, GPL-2.0-or-later, independent source | Its BMCR reset polls without a deadline, its link path uses PHYSTS, and its indirect helper preserves DEVAD in data mode 01. Use it as a comparison for wire order. The portable owner retains bounded waits and a single destructive-read collector. [Pinned source](https://github.com/Oryx-Embedded/CycloneTCP/blob/28b61194d167b9abc157fdcb62e0adb4c5b36db5/drivers/phy/dp83822_driver.c) |

### Sibling audit

The three sibling trees were clean on `main` tracking `origin/main`.
The review covered their own AGENTS files, implementations, examples,
runner and test registrations. Two read-only agents used the session's
high-reasoning model for the silicon comparison and the independent peer
audit. The lead owns schema, source-conflict decisions, docs and integration.

| Sibling and revision | Paths inspected and convention selected |
| --- | --- |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h` (VariantInfo/KNOWN_VARIANTS), `src/MB85RC.cpp` (selection and ID validation), `test/test_basic.cpp` (explicit, unknown and mismatched IDs), `examples/common/DiagnosticCore.h`, `tools/hil_runner.py` HIL-007 and `.github/workflows/ci.yml`. Adopt per-part capabilities and inverse cases. AUTO selection by product ID cannot resolve A240 suffixes. |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h`, `src/TMP1x2Configuration.cpp`, `test/variants/test_variants.cpp`, `CMakeLists.txt` registration `variants`, `examples/common/Tmp1x2CliDiagnostics.cpp` and `tools/hil_tmp1x2_runner.py`. X2SON's absent ALERT wire leaves threshold and interrupt controls available. Treat FX capability, SD wiring and optical evidence separately. |
| TUSB32x `6989c77fad3bc4651403942a4c9f7ca3940f859b` | `include/TUSB32x/Config.h` (Variant/DeviceProfile), `src/TUSB32x.cpp` (validateProfile), `src/TUSB32xProtocol.cpp`, `test/test_validators.cpp`, profile application/observation tests, `test/CMakeLists.txt`, `examples/common/Tusb32xCli.cpp` and `tools/tusb32x_hil_runner.py`. Adopt independent declared-profile cases and pure validation before I/O. Derive address policy and legal media from this PHY. |

### Phase (b) deliverables and verification

1. Create one family ledger at `docs/registers/dp83822.json`. Use stable
   procedure IDs with `owner.implementationStage` 63–67. Record the four
   declarations and their evidence separately. Account for every heading,
   field and legal encoding, including continuations, reset/strap defaults,
   mixed control/status, collector ownership and neutral write values.
   Attach an explicit decision and next evidence to each unresolved conflict.
2. Reconcile the register counts with the original PDF independently of the
   ledger. Walk feature prose, timing footnotes, strap tables, SNLA253,
   SNLA437A, SNLA266B, SNLA261A, the applicable EEE guidance, and SNLU179's
   interface/jumper description. Separate literal recipe words from addresses
   and keep opaque writes constrained to the exact documented procedure.
   Examine tool notes as fixture context. Record the source revision, license
   and actual review depth for each comparison. [Source index](../../reference/README.md)
3. Extend the existing `tools/verify_ledger.py` entry point with an exact-part
   verifier. Provide independently authored original-table and adverse cases in
   `test/test_p22_ledger.py`. Reject a missing address, field, owner or
   subtype, transposed wire namespace, inverted shift/mask polarity and
   an ordinary write to a reserved/recipe-only location. Report any missing
   PDF cells as source gaps. Keep the ordinary checker free of PDF dependencies.
4. Update the register guide, variants, family reference and architecture
   with the owner assignments and remaining gates. The lead reconciles shared
   contracts and the progress/report. Split register extraction from independent
   tests if delegating. Phase (c) reviews the actual ledger against original
   tables and the procedure/part mapping. Phase (d) reruns reference/link,
   ledger, Python and applicable source-document checks. Runtime subtype
   implementation belongs to 63–67. Remote CI belongs to 71.

### Local evidence

Windows x86-64, Python 3.12.10 and PyMuPDF 1.27.2.2. A read of each original
PDF page using `get_text(sort=True)` and deduplication by numbered
`Table 8-N. 0xADDR` heading produced 142 numbers and 142 addresses. The
section 6.20/.21 footnote and table 8-36 were checked separately in the PDF.

- Host research checks PASS. `python -B tools/verify_ledger.py` checked the
  existing I/E, A and DP83825I inventories. The discovery selection
  `python -B -m unittest discover -s test -p 'test_*ledger.py' -q` passed
  104/104. `python -B scripts/generate_version.py check` passed.
- Full Python discovery with `-p 'test_*.py' -q` passed 221 tests and
  skipped 59 fixture-dependent cases. `python -B tools/verify_references.py`
  verified 29 vendor artifacts, 27 extracts and 2,458 local links in 282
  Markdown files. Tracked diff and new-report whitespace checks passed.
- Compiled host, portable/exported consumer, SDK link, remote CI and physical
  evidence NOT RUN in phase (a). The next phase has all inputs needed to build
  the research ledger. Board measurements remain pending for each suffix.

## Phase (b) register and procedure reconciliation

**IN PROGRESS, implementation phase finished.** Entry was clean on `main`
at `213b1a44e5cee6820a1502fe059b9a8a2637603c`. The wrapper verified the
phase (a) push and the local comparison was 0 ahead, 0 behind. The lead
owned subtype declarations, procedure assignments, conflict decisions,
shared documentation and verifier integration. Two agents used the session's
high-reasoning model independently for table extraction and test oracles.
[Family ledger](../../registers/dp83822.json) and
[schema guide](../../registers/README.md#dp83822i-h-if-and-hf)

### Denominator and source limits

The original SNLS505H tables contain **576 printed field rows**, including
432 named fields and 144 reserved rows. Two additional ledger entries protect
the gaps at 000B[11] and 0101[6:0]. All 142 register partitions cover 16 bits.
Table 8-38 leaves TYPE blank for reserved 0106[11]. Six FUNCTION cells are
blank, at tables 8-3[15:0], 8-4[15:10], 8-34[7:4], 8-50[11:4],
8-70[15:5] and 8-71[13:0]. Where names or example settings supply some
meaning, the field record retains those limits. Whole-register reserved
rows occur at 0107, 0114, 0116 and 04D4–04D6.

Each named field resolves to one of 31 procedures. Their owner counts are
10 for copper core (63), two for interface/clock (64), three for fiber (65),
10 for diagnostics/electrical (66), and six for power, wake and pins (67).
The four part rows record temperature, package, accepted shared revision,
interfaces, medium restrictions, and separate pending execution and fixture
evidence. The 10 strap pins use their own four-level encodings. There are
21 named decisions with next evidence and 11 timing records. The figure 6-1
note requires a 100 µs minimum from stable external clock to reset release
when that clock was absent at rail ramp. Sections 6.7 and 6.8 give separate
1 µs clock conditions with different reset edges.
[Original tables and figures](../../reference/source/TI-DP83822-SNLS505H.pdf)

| Procedure outside the detailed table descriptions | Selected evidence and limit |
| --- | --- |
| RGMII FAST_RXDV timing | The 100BASE-TX footnotes pair CR1=0002 with vendor 0457=0410. TX latency 170 ns and RX latency 159 ns are typical values. Normal interface selection has no grant to write 0457. [SNLS505H §§6.20/.21](../../reference/source/TI-DP83822-SNLS505H.pdf) |
| Copper MSE and analog PRBS | The troubleshooting guide reads channel A at vendor 0218. Its analog example writes 030B=3380 between BISCR 0108 and 3108, then uses 001B=807D to capture the counter with that example's IPG. The exact register table gives the lock and lost-sync bits at 0016[11:10]. The guide's 0017 lock reference and 3B04 expected word remain disputed. [SNLA437A §§2.3.3, 2.5.2/.3](../../reference/source/TI-DP83822-Troubleshooting-SNLA437A.pdf) |
| PROFINET copper cable-break profile | SNLA438 table 3-3 describes the link settings. The linked TI study adds a separate 22-write program, beginning with hard reset and ending with digital restart. Step 21 optionally disables EEE. Writes include 0107=0605, 04D4=3322 and 003F=B40F. The ledger scopes every step to the entire example and assigns restoration to 66 with 63/67 coordination. TI's cable and break-machine results describe its fixture. [SNLA438](../../reference/source/TI-PROFINET-SNLA438.pdf) and [TI SNLA383, August 2023, §§2–3](https://www.ti.com/lit/an/snla383/snla383.pdf) |
| TDR and compliance | The TDR example gives 36.31 m for location 33 hex. The amplitude tables have seven-bit samples and leave physical scaling open. For 10BASE-Te compliance the DP83822 analog example uses 7108. SNLA266B table 5-5 proposes repetitive ones for harmonics while current SNLS505H table 8-32 prohibits that use. The harmonic measurement remains gated. [SNLA253 §2.3](../../reference/source/TI-DP83822-TDR-SNLA253.pdf) and [SNLA266B §5](../../reference/source/TI-DP8382x-Compliance-SNLA266B.pdf) |
| Wake, SFD and EEE | The ledger covers 64 pattern bytes, all four masks, offset, CRC, RXFS selector and 0800 level-clear action. SFD's 0456=000A write has its own exception. The DP83825I legacy-MAC LPI example uses a bit reserved on DP83822. The DP83822 MAC pin route belongs to 67. [SNLA261A](../../reference/source/TI-Wake-on-LAN-SNLA261A.pdf), [SNLA328](../../reference/source/TI-Energy-Efficient-Ethernet-SNLA328.pdf) and [SNLS505H §§7.3.1–3](../../reference/source/TI-DP83822-SNLS505H.pdf) |

The EVM guide identifies J6 for rail selection and J13/J14 for MAC interface
access. SPRACC8A describes an AMIC110 controller and how its PRU observes a
PHY. Both are fixture/platform context. The source catalogue above records
the phase (a) code comparison. Phase (b) reopened TI RTOS lines 120–237,
updated TI Linux lines 566–606, and CycloneTCP's MMD helpers, lines 340–end.
The RTOS MDIX helper returns void across multiple MDIO updates. Preserve
transfer failures in the planned core. The Linux FX path still supplies
unshifted 1 for the bit-14 mask. The independent MMD helpers preserve DEVAD
in both selector frames. The archived vendor bytes and licenses are intact.
Targeted searches for extra 0457 and ALCD guidance left the restoration and
conversion gates open. [EVM guide](../../reference/source/TI-DP83822EVM-SNLU179.pdf),
[AMIC110 example](../../reference/source/TI-MDIO-Configuration-SPRACC8A.pdf)
and [pinned implementation comparisons](#implementation-comparisons)

The three clean sibling revisions match the [phase (a) inventory](#sibling-audit).
The re-audit covered their listed headers, source, examples, runners, tests,
registrations and AGENTS. MB85RC supplies the per-part capability pattern,
TMP1x2 separates absent wiring from internal controls, and TUSB32x checks
profile incompatibility before bus access. The local repository has no CI
workflow in this execution phase. README and CHANGELOG describe shipped
driver behavior. This change adds source research and offline verification.

### Phase (b) local checks and handoff

On Windows x86-64 with Python 3.12.10 and PyMuPDF 1.27.2.2, the independent
PDF test compared the bounds, names, access types and reset cells of all 576
printed rows. Additional tests exercise mask polarities, collector selection,
electrical codes, chip declarations, timings and the ordered cable-break
trace. The ordinary verifier reads strict JSON using the standard library.
[Verifier](../../../tools/verify_p22_ledger.py) and
[original-table and mutation tests](../../../test/test_p22_ledger.py)

| Evidence layer | Result |
| --- | --- |
| Host research | PASS. `python -B tools/verify_ledger.py` checked all family ledgers. `python -B -m unittest discover -s test -p 'test_p22_ledger.py' -q` passed 13/13. Discovery with `test_*ledger.py` passed 117/117. Full `test_*.py` discovery ran 293, with 234 passed and 59 fixture-dependent skips. |
| Reference and source checks | PASS. `python -B tools/verify_references.py` verified 29 vendor artifacts, 27 extracts and 2,481 local links. `python -B tools/check_doxygen.py` verified 296 pages and rejected both negative fixtures. `python -B scripts/generate_version.py check` and tracked, staged and new-file whitespace checks passed. |
| Executing PHY host tests, portable and exported consumers, ESP-IDF links | NOT RUN in phase (b). This checkpoint changes research data, Python verification and documentation. |
| Remote CI and physical evidence | NOT RUN. Prompt 71 owns CI. Each declared grade needs an identified board; the two F grades also need optical evidence. |

Phase (c) should run two independent read-only reviewers in parallel on the
wrapper's actual checked commit. One should audit table cells, field semantics,
collector and neutral masks and subtypes against the original pages. The other
should trace feature prose, application-note sequences, omissions and
conflicts through the procedure owners and adverse tests. Both should
distinguish an assigned software obligation from an executing driver path.
