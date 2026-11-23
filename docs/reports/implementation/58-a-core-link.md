# Prompt 58 · DP83826AI/AE identity, reset and link

**IN PROGRESS. Phase (a) assessment complete.** The application can currently
select I or E. Phase (b) can implement the A core procedures from the
verified source record. AI and AE need separate software test rows and
separate identified fixtures. [Prompt requirements](../../prompts/58-a-core-link.md)

## Phase (a) assessment

On 2026-11-24 the index and worktree were clean on `main`. The wrapper
verified the prompt 57 completion commit
`8abeeb763fdbd8f2afbda06604b1c42bd9324cf0` equal to `origin/main`
at 00:22 Prague time. The local comparison was 0/0. Two read-only agents
used the session's high-reasoning model in parallel for silicon sequences
and for sibling conventions and consumer admission. The lead checked the
ledger against the current entry points and owns public selection, reset and
timing contracts, metadata, integration and the checkpoint request.

The A ledger assigns 138 fields, 21 procedures, 17 features, seven gates and
15 timing records to this prompt. The remaining 31 procedures and all 56
exceptional write-action records belong to prompt 59. Eligibility for a
shared register is determined per field and operation. The full assignments
and TI document provenance are in the [field ledger](../../registers/dp83826a.json),
[procedure ledger](../../registers/dp83826a-procedures.json) and
[verified prerequisite](57-a-source-policy.md#phase-d-final-verification).

| Owner in the existing core | Assigned procedure IDs |
| --- | --- |
| Declaration, register policy and access | `identify`, `direct_access`, `mmd_access` |
| Initialization and reset | `pcs_reset`, `hard_reset`, `restart`, `external_reset`, `initialize_10base` |
| Link setup and observation | `configure_an`, `configure_forced`, `next_page_exchange`, `extended_link_configure`, `observe_link` |
| Quality controls | `configure_fast_drop`, `link_recovery_configure` |
| Interface and clock | `mii_configure`, `rmii_configure`, `clock_configure`, `repeater_configure`, `compatibility_configure`, `isolation_configure` |

### Implementation order and acceptance

1. **Bind the declared part and bound its permissions together.** Append AI/AE
   to `Variant`, preserving existing values. Update `isValidVariant` in
   `include/DP8382x/Config.h` and the private profiles in `src/PartTable.h`.
   Use revision mask `0010` for both A grades. C22 reads 02 then 03 must give
   `2000:A114` BASIC or `2000:A134` ENHANCED. Retain the declared grade in
   identity, capabilities context and caller-visible records. Test each grade,
   all 16 revision nibbles, both required modes and unrestricted mode, wrong
   OUI/model/generation, zero/ones and failure on the second read. Update
   `test/identity.cpp`'s invalid cast of 3 when that value becomes selectable.
   Invalid declarations/settings reject before clock or MDIO callbacks. An
   observed mismatch retains the actual prefix and stops the operation at
   identity, before selector or target writes. See SNLS783A tables 8-22/23.
2. **Apply the A policy at every executable entry.** `registerPolicy` and
   `readableMmd` in `src/DP8382x.cpp` currently handle only the original map.
   Share proven access rows, preserve consuming and follow-up classifications
   and select write permissions by generation. Test direct holes 0C/1D, both
   selectors, aliases, removed recipe offset 04D3, raw maintenance and
   source-backed stable reads including DEVAD 7 offset 003C with selector
   4007. A completed identity currently suffices to pass generic feature checks
   such as `startConfigureEee` after initialization. Before exposing AI/AE,
   check typed, raw, CLI, native and replay paths for access to prompt 59 work.
   Fence unimplemented A operations before I/O, including mixed quality
   profiles requesting TX settings. Retain the narrow collector work required
   by link, RCSR and FLDS. Keep original I/E regression rows executing.
3. **Trace startup, reset and restoration.** Reuse the identity, SOR1/SOR2,
   command, wait and VOD steps in `src/DP8382x.cpp`. PCS writes C22 `00=8000`
   and retains vendor words. HARD writes `1F=8000`, invalidates device
   knowledge and requires fresh identity at the declared address and new SOR
   evidence. ENHANCED Strap1/6 are POR-only. RESTART writes `1F=4000` and
   resets the circuitry while retaining register settings. Reacquire live
   evidence. Poll BMCR[15] for PCS and PHYRCR mask C000 for the other two.
   Test an independently active command, uncertain writes and address/mode
   changes. For an attempted HARD command preserve shared bus silence through
   the completion upper bound plus max(2 ms, poll interval). Software command
   completion has its own finite deadline. SNLS783A §7.6 gives a 25 µs minimum
   pin pulse, 2 ms reset-to-SMI and 50 ms POR-to-SMI maxima. Check the
   late-clock 100 µs condition at the board owner. RMII leader clock at 0.2 ms
   and FLP at 1500 ms are nominal. See §§7.6, 8.4.1 and tables 8-20/49.

   Every initialization writes vendor MMD 31 offset 030E after identity and
   reset checks. The word resets to 8400 on POR/HARD. Write
   `W0D=001F W0E=030E W0D=401F W0E=4A40`, then repeat the three selectors
   and read 0E. Each four-frame transaction holds its own controller lock.
   Verify mask/value FFC0/4A40 before any admitted persistent analog override.
   Preserve this order for prompt 59. Check the no-extra-owner baseline of
   20 frames for NONE and 32 for a commanded initialization when the first
   completion sample clears. Count any additional board/profile prerequisites
   explicitly. Distinguish a preserving restart from a requested initialization.
   See §7.5 note 2, §8.3.13 and table 8-82.
4. **Share link configuration and change the A acquisition order.** In
   `src/DP8382xConfiguration.cpp`, trace transitions into and out of forced
   operation with CR1[5] robust throughout the forced interval. Its reset
   is zero in BASIC and strap-dependent in ENHANCED. Preserve PHYCR[5] and
   the CR3 mirror pair. Exercise 10/100 × half/full, MDIX, parallel detect,
   extended-duplex applicability and all 16 pause combinations. Assert local
   ANAR[13] clear, including next-page setup, cleanup and replay. Retain
   partner fault and latched BMSR fault separately. Exercise D8 toggle/ack,
   null pages and interrupted page evidence.

   `src/DP8382xLink.cpp::readLinkStatus` currently assumes the I/E read order
   in both the request and its word indexes. For A use hex C22 batches
   `10 01 01 00 04 05 06 19` and `10 01`. Reserve ten frames and two
   callbacks. Decode both bookends in this order, retain PHYSTS[6] before a
   BMSR read clears it, and retain the first sample if that BMSR read fails.
   Use BMCR for forced speed/duplex with coherent link evidence. Keep the
   resolved tuple, pause validity, transport health and SDK notification
   evidence independent. See §§8.3.1/2 and tables 8-20–31/35/44.
5. **Implement A fast-drop admission and observed straps.** In
   `src/DP8382xQuality.cpp` the current timing flag tests combinations after
   programming. On A reject any selection containing 0406 bits without at least
   one 0009 bit before programming, including raw threshold overrides. Enable
   only in declared and observed MII. Disable detectors, program the applicable
   thresholds (MLT3 0117=0417, descrambler 0131[5:0]=08 for the documented
   recipe), verify, then enable with CR3 mask 040F. Check 042F→0021 and
   044F→0041 for energy only. FLD_CFG2 resets to 0288. CR3[10] resets to zero
   in BASIC and follows straps in ENHANCED. Retain PHYSCR[1:0] reset evidence
   (3 BASIC, 0 ENHANCED) through unrelated updates. CR1[7] recovery preserves
   autorun and robust bits. Consume FLDS once through the collector.

   Expose SOR1[11] (pin 19) with validity tied to the current reset. The
   current initialization stores SOR words in the operation result. A later
   quality operation needs a durable observation from that epoch or a new
   read. Fail the second SOR read and check partial validity. Exercise both
   BASIC and all six ENHANCED rows in the ledger. ENHANCED option 4 is
   000C, option 5 is 0001 with Strap8 LOW. §8.3.17 recommends disabling energy
   detection for expected baseline wander. The rollover guide recommends
   Strap11 HIGH when using FLD for its EMC result. Leave policy selection to
   the application. A fresh SOR1 read adds four frames and one callback.
   Derive the admitted 58-only budgets from the selected rows. The ledger's
   combined 52/19 apply and 23/9 verify examples include TX controls and
   assume current SOR1. See SNLS783A tables 8-5/6/29/31/34/36/53/54/88
   and SDAA067A §§2–3.
6. **Split pin 30 from the RMII and pin 31 routes.** In
   `src/DP8382xInterface.cpp`, BASIC Strap10/4 = 00/01/10/11 means MII,
   RMII leader, reserved, RMII follower. In ENHANCED only Strap1/8 = 11
   selects RMII. Compare raw straps, SOR2[4:3] and live collected RCSR
   independently. Test 25/50 MHz roles, follower-only shift, both revisions,
   elasticity at 50/100 ppm and isolation with dependent clocks. Repeater
   readiness also requires both PHYs, crossed wiring and compatible drivers.
   Verify GENCFG[3] for 120/200 ns IPG and CR2[1] odd-nibble disable.

   On A, RMII owns 0302[8]. Preserve reserved 0302[6]. The pin 30 clock
   owner writes and verifies 0303[5:0] as 0029 for PLL or 0031 for recovered
   125 MHz with a valid partner. Preserve the upper bits and require board
   pin/clock ownership. The current snapshot has eight indexed words and an
   eight-bit validity mask. Retain 0303 independently of 0302 and 0308 and
   preserve existing indexes. For RMII with owned isolation, PLL and current
   prerequisites, two preflight reads plus five direct and three MMD rows
   take 44 frames/17 callbacks to apply, 19/9 to verify. A requested ENHANCED
   pin 31 status sample at 0308[0] costs another 4/1. Its mux programming
   remains source-gated. See §§8.3.7–10, fig. 8-13 and tables
   8-12/13/42/74–79/85/89.

Silicon facts above use the [exact-part datasheet](../../reference/source/TI-DP83826AI-AE-SNLS783A.pdf)
and [rollover guide](../../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf).
TI Linux's revision-masked match and the TI Zephyr selector that loses DEVAD
in the data function are counterexamples documented at combined TI commit
`450af1144b8caeb58724bcbae081b409f6bf5c3a`. [Pinned comparison](../../reference/library-family.md#identity-and-a-generation-comparison)

### Peer conventions and consumer checks

The live checkouts below were clean on `main`. The comparison included each
peer's agent rules and test registration.

| Sibling and revision | Reopened paths and selected convention |
| --- | --- |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h` VariantInfo/KNOWN_VARIANTS/findVariantByProductId, `src/MB85RC.cpp` _selectVariant/_validateActiveDeviceId, `test/test_basic.cpp`, `test/test_public_contracts.cpp`, `examples/common/DiagnosticCore.h` deviceVariantToStr, `tools/hil_runner.py` --require-variant and `platformio.ini` native. Use the compact table and retain observed mismatch evidence. Product-ID auto-selection requires a uniquely identifying ID. |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h` isValidModel/isValidAddress, `src/TMP1x2.cpp` bind/probe, `test/variants/test_variants.cpp`, `examples/common/Tmp1x2CliDiagnostics.cpp`, `tools/hil_tmp1x2_runner.py` and the CMake variants test. Adapt the exhaustive model/address matrix and zero-callback checks to declaration, generation, mode and revision. |
| TUSB32x `6989c77fad3bc4651403942a4c9f7ca3940f859b` | `include/TUSB32x/Config.h` DriverConfig/DeviceProfile, `src/TUSB32x.cpp` validateDriverConfig/validateProfile, `src/TUSB32xOperations.cpp`, `test/test_validators.cpp`, `test/test_profile_application.cpp`, `examples/common/Tusb32xCli.cpp`, `tools/tusb32x_hil_plans.py`, `tools/tusb32x_hil_runner.py` and `test/CMakeLists.txt`. Validate enums before capabilities and compare declared target with firmware evidence independently. |

Phase (b) should carry the actual declaration through CLI `variantName`,
the compiled fixture, Python protocol, campaign `check_device`, selftest and
offline replay. The fixture and both runner checks currently expect original
revision-1 IDs. Test each admitted A declaration against each mode, swapped
declarations, wrong-generation transcripts and historical I/E artifacts.
The native adapter's `Phy::create` binds the same core. Exercise its lifecycle,
admission and publication with an A model. Keep the fitted P4 board declaration
and board-readiness gate tied to that board.

Register named AI and AE CTests with Release-active assertions. Use literal
wire expectations independent of production metadata. For each changed and
inherited path exercise insufficient aggregate time/frames, busy/release
failure, each partial transaction, cancellation before and after the first
target write, retained uncertainty, cleanup/take/replay and passive snapshots
with zero callbacks. Run existing I/E and tests-OFF consumer regressions,
focused native/CLI/Python suites, Doxygen, ledger, references and version
checks. Link changed SDK inputs for ESP32-P4 using `scripts/idf.cmd` under
`C:/esp/build/dp8382x-loop` with ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`. Publish actual per-procedure
test names and grade-specific fixture cases in the maintained guides.

Track the seven source dispositions during implementation: `forced_physts`,
`hard_reset_straps`, `pcs_reset_extent`, `rmii_rc`, `fld_reset_defaults`,
`next_page_handshake` and `fld_threshold_recipe`. Carry their observation,
access and timing limits into tests and fixture guidance.

The implementation updates public Doxygen, README/CHANGELOG, variants, API,
feature coverage, identity/reset/link/quality/interface guides, adapter and
fixture instructions alongside the code. Keep one collector and one writer
per function, existing C++17 naming, and separate primary/release/cleanup
receipts. Reuse shared responsibility modules and preserve retained feature
results under the current operation contract. The present report and progress
row describe planning only.

### Phase (a) verification

Windows x86-64, Python 3.12.10, existing GNU Release/Ninja host build with
pinned native tests enabled. These commands ran at the prompt 57 checkpoint.
The two scratch PDF comparisons are local to this checkout.

```powershell
python -B tools/verify_ledger.py
python -B -m unittest discover -s test -p 'test_*ledger.py' -q
python -B build/p57-final-sources/crosscheck.py
python -B build/p57-procedures/check.py
ctest --test-dir build/p53-baseline -R '^(e_)?(identity_|operation_initialization$|reset_|negotiation_|forced_|link_|quality_|interface_|mmd_traces$)' --output-on-failure --no-tests=error -j 3
ctest --test-dir build/p53-baseline -R '^(e_native_(factory|configuration_admission|cli_discovery|cli_feature_admission)|cli_fixture_transcript|cli_matrix_registrations)$' --output-on-failure --no-tests=error
```

- **Host research PASS.** Both ledgers and all 100 ledger tests passed. The
  two original PDFs each yielded 79 tables and 427 rows. The procedure check
  reproduced the eight FLD strap vectors and verified the owner assignments.
- **Host driver/CLI PASS for the existing I/E baseline.** 74 selected core
  and six native/CLI registrations passed. AI/AE runtime execution is pending.
- **Documentation PASS.** `python -B tools/verify_references.py` checked
  29 vendor artifacts, 27 extracts and 2,355 local links.
  `python -B tools/check_doxygen.py` checked 294 generated pages and rejected
  both negative fixtures. `python -B scripts/generate_version.py check`,
  `git diff --check`, new-report whitespace and the exact 21-ID table join passed.
- **Portable consumer, exported package consumer, SDK image link, remote CI
  and physical evidence NOT RUN in phase (a).** The first CI run belongs to
  prompt 71. Each grade needs an identified and authorized fixture for
  strap sampling, reset/clock timing, link behavior and electrical/EMC claims.
  Phase (b) has no blocking dependency on these measurements.
