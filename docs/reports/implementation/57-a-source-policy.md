# Prompt 57 · DP83826AI and DP83826AE source and identity policy

**COMPLETE** at the research layer. The [final verification](#phase-d-final-verification)
records the source coverage, implementation assignments and remaining evidence gates.

## Phase (a) assessment and implementation brief

The research started from the original I/E ledger and
the declared-part profile verified by prompt 56. AI and AE share a datasheet
and observable identities. Their ordering grades specify different temperature
ranges, AI −40 to +85 °C and AE −40 to +105 °C. Retain the grade supplied
by the application throughout. The core and adapter currently admit I/E only.
[Prompt 56 verification](56-e-support.md#phase-d-final-verification) ·
[Current admission](../../identity.md)

On 2026-11-23 the index and worktree were clean on `main` at
`59cde6703c68114b8f8f6db21c9f151b17a8b8c2`. The wrapper verified upstream
equality and the local comparison was 0/0. No procedure in the original ledger
has implementation owner 57. Two read-only agents used the session's
high-reasoning model for the table comparison and the sibling identity audit.
The lead owns the policy, evidence dispositions, downstream assignments and
integration. First-execution remote CI belongs to prompt 71.

### Identity evidence and conflicts

SNLS783A tables 8-22/23 give ID1 `2000`, models `11` BASIC and `13` ENHANCED,
and revision `4` (hex). Use `2000 A114` and `2000 A134` as the positive
research vectors for **each** A declaration. Original I/E accept `2000 A111`
and `2000 A131` at revision 1. The revision masks in the private part table
are `0010` for the future A map and `0002` for the selectable original map.
An additional revision requires exact-part evidence and an explicit admission
decision. [A datasheet, pp. 57–58](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf) ·
[Original I/E datasheet, table 8-22](../../reference/source/TI-DP83826I-E-SNLS647I.pdf)

| Source reopened in phase (a) | Finding and consequence for the ledger |
| --- | --- |
| TI combined Linux, GPL-2.0, ID definitions, registration macros and MDIO table | Explicit A BASIC and ENHANCED IDs corroborate the A table. The matching mask `FFFFFFF0` discards the revision nibble. Its NC label is `2000A110`. Preserve the constants as observations and use the datasheet for revision admission. [Pinned file](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/dp83822.c) |
| TI combined Zephyr, per-file Apache-2.0, ID switch, configuration calls and both extended-access helpers | NC is `2000A131`, and the A switch case is `2000A134` only. VOD_CFG3 appears as a definition. Both helpers use `0x4000 & devad`, which yields zero at DEVAD 31. Test for this defect in both read and write wire vectors. [Pinned file](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c) |
| TI Ethernet RTOS, BSD-3-Clause, version constants and support predicates | Its original-chip path accepts OUI `080028`, either model, revision 1 and MII/RMII. Record this limit when comparing SDK code to the A part. [Pinned file](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) |
| Upstream Linux, GPL-2.0, ID constants, PHY registration and MDIO table | C and NC share the TI driver lineage. Its model matching provides no second independent measurement of the A revision or suffix. [Pinned file](https://github.com/torvalds/linux/blob/33e83ffe4c57132c73b7d3fb7919006c5296c496/drivers/net/phy/dp83822.c) |
| TI A troubleshooting guide SNLA491, August 2025, §2.2.6.1 table 2-4 | The ID2 cell prints `A1x6`, while the notes give `A111` and `A131`. Revisions 6 and 1 must both fail on each A declaration. The October 2025 exact-part PHYIDR2 table governs. [Archived guide, p. 8](../../reference/source/TI-DP83826A-Troubleshooting-SNLA491.pdf) · [TI product documentation](https://www.ti.com/product/DP83826AI) |

The primary archive hashes are recorded in [provenance](../../reference/source/provenance.json).
The lead checked the original PDF pages for identity, indirect access, reset,
FLD, VOD and the EEE sequence across its page break. The SDK web guide at
AM243x 12.01.00.33 returned a cache miss on direct opening. The RTOS source
above was readable. Phase (b) should trace actual SDK and independent
implementation paths for the relevant features and record inspection depth,
including where a project has no exact-A implementation.

### Reconciliation work for phase (b)

1. Build one cited A-generation evidence ledger in `docs/registers/dp83826a.json`,
   keyed to SNLS783A table 8-18 and §§8.5.1.1–79. Both datasheets have 79
   detailed entries at corresponding addresses. Reconcile each 16-bit
   partition and each non-reserved field against its A table and the original
   semantic ID. Record access, read/command effects, legal values, known reset
   value/mask, mode and strap dependencies. Allow an inherited row to refer to
   an original record by ID with an explicit A source and delta. Record
   removed fields and original-only recipes, plus A-only additions. The input
   ledger contains 97 registers (including 18 recipe entries), 283 fields,
   5 printed EEE addresses, 47 exceptional write actions, 52 procedures,
   35 features and 25 gates. These are starting counts. Feature prose,
   electrical notes, troubleshooting scripts and recipe-only offsets also
   contribute to the A denominator. [Original ledger](../../registers/dp83826i.json) ·
   [Evidence contract](../../prompts/EVIDENCE.md)
2. Compare every procedure and feature, including inherited and opt-in
   behavior, against A sources. Record applicability to AI and AE, evidence
   for each action, prerequisites, timing, destructive samples, failure
   boundaries, replay/cleanup and a concrete regression case. Keep C22,
   vendor MMD, aliases and printed EEE notation distinct. Recheck the original
   D2 address inference and D3 TDR selection on A sources. Give each field
   and procedure one implementation owner, 58 or 59, even in shared registers.
   Preserve unresolved cases with the exact missing authority and the
   operation that depends on it. The I/E deep-power, pin and electrical gates
   identify questions to check on A. [Original decisions](../../design-decisions.md) ·
   [Primary feature closure](36-primary-feature-closure.md)
3. Reconcile the two implementation owners and document their literal test inputs.

   | Owner | Decisions and independent observations required |
   | --- | --- |
   | 58 · identity, reset, startup, link, clock/interface and FLD | Exercise identical raw IDs on both A grades, original/A cross-generation rejection, all model/revision encodings, contradictory required mode, absence and failed second read. Preserve raw evidence and the existing two-frame identity boundary. Rejected local declarations use zero callbacks. For an observed mismatch, stop before a write. Compare AN, forced-mode robust crossover, MDIX, pause and both MII/RMII cases. Retain partner remote-fault observation and clear local ANAR bit 13 in every advertisement composition (table 8-24). |
   | 58 · reset domains and restored settings | BMCR[15] resets PCS and retains vendor registers (table 8-20). PHYRCR[15] has pin-reset effect, [14] restarts circuitry and retains registers (table 8-49). Reconcile §8.4.1's software-reload wording with that pin equivalence and the Strap1/6 POR-only exceptions. Record observed SOR values and any resampling ambiguity. External timing in §7.6 has a 25 µs minimum pulse, 2 ms maximum reset-to-SMI and 50 ms maximum POR-to-SMI. Preserve the nominal 0.2 ms leader-clock and 1,500 ms FLP qualifiers. Derive software polling and replay bounds separately. SNLA491 p. 20 calls BMCR[15] a hard reset in its debug script. Use the detailed register domains. |
   | 58 · VOD, selector and FLD vectors | VOD_CFG3 reset is `8400` (table 8-82). For 10BASE-Te, §8.3.13 and p. 15 require `030E=4A40`. Spell out a fresh addressed write as `W 0D:001F, W 0E:030E, W 0D:401F, W 0E:4A40`. For readback repeat frames 1–3, then `R 0E`, all under one bus owner per transaction. For each reset kind specify readback, timing, and ordering ahead of an owned analog override. Record CR1[5], CR3[10], PHYSCR[1:0], FLD_CFG2 `2284→0288`, consuming FLDS RC→RCH, SOR1[11]/pin 19, and the other PHYSTS/SOR access changes. |
   | 58 · FLD strap and EMC evidence | Tabulate SNLS783A tables 8-5/6 including BASIC Strap11 and MII eligibility. Preserve §8.3.17's dependency for MLT3/low SNR/descrambler with signal loss or RX error and its descrambler threshold. For ENHANCED option 5 use Strap8 LOW per table 8-5 and SDAA067A fig. 3-2. SDAA067A table 3-3 prints HIGH. Its Strap11 HIGH EMC guidance applies **if FLD is used**. Check `CR3` writes with owned mask `040F` and asymmetric sentinel bits 6:5. Read back SOR1[11]. Electrical equivalence requires identified measurements. [Rollover guide, §§2–3](../../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf) |
   | 59 · diagnostics, pins, wake and power | Transcribe LEDCFG reset `5565`, LED3/2/1 nibbles at 8/4/0 and reserved upper nibble. Check LEDCFG2[10:8], access on all four muxes, strap-loaded `0304/0305`, and newly reserved IO_CFG1[6] and PHYCR[5]. Preserve RXFCFG1 `10X1`, bit 7 mode dependence, bit 0 strap wording and W0C bit 11. Check BISCR[11,9,8], CDCR[1:0] and CLK_OUT_LED_STATUS[0] hardware-updated status. Reconcile the remaining interrupts, counters, loopback, BIST, TDR, electrical, power, WoL and optional operations. SNLA491 table 2-6 gives MSE at `0225`; SNLS783A table 8-73 defines `0218`. Keep the former an unresolved debug observation pending demonstrated addressing and conversion. |
   | 59 · EEE/LPI | Transcribe the 27 rows in §8.3.3.1, pp. 27–28, in printed address space; compare each row to the original 18 and derive the legal wire action separately. The repeated `04F5=2864` occupies rows 7 and 25. Row 26 is `0000=3300`; row 27 is `0466=FD00`. Check the omitted `04D3`, changed `0416`, nine added offsets, reset, waits and restoration. Preserve each write's position and applicability, including the printed `7:203C` at row 2. |

4. Extend the offline ledger checks and independent literal tests. Require a
   complete join across the A source inventory, original comparisons, assigned
   owners and test requirements. Include mutations for a missing inherited
   field, a reserved bit made writable, an unknown reset bit made known, an
   unassigned procedure, an omitted recipe-only offset and reordered/merged
   EEE writes. For example, a reset oracle for CR1[5] or CR3[10] must expect
   zero in BASIC and an observed strap value in ENHANCED. For PHYSCR[1:0]
   it must expect 3 in BASIC and 0 in ENHANCED. Preserve the X positions in
   the mode-independent reset masks. For field-access comparisons check
   PHYSTS[14] R→RH and PHYSTS[6] RC→RCH (table 8-35) alongside the FLDS
   collector. Keep original I/E invariants active. Update the variants,
   reference and architecture guides to point to the reconciled ledger. Phase
   (c) should independently compare both generations' original PDF tables and
   procedures with the actual phase (b) changes. Phase (d) owns final checks.
   [Prompt requirements](../../prompts/57-a-source-policy.md) ·
   [Open G8 dispositions](../promptset-audit-dispositions.md)

### Peer conventions and current entry points

The three read-only checkouts were clean on `main` tracking `origin/main`.
The audit covered each repository's AGENTS.md, CI registration and these
implementation paths.

| Peer and revision | Specific sources and convention selected |
| --- | --- |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h` VariantInfo/KNOWN_VARIANTS/findVariantByProductId, `src/MB85RC.cpp` _selectVariant/_validateActiveDeviceId, `test/test_basic.cpp`, `test/test_public_contracts.cpp`, `examples/common/DiagnosticCore.h`, `tools/hil_runner.py`, `platformio.ini`. Use immutable variant facts and explicit mismatch evidence. Its product-ID auto-selection requires uniquely identifying data. |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h`, `src/TMP1x2.cpp` probe, `test/variants/test_variants.cpp`, `examples/common/Tmp1x2CliDiagnostics.cpp`, `tools/hil_tmp1x2_runner.py`, `CMakeLists.txt` variants registration. Adapt its exhaustive address/declaration tests to PHY model and revision axes. Bind from the board declaration and acquire identity separately. |
| TUSB32x `6989c77fad3bc4651403942a4c9f7ca3940f859b` | `include/TUSB32x/Config.h` Variant/DriverConfig/DeviceProfile/validateProfile, `src/TUSB32x.cpp` validators, `src/TUSB32xOperations.cpp`, `test/test_validators.cpp`, `test/test_profile_application.cpp`, `examples/common/Tusb32xCli.cpp`, `tools/tusb32x_hil_plans.py`, `tools/tusb32x_hil_runner.py`, `test/CMakeLists.txt`. Adopt pure profile preflight and independent target/observation checks. A readable field alone grants no write permission. |

Here `src/PartTable.h` keeps the future A map unselectable. The current
`src/DP8382x.cpp` identity path and `test/identity.cpp` already distinguish
declaration, raw words and mismatch reason. The compiled CLI fixture and
runner independently identify the producer and expected part. Phase (b)
changes research and its tests. Firmware, CLI admission, packaging and public
headers first change with an implemented procedure. [CLI and runner](../../hil-runner.md)

### Phase (a) checks

Windows x86-64, Python 3.12 and the existing GNU 15.1.0 Release build.
Original-ledger validation passed for 97 registers, 52 procedures and 35
features. All 80 selected ledger unit tests and all 12 I/E identity CTests
passed. The reference archive and local links passed verification.

```powershell
python tools/verify_ledger.py
python -m unittest discover -s test -p 'test_*ledger.py' -q
ctest --test-dir build/p53-baseline -R '^(e_)?identity_(decode|policy|failures|lifecycle|ownership|scan)$' --output-on-failure --no-tests=error
python tools/verify_references.py
git diff --check
```

**Host PASS** for the existing I/E identity baseline. **A software execution,
portable/package consumers, SDK link, remote CI and physical evidence NOT RUN**
in phase (a). The exhaustive A comparison remains the deliverable for phase
(b). Physical A identity, strap resampling and EMC performance remain open.

## Phase (b) source reconciliation

The two comparison files assign every detailed field and procedure to 58 or
59 for both declared A grades. The field file records all 427 printed rows
in the 79 detailed register tables. The 286 non-reserved rows describe 285
semantic fields because BISCR describes its two-bit generator/checker mode
across two rows. Of the 283 original field IDs, 281 have A counterparts,
two became reserved, and four new A fields have no original counterpart.
Separate inventories cover 27 recipe-address dispositions (including the retired
`04D3`), five printed IEEE tuples, and the unresolved debug offset `0225`.
[Register comparison](../../registers/dp83826a.json).

The procedure file compares 52 procedures, 35 features, 25 original gates,
20 timings and 56 exceptional write-action records. Of the original 47 action
records, one has no A write permission, and nine new addresses contribute new
actions. The ordered recipe owns **both** writes of `04F5=2864` at positions
7 and 25. If row 27 fails, the row-26 AN restart may already have taken effect.
Fields with data-dependent defaults carry their printed reset cells alongside
the mask of bits predictable across boards. The newly documented OTP loading
on VOD_CFG1/2 makes their programmed values an observation for each device.
[Procedure comparison](../../registers/dp83826a-procedures.json).

### Implementation inputs

| Owner | Source decision and literal regression |
| --- | --- |
| 58, identity and link | Read C22 02 then 03. For each declared AI or AE accept `2000:A114` BASIC and `2000:A134` ENHANCED at revision 4 only. Match the required mode after checking OUI, model and revision. On a mismatch retain the raw identity and stop before a write. Assert the identical pair preserves each separately bound grade. In forced mode require robust crossover. ANAR[13] must remain clear in local advertisements. Partner bit 13 and the latched BMSR remote fault still count as observations. |
| 58, reset and startup | BMCR `00=8000` resets the PCS and retains vendor settings. PHYRCR `1F=8000` has pin-reset effects. Refresh identity, address and SOR after HARD. ENHANCED Strap1/6 latch only on POR. PHYRCR `1F=4000` resets all PHY circuitry except the registers (table 8-49). Check both PHYRCR command bits before subsequent work. External POR-to-SMI 50 ms MAX and pin-reset-to-SMI 2 ms MAX establish readiness for their respective events. Pin reset needs a 25 µs minimum pulse. Anchor bus silence to observed completion or a conservative later time, with a separate finite software poll budget. |
| 58, 10BASE-Te | Both parts use VOD_CFG3 at vendor MMD 31 offset `030E`, reset `8400`. After initialization's reset and identity checks write `0D:001F, 0E:030E, 0D:401F, 0E:4A40`. For verification repeat the first three writes and read `0E`, comparing the stable owned bits. Each four-frame transaction has one bus owner. Apply an admitted persistent analog override only after mandatory verification. Interrupt each transfer, including the final possibly accepted write. |
| 58, FLD and interface | Use the six ENHANCED and two BASIC rows in `fldStraps`. For ENHANCED option 5 the ordered strap values (7, 1, 8, 11) are (1, 0, 0, 1). MII and an eligible board role are prerequisites to enabling FLD. MLT3, low SNR or descrambler detection requires energy loss or RX error. Preserve CR3[6:5] when composing mask `040F`. Energy-only changes `042F→0021` and `044F→0041`. Report pin 19 at SOR1[11] and retain consumed FLDS even if a subsequent read fails. |
| 59, pin and diagnostic features | Check LED3/2/1 source masks `0F00/00F0/000F` on asymmetric inputs and preserve the reserved upper nibble. Known-reset masks exclude the strap-dependent low bits at `0304/0305`. Preserve PHYCR[5] and IO_CFG1[6]. IRQ and wake consume status through one collector. For RXFCFG1[11], write one during unrelated updates and zero for acknowledgement. Test the possible effect if the acknowledgement frame fails. |
| 59, recipe and opaque controls | Test all 27 EEE writes and every interrupted prefix against both declared grades. Resolve the printed `7:203C` only through the selected D2 tuple `7:003C`. The data selector is `4007`. The D3 six-segment transmit-A cable procedure uses the specific TDR application report linked from SNLS783A §8.3.16.1. Keep explicit waits, verification, cleanup and measured cable-delay requirements for this procedure. |

The Strap11 HIGH recommendation applies **if FLD is used**. SDAA067A table 3-3
prints Strap8 HIGH for option 5. Its figure 3-2 and SNLS783A table 8-5 give LOW.
SDAA067A §3.2 attributes the EMC improvement to a physical strap and an EVM
test. A successful CR3 readback proves the programmed mechanism bits.
Strap substitution for that electrical result requires identified measurements.
[Exact-part datasheet](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf) ·
[Rollover guide](../../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf).

### Additional implementation and troubleshooting evidence

| Source and inspection depth | Disposition |
| --- | --- |
| TI combined Linux and Zephyr, at the phase (a) pins above. Reopened the Linux ID definitions and Zephyr selector helpers and configuration paths. | Linux NC `A110`, Zephyr NC `A131` and a revision-insensitive Linux match cannot distinguish suffixes. In both Zephyr helpers the data function collapses to zero. Use the source-backed wire literals above on read and write. |
| TI MCU+ AM263Px, BSD-3-Clause, Enet PHY version constants. Independently inspected the ICSS-EMAC E driver's CR2 RMW paths at lines 475–515 and their file notice. | The Enet constants select original revision 1. The ICSS helper named for enabling odd-nibble detection sets the disable bit. On A, CR2[1]=1 disables detection. Use table 8-30, with BASIC default 1 and ENHANCED strap-loaded. [Enet source](https://github.com/TexasInstruments/mcu_plus_sdk_am263px/blob/dee0b767c4ebaa9b19d12b145af14046e3eee0a3/source/board/ethphy/enet/rtos_drivers/src/dp83826.c) · [ICSS source](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.c) |
| Oryx CycloneTCP Open v2.6.6, GPL-2.0-or-later, source blob `dbb6b2a5fe82ec78f655077d49802fbf045197d9`. Inspected init, reset polling, debug dump and both four-frame MMD helpers. | The helpers compose the data function with the DEVAD. The reset wait has no finite poll bound and the debug dump can read consuming registers. Use the table-defined selectors, bounded polls and dedicated collector. The general DP83826 driver supplies an independent implementation comparison. It supplies no A revision admission or A-grade qualification. [Pinned release source](https://github.com/Oryx-Embedded/CycloneTCP/blob/v2.6.6/drivers/phy/dp83826_driver.c) |
| TI SNLA491, August 2025, §§2.2.6, 2.3.3, 2.5. Reopened A troubleshooting ID/MDIO tables and loopback scripts. | The guide corroborates the four-frame vendor transactions. Use the October 2025 datasheet for revision 4, reset domains and legal amplitude settings. Its MSE register is `0218`. The guide's `0225` and dB conversion need separate authority. Analog BIST example `3108` with lock and generator active gives `3B08`, while the guide's `3B04` selects digital loopback. [Troubleshooting guide](../../reference/source/TI-DP83826A-Troubleshooting-SNLA491.pdf) |
| TI SNLA488, July 2025, §3.2.1 and table 3-3, application example and working register dump. | `0019=8020` sets the subsequently documented reserved PHYCR[5]. The settings `0460=0005` and `000B=0001` would overwrite unrelated LED and mirror settings. Use field-level composition under the owning procedure. The working-register example reports `A11X`, leaving the revision unspecified. [TI EtherCAT application note](https://www.ti.com/lit/pdf/snla488) |

SNLS783A tables 8-75/76 give a usable PLL/recovered clock selection on pin 30
but leave pin 31's clock selection in conflict with its mux encodings. Keep the
pin 31 input/clock requests gated where no legal code is established. Similar
source gates remain for reversible deep power, 100 Mbit/s compliance stimulus,
MSE-to-dB scaling, the physical D2 mapping and TDR calibration. Each procedure
record identifies the inhibited action and the evidence needed to qualify it.

The lead integrated the two disjoint source inventories and wrote the verifier,
literal tests and guides. Both implementers used the session's high-reasoning
model for cross-table and procedure reconciliation. Phase (b) entered on clean
`main` at `bcb27e1a9b21ef9d6222ca4ec40b588e5e905ba6`, wrapper-verified equal
to `origin/main`. The three peer checkouts remained clean at the phase (a)
revisions. Reopening the listed declarations, dispatch, CLI, runners, tests and
rules confirmed explicit board selection, pure preflight and retained mismatch
evidence. No procedure has original implementation owner 57. The README,
changelog and shipped public declarations still describe the I/E runtime.

### Phase (b) local checks

Windows x86-64, Python 3.12.10 and existing GNU 15.1.0 Release/Ninja host
binaries. The verifier matched every detailed A table's address, header reset,
field masks, access and reset cells, and reserved rows against the archived
SNLS783A extract. Independent checks against the two original PDFs covered
the 18/27 EEE rows, all eight A FLD strap configurations and the 48 tuning
words of the selected TDR recipe. The unit tests rejected seven deliberately
corrupted comparison files in memory, including an invented known reset,
an omitted offset and a merged or reordered EEE write.

```powershell
python tools/verify_ledger.py
python -m unittest discover -s test -p 'test_*ledger.py' -q
python -m unittest discover -s test -p 'test_*.py' -q
ctest --test-dir build/p53-baseline -R '^(e_)?identity_(decode|policy|failures|lifecycle|ownership|scan)$' --output-on-failure --no-tests=error
ctest --test-dir build/p56d-consumer --output-on-failure --no-tests=error
python tools/check_doxygen.py
python tools/verify_references.py
python scripts/generate_version.py check
git diff --check
```

| Evidence | Result |
| --- | --- |
| Host research | PASS. Both ledgers passed validation. All 89 ledger tests passed. Full Python discovery ran 255 tests with 52 compiled-fixture skips. The nine A tests include both selector transactions, mode-specific unknowns, moved fields, FLD straps and the complete EEE write order. |
| Host driver | PASS. All 12 selected I/E identity CTests passed. A driver execution remains pending implementation. |
| Portable consumer | PASS. Both tests-OFF consumer CTests passed using the existing Release binaries. |
| Documentation and archive | PASS. Doxygen generated 294 pages and rejected both negative fixtures. Reference verification checked 29 vendor payloads and 27 extracts. Version, local links and tracked/new-file whitespace passed. |
| Exported package consumer / SDK link | NOT RUN in this phase. The changed inputs are research records, documentation and offline checks. |
| Remote CI / physical | NOT RUN. Prompt 71 owns remote CI. Each A grade requires its own authorized fixture and identity, reset, strap and electrical evidence. |

Prompt 57 remains **IN PROGRESS**. Phase (c) reviews the actual phase (b)
checkpoint against SNLS783A, SNLS647I and the cited procedure sources. The
wrapper records the commit and verifies synchronization after this run.

### Phase (c) review of EEE frame accounting

Two independent read-only reviewers examined the phase (b) change from
`bcb27e1` to `ac8e3d9` on clean `main`, verified equal to `origin/main`.
They used the session's high-reasoning model to check the reset tables and
EEE procedure against the archived silicon evidence in parallel.

The I/E timeout inherited by `eee_initialize` specified 66 recipe frames
and 74 with self-checks. For AI and AE, the 27 rows include two direct writes
and 25 four-frame MMD writes, totaling 102 frames. The capability read and
advertisement readback bring the minimum to **110**. Admission must also
cover polls, status checks and persistent replay. The A procedure now has its
own frame counts and timeout contract. A failed row-26 write can leave the
AN restart uncertain. A later row-27 failure retains the accepted row-26
command and the outcome of the final write. [SNLS783A §§8.3.3.1 and 8.3.11.1](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)

Host research PASS on Windows x86-64 with Python 3.12. The default ledger
verifier passed and all 90 ledger tests passed, including rejection of the
original 66/74 counts on A. The phase (b) PDF comparison reproduced all 18
original rows and 27 A rows. Portable/package consumer, SDK link, remote CI
and physical evidence were NOT RUN in this slice.

```powershell
python -B tools/verify_ledger.py
python -B -m unittest discover -s test -p 'test_*ledger.py' -q
```

Phase (c) remains open. In memory, the field reviewer changed the VOD_CFG2
known reset mask to `FFFF` and reset value to `0410`. The validator accepted
both despite the OTP annotations in table 8-81. The committed zero mask is
correct. The next slice should check OTP, strap and mode-dependent reset
knowledge, including bits common to both PHYIDR2 models, then continue the
remaining field and procedure review.

### Phase (c) review of reset masks

The wrapper verified clean `main` at `a58e85d35f8c03d2387a8f945671babff0bdd83e`,
equal to `origin/main`. Two read-only reviewers used the session's high-reasoning
model in parallel. One checked TI's reset cells and their mode, strap and OTP
qualifications. The other tested what changes the validator would accept.
The three sibling checkouts remained clean at the phase (a) revisions.

The verifier now requires the exact board-independent mask and masked value
for every detailed register. It reads the six OTP flags directly from tables
8-80/81 and checks each field annotation. The expected bits also account for
the two PHYIDR2 models, CR3 fast-drop straps, LED2 mux, both strap observation
registers and RXFCFG1 magic-packet enable. For PHYIDR2, only bit 5 differs
between `A114` and `A134`, giving mask/value `FFDF/A114`. Recipe-only
addresses retain null reset data.

Two records needed corrections for usable readback. Table 8-35 gives PHYSTS
the reset word `0002`. With negotiation disabled, speed and duplex depend
on straps in BASIC and both status bits read one in ENHANCED. Its independent
mask/value is `FFF9/0000`. Table 8-79 also prints `0002` for CLK_OUT_LED_STATUS.
Bit 0 reports the ENHANCED pin 31 selection made by POR-latched Strap6 in
table 8-10. Its independent mask/value is `FFFE/0002`. Both ledger records
retain the published header and field reset cells.
[SNLS783A, tables 8-10, 8-35 and 8-79](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)

Host research PASS on Windows x86-64 with Python 3.12.10. Against the entry
checkpoint the new tests reported 20 failing subcases. They covered 16 mask
mutations, a missing OTP annotation, an invented recipe reset and the two
incorrect readback masks. After the fix the ledger verifier passed and all
93 ledger tests passed, using the two commands in the preceding slice.
Host driver, portable/package consumer, SDK link, remote CI and physical
evidence were NOT RUN in this research slice. Phase (c) continues with
the field ownership joins and remaining procedure/source comparisons.

### Phase (c) review of field mappings and owners

The wrapper verified clean `main` at `fc7ce95d39c74d71521bc63aa601f1a615fef8e3`.
Two independent read-only reviewers used the session's high-reasoning model.
One compared the field mappings to both TI tables. The other tried to break
the mappings and checked each inherited field against its feature's stage.
The three sibling checkouts were clean at the revisions recorded in phase (a).

Of the 283 original fields, 281 have an A counterpart. Three LEDCFG source
nibbles move down four bits. The two removed fields are IO_CFG1[6] and
PHYCR[5]. The four additions are SOR1[11] and LEDCFG2[10:8]. The verifier
now checks all 285 A field identities, masks and stage assignments. Each
removal must identify the original register and mask and the specific A
table in which those bits became reserved. The old verifier accepted a
zero mask, a different reserved bit, duplicate removals and renamed additions.
[Original tables 8-43/73/85/87/89](../../reference/source/TI-DP83826I-E-SNLS647I.pdf)
and [A tables 8-44/74/86/88/90](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)

Three fields now have the same stage owner as their procedure. Prompt 58
consumes GENCFG[3] in `compatibility_configure`, selects the pin 30 clock
through 0303[5:3] and observes 0308[0] in `clock_configure`. The pin 31
clock-source selection remains gated under 59. Prompt 57 explicitly assigns
mux and IO_CFG1 closure to 59. Prompt 58 still tests the specific mux setting
used for the pin 30 clock and the IO_CFG1[8] path used by RMII. Both operations
need one writer through the application owner. [A procedure assignments](../../registers/dp83826a-procedures.json)

FLDS[8:4] changes from RC to RCH and still consumes the five cause bits on
read. The defined RXFS event mask is 00E1 in both generations. Bits 4:1
are reserved RCH and bit 12 selects the interrupt source. RXFCFG1[0] now
has an A validity override for its printed reset of 1h and strap-dependent
default. [SNLS783A tables 8-34 and 8-91/92](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)

Host research PASS on Windows x86-64 with Python 3.12.10. The two local
ledger commands recorded above passed, including all 95 ledger tests.
Host driver, portable/package consumer, SDK link, remote CI and physical
evidence were NOT RUN in this slice. Phase (c) continues with procedure
ownership, inherited effects and source gates.

### Phase (c) procedure review and fixes

Phase (c) finished on clean-entry `main` at
`40c05e6747a5ced7a84277b0515409ec746e9b4a`, verified equal to
`origin/main` by the wrapper. Two parallel read-only reviewers used the
session's high-reasoning model for separate silicon questions. One checked
FLD, reset, power and the D2/D3 address and recipe decisions. The other
checked interfaces, pin owners, diagnostic scripts, events, wake and EEE.
The lead reviewed the cumulative diff from `bcb27e1`, checked the source
locations and integrated the corrections. The current MB85RC, TMP1x2 and
TUSB32x checkouts were clean at the revisions in the phase (a) peer table.
Their variant dispatch, validators, CLI, runners, tests and registration
continue to support board-declared identity and bounded preflight. The
pinned TI Linux ID definitions and the Zephyr definitions, license and two
selector helpers were reopened at the revisions linked above.

| Implementation input | Review result |
| --- | --- |
| FLD and reset, prompt 58 | SNLS783A §8.3.17 recommends disabling energy detection for EtherCAT or expected baseline wander. ENHANCED option 4 enables RX error and MLT3 (`000C`). Option 5 enables energy alone (`0001`), with Strap8 LOW. SDAA067A recommends Strap11 HIGH if FLD is used for its EMC result. The application selects its profile. A CR3 update owns only `040F`. Reuse a retained SOR1 from the current reset epoch or budget four frames and one callback for a fresh read. Table 8-49 defines PHYRCR[14] as resetting all PHY circuitry except the registers. Reacquire live state. [A datasheet, §§8.3.17, 8.4.1 and table 8-49](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf) and [rollover guide, §3](../../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf). |
| Interface and clock, prompts 58–59 | SNLA491's Strap8 shortcut misses ENHANCED Strap1 and the BASIC Strap10/4 pair. Keep the observed strap levels, SOR2 mode/role and current RCSR separate. The RMII owner writes IO_CFG1[8] at 0302. The clock owner writes pin 30 source and mux together at 0303. The I/E interface planner could merge both functions at 0302. With current prerequisites, owned isolation and the pin 30 PLL, the A example costs 44 frames in 17 callbacks. Its read-only pass costs 19 in nine. An additional ENHANCED pin 31 status read costs four in one and supplies evidence only for pin 31. LED, pin, IRQ and wake budget their actual route and collector work. [Original planner](../../../src/DP8382xInterface.cpp), [SNLS783A, figure 8-13 and tables 8-12/13, 8-74–79, 8-89](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf) and [SNLA491, pp. 13, 15](../../reference/source/TI-DP83826A-Troubleshooting-SNLA491.pdf). |
| Link and pin evidence | The I/E link procedure reads BMSR first. Table 8-35 specifies that this clears PHYSTS[6]. The A contract reads PHYSTS before BMSR in both batches and before ANER in the first. The ten-frame budget still covers all reads. Pin 31 software selection of mux 5/6 remains gated by the conflict between figure 8-10 and table 8-76. Table 8-90 prints strap-dependent polarity fields. Section 8.3.18 fixes effective BASIC LED polarity active low and BASIC exposes LED0/1. The field records now carry those conditions. [A datasheet](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf). |
| Diagnostics, prompt 59 | SNLA491 p. 18's reverse script prints `0110`. Its low five bits select reverse, as p. 19 confirms. BISCR[8] is hardware-updated status. Page 19 also asks for fresh checker lock after `5000`, which selects one constant packet with the checker disabled. One second of dwell cannot establish lock for that test. For the analog example, adding lock and active status to `3108` gives `3B08`. Tests cover both decodes. [Troubleshooting guide, pp. 18–20](../../reference/source/TI-DP83826A-Troubleshooting-SNLA491.pdf) and [SNLS783A, table 8-41](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf). |

All 52 procedure-to-feature assignments, 35 feature gate sets, 25 gate
owners and 20 timing values, units and qualifiers were checked against the
original comparison and assigned stages. The validator checks exact owned
masks, including the five procedures whose pin or LED masks changed.
The 56 exceptional actions retain their inherited limits or their exact
A recipe positions. D2 retains its five source-implied tuples and
whole-word capability/advertisement checks. D3 uses the linked six-run
transmit-A recipe. Physical mapping, TDR calibration and fresh-capture
evidence remain open. Reversible deep-power entry still lacks safe 0428
composition and exit evidence. [A procedures](../../registers/dp83826a-procedures.json),
[original map](../../registers/dp83826i.json) and [TDR application report](../../reference/source/TI-DP83826-TDR-SNLA428.pdf).

**Host research PASS**, Windows x86-64, Python 3.12.10. The new mutation
test initially failed all ten subcases against the entry validator. After
the fixes, `python -B tools/verify_ledger.py` passed both ledgers and
`python -B -m unittest discover -s test -p 'test_*ledger.py' -q` passed
100 tests. Full offline discovery with `-p 'test_*.py'` ran 266 tests,
214 passed and 52 skipped. Calling `parse()` from
`build/p57-registers/parse_tables.py` on each archived PDF reproduced the
79 tables and 427 rows per generation. Running
`python -B build/p57-procedures/check.py` matched both EEE sequences
(18 and 27 rows), eight A FLD vectors, all 48 SNLA428 tuning words and the
exact-A PDF link to that report.
`python -B tools/verify_references.py` passed for 29 vendor artifacts,
27 extracts and 2,349 local links. `git diff --check` passed.

Host driver execution, portable/package consumer, SDK link, remote CI and
physical evidence were **NOT RUN** in this review. This stage changes the
research ledgers, verifier, tests and related documentation. Phase (d) owns
final checks and the prompt 57 completion decision.

## Phase (d) final verification

On 2026-11-24 the wrapper verified `main` equal to `origin/main` at
`0e2850fd80a9b8e33a2e73a184e58ce54f8eb275`. The index and worktree were
clean. The checked changes since prompt 56 comprise the research ledgers,
verifier, tests, guides and progress record. Two agents used the session's
high-reasoning model to check the silicon table interpretation and the
host build and test coverage independently. The lead checked the
research suites, documentation and completion records.

### Source coverage and implementation assignments

The PDF comparison used the bit, field, access, reset and OTP columns in
SNLS647I and SNLS783A. Each has 79 detailed tables and 427 printed rows.
The comparison accounted for 281 inherited fields, four additions and two
removals. All 285 A fields have an owner. The usable reset masks exclude
strap, mode and OTP uncertainty, including all six OTP-loaded fields.
The recipe-address inventory accounts for 27 entries, including the retired
original-only offset `04D3`. A second check compared 18 original and 27 A
EEE rows, eight A FLD vectors and 48 tuning words from the linked TDR report.
[Field comparison](../../registers/dp83826a.json) and
[procedure comparison](../../registers/dp83826a-procedures.json).

| Implementation owner | Fields | Procedures | Features | Gates | Timings | Exceptional action records |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 58, identity, reset and link | 138 | 21 | 17 | 7 | 15 | 0 |
| 59, diagnostic and optional controls | 147 | 31 | 18 | 18 | 5 | 56 |

The application declares AI or AE. Both declarations use revision 4 and the
same BASIC and ENHANCED ID pairs. The README, changelog, public API and CLI
still describe the shipped I/E runtime. The three peer checkouts were clean
at the revisions recorded in phase (a). Their dispatch, validators, CLI,
runners and test registrations support retaining that declaration and testing
the observed identity independently.
[Peer paths and revisions](#peer-conventions-and-current-entry-points).

### Local evidence

The research and documentation checks ran on Windows x86-64 with Python
3.12.10.

```powershell
python -B tools/verify_ledger.py
python -B -m unittest discover -s test -p 'test_*ledger.py' -q
python -B -m unittest discover -s test -p 'test_*.py' -q
python -B tools/check_doxygen.py
python -B scripts/generate_version.py check
python -B tools/verify_references.py
git diff --check
```

- **Host research PASS.** Both ledgers passed. All 100 ledger tests passed.
  Full Python discovery ran 266 tests, with 214 passes and 52 skips for
  cases registered in the compiled host build. The additional local PDF
  comparisons ran with `python -B build/p57-final-sources/crosscheck.py`
  and `python -B build/p57-procedures/check.py`. These scratch scripts
  used the archived PDFs directly.
- **Documentation PASS.** Doxygen checked 294 generated pages and rejected
  both negative fixtures. Reference verification checked 29 vendor artifacts,
  27 text extracts and the local file links. The version and diff checks passed.

The host Release build used GNU 15.1.0, CMake 4.0.1 and Ninja 1.12.1.
The pinned host tests checked the SDK sources against ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`.

```powershell
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'safe.directory'
$env:GIT_CONFIG_VALUE_0 = 'C:/esp/v6.1/esp-idf'
cmake -S . -B build/p53-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p53-baseline --parallel 5
ctest --test-dir build/p53-baseline -E '^cli_hil_' --no-tests=error --output-on-failure -j 4
ctest --test-dir build/p53-baseline -R '^cli_hil_' --no-tests=error --output-on-failure -j 1
cmake -S test/consumer -B build/p57d-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/p57d-consumer --parallel 3
ctest --test-dir build/p57d-consumer --no-tests=error --output-on-failure
```

- **Release host PASS.** All 697 CTests passed on the first attempt. The
  691 non-HIL registrations took 4.31 s. The six offline CLI fixture and
  simulated serial registrations took 175.74 s. These exercise the shipped
  I/E paths. AI/AE driver execution remains assigned to prompts 58 and 59.
- **Portable consumer PASS.** Both registrations passed in a fresh tests-OFF
  build of the standalone consumer.
- **Exported package consumer and SDK image link NOT RUN.** Prompt 70 owns
  the installed package check. This research changed no firmware or build
  inputs. **Remote CI NOT RUN.** Prompt 71 owns first-execution CI.
- **Physical evidence NOT RUN.** Each A grade still requires an authorized
  fixture with identified silicon, mode and straps. D2 address mapping, D3
  TDR calibration and capture freshness, and FLD electrical behavior retain
  their measurement gates. The ledger also records the missing authority for
  reversible deep power, pin 31 mux encodings, 100 Mbit/s compliance stimulus
  and MSE scaling.
