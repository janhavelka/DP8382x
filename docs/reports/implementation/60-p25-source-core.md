# Prompt 60 · DP83825I source and core assessment

## Phase (a) · Implementation brief

**IN PROGRESS.** The next implementation must add the first selectable DP83825I.
At entry on 2026-11-25, the worktree and index were clean on `main` at
`5aa3a5f0371fa720fa43cc7fddb922f018faa6c9`. The wrapper verified equality
with `origin/main` after prompt 59. Local comparison was 0 ahead and 0 behind.
The reusable inputs are the exact-part profile, bounded Clause 22 and indirect
transactions, identity and reset operations, link and collector owners, and
the pinned native owner. The existing I/E and AI/AE results establish their
own baselines. The three register JSON files on entry contain **no DP83825I
procedure IDs assigned to 60**. Phase (b) must add the exact-part ledger,
assign each procedure and check it against the original tables.
[Prompt 60](../../prompts/60-p25-source-core.md) ·
[Existing profiles](../../../src/PartTable.h) ·
[Last completed checkpoint](59-a-feature-closure.md#phase-d-final-verification)

Two read-only agents used the current high-reasoning model in parallel for
the PDF inventory and the cross-layer admission trace. The lead checked the
table counts, the timing footnote, current sources and sibling conventions.
For phase (b), the lead owns the declarations in `include/DP8382x/`, private
part selection, shared state machine, integration and checkpoint. Disjoint
work can cover the exact-part ledger and its checker, and the compiled
fixture/runner tests against agreed public values. Follow the existing C++17
names, one instance owner and one destructive-read collector. Keep the short
bus lock around the complete indirect or RMW transaction. Document state,
units, bounds and partial effects with each public change.

### What the primary sources establish

| Evidence inspected | Decision for implementation |
| --- | --- |
| SNLS638C, April 2026, original PDF pp. 39–81, sections 7.1–7.101 and tables 7-3 through 7-103 | The 101 detailed registers comprise 31 direct, 64 vendor extended and six printed MMD headings. All 101 partition bits 15 through 0. There are 570 printed field rows, 201 named reserved and 369 with other names. Some named fields are described as reserved. Keep original access, reset, strap, prose and callable-operation dispositions per field. |
| SDAA081, September 2025, sections 2.2.4–2.4, and SNLU239A, December 2023, section 3.1 | Troubleshooting uses strap status and clock readback to confirm board wiring. The EVM documents physical strap and LED configuration. The application supplies clock direction, reset and pin ownership for its own board. |
| Pinned TI Linux and combined mirror, `dp83822.c`, first 50 lines of each file, GPL-2.0 | Both label A150 as I, A140 as S and A160/A170 as CM/CS. Table 7-6 governs the declared I policy. Keep these labels as conflicting source evidence. |
| Zephyr `phy_ti_dp83825.c`, Apache-2.0, inspected clock, IRQ, reset and link paths | Its RCSR read/write, single link IRQ, 1 ms reset assertion, 50 ms waits and AN-only configuration describe that driver. The portable owner uses the exact datasheet timing, supports forced links, observes the clock strap and releases the controller before waits. |

The original PDFs, hashes, rights and retrieval records are in the
[archive manifest](../../reference/source/provenance.json). Sources used here
are [SNLS638C](../../reference/source/TI-DP83825I-SNLS638C.pdf),
[SDAA081](../../reference/source/TI-DP83825-Troubleshooting-SDAA081.pdf) and
[SNLU239A](../../reference/source/TI-DP83825EVM-SNLU239A.pdf).
The Linux pins are
[97becb13, drivers/net/phy/dp83822.c](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c)
and [450af114, linux_drivers/dp83822.c](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/dp83822.c).
The Zephyr pin is [bc5d59d1, drivers/ethernet/phy/phy_ti_dp83825.c](https://github.com/zephyrproject-rtos/zephyr/blob/bc5d59d1b6657cfeef30124b818c8fbd69da68ac/drivers/ethernet/phy/phy_ti_dp83825.c).

**Inventory boundary.** Start with table 7-1, reconcile every heading, then
review the timing footnotes, reset/strap prose, diagnostic guide, functional
modes and application tables. Explicitly account for the prior PartTable audit
offsets `000C, 0101, 010A, 0123, 0130, 0172, 0174, 030F, 0311, 0313, 031C,
031F, 033C, 033E, 0416, 0428, 0429, 04CD..04D2, 04D4..04D7` (hex). Keep
printed MMD headings `1000, 1001, 1014, 1016, 203C, 203D` distinct from
DEVAD and wire offset. Section 6.3.11.1 describes vendor DEVAD 31. Table 7-16
and the EEE section also identify DEVAD 3 and 7. Table 7-92 explicitly
mentions low offsets 0014, 003C and 003D. Document the evidence for each
translation, including PCS control/reset at printed 1000. A heading or an
existing DP83826 tuple alone cannot authorize the transfer. Keep REGCR/ADDAR
direct-only and protect direct-register aliases, reserved ranges and consuming
reads across all routes. [PartTable audit](../../prompts/REMAINING_PROMPTS_AUDIT.md) ·
[Current dispositions](../promptset-audit-dispositions.md)

### Implementation order and acceptance

1. **Declaration and rejection.** Append the exact I enum and activate its
   private profile. Require an explicit RMII LEADER or FOLLOWER in `Config`.
   An omitted role needs a distinct representation because the present
   interface clock settings default to leader. Preserve earlier enum values
   and bindings. The documented identity is `2000 A140` (model 0x14,
   revision 0), with no BASIC/ENHANCED mode. Strap-address discovery defaults
   to 0–3. Capabilities are copper RMII and the four 10/100 half/full choices.
   Exercise every revision nibble, wrong OUI/model, A150/A160/A170, I/E and
   A-grade words, partial reads, all-zero and all-one pairs. Preserve raw
   rejection evidence. In `src/DP8382x.cpp`, extend the A-only rejected-ID
   guard in `_execute` to the 825 path so only bounded ID observation remains.
   Check the effect of descriptor batches containing indirect selector writes.
   An invalid declaration, interface, medium request or rebind must leave
   outputs, pending work and prior configuration intact with zero callbacks.

2. **Startup and reset.** Use tables 7-3 and 7-33 for BMCR PCS reset,
   PHYRCR HARD and digital RESTART respectively. Preserve raw post-command
   identity and strap evidence. PCS retains vendor registers, HARD is described
   as pin-equivalent and RESTART retains registers. Reconcile section 6.5.1's
   sampled-versus-reloaded strap wording per reset. In the default functional
   mode section 6.4 requires no additional programming. Assert absence of
   `030E=4A40` across NONE, PCS, HARD, RESTART, verify and restoration,
   including failure/retry paths. Recheck the current VOD skip and the verify
   path that can finish without fresh role evidence. Replay only enabled,
   persistent owned settings after qualification.

   Section 5.6 gives a minimum 25 µs external reset pulse, a maximum 2 ms
   from release to SMI readiness and a maximum 50 ms POR-to-SMI interval.
   Figure 5-1 requires release at least 100 µs after the clock stabilizes when
   it arrives late. Section 6.3.11 also requires an idle MDC cycle after reset.
   Check these against the application-owned clock and the native
   `prepareBoard` path. Use the 20 MHz MDC maximum from timing row T4 over the
   24 MHz pin/SMI prose. The nominal 0.2 ms leader clock and 1500 ms FLP
   timings are observations to qualify on the bench. Anchor waits to observed
   completion or a later sample and retain shared-bus silence across failures.

3. **Clock evidence and safe modification.** Tables 6-9/6-10, 7-26 and 7-80
   give independent checks. Read vendor 31:0467 bit 3 for the RX_D1 strap and
   read C22 17 bit 7 through the collector. LEADER is 0/0 (25 MHz XI, PHY
   supplies 50 MHz to the MAC), FOLLOWER is 1/1 (50 MHz XI). Cover all four
   observed pairs against both declarations. Publish a mismatch, retain the
   consumed overflow/underflow bits 3:2 on later failure and revoke old role
   proof on reset or contradiction. Table 7-26 prints reset 0061 while bits
   6:5 are reserved/reset zero. Preserve their actual readback. Avoid the
   DP83826 interface row's `01B3` mask and bit-5 interface decode. Only bit
   4 and bits 1:0 are common writable RMII settings here. Bit 8 applies to
   follower and bit 7 is strap confirmation. Use asymmetric reset-loaded
   samples such as `006D` and `00ED` for leader and follower. Both include
   reserved 6:5 and both FIFO indications. Check the consuming read inside
   the actual RMW transaction as well as ordinary acquisition. Retain its
   evidence when the following write or controller release fails.

   **Follower setup-time option.** Section 5.6 footnote (2), PDF p. 10,
   pairs `0017[8]=1` with `0042=0014` for the 7.5–10.5 ns range. Record the
   latter as a recipe-only vendor offset outside the detailed map. Test the
   two effects and interruption between them. Establish documented recovery
   back to default timing before advertising reversible selection. Preserve
   an explicit limitation if the prior opaque value cannot be restored safely.
   Simulated MDIO can verify the sequence. Electrical setup time needs a
   measured fixture. [TI datasheet, section 5.6](https://www.ti.com/lit/ds/symlink/dp83825i.pdf)

4. **Clock-loss admission and link.** For declared leader, require explicit
   clock-loss approval before I/O for operations that select nonzero 0305[2:0],
   BMCR isolation [10] or deep power 0428[2]. Even with approval, a later
   SOR1 mismatch blocks those writes. Check the effective outgoing word in
   masked updates. For example, `activateProfile` preserves BMCR[10] through
   `0x44FF`. Apply the same owner rule on replay, raw aliases, CLI and SDK
   routes. Isolation follows table 7-3. Mux/deep-power clock loss is inferred
   from table 7-61 and section 6.3.8 and requires physical Q15. These guards
   also apply while optional feature operations remain unimplemented. Reject
   a known prohibited request at preflight. If a prerequisite read reveals
   isolation or a role mismatch, stop before the mutation and retain the read.

   Use `DP8382xConfiguration.cpp` and `DP8382xLink.cpp` for general AN,
   next-page, extended/forced link, crossover, pause and live status. Cover
   all advertisement subsets and forced 10/100 × half/full, optional robust
   crossover (table 7-12), coupled pair/polarity swap and table 7-13
   extended-duplex conditions. Force local ANAR[13] low, including raw
   maintenance. For a live remote fault, table 7-19 says BMSR clears the
   PHYSTS mirror. Capture PHYSTS before the clearing BMSR reads and distinguish
   cable down, pending negotiation, fault evidence and failed transport.
   Include exact-part fast link drop, extended-link controls and RMII repeater
   configuration from sections 6.3.10 and 6.3.16.2. Check source-specific
   prerequisites, independent field masks and finite recovery. MII-only
   configuration needs an explicit inapplicability disposition.

5. **Cut off inherited operations until qualified.** `capabilities().valid`
   can currently admit DP83826 optional paths. Test feature preflight, raw
   access, collection masks, dumps, IRQ/wake modifiers and every reset tail
   with 825 selected. Exclude `000C, 000F, 0012, 0013` and mixed `0017`
   from ordinary reads. Keep MISR1/2 as the IRQ collector source. The latch
   relation to 000C is unresolved. An inaccessible feature must reject before
   callbacks or desired-state mutation. Distinguish successfully consumed
   evidence from history lost after a partial transfer.

6. **Prove the public routes.** Keep the SDK factory boundary in
   `src/esp_idf/Phy.cpp`. Reject MII/RGMII, fiber and inconsistent clock
   declarations at applicable portable, native and fixture entry points.
   Measure callback counts, including clock/timer hooks, and verify sentinel
   outputs. Extend the compiled CLI fixture, `test/support/PartSelection.h`
   and `tools/dp8382x_hil_{protocol,campaign}.py`. Current campaign logic
   expects model 17/19, interface mask 3, BASIC/ENHANCED and VOD. Use model
   20, RMII mask 2, no operating mode and explicit role evidence for 825.
   Preserve earlier artifact replay. Show separate outcomes for both roles,
   failed second reads, role mismatch and prohibited writes.

### Scope handed to prompt 61

Phase (b) of **60** owns the complete source inventory, including the following
finite families. Give every row a semantic ID, source, stage, legal values,
partial-effect/restore contract and independent test or cited disposition.
The ownership below assigns implementation to 61 after the core is verified.
[Prompt 61 requirements](../../prompts/61-p25-feature-closure.md)

| Family assigned to 61 | Evidence and boundary |
| --- | --- |
| Functional modes, EEE, LPI and power | Tables 6-5/6-6/6-7 contain 13/13/32 ordered writes, each ending in 001F=4000. Cover power-optimized, reach-optimized and EEE recipes, legacy LPI, IEEE, active sleep, PLL and deep-power transitions. Record exact 04D1=00C2 bypass polarity against table 7-92 and SNLA328. |
| IRQ, event/counter diagnostics, link quality | Complete service, enable, test, latch and partial-sample behavior. Include consuming 000C, FLDS, MISR1/2, RCSR FIFO history, counters and the SDAA081 section 2.3.3 MSE procedure at 0218. Core role/link reads already require correct collection in 60. |
| Loopback, BIST/PRBS and TDR | All documented local, PCS, digital, analog and reverse loops, generation/checking, error counters, cable configuration, manual/automatic capture and interpretation. Use the exact 825 procedures and limits. |
| Pins, LEDs and wake | Exact implemented pins/functions, polarity, overrides, IRQ routing, filtering, Magic Packet and Secure-ON. SNLA261A table 2-1 and section 2.2.2 identify 04A5–04A7 despite their absence from table 7-1. Qualify these separately in Q15. |
| Electrical, analog and compliance | Tables 8-2/8-3 VOD controls, RX thresholds and squelch, amplitude, test patterns, collision and applicable compliance/diagnostic procedures. Inventory recipe-only 030E, 0333, 0218, 031B, 033A and 033D as exact actions with prerequisites. Track conflicting or opaque values explicitly. |

### Sibling audit and verification plan

All three sibling checkouts were clean on `main` at the revisions below.
Each was inspected read-only, including `AGENTS.md` and
`.github/workflows/ci.yml`. The target repository has no workflow on this
checkpoint. First-execution remote CI belongs to 71.

| Sibling and revision | Paths inspected and convention selected |
| --- | --- |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h` (`isValidModel`, `hasAlertOutput`), `src/TMP1x2Configuration.cpp`, `test/variants/test_variants.cpp`, `examples/common/Tmp1x2Cli.cpp`, `tools/hil_tmp1x2_runner.py`, `CMakeLists.txt`. Adopt explicit declaration, per-model capability checks and registered invalid-model cases. |
| 24Cxx `139e76cd38ecdeec40be5fdb1ce5e633158ea59a` | `include/EEPROM24Cxx/Config.h`, `src/EEPROM24Cxx.cpp::geometryFor`, `test/test_core/test_core.cpp::geometryValidation`, `test/test_detection/test_detection.cpp`, `examples/common/Eeprom24CxxCli.cpp`, `tools/hil_identity.py`, `CMakeLists.txt`. Adapt the separation of declared part, discovery observations and firmware identity. EEPROM geometry and bank probing do not apply. |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h` (`VariantInfo`, product matching), `include/MB85RC/Config.h`, `src/MB85RC.cpp`, `test/test_basic.cpp`, `examples/common/DiagnosticCore.h`, `tools/hil_runner.py`. Reuse the discipline of checking manufacturer and product before claiming a match. Keep the PHY board declaration and exact revision policy. |

For phase (b), extend the existing CTest identity, access, reset, negotiation,
forced, link, interface and ownership suites at their normal entry points.
Inject failure at each transfer, including release failure, late completion,
possibly applied writes, cancellation, exhausted frame slices, stalled clocks
and elapsed deadlines. Check untouched ordinary outputs, cached bus silence,
configuration trust, retained consumed data and separate finite cleanup.
Exercise accepted and failed native factory/owner paths using the pinned host
driver and link the real ESP32-P4 adapter selection and both examples with
`scripts/idf.cmd`. The local ESP-IDF v6.1 checkout resolves to
`fff9895c82d744c7237be8847347bdd1b07c6643`. Run the existing Release, focused
UBSan, tests-OFF consumer, source-package consumer and offline CLI/replay
checks using the [predecessor commands](59-a-feature-closure.md#phase-d-final-verification).

Update public Doxygen, README/CHANGELOG and the variants, API, identity,
initialization, link, interface-clock, native and hardware-validation guides
with working behavior. In particular, qualify the unconditional VOD sentence
in the API contract. Physical validation still needs an identified DP83825I
board with observed ID, role straps, reset, XI/CLKOUT and clock-loss evidence.
No physical operation was authorized for this phase.

### Phase (a) verification receipts

Windows 11 AMD64, CPython 3.12.10. The lead independently opened the archived
SNLS638C PDF with PyMuPDF. On PDF pp. 39–81 use `get_text("words")` in y/x
order. Section headings start near x=57 pt, bit ranges at x=68–82 pt and field
names at x=107–110 pt on the same baseline (within 1 pt). Count each printed
row once across page continuations. Each section's masks were disjoint and
their union was FFFF. The heading and row counts above passed on the archived
SHA-256 `ec16ad2e7f01ede18aea5fbc39349cf4e6bceabc3541e6d3d43fc7bf17e3cb06`.
On pp. 33–35, match address/value pairs case-insensitively within each mode
table. An initial lowercase-prefix-only count missed the 030C and 0311 rows
printed with uppercase X. The corrected check passed 13/13/32 including each
terminal restart.

| Layer | Command or evidence | Result |
| --- | --- | --- |
| Host research and runner tests | `python -B -m unittest discover -s test -p 'test_*.py' -q` | PASS, 216 passed and 56 compiled-fixture skips. |
| Archive and existing ledgers | `python -B tools/verify_references.py` and `python -B tools/verify_ledger.py` | PASS, 29 payloads, 27 extracts and 2,412 local links. The existing I/E and A ledgers passed. The DP83825I inventory check above used the original PDF. |
| Documentation, metadata and whitespace | `python -B tools/check_doxygen.py`, `python -B scripts/generate_version.py check`, `git diff --check` and the new report's whitespace check | PASS, 294 generated pages and two rejected negative fixtures. Version header current. |
| DP83825I executing host behavior, portable/package consumer and SDK link | Assessment only | NOT RUN for each layer. Phase (b) supplies the first executing 825 paths. |
| Remote CI and physical DP83825I | First execution CI belongs to 71. Physical Q15 requires a separate fixture. | NOT RUN for each layer. |

## Phase (b) implementation

**IN PROGRESS, ready for phase (c).** The wrapper verified clean `main` and
`origin/main` at `5115dd0a0e5a4d7920a0bc1a3a392223fff88df4` before this
phase. The lead integrated the shared API, part selection, controller and
operation contracts, native owner, examples, tests and guides. Two concurrent
workers used the session's high-reasoning model for the independent PDF
inventory and the disjoint compiled fixtures/runner. Both finished. The three
sibling checkouts, revisions and conventions in the phase (a) table were
rechecked, including the selection headers, variant/geometry tests, CLI and
Python identity runners. The repository still has no workflow.

### Source and procedure coverage

The [DP83825I ledger](../../registers/dp83825i.json) assigns 18 procedures to
60: `identify`, `mdio`, `external_reset`, `pcs_reset`, `hard_reset`, `restart`,
`initialize`, `clock_role`, `rmii`, `follower_timing`, `clock_loss`,
`configure_an`, `configure_forced`, `next_page`, `observe_link`, `fast_drop`,
`link_options` and `repeater`. The 15 remaining semantic families retain
implementation stage 61. The original PDF comparison covers 101 registers,
570 field rows, six IEEE MMD translations and ten exceptions outside the
detailed map. The three functional-mode recipes contain 13, 13 and 32 writes.
The [ledger tests](../../../test/test_p25_ledger.py) independently reconstruct
the PDF field cells and literal recipes. The Linux ID-label discrepancy and
RCSR reset contradiction remain in the ledger.

| Public route | Executable behavior and evidence |
| --- | --- |
| Bind, identify and discover | Appended the DP83825I enum. Only copper RMII with an explicit board role and no BASIC/ENHANCED requirement binds. A specified MDC rate above 20 MHz rejects. Probe retains both raw words or the completed prefix. Post-rejection traffic is restricted to direct ID reads. The literal identity test covers all 1,024 model/revision combinations and native factory tests count timer and clock callbacks. |
| Initialize, reset and verify | SOR1[3] and collector RCSR[7] must both match. FIFO events survive a failed write or release. Each reset recaptures identity and straps before owned replay. NONE and verify use the default functional mode. There is no 030E=4A40 on these paths. HARD reserves the 2 ms readiness floor and a longer idle MDC period when needed. The native board test exercises a 25 ms period at 40 Hz. |
| Interface, clock loss and link | RCSR preserves 6:5 and checks the strap inside the RMW transaction. Follower extended setup applies bit 8 then 0042=0014. A partial operation retains effects and HARD can replay the desired pair. Default timing recovery requires a separately established reset baseline and fresh binding because TI gives no inverse companion value. The leader needs clock-loss approval for isolation and pin 2 mux changes. A guarded dependent write prevents a newly observed isolation bit from being echoed. The native executor and test controllers enforce the predicate under the controller lock. |
| Link policy and observation | All advertisement subsets, optional robust crossover, coupled pair/polarity swap, forced speeds/duplex, next pages, extended duplex and pause use the common owner. The local ANAR never sets remote fault. The link observation samples PHYSTS before BMSR. Fast link drop accepts all 32 combinations without a DP83826 threshold recipe. The RMII repeater owner requires admitted wiring and coordinated external reset. |
| Optional controls and offline tools | Unqualified optional procedures reject before I/O or intent changes. The raw map excludes selectors, consuming reads and undocumented tuples. Fourteen compiled campaigns cover seven cases in each RMII role with offline replay and forged-evidence rejection. Older artifact shapes remain supported. |

### Local verification

Windows 11 AMD64, GNU 15.1.0, CMake 4.0.1, Ninja and Python 3.12.10. For the
pinned host build, CTest and SDK links use the three process-local Git safe
directories, local clangd PATH and CCACHE_DIR from the
[prompt 53 build receipts](53-regression-transaction-ownership.md). The native
source verifier checked 45 SDK files at ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`.

| Layer | Result |
| --- | --- |
| Host Release | **PASS**. Full CTest in `build/p53-baseline` passed 1,013/1,013 in 265.34 s. After the slow-MDC reset, mixed-RCSR interruption and optional-feature admission changes, the 132 affected Release registrations in `build/p60-tests` passed. The final identity, startup and interface/profile suites performed 27,864, 5,524 and 12,084 checks respectively. |
| Host UBSan and Python | **PASS**. The final 132 focused UBSan trap-mode registrations passed. Python discovery ran 277 tests with 57 skips in the standalone environment. Compiled CLI CTests supply the exercising fixture. The 104 ledger tests and the original-PDF comparison passed. |
| Consumer builds | **PASS**. Fresh tests-OFF portable consumer 2/2. Git-free source-package consumer 1/1 in `build/p60-export/out`, using exported headers and sources, license, manifests and root CMake file. It links the public target and checks the 825 strap-address limit. |
| SDK link | **PASS**. The 825 leader adapter fixture and both existing ESP32-P4 example selections linked. All three binaries identify the phase (a) SHA above with a dirty worktree. The example board gates stayed closed. |
| Remote CI | **NOT RUN**. First-execution CI belongs to 71. |
| DP83825I physical | **NOT RUN**. Q15 needs identified silicon, XI/RMII clock and reset measurements. Clock interruption from mux and deep power is inferred. The relation of 000C to MISR latch clearing and the inverse follower companion word remain unresolved. |

The initial test build caught a signed/unsigned conversion in the timing-cut
index. The first reused negotiation-admission test also rejected a legal
825 forced setting with robust crossover disabled. Both were corrected and
rerun. An 825 BIST test then reproduced token admission through the private
loopback path. Moving the variant check to the shared procedure made both
entry points reject the request before changing the token or accessing MDIO.
The first reference check found development README copies with missing
relative links inside the scratch package. The consumer was rebuilt from
the intended code, manifest and license payload. The maintained documentation
covers binding, identity, reset, clock, quality, physical qualification, direct
access, CLI fixtures and native integration.

```powershell
cmake --build build/p53-baseline --parallel 6
ctest --test-dir build/p53-baseline --output-on-failure --no-tests=error -j 1
cmake --build build/p60-tests --parallel 7
ctest --test-dir build/p60-tests -R '^(p25_|native_factory|native_board|native_shared|cli_fixture_transcript$|cli_processor|bist_|loopback_|lpi_|ai_(bist|lpi|loopback)|ae_(bist|lpi|loopback))' --output-on-failure --no-tests=error -j 3
ctest --test-dir build/p55-ubsan -R '^(p25_|native_factory|native_board|native_shared|cli_fixture_transcript$|cli_processor|bist_|loopback_|lpi_|ai_(bist|lpi|loopback)|ae_(bist|lpi|loopback))' --output-on-failure --no-tests=error -j 3
ctest --test-dir build/p60-consumer --output-on-failure --no-tests=error
ctest --test-dir build/p60-export/out --output-on-failure --no-tests=error
python -B -m unittest discover -s test -p 'test_*.py' -q
python -B tools/verify_references.py
python -B tools/verify_ledger.py
python -B tools/check_doxygen.py
python -B scripts/generate_version.py check
git diff --check
```

SDK builds used `scripts/idf.cmd -C <project> -B <dir> -D IDF_TARGET=esp32p4
-D SDKCONFIG=<dir>/sdkconfig build`. For the adapter, also pass
`-D DP8382X_FIXTURE_VARIANT=DP83825I`. Directories below are under
`C:/esp/build/dp8382x-loop`.

| Directory | Project | Binary bytes | SHA-256 |
| --- | --- | ---: | --- |
| 56l | test/esp_idf/adapter_link | 400176 | `a0b0b3c763c0199bc96527ca01651e75e76491b3f9c320e0d60da8683e187d6e` |
| 56b | examples/esp_idf/basic | 705040 | `817fe183ee51e68a6d18b531bcab65a4f43813610e34853350aeed74e95f87ff` |
| 56o | examples/esp_idf/owner | 583168 | `c9017bed38b002e6d2c04b69d9b7ee3f16e85a54f447c21e0d460c27c3bac27c` |

Phase (c) should independently review the actual synchronized diff against
the source tables, controller contract and offline replay. The wrapper owns
the phase (b) commit and post-push equality verification.

## Phase (c) review of identity and interrupted strap capture

The wrapper verified phase (b) at `3d7487908bc1d1e626dbfe288697ff202ac89cae`.
The tree and index were clean on entry, with `main` equal to `origin/main`.
Two read-only reviewers worked in parallel using the session model for exact
identity admission and the reset state machine. The identity review traced
SNLS638C tables 7-5/6 through the part table, failed-probe prefix, direct
ID-only guard and bounded native discovery. It found no defect in those paths.

The clock review checked SOR1[3], RCSR[7] and the FIFO read-clear fields in
SNLS638C tables 7-26 and 7-80. When a HARD reset interrupted recapture, the
driver could discard earlier returned words or omit a role mismatch already
proved by a completed read. The new `p25_core` test uses distinct pre- and
post-reset words, then injects a transfer or release failure at SOR1 or RCSR.
It reproduced 16 failed assertions on the phase (b) implementation.
The operation result now retains historical words and their stale status.
The live snapshot records the current capture. The first observed role
mismatch survives alongside the terminal transport failure.

On Windows x86-64 with GCC 15.1.0, the focused Release and UBSan trap-mode
suites each passed 19/19 after the fix. Run the following for both
`build/p60-tests` and `build/p55-ubsan` as `<dir>`.

```powershell
cmake --build <dir> --target dp8382x_interface_tests dp8382x_identity_tests dp8382x_reset_tests --parallel 3
ctest --test-dir <dir> -R '^(p25_(core|profiles|identity)|reset_.*|interface_.*|ai_interface|ae_interface)$' --output-on-failure --no-tests=error -j 2
```

Portable/package consumers, SDK link, remote CI and physical qualification
were **NOT RUN** in this slice. Phase (c) remains in progress. Continue
with follower timing restoration, clock-loss guards across controllers,
link semantics, the source inventory and native/CLI replay evidence.

## Phase (c) review of follower timing and leader isolation

Two independent read-only reviewers traced the changes from `5115dd0` to the
synchronized `395ba14` checkpoint. The follower review confirmed the section
5.6 footnote 2 sequence in SNLS638C. The interface owner sets RCSR[8], writes
vendor 0042=0014 and retains proof for later verification. The host fixture
clears both registers on HARD. Its interruption tests check that replay restores
shifted timing after confirmed and uncertain writes. A board reset baseline and
fresh binding remain necessary to select default timing.

The leader review checked BMCR isolation against table 7-3 and traced the
pin 2 mux and deep-power guards against table 7-61 and section 6.3.8. The
portable test already changes BMCR[10] after link preflight. The installed
native fixture now repeats that change just before the command. Without
clock-loss approval the instrumented MAC records one locked BMCR read, the
mutex releases, and the result retains CONFIG_MISMATCH and cleanup owed for
earlier configuration writes. With approval the MAC records the read and
the isolation-preserving write under lock, and configuration finishes.

Host **PASS** on Windows x86-64 with GCC 15.1.0. The six selected CTests
passed in both Release and UBSan trap mode, including native factory tests at
100, 333 and 1000 Hz. Repeat with `build/p60-tests` and `build/p55-ubsan`
as `<dir>`. The first build used an incorrect SDK directory for Git ownership
and stopped at the verifier. Both reruns verified 45 pinned SDK files.

```powershell
$env:GIT_CONFIG_COUNT='1'
$env:GIT_CONFIG_KEY_0='safe.directory'
$env:GIT_CONFIG_VALUE_0='C:/esp/v6.1/esp-idf'
cmake --build <dir> --target dp8382x_interface_tests dp8382x_native_host_tests dp8382x_native_host_100hz_tests dp8382x_native_host_333hz_tests --parallel 3
ctest --test-dir <dir> -R '^(p25_(core|profiles)|(e_)?native_factory(_100hz|_333hz)?)$' --output-on-failure --no-tests=error
```

Portable/package consumers, SDK link, remote CI and physical evidence were
**NOT RUN** in this test-only slice. Phase (c) continues with link semantics,
inventory and optional feature boundaries, and native/CLI campaign evidence.
