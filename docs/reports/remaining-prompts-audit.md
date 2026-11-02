# Audit of remaining prompts 13-80

Date: 2026-11-02. Baseline commit: `e105af6`, branch
`promptset/dp8382x-implementation-20261028`. Prompts 1-12 are complete; 13-80
have not run. This audit changes no prompt, contract, ledger or code. It tells the
coder what to change, in which order, before executing prompt 13.

## 1. Result

The library is a framework-independent C++17 driver for TI DP8382x Ethernet PHYs,
DP83826I first, plus a native ESP-IDF adapter for the ESP32-P4. The shipped code is
bounded Clause 22 and MMD register access with terminal batch transport.

**The foundation is sound:**

- The register ledger matches SNLS647I: 79 of 79 detailed registers and 283 of 283
  fields, with no address, reserved-mask, reset or access error (G13, scripted
  comparison plus original-page checks on mismatches).
- The shipped code has no silicon-map error, no overflow and no publication of
  unvalidated values. 14 of 14 CTest suites, 98 Python tests, the reference check,
  the ledger check and the Doxygen check pass. A 1.2M-operation fuzz found no
  invariant violation (G12).
- Sibling paths and symbols named by the prompts exist, apart from one mangled
  path (M-1).

**The problems are in the instructions, not the data.** Five distinct S1 findings
would ship wrong behaviour or block a stage:

