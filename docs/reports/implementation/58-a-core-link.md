# Prompt 58 · DP83826AI/AE identity, reset and link

**IN PROGRESS. Phase (d) SDK links and AI/AE UBSan passed.** The portable core and native owner accept
declared DP83826AI and DP83826AE for the 21 procedures below. The two grades
share revision-4 identity words. The Release host suite has a reproducible
fixture shutdown failure. Prompt 59
owns optional and diagnostic controls.
[Prompt requirements](../../prompts/58-a-core-link.md)

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

## Phase (b) implementation

The wrapper verified clean `main` at `4bcfe04ebb2b01147e0e4876b518912c0a844ee3`
equal to `origin/main`. The lead implemented selection, map gates, strap
lifetime, pass admission, tests and integration. Two high-reasoning agents
implemented disjoint interface/quality and link/consumer paths. Both tasks
required reasoning about destructive reads, partial results and exact-part
source evidence. The lead retained shared declarations, registrations, this
report and the commit request.

The phase (a) sibling table still applies. On this run MB85RC, TMP1x2 and
TUSB32x were clean at the listed revisions. The lead reopened their live
declarations, source, example/CLI, test and runner conventions. The compact
part table, exhaustive enum validation and independent declaration versus
observed identity checks carry over. Automatic grade selection cannot
distinguish either pair of temperature grades. The two PDF cross-checks listed
under phase (a) passed again. They compared 79 detailed tables and 427 rows
per generation and reproduced the eight FLD vectors. No workflow exists at
this checkpoint. ESP-IDF host verification used v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`.

Each test suffix below runs with both the `ai_` and `ae_` prefix. The CTests
execute with Release-active checks. The stage uses the existing register and
procedure modules and extends their current fake controllers.

| Procedure IDs | Executing grade-specific CTest suffixes | Evidence |
| --- | --- | --- |
| `identify` | `identity_decode`, `identity_policy`, `identity_failures`, `operation_initialization` | Retained declaration, two modes, every model/revision encoding, partial pair, rejected requirements and wrong generation before any write. `foundation_link` covers all 256 enum encodings. |
| `direct_access`, `mmd_access` | `reset_access` | Direct holes, aliases, selector exclusions, DEVAD 7 data function 4007, original-only raw writes and optional typed owners rejected, once-collected FLDS and passive cached evidence. |
| `initialize_10base`, `pcs_reset`, `hard_reset`, `restart` | `operation_initialization`, `operation_failures`, `reset_traces`, `reset_floor`, `reset_failures`, `reset_timing` | Literal command and completion reads, distinct reset domains, reidentification, 030E=4A40, partial batches, deadlines, cancellation, recovery and retained bus silence on uncertain HARD. NONE takes 20 frames and a commanded initialization takes 32 on first-clear with no owned profiles. |
| `external_reset` | `reset_external`, `reset_ownership`, `native_lifecycle` | External invalidation, owner/pin preparation and independent board timing bounds. The native owner requires a 25 us minimum assertion, waits for clock/power readiness including its 100 us margin, and preserves 50 ms POR and 2 ms post-reset SMI floors. |
| `configure_an`, `next_page_exchange` | `negotiation_vectors`, `negotiation_pages`, `negotiation_page_failures`, `negotiation_page_boundaries`, `negotiation_replay` | ANAR prohibits local bit 13, D8 toggle, ACK, null page, partial receive and cleanup. |
| `configure_forced`, `extended_link_configure` | `forced_matrix`, `forced_transitions`, `forced_verification_outcomes`, `forced_replay`, `forced_power_pin` | Robust CR1 through the forced interval, BMCR and duplex restrictions, MDIX and unrelated bits through apply/replay. |
| `observe_link` | `link_read_plan`, `link_failures`, `link_coherence`, `link_forced`, `link_pause`, `native_lifecycle` | Both PHYSTS-first bookends, partial mirror retention, coherent tuple, forced BMCR source, parallel detect, pause matrix and separate native notification failures. |
| `configure_fast_drop`, `link_recovery_configure` | `quality` | All eight strap vectors, pin 19/SOR1 and failed SOR2, reset PHYSCR bits, MII precondition, detector prerequisites, 0117/0131 thresholds, separate CR1[7] and CR3[6:5] preservation. Combined admitted FLD/recovery case takes 32 frames/11 callbacks to apply and 15/5 to verify with current straps. |
| `mii_configure`, `rmii_configure`, `clock_configure`, `repeater_configure`, `compatibility_configure`, `isolation_configure` | `interface` | Raw straps, sampled startup mode and live RCSR are distinct. Pin 30 control at 0303, optional ENHANCED pin 31 observation at 0308, clock-role/wiring checks, IPG, odd nibble and isolation readbacks. RMII with PLL and owned isolation takes 44/17 to apply and 19/9 to verify. Recovered clock adds one PHYSTS frame; requested status adds a four-frame transaction. |

`strapSnapshot()` retains the independently valid current SOR prefix. A new
capture, reset, invalidated device or changed identity withdraws previous
epoch validity. The operation result also retains historical evidence with
its own stale flag. Capabilities carry the accepted declaration. The interface
snapshot extends the original word indexes with pin 30 control at index 8.
Finite profile-pass admission checks the selected rows in polling and
resamples the available allowance at terminal callback entry.

The CLI, compiled fixture and Python runner validate exact declarations and
raw words. Each A grade runs the 21 core campaign cases outside observe/INT,
observe in both modes, and six admitted interface/repeater/quality cases.
The simulated selftest and offline replay reject wrong-generation or
swapped-grade evidence. Pin 30 programming preserves the reserved IO_CFG1
bit and pin 31 observation grants no mux write. Existing I/E artifact schema
and fitted P4 board declarations remain in use. The adapter link fixture
takes an explicit build-time grade so AI and AE can be linked independently.

The seven source dispositions in phase (a) retain their limits. In particular,
FLD readback and Strap11 evidence establish the programmed selection only.
An authorized board with a known fault stimulus is required for timing and
EMC evidence. Prompt 59 retains its 31 procedures and 56 exceptional writes,
including the distinct A EEE recipe. Physical evidence for both grades is
**NOT RUN**. Exported package consumption and remote CI are **NOT RUN** in
phase (b). Prompt 71 owns remote CI.

### Phase (b) local verification

Windows x86-64, GNU 15.1.0, Ninja, CMake 4.0.1 and Python 3.12.10.
The host configure step verified 45 SDK inputs at the pin above. Supply the
process-scoped SDK safe.directory setting from the prompt 53 report for the
SDK identity check. The first configure encountered Git's SDK ownership check
and succeeded with that exact checkout setting. The first focused build found
signed test literals under `-Werror=sign-conversion`; the raw-address vector
now uses unsigned literals. The integrated suite caught the earlier two-part
expectation in `foundation_link`. Its 256-value check now exercises both A
declarations, both original declarations and every unsupported encoding.

```powershell
cmake -S . -B build/p53-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p53-baseline --parallel 6
ctest --test-dir build/p53-baseline --no-tests=error --output-on-failure -j 3
ctest --test-dir build/p55-ubsan -R '^(ai|ae)_' --no-tests=error --output-on-failure -j 3
cmake -S test/consumer -B build/p58-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/p58-consumer --parallel 3
ctest --test-dir build/p58-consumer --no-tests=error --output-on-failure
python -B -m unittest discover -s test -p 'test_*.py' -q
python -B tools/verify_ledger.py
python -B tools/verify_references.py
python -B tools/check_doxygen.py
python -B scripts/generate_version.py check
git diff --check
```

The final integrated **Release host PASS** was 791/791 in 141.91 seconds.
The interruption registration completed in 141.83 seconds. The procedure/test
join above matches the 21 owned ledger IDs and all named per-grade CTests.
Both implementations were joined before handoff. Phase (c) should review
the actual checkpoint, particularly map exclusions, reset-epoch validity,
consuming order, interface/quality admission and grade-aware offline replay.

The Release UBSan build used `-fsanitize=undefined -fsanitize-trap=undefined`
for C and C++. Its rebuilt identity, operation, reset, forced, negotiation,
link, quality, interface and native host executables passed all 94 A-specific
tests. Both tests-OFF portable consumers passed. Standalone Python discovery
ran 269 tests with 55 compiled-fixture skips; the Release CTests supply those
fixtures. The ledger check passed, including all 79 detailed A tables. Version, whitespace, references (29 payloads, 27 extracts, 2363 links)
and Doxygen passed (294 generated pages and both negative fixtures).

ESP32-P4 **SDK link PASS**. The adapter fixtures explicitly select AI and AE.
The basic and owner applications use their fitted I declaration. All four
ELF and binary pairs contain the entry SHA with dirty worktree identity.
The two fixture binaries also contain the selected grade strings. The builds
used the process-local SDK environment and cache from prompt 53 and the
repository wrapper. Reproduce with
`scripts/idf.cmd -C <project> -B <dir> -D IDF_TARGET=esp32p4 -D SDKCONFIG=<dir>/sdkconfig build`,
adding `-D DP8382X_FIXTURE_VARIANT=DP83826AI` or `DP83826AE` for the adapter
link fixture. Short directories below are under `C:/esp/build/dp8382x-loop`.

| Project and directory | Binary bytes | Binary SHA-256 |
| --- | ---: | --- |
| AI adapter fixture, `53l` | 391216 | `d2763a653597804d71e88ee3bb77000098662afb84da5b27b9aa37a417c1f990` |
| AE adapter fixture, `56l` | 391216 | `a148fa5c86a875d7a4439680df4ffcc82e1f3e78da15be4833facbde578b9d9a` |
| I basic example, `56b` | 688240 | `8344fc3bfa89992fa2ad947cf2fb120f416264aa4f8c8fc6e51ad54867901976` |
| I owner example, `56o` | 566336 | `9b7bf0940adda98de618126094fc5a215ab2ec7a4aaa3bf688f586a886c653d3` |

## Phase (c) independent review and fixes

The wrapper synchronized the implementation at
`1e5e035bc781efbf835ebfbbfce71098839db92c`. The index and worktree were clean
on entry and local `main` was 0/0 against `origin/main`. Two read-only reviewers
used the session's high-reasoning model in parallel to trace destructive reads,
reset domains, deadline admission and state retention across the 21 assigned
procedures. One examined identity, resets, link, crossover and FLD. The other
examined interface, pin and threshold ownership, replay and partial effects.
Both completed. Their pre-fix reproducers are in local `build/p58c-core` and
`build/p58c-io`. The lead checked the findings, CLI fixture, runner and native
publication contract and integrated the following Release-active regressions.

| Finding at the implementation checkpoint | Correction and executing test (both AI and AE) |
| --- | --- |
| After a known wrong-generation ID, a retry that timed out on ID2 reopened MMD access. | Retain the last conclusive rejection independently of the most recent identity receipt. `reset_access` checks partial ID1, zero feature frames while rejected, and restored access after an accepted pair. |
| Independently observing 1F=4000 cleared the live SOR pair while the operation still reported it as valid. | Preserve the strap capture across register-preserving restart. `reset_traces` compares the two snapshots and `quality` replays owned FLD through an independently active restart during NONE initialization. HARD still withdraws strap validity. |
| A scheduling delay between the initial clock sample and the first link or interface submission could start a pass whose full cost no longer fit. | Recheck the aggregate bound at callback entry. `link_budgets` delays the second sample by 15 µs. `interface` delays it by 240 µs on a 44-frame, 17-callback pass. Both deny the callback. The direct interface call preserves the previous desired state and caller output. |
| A BMCR reset during an A link read invalidated identity, yet a second destructive batch could publish fresh UP. | Stop after the first eight frames, keep collected evidence and return DEVICE_INVALIDATED. `link_failures` checks the unchanged output and stale cache. `native_lifecycle` verifies administrative DOWN and required stop for both carrier states. The original-grade native registration still checks its historical physical sample and administrative withdrawal. |
| Pin words from before HARD regained valid bits when the caller had dropped those pin requests and replay skipped them. | Invalidate interface and quality word masks when the device configuration is lost. Retain the historical words. `interface` checks that an unowned 0303 and unrequested 0308 stay invalid after successful replay. |
| An independent read of 0117=0416 after threshold verification could leave FLD enabled and trusted. | Use FFFF/0417 for all observations of the A recipe and FFF0 for its write. `quality` injects the read after the MLT3 row, immediately before enable, and after completion. The pending pass stops without further frames, retains the mismatch and reports cleanup owed. |
| The compiled A fixture retained programmed pin/threshold words across HARD and omitted fixed bits from reset readback. | Reload the simulated A interface and FLD controls from the fixed strap inputs. The CLI checks the unowned pin 30 transition 0029→0008, BASIC/ENHANCED PHYSCR 010B/0108 and FLD_CFG2=0288. The runner checks full register words, including raw pin 31 status 0002 with bit 0 clear, and replays quality and RMII cases in BASIC as well as ENHANCED. |

Source review used SNLS783A §8.3.17, figure 8-13 and tables 8-5/6,
8-20/21, 8-29 through 8-31, 8-35/36, 8-49, 8-53/54, 8-74/75 and 8-79.
SDAA067A leaves Strap11 EMC qualification with the physical fixture. The two
phase (a) PDF cross-checks passed again for 79 tables and 427 rows per
generation, eight FLD vectors and the 52 procedure assignments. Reopened
the live sibling headers, sources, CLI, runners, tests and agent rules in
the phase (a) table. MB85RC, TMP1x2 and TUSB32x were clean at those same
revisions. Their distinction between declared identity and partial observation
applies directly to the inconclusive reprobe regression.

### Phase (c) local verification

Windows x86-64, GNU 15.1.0, Ninja, CMake 4.0.1, Python 3.12.10 and the
phase (b) pinned ESP-IDF v6.1 checkout. Use the process-local SDK Git and tool
settings in the [prompt 53 report](53-regression-transaction-ownership.md) for host and SDK
commands. The initial full run caught three original-grade native reset
contract regressions and an SDK identity check run without its scoped Git
setting. The corrected native tests cover the original sample contract and
the A rejection separately. The final commands use the configured SDK path.

```powershell
cmake --build build/p53-baseline --parallel 8
ctest --test-dir build/p53-baseline --no-tests=error --output-on-failure -j 3
cmake --build build/p55-ubsan --parallel 5
ctest --test-dir build/p55-ubsan -R '^(ai|ae)_' --no-tests=error --output-on-failure -j 3
cmake --build build/p58-consumer --parallel 3
ctest --test-dir build/p58-consumer --no-tests=error --output-on-failure
python -B test/test_hil_features.py --fixture build/p53-baseline/test/cli/dp8382x_cli_fixture.exe --work-dir build/p58c-runner CompiledFeatureTests.test_a_feature_cases_require_their_generation_profiles
python -B -m unittest discover -s test -p 'test_*.py' -q
python -B build/p57-final-sources/crosscheck.py
python -B build/p57-procedures/check.py
python -B tools/check_doxygen.py
python -B tools/verify_ledger.py
python -B tools/verify_references.py
python -B scripts/generate_version.py check
git diff --check
```

SDK link **PASS** for both declared adapter fixtures and both fitted-I
applications. The four ELF/bin pairs contain the review entry SHA and dirty
tree identity. Reproduce with the four phase (b) wrapper invocations.

| P4 project and directory | Binary bytes | SHA-256 |
| --- | ---: | --- |
| AI adapter, `53l` | 391664 | `9a6c5068ddb0d149a9a8549d4b8b4bdb910a2301c9ed7665a46854d903051ded` |
| AE adapter, `56l` | 391664 | `bccaba49767725799941f1f79e8b9d186a60fe97b74a3d2a07174bea6669e93b` |
| I basic, `56b` | 688896 | `1f7abd3717329d07c931853a124630c33803a9fc365423cf7c188f5eab2e3552` |
| I owner, `56o` | 567008 | `74a5d4166472dec19177359a3e31d551add78cbf94c640379308e919adca6299` |

Release host **PASS** 791/791 in 147.33 s, including pinned native integration,
compiled CLI campaigns, replay and interruption tests. A-specific UBSan trap
**PASS** 94/94. The focused BASIC/ENHANCED campaign regression also passed.
Standalone Python ran 269 tests with 55 compiled-fixture skips. Doxygen passed
with 294 generated pages and two rejected negative fixtures. Source, ledger,
reference, version and whitespace checks passed. The portable tests-OFF
consumer build passed 2/2. Exported package, remote CI and physical evidence
were **NOT RUN**. Remote CI belongs to prompt 71. Grade-specific clock, reset,
FLD timing and Strap11 EMC qualification still require identified, authorized
fixtures. Phase (d) starts after the wrapper synchronizes this review.

## Phase (d) SDK links

The wrapper verified `main` equal to `origin/main` at the review commit
`38ec9f1ef4d50d6af34c0ad4a22e3c2877b0299b`. The worktree and index were
clean throughout the builds. On Windows x86-64, two parallel build workers
ran the phase (b) `scripts/idf.cmd` commands in separate ESP32-P4 build
directories against ESP-IDF v6.1 at `fff9895c82d744c7237be8847347bdd1b07c6643`.
Each used the scoped Git and clangd settings from the prompt 53 report, with
`CCACHE_DIR` under its own build directory. The examples used
`IDF_COMPONENT_MANAGER=0`. All four commands exited 0.

**SDK link PASS (4/4).** Each ELF and binary contains the full review SHA
and clean-tree identity. Both adapter fixture pairs contain their selected
grade. The basic and owner examples build with the fitted DP83826I board
declaration. The directories below are under `C:/esp/build/dp8382x-loop`.

| P4 image | Directory | ELF bytes | Binary bytes | Binary SHA-256 |
| --- | --- | ---: | ---: | --- |
| DP83826AI adapter | `53l` | 7379936 | 391664 | `802718f83c317bde315137d6a33cf8be15b2d400386d21976717c170e8d9a794` |
| DP83826AE adapter | `56l` | 7379936 | 391664 | `80721b4b636318be70624ff9b3a4cc2431c8db1875397206f7ef696abc7333fe` |
| DP83826I basic | `56b` | 10055116 | 688896 | `053cc6b7ac5d2917146ee1ed71c21e2a3bcd91ad19202ab970482fcde1a2f11f` |
| DP83826I owner | `56o` | 8748588 | 567008 | `135a5d0d95b4499c7a31c2765c7a4d6a102e7c88c79e592cf6f26ec1ed2dac65` |

The build logs are `58d-link.log` for the fixtures and `phase-d-link.log` for
the examples, each in its image directory. Host and portable consumer checks
remain for phase (d). Exported package, remote CI and physical evidence
are **NOT RUN** for this slice.

## Phase (d) host verification

The wrapper synchronized the SDK receipt at
`12edfac4037de9a709e9974fa8edb58c368aa961`. Only the report and progress
row changed since the four SDK links. The lead ran Release on clean Windows
x86-64 with GNU 15.1.0, CMake 4.0.1 and Ninja. A second worker ran UBSan in
its separate build directory. The three peer checkouts listed in phase (a)
were clean at their recorded revisions. The Release build verified the
37-file example and 45-file native manifests at the recorded v6.1 SDK pin.
Both builds used the process-local SDK Git setting from the prompt 53 report.

```powershell
cmake -S . -B build/p53-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p53-baseline --parallel 6
ctest --test-dir build/p53-baseline --no-tests=error --output-on-failure -j 3
ctest --test-dir build/p53-baseline -R '^cli_hil_features$' --no-tests=error --output-on-failure
cmake --build build/p55-ubsan --parallel 5
ctest --test-dir build/p55-ubsan -R '^(ai|ae)_' --no-tests=error --output-on-failure -j 3
```

**Release host FAIL.** 790/791 registrations passed in 134.85 s.
`cli_hil_features` also failed alone in 26.66 s. In both runs the compiled
DP83826I `wol-pulse` case and restoration passed, then fixture closure
expired and both output workers reported failure to stop. The campaign
recorded `complete=False`, verdict `INCONCLUSIVE` and exit 2. The test
expected exit 0. The full and isolated receipts are
`build/p53-baseline/58d-release.log` and `58d-features-retry.log` in the
same directory. Trace child exit, pipe EOF and reader completion in
`tools/dp8382x_hil_fixture.py` before repeating the focused and full suites.
The cause of the shutdown delay remains open.

**AI/AE UBSan PASS.** All 94 registrations passed in 0.83 s, 47 per grade,
with C and C++ trap flags `-fsanitize=undefined -fsanitize-trap=undefined`.
The receipts are `p58d-build.log` and `p58d-ctest.log` in `build/p55-ubsan`.
Final portable consumer, Python, source and documentation checks remain.
Exported package, remote CI and physical evidence were **NOT RUN**.
