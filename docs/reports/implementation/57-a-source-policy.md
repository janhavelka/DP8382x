# Prompt 57 · DP83826AI and DP83826AE source and identity policy

## Phase (a) assessment and implementation brief

**IN PROGRESS.** The research can proceed from the original I/E ledger and
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
| 58, reset and startup | BMCR `00=8000` resets the PCS and retains vendor settings. PHYRCR `1F=8000` has pin-reset effects. Refresh identity, address and SOR after HARD. ENHANCED Strap1/6 latch only on POR. PHYRCR `1F=4000` restarts digital circuits and retains registers. Check both PHYRCR command bits before subsequent work. External POR-to-SMI 50 ms MAX and pin-reset-to-SMI 2 ms MAX establish readiness for their respective events. Pin reset needs a 25 µs minimum pulse. Anchor bus silence to observed completion or a conservative later time, with a separate finite software poll budget. |
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