| ID | Problem | Consequence if unfixed |
| --- | --- | --- |
| [G2-1](#g2-1) | Forced mode can never report a resolved speed/duplex | The ESP-IDF adapter can never publish link UP on a forced link |
| [G3-1](#g3-1) | The DP83826 TDR note, SNLA428, is not archived or used, although the datasheet defers to it | Prompt 25 ships a cable test without TI's gain writes or 1A/1B runs |
| [G4-1](#g4-1) | The EEE wire-address gate is overstated | EEE and LPI (prompts 29-30) stay blocked, and tests enshrine rejecting the likely correct address |
| [G6-1](#g6-1) | Native support requires a private ESP-IDF fork that nothing can deliver (also G11-1) | Prompts 37-42, 47, 70 and 74 stay blocked |
| [G6-2](#g6-2) | Native failure tests have no layer on which they can run | Prompts 37-42 cannot honestly complete |

The largest maintainability risk is the operation contract ([G11-2](#g11-2) to
[G11-8](#g11-8)). No operation code has shipped yet, so this is the cheapest moment
to simplify it.

In total: 172 auditor findings plus one lead finding, about 60 S2 and 90 S3 alongside the
S1s above. See [coverage and limits](#9-coverage-and-limits).

## 2. How to use this document

Apply the work packages in section 4 in order, before prompt 13. The coder may
reorder inside a package.

Rules while applying:

- A proposed text here is a direction, not evidence. Reopen the cited source for the
  field you are changing. If this audit and the exact-part datasheet disagree, the
  datasheet wins: record the disagreement and tell the owner.
- Keep prompt numbering. Do not renumber or merge prompt files; many links depend on
  them.
- Implementation status stays unchanged. Editing prompts and contracts is planning
  maintenance. Record each work package under `docs/reports/` and commit and sync per
  `AGENTS.md`.
- Line numbers refer to the baseline commit and will drift; the quoted words are the
  stable anchor.
- Physical steps need their own authorized fixture scope. Section 8 lists them as
  planned cases, not as authorization.
- Do not copy code from TI application notes or GPL sources into MIT files. Derive
  independent literal vectors.

Ledger edits can trip the validators, which pin some of this data. Check
`tools/verify_ledger.py` (the feature-to-gate map at lines 118-120) and
`test/test_remaining_ledger.py` (lines 377 and 394) and
`test/test_ledger.py` (line 273), and change them with the ledger on purpose.

Abbreviations:

| Short | Meaning |
| --- | --- |
| `P25:25-31` | `docs/prompts/25-*.md`, lines 25-31 |
| AC, DD, ARCH | `docs/api-contracts.md`, `docs/design-decisions.md`, `docs/architecture.md` |
| ENG, CONV, VER, FC, CLI | `docs/prompts/` ENGINEERING, CONVENTIONS, VERIFICATION, FULL_COVERAGE, CLI_CONTRACT |
| DB, RV, COV | `docs/device-behavior.md`, `docs/register-vectors.md`, `docs/feature-coverage.md` |
| LIBC, LIBF, FAM, SDK | `docs/reference/` library-ti-core, library-ti-features, library-family, library-sdk |
| LEDGER | `docs/registers/dp83826i.json`, named by gate or procedure id |
| SRC, CFG | `src/DP8382x.cpp`, `include/DP8382x/Config.h` |
| 826, A, 825, 822 | SNLS647I (DP83826I/E), SNLS783A (AI/AE), SNLS638C (DP83825I), SNLS505H (DP83822) |
| idx | Zero-based PDF page index |

Each finding carries a severity and a verification mark:

- **S1** ships wrong or damaging behaviour, or blocks the stage. **S2** is a likely
  defect or substantial rework. **S3** is localized.
- **[L]** The lead re-read the primary source and confirmed it.
- **[A]** An auditor read the primary source; the lead did not repeat it.
- **[I]** Rests partly on inference or on knowledge outside the repository (IEEE 802.3,
  general ESP-IDF behaviour, static review only). Treat as a strong hypothesis.

## 3. Decisions taken

Auditors disagreed or left a choice open on these. Each is decided here so work can
proceed. The owner can override any of them; **D9 to D11 change frozen contracts and
are the ones most worth a look.**

| # | Decision | Reason | What is given up |
| --- | --- | --- | --- |
| D1 | Native support runs on **unmodified ESP-IDF v6.1** through public APIs (G6-1). A failure the public route cannot resolve is terminal until reboot (G11-1). Drop the timer fence once the baseline contains upstream `d2ed96d2`. | Every required behaviour maps to public APIs except one unobservable HAL result. A fork has no delivery route and contradicts ENG:22. | In-process retry after a failed teardown; detection of errors the SDK swallows. |
| D2 | Adopt the EEE tuples `(3,0x0001) (3,0x0014) (3,0x0016) (7,0x003C) (7,0x003D)` as **source-implied**, in one constant table, with a fail-closed runtime self-check and a planned probe (G4-1). The public raw API keeps rejecting `(3,0x0001)` and `(3,0x0016)`, which are destructive. | The evidence chain is strong and a wrong tuple is caught before the recipe's opaque writes. | A short period where support rests on implication; status stays "physical check pending". |
| D3 | Default TDR recipe is **SNLA428 table 2-1**. Keep the datasheet words as independent vectors and record source conflict `tdr_recipe` (G3-1). | 826 8.3.16 and 10.1 defer to this note "for details", and its offset and thresholds pair with its recipe. This is not an app note overriding a datasheet by weight; the datasheet delegates. | Physical comparison stays NOT RUN; the choice may need revisiting. |
| D4 | When AN is disabled and link is up, **BMCR is the forced speed/duplex source**; PHYSTS low bits are raw evidence only (G2-1). | Table 8-19 says bits 13 and 8 determine the mode; the PHYSTS sentence contradicts the part's own strap table. | Nothing proved is lost. |
| D5 | Keep `robust=true` for every forced mode on DP83826/A. Move the rule out of the part-agnostic validator into the part table when prompt 56 introduces it (G2-4, G8-10, G9-5). | Satisfies "shall" under either reading; 825 and 822 have no such rule. | Nothing. |
| D6 | Write and verify `VOD_CFG3=0x4A40` **unconditionally** in `startInitialize` and after hard, external or POR reset; verify only after PCS reset or restart. Owner stage 15 (G1-2, G2-9, G11-10). | Strapped ANAR can advertise 10M from POR; all fields are scoped to 10M mode. | A write on 100-only boards; the 100M-inert claim rests on field naming. |
| D7 | Un-gate 10M compliance patterns `COMPT=0x10..0x1D`. Keep the 100M `0x0428` branch gated (G5-1, G13-2). | Table 8-50 documents them and `0x0013` confirms the reading; `0x0428` still has no register table or read-effect record. | Nothing. |
| D8 | Implement next-page exchange from IEEE Clause 28, with NP=1 only for one admitted epoch (G2-2, G2-3). Gate `next_page_handshake` becomes a physical case. | 826 8.3.1 designates Clause 28, and the gate's evidence can never arrive during software work. | Behaviour against real partners is unmeasured until 73. |
| D9 | **Remove the CLEANUP operation phase**; cleanup is a normal second operation behind a `_cleanupOwed` barrier (G11-2), with one acknowledge call (G11-3). | The largest single cost in the state machine, for a route mostly never run, with "not guarantees" conceded by the contract. | One token covering both phases; automatic continuation by polling; pre-reserved capacity. |
| D10 | Remove pre-mutation reserve arithmetic (G11-6). Remove poll allowances, or spend them only on polls that issue a batch or see a frozen clock (G11-8). | The sum drifts from the real sequence at every feature stage. Frames already bound I/O. | Early rejection of too-small budgets, except one fixed rule for reset commands. |
| D11 | Merge `startReset`/`startRecover` into `startInitialize(options)` plus `startVerify` (G11-12). | One private plan with three entry points. | Peer spelling `startRecover`. Check prompt 22 when applying. |
| D12 | PlatformIO is not a supported consumer in this execution. `library.json` stays version-truth only (G10-3 option a, G10-4). | No backend to link against and no consumer. | A PlatformIO package path; add it later with a real target. |
| D13 | No renumbering of review stages. Prompt 55 owns the single coverage matrix; 75 and 78 sample it (G10-11). | Five passes over one denominator produce reports, not fixes. | Exhaustive re-derivation at each review stage. |
| D14 | Apply a 2 ms frame-free floor after PHYRCR hard reset (G1-3). | Table 8-48 equates it to the reset pin; it costs 2 ms. | Nothing measurable. |
| D15 | Reserved-bit handling is per register, not global zero (G13-1). | Seven registers reset reserved bits non-zero. | Nothing. |
| D16 | Keep exact-part W0C for RXFCFG1[11] and the five-frame RMW, and qualify both physically (G4-3, G12-8). | Datasheet authority stands, and family parts differ. | Nothing. |

## 4. Work packages

Run the standard checks after each package: `python tools/verify_references.py`,
`python tools/verify_ledger.py`, `python -m unittest discover -s test -p "test_*.py"`,
`python scripts/generate_version.py check`, the CMake host and consumer builds with
`ctest --no-tests=error`, `python tools/check_doxygen.py` and `git diff --check`.

| WP | Scope | Findings | Done when |
| --- | --- | --- | --- |
| WP0 | Evidence, truth, ownership | [Section 5](#5-wp0-evidence-truth-and-ownership) | Sources archived; one owner per ledger procedure; COMMON has read map and authority order |
| WP1 | Shipped code, behaviour-preserving | [Section 6.1](#61-wp1-shipped-code) | One access-policy table; `_execute` split; existing tests pass unchanged except where listed |
| WP2 | Contracts, before any operation code | [Section 6.2](#62-wp2-contracts-before-prompt-15) | AC, ENG and CONV agree; timing probes updated; AC roughly halved |
| WP3 | Prompts 13-36 | [Section 7.1](#71-wp3-prompts-13-36-primary-part) | Every listed prompt edited; RV and LEDGER updated with it |
| WP4 | Prompts 37-42 and DD section 6 | [Section 7.2](#72-wp4-prompts-37-42-native) | No "corrected SDK" wording remains; host-driver evidence layer defined |
| WP5 | Prompts 43-55 | [Section 7.3](#73-wp5-prompts-43-55-cli-runners-regression) | One fixture, one matrix, one record format |
| WP6 | Prompts 56-67 | [Section 7.4](#74-wp6-prompts-56-67-variants) | Part-table design recorded; variant deltas seeded |
| WP7 | Prompts 68-80 | [Section 7.5](#75-wp7-prompts-68-80-packaging-ci-review) | Toolchain, CI and packaging instructions are executable |

## 5. WP0: evidence, truth and ownership

**Archive two sources** with provenance and hash, and add them to
`docs/reference/README.md`:

- SNLA428, *Time Domain Reflectometry with DP83826*, June 2023, 11 pages,
  <https://www.ti.com/lit/pdf/snla428>, SHA-256
  `b262302866cc3e8abb401f435c6935368192500b4d5fbc27faeca21c75799d33` [L].
- SNLS603D, DP83TG720R-Q1 datasheet, <https://www.ti.com/lit/ds/symlink/dp83tg720r-q1.pdf>,
  SHA-256 `a13ecacc0d2d28c404c4e719fc76d665a4472723fceafbe7b5e5a49baecfb627` [L]. Cross-part
  evidence only, for the MMD notation statement at idx 46.

<a id="g11-17"></a>
- **G11-17** S2 [L] COMMON:3-9, `AGENTS.md`, prompts `README.md`:51-61 omit AC, and 78 of 80
  prompts never name it. No document precedence exists, though DD:11, ARCH:4 and CONV:3-5
  each claim authority. **Fix:** add to COMMON: "Authority: exact datasheet; ledger; shipped
  headers (shipped API); AC; DD; ENG/CONV; ARCH (overview only). Stages 13-36 and 53-67 read
  AC. Stages 37-42 read DD sections 3-7 and AC's native parts. Stage 43 onward reads CLI."
- **G13-3** S2 [A] (also **G5-2**) Only 2 of 52 ledger procedure ids appear in their owner
  prompt, and no rule binds a stage to its ledger procedures. **Fix:** add to COMMON: "At
  stage N, implement every ledger procedure with `owner.implementationStage == N`; list them
  at prompt entry." Then add the missing owners named under P23, P33 and P34 in
  [section 7.1](#71-wp3-prompts-13-36-primary-part).
- **G10-8** S3 [L] (also **G11-21**) Stale text: prompts `README.md`:6 "All stages are currently
  **NOT STARTED**" and :176-177; `test/README.md` near line 44 "These 77 tests" reads as one
  file, and line 59 says "exports" with no install path; `AGENTS.md` and VER:153-165 list only
  research commands. **Fix:** point to the progress record; say "The 77 ledger tests above"
  and "provides"; add the CMake, CTest, consumer, Doxygen and `generate_version.py check`
  commands.
- **G11-19** S3 [A] 15 ledger procedures carry 8-frame-era text ("at most 36 MDIO frames",
  `primaryDurationUs`, `recoveryRequired`), none of which exist in AC. **Fix:** regenerate as
  "snapshot read 4 + atomic modify 5 + verify 4 = 13 per register; deadlines via
  OperationBudget"; drop `recoveryRequired`.
- **G13-4** S3 [L] LEDGER `calibration_recovery` cites section 8.4.5; 826 has only 8.4.1.
  **Fix:** `Table 8-47 p73; table 8-48 p73; 8.4.1 p48`.
- **G13-7** S3 [A] RV:201 presents BMCR `0x6100`/`0x4100` as table 8-19 "examples". The table
  prints no such literal and no duplex rule. **Fix:** say "Composed from table 8-19 (not
  printed); duplex is the test's choice."
- **M-1** S3 [L] P41 and P68 write `FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`, which is
  not a path. **Fix:** name `.h` and `.cpp` separately, as SOURCE_MAP does.

**Ledger and coverage owner corrections.** Apply together, in the LEDGER and COV:

| Item | Now | Set to | Findings |
| --- | --- | --- | --- |
| `interrupt_configuration`, `interrupt_test` | 20 | 27 | G3-9, G11-19 |
| `cable_autorun` | 25 | 26 | G3-9 |
| `tdr_calibration` gate and owner | 26 | 25 | G3-9 |
| `calibration`, `calibration_recovery` | 15 | 35 (hard reset arrives at 16) | G1-9, G5-3 |
| `link_recovery`, `link_recovery_configure` | 18 | 34 | G5-3, G11-19 |
| Gates `recr_width`, `rmii_rc` | 21 | 20; physical evidence 72-73 | G2-13 |
| Gate `rxfs_sfd_reference` | 29 | 31 | G13-5 |
| Gate `fld_reset_defaults` | 4 (finished) | 34; physical evidence 73 | G13-6, G5-4 |
| `initialize_10base` step text | "may choose" | unconditional per D6 | G1-2 |

Add a line to P25: "Manual sessions read CR1[8] and reject when it is set; prompt 26 adds the
disable/restore handoff." Add to P15: "CDCR rescal recovery is not owned here; CDCR stays
rejected for raw read."

## 6. Shipped code and contracts

### 6.1 WP1: shipped code

Refactors here must not change behaviour unless stated. An auditor prototype of G12-1, G12-3
and G12-4 matched the shipped source over 1.2M fuzz operations and passed all transport and MMD
suites; the prototype is not retained, so re-derive it. Only about 1.2% of those operations
returned OK, so failure paths dominated.

<a id="g12-1"></a>
- **G12-1** S2 [L] SRC:44-78 holds one concept in three functions with 55 bare hex cases.
  **Fix:** one table, one row per register, including refused ones. Addresses absent from it
  are holes; linear search is negligible beside MDIO frame time.

  ```cpp
  enum class Access : uint8_t { SAFE_READ, CONSUMING, SELECTOR, RESERVED_ONLY, FOLLOW_UP,
                                PROCEDURE, SOURCE_GATED };
  struct RegisterPolicy {
    uint16_t address; Access access; uint16_t writableMask;
    uint16_t preserveMask;  // Read-back bits kept outside the caller mask (G13-1).
  };
  constexpr RegisterPolicy REGISTER_POLICY[] = {
    {0x0000, Access::SAFE_READ, 0, 0},  // BMCR
    {0x0001, Access::CONSUMING, 0, 0},  // BMSR
    {cmd::REG_LEDCR, Access::SAFE_READ, cmd::MASK_LEDCR_DIRECT, cmd::MASK_LEDCR_DIRECT},
  };
  ```

  Add a `static_assert` that every row with a nonzero `writableMask` is `SAFE_READ`, so a future
  mask cannot allow an RMW through a consuming read. Prompt 13 then extends this table instead
  of adding switches. It stays a policy table, not a second copy of the field map (G1-4).
<a id="g13-1"></a>
- **G13-1** S2 [L] AC:847 and DD say "reserved bits stay zero"; SRC:227 says "Never preserve
  reserved bits"; `docs/direct-access.md`:112 agrees. Other documents and prompts 17, 18, 32 and
  34 require preservation. Non-zero reserved resets: CR2 [13:7]=2h, RCSR [6]=1h, 10M_CFG
  [13:0]=3998h, FLD_CFG2 [15:6]=8Ah, CDSCR [11],[3:0], LED2_GPIO_CFG [5:3]=1h, LEDCFG [3:0]=5h;
  LEDCFG2 bit 10 is RH/W with unknown reset. Extending `writableMask()` as AC reads would clear
  them. LEDCR is safe only because its reserved bits reset to 0. **Fix:** AC:847 becomes "Reserved
  handling is per target. LEDCR forces zero; every other target keeps read-back reserved bits
  outside mask unless a sourced whole-word recipe says otherwise." Mark the SRC comment and
  direct-access text LEDCR-only. Carry it in `preserveMask`. Add LEDCFG and CR2 poison-read RMW
  vectors to RV. See D15.
- **G12-3** S3 [L] `_execute` (SRC:246-359) mixes admission, call, audit, classification and
  publication. Line 345 recomputes four predicates that equal
  `violation != NONE && violation != CONTROLLER_STATE`. **Fix:** helpers in the anonymous
  namespace, no header change: `admitBudget`, `checkPublication`, `flagsConsistent`,
  `auditFrames` returning `{attempted, completed, valid}`, `classifyReport` returning
  `{primary, release, violation}`. `_execute` falls to about 47 lines in four commented steps:
  admit the whole batch; run and keep the report; judge the report; first failure wins (primary,
  release, clock, deadline).
- **G12-4** S3 [L] SRC:205 and :217 repeat line 69, only to hold the order ADDRESS_RANGE,
  VALUE_WIDTH, SOURCE_GATE. Line 222's `!readable` cannot change today's result. **Fix:** one
  `checkMmdCall(devad, address, widthOk)` doing range, width, DEVAD and gate in that order; add
  `directBatch()` beside `mmdBatch()`; keep the `!readable` intent through G12-1's assert.
- **G12-5** S3 [L] **Behaviour change.** SRC:336-347 accepts BUSY/CONTROLLER_QUARANTINED with
  `controllerReusable=true` and publishes reusable. AC:587 and CFG:225 say otherwise; the fakes only
  return false. Diagnostics only, since the field never authorizes I/O. **Fix:** treat it as a
  `RELEASE_REPORT` violation, set `_stopped`, add one test.
- **G12-6** S3 [L] **Behaviour change.** After a pre-callback admission failure only `lastStatus`
  changes, so a reader can pair `late` or `violation` with an unrelated status. Default `lastStatus`
  is OK before any traffic. `lastStatus` and `lastBatchValid` have zero test or example uses. **Fix:**
  remove both (`lastBatchValid` equals `totalBatches != 0`); update AC:563 and :565.
- **G12-7** S3 [L] **Behaviour change.** SRC:250 reports local stop as BUSY/CONTROLLER_QUARANTINED,
  identical to backend quarantine though recovery differs (local stop clears on `end`/`bind`,
  backend quarantine needs controller repair). Tests assert only `.is(BUSY)`. **Fix:** add
  `bool stopped` to the snapshot and assert the detail in tests.
- **G12-8** S3 [A] (see D16) `docs/direct-access.md`:99 "give these complete traces", CHANGELOG:9
  "establishes" and AC:403-405 "independently verified" overstate the five-frame RMW. 826 prints
  only separate read and write sequences; the design rests on the state-based text in 8.3.11.1.
  The test model assumes persistence, so it cannot detect a fault. **Fix:** reword to "inferred
  from 8.3.11.1; not a printed sequence". Keep the design. Add physical case Q4. If silicon
  disagrees, the fallback is one extra `W 0D=401F` frame, which fits `MAX_BATCH_FRAMES`.
- **G12-9** S3 [A] Full-word LEDCR writes replace all five fields. `0x0012` also selects 20 Hz blink
  and forces active-low polarity, overriding the ENHANCED strap (table 8-42, reset `04X0h`), and
  `test/consumer/main.cpp`:83 does this. **Fix:** add to both Doxygen blocks: "Replaces all five
  fields. Unset bits select 20 Hz blink and active-low polarity, overriding the strap. Use
  `modifyMmd` to change one field."
- **G12-10** S3 [L] 13 of 14 opaque EEE recipe offsets return SOURCE_GATE while `0x0123` returns
  ACCESS_POLICY (SRC:73-75, `test/mmd.cpp`:319-332), with no documented reason. Table 8-31's "0x04D1
  and lower" covers 8 of the 13. **Fix:** one `PROCEDURE` result for all recipe-only addresses; keep
  SOURCE_GATE for DEVAD 3/7 until D2 lands.
- **G12-11** S3 [A] The allowlist has no machine link to the ledger, and `--verify-failure-path` is
  not registered in CTest. **Fix:** `test/test_access_policy.py` requires SAFE_READ to equal ledger
  records with no RC/RCH fields, minus a named exception list (CDCR, ANER, 10BTSCR, REGCR, ADDAR,
  all-reserved, RECIPE). Register the failure path for each executable with `WILL_FAIL TRUE`.
- **G12-2** S2 [L] Test layout will not scale: `check`/`CHECK`/`main` copied three times, the fake
  `Controller` twice, 48 `CHECK(x.bind(...))` setup lines, unnamed integer faults. **Fix, no framework:**
  `test/support/Check.h` (CHECK plus `runSuites`), `test/support/FakeController.h` (the mmd.cpp
  superset with `enum class ReportFault`), a `BoundDriver` fixture, one table of
  `{fault, expectedAttempted, expectedCompleted, expectedViolation}`, one statement per line. Later
  prompts add suites, not harnesses.
- **G12-12** S3 [L] Names and comments. `MMD_VENDOR` is both a DEVAD and a REGCR word: split into
  `DEVAD_VENDOR`, `REGCR_VENDOR_ADDRESS`, `REGCR_VENDOR_DATA`. CFG:229-255 is one 27-line Doxygen block:
  keep about 6 lines and point to `docs/direct-access.md`. "Copied all-source diagnostics", "Passive
  receipt" and "Independently valid prefix" need plain words. `examples/portable/DirectRead.h`:18-19
  copies and discards a receipt; `MmdAccess.h`:11-15 stages a needless local copy.

### 6.2 WP2: contracts, before prompt 15

No operation code exists yet. Apply before prompt 15, the first stage that ships operations, and
update `test/test_operation_timing.py`, ENG, CONV and every prompt that repeats the rule.

<a id="g11-2"></a>
- **G11-2** S2 [A] (D9) AC:695-714, 1250-1256, 1349-1355 and 1426 mandate a CLEANUP phase with two
  deadlines, frame counts and poll counts, `CleanupState` with 5 values, and a 10-row transition
  table. Sources disagree too: ENG:110 "separately admitted", AC:1426 "reserved at start", P15:53
  either. AC:1354-1355 concedes these are "not guarantees". For the four frozen kinds, cleanup is
  replay. Peer TMP1x2 operations are 268 lines with no cleanup phase. **Fix:** `OperationState`
  {IDLE, RUNNING, RESULT_PENDING}. A failed or cancelled operation goes to RESULT_PENDING with
  `mutationPossible` and `temporaryMayBeActive`, and sets `_cleanupOwed`. After `takeResult`, other
  starts and raw writes return BUSY/CLEANUP_REQUIRED, except the owning stop/restore start and
  `startInitialize`. Those carry an ordinary budget. Successful stop or restore, or an external-reset
  notification, clears the flag. `end` rejects while it is set until the acknowledge call.
- **G11-3** S2 [A] (D9) AC:703-705, 771-772, 894-895, 923-924, 1447-1466: no code path branches on
  the three `QuiescenceDisposition` values, and the clock latch already forces rebind. **Fix:**
  `Status acknowledgeCleanupAbandoned()`: clear the flag, set initialized=false and trust DIRTY, keep
  all evidence, never claim restored. Delete `QuiescenceDisposition`, `retiredByOwner`,
  `RETIRE_REQUIRED`.
- **G11-4** S2 [I] AC:353-355 and CFG:273 require proving `clockLagUs` "including... scheduling",
  which is impossible on an RTOS and not needed. The sample is read after the callback returned, so
  preemption before the read moves it later (safe) and preemption after does not change it. **Fix:**
  "Positive bound on how far the returned value can be behind true time at the instant of the read
  (quantization plus source update latency). Scheduling after the read is excluded." No behaviour
  change.
- **G11-5** S2 [L] AC:1321-1327: an insufficient positive poll slice terminates the phase, yet
  `OperationSnapshot` exposes no next-batch size, so a short EtherCAT slot kills a reset. **Fix:**
  `Status pollOperation(OperationToken token, uint32_t maxFrames = MAX_BATCH_FRAMES)`. If the next
  batch does not fit, do no I/O and return IN_PROGRESS. Only admission deadline and frame totals are
  terminal. Remove `slice.deadlineUs`.
- **G11-6** S2 [A] (D10) AC:1331-1355 and :1508 require per-procedure reserve sums. Every failure
  already leaves trust DIRTY with effect evidence, and the sum grows at each of prompts 17-35. "Compile
  its finite order" invites a plan engine, against ENG:160-161. **Fix:** delete these and scenario
  :1556. Keep per-batch admission and terminal timeout at the first non-fitting batch, plus one fixed
  rule: reset commands require remaining time >= `pollIntervalUs` + one batch.
- **G11-7** S2 [A] AC:590-592 count timing and clock faults toward OFFLINE, while CONV:73-74 defines
  OFFLINE as repeated MDIO failure; three tight deadlines drive OFFLINE with every frame good. **Fix:**
  replace the table with: "A callback is counted unless zero frames were attempted, status is BUSY or
  OPERATION_TIMEOUT, and release is clean. It is a failure if status or release is not OK, the
  controller is newly not reusable, or the report is malformed. Otherwise success, including late
  success and clock regression, which stay in `TransportSnapshot`." Delete the post-callback publication
  reserve (AC:395-399, SRC:352-355), so only a raw post-sample >= deadline denies publication. **Behaviour
  change in SRC; update tests.**
<a id="g11-8"></a>
- **G11-8** S3 [A] (D10) AC:711-712 and 1311-1316: polling every 100 us during a 50 ms wait exhausts
  `maxPrimaryPolls` and yields a false POLL_BUDGET timeout on healthy hardware. **Fix:** spend allowances
  only on polls that issue a batch or see no clock advance since the previous poll.
- **G11-11** S2 [A] AC:941-942 "incompatible active work" is undefined, with two materially different
  readings, one of which stalls native link polling behind an unconsumed result. **Fix:** add one table.
  IDLE: everything. RUNNING or RESULT_PENDING: stable reads and the collector are allowed between polls,
  because no selector state survives a batch; writes are BUSY. During a reset command or hard-reset silence
  phase: all frames BUSY. Pending result: only starts and writes are blocked.
- **G11-12** S2 [A] (D11) AC:699-700, 715-719, 755-756, 1506-1532: initialize, reset and recover are one
  plan with three entries; `RecoveryMode` omits RESTART. **Fix:**
  `startInitialize(InitializeOptions{ResetKind reset})` with NONE, PCS, HARD, RESTART, plus `startVerify()`.
  Remove `startRecover`, `RecoveryMode`, `recoveryMode`; keep `startReset` only as a thin alias if wanted.
- **G11-13** S3 [A] AC:742-750, 777-778, 784: results carry two raw batch dumps. Measured with g++ 15 on
  x86-64: `OperationResult` 880 B, `OperationSnapshot` 944 B, copied by the owner each poll. **Fix:**
  `BatchSummary{phase, attemptedFrames, completedFrames, effectPossible, normalized, release, violation,
  late}` of about 50 B. Make `OperationSnapshot` result-free. The raw report stays in `transportSnapshot`.
- **G11-14** S3 [L] Acquire, release and overhead are only summed (CFG:268-274, SRC:258-259). **Fix:**
  `TransportBounds{fixedUs, frameUs, clockLagUs}`. Behaviour identical. Update CFG, AC and tests.
- **G11-15** S3 [A] AC:411-413's `storedMs*1000+999` exists only because some fields are ms while the only
  clock is us. **Fix:** store `...Us` everywhere; delete the paragraph.
- **G11-16** S3 [L] (with G8-2) `RevisionPolicy` is a one-value public enum (CFG:35-37, 292; SRC:131); no
  prompt mentions it. **Fix:** delete the enum and config field. Use a private accepted-revision lookup,
  which becomes a part-table field at prompt 56.
- **G11-18** S2 [A] Rules are spread across about 260 KB; "release the lock before waits" appears at least
  six times. AC's poll and transport sections pack several MUST rules into wrapped paragraphs. **Fix:**
  (1) replace AC's code blocks for shipped Status, Config and Transport with links to headers; (2) move native
  material (AC:1087-1106, 1626-1733) to DD; (3) move the feature map (AC:1108-1176) to COV; (4) move scenarios
  (AC:1542-1574) to tests or `test/README.md`; (5) replace ARCH:114-166 with pointers; (6) turn operation
  sections into numbered rules (OP-1...) with one normative location, and have prompts cite the ids.
- **G11-20** S3 [L] (also **G1-12**) CONV:61-63 forbids unused codes, yet `BOARD_PREREQUISITE`, `LINK_MODE`,
  `FEATURE` have no assigned use. P15:30 and :60-61 require testing token exhaustion, which takes 2^64 starts
  or a test seam CONV:156 forbids. **Fix:** remove those three until a consumer exists; delete `TOKEN_EXHAUSTED`
  and its test with "A 64-bit lifetime counter cannot be exhausted in practice"; update CONV:122 and prompts 8,
  15 and 43.
- **G11-22** S3 [A] (also **G2-14**) `getSettingsSnapshot`/`getOperationSnapshot` versus `healthSnapshot`/
  `linkSnapshot`/`transportSnapshot`: rename to `settingsSnapshot()` and `operationSnapshot()` before they ship.
  CONV:114-115 omits `modifyMmd` and `transportSnapshot`. AC:796 "17?18" should read "17-18". DD:223 "Power,
  `startConfigure`" contradicts AC:873. ENG:101, VER:51, P15, P16, P19:58 and P21:51 require "wrap" tests, while
  CFG:261 and AC:346-347 say nonwrapping. **Fix:** replace with "uint64 overflow" and "regressing clock
  (INVALID_CONFIG/CLOCK_CONTRACT, latch, no new valid stamp)".

## 7. Prompt corrections

### 7.1 WP3: prompts 13-36, primary part

#### Prompt 13

- **G1-1** S2 [A] P13:44-53 builds a collector with no legal consumer: AC:956-965 and SRC reject these accesses,
  and CONV:155-157 forbids test-only exposure. **Fix:** "Stage 13 adds no public destructive API. Deliver pure
  `cmd::` composition constants and functions for BMCR, PHYRCR and RXFCFG1 with literal tests, and confirm the
  access classification. Create the private collector with its first real typed consumer (prompt 17's ANER page
  read, or prompt 19's `readLinkStatus`). The phase-failure, history-loss and cached-reader matrix moves there."
  Change P19:30 to "the single collector".
- **G1-4** S2 [I] P13:27-29 mandates a second full register catalogue in C++ (96 registers, 283 fields), against
  AC:1117-1118 and LIBC:127. **Fix:** "The ledger stays the per-field catalogue. Stage 13 classifies every ledger
  address in the G12-1 table and adds named masks only for fields with a consumer now. Add a test that every
  `cmd::` constant matches the ledger."
- **G1-5** S3 [L] P13:33 and ENG:135 require W1C composition. None of the four datasheets use W1C. **Fix:** "for
  W0C (RXFCFG1[11] only), RH/W1S commands, RC/RCH read-clear fields and reserved bits."

#### Prompt 14

- **G1-6** S3 [L] P14:27 calls `readIdentity` and capability queries "frozen"; AC declares neither. **Fix:** add
  `Status readIdentity(DeviceIdentity& identity, const IoBudget& budget) noexcept;` (bound only, one two-frame C22
  batch, output unchanged unless complete and accepted), `DeviceIdentity identitySnapshot() const noexcept;` and
  `Capabilities capabilities() const noexcept;`.

#### Prompt 15

- **G1-2** S2 [A] (also **G2-9**, **G11-10**) P15 never mentions VOD_CFG3, though LEDGER `initialize_10base`, COV:40
  and AC:1400-1401 put it there; P17:42 and P18:41 reimplement it. **Fix (D6):** add to P15: "Mandatory step, one
  vendor batch `W(0D,001F) W(0E,030E) W(0D,401F) W(0E,4A40)`, then verify stable mask `0xFFC0`. Prompt 16 replays it after
  hard reset; PCS reset and restart verify only. Prompts 17, 18 and 22 reuse this procedure." Replace ARCH:239-241 with a
  pointer to DB.
- **G1-8** S3 [A] P15:69-70 "before RCSR or other mutation" implies init writes RCSR, which has RC bits. **Fix:** "before
  any mutation. Initialization does not write RCSR; interface control belongs to prompt 33."
- **G1-13** S3 [I] P15:67 has two readings: `W(00,8000)` or read then write. **Fix:** "Single `W(00,8000)`, no preceding
  read; desired fields are replayed after." Same for `W(1F,8000)` and `W(1F,4000)` in P16.
- **G1-14** S3 [A] P15:26-28's list omits the only escape from failed cleanup. **Fix:** add the acknowledge call from
  G11-3, `invalidateDeviceState` and `isInitialized`.
- **G1-11** S3 [A] P15:71-72 and P16:68-69 understate timing. Figure 7-1's T4 starts at the end of the VDDA3V3 ramp, and
  T1 to T3 can add up to 300 ms before it. **Fix:** "at least 50 ms after both supplies reach final level (end of T3)
  before SMI; a clock late relative to the power ramp requires an additional RESET_N pulse, released at least 100 us after
  the clock is stable and POR is complete."

#### Prompt 16

<a id="g1-3"></a>
- **G1-3** S2 [L] (also **G11-9**, D14) P16:64-65 "Poll `0x1F`" with AC:1392-1393 and default `pollIntervalUs` 1000 places the
  first read about 1 ms after a command that table 8-48 says "has the same effect as Hardware reset pin", inside the window
  of 8.3.11: "must remain inactive... at least until 2ms after reset is de-asserted". A read during reset can return zero and
  falsely show bit 15 clear. **Fix:** "Schedule the first PHYRCR read no earlier than write-completion upper bound + 2000 us
  via `nextPollAtUs`, reserved at admission, then poll `0x1F` mask `0x8000`. 2 ms is a minimum silence, not a completion
  maximum; no sourced maximum exists. Restart `0x4000` and BMCR reset have no such window. Core cannot stop other PHYs: the
  lifecycle guide requires the shared-bus owner to withhold frames for the same window. Test no frame before due with a 1000
  us interval." Update AC:1392-1393, ENG:71, LEDGER `hard_reset.waits` and `calibration_recovery`. The EEE recipe's leading
  `001F=8000` needs the same floor.
- **G1-10** S3 [L] P16:25-28, DB:64 and gate `hard_reset_straps`: 8.4.1 says straps are "sampled at power up or hard reset",
  table 8-47 names `Hard Reset (Reg 0x1F[15])` and table 8-48 equates it to the pin. Only SNLA423A calls BMCR[15] "hard
  reset". **Fix:** "PHYRCR hard reset: eligible straps resampled per 8.4.1 with 8-47/8-48; ENHANCED Strap1 and Strap6 not
  re-latched; BMCR reset reloads stored values; physical confirmation pending." The code path is unchanged.
- **G1-7** S3 [A] P16:16 and :27-28 require POR-versus-reset differences, but `InvalidationReason` (AC:571-573) cannot express
  POR. **Fix:** name `invalidateDeviceState`; add `EXTERNAL_POWER_ON`, implying Strap1 and Strap6 re-latch only in ENHANCED.

#### Prompt 17

- **G2-2** S2 [I] (D8) P17:18-22 and :65-66 order an exchange while AC:256-259 and LEDGER gate it as SOURCE_GATE; no page API is
  frozen. **Fix:** "Implement ledger `next_page_exchange`, the IEEE Clause 28 sequence that 826 8.3.1 designates. Gate
  `next_page_handshake` becomes a physical case for 72-73. Freeze the typed page API here." Update AC and the LEDGER. Say "IEEE
  Clause 28", not "documented".
- **G2-3** S2 [I] (D8) A persistent ANAR NP bit with a one-shot exchange can stall every later AN against an NP-able partner,
  since arbitration waits for a register 7 write that AN restart clears (IEEE 28.2.3.4, from knowledge; IEEE is not in the repo).
  **Fix:** "NP=1 obliges the owner to service pages on every AN cycle. Set NP=1 only for the admitted epoch. At every terminal
  state, including timeout and cancel, write ANAR NP=0, and restart AN only when not complete. Reset replay never replays NP=1.
  Test re-AN with NP=1 and no page load: report explicit pending or stalled negotiation, never silently down."
- **G2-7** S3 [I] P17:49 verifies "all stable owned bits", but default `robust=true` is not applied before stage 18, so a default
  profile reaches a false VERIFIED, fails, or pulls stage 18 forward. **Fix:** "Until 18, trust covers ANAR and BMCR only and the
  snapshot says crossover and extended duplex are not yet applied; non-default values reject at preflight."
- **G2-8** S3 [A] P17:16-17 and :34-35 "invalid selectors" have no field (AC:246-247). **Fix:** "The selector is always encoded
  `0x0001`; a non-default prior selector is overwritten and verified."

#### Prompt 18

- **G2-4** S2 [L] (D5) P18:28-33 and :75 reopen a rule CFG:97, AC:250 and `test/foundation.cpp`:121 froze. Table 8-28: "When using in
  Force Mode, Robust Auto-MDIX shall be enabled". **Fix:** "Every forced profile writes CR1[5]=1. PHYCR[15:14] alone selects routing:
  `0x8000` automatic (bit 14 preserved), `0x0000` MDI, `0x4000` MDIX. Forced manual with CR1[5]=1 is legal, not contradictory; link
  and routing with it are a physical case."
- **G2-5** S2 [L] P18:35-36 says opt-in; AC:254-259 and LEDGER require partner evidence no type can carry, so CR2[5] is unreachable.
  Table 8-29 lists no prerequisite. **Fix:** "`extendedFullDuplex=true` is the owner's declaration; no preflight gate. Write CR2 `0x0020`
  only, preserving other bits. Doxygen warns this changes the IEEE result against a forced-100 partner." Reject forced-10 plus extended
  as LINK_COMBINATION and update `test/foundation.cpp`:120-122. Add a P19 case: AN parallel-detect with CR2[5]=1.
- **G2-6** S3 [A] P18:34 and :43-45 "advertisement stays available" contradicts CFG:160-163. **Fix:** "A forced profile carries no AN
  intent and neither writes nor verifies ANAR. Returning to AN is a new `startConfigure` with a complete Advertisement."
- **G2-10** S3 [A] P18 never names port mirror, which it owns (COV:43). **Fix:** "CR3 `0x0060` is port mirror (table 8-30), not two
  independent corrections; include it in the matrix, Doxygen and vectors."
- **G2-16** S3 [A] P18:76-77 and P22:65-69 test features from later prompts. **Fix:** move to P23: "Loopback exit restores PHYCR[15:14]
  and CR1[5] from the persistent profile, including manual MDIX, with failure at each write."

#### Prompt 19

<a id="g2-1"></a>
- **G2-1** S1 [L] (D4) P19:35-36 and :66-68, AC:1003-1010, DD:261-264, DB:123-126 and gate `forced_physts` keep forced mode unresolved
  indefinitely. Table 8-19: "Disable Auto-Negotiation - bits [8] and [13] determine the port speed and duplex mode". Table 8-34: "ENHANCED
  Mode : 1 when Auto-Negotiation is disabled", which cannot be literal since speed `1h = 10 Mbps` contradicts strap table 8-8 ("Force mode
  100M enabled"). A contract-following adapter never publishes UP on a forced link, because SDK publication needs the tuple. The gate's next
  evidence may never arrive, since A repeats the wording. **Fix:** "When a coherent pass reads BMCR[12]=0 and current link up: speed=BMCR[13]
  (1=100, 0=10), duplex=BMCR[8], `resolvedModeValid=true`, `pauseValid=false`. This describes the local forced mode, not the partner's duplex.
  Exception: forced 100/half with `extendedFullDuplex=true` leaves duplex UNKNOWN and `resolvedModeValid=false` (table 8-29). Never use
  PHYSTS[2:1] while AN is disabled; retain it raw." Test: forced 100/half with PHYSTS low bits `0x0007` yields 100/half. Narrow `forced_physts`
  to a non-blocking question about the raw value. Update AC, DD, DB, LEDGER and RV:144-145.
- **G2-11** S3 [I] P19:37-38 gives two pause sources and no rule. **Fix:** "Compute RX/TX from ANAR and ANLPAR with the Table 28B-3 oracle. Keep
  PHYCR[13:12] as a cross-check; on disagreement `pauseValid=false` with raw words retained."

#### Prompt 20

- **G2-12** S3 [A] (also **G3-7**, **G9-9**) P20:16-25 and :67-73 omit read-clear dependencies. PHYSTS[12] clears on 10BTSCR read and PHYSTS[8] on
  ANER read. RXFS[12] resets to 1, so MISR2[9] means WoL by default, and reading RXFS consumes status. **Fix:** "Collector also owns 10BTSCR `0x1A`
  (polarity `0x0010`), ANER `0x06` and RXFS status. MISR2 `0x0200` stays raw polarity-or-WoL unless owned RXFS[12] state is known; never read RXFS
  to decode it. Reset selects WoL, so polarity needs RXFS[12]=0, written as a whole word `0x1000`/`0x0000` without a read." The same rule applies on
  DP83822, where SNLS505H's MISR2 bit 9 is shared likewise.

#### Prompt 22

- **G2-15** S3 [A] P22:41-42 "Select required verify/reapply/reset steps" reads as auto-escalation. **Fix:** "Options select the steps; the driver never
  escalates. Verify performs no mutation, reapply performs no reset. Insufficient evidence gives an untrusted result, not a stronger action."

#### Prompt 23

- **G3-3** S2 [I] P23:31, :41-43 and P24:29-30: SNLA423A pp18 and 20 add `0x001F=0x4000` after reverse and analog selection, and SNLA423A 2.5.3 adds
  `030B=3380`, yet ledger `loopback_reverse` and `loopback_near` have no restart. TI is inconsistent (2.5.2 has none). **Fix:** P23: "Register this as
  an explicit difference under `loopback_recipe` and include the restart step in those start sequences, with a bounded relink wait for reverse. It
  preserves registers per table 8-48, so it adds no restore debt." P24: "After any restart, read PHYSTS link before writing PRBS mode. `030B=3380`
  remains unadopted: its [11:6]=0x0E is outside the 0x28..0x38 range at LIBF:166. If analog BIST cannot lock, report inconclusive and cite the gate."
- **G13-3 (P23)** Add: "Owns PHYSCR[9:8] FIFO depth for `loopback_reverse`."

#### Prompts 25 and 26

<a id="g3-1"></a>
- **G3-1** S1 [L] (D3) P25:25-31 and :38-39, LEDGER `cable_configure`, `cable_manual` and `tdr_calibration`, LIBF:109 and RV:221-223 build only from
  datasheet words and reject gain writes as DP83825 imports, with SNLA428 absent from the repo. 826 8.3.16: "Refer to the application report Time Domain
  Reflectometry with DP83826 for details." SNLA428 table 2-1, requiring two writes for segment 1:

  | Reg | 1A | 1B | 2 | 3 | 4 | 5 |
  | --- | --- | --- | --- | --- | --- | --- |
  | 0170 | 5C12 | 5C12 | 5C22 | 5E32 | 5E42 | 5E52 |
  | 0173 | 0D00 | 0D0C | 0D13 | 1A20 | 343A | 8F6E |
  | 0175 | 1007 | 1007 | 1007 | 1007 | 100A | 100D |
  | 0178 | 0002 | 0002 | 0002 | 0002 | 0002 | 0006 |
  | 0416 | 1FA0 | 1FA0 | 1FA0 | 1FA0 | 1FA0 | 1F90 |
  | 0411 | 0813 | 0813 | 0813 | 0815 | 0816 | 0816 |
  | 0456 | 0608 | 0608 | 0608 | 0608 | 0608 | 0608 |
  | 001E | 8102 | 8102 | 8102 | 8102 | 8102 | 8102 |

  Datasheet tables 8-56/57 differ (0173 = 0D07/0D14/1A21/343B/8F6F; 0175 = 1004/1004/1005/1008/100B; auditor-read). **Fix:** "SNLA428 is the exact-part
  TDR procedure named by 826 8.3.16 and 10.1. Implement its six-run recipe in order 1A, 1B, 2, 3, 4, 5 as literal vectors, confirming `0x001E[1]=1` and reading
  0x180/0x185/0x18A per run. Tables 8-56/57 print different words: record source conflict `tdr_recipe`, keep both as vectors, and keep the typed fields able
  to program either." Add a procedure-only record for 0x0411. Extend 0x0416, which overlaps EEE's `0416=1F30`. Treat 0x0456 as bits 10:9 only, since the full word
  forces GENCFG[3]=1, which prompt 33 owns. Apply prompt 23's restore rule to all three.
- **G3-2** S2 [L] P25:29-31 and :62-64, LIBF:111, AC:1163, DB:213-216 and gate `tdr_calibration` withhold a distance and open/short reading that SNLA428 supplies.
  **Fix:** "Maximum completion time and freshness remain gates. Provide a pure helper over retained raw samples, valid only for samples taken with the SNLA428
  recipe: scan runs in order 2, 3, 4, 5, 1B, 1A; a peak counts when 0x185 > 10 for segments 2-5 or > 24 for segment 1; metres =
  |(0x180 - 5) * 8 / (2 * prop_dly)| with caller-supplied prop_dly (table 2-4: Cat5/5e 5.35, Cat6 5, Cat7 4.6); sign zero = open, nonzero = short. Raw values stay
  available. Physical accuracy stays NOT RUN." Test the note's example: `|(0x78-5)*8/(2*4.6)| = 100`. Do not copy the note's code: it is copyrighted and inconsistent
  (MATLAB pairs `peak_val(5)` with `peak_indx(2)`, `tmp` is undefined, prop_dly 5.2 differs from table 2-4, and flowchart `threshold2` has no value). The 24 threshold
  comes from MATLAB only. The sign word also holds bit 5 (cross), so test bit 11 for short and retain bit 5 separately; that bit-11 reading is an inference.
- **G3-5** S3 [A] P25:49, :66-67, :72-74 and P26:45-46, :65-66 route TDR words through read-clear machinery. Tables 8-47, 8-61, 8-66 and 8-71 show type R. **Fix:**
  "0x001E, 0x0180, 0x0185 and 0x018A are non-destructive. A failed read gives per-word invalid status with no history-loss flag. Bounded re-read is allowed. Only BMSR
  and MISR reads used as prerequisites go through task 20's collector." Delete "clear/freeze order".
- **G3-6** S3 [A] P26:49 "documented disable/quiesce sequence" and :36 "every source-supported setting" do not exist. **Fix:** "No silicon quiesce sequence is documented.
  Policy is one enable flag, CR1[8]: clear it, verify readback, report engine quiescence as unknown, or reject manual start while enabled. A capture is at most one run under
  the programmed settings, never SNLA428's six-run result, and never carries derived distance unless the recipe context is known."

#### Prompt 27

- **G3-4** S2 [I] P27:41-44: an event set after the MISR1 read and before the MISR2 read leaves the pin asserted with no new edge, so an edge-triggered GPIO never fires again.
  **Fix:** "Service reads MISR1 and MISR2 regardless of PHYSTS[7] and returns `morePossiblyPending`, true when any event byte was nonzero or the budget ended. The owner runs
  another bounded pass or schedules a poll; it never relies on a new edge. Document level-triggered or pin-sampling IRQ wiring. Half-full events request FCSCR/RECR collection in the
  same collector. Empty MISRs with the test bit set are test stimulus, not an error, and service never loops on them."
- **G3-8** S3 [A] P27:17 and :19 leave pin routing ambiguous, and OE semantics are mode-specific. **Fix:** "27 implements PHYSCR[3,1,0] and the conflict rule. Prompt 32 implements
  LED1/LED2 mux=4 writes; LED0/LED3 mux stay read-only. OE semantics are ENHANCED-only: in BASIC, report the strap-mode pin role and do not describe OE=0 as power-down. OE 1 to 0 in
  ENHANCED needs declared board compatibility, since low on the pin means power-down."

#### Prompt 28

- **G4-4** S2 [L] P28:49-51: table 8-19: "this bit is OR'ed with the input from the INT/PWDN_N (in ENHANCED mode) pin. When the active low INT/PWDN_N is asserted, this bit is set."
  So no separate request readback or actual-state register exists, and any BMCR RMW composed from a live read can write bit 11 back as 1. **Fix:** "Report `requestedPowerDown` from
  owner state and `observedBmcrPowerDown` from the read. BISCR[8] is active sleep only. Compose every BMCR write's bit 11 from desired state, never from the read. Readback 1 after a clear is
  `WAKE_BLOCKED_OR_UNCONFIRMED`, with one write and no repeats." The sticky hazard itself is unmeasured (Q6).
- **G4-7** S3 [A] P28:15 never names PHYSCR[15], which LEDGER `power_pll_control` assigns here. **Fix:** "PHYSCR[15] PLL disable (mask `0x8000`) is a separate operation, admitted only in IEEE
  power-down with application approval of clock loss; it is not a substitute for the `0x0428[2]` DPD recipe. The active-sleep mask `0x7000` must not touch it."

#### Prompts 29 and 30

<a id="g4-1"></a>
- **G4-1** S1 [L] (D2, also **G9-7** in part) P29:25-27 and :38-44, RV:326 "Guessed lower offsets 0001/0014/003C fail", the inventory:317-323, LIBF:123 and :142, P36:71, SRC:70 and `test/mmd.cpp`
  `testAdmission` all block on an address question the sources nearly answer. The evidence chain:

  - DEVAD is established for this part: 826 8.3.11.1 gives 3 and 7, "preceded by MMD3" and "MMD7".
  - 825 prints headings `1014h` and `203Ch`, and its text says "Register 0x003C and Register 0x003D in MMD7" and "Register 0x0014 in MMD3".
  - 822 7.4.2.9 is a worked trace for `0x703D`: write `0x0007` to `0x000D`, `0x003D` to `0x000E`, then `0x4007`. The printed high nibble is not sent.
  - SNLS603D idx 46 states the notation: "most significant nibble of the register address is used to denote the respective MMD space. This should be ignored during actual register access
    operation."
  - The low bytes match IEEE 3.1, 3.20, 3.22, 7.60 and 7.61 (IEEE from knowledge).
  - Counter-evidence: A 8.3.3.1 has a literal column `7h / 203Ch`, and no exact-part wire example exists. The prefix digits differ by family (1/2 here, 3/7 on 822), so constants are not shared.

  `(31,0x203C)` is excluded by the exact-part text. `0x04D1` and the other recipe vendor rows use DEVAD 31 per table 8-31, so their open questions concern field semantics. **Fix:** replace P29:25-27 and :38-44
  with: "DEVAD 3/7 is established. Adopt the five D2 tuples as source-implied, recording the chain and the A counter-reading. Hold them in one constant table. Before the recipe's `001F=8000`, read
  `(3,0x0014)` and require bit 1 = 1. After writing `(7,0x003C)=0x0002`, read back and require bit 1 = 1. A wrong mapping then errors before the remaining opaque writes. Route `(3,0x0001)` (latched) and
  `(3,0x0016)` (read-clear) through the collector. Keep the check until a probe closes the gate." Extend `mmdBatch` to take a DEVAD; rewrite gate `eee_wire`; update `test/mmd.cpp` and RV:326. Probe: Q1.
- **G4-2** S2 [L] P30:1, :16-19, :33-34, :45-47 and :52-57, AC:1133 and LEDGER `legacy_lpi` model forced LPI as a finite session. 826 8.3.4: "Forced Tx LPI idles by writing 0x04D1.12 = 1", "Write 0x04D1.12 =
  0 to stop", and "responsibility of enabling and disabling LPI signaling lies on the Host Controller". The part has no LPI wait, readiness event or session bound, and a session would block link polling for
  the whole dwell. "TX-inhibit" is wrong: the PHY transmits LPI, and MAC frames are not buffered. **Fix:** rename "forced TX LPI control". `startLpi()` and `stopLpi()` are independent bounded operations, each
  one `0x04D1` bit-12 RMW with stable readback: after the recipe `0x008B` to `0x108B` to `0x008B`, generally mask `0x1000`. Cached state `forcedLpi {OFF, REQUESTED, UNCERTAIN}`. The application owns dwell, MAC
  drain and post-exit delay; 36 us is nominal. Failure leaves UNCERTAIN, reported as "forced LPI may remain active; frames sent now can be lost". Reject below 100 Mbit/s or without qualified EEE/link evidence.
- **G4-8** S3 [A] P30:23-24: 825 table 7-92 says bit 12 "shall be set after setting bits [14:13]"; 826 mentions bit 12 only. **Fix:** "Implement bit 12 only per 8.3.4. Do not import [14:13]. Prompt 73 observes
  LPI after `0x108B`; failure records this comparison as the next question."

#### Prompt 31

- **G4-3** S2 [L] (also **G1-15**, D16) P31:22 and :45-47, RV:300-303 treat W0C as settled. 826 table 8-90: "W0C ... this bit clears the level upon a write. 0h = Clear". But SNLA261A table 2-4 clears with `0981` (bit
  11 = 1) on DP83822, and 822's type is W1S, the exact inverse. TI Linux writes `value | DP83822_WOL_CLR_INDICATION` on the path shared with DP83826 [A]. If silicon behaves like 822, every "neutral" `0x1981`
  erases the level and the acknowledge `0x1181` leaves it stuck; host tests pass either way. **Fix:** keep W0C and add: "SNLA261A, 822 and TI Linux use write-1 on family parts. Implement the exact-part table
  and qualify in prompt 73 (Q3). Harvest RXFS and MISR2 through the collector before any RXFCFG1 write, and skip RXFCFG1 writes that change nothing, so evidence survives either polarity."
- **G4-5** S3 [A] (also **G8-11**, **G9-8**) P31:28-29 and :42-43 warn of a different DP83822 word order, and P59:31-35 and FAM:86 call packing unresolved. Wire values are identical; only labels differ. **Fix:**
  "DS byte 5 (MSB) is API octet 0. MAC `11:22:33:44:55:66` gives `04A2=2211, 04A3=4433, 04A4=6655`; password `2A..2F` gives `04A5=2B2A, 04A6=2D2C, 04A7=2F2E`. SNLA261A tables 2-3, 2-6 and 2-7 and TI Linux yield the
  same words; ignore byte labels. The real difference from DP83822 is custom pattern matching, which is not imported." Correct LIBF:127 too. On DP83822: example RXFCFG words clear the bit-12 CRC gate, so compose with bit
  12 preserved rather than copy them as vectors.
- **G4-6** S3 [A] P31:55 omits that WoL Enable is already 1 at reset (`108Xh`, bit 0 unknown; RXFS[12]=1). **Fix:** "Reset state is not WoL-off. Initialization does not write RXFCFG1 unless settings own it. Publish
  observed fields. An explicit disable clears bits 7, 5 and 0 with W0C neutral. Bit 0 is RH: confirm stability in qualification."

#### Prompt 32

- **G5-7** S3 [A] P32:36-39 and gate `pin_fields` treat PHYCR[5] as sourceless; SNLA344C 3.2.1 labels `0x8020` "enable LED0 config". That note also has values contradicting the datasheet, so do not trust it as a recipe.
  **Fix:** "Keep it out of the typed profile until clarified; name it for the TI gate and Q12."
- **G5-10** S3 [I] P32:24 and :16 omit per-mode pin availability. **Fix:** "Field vectors are mode-independent, but profile validation is not. BASIC exposes LED0/LED1 only. ENHANCED RMII exposes LED0-3 (LED1 subject to CLKOUT
  selection). ENHANCED MII exposes LED0/LED1, plus LED2 only if the LED2 mux is set and COL is not consumed; LED3 cannot be routed because its mux is read-only. Reject others, and offer no TX_ER mux on pin 28."

#### Prompts 33 and 34

- **G13-3 (P33)** Add: "`mii_configure` and `compatibility_configure` own CR1[1], CR2[1], PHYSCR[4] and GENCFG[3]."
- **G5-2** S2 [A] (also **G13-3**) P34 never names `tx10_configure` or `tx100_configure`, which COV:53-54 assign to it, and no prompt names squelch, 10M_CFG, 10BTSCR, scrambler bypass or FIFO depth. **Fix:** add to scope:
  "`tx100_configure`: PHYSCR[11] scrambler bypass, PHYSCR[9:8] FIFO 0/1/2/3 = 4/5/6/8 nibbles, CR2[2]. `tx10_configure`: 10BTSCR[13] lower threshold; [12:9] squelch codes 0..8 = 200..600 mV in 50 mV steps, reject 9..F; [7] NLP
  disable; [0] jabber disable; `0x002A[14]` 1=short, 0=long preamble. Reading 10BTSCR clears [4] and PHYSTS[12], so every RMW and readback goes through the task 20 collector." Change P34:19 to "tasks 17-20 and 33" and P34:30 to
  "Reuse task 33's CR2[1] control; add only the interaction test."
- **G5-4** S3 [A] P34:47-49 and :62 "reject RMII use" also blocks a zero-cause disable, the only documented way to turn off a detector on a BASIC RMII board (table 4-3: "No (Always enabled)"). **Fix:** "Reject any nonzero cause set on RMII
  before transfers. A zero-cause request and FLDS collection stay legal. CR3 FLD nibble reset is unknown: observe it and assert no fixed value."
- **G5-5** S3 [A] P34:25-28 omits footnote 5: modes 2, 3 and 5 meet maxima only with RX-error or energy-loss enabled. **Fix:** "Accept those profiles but report no maximum claim otherwise. Selecting MLT3 programs `0117=0417`; descrambler programs
  `0131[5:0]=08`, per 8.3.17. Other codes are explicit raw overrides under gate `fld_threshold_recipe`."

#### Prompt 35

- **G5-1** S2 [L] (with **G13-2**, D7) P35:30-31, :35-36 and :69 order work the ledger blocks and name only two gates. Table 8-50: "Bit [4] in Register 0x0027 = 1, Enables 10Base-T Test Patterns", and SNLA266B table 5-5 writes `0x0013`. **Fix:**
  replace `compliance_10_harmonic` with action family `compliance_10_pattern`: mask `0x001F`, value `0x0010|code`, code 0..0xD, rejecting E and F, with the existing forced-10M, hard-reset and 4A40 prerequisites. Narrow `compt_reserved` to bits 15:5 and the
  100M `0x0428` branch. P35 becomes: "Open gates: `ioctrl_example`, `analog_rx_gain`, `compt_reserved`. Eligible: 100BASE-TX standard, all 10M patterns, each with hard-reset stop. The 100M `0x0428` branch keeps typed validation and literal vectors but
  returns a gated rejection before any I/O." Change "all documented options" to "every eligible option, plus zero-I/O rejection of each gated option". Update `tools/verify_ledger.py`:105 and `test/test_remaining_ledger.py`:273, which pin the single `0x0013` action.
- **G5-6** S3 [A] P35:33-34: PHYCR `0021` also sets undocumented R/W bit 5 and clears bits 7 and 15; `7101` contains read-only BISCR bit 8. **Fix:** "Forced MDI/MDIX composes only PHYCR[15:14], preserves bit 7, does not write bit 5 from the script, and ignores
  RH [4:0]. In `7101`, write only BISCR [14:12] and [4:0]; no test expects bit 8 as written."
- **G5-8** S3 [A] P35:71-72 "entry/exit preserves" settings is impossible, since every SNLA266B recipe begins `0x1F=0x8000`. **Fix:** "Test that no session write sets BMCR[11:10] or targets PHYCR[4:0] except by explicit composition, that the session is refused while
  IEEE power-down or isolation is requested, and that after stop/reset the persistent profile is restored by one finite reapply and readback."
- **G5-9** S3 [A] P35:53-54 and :74 require calibration "completion" evidence that does not exist. **Fix:** "The contingency is a bounded CDCR read; if bit 14 is high, clear only `0x4000` with bit 15 written 0, then PHYRCR `0x8000`. Completion evidence is the hard-reset
  procedure's own; nothing proves calibration completed."

#### Prerequisites

- **G5-11** S3 [A] Add 11, 12, 15, 17 to P36; 17, 19 to P34; 24 to P35; 28 to P33.

### 7.2 WP4: prompts 37-42, native

<a id="g6-1"></a>
- **G6-1** S1 [L] (also **G11-1**, **G6-13**, D1) P37:10-17, DD:293 and :300-312 ("required dependency, not an alternative wrapper-only implementation"), ARCH:82-86, AC:1106, SDK:150-151 and :229 require forking `esp_eth` and `esp_hal_emac`, changing the
  public install contract. No document says how a consumer gets this, prompts 69-71 have no delivery path, and it contradicts ENG:22. P37:101-102 also conflicts by allowing a preflight path. Confirmed in pinned source: install ignores `set_mediator` and
  `reset_hw` results and proceeds to `mac->init`; `esp_eth_stop` uses non-blocking `esp_timer_stop`; `esp_timer.h` documents polling `esp_timer_is_active()`; `esp_timer_is_active` is true "if it is armed or its callback is currently running"; task dispatch is
  strictly serialized. Upstream commit `d2ed96d2` changes the stop to `esp_timer_stop_blocking(..., portMAX_DELAY)`, which SDK:208-211 dismissed, but that wait is bounded by the adapter's own `get_link`. **Fix:** replace P37:10-17, :98-105 and DD section 6:

  1. The owner drives board reset and waits readiness **before** `esp_eth_driver_install`, so REF_CLK exists for `mac->init` (sw reset polls up to 100 ms) and reset failure precedes MAC init. Adapter `reset_hw` returns OK with no action; `set_mediator` only stores.
  2. Adapter `init` is local and `autonego_ctrl(G_STAT)` returns desired AN without MDIO, so neither can fail. Any install error leaves only `mac->del` and `phy->del`. No cleanup-only handle exists.
  3. After install, the owner runs `startInitialize` and configure through the typed route (direct MAC frames, same batch lock). Failure takes the ordinary uninstall path.
  4. `esp_eth_start` error: call `esp_eth_stop`. `ESP_ERR_INVALID_STATE` from an unarmed timer means FSM stopped but link-down and the STOP event were skipped. If UP was published, call adapter `set_link(DOWN)`; if START was posted and no STOP followed, post STOP via `esp_event_post`.
  5. After every `esp_eth_stop`: call `mac->stop(mac)` to read the real HAL result (finite retry). Then run the fence: an adapter-owned ESP_TIMER_TASK one-shot, `esp_timer_start_once(fence, 0)`, poll `esp_timer_is_active` to false under a finite deadline. Timeout consumes nothing and resources stay held.
  6. Teardown: `esp_eth_del_netif_glue`; `mac->set_mediator(mac, &stub)`, where the stub frees RX buffers and `on_state_changed` returns OK; `esp_eth_driver_uninstall`; `mac->del`; `phy->del`. Check FSM and refcount first; past them uninstall cannot fail at the pinned source.
  7. A failure this route cannot resolve is terminal for this boot: keep MAC, PHY wrapper and borrowed resources alive, keep admission closed, report the record, and recover by system restart. The HAL FIFO-flush timeout (`emac_hal.c`:680) is unobservable: a documented SDK limit, not a gate.

  Delete "corrected SDK" wording everywhere. The fence, stub mediator and `mac->stop` re-call are source-derived, not executed: Q13.
<a id="g6-2"></a>
- **G6-2** S1 [L] P37:115-118, P38:87-91, P40:73, P42:54-58, VER:75-82 and DD:352-354 require failure injection on real SDK paths, but `esp_eth`'s `CMakeLists.txt` returns on target linux, a linked image executes nothing, and P4 runs belong to 72-73. Stages 37-42 cannot
  complete honestly. **Fix:** add evidence layer "Pinned driver source on host" to VER: a CMake test downloads and hash-checks unmodified `esp_eth.c` (same pattern as `tools/probe_sdk_mmd.py`, not vendored) and compiles it with the adapter against small fakes for esp_log, esp_check, esp_event,
  esp_heap_caps, esp_timer and FreeRTOS, with an instrumented `esp_eth_mac_t`. It proves real install, start, stop, uninstall order and ioctl routing; not timer concurrency, MAC behaviour or timing. Acceptance for 37-42 becomes failure injection PASS on this layer plus P4 `idf.py build` link. Keep example
  owner state free of SDK headers so 41 and 42 state tests run on host. The fake-header effort is an estimate.
- **G6-3** S2 [A] DD:117-131, AC:1626-1637 and :1660-1721 build poll loops, tokens and six journals for paths the SDK never calls: `phy->reset` and `phy->pwrctl` have no caller in `esp_eth.c`. **Fix:** `reset` and `pwrctl` return ESP_ERR_NOT_SUPPORTED and point at typed operations. Same for
  ETH_CMD_S_PHY_LOOPBACK if its procedure has waits. Delete `ownedReset`. Reservation becomes one `Phase` enum (Stopped, Control, Starting, Running, Stopping, Maintenance) in a phase-by-method switch. One record: `{stage, firstError, cleanupError, publishedTuple, publicationUncertain}`.
- **G6-4** S2 [A] P37:83-89, DD:190-198, DD:346, DD:359-363 and VER:79-80 require hook context resolution for hooks that always reject. **Fix:** two static functions, each `return ESP_ERR_NOT_SUPPORTED;`. Delete the P37:83-89 paragraph and the "two instances" tests. Test: a real ioctl returns the rejection with zero MAC frames.
- **G6-5** S2 [L] P38:49-60 describes admitted forwarding hooks; DD:184-186 and :232 reject everything. Pinned ioctl goes straight through the mediator. **Fix:** "Raw ioctls reach the mediator outside PHY methods. Hooks unconditionally reject them, including from the owner task. The owner-routed typed raw
  API uses the core access policy and calls MAC frame methods inside the batch." Fix P37:78-79 to match.
- **G6-6** S2 [L] P42:34-36 requires TX ownership "through actual completion"; the DMA path copies with `memcpy` and no completion callback exists. **Fix:** "`esp_eth_transmit(hdl, buf, len)` borrows `buf` only for the call. ESP_OK means queued to DMA, not sent. ESP_ERR_NO_MEM is backpressure; ESP_ERR_INVALID_STATE
  means cached link down. If `CONFIG_ETH_TRANSMIT_MUTEX` is off (default n), call from one task only. RX: `stack_input` owns `buffer` on every return, including error, and releases with `free()`."
- **G6-7** S2 [L] P40:40-41, `examples/esp_idf/README.md`:42 and `docs/esp32p4-board.md`:19-20 omit data-plane pins. The P4 default macro has MDC 31 and TX_EN 49, and this board's MDC is GPIO49, so overriding only the listed fields makes GPIO49 both MDC and TX_EN with no error. **Fix:** require in `BoardConfig.h`, with a startup check that all fields
  are set: `.smi_gpio={49,43}`, RMII interface, `.clock_config.rmii={EMAC_CLK_EXT_IN,44}`, `.emac_dataif_gpio.rmii={.tx_en_num=40,.txd0_num=41,.txd1_num=42,.crs_dv_num=51,.rxd0_num=52,.rxd1_num=53}`. State in the board doc that signal mapping is fixed by the P4 IO_MUX (`emac_periph.c`) and the netlist only confirms wiring.
- **G6-8** S3 [I] P42:39-41 and :46-48 give RX teardown no mechanism: the RX task keeps draining after stop and dereferences `emac->eth`. **Fix:** application context with static lifetime that outlives `mac->del`; nonblocking callback that drops and frees after session end; in-flight counter reaches zero under a finite deadline before `mac->del`; the
  stub mediator from G6-1; document that deletion in the pre-handoff window can leak at most one frame buffer.
- **G6-9** S3 [A] DD:87-96, P37:50-64 and P39:36-37 wrap `phy_802_3_t` though all I/O is overridden. **Fix:** `struct PhyHandle { esp_eth_phy_t phy; Phy* owner; };` with recovery by a first-member cast and one `static_assert`. Do not include the base header. Remove "base" from the conformance tables.
- **G6-10** S3 [A] P39:26-27 accounts for "10 ms reset polling, 100 ms AN polling", which the design removed. **Fix:** "These base loops do not run. Bounds come from 100 us MAC steps to nominal 1000 us per frame, the owner's own polls and `check_link_period_ms`."
- **G6-11** S3 [A] P38:70-71 guards "another instance's address", needing shared state that DD:195 and ENG:45 forbid. **Fix:** `set_addr` returns OK only for the same value, otherwise ESP_ERR_NOT_SUPPORTED. Retarget by delete and recreate.
- **G6-12** S3 [I] DD:281-289 and P39:52-54 force stop/drain on a full event queue. On P4 the only reachable failure is `esp_event_post(..., 0)` after MAC and cached link changed. **Fix:** "After `on_state_changed(LINK,x)` returns, record published=x regardless of result and never replay it. If it errored with a valid
  argument, set eventLost and let the owner re-post. An instrumented MAC error still marks publication uncertain and requires stop." Another MAC must re-audit.

### 7.3 WP5: prompts 43-55, CLI, runners, regression

- **G7-1** S2 [A] P44:47 and CLI cite "address-zero/broadcast" rules no document defines; 826 has no MDIO broadcast. **Fix:** "Discovery reads PHYIDR1/PHYIDR2 only, no writes, over a requested inclusive range. Default 0..7, the strap-reachable set; 0..31 needs an explicit argument. Address 0 is read like any other. `0xFFFF` or `0x0000` is 'no
  response evidence', not an error. A transport failure is reported per address." Isolation not stopping MDIO is general knowledge; check it.
- **G7-2** S2 [I] P43:61 "cannot interleave" is unachievable on P4 unless `esp_log` is routed. **Fix:** P47: "Install `esp_log_set_vprintf` so every log line uses the same bounded queue as a whole line. A machine record is queued as one unit. Records start at column 0. The runner treats a line as a record only if it starts with the
  prefix; garbled records are invalid, never repaired." Add one host test with log text between queue drains.
- **G7-3** S2 [A] P43:46-58, CLI:75-77 and P48:36-48 build a protocol layer with receive windows and reconfiguration; INA228 and TMP1x2 do this with begin/end tokens. **Fix:** "Firmware picks a u32 `boot` at boot. A `session` command passively prints `boot`, identity and schema version. Each record carries `boot=` and `req=`; `req` is a firmware
  counter per accepted line, rejecting on exhaustion until reboot. The runner has one outstanding command and treats any other `boot` or `req` as stale. Reconfiguration is not a feature."
- **G7-4** S2 [A] P48:58-72 and CLI mandate bundle trees and manifest cross-validation; `EvidenceFiles` in TMP1x2 is about 60 lines. **Fix:** "Exactly two files per run: `<run>.json` (verdict, cases, identity, fixture, UTC times, cleanup, transcript sha256 and length) and `<run>.txt`. Keep exclusive create and an INCONCLUSIVE placeholder; write JSON last by
  temp file and `os.replace`. Offline `verify` checks hash, length, schema, and that the verdict is the worst case verdict."
- **G7-5** S2 [A] P43:72, P47:26, P48:76-80, P50:62-65 and P51 depend on a host fixture with no owner or fault spec. **Fix:** P43: "One program `test/cli/dp8382x_cli_fixture` links the shared CLI plus a table-driven simulated backend, using stdin and stdout, with options `--identity`, `--partner`, `--fail-after N`, `--reboot-after N`, `--partial-record`.
  Register values are a plain array with only the behaviours a test needs. Campaign tests use recorded or synthetic transcripts for classification." P48's prerequisite becomes "fixture and schema from 43-46", so an SDK block in 47 does not stop 48-52.
- **G7-6** S2 [L] P44:70-72, P46:65-68, P47:52-58, P53 and P55:29-35 maintain an unnamed "exhaustive ledger". **Fix:** "`docs/cli.md` holds one table keyed by the 52 procedure and 35 feature ids: CLI command, owner route, host test name, physical case or N/A with reason. A unit test requires every id with non-empty cells. 44, 46 and 47 append; 47 closes; 53 and 55 add only gaps."
- **G7-7** S2 [A] P43:19-22 says "no frameworks" with 283 fields ahead. **Fix:** "Commands are one `static const` array of `{name, minArgs, maxArgs, flags, handler}`, with a second table of `{group, name, enumValues}` for `cfg set <group> <name> <value>`. Used by dispatch and help. No dynamic registration or virtuals. Each handler calls one public API."
- **G7-8** S3 [A] CLI rows, P44:44-48 and P46:35-40: discovery, dump, selftest, watch and stress have no API and conflict with "no CLI-only state machine". **Fix:** "These are CLI-local sequencers over public reads: a small fixed struct (index, count, deadline) advanced by `tick()`. Discovery reads through the application's transport callback and does not touch the bound core."
- **G7-9** S3 [A] P49:60-66 offers subprocess isolation. **Fix:** "Open with `timeout` and `write_timeout`. Never call `flush`/`drain`. If a platform cannot bound open or close, mark the backend BLOCKED; add no child process."
- **G7-10** S3 [A] P50:62-65, P51:65-66 and P52 imply a hand-written negative set. **Fix:** "Generate negatives by mutating the positive transcript: for each required field, remove, corrupt or duplicate it and assert the verdict is not PASS."
- **G7-11** S3 [A] CLI:12 "Prompt 43 freezes grammar" for commands that do not exist yet. **Fix:** "43 freezes tokenization, aliases, numeric parsing, group names, `<group> <verb> [args]`, the record schema and error vocabulary. 44-46 append commands."
- **G7-12** S3 [A] P48:38 and :42 contradict each other on ANSI; "trailer" conflicts with P43:55. **Fix:** "Records are single uncolored lines at column 0 with the prefix. The runner discards non-record lines and strips nothing from a candidate. Replace 'trailer' with 'record'."
- **G7-13** S3 [A] P53:28-29 and :41-42, P44:57-60, P54:71-74 re-require existing tests (`test/mmd.cpp` already asserts `0x001F`/`0x401F` and per-frame failure). **Fix:** "List existing tests by name for each item. Add one only when none covers it, and for cross-entry cases."
- **G7-14** S3 [I] P47:35, P49:25-31 and P50:44-46 assume console type and equipment; the board has no connected PHY interrupt, so IRQ cases on it are BLOCKED. **Fix:** P47 records console type (UART bridge or USB-Serial-JTAG). P50 lists the minimum partner in `docs/hardware-validation.md` and marks cases NOT RUN otherwise.

### 7.4 WP6: prompts 56-67, variants

- **G8-2** S2 [L] (with **G11-16**, **G8-10**, **G9-5**, D5) Shipped policy takes no part argument, and P56:45-47, P58:42-44 and P60:52-53 name no shape. 825 has no `0x002A`, `0x0117`, `0x0131`, `0x0218`, `0x0303`, `0x0304`, `0x0306`, `0x030E` or `0x04A5-7` in its table 7-1, yet `readable()` allows them; it adds `0x000C` and others. The coder would likely branch on `variant` in every function.
  **Fix:** add to P56, before any part:

  ```cpp
  // src/PartTable.h (private)
  enum class RegisterMap : uint8_t { DP83826, DP83826A, DP83825, DP83822 };
  struct PartProfile {
    Variant variant; RegisterMap map;
    uint8_t modelCount; uint8_t models[2];   // accepted PHYIDR2[9:4]
    uint16_t revisionMask;                   // bit n = revision n accepted
    uint8_t interfaceMask; uint32_t featureMask;
    bool hasOperatingMode; uint8_t strapAddressMax;
    bool forcedRequiresRobust;               // D5: true on DP83826/A only
  };
  ```

  `bind()` resolves `_part`. Policy functions take `RegisterMap` and search one array per map (AI/AE may share while their sets match). Procedures branch only on `map` at real differences. A new revision is a table edit. `requiredMode != UNKNOWN` on a part with `hasOperatingMode=false` is INVALID_CONFIG. No per-part files. Split `validateLinkSettings`: enum, mask and cross-field only; robust
  and VOD rules move to the part-keyed preflight.

#### Prompts 56-59: DP83826E and AI/AE

- **G8-1** S2 [A] (with **G9-19**) P56:35-41 repeats a complete audit for a part with no documented software-visible difference; E and I differ only in temperature and TA-keyed leakage rows, and AE/AI are alike. **Fix:** "One document-level cited record: one register map and procedure set with no suffix-conditional content. Add `Variant::DP83826E` sharing I's profile. Reopen only if a new TI revision adds a suffix
  condition. Parameterize one identity and bind test." Add the exception to FC:170 and use it for AI/AE. Shrink `docs/variants.md`:77 to "temperature only; no suffix detection from shared ID".
- **G8-3** S2 [L] P59:23-37, P59:46-47 and P57:26-35 say "shared procedures" but omit field moves. A table 8-86 confirms `15-12 RESERVED`, `11-8 LED3 Control`, reset `5565h`, versus I/E `5665h` with LED3 at [15:12]. Reusing P32's masks makes LED1 writes land on A's LED2 and LED3 writes on reserved. **Fix:** seed P57 with these [A unless noted]: LEDCFG LED3 [11:8], LED2 [7:4], LED1 [3:0], [15:12] preserved [L
  for reserved and LED3]; LEDCFG2 defines LED3 polarity, drive value and enable where I/E has reserved; IO_CFG1 reserves `cfg_clkout25m_off`; SOR1 adds bit 11 Strap11; FLD_CFG2 reset `0x2284` to `0x0288`, threshold 4h to 8h; CR1 robust, CR3 bit 10 and PHYSCR[1:0] are strap-dependent; RXFCFG1 reset `10X1`; FLDS RC to RCH; 8.3.17 Note requires MLT3, low SNR and descrambler to be enabled with energy loss or RX error; the 8.3.3.1
  recipe differs entirely, with `0416=1830`, new rows and `0466=FD00`. Assign LEDCFG, LEDCFG2, IO_CFG1 and EEE to 59; identity, CR1, CR3, PHYSCR, FLD note and SOR1 to 58. Add literal tests.
- **G8-9** S3 [A] P57:59-60, `docs/variants.md`:67-69 and FAM:55: Strap11 wording omits "if FLD is used"; SDAA067A table 3-3 option 5 says Strap8 HIGH while its Figure 3-2 and A table 8-5 say LOW; literal `0x000B=0x0001` clears pair and polarity swaps. **Fix:** reword; record the Strap8 conflict (datasheet governs); "Software writes only mask `0x040F`, preserving bits 6:5. Report Strap11 from SOR1 bit 11. No EMC claim."

#### Prompts 60-61: DP83825I

- **G8-4** S2 [A] P61:25-27 and FAM:79-84: `0x000C` (Register_12, status RC) is unnamed, while table 7-21 marks MISR1 status "R" and table 7-19 says reading clears. **Fix:** "Treat `0x0012`, `0x0013` and `0x000C` as read-consuming, owned only by the collector and excluded from `readable()`. Record the conflict and unknown latch relation. Collect through MISR1/2 only. Test consumed-first-sample then failed second read."
- **G8-5** S2 [I] P60:30-31, :37-38, :63 and FAM:110 frame RCSR[7] as selectable, but the strap sets the role. Table 7-26 reset 0h conflicts with SDAA081, which reads it as strap confirmation. **Fix:** "RMII role is a board declaration, required on DP83825I (LEADER or FOLLOWER). Observe SOR1[3] and RCSR[7]; disagreement is a reported mismatch with no write. No independent clock selector. Do not assert bit 7 reset equal to 0h; vectors are strap-parameterized."
- **G8-6** S2 [I] P61:36-38 and P60:58-59: in Leader mode (the strap default) the PHY sources the MAC's clock, and generic LED/GPIO setters or isolate can remove it. That nonzero `0x0305` replaces the clock is inferred from table 7-61. **Fix:** "In observed Leader mode (SOR1[3]=0), reject nonzero `0x0305[2:0]` before callbacks unless the application passes an explicit clock-loss opt-in. Gate BMCR isolate the same way. Add zero-callback tests and Q15."
- **G8-7** S3 [A] P61:30, :36-38, `docs/variants.md`:79 and P60:69-71: 825 tables 6-5, 6-6 and 6-7 define whole-word modes; `0x04D1` bypass polarity is inverted from 826; `0x030E`/`0x0333` and `0x0218` are used but absent from table 7-1. **Fix:** "Functional modes as one finite typed mode procedure; `0x04D1` polarity per 825; `0x030E=0x4A40` never runs on 825; `0x030E`, `0x0333` and `0x0218` are exact-address allowlist entries with a source note."
- **G8-8** S3 [A] FAM:86 and P61:23-25: Secure-ON is claimed but `0x04A5..7` are absent from 825's table. **Fix:** "Named source gap; implement only under an explicit Linux/SNLA261A corroboration record with physical verification required."
- **G8-12** S3 [L] CFG:251-252 and P60:45-47: 825 T4 MDC is 20 MHz max; its prose says 24 MHz. **Fix:** "Record the conflict; document 20 MHz as the conservative DP83825I limit."
- **G8-10** S3 [A] Merged into G8-2 and D5. 825 table 7-12 has no robust requirement.
- **G8-11** S3 [A] Merged into G4-5.

#### Prompts 62-67: DP83822

- **G9-1** S2 [L] P65:31-32 and FAM:130: table 7-10: "FX_EN strap must be enabled for SD_EN strap to be functional." **Fix:** "SD is strap-only. `sdActive = SOR1[11:10] in {01,10} AND SOR1[9:8] in {10,11}`. CR2[14] alone never activates SD, and RX_ER keeps its AMDIX_EN meaning unless the FX_EN strap is set. FIBER GENCFG `0x0465[0]` sets polarity only (1=active low). Report FX source and SD state separately. Without active SD, electrical-link loss recovers only by soft reset (7.4.4.2)."
- **G9-2** S2 [L] P65:39-40 and :63-64: SNLA437A 2.3.4: "In Fiber, this bit does NOT toggle when the link status changes. Soft-Reset must be performed (Set Register 0x001F = 0x4000) before reading". **Fix:** "In FX, BMSR[2] and its PHYSTS[0] duplicate are not change-driven: mark the value as not change-tracked. Provide an explicit maintenance-admitted refresh: `0x001F=0x4000`, bounded wait, read. Never issue it from polling, snapshots or the SDK timer. Fakes model a non-toggling BMSR." API shape is a suggestion; Q14.
- **G9-3** S2 [L] FAM:131, P62:32 and P63:29-30 record only a label dispute. Real traps: BMCR says "Straps are re-latched only during power up and pin RESET_N", while table 8-30 and 7.5.1 add PHYRCR[15]; TI prose says "software reset" and means `0x4000`, while table 8-30 names bit 15 "Software Reset". **Fix:** four domains, with recipes using literal `0x4000`. RESET_N/POR: straps latched, all reset. `0x001F[15]`: pin-equivalent, treat straps as re-latched, record the internal conflict. `0x001F[14]`: digital restart, registers retained, what TI recipes call "software reset". BMCR[15]: PCS only, vendor registers kept, no strap latch; `0x3000[15]`: MMD3/MMD7/PCS.
- **G9-4** S2 [L] FAM:138 drops a write and P67:32 mislabels a dependency. 822 7.3.3: "Program (Register 0x0456 = value 0x000A)", "Program (Register 0x04A0 = value 0x1080)", WoL "is not a must change", then `0x4000`. **Fix:** "After IOCTRLx selects RX/TX SFD: `0x0456=0x000A` (recipe-only; bit 1 is reserved in table 8-78, so allowlist it and preserve bit 3=1), optionally RXFCFG bit 7 composed with owned WoL fields, then `0x001F=0x4000`." P67: "optional WoL-enable accuracy aid". Assign PTPPSEL `0x003E` and PTPCFG `0x003F` to P67.
- **G9-5** S2 [A] Merged into G8-2 and D5. FX ignores crossover and writes no CR1, PHYCR or CR3 MDIX bits. Add a DP83822 revision rule; `DeviceIdentity.mode` stays UNKNOWN there.
- **G9-6** S2 [A] P64:30-35 "source-proved order" does not exist. `0x4300` means RX_D3 = 50 MHz output and LED_1 tri-state, per table 8-81. **Fix:** "Issue the literal word only on a software RCSR[5] 0-to-1 transition, never for strap-selected RMII. Reject before I/O if an owner holds GPIO3 fields [14:8]. If an owner holds GPIO1 [6:0], reapply them after the `0x001F` write by masked write, labelled INFERRED, physical NOT RUN."
- **G9-7** S2 [L] Addressing part merged into G4-1. **Fix for 822:** add to P62 or P67: "Printed `0xDnnn` on 822 maps to DEVAD=D, offset=`0x0nnn` (7.4.2.9). Admit DEVAD 3 and 7 with the part-keyed policy. Literal vector: `0x0007, 0x003D, 0x4007`, read. The DP83826 printed scheme differs: do not share constants."
- **G9-8** S2 [A] Merged into G4-5. Also: "Acknowledge on 822 = write owned config `| 0x0800` (table 2-4 step 6); all other writes keep bit 11=0."
- **G9-9** S3 [A] Merged into G2-12.
- **G9-10** S3 [A] FAM:100-101 sends the coder to table 8-87 for bit 6, whose whole text is "Bit Mask Flag". **Fix:** "No documented semantics. Preserve it, provide no typed setter, record as OPEN."
- **G9-11** S3 [A] FAM:135-136 and P66:26-28: amplitudes are 7 bits (bit 15 and bit 7 reserved), while SNLA253 says 8. **Fix:** "Mask `0x7F` per tables 8-60..8-64; locations are 8-bit; peak map per tables 8-55..8-59."
- **G9-12** S3 [A] P66:26: CDCR Done resets to 1, so polling Done alone completes immediately. **Fix:** "Manual completion = bit 15 read back clear AND bit 1=1; bit 0 = fail. Test a fake with Done=1 before start."
- **G9-13** S3 [A] FAM:114 and P64:40 omit `0x0462[6:4]` GPIO1 clock output. **Fix:** add it (active with [2:0]=011), mutually exclusive with FX SD on LED_1.
- **G9-14** S3 [A] P65:27-28 and FAM:130 omit `0x0040` Fiber Far-End Fault. **Fix:** add bits 13, 6 and 5 with preserve-reserved RMW, plus LDCTRL[11:8] FX swing from P66.
- **G9-15** S3 [A] P62:57-58 and P64:56: "no arbitrary tap range" could hide TXCPSR `0x0042`. **Fix:** scope to "RGMII RCSR[12:11]"; add TXCPSR as an MII TX_CLK phase control owned by 64; record the [4:0] versus [3:0] wording conflict.
- **G9-16** S3 [A] P65:27 and FAM:130: "GENCFG 0x0465" collides with real GENCFG `0x0456`. **Fix:** use `FIBER_GENCFG` and `GENCFG` everywhere, with literal tests.
- **G9-17** S3 [A] `docs/variants.md`:63-64, FAM:130, P62:58 and P65:45 cite 7.4.4/7.4.6 for MAC independence, which say nothing about it. **Fix:** cite section 4 and 7.4.1.1/7.4.1.2 for MII and RMII. Record FX+RGMII as "no documented exclusion, not stated, physical NOT RUN".
- **G9-18** S3 [A] P65:15 and :66 refer to a medium selection no stage builds; one `linkModeMask` cannot express copper and FX on one part. **Fix:** outputs of P65: `Config` medium enum (COPPER, FIBER), per-medium link mask, observed medium from CR2[14] with SOR1 evidence, CLI grammar, adapter admission.
- **G9-19** S3 [A] Merged into G8-1. 822 subtype deltas are exactly fiber capability and temperature: "one DP83822 ledger with `parts: [I,H,IF,HF]`, two attributes per Variant, per-part rows only for admission and physical results."

### 7.5 WP7: prompts 68-80, packaging, CI, review

- **G10-1** S2 [L] This host has no `idf.py`, `IDF_PATH` or Espressif tools, and no prompt provisions them; remote CI is banned for 1-70, so SDK builds cannot defer. **Fix:** add to P37: "If no pinned ESP-IDF is installed, clone tag v6.1 (commit `fff9895c`) outside the repo with submodules, run its `install.ps1 esp32p4`, and record path, commit and tool versions. Later prompts reuse it. If installation is impossible, SDK layers are BLOCKED; no header shim."
- **G10-2** S2 [I] P71:46-52 and :62: no `.github` exists, and GitHub only offers `workflow_dispatch` for a workflow on the default branch, so `gh workflow run` fails. There are no triggers to "preserve". **Fix:** "`ci.yml` triggers on `push` (all branches), `pull_request` and `workflow_dispatch`. Get the run via `gh run list --commit <sha>`. Do not merge to `main` to enable dispatch. The rule applies only to what 71 creates." Same in `AGENTS.md`.
- **G10-3** S2 [L] (D12) `scripts/generate_version.py`:49 rejects any `library.json` field beyond five, so P69's packaging fields would fail `check`. **Fix:** state option (a): `library.json` is version truth only.
- **G10-4** S2 [A] (D12) P70:15-16 and :54-56, `AGENTS.md`, P77 require a wrapper that does not exist, for a consumer with no backend. **Fix:** "PlatformIO is not a supported consumer in this execution. Do not adopt SCD41's `platformio_configuration`. Create `scripts/pio.cmd` only if a prompt actually runs PlatformIO." Remove it from P77's list.
- **G10-5** S2 [L] `CMakeLists.txt` has no install, no `project(VERSION)` and no IDF branch. P69:40-48 "keep IDF component consumption separate" is ambiguous. **Fix:** "`if(COMMAND idf_component_register)` selects the IDF component including `src/esp_idf/`; otherwise the portable project plus `install(TARGETS ... EXPORT)`, `install(DIRECTORY include/)`, `DP8382xConfig.cmake` via `CMakePackageConfigHelpers`, namespace `DP8382x::`. `project(VERSION)` is kept consistent with `library.json` by `check`. Do not use `FILE_SET` (needs CMake 3.23; minimum is 3.16)."
- **G10-6** S3 [A] P69:30-36 and :60-67 re-require what prompt 10 ships and tests. **Fix:** "Prompt 10 covers these. 69 adds only new outputs and their fixtures."
- **G10-7** S3 [A] Files named but not created: `docs/validation.md`, `CONTRIBUTING.md`, "fixture manifest", `ciTestedCommit`. **Fix:** delete "validation" from 72-80 and VER:15; "Create CONTRIBUTING.md" in P69; P72 "extend the artifact manifest and fixture selection from 48-52"; reword to "the tested SHA".
- **G10-9** S3 [A] P68:55-56 "second available compiler": only MinGW g++ exists, and ASan is unavailable. **Fix:** "Record NOT RUN locally; the second compiler and sanitizers run in the Linux job at 71. Do not install toolchains for this."
- **G10-10** S3 [A] P68:24-45 creates a third example location and puts assertions in an example. **Fix:** extend `examples/portable/`; keep `test/consumer` as the link check; put independence and failure assertions in registered tests.
- **G10-11** S2 [A] (D13) P36, P55, P75, P78 and P80 each reconcile every register and field; P79 repeats P53-P55. **Fix:** "55 owns the single coverage matrix, in COV. 75 and 78 verify it by sampling and independent re-derivation of high-risk rows only. 79 consumes 53-54 results and reviews only the delta. 76 is limited to checker adverse tests and Doxygen. An unchanged sibling revision table may be reused across 68-80. Record commands and counts in the stage report only, with a progress-row link."
- **G10-12** S3 [A] P72:65-68, P73:65-66 and P80:18-20: the progress doc has no status for a physical gate. **Fix:** add legend values: "72/73: BLOCKED (software and readiness done, physical NOT RUN). 80 may be COMPLETE-with-open-gates, and the final handoff says the set is PARTIAL."
- **G10-13** S3 [A] `.gitignore` misses `sdkconfig.<target>`, `dependencies.lock` and `*.log`; repo-wide `*.bin` would hide a fixture. **Fix:** add `sdkconfig.*`, `!sdkconfig.defaults*`, `dependencies.lock`, `*.log`, and `!test/fixtures/**` if binary fixtures are planned.
- **G10-14** S3 [I] P70:23-24 and :45 name no archive emitter, and 71 is the first time Linux compilers see the code. **Fix:** P69: "`git archive --format=tar.gz` of HEAD with `.gitattributes export-ignore` for `docs/reference/source`, `docs/reference/text`, `docs/reports`, `test/`, `tools/`. `tools/check_package.py` validates exactly this." P71 notes that first-run Linux failures are expected and fixed there.

## 8. Physical qualification cases to add

These extend prompts 72-73. They need identified authorized fixtures; none has run. Each protects a decision above.

| Case | What to observe | Protects |
| --- | --- | --- |
| Q1 | EEE wire probe, after hard reset, no partner: read `(3,0x0014)` and `(3,0x1014)`; read `(7,0x003C)` and `(7,0x203C)`; write `(7,0x003C)=0x0002` and read both; only if the low-offset readback is not 2, write `(7,0x203C)=0x0002` and read both; hard reset. Never read 3.1 or 3.22 here. Low works: gate closed. Only full works: change the constant table. Neither: blocked, ask TI. End to end at 73: `(7,0x003D)` bit 1 = 1 against an EEE peer. | D2, G4-1 |
| Q2 | LPI on the line or partner after `0x108B` | G4-8 |
| Q3 | RXFCFG1: level mode, valid Magic Packet, write neutral `0x1981` (level holds?), write `0x1181` (level clears?) | D16, G4-3 |
| Q4 | `modifyMmd` on LEDCR, then a direct `readClause22(0x18)` comparison | D16, G12-8 |
| Q5 | PHYRCR hard reset: first valid read time, and strap re-latch against an identified strap change | D14, G1-3, G1-10 |
| Q6 | BMCR[11] after pin release in ENHANCED mode | G4-4 |
| Q7 | Both TDR recipes against known cables, open and short, with measured distance | D3, G3-1, G3-2 |
| Q8 | Reverse and analog loopback with and without restart | G3-3 |
| Q9 | INT pin level versus pulse, and MISR latching while its enable is 0 | G3-4 |
| Q10 | Next-page exchange and re-AN against an NP-able partner | D8, G2-3 |
| Q11 | Raw PHYSTS in software-forced modes; CR1[5] with manual MDI; CR3 `0x0060` with PHYCR automatic | D4, D5, G2-10 |
| Q12 | Whether LED0 source or polarity depends on PHYCR[5] | G5-7 |
| Q13 | On P4: timer fence, `mac->stop` re-call, stub mediator, RX teardown | D1, G6-1, G6-8 |
| Q14 | DP83822 FX: BMSR behaviour on link change, and the `0x4000` refresh | G9-2 |
| Q15 | DP83825I: `0x000C` latch relation, identity A140 or A150, `0x0305` clock loss in Leader, Secure-ON at `0x04A5..7` | G8-4, G8-6, G8-8 |

## 9. Coverage and limits

**Count: 172 findings from 13 scoped auditors plus one lead finding (M-1). Five distinct S1s remain once G11-1 is merged into G6-1.** Duplicates are merged above, and every id appears in this document. Of the catalogue entries, 49 are marked [L], 89 [A] and 18 [I]. All five S1s are [L].

**Read completely and compared with primary sources:** prompts 13-80, the shared contracts, AC, DD, ARCH, the ledger, the catalogues and all shipped code. All 80 prompts have working links and anchors; prerequisite references never point forward, though some omit stages (G5-11). 21 prompts use a "Required inputs" sentence instead of a prerequisite line, which is a format difference, not a defect.

**Checked and correct, so do not re-audit without cause:**

- 826 ledger data as stated in section 1, and all literal vectors in RV independently recomputed, apart from G13-7's labelling.
- Identity: I/E `0x2000A111`/`0x2000A131`, revision 1; A model 0x11/0x13, revision 4; 825 `0x2000A140`, revision 0; 822 `0x2000A240`, revision 0. Mode is set by pin, not software.
- Timing: 25 us reset pulse without debounce caps, 2 ms and 50 ms MAX, FLP 1500 ms and EEE values NOM only, FLD 10/11 us MAX, MDC 2.5 MHz NOM and 24 MHz MAX on 826.
- The 18-pair EEE recipe, bypass polarity, REGCR words `0x001F`/`0x401F`, LEDCR mask `0x0692`, the shipped `readable()` classification, and MLEDCR mask `0x0078`.
- WoL masks, MAC and password packing, BISCR and BIST fields, CDCR masks, MISR pairs, FLD fields, VOD codes, RMII and RCSR fields.
- 822 RGMII: bit 12 1=RX on, bit 11 0=TX on, both 3.5 ns; vectors `0x0800/0x1800/0x0000/0x1000`.
- SDK claims in SDK: flow-control ioctl width, no full-advert ioctl, raw ioctl bypass, `auto_nego_en` cache, peer pause Boolean, repeated UP resets DMA descriptors.
- All 17 shipped enums match AC, and every AC code block type-checks with g++ 15.
- Sibling paths and symbols named by the prompts exist. Prompt 25's exclusion of SNLA330A and SNLA253 stays correct; they are other parts.

**Not checked:**

- Physical behaviour. No PHY or P4 was used, and no case in section 8 has run.
- IEEE 802.3 originals are not in the repository; those claims come from knowledge and TI tables that mirror them.
- ESP-IDF was reviewed statically: no build or run, no QEMU, no install on this host. G6-1's sequence is source-derived.
- ASan was unavailable on this toolchain; UBSan ran in trap mode only.
- Many table cells for the A, 825 and 822 parts were read from text extracts, as marked by [A]. The lead re-read the [L] items only.
- TI E2E threads 924830 and 792015 returned HTTP 403; TI errata were not searched again. A full 825 versus 826 field comparison belongs to prompt 60.
- Behaviour of sibling code beyond existence of the named symbols.
