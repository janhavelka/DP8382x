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
