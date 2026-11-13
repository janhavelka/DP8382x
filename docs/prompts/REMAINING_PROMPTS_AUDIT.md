# Audit of remaining prompts 13-80

Date: 2026-11-02. Baseline commit: `e105af6`, branch
`promptset/dp8382x-implementation-20261028`. Re-audited finding by finding the same day; the
record is in [section 11](#11-re-audit-record). Prompts 1-12 are complete; 13-80
have not run. Apart from links to this file in COMMON and the prompt index, no prompt,
contract, ledger or code has been changed yet. This audit tells the coder what to change,
in which order, before executing prompt 13.

**The owner delegated every open choice to this audit on 2026-11-02.** The decisions in
[section 3](#3-decisions-final) are final; apply them without asking again. Reopen one only if
a primary source contradicts it. Start with [section 4](#4-work-packages); use
[section 10](#10-per-prompt-index) to see what applies to a given prompt.

Application record (2026-11-07): WP0–WP7 corrections are integrated with
[prompt 13](../reports/implementation/13-side-effect-register-access.md).
The opening baseline and findings below remain historical review evidence.
The latest explicit prompt-13 request supersedes the collector deferral: its
current acquisition and mixed-register consumers are implemented now.

## 1. Result

The library is a framework-independent C++17 driver for TI DP8382x Ethernet PHYs,
DP83826I first, plus a native ESP-IDF adapter for the ESP32-P4. The shipped code is
bounded Clause 22 and MMD register access with terminal batch transport.

**The foundation is sound:**

- The register ledger matches SNLS647I: 79 of 79 detailed registers and all 283 field
  records, which cover 284 printed rows because BISCR[13:12] is one coded field, with no
  address, reserved-mask, reset or access error. Two independent extractions agree.
- The shipped code has no silicon-map error, no overflow and no publication of
  unvalidated values, apart from one small report-handling gap (G12-5). 14 of 14 CTest
  suites, 98 Python tests, the reference, ledger, version and Doxygen checks pass, re-run
  independently. Fuzzing found no invariant violation, and an independently written
  refactor was hash-identical to the shipped code over 3M operations.
- Sibling paths and symbols named by the prompts exist, apart from combined `.h/.cpp`
  spellings (M-1).

**The problems are in the instructions, not the data.** Five distinct S1 findings
would ship wrong behaviour or block a stage:

| ID | Problem | Consequence if unfixed |
| --- | --- | --- |
| [G2-1](#g2-1) | Forced mode can never report a resolved speed/duplex | The ESP-IDF adapter can never publish link UP on a forced link |
| [G3-1](#g3-1) | The DP83826 TDR note, SNLA428, is not archived or used, although the datasheet defers to it | Prompt 25 ships a cable test without TI's gain writes or 1A/1B runs |
| [G4-1](#g4-1) | The EEE wire-address gate is overstated | EEE and LPI (prompts 29-30) stay blocked, and tests enshrine rejecting the likely correct address |
| [G6-1](#g6-1) | Native support requires a private ESP-IDF fork that nothing can deliver (also G11-1) | Prompts 37-42, 47, 70 and 74 stay blocked |
| [G6-2](#g6-2) | Native failure tests have no layer on which they can run | Prompts 37-42 cannot honestly complete |

The largest maintainability risk is the operation contract. No operation code has shipped
yet, so this is the cheapest moment to simplify it. [Section 6.3](#63-target-operation-contract)
gives the complete replacement.

In total: 172 auditor findings, one lead finding and six added by the re-audit, about 60 S2 and
90 S3 alongside the S1s above. See [coverage and limits](#9-coverage-and-limits).

## 2. How to use this document

Apply the work packages in section 4 in order, before prompt 13. The coder may
reorder inside a package.

Rules while applying:

- A proposed text here is a direction, not evidence. Reopen the cited source for the
  field you are changing. If this audit and the exact-part datasheet disagree, the
  datasheet wins: record the disagreement and tell the owner. D3 is the one deliberate
  exception, for the TDR recipe words only.
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
`tools/verify_ledger.py` (lines 77, 105, 116-120, 155-156, 528-530, 578, 596-606),
`test/test_remaining_ledger.py` (lines 194-198, 259, 273, 286, 357, 377, 389, 394-419) and
`test/test_ledger.py` (line 273), and change them with the ledger on purpose. No validator
pins stage numbers in `owner.test` or gate `nextEvidence` text, so a missed edit there
will not be caught.

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

## 3. Decisions (final)

| # | Decision | Why | Rejected alternatives | Accepted cost |
| --- | --- | --- | --- | --- |
| D1 | Native support runs on **unmodified ESP-IDF v6.1** through public headers (G6-1), including an undocumented but source-checked `mac->stop` and `set_mediator` sequence that must be re-audited per SDK revision. A failure the public route cannot resolve is terminal until reboot (G11-1). Once the baseline contains upstream `d2ed96d2`, drop the fence and re-audit step 4, since stop no longer returns INVALID_STATE for an unarmed timer. | Every required behaviour maps to public headers except a few unobservable results, chiefly the HAL FIFO flush. A fork has no delivery route and contradicts ENG:22. | A private fork. Pinning unreleased master: still swallows MAC/HAL errors and leaves the release baseline. Terminal-only with no fence: a fence is cheap and gives real quiescence. | No in-process retry after a failed teardown; errors the SDK swallows stay invisible. |
| D2 | Adopt the EEE tuples `(3,0x0001) (3,0x0014) (3,0x0016) (7,0x003C) (7,0x003D)` as **source-implied**, in one constant table, with a fail-closed runtime self-check and planned probe Q1 (G4-1). Rewrite both gates, `eee_wire` and `eee_control_semantics` (N-1). Raw reads admit exactly `(3,0x0014)`, `(7,0x003C)`, `(7,0x003D)`. No raw writes. Every other DEVAD 3/7 offset is rejected, including the destructive `(3,0x0001)`, `(3,0x0016)` and any `0x1xxx`/`0x2xxx` form. | The evidence chain is strong. A wrong `(3,0x0014)` or `(7,0x003C)` errors before the opaque rows; the other three tuples inherit the same scheme unchecked until Q1 and prompt 73. | Blocking on a TI clarification that may never come. Treating the tuples as proven. | Status stays "physical check pending" until Q1 runs; add that status to `tools/verify_ledger.py`:596. |
| D3 | Default TDR recipe is **SNLA428 table 2-1**. Keep the datasheet words as independent vectors and record source conflict `tdr_recipe` (G3-1). This is a deliberate exception to section 2's "the datasheet wins" and to the G11-17 order, for these recipe words only; tables 8-56/57 do not reopen D3. All six CDSCR words select transmit A, same-channel capture, so the recipe covers channel A only. | 826 8.3.16.1 defers to this note "for details" and 10.1 lists it. The datasheet was revised after the note and still prints different words, so which words perform better is unproven; the choice rests on the delegation and on the note's offset, thresholds and formula belonging to its recipe. | Datasheet words only: no gain writes and no 1A/1B runs. | Physical comparison stays NOT RUN (Q7). |
| D4 | With AN disabled and link up, **BMCR is the forced speed/duplex source**; PHYSTS low bits stay raw evidence (G2-1). | Table 8-19 says bits 13 and 8 determine the mode; the PHYSTS sentence contradicts the part's own strap table. BMCR readback under ENHANCED straps is source-implied, not stated. | Keeping the gate: forced mode could never publish UP. | Strap-forced BMCR readback unmeasured until Q11. |
| D5 | Keep `robust=true` for every forced mode on DP83826 and DP83826A. Move the rule from the part-agnostic validator into the part table at prompt 56 (G2-4, G8-10, G9-5). | Satisfies "shall" under either reading; 825 and 822 have no such rule. | Relaxing it for manual pairs: unsupported by the table. | None. |
| D6 | `startInitialize` writes and verifies `VOD_CFG3=0x4A40` **unconditionally, for every ResetKind**. There is no verify-only path. Owner stage 15 (G1-2, G2-9, G11-10). | Strapped ANAR can advertise 10M from POR; all its fields are scoped to 10M mode. | Per-profile application: tracking for 100-to-10 transitions. | A write on 100-only boards. |
| D7 | Un-gate 10M compliance patterns `COMPT=0x10..0x1D`. Keep the 100M `0x0428` branch gated (G5-1, G13-2). | Table 8-50 documents them and `0x0013` confirms the reading. `0x0428` still has no register table or read-effect record. | Keeping `0x0013` as the only exception. | None. |
| D8 | Implement next-page exchange from IEEE Clause 28, with NP=1 only for one admitted epoch (G2-2, G2-3). Remove `nextPage` from persistent `Advertisement` (N-3). Gate `next_page_handshake` becomes physical case Q10. | 826 8.3.1 designates Clause 28, and the gate's evidence cannot arrive during software work. | Rejecting next page indefinitely. A persistent NP bit: it can stall every later AN. | Partner behaviour unmeasured until 73. |
| D9 | **Remove the CLEANUP phase.** A failed or cancelled operation ends at once; restoration is an ordinary later operation behind one barrier flag, with one escape call `abandonCleanup` (G11-2, G11-3). See OP-9 and OP-10. | The largest cost in the state machine, for a route mostly never run. ENG:110 already says "separately admitted"; AC's "reserved at start" is the outlier, and AC:1354-1355 concedes it guarantees nothing. It also allows an application retry, which the old design forbade. | Keeping CLEANUP with fewer fields: still two budgets per operation. Dropping the barrier: other work could run over an active stimulus. | No single token across both phases; the owner makes one more call. |
| D10 | **Remove pre-mutation reserve sums and per-phase poll allowances.** Keep one fixed rule before a reset command (OP-6) and one stall counter `maxStalledPolls`, default 100000, for polls that issue no batch and see no clock advance (OP-5) (G11-6, G11-8). | Frames already bound I/O, and normal polling must not time out. Only a frozen clock needs a separate limit, which ENG:104 requires. | Deleting poll limits outright: a frozen clock would then run forever unless the owner steps in. Counting batch polls too: redundant with frames. Keeping allowances: false timeouts on healthy hardware. | Less early rejection of a too-small budget, outside reset commands. |
| D11 | **Two entry points:** `startInitialize({reset})` and `startVerify`. Remove `startReset`, `startRecover`, `RecoveryMode` and `ResetOptions`, with no alias (G11-12). See the mapping in section 6.3. | One private plan had three entries and precondition rules. CONV:107 says "one method for one meaning". | Thin aliases: more names, docs and tests for no behaviour. Keeping peer spelling `startRecover`: reads well but implies a second procedure. | Differs from peers' `startRecover`; record this under CONV. |
| D12 | PlatformIO is not a supported consumer in this execution; `library.json` keeps its five version fields plus the owner `authors` list, which the generator requires exactly (G10-3, G10-4). | No backend to link against and no consumer. | Relaxing the generator's five-field rule for an unused path. | Add it later with a real target. |
| D13 | Keep prompt numbering. COV holds the one coverage matrix: 44, 46 and 47 append CLI columns, 55 closes the primary part, 59, 61 and 67 append exact-part rows, and 75 and 78 sample it (G7-6, G10-11). The rule to re-audit siblings at each stage stays: it is a recorded owner requirement. | Five passes over one denominator produce reports, not fixes. Renumbering would break many links. | Merging prompts 74 and 75. | Less re-derivation at each review stage. |
| D14 | Apply a 2 ms frame-free floor after any attempted PHYRCR hard reset, for every kind of frame (G1-3, OP-14). | Table 8-48 equates it to the reset pin, and 2 ms costs nothing. | Polling after 1 ms: risks a false "reset complete". | None measurable. |
| D15 | Reserved-bit handling is per register, not global zero (G13-1). | Fourteen registers have non-zero or unknown reserved reset bits; nine have R/W reserved bits a generic write could reach. | A global zero or a global preserve rule. | One table column. |
| D16 | Keep exact-part W0C for RXFCFG1[11] and the five-frame RMW, and qualify both (Q3, Q4) (G4-3, G12-8). | Datasheet authority stands, and family parts differ. | Importing the DP83822 write-1 recipe. Reverting to eight frames with no evidence. | None. |
| D17 | Also adopted without alternatives worth keeping: G11-4 clock-lag meaning, G11-5 poll signature, G11-7 health predicate and no publication reserve, G11-11 direct-call matrix, G11-13 small result, G11-14 three transport bounds, G11-15 microseconds everywhere, G11-16 no `RevisionPolicy`, G11-20 no unused codes or token exhaustion, G11-22 snapshot names. | Each removes state or a rule, with the one small loss noted as a cost. | Keeping them for peer-name parity. | Peer field names `lastOkMs`/`lastErrorMs` become `...Us`. A success can be reported up to `clockLagUs` after the true deadline. |

## 4. Work packages

Run the standard checks after each package: `python tools/verify_references.py`,
`python tools/verify_ledger.py`, `python -m unittest discover -s test -p "test_*.py"`,
`python scripts/generate_version.py check`, the CMake host and consumer builds with
`ctest --no-tests=error`, `python tools/check_doxygen.py` and `git diff --check`.

| WP | Scope | Findings | Done when |
| --- | --- | --- | --- |
| WP0 | Evidence, truth, ownership | [Section 5](#5-wp0-evidence-truth-and-ownership) | Sources archived; one owner per ledger procedure; COMMON has read map and authority order |
| WP1 | Shipped code, behaviour-preserving | [Section 6.1](#61-wp1-shipped-code) | One access-policy table; `_execute` split; existing tests pass unchanged except where listed |
| WP2 | Contracts, before any operation code | [Sections 6.2](#62-wp2-contracts-before-prompt-15) and [6.3](#63-target-operation-contract) | AC, ENG and CONV agree with section 6.3; timing probes updated; AC roughly halved |
| WP3 | Prompts 13-36 | [Section 7.1](#71-wp3-prompts-13-36-primary-part) | Every listed prompt edited; RV and LEDGER updated with it |
| WP4 | Prompts 37-42 and DD section 6 | [Section 7.2](#72-wp4-prompts-37-42-native) | No "corrected SDK" wording remains; host-driver evidence layer defined |
| WP5 | Prompts 43-55 | [Section 7.3](#73-wp5-prompts-43-55-cli-runners-regression) | One fixture, one matrix, one record format |
| WP6 | Prompts 56-67 | [Section 7.4](#74-wp6-prompts-56-67-variants) | Part-table design recorded; variant deltas seeded |
| WP7 | Prompts 68-80 | [Section 7.5](#75-wp7-prompts-68-80-packaging-ci-review) | Toolchain, CI and packaging instructions are executable |

## 5. WP0: evidence, truth and ownership

**Archive two sources** in `docs/reference/source/` with a complete `provenance.json` entry (`tools/verify_references.py`:40-60 requires
title, document, source_url, resolved_url, retrieved_utc, sha256, bytes, rights_holder, terms, pages, derived_text), a UTF-8 extract in
`docs/reference/text/` with `derived_text_sha256`, and update the "25 PDFs" count in `docs/reference/README.md`. Add `snla428` to LEDGER
`sources` before any record cites it. A third source is also recorded by URL below.

- SNLA428, *Time Domain Reflectometry with DP83826*, June 2023, 11 pages,
  <https://www.ti.com/lit/pdf/snla428>, SHA-256
  `b262302866cc3e8abb401f435c6935368192500b4d5fbc27faeca21c75799d33` [L].
- SNLS603D, DP83TG720R-Q1 datasheet, <https://www.ti.com/lit/ds/symlink/dp83tg720r-q1.pdf>,
  SHA-256 `a13ecacc0d2d28c404c4e719fc76d665a4472723fceafbe7b5e5a49baecfb627` [L]. Cross-part
  evidence only, for the MMD notation statement at idx 46. Its nibble equals the MMD number, unlike DP83826's `1`/`2`, so it supports the
  notation by analogy only.

<a id="g11-17"></a>
- **G11-17** S2 [L] COMMON:3-9, `AGENTS.md`, prompts `README.md`:51-61 omit AC, and 78 of 80 prompts never name it; it is reached only through CONV
  and ENG. Precedence is partial and scattered (README:65-66, DD:7, DD:11, ARCH:4). **Fix:** add to COMMON: "Authority: exact datasheet; ledger; shipped
  headers (shipped API); AC; DD; ENG/CONV; ARCH (overview only). Stages 13-36, 43-47, 53-70 and 76 read AC. Stages 37-42 read DD sections 2-7 and AC's operation
  contract. Stage 43 onward reads CLI."
- **G13-3** S2 [A] (also **G5-2**) Only 2 of 52 ledger procedure ids appear in their owner
  prompt (`identify` and `restart`, both as ordinary words), and no rule binds a stage to its ledger procedures. **Fix:** add to COMMON: "At
  stage N, implement every ledger procedure with `owner.implementationStage == N`; list them
  at prompt entry." Then add the missing owners named under P23, P33 and P34 in
  [section 7.1](#71-wp3-prompts-13-36-primary-part).
- **G10-8** S3 [L] (also **G11-21**) Stale text: prompts `README.md`:6 "All stages are currently
  **NOT STARTED**" and :176-177; `test/README.md` near line 44 "These 77 tests" reads as one
  file, and line 59 says "exports" with no install path; `AGENTS.md` and VER:153-165 list only
  research commands. **Fix:** point to the progress record; say "The 77 ledger tests above"
  and "provides"; add the CMake, CTest, consumer, Doxygen and `generate_version.py check`
  commands.
- **G11-19** S3 [A] 15 ledger procedures carry `primaryDurationUs` and `cleanupDurationUs`; 16 carry `recoveryRequired` (those 15 plus `calibration_recovery`); 14 carry a
  "12 x N MDIO frames" sentence from the eight-frame RMW. None exists in AC. **Fix:** in the 14, replace the frame sentence with "Per MMD target: snapshot read 4 + atomic
  modify 5 + verify 4 = 13 frames. Per Clause 22 target: read 1 + write 1 + verify 1 = 3. Deadlines via OperationBudget." and delete the "Cleanup one restore/readback pass"
  sentence (D9). In `analog_configure` keep its counted 68-frame and 56-frame text and replace only the duration sentence. In all 16, replace `recoveryRequired` with `cleanupOwed` (OP-9).
- **G13-4** S3 [L] LEDGER `calibration_recovery` cites section 8.4.5; 826 has only 8.4.1.
  **Fix:** `Table 8-47 p73; table 8-48 p73; 8.4.1 p48`.
- **G13-7** S3 [A] RV:201 presents BMCR `0x6100`/`0x4100` as table 8-19 "examples". The table
  prints no such literal and no duplex rule. **Fix:** say "Composed from table 8-19 (not
  printed); duplex is the test's choice."
- **M-1** S3 [L] P41:11, P43:11 and P68:14 write combined paths such as `FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`, which are not paths. **Fix:** name `.h` and `.cpp`
  separately, as SOURCE_MAP:19 does.

**Ledger and coverage owner corrections.** Apply together, in the LEDGER and COV. In every moved record also update the stage in `owner.test` and in the gate's
`nextEvidence`. No validator catches a miss.

| Item | Now | Set to | Findings |
| --- | --- | --- | --- |
| `interrupts` (feature; stages stay 20 and 27), `interrupt_configuration`, `interrupt_test`, COV:62 | 20 | 27 | G3-9, G11-19 |
| `cable_autorun` | 25 | 26 | G3-9 |
| `tdr_calibration` gate and owner | 26 | 25; physical evidence Q7 | G3-9 |
| `calibration` (`implementationStages` `[16, 25, 35]`), `calibration_recovery` | 15 | 35 (hard reset arrives at 16) | G1-9, G5-3 |
| `link_recovery` (`implementationStages` `[34]`), `link_recovery_configure` | 18 | 34 | G5-3, G11-19 |
| Gates `recr_width`, `rmii_rc` | 21 | 20 keeps them gated with raw samples; physical cases Q16, Q17 at 72-73 | G2-13 |
| Gate `rxfs_sfd_reference` | 29 | 31 | G13-5 |
| Gate `fld_reset_defaults` | 4 (finished) | 34; physical case Q18 at 73 | G13-6, G5-4 |

**Ledger text corrections, same pass:**

| Item | Now | Set to | Findings |
| --- | --- | --- | --- |
| `initialize_10base` step 1 and `restoration` | Conditional | Step 1: "Every `startInitialize`, for every ResetKind, applies exact TI 10BASE-Te initialization." Restoration: "Written and verified by every `startInitialize`. POR, pin and PHYRCR hard reset lose it; PCS reset and restart retain it." | G1-2 |
| `pcs_reset` step 5, `hard_reset` step 5, P17:43-44, P22:79, DB:101-102 | Conditional wording | Unconditional per D6; say the write is repeated even when the reset kind retained the value | G1-2 |
| `cable_manual` step 1 and cleanup | Disable and restore CR1[8]; "transfers to calibration recovery" | "Read CR1[8]; if set, reject before mutation. Prompt 26 adds the disable and restore handoff. Read live CDCR before mutation. Bit 14 high ends the session with no cable write and an explicit error, retaining the raw word; prompt 35's `calibration_recovery` is the recovery route. Never clear the bit here without the hard reset table 8-47 requires." Keep `ownedMasks` | G3-9, G1-9, N-5 |
| `pcs_reset`, `hard_reset`, `restart` step 2 | Composed with "reserved preservation" | Literal whole words per G1-13 | G1-13 |

Add the `cable_manual` CR1[8] and CDCR[14] sentences to P25 too. Add to P15: "CDCR rescal recovery is not owned here; CDCR stays rejected for raw read."

## 6. Shipped code and contracts

### 6.1 WP1: shipped code

Refactors here must not change behaviour unless stated. Two independently written prototypes of G12-1, G12-3 and G12-4 matched
the shipped source. The re-audit's compiled under the project flags on GCC 15.1 and passed all 14 suites, and was hash-identical over a 309,561-operation address sweep and
3M fuzz operations with OK rates of 1.3-30.8%. Neither prototype is retained, so re-derive it. MSVC and SDK builds are NOT RUN.

<a id="g12-1"></a>
- **G12-1** S2 [L] SRC:44-78 holds one concept in three functions with 68 bare hex cases (55 readable, 13 gated).
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

  The dependent write uses `policy.preserveMask & ~mask`. Add a `static_assert`, through a constexpr function that loops over the array, that every row with a nonzero
  `writableMask` is `SAFE_READ`, so a future mask cannot allow an RMW through a consuming read. Behaviour-preserving row classes, 96 rows: 55 SAFE_READ; 11 CONSUMING (BMSR, ANER, FLDS,
  PHYSTS, MISR1, MISR2, FCSCR, RECR, RCSR, 10BTSCR, RXFS); 2 SELECTOR; 10 RESERVED_ONLY (CDSCR2, CDLRR2-5, CDLAR2-5, ANA_LD_PROG_SL); 1 FOLLOW_UP (CDCR); 13 SOURCE_GATED; 4 PROCEDURE (0x0122,
  0x0123, 0x0130, 0x0428). G12-10 then removes SOURCE_GATED. The assert means RCSR cannot use `modifyMmd`; its typed owner preserves bit 6 and collects RC [3:2]. Update direct-access:91-92
  "explicit switch" to "policy table". Later stages extend this table. It stays a policy table, not a second copy of the field map (G1-4).
<a id="g13-1"></a>
- **G13-1** S2 [L] (D15) AC:847 says "reserved bits stay zero"; SRC:227 says "Never preserve reserved bits"; `docs/direct-access.md`:112 agrees. (DD:104 concerns the REGCR word only and stays.)
  DB:52 and prompts 17, 18, 32 and 34 require preservation. Non-zero reserved resets, R/W: CR2 [13:7]=2h, RCSR [6]=1h, 10M_CFG [13:0]=3998h, FLD_CFG2 [15:6]=8Ah, CDSCR [11]=1h and [3:0]=2h,
  LED2_GPIO_CFG [5:3]=1h, LED3_GPIO_CFG [5:3]=1h, CLK_OUT_LED_STATUS [15:1]=1h, LEDCFG [3:0]=5h. All-reserved and refused: CDSCR2=C850h, ANA_LD_PROG_SL=80h. Read-only: CDCR [13:2]=40h, FLD_CFG1
  [3:0]=7h (writes have no effect, but expected read-back words must include them). LEDCFG2 bit 10 is RH/W with unknown reset. Extending `writableMask()` as AC reads would clear them. LEDCR is
  safe only because its reserved bits reset to 0. **Fix:** AC:847 becomes "Reserved handling is per target. LEDCR forces zero; every other target keeps read-back reserved bits outside mask unless a
  sourced whole-word recipe says otherwise." Mark the SRC comment and direct-access text LEDCR-only. Carry it in `preserveMask`; RCSR is CONSUMING, so its typed owner preserves bit 6 itself. Add
  poison-read RMW vectors to RV: LEDCFG read FFFF, mask 00F0, value 00A0, writes FFAF; CR2 read FFFF, mask 0020, value 0000, writes FFDF.
- **G12-3** S3 [L] `_execute` (SRC:246-359) mixes admission, call, audit, classification and
  publication. Line 345 recomputes four predicates that equal
  `violation != NONE && violation != CONTROLLER_STATE`. **Fix:** helpers in the anonymous
  namespace, no header change: `admitBudget`, `checkPublication`, `flagsConsistent`,
  `auditFrames` returning `{attempted, completed, valid}`, `classifyReport` returning
  `{primary, release, violation}`. `_execute` falls to about 47 lines in four commented steps:
  admit the whole batch; run and keep the report; judge the report; first failure wins (primary,
  release, clock, deadline). `checkPublication` keeps SRC:352-355 for WP1 only; G11-7 reduces it to the late check in WP2, so inline it then.
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
- **G12-7** S3 [L] **Behaviour change.** SRC:250 reports local stop as BUSY/CONTROLLER_QUARANTINED, identical in code and detail to backend quarantine. The mechanism differs: a successful `end`/`bind`
  clears the local stop, although the owner must still repair the backend first (DP8382x.h:59-60), while the backend quarantine persists until controller repair. Tests assert only `.is(BUSY)`.
  **Fix:** add `bool stopped` to `TransportSnapshot` (header and AC:559-570), mention it at direct-access:146-149, and assert code, detail and `stopped` in tests.
- **G12-8** S3 [L] (see D16) `docs/direct-access.md`:99 "give these complete traces", :114-116, CHANGELOG:9 "establishes", AC:403-405 "independently verified", DD:38-40, LIBC:207-208, the LEDGER stable-RMW
  step, DP8382x.h:124-125 and `docs/reports/README.md`:60 overstate the five-frame RMW. 826 prints only separate read and write sequences; the design rests on the state-based text in 8.3.11.1. The test
  model assumes persistence, so it cannot detect a fault. **Fix:** reword to "inferred from 8.3.11.1; not a printed sequence". Keep the design. Add physical case Q4. If silicon disagrees, the fallback is
  six frames: re-issue `W 0D=401F` before the write, as the 8.3.11.4 Note permits; it fits `MAX_BATCH_FRAMES`.
- **G12-9** S3 [L] Full-word LEDCR writes replace all four fields (five bits). `0x0012` also selects 20 Hz blink and forces active-low polarity, overriding the ENHANCED strap (table 8-42, reset `04X0h`), and
  `test/consumer/main.cpp`:83 does this. **Fix:** add to both Doxygen blocks: "Replaces all four fields. Unset bits select 20 Hz blink and active-low polarity, overriding the strap. Use `modifyMmd` to change one field."
- **G12-10** S3 [L] **Behaviour change.** 13 EEE-only recipe offsets return INVALID_CONFIG/SOURCE_GATE (SRC:73-75, `test/mmd.cpp`:319-322), while the other four RECIPE_ONLY records (0x0123, shared by the EEE and
  loopback recipes, 0x0122, 0x0130, 0x0428) return INVALID_PARAM/ACCESS_POLICY (`test/mmd.cpp`:328-332). The split follows ledger gates, but raw access to all 17 is refused whatever the evidence says. Table 8-31's
  "0x04D1 and lower" covers 8 of the 13. **Fix:** all 17 become `Access::PROCEDURE` and return INVALID_PARAM/ACCESS_POLICY; add no new Status detail. Keep SOURCE_GATE for DEVAD 3/7 until D2 lands; ledger procedure gates
  are unchanged. Remove `SOURCE_GATED` from the enum. Move the 13 addresses in `test/mmd.cpp` to the 328-332 list; update direct-access:81-83 and DP8382x.h:99-100.
- **G12-11** S3 [L] The allowlist has no machine link to the ledger, and `--verify-failure-path` is not registered in CTest. **Fix:** `test/test_access_policy.py` requires SAFE_READ to equal ledger records with no RC/RCH
  fields, minus a named exception list (CDCR, ANER, 10BTSCR, REGCR, ADDAR, records with `reservedMask == 0xFFFF`, records with `status == RECIPE`). That yields exactly the shipped 55. Register the failure path for each of the
  three executables with `WILL_FAIL TRUE`; update the suite count at `test/README.md`:70.
- **G12-2** S2 [L] Test layout will not scale: `check`/`CHECK`/`main` copied three times, the fake
  `Controller` twice, about 46 `CHECK(x.bind(...).ok())` setup lines, unnamed integer faults. **Fix, no framework:**
  `test/support/Check.h` (CHECK plus `runSuites`), `test/support/FakeController.h` (the union of both fakes: mmd.cpp's wire model and fault injection plus transport.cpp's
  `custom/response`, `regress`, `reenter`, `clockReenter`, `readValue`, `written`, with the single-frame asserts made suite-level, and `enum class ReportFault`), a `BoundDriver` fixture, one table of
  `{fault, expectedAttempted, expectedCompleted, expectedViolation}`, one statement per line. Later
  prompts add suites, not harnesses.
- **G12-12** S3 [L] Names and comments. `MMD_VENDOR` is both a DEVAD and a REGCR word: split into
  `DEVAD_VENDOR`, `REGCR_VENDOR_ADDRESS`, `REGCR_VENDOR_DATA`. CFG:229-255 is one 27-line Doxygen block: first move the dependent-write and report-before-reopening rules into
  `docs/direct-access.md`, then keep about 6 lines and point there. "Copied all-source diagnostics", "Passive
  receipt" and "Independently valid prefix" need plain words. `examples/portable/DirectRead.h`:18-19
  copies and discards a receipt; `MmdAccess.h`:11-15 stages a needless local copy.

### 6.2 WP2: contracts, before prompt 15

No operation code exists yet. Apply before prompt 15, the first stage that ships operations, and
update `test/test_operation_timing.py`, ENG, CONV and every prompt that repeats the rule.

- **G11-2** S2 [L] (D9) AC:695-714, 1250-1256, 1349-1355 and 1426 mandate a CLEANUP phase with two
  deadlines, frame counts and poll counts, `CleanupState` with 5 values, and an 11-row transition
  table. Sources disagree too: ENG:110 "separately admitted", AC:1426 "reserved at start", P15:50
  "reserved/separately admitted". AC:1354-1355 concedes these are "not guarantees of hardware
  completion". Peer TMP1x2 operations are 268 lines with no cleanup phase. **Fix:** OP-4 and OP-9.
- **G11-3** S2 [L] (D9) AC:703-705, 771-772, 894-895, 923-924, 1447-1466: no code path branches on the
  three `QuiescenceDisposition` values; the only rule it creates is about itself (AC:1460-1461), and the
  clock latch already forces rebind. **Fix:** OP-10. Delete `QuiescenceDisposition`, `retiredByOwner`,
  `RETIRE_REQUIRED`.
- **G11-4** S2 [L] AC:353-355 requires proving `clockLagUs` "including... scheduling", and CFG:273 says "on clock return". That is
  impossible on an RTOS and unnecessary. The proof assumes the raw sample is never ahead of true time (AC:352). The sample is read after the callback returned, so preemption
  before the read moves it later (safe) and preemption after does not change it. A later raw sample >=
  `sample + lag + minimum` proves the minimum elapsed under any scheduling. For deadlines no lag value can
  guarantee anything; `late` handles that. **Fix:** "Positive bound on how far the returned value can be
  behind true time at the instant of the read (quantization plus source update latency). Scheduling after
  the read is excluded." No behaviour change.
- **G11-5** S2 [L] AC:1321-1327: an insufficient positive poll slice terminates the phase, and
  `OperationSnapshot` exposes no next-batch size, so a short EtherCAT slot kills a reset. **Fix:** OP-5.
- **G11-6** S2 [L] (D10) AC:1331-1355 and :1508 require per-procedure reserve sums. Every failure already
  leaves trust DIRTY with effect evidence, and the sum grows at each of prompts 17-35. "Compile its finite
  order" invites a plan engine, against ENG:160-161. **Fix:** OP-6. Delete these lines and scenario AC:1556.
- **G11-7** S2 [L] AC:590-592 count timing and clock faults toward OFFLINE, while CONV:73-74 defines OFFLINE as repeated MDIO failure; three tight deadlines drive OFFLINE with every frame good. **Fix:** replace rows
  586-592 with: "A callback is not counted if zero frames were attempted, normalized status is BUSY or OPERATION_TIMEOUT, raw release is OK, and either the controller stays reusable or the report is a valid
  BUSY/CONTROLLER_QUARANTINED. Otherwise it is a failure if normalized status or release is not OK, the controller is newly not reusable, or the report is malformed. Otherwise it is a success, including late success and clock
  regression, which stay in `TransportSnapshot`." Delete the publication reserve at AC:395-399 and SRC:352-355, and align AC:356, 376-377, 408-409, 498, 609, CFG:272, `docs/direct-access.md`:61-62 and 141-142, and DD:372.
  **Behaviour change in SRC; update `test/transport.cpp`:455-468 and `test/test_operation_timing.py`.**
- **G11-8** S3 [L] (D10) AC:711-712 and 1311-1316: polling every 100 us during a 50 ms wait exhausts
  `maxPrimaryPolls` and yields a false POLL_BUDGET timeout on healthy hardware. **Fix:** OP-5.
- **G11-11** S2 [L] AC:941-942 "incompatible active work" is undefined, with two materially different
  readings, one of which stalls native link polling behind an unconsumed result. **Fix:** OP-14.
- **G11-12** S2 [L] (D11) AC:699-700, 715-719, 755-756, 1506-1532: initialize, reset and recover are one plan
  with three entries, and `RecoveryMode` omits RESTART. **Fix:** declarations and mapping in section 6.3.
- **G11-13** S3 [L] AC:742-750, 777-778, 784: results carry two raw batch dumps. Auditor measurement with g++ 15
  on x86-64: `OperationResult` 880 B of which 656 B is batch dumps, `OperationSnapshot` 944 B, copied by the owner
  each poll. **Fix:** `BatchSummary` of 48 B, giving a result of about 256 B and a result-free snapshot of 96 B (compiled with AC-shaped stand-ins). The raw report stays in `transportSnapshot`.
- **G11-14** S3 [L] Acquire and release are only summed; overhead is summed, validated positive (SRC:137) and used by the publication reserve (SRC:353), which G11-7 removes. **Fix, after G11-7:**
  `TransportBounds{fixedUs, frameUs, clockLagUs}`, with `fixedUs = acquire + release + overhead`; all three must be positive, else INVALID_CONFIG/TRANSPORT. Admission is then identical. Update CFG, AC, `docs/direct-access.md`:15
  and :56, and tests.
- **G11-15** S3 [A] AC:411-413's `storedMs*1000+999` exists only because some fields are ms while the only
  clock is us. **Fix:** store `...Us` everywhere; delete the paragraph.
- **G11-16** S3 [L] (with G8-2) `RevisionPolicy` is a one-value public enum (CFG:35-37, 292; SRC:131); no
  prompt mentions it. **Fix:** delete the enum and config field. Use a private accepted-revision lookup,
  which becomes a part-table field at prompt 56.
- **G11-18** S2 [A] Rules are spread across about 250 KB; "release the lock before waits" appears at least six times. AC's poll and transport sections pack several MUST rules into wrapped paragraphs. **Fix:** (1) replace AC's
  code blocks for shipped Status, Config and Transport with links to headers; (2) move native material (AC:1087-1106, 1408-1414, 1626-1733) to DD, rewritten to the section 6.3 API; (3) move the feature map (AC:1108-1176) to COV; (4)
  replace AC:1542-1574 with section 6.3's scenarios and keep that heading in AC so `test/README.md`:51 stays valid; (5) replace ARCH:114-166 with pointers; (6) turn operation sections into numbered rules (OP-1...) with one normative
  location, and have prompts cite the ids.
- **G11-20** S3 [L] (also **G1-12**, D17) CONV:61-63 forbids unused codes, yet `BOARD_PREREQUISITE`, `LINK_MODE`, `FEATURE` have no assigned use. P15:30 and :60-61 require testing token exhaustion, which takes 2^64 starts or a test seam
  CONV:156 forbids. **Fix:** remove those three until a consumer exists; delete `TOKEN_EXHAUSTED` and its test with "A 64-bit lifetime counter cannot be exhausted in practice"; update CONV:122 and P15:30, :60-61. Leave P8:46 as history. Keep
  P43:51: CLI session and request identifiers have their own widths.
- **G11-22** S3 [A] (also **G2-14**) `getSettingsSnapshot`/`getOperationSnapshot` versus `healthSnapshot`/`linkSnapshot`/`transportSnapshot`: rename to `settingsSnapshot()` and `operationSnapshot()` before they ship (also CONV:94-95, AC:1259, :1500, :1601,
  :1632, :1697, :1719, P15:27). CONV:114-115 omits `modifyMmd` and `transportSnapshot`: add them. AC:796 "17?18" should read "17-18". DD:223 becomes "Power, typed power operation". ENG:101, VER:51, P15:36 and :59, P16:53, P19:58, P21:52 and P54:36 require clock
  "wrap" tests (P24's counter wrap is unrelated), while CFG:261 and AC:346-347 say nonwrapping. **Fix:** replace with "uint64 overflow" and "regressing clock (INVALID_CONFIG/CLOCK_CONTRACT, latch, no new valid stamp)".
### 6.3 Target operation contract

This replaces AC's "Cooperative values", the operation methods in "Exact local and direct declarations" and "One cooperative
operation and retained result" up to its acceptance scenarios.

**Kept from AC, reading "cleanup" per OP-9:** admission meaning (1185); past-deadline detection (1202-1204); gate admission facts (1213-1216); token scope (1229-1233) and lookup (1235-1240); result-flag meanings
(1270-1284); collector retention (1290-1291); finishing a verification pass before freezing CONFIG_MISMATCH (1292-1297); `nextPollValid` (1299-1303); the post-callback sample (1318-1320); wait anchors (1357-1365); error precedence (1367-1374,
without the publication reserve, and 1374 reads "the SDK's nominal loop limit"); readiness anchors (1385-1391, with G1-3); nominal figures (1394); the phase plan (1396-1406); no configuration write before re-identification (1433-1435); desired-intent
rules (1470-1485); the restoration order (1508-1521, with item 1 rewritten per the file list, and 1510 gaining "HARD itself is the stop for a hard-reset exit."); operation semantics (1523-1532, renaming per the mapping); and future recipe order and temporary
triggers (1534-1540). AC:1408-1414 moves to DD with the native material.

**Evidence for this design.** It tests the design only; no driver implements it yet.

- The declarations compile against the shipped headers with `-Wall -Wextra -Wpedantic -Wconversion -Werror` on g++ 15, using stand-ins for the values not shipped yet (`OperationToken`, `OperationPhase`, `DeviceIdentity`, `StrapEvidence`, `SettingsSnapshot`, `ConfigurationMismatch`).
- An independent adversarial review found 18 defects in the first draft, R-1 to R-18, two of them S1. A later finding-by-finding verification found further defects in OP-6, OP-9, OP-11 to OP-14 and OP-16. All are applied below; section 11 records where.
- A state model checks admission and barrier rules: 1,346 states, no violation, and every state reaches unbound through local calls, including with a broken clock. Seven flawed variants, re-introducing R-1, R-2, R-3 and the old PCS-over-session gap, each produce violations. It is a
  partial model. It assumes a reported-good HARD write applied and that a successful initialize restores all dirty fields, and it stops checking physical invariants after abandon. It has no raw writes, direct session calls, reset-wait read rules, non-external invalidation, tokens, timing
  arithmetic, register content or link evidence, so those rules, OP-6, OP-13's details and the OP-14 link note rest on review alone.

```cpp
enum class OperationKind : uint8_t { NONE, INITIALIZE, CONFIGURE, VERIFY };  // Features add kinds with their code.
enum class OperationState : uint8_t { IDLE, RUNNING, RESULT_PENDING };
enum class ResetKind : uint8_t { NONE, PCS, HARD, RESTART };

struct OperationBudget {
  IoBudget io{};                   // Absolute deadline and total frames for this operation.
  uint32_t pollIntervalUs{1000};   // Positive owner policy between completion reads, not silicon timing.
  uint32_t maxStalledPolls{100000};  // Positive; consecutive polls with no batch and no clock advance.
};
struct InitializeOptions { ResetKind reset{ResetKind::PCS}; };

struct BatchSummary {
  OperationPhase phase{OperationPhase::NONE};
  uint8_t attemptedFrames{0};
  uint8_t completedFrames{0};
  bool effectPossible{false};
  Status normalized{Err::RESULT_NOT_AVAILABLE, 0, "No batch"};
  Status release{Err::RESULT_NOT_AVAILABLE, 0, "No batch"};
  TransportViolation violation{TransportViolation::NONE};
  bool late{false};
};
struct OperationResult {
  OperationToken token{};
  OperationKind kind{OperationKind::NONE};
  ResetKind reset{ResetKind::NONE};
  Status status{Err::RESULT_NOT_AVAILABLE, 0, "No operation"};
  bool mutationAttempted{false};     // Same meanings as AC:1270-1284.
  bool mutationCompleted{false};
  bool mutationPossible{false};
  bool commandRequired{false};
  bool commandAccepted{false};
  bool protocolComplete{false};
  bool temporaryMayBeActive{false};
  bool persistentRestored{false};
  bool cleanupOwed{false};           // Barrier state when this operation ended.
  bool clockInvalid{false};
  bool late{false};
  DeviceIdentity identity{};
  StrapEvidence straps{};
  SettingsSnapshot settings{};
  ConfigurationMismatch mismatch{};
  BatchSummary failingBatch{};       // First failing batch, otherwise the last batch.
};
struct OperationSnapshot {           // Bus and clock silent. Holds no result copy.
  OperationToken token{};
  OperationState state{OperationState::IDLE};
  OperationKind kind{OperationKind::NONE};
  OperationPhase phase{OperationPhase::NONE};
  Status status{Err::RESULT_NOT_AVAILABLE, 0, "No operation"};
  bool cleanupOwed{false};
  bool temporaryMayBeActive{false};
  uint64_t nextPollAtUs{0};
  bool nextPollValid{false};
  uint64_t busQuietUntilUs{0};       // Hard-reset floor end; other PHYs on this bus stay silent until then.
  bool busQuietValid{false};         // True in every state while the floor is pending.
  IoBudget remaining{};
  uint32_t stalledPolls{0};
};

Status startInitialize(const InitializeOptions& options, const OperationBudget& budget, OperationToken& token) noexcept;
Status startConfigure(const LinkSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept;
Status startVerify(const OperationBudget& budget, OperationToken& token) noexcept;
Status pollOperation(OperationToken token, uint32_t maxFrames = MAX_BATCH_FRAMES) noexcept;
OperationSnapshot operationSnapshot() const noexcept;
Status takeResult(OperationToken token, OperationResult& result) noexcept;
Status cancel(OperationToken token) noexcept;
Status abandonCleanup() noexcept;
```

`OperationToken`, `OperationPhase`, `StrapEvidence`, `ConfigurationMismatch` and `SettingsSnapshot` stay as AC defines them. `DeviceIdentity` stays as AC defines it except for G11-15 (`observedAtUs`) and G11-16 (no `policy` field). Enum values ship with their behaviour: NONE and PCS at prompt 15; HARD, RESTART and `EXTERNAL_POWER_ON` at 16;
CONFIGURE at 17-18; VERIFY at 22.

| Old | New |
| --- | --- |
| `startReset({PCS})`, `startRecover({PCS_RESET})` | `startInitialize({ResetKind::PCS})` |
| `startReset({HARD})`, `startRecover({HARD_RESET})` | `startInitialize({ResetKind::HARD})` |
| `startReset({RESTART})` | `startInitialize({ResetKind::RESTART})` |
| `startRecover({REAPPLY})` | `startInitialize({ResetKind::NONE})` |
| `startRecover({VERIFY_ONLY})` | `startVerify` |
| `InitializeOptions.pcsReset` | `InitializeOptions.reset` |
| `pollOperation(token, slice)` | `pollOperation(token, maxFrames)` |
| `acknowledgeQuiescence(token, disposition)` | `abandonCleanup()` |
| `getOperationSnapshot`, `getSettingsSnapshot` | `operationSnapshot`, `settingsSnapshot` |
| `OperationBudget.primary`, `.cleanup`, `.maxPrimaryPolls`, `.maxCleanupPolls` | `.io`, removed, `.maxStalledPolls`, removed |
| `result.primaryStatus`, `.cleanupStatus`, `.cleanupState` | `result.status`, removed, `result.cleanupOwed` |
| `primaryBatch`, `cleanupBatch` | `failingBatch` |
| `OperationKind::RESET`, `::RECOVER` | `INITIALIZE`, `VERIFY` |
| `result.resetKind`, `.recoveryMode` | `result.reset`, removed |
| `result.primaryLate`, `.cleanupLate` | `result.late`, removed |
| `result.retiredByOwner`, `.disposition`, `ResetOptions`, `RecoverOptions`, `CleanupState` | Removed |
| `OperationBatchEvidence` | `BatchSummary` |
| `snapshot.result`, `.primaryRemaining`, `.cleanupRemaining`, `.primaryPollsRemaining`, `.cleanupPollsRemaining` | `snapshot.status`, `.remaining`, removed, `.stalledPolls` (counts up; different meaning), removed |

**Rules:**

- **OP-1 One slot.** One instance holds one operation, with no task, scheduler or registry. Starts copy their inputs,
  return IN_PROGRESS and assign a token. They call neither clock nor MDIO.
- **OP-2 Start rejection order.** Syntax and budget shape (`pollIntervalUs` or `maxStalledPolls` of zero is
  INVALID_PARAM/BUDGET_RANGE); NOT_BOUND; slot busy (BUSY/OPERATION_ACTIVE, then BUSY/RESULT_PENDING); known clock fault
  (INVALID_CONFIG/CLOCK_CONTRACT); barrier (OP-9, BUSY/CLEANUP_REQUIRED); NOT_INITIALIZED; capability and source admission; zero budget
  (OPERATION_TIMEOUT/FRAME_BUDGET or DEADLINE). `startConfigure`, `startVerify`, feature starts, session starts and session acquire require
  initialized. `startInitialize` and the owning stop do not, and use the bootstrap identity path. OFFLINE never denies a start. `startVerify` issues no semantic or command write.
  A rejected start changes nothing. (R-4)
- **OP-3 Tokens.** A 64-bit counter for the object's lifetime, from 1, never reset by end or bind. There is no exhaustion
  branch, code or test.
- **OP-4 States.**

  | State and event | Next |
  | --- | --- |
  | IDLE, accepted start | RUNNING |
  | RUNNING, poll with work remaining | RUNNING, IN_PROGRESS |
  | RUNNING, full success | RESULT_PENDING, status OK |
  | RUNNING, error, timeout or clock fault | RESULT_PENDING, status frozen; set barrier per OP-9 |
  | RUNNING, cancel | RESULT_PENDING, status CANCELLED unless an earlier error froze; set barrier per OP-9 |
  | RUNNING, `invalidateDeviceState` | RESULT_PENDING, INVALID_CONFIG/DEVICE_INVALIDATED unless an earlier error froze; set barrier per OP-9, then apply OP-15. `result.cleanupOwed` is the value after both (R-15) |
  | RESULT_PENDING, poll or repeated cancel | Unchanged; poll repeats the status silently; cancel returns OK |
  | RESULT_PENDING, exact take | IDLE; result copied once; barrier unchanged |

  The status never changes once frozen. `takeResult` before terminal returns RESULT_NOT_AVAILABLE. Take always succeeds
  once pending, so evidence can always be delivered.
- **OP-5 Poll.** At most one batch per call, in this order:
  1. Sample time. A regression latches CLOCK_CONTRACT and freezes.
  2. If raw time >= deadline, freeze OPERATION_TIMEOUT/DEADLINE before any I/O.
  3. If the next action is not due, or a hard-reset floor is pending, return IN_PROGRESS with no I/O. The floor applies to every
     operation kind (R-1).
  4. If the next batch exceeds remaining frames, freeze FRAME_BUDGET. If its bound cannot fit before the deadline, freeze
     BOUND_ADMISSION.
  5. If the next batch exceeds `maxFrames`, return IN_PROGRESS with no I/O. So `maxFrames=0` is bookkeeping only, and a short
     slot is never fatal.
  6. Otherwise run it.

  A poll that issues no batch and sees the same raw clock value as the last sample any poll took, including a post-callback sample,
  increments `stalledPolls`. Any batch or clock advance resets it, and the first poll of an operation never stalls (R-14). Reaching
  `maxStalledPolls` freezes OPERATION_TIMEOUT/POLL_BUDGET. Frames bound batches and stalls bound waits, so a frozen clock always
  terminates, within `(frames + 1) * maxStalledPolls + 1` polls (brute-forced as the exact worst case with one-frame batches). The
  default of 100000 is deliberately large, because this is a fault path: a tight loop on a 100 Hz tick clock must not trip it, while a
  frozen clock still ends in about 100 s at 1 ms polls. Lower it only if the clock's tick is shorter than poll spacing times the value (R-13).
- **OP-6 Reset admission.** This is the only look-ahead. Before issuing a reset command, require frames for the command batch and
  the first completion read, and `raw + 4 * clockLagUs + bound(command) + wait + bound(read) < deadline` with checked addition. `wait` is
  `pollIntervalUs`, or `max(2000, pollIntervalUs)` us for HARD. Otherwise freeze BOUND_ADMISSION with no mutation. Every other step
  checks only its next batch. The four lags are: upper bound at entry, anchor at completion, overshoot of the first raw sample >= Due on a quantized clock, and upper bound at the read (R-12). This assumes the raw step does not exceed
  `clockLagUs`. It rejects budgets too small under prompt polling; a late owner poll can still time out after the command, and OP-9 covers that.
- **OP-7 Time.** `clockLagUs` is defined by G11-4. Only a raw post-sample >= deadline denies success; there is no publication reserve.
- **OP-8 Primary and cleanup stay separate.** Each operation has one status and one result. Restoration is a later
  operation with its own token, result and budget, so its success can never rewrite the earlier failure. There is no
  automatic retry; the application may retry.
- **OP-9 Barrier.** Two pieces of knowledge, plus one private field recording what set them:
  - `temporaryMayBeActive` becomes true when the first write of a session or temporary profile is **attempted**, including companion
    writes. It becomes false only on a successful owning stop, a successful HARD, EXTERNAL_RESET/EXTERNAL_POWER_ON invalidation, or a successful `bind` (OP-12). A
    reset command alone never sets it; a pending reset is `commandAccepted && !protocolComplete` (R-2).
  - `cleanupOwed` is set when an operation ends without `persistentRestored` after a semantic effect (`mutationCompleted` or
    `mutationPossible`), and when `temporaryMayBeActive` becomes true. Selector writes do not count.

  | While `cleanupOwed` or `temporaryMayBeActive` (R-16) | Admitted |
  | --- | --- |
  | Always, except where OP-14 blocks them | Admitted raw reads and the collector |
  | Temporary active | The owning session's own acquire operations and its stop, even if its start failed (R-2, R-7); `startInitialize({HARD})` |
  | No temporary active | `startInitialize` with any reset; `startVerify`; `startConfigure` only if a CONFIGURE operation set the barrier (R-3) |
  | Everything else | Other starts and raw and typed direct writes return BUSY/CLEANUP_REQUIRED. `end` and `bind` return BUSY/CLEANUP_REQUIRED only while `cleanupOwed`; after `abandonCleanup` they are admitted even with `temporaryMayBeActive` set |

  `cleanupOwed` clears only on: a successful owning stop; a successful `startInitialize` with `persistentRestored`; a successful
  `startConfigure` admitted by the row above; `invalidateDeviceState` with EXTERNAL_RESET or EXTERNAL_POWER_ON; or `abandonCleanup`.
  **`startVerify` never clears it**, because stable bits cannot prove opaque recipe state (R-3, P22:45-46), and it is rejected while a
  temporary is active, because overridden fields would give false mismatches (R-17). A failed restoration leaves the barrier set. PCS
  reset and RESTART cannot be assumed to stop a stimulus: they keep vendor registers (table 8-19, table 8-48).

  **Sessions (R-8, R-9).** A stimulus that stays active across owner decisions is two operations with separate budgets; its duration is
  application policy, and the core only blocks other work. A self-contained test (cable capture, a counted BIST run) may be one operation
  that must stop and restore before success. A start or stop with no wait may be a direct bounded call: it follows the same set and clear
  rules at return, requires slot IDLE, and on failure leaves the barrier set. Loopback, BIST, compliance, forced LPI and collision test
  are sessions.
- **OP-10 Abandon.** `abandonCleanup()` is bus and clock silent. While RUNNING it returns BUSY/OPERATION_ACTIVE. With no
  obligation it returns OK and changes nothing. Otherwise it clears `cleanupOwed`, sets initialized false and trust DIRTY,
  and keeps `temporaryMayBeActive` and all evidence. It never claims restoration, clears controller quarantine or authorizes freeing
  borrowed resources. Before calling it the owner must close admission, drain callbacks with no lock held across SDK calls, and
  establish the external disposition: reset, power removal, isolation or transfer to another owner. A later `startInitialize` on
  this binding still needs HARD while `temporaryMayBeActive` is set.
- **OP-11 Broken clock.** Timed work is refused once latched, so restoration cannot run. The finite route is: `takeResult` if pending, `abandonCleanup` if owed, `end`, repair the clock, wait out any OP-12 floor using a time source the owner trusts, `bind`.
- **OP-12 End and bind.** Both reject BUSY/OPERATION_ACTIVE, BUSY/RESULT_PENDING or BUSY/CLEANUP_REQUIRED, with no callbacks. A
  successful `bind` resets local knowledge, including `temporaryMayBeActive`; the owner carries any known hardware state across it. `end`
  and a replacing `bind` discard a pending hard-reset floor, so the owner must wait at least 2000 us after the last possible PHYRCR[15] write before the new
  binding's first frame (R-1). Neither touches hardware or controller quarantine.
- **OP-13 Initialized.** A successful `startInitialize` sets it. It clears when a PCS or HARD reset is attempted or possibly applied,
  when `invalidateDeviceState` runs, or on abandon. `startInitialize({NONE})` on an initialized object leaves it set unless identity or
  verification fails, or a mandatory-initialization write is possibly applied and unverified, so link polling continues through a
  reapply (R-17). RESTART never clears it, because table 8-48 keeps registers; a HARD inside a session recipe does. An owning stop that
  completes the full identity, mandatory and replay procedure sets it (R-6).
- **OP-14 Direct calls during operations.** "Raw writes" here and in OP-9 means raw and typed direct writes (R-8), except the owning session's direct stop (OP-9).

  | Situation | Admitted raw reads and collector | Raw writes |
  | --- | --- | --- |
  | IDLE, no barrier | Yes | Per access policy |
  | IDLE, barrier or temporary | Yes | BUSY/CLEANUP_REQUIRED |
  | RUNNING, between polls | Yes: no selector state survives a batch | BUSY/OPERATION_ACTIVE |
  | RUNNING, reset wait | BUSY/OPERATION_ACTIVE | BUSY/OPERATION_ACTIVE |
  | RESULT_PENDING | Yes, so link polling continues | BUSY/RESULT_PENDING |
  | Hard-reset floor pending, in any state | BUSY/OPERATION_ACTIVE | BUSY/OPERATION_ACTIVE |

  **Hard-reset floor (R-1).** It starts whenever any operation attempts a PHYRCR[15] write, including a failed or late write, anchored at
  the post-callback upper bound plus `max(2000, pollIntervalUs)`. It survives cancel, take, abandon and invalidation. Publish it as
  `busQuietUntilUs`. A shared-bus owner holds other PHYs from before the poll that can issue PHYRCR[15] until that time (R-11). If the post-callback sample regressed, no anchor exists: `busQuietValid` is false, timed I/O is already refused, and the owner applies OP-12's 2000 us.

  **Link evidence during writes (R-10).** If a RUNNING operation has attempted a semantic write, or `temporaryMayBeActive` is set,
  `readLinkStatus` publishes the link bit only, with `resolvedModeValid=false` and `pauseValid=false`, and copies
  `temporaryMayBeActive` into `LinkSnapshot`. Otherwise a reader could combine new ANAR with old ALNPAR, or report an internal loopback
  path as a normal link.
- **OP-15 Invalidation.** Bus and clock silent in every state. It marks trust DIRTY, observations stale, capabilities invalid and
  initialized false, and keeps desired intent, samples and evidence. IDLE stays IDLE. RUNNING freezes per OP-4. RESULT_PENDING keeps its
  historical result. Only EXTERNAL_RESET and EXTERNAL_POWER_ON clear the barrier and `temporaryMayBeActive`; this is the owner's
  attestation that hardware was reset.
- **OP-16 Health.** `startInitialize`, `startConfigure`, `startVerify`, typed persistent feature operations and live link reads are managed traffic under G11-7. A
  successful `startInitialize` may restore READY. Session stimulus, session stop and raw access count only in transport totals.
- **OP-17 Result flags.** `result.cleanupOwed` and `temporaryMayBeActive` are copies at termination. The current truth is in
  `operationSnapshot`.

**Acceptance scenarios.** These replace AC:1542-1574, except keep AC:1552, 1553, 1558, 1559 and 1564 with renamed fields. Use literal traces and a fake clock:

| Scenario | Required |
| --- | --- |
| Start, polls, take, cable unplugged | Status OK; separate `readLinkStatus` gives valid DOWN |
| Short slot, `maxFrames=1`, next batch needs 4 | IN_PROGRESS, no I/O while the batch still fits; then OPERATION_TIMEOUT/BOUND_ADMISSION, or DEADLINE if the first poll after that is already at or past the deadline |
| Polling every 100 us through a 2 ms wait on a 1 us clock | No POLL_BUDGET; first read not before due |
| Frozen clock during a wait | No I/O; POLL_BUDGET on stall count `maxStalledPolls`, where the first poll never stalls; barrier set if a reset was issued |
| Tight loop on a 100 Hz tick clock through a 5 ms wait | No POLL_BUDGET; first read only once a raw sample reaches due |
| 100 Hz clock, lag 10000 us, HARD, deadline now + 5 ms plus bounds | BOUND_ADMISSION before any write, per OP-6's literal formula |
| Budget too small for reset command, wait and first read | BOUND_ADMISSION, zero writes, no barrier |
| Budget just enough for the reset, restore runs out | Timeout at the first non-fitting batch; barrier set; restoration later succeeds with a new budget and clears it |
| Hard reset with 1000 us interval | No frame before command-completion upper bound + 2000 us |
| Hard-reset write fails or returns late | Floor still applies; `busQuietValid` true; status frozen |
| Cancel after a hard-reset write, take, immediate new `startInitialize({HARD})` | Polls return IN_PROGRESS with no frame until the floor ends. `startVerify` in the same state returns NOT_INITIALIZED with no token |
| Abandon, end and bind within the floor | Owner waits out 2000 us; a contract test documents this, and the core cannot enforce it |
| Cancel before any effect | Pending CANCELLED, no barrier, `end` after take succeeds |
| Cancel after accepted reset | Pending CANCELLED, barrier set; take OK; other starts BUSY/CLEANUP_REQUIRED; `startInitialize` clears it |
| Restoration fails | Barrier remains; application may retry with a new token; the first result is unchanged |
| Loopback active, `startInitialize({PCS})` | BUSY/CLEANUP_REQUIRED; `stopLoopback` or `{HARD}` admitted |
| Cancel after a loopback start's first write | `temporaryMayBeActive` true; `{PCS}` rejected; `stopLoopback` admitted though the start failed |
| BIST session active, capture requested | The session's acquire operation is admitted; other starts BUSY/CLEANUP_REQUIRED |
| Power operation fails after a write, then `startConfigure` | BUSY/CLEANUP_REQUIRED; `startInitialize` restores and clears |
| Configure fails after a write, then `startConfigure` succeeds | Barrier clears |
| `startVerify` succeeds while a barrier is set | Barrier stays set; evidence only |
| `readLinkStatus` during reapply after ANAR write | Link bit only, `resolvedModeValid=false`, `pauseValid=false` |
| External reset notified while barrier set | Barrier and temporary cleared; initialized false; pending result unchanged |
| Clock regression mid-operation, with and without a loopback session active | Frozen CLOCK_CONTRACT; route of OP-11 completes with no timed call |
| Link poll with result pending | `readLinkStatus` succeeds; start and raw write BUSY/RESULT_PENDING |
| Wrong token, stale token, double take | TOKEN_MISMATCH or RESULT_NOT_AVAILABLE; no state or output change |

**Files to change for this contract:**

| File | Change |
| --- | --- |
| `docs/api-contracts.md` | Replace the sections above, keeping the listed lines. Add a line that this supersedes prompt 8's cooperative design. Fix 353-355, 356, 376-377, 408-409, 498, 585-592, 609, 629-632, 847, 922-924, 931-938, 941-942, 1392-1393, 1473 and 1510. Rewrite 1508-1509 as: "1. Validate the complete request against currently implemented owned fields and source gates; establish board/controller readiness." Rewrite the OwnerSession example, 1576-1624, to the new API. Add `temporaryMayBeActive` to `LinkSnapshot` |
| `include/DP8382x/Status.h` | Remove `TOKEN_EXHAUSTED`, `RETIRE_REQUIRED`, `BOARD_PREREQUISITE`, `LINK_MODE`, `FEATURE`. Keep `CLEANUP_REQUIRED` and `POLL_BUDGET` |
| `include/DP8382x/Config.h`, `src/DP8382x.cpp` | G11-14 bounds, G11-16, publication reserve removed, G11-4 comment |
| `test/transport.cpp`, `test/mmd.cpp`, `test/consumer/main.cpp` | Bound rename, publication change, and remove the `revisionPolicy` case at transport.cpp:182. `{3,5,2,4,2}` becomes `{9,5,2}`, `{3,4,2,3,1}` becomes `{8,4,1}`, `{0,1,0,1,1}` becomes `{1,1,1}` |
| `test/test_operation_timing.py` | Delete :65-78 and :88-106. Rename :80 to drop token exhaustion and keep its overflow checks. Rewrite :23-37 to OP-6. Add probes for OP-5's frozen-clock bound and OP-6, including the 2 ms floor and literal 100 Hz vectors |
| `docs/direct-access.md` | G11-7 and G11-14 at 15, 56, 61-62, 141-142 |
| `docs/prompts/COMMON.md` | Add: "In prompts 13-80 and in ledger text, 'cleanup' means the OP-9 barrier and a later restoration operation; 'primary/cleanup status' means two separate results. A stimulus that stays active across owner decisions is two operations with separate budgets, its duration is application policy and the core only blocks other work; a self-contained test may be one operation that must stop and restore before success." This covers the general mentions in about 50 prompts and the 193 ledger strings that mention cleanup (79 distinct) |
| `docs/prompts/ENGINEERING.md` | 96-98, 101, 104 and 110-113 point to AC |
| `docs/prompts/CONVENTIONS.md` | 30 and 112-114 names; 41 and 79-81 `...Us` fields; 94-95 snapshot names; 96 and 99-101 (restoration through `startInitialize` may restore health, per OP-16); 122 no exhaustion; 130 barrier. Record the `startRecover` difference from peers |
| `docs/design-decisions.md`, `docs/architecture.md` | DD:117-131 and :336, DD:372 per G11-7, and rewrite the moved native example to this API; ARCH:114-166 become pointers (G11-18), and ARCH:230 per G11-7 |
| `docs/prompts/` 15, 16, 22-25, 28, 30, 35, 45, 79 | P15:26-28, :50-52, :60-61; P16:31-33, :45-47; P22 per G2-15; P23:20 and :58-60; P24:46-50; P25:19; P28 power operations under OP-9; P30:46 per G4-2; P35:61-65; P45's retained-operation commands; P79:43 |
| `docs/registers/dp83826i.json` | The 4 procedures and the `reset` feature with API `startReset / startRecover`, including `cli` and `test` text, become `startInitialize`; `primaryDurationUs`, `cleanupDurationUs` per G11-19 |
| `docs/feature-coverage.md` | Only line 29's cleanup wording |
| `docs/prompts/8-operation-contracts.md`, completed stage reports, `docs/library-comparison.md` | Do not edit: they are history |

## 7. Prompt corrections

### 7.1 WP3: prompts 13-36, primary part

#### Prompt 13

- **G1-1** S2 [A] P13:44-53 builds a collector with no legal consumer: AC:956-965 and SRC reject these accesses,
  and CONV:155-157 forbids test-only exposure. **Fix:** "Stage 13 adds no public destructive API. Deliver pure
  `cmd::` composition constants and functions for BMCR, PHYRCR and RXFCFG1 with literal tests, and confirm the
  access classification. Create the private collector with its first real typed consumer (prompt 17's ANER page
  read, or prompt 19's `readLinkStatus`). The phase-failure, history-loss and cached-reader matrix moves there." Also delete P13:17-18's collector sentence and P13:63's collector clause; keep 0x0F classified as CONSUMING.
  Prompt 17's next-page exchange creates the collector, because it has the first typed consumer, and P17 must say so. If next page is deferred, prompt 19 creates it. Change P19:30 to "the single collector".
- **G1-4** S2 [I] P13:27-29 mandates a second full register catalogue in C++ (283 fields in 69 of the ledger's 96 register records), against AC:1117-1118. **Fix:** "The ledger stays the per-field catalogue. Stage 13 re-checks the WP1 classification of every
  ledger address in the G12-1 table against the access tables and adds named masks only for fields with a consumer now. Add a test that every `cmd::` constant matches the ledger." Apply the same change to P13:16, P13:65-66 and P17:80: "catalogue" there means the ledger.
- **G1-5** S3 [L] P13:33 and ENG:135 require W1C composition. No W1C and no write-1-to-clear wording appears in any of the four datasheets. **Fix:** P13:33: "for W0C (RXFCFG1[11] only), RH/W1S commands, self-clearing commands typed R/W (CDCR[15]), BICSR1's write-1 lock-and-clear
  on bit 15, RC/RCH read-clear fields, read-clearing fields typed R (10BTSCR[4]) and reserved bits. Read the description column; the type code alone is not enough." ENG:135 and the lists at FC:123, VER:42, P55:44, P66:51, P78:52: drop W1C and keep part-neutral wording: "exact per-part
  W0C/W1S/RC/latch semantics, including description-only effects". On 825 and 822 the RXFCFG bit differs, so "RXFCFG1[11] only" stays in P13.
#### Prompt 14

- **G1-6** S3 [L] P14:27 calls `readIdentity` and capability queries "frozen"; AC declares neither. **Fix:** add
  `Status readIdentity(DeviceIdentity& identity, const IoBudget& budget) noexcept;` (bound only, one two-frame C22
  batch, output unchanged unless complete and accepted), `DeviceIdentity identitySnapshot() const noexcept;` and
  `Capabilities capabilities() const noexcept;`. `identitySnapshot()` returns the last evidence, including partial and rejected words with their validity flags (AC:227-229). `capabilities()` has `valid=false` until identity is accepted. `readIdentity` needs no initialization and is admitted as a raw read under OP-14, so it returns BUSY/OPERATION_ACTIVE in a reset wait or hard-reset floor.

#### Prompt 15

- **G1-2** S2 [A] (also **G2-9**, **G11-10**) P15 never mentions VOD_CFG3, though LEDGER `initialize_10base`, COV:40
  and AC:1400-1401 put it there; P17:42 and P18:41 reimplement it. **Fix (D6):** add to P15: "Mandatory step, one
  vendor batch `W(0D,001F) W(0E,030E) W(0D,401F) W(0E,4A40)`, then verify stable mask `0xFFC0`, for every ResetKind. There is no verify-only branch in `startInitialize`: PCS reset leaves vendor registers untouched (table 8-19), so after POR the register still holds `0x8400` until written.
  Prompts 17 and 18 add no second write; `startConfigure` requires initialized (OP-2), which includes this step, and they test 10M profiles after it. Prompt 22's `startVerify` compares it under `0xFFC0` without writing, and restoration runs through `startInitialize`." Replace ARCH:239-241 with a
  pointer to DB.
- **G1-8** S3 [A] P15:69-70 "before RCSR or other mutation" implies init writes RCSR, which has RC bits. **Fix:** "before
  any mutation. Through prompt 32 initialization does not write RCSR. Prompt 33 adds interface control and its replay; because RCSR[3:2] are RC, it reads RCSR through the collector and composes only owned stable bits."
- **G1-13** S3 [L] P15:67 has two readings: `W(00,8000)` or read then write. **Fix:** "A single literal whole-word `W(00,8000)`, never composed from a sampled word. AC:1398's earlier check for an already-active reset is a separate state read and stays. Desired fields are replayed after."
  Same for `W(1F,8000)` and `W(1F,4000)` in P16. For D15 these are the sourced whole-word case (SNLA266B table 5-1). That zero in other BMCR bits of the same frame is harmless is an inference, because the reset reloads them.
- **G1-14** S3 [L] P15:26-28's list omits the only escape from failed cleanup. **Fix:** replace it with:
  "Implement `startInitialize` with reset kinds NONE and PCS, `pollOperation`, `takeResult`, `cancel`,
  `operationSnapshot`, `abandonCleanup`, `invalidateDeviceState` and `isInitialized`, per the operation rules in AC."
  Change P15:50-52 to the OP-9 barrier and delete P15's "reserved/separately admitted" wording.
- **G1-11** S3 [L] P15:71-72 and P16:67-69 understate timing. Figure 7-1 draws T4 from the end of the VDDA3V3 ramp, and T1 to T3 can add up to 300 ms before it. **Fix:** "at least 50 ms after both supplies reach final level before SMI. If the external clock is not available at the power ramp, either hold
  RESET_N low and release it at least 100 us after the clock is stable (Figure 7-1), or apply an additional RESET_N after POR completion, initiated at least 100 us after clock stabilization and POR completion (7.6 footnote 2). SMI then stays inactive 2 ms after release."
#### Prompt 16

<a id="g1-3"></a>
- **G1-3** S2 [L] (also **G11-9**, D14) P16:64-65 "Poll `0x1F`" with AC:1392-1393 and default `pollIntervalUs` 1000 places the
  first read about 1 ms after a command that table 8-48 says "has the same effect as Hardware reset pin", inside the window
  of 8.3.11: "must remain inactive... at least until 2ms after reset is de-asserted". A read during reset could return zero and
  falsely show bit 15 clear (inference). **Fix:** "Schedule the first PHYRCR read no earlier than write-completion upper bound + `max(2000, pollIntervalUs)` us via `nextPollAtUs`, admitted by OP-6's formula immediately before the command, then poll `0x1F` mask `0x8000`. 2 ms is a minimum silence, not a completion
  maximum, and because the internal release time after the write is undocumented it does not prove table 7.6's T2 is met; Q5 measures this. Restart `0x4000` and BMCR reset have no such window. Core cannot stop other PHYs: the
  lifecycle guide requires the shared-bus owner to withhold frames for the same window. Test no frame before due with a 1000
  us interval." Update AC:1392-1393, ENG:71, LEDGER `hard_reset.waits` and `calibration_recovery`. The floor starts on any attempted write and covers
  every frame source (OP-14). The EEE recipe's leading `001F=8000` and SNLA266B's compliance recipes need the same floor.
- **G1-10** S3 [L] P16:25-28, DB:64 and gate `hard_reset_straps`: 8.4.1 says straps are "sampled at power up or hard reset", table 8-47 names `Hard Reset (Reg 0x1F[15])` and table 8-48 equates it to the pin. Only SNLA423A and its DP83826A twin SNLA491 call BMCR[15] "hard reset". Against that, table 8-48 names the field
  "Software Hard Reset" and 8.4.1 says "During software resets, the strap options are internally reloaded". **Fix:** "PHYRCR hard reset: best reading is that eligible straps are resampled (8.4.1 'hard reset', 8-47, 8-48), although the field name keeps 8.4.1's software-reset sentence arguable. ENHANCED Strap1 and Strap6 are not re-latched. BMCR reset reloads stored
  values. Keep gate `hard_reset_straps` open with this wording, report provenance as possible resample, and confirm in Q5." The code path is unchanged.
- **G1-7** S3 [A] P16:16 and :27-28 require POR-versus-reset differences, but `InvalidationReason` (AC:571-573) cannot express
  POR. **Fix:** name `invalidateDeviceState`; add `EXTERNAL_POWER_ON`. It means all straps were latched again in either mode. It differs from `EXTERNAL_RESET` only for ENHANCED Strap1 and Strap6, which a reset pin does not re-latch (tables 8-7, 8-9). Make no POR-only claim for BASIC.

#### Prompt 17

- **G2-2** S2 [I] (D8) P17:18-22 and :65-66 order an exchange while AC:256-259 and LEDGER gate it as SOURCE_GATE; no page API is
  frozen. **Fix:** "Implement ledger `next_page_exchange`, the IEEE Clause 28 sequence that 826 8.3.1 designates. Gate
  `next_page_handshake` becomes a physical case for 72-73. Freeze the typed page API here." Update AC and the LEDGER. Say "IEEE
  Clause 28", not "documented".
- **G2-3** S2 [I] (D8) A persistent ANAR NP bit with a one-shot exchange stalls every later AN against a partner whose base page has NP=1: IEEE 802.3-2012 Figure 28-18 leaves COMPLETE ACKNOWLEDGE only with `mr_next_page_loaded=true`, TRANSMIT DISABLE clears it on every restart, and 28.2.4.1.6 sets it only on a
  register 7 write. A partner with NP=0 is unaffected (28.2.3.4). The IEEE text is recorded in `docs/reports/implementation/3-source-audit.md`:110-129; DP83826 silicon is unmeasured (Q10). **Fix:** "NP=1 obliges the owner to service pages on every AN cycle. Set NP=1 only for the admitted epoch. On success the operation's last step writes ANAR NP=0,
  with no restart because AN is complete. On timeout, cancel or failure the operation ends at once under OP-9 with `cleanupOwed` set and issues no further write; NP may remain 1 and the result says so. The restoring `startInitialize`, or `startConfigure` when the exchange was admitted as CONFIGURE, writes NP=0 in its ordinary ANAR replay and restarts AN. Reset
  replay never writes NP=1. Test re-AN with NP=1 and no page load: report explicit pending or stalled negotiation, never silently down."
- **N-3** S2 [L] (D8) `bool nextPage` sits in persistent `Advertisement` (CFG:85, AC:177, AC:257), and LEDGER `configure_an` owns ANAR `0x8DFF` including bit 15. A profile with `nextPage=true` has desired NP=1 while hardware is deliberately NP=0 after the epoch, so `startVerify` reports a mismatch or a coder replays NP=1, and the stall returns. **Fix:** remove
  `nextPage` from persistent `Advertisement`; the NP=1 request belongs to the typed page API. Update CFG:85 and :163, AC:177, :252 and :256-259, and `test/foundation.cpp`:31, :96, :117-121. LEDGER `configure_an` ANAR mask becomes `0x0DFF`; `next_page_exchange` keeps `0x8000`.
- **G2-7** S3 [I] P17:49 verifies "all stable owned bits", but default `robust=true` is not applied before stage 18, so a default
  profile reaches a false VERIFIED, fails, or pulls stage 18 forward. **Fix:** "Until 18, trust covers ANAR, BMCR and the stage-15 VOD_CFG3 only. Doxygen and the stage report say crossover and extended duplex are not yet applied; non-default values reject at preflight."
- **G2-8** S3 [A] P17:16-17 and :34-35 "invalid selectors" have no field (AC:246-247). **Fix:** "The selector is always encoded
  `0x0001`; a non-default prior selector is overwritten and verified."

#### Prompt 18

- **G2-4** S2 [L] (D5) P18:28-33 and :75 reopen a rule CFG:97, AC:250 and `test/foundation.cpp`:121 froze. Table 8-28: "When using in
  Force Mode, Robust Auto-MDIX shall be enabled". **Fix:** "Every forced profile writes CR1[5]=1. PHYCR[15:14] is the only documented routing selector:
  `0x8000` automatic (bit 14 preserved), `0x0000` MDI, `0x4000` MDIX. Forced manual with CR1[5]=1 is legal, not contradictory; link
  and routing with it are a physical case."
- **G2-5** S2 [L] P18:35-36 says opt-in; AC:254-259 and LEDGER require partner evidence no type can carry, so CR2[5] is unreachable.
  Table 8-29 lists no prerequisite. **Fix:** "`extendedFullDuplex=true` is the owner's declaration; no preflight gate. Write CR2 `0x0020`
  only, preserving other bits. Doxygen warns this changes the IEEE result against a forced-100 partner." Reject forced-10 plus extended
  as LINK_COMBINATION and update `test/foundation.cpp`:120-122. Rewrite LEDGER `extended_link_configure` step 1 to match, and change P18:36-37 to: "With CR2[5]=0, parallel-detection results must not gain the extension; negotiated pause is never inferred." P19 case: AN enabled, CR2[5]=1, ANER[0]=0, PHYSTS[2]=1 yields 100/full from PHYSTS with `pauseValid=false`; host test only, physical behaviour unmeasured.
- **G2-6** S3 [A] P18:34 and :43-45 "advertisement stays available" contradicts CFG:160-163. **Fix:** "A forced profile carries no AN
  intent and neither writes nor verifies ANAR. Returning to AN is a new `startConfigure` with a complete Advertisement."
- **G2-10** S3 [A] P18 never names port mirror, which it owns (COV:43). **Fix:** "CR3 `0x0060` is port mirror (table 8-30), not two
  independent corrections; include it in the matrix, Doxygen and vectors."
- **G2-16** S3 [L] P18:76-77 and P22:66-69 test features from later prompts. **Fix:** move P18:76-77 to P23: "Loopback exit restores PHYCR[15:14] and CR1[5] from the persistent profile, including manual MDIX, with failure at each write." Also move P22's temporary-diagnostic override sentence and overlapping-field restore test to P23. P22 keeps the single restoration
  ledger, one owner per stable field, and the interrupted reset/reapply test. Add CR1 `0x0020` and PHYCR `0xC000` to LEDGER `loopback_near.ownedMasks`.
#### Prompt 19

<a id="g2-1"></a>
- **G2-1** S1 [L] (D4) P19:35-36 and :66-68, AC:534, AC:1003-1010, DD:226, DD:261-264, DB:123-126, DB:252 and gate `forced_physts` keep forced mode unresolved indefinitely. Table 8-19: "Disable Auto-Negotiation - bits [8] and [13] determine the port speed and duplex mode". Table 8-34: "ENHANCED Mode : 1 when Auto-Negotiation is disabled", which read literally
  is always 10/full and contradicts strap table 8-8 ("Force mode 100M enabled") and BMCR selection. The BASIC wording cannot be a bit copy either, since tables 8-14 and 8-15 use the opposite polarity. The datasheet does not say what these bits do in silicon. A contract-following adapter never publishes UP on a forced link, because SDK publication needs the tuple. The
  gate's next evidence may never arrive, since A repeats the wording. **Fix:** "When a coherent pass reads BMCR[12]=0 and current link up: speed=BMCR[13] (1=100, 0=10), duplex=BMCR[8], `resolvedModeValid=true`, `pauseValid=false`. This describes the local forced mode as read from BMCR, not the partner's duplex. Exception: when BMCR reads 100/half and CR2[5] is not known to be 0
  (desired `extendedFullDuplex=true`, or configuration trust not VERIFIED), speed=100, duplex UNKNOWN, `resolvedModeValid=false`, because table 8-29 makes the result depend on the partner's mode. The SDK adapter rejects forced 100/half with `extendedFullDuplex=true` before mutation, since it could never publish UP. Never use PHYSTS[2:1] while AN is disabled; retain it raw. Add no read: BMCR is
  already in the 10-read plan." Test: forced 100/half with PHYSTS low bits `0x0007` yields 100/half. Narrow `forced_physts` to a non-blocking question about the raw value. Update AC, DD, DB, RV:144-145, LIBC:13, :134, :270, :354, `docs/reference/dp83826i-feature-inventory.md`:268, P36:71, COV:43-44, :63, and LEDGER `configure_forced` step 5, `extended_link_configure` step 3, `observe_link` step 5, `collect_event_counters` step 2 and PHYSTS `resetNotes`.
- **G2-11** S3 [I] P19:37-38 gives two pause sources and no rule. **Fix:** "Compute RX/TX from ANAR and ANLPAR with the Table 28B-3 oracle. Keep
  PHYCR[13:12] as a cross-check; on disagreement `pauseValid=false` with raw words retained."

#### Prompt 20

- **G2-12** S3 [L] (also **G3-7**, **G9-9**) P20:16-25 and :67-73 omit read-clear dependencies that appear only behind links (LIBC:303-307, LEDGER `collect_event_counters`). Table 8-34 clears PHYSTS[13] on RECR read, [12] on 10BTSCR read, [11] on FCSCR read, [8] on ANER read, [7] on MISR read and [6] on BMSR read. On 826 RXFS[12] resets to 1, so MISR2[9] means WoL by default, and
  reading RXFS consumes status. **Fix:** "Collector also owns 10BTSCR `0x1A` (polarity `0x0010`; every RMW or readback is collector-routed), ANER `0x06` and RXFS status. Read PHYSTS before the registers that clear its mirrors when those mirrors are wanted. MISR2 `0x0200` stays raw polarity-or-WoL unless owned RXFS[12] state is known; add no RXFS read only to decode it, and any requested RXFS read retains its status through the collector. On 826
  reset selects WoL, so polarity needs RXFS[12]=0, written by prompt 27 as a whole word `0x1000`/`0x0000` without a read; all other bits are R or RCH with no write action in table 8-18. Verify with one collector read. Enabling WoL (RXFCFG1[7], which resets to 1) or a hard reset returns the state to WoL or unknown." DP83825I RXFS matches 826 on this bit (table 7-84). **DP83822 differs:** it shares MISR2 bit 9 (table 8-19), but its RXFS[12] resets to 0 (table 8-88), so polarity is its default and enabling WoL sets it to 1 automatically; take the
  reset meaning from the part table, and because its status bits are W1S, write only zeros there.
#### Prompt 22

- **G2-15** S3 [L] P22:16 and :41-42 "Select required verify/reapply/reset steps" reads as auto-escalation, and
  P22:49-52 describes a deferred cleanup allowance. **Fix:** P22:16 becomes "`startVerify` and reconciliation through
  the existing `startInitialize` procedures". P22:41-42 becomes "Options select the steps; the driver never escalates.
  `startVerify` performs no mutation; `reset=NONE` performs no reset. Insufficient evidence gives an untrusted result, not
  a stronger action." P22:49-52 becomes OP-9.
#### Prompt 23

- **G3-3** S2 [I] P23:31, :41-43 and P24:29-30: SNLA423A pp18 and 20 add `0x001F=0x4000` after reverse and analog selection, and p20 adds `030B=3380`, yet ledger `loopback_reverse` and `loopback_near` have no restart. **Fix:** P23: "Register this as an explicit difference under `loopback_recipe` and add `001F=4000` to the reverse and analog start sequences only, with a bounded relink wait for
  reverse. MII and PCS modes get no restart. The datasheet (8.3.14.5, 8.3.14.6) and SNLA423A 2.5.2 and 2.5.3's numbered steps give none; only the scripts on pp18 and 20 do. It preserves registers per table 8-48, so it adds no register restore debt." P24: "After any restart, read PHYSTS link through the collector before writing PRBS mode. `030B=3380` remains unadopted: its [11:6]=0x0E is outside the 0x28..0x38 range at LIBF:166. If analog BIST cannot lock, report inconclusive and cite the gate."
- **G13-3 (P23)** Add: "Owns temporary PHYSCR[9:8] FIFO depth for `loopback_reverse`: save, set, restore. Persistent control is `tx100_configure` at P34."

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

  Datasheet tables 8-56/57 differ (0173 = 0D07/0D14/1A21/343B/8F6F; 0175 = 1004/1004/1005/1008/100B; auditor-read). **Fix:** "SNLA428 is the exact-part TDR procedure named by 826 8.3.16.1 and listed in 10.1. Implement its runs in order 1A, 1B, 2, 3, 4, 5 as literal vectors, writing 0170, 0173, 0175, 0178, 0416, 0411, 0456, 001E in that order. The caller may select a subset (SNLA428 step 4); a run not performed is marked not-run, never zero-filled. A run completes only when 0x001E[15] has self-cleared and [1]=1 under the ledger's freshness rule, because reset and the previous run both leave [1]=1. Retain [0], then read 0x180/0x185/0x18A. Tables 8-56/57 print different words: record source conflict `tdr_recipe`, keep both as vectors, and keep the typed fields able to program either. All six CDSCR words select transmit A, same-channel capture, so the recipe tests channel A only: no result may describe pair B or the whole cable. Channel B and cross-pair runs stay raw typed settings with no derived distance; no TI recipe exists for them."

  Add a procedure-only record for 0x0411. Extend 0x0416, which overlaps EEE's `0416=1F30`. Add separate TDR write actions with gate `tdr_recipe` in `WRITE_ACTIONS` and `ACTION_GATES`, so the record's EEE gates do not block TDR. Write 0x0456 as read-modify-write on mask 0x0600 with an explicit reserved-bit exception under D15: GENCFG[3] is prompt 33's. With [3]=1 (reset) this equals TI's 0x0608; with [3]=0 it is 0x0600, which TI did not print, so cover it in Q7. Restore: GENCFG is readable, so restore [10:9] from the saved word. 0x0411 and 0x0416 have no documented read or reset value and PHYRCR[14] preserves registers, so the only documented exit is a hard reset and replay through `startInitialize`, which also replays EEE. Configuration trust stays false until then.
- **G3-2** S2 [L] P25:29-31 and :62-64, LIBF:111, AC:1163, DB:213-216 and gate `tdr_calibration` withhold a distance and open/short reading that SNLA428 supplies. **Fix:** "Maximum completion time and freshness remain gates. Provide a pure helper over retained raw samples, valid only for samples taken with the SNLA428 recipe: scan the runs performed in order 2, 3, 4, 5, 1B, 1A and stop at the first whose `0x185 & 0x7F`
  exceeds decimal 10 for segments 2-5 or decimal 24 for segment 1, using that same row's `0x180 & 0xFF` and 0x18A. Metres = |(index - 5) * 8 / (2 * prop_dly)| with caller-supplied prop_dly (table 2-4: Cat5/5e 5.35, Cat6 5, Cat7 4.6). 0x18A zero = open, nonzero = short (SNLA428 2.2.1; table 8-71 bit 5 also says short). Report raw bits 11 and 5 separately; no source maps bit 11's value to a polarity. No row over threshold returns NO_PEAK, never good cable or zero metres. Raw values stay available. Physical accuracy stays
  NOT RUN." Test the note's table 2-5 as a whole: rows 2-4 zero, 5 = (0x78, 0x11, 0), 1B = (0x0C, 0x06, 0x800), 1A = (0x06, 0x44, 0) gives segment 5, open, 100 m within 1e-9. Do not copy the note's code: it is copyrighted and inconsistent (MATLAB pairs `peak_val(5)` with `peak_indx(2)` and `peak_val(6)` with `peak_indx(1)`, `tmp` is undefined, prop_dly 5.2 differs from table 2-4, and flowchart `threshold2` has no value). The 24 threshold comes from MATLAB only.
- **G3-5** S3 [A] P25:49, :66-67, :72-74 and P26:45-46, :65-66 route TDR words through read-clear machinery. Tables 8-61, 8-66 and 8-71 show type R. **Fix:**
  "0x001E, 0x0180, 0x0185 and 0x018A are non-destructive. A failed read gives per-word invalid status with no history-loss flag. Bounded re-read is allowed. Only BMSR, PHYSTS
  and MISR reads used as prerequisites go through task 20's collector." Delete "clear/freeze order". Table 8-47 is mixed R/W and R, with no read-clear.
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

- **G4-4** S2 [L] P28:49-52: table 8-19: "this bit is OR'ed with the input from the INT/PWDN_N (in ENHANCED mode) pin. When the active low INT/PWDN_N is asserted, this bit is set."
  So no separate request readback or actual-state register exists, and any BMCR RMW composed from a live read can write bit 11 back as 1. **Fix:** "Report `requestedPowerDown` from
  owner state and `observedBmcrPowerDown` from the read. BISCR[8] is active sleep only. Compose every BMCR write's bit 11 from desired state, never from the read. Readback 1 after a clear is
  `WAKE_BLOCKED_OR_UNCONFIRMED`, with one write and no repeats." The sticky hazard itself is unmeasured (Q6).
- **G4-7** S3 [A] P28:15 never names PHYSCR[15], which LEDGER `power_pll_control` assigns here. **Fix:** "PHYSCR[15] PLL disable (mask `0x8000`) is a separate operation, admitted only in IEEE
  power-down with application approval of clock loss; it is not a substitute for the `0x0428[2]` DPD recipe. The active-sleep mask `0x7000` must not touch it."

#### Prompts 29 and 30

<a id="g4-1"></a>
- **G4-1** S1 [L] (D2) P29:25-27 and :38-44, RV:326 "Guessed lower offsets 0001/0014/003C fail", the inventory:317-323, LIBF:123 and :142, P36:71, SRC:70 and `test/mmd.cpp`:275-280 all block on an address question the sources nearly answer. The evidence chain:

  - DEVAD is established for this part: 826 8.3.11.1 gives 3 and 7, "preceded by MMD3" and "MMD7".
  - 825, strongest: table 7-92 and headings `1014h` and `203Ch`, with text "Register 0x003C and Register 0x003D in MMD7" and "Register 0x0014 in MMD3".
  - 822: table 8-133, and 7.4.2.9 is a worked trace for `0x703D`: write `0x0007` to `0x000D`, `0x003D` to `0x000E`, then `0x4007`. The printed high nibble is not sent.
  - SNLS603D idx 46 states the notation for another part: "most significant nibble of the register address is used to denote the respective MMD space. This should be ignored during actual register access operation." Its nibble equals the MMD number, while here `1`/`2` map to MMD3/MMD7, so it is an analogy only.
  - The low bytes match IEEE 3.1, 3.20, 3.22, 7.60 and 7.61 (IEEE from knowledge).
  - Counter-evidence: A 8.3.3.1 has a literal column `7h / 203Ch`, and SNLA328 tables 4 and 5 print `03 / 1001` and `03 / 1014` for 825, whose own datasheet uses 0x0014, which weakens A's column. No exact-part wire example exists. The prefix digits differ by family, so constants are not shared.

  `(31,0x203C)` is excluded by the exact-part text. Rows at or below `0x04D1` use DEVAD 31 per table 8-31. Rows `04D3, 04DF, 04E0, 04F4, 04F5` exceed that prose and rest on 8.3.11.1 item 1 ("general MMD register accesses"), source-implied, with A printing `1Fh` for 04DF, 04E0, 04F4 and 04F5.

  **Fix:** in P29, replace from "These are printed facts" through "need exact-part addressing evidence." at :25-27, and replace :38-41 plus the sentence "Missing essential mapping evidence blocks that procedure and remains required work" at :43-44. Keep "Record independent literal selector/address/data traces and provenance for every procedure-only access" and "other documented controls and unrelated tasks can proceed". New text: "DEVAD 3/7 is established (8.3.11.1). Adopt the five D2 tuples as source-implied, recording the chain and the A and SNLA328 counter-readings. Hold them in one constant table, and never route them through the DEVAD 31 C22 alias map, where offsets 0x01, 0x14 and 0x16 are BMSR, FCSCR and BISCR. Before the recipe's `001F=8000`, read `(3,0x0014)` and require the word to equal `0x0002` (cross-part: 825 table 7-100, 822 table 8-139; no exact-part table). Failure stops with zero writes. After the hard-reset wait, write `(7,0x003C)=0x0002` as the recipe's second row, read back and require the word to equal `0x0002`. Failure stops before the remaining 16 rows, leaves a hard-reset PHY and follows OP-9 and OP-13. Route `(3,0x0001)` (latched) and `(3,0x0016)` (read-clear) through the collector. Keep both checks until a probe closes the gate." A word comparison rejects `0xFFFF`, which a bit test would pass. The default bypass state could cause a false failure, which blocks EEE rather than shipping wrong behaviour, and Q1 tests it.

  Extend `mmdBatch` to take a DEVAD, with literal vectors `0x0003, 0x0014, 0x4003` and `0x0007, 0x003C, 0x4007`. Rewrite gate `eee_wire`, the `eee_printed_*` records and ledger:11355-11358, :11364 (floor 74 frames) and :11422-11425. Change `tools/verify_ledger.py`:155-156, :596, :601, :604-606 and `test/test_remaining_ledger.py`:397-419. Update `test/mmd.cpp`, RV:324-326, inventory:255-258 and :317-323, LIBF:123, :129-130 and :142, `docs/reference/library-ti-core.md`:108 and :215-218, P36:71, SRC:70. Probe: Q1.
- **N-1** S1 [L] (D2) Gate `eee_control_semantics` also blocks `eee_initialize`, `eee_negotiation_status` and `legacy_lpi`, and its decision reads "Do not invent safe RMW, readback or status fields from another part". That directly forbids G4-1's capability check, G4-2's `0x04D1` RMW and readback, Q1's `(7,0x003D)` bit 1 and the collector's bit meanings, so G4-1 alone would leave prompts 29-30 blocked and the two gates contradicting each other. **Fix:** rewrite the gate: "The recipe runs as printed opaque full words. Cross-part fields (825 tables 7-99 to 7-103, 822 tables 8-138 to 8-142) are adopted as source-implied for bits 3.1[11:8], 3.20[1], 7.60[1], 7.61[1], and 3.22 as a read-clear counter, in the same constant table and under Q1. `0x04D1` gets only exact-part bits 0 and 3 (8.3.3.2) and bit 12 (8.3.4), readback by those bits. Disable stays hard reset plus reapply, with no invented inverse." `eee_initialize` also carries `pin_fields` because of the recipe's `0308=0000` row and possible clock or pin conflicts: that row is a sourced whole-word write, so remove `pin_fields` from `eee_initialize`'s blocking gates, and instead reject the recipe before I/O if an owner holds a clock or LED route that `0x0308` could affect. The gate ids stay on the features, as `tools/verify_ledger.py`:118-119 requires.
- **G4-2** S2 [L] P30:1, :16-19, :33-34, :45-47 and :52-57, AC:1133 and LEDGER `legacy_lpi` model forced LPI as a finite session. 826 8.3.4: "Forced Tx LPI idles by writing 0x04D1.12 = 1", "Write 0x04D1.12 = 0 to stop", and "responsibility of enabling and disabling LPI signaling lies on the Host Controller". The part has no LPI wait, readiness event or session bound, and one finite operation would hold its slot and budget across an application-chosen dwell. "TX-inhibit" is wrong: the PHY transmits LPI, and MAC frames are not buffered. **Fix:** rename "forced TX LPI control". `startLpi()` and
  `stopLpi()` are independent bounded operations, each one `0x04D1` bit-12 RMW with readback comparing bit 12 only (no exact-part field table): after the recipe `0x008B` to `0x108B` to `0x008B`, generally mask `0x1000`. Cached state `forcedLpi {OFF, REQUESTED, UNCERTAIN}`. The application owns dwell, MAC drain and post-exit delay; 36 us is nominal. Failure leaves UNCERTAIN, reported as "forced LPI may remain active; frames sent now can be lost". Rejecting below 100 Mbit/s is policy [I]: 8.3.4 is silent and 7.6 gives only 100M timings. Forced LPI is a two-call session under OP-9: the barrier blocks other starts and writes while it is active; admitted reads and link polling continue (OP-14), and `readLinkStatus` publishes the link bit only (R-10).
- **G4-8** S3 [A] P30:23-24: 825 table 7-92 says bit 12 "shall be set after setting bits [14:13]"; 826 mentions bit 12 only. **Fix:** "Implement bit 12 only per 8.3.4. Do not import [14:13]. Prompt 73 observes
  LPI after `0x108B`; failure records this comparison as the next question."

#### Prompt 31

- **G4-3** S2 [L] (also **G1-15**, D16) P31:22 and :45-47, RV:300-303 treat W0C as settled. 826 table 8-90: "W0C ... this bit clears the level upon a write. 0h = Clear". But SNLA261A table 2-4 clears with `0981` (bit 11 = 1) on DP83822, and 822's type is `W, RH/W1S` (825: `W`, "0h = Clear"; A: W0C). TI Linux writes `value | DP83822_WOL_CLR_INDICATION` on the path shared with DP83826, which shows intent; on a true W0C part that same write is neutral. If silicon behaves like 822, every "neutral" `0x1981` erases the level and the acknowledge `0x1181` leaves it stuck; host tests pass either way. **Fix:** keep
  W0C and add: "SNLA261A, 822 and TI Linux use write-1 on family parts. Implement the exact-part table and qualify in prompt 73 (Q3). Harvest RXFS and MISR2 through the collector before any RXFCFG1 write, and skip RXFCFG1 writes whose result is unchanged over stable mask `0x17E1` (bit 11 excluded), so evidence survives either polarity."
- **G4-5** S3 [A] (also **G8-11**, **G9-8**) P31:28-29 and :42-43 warn of a different DP83822 word order, and P59:31-35 and FAM:86 call packing unresolved. Wire values are identical; only labels differ. **Fix:**
  "DS byte 5 (MSB) is API octet 0. MAC `11:22:33:44:55:66` gives `04A2=2211, 04A3=4433, 04A4=6655`; password `2A..2F` gives `04A5=2B2A, 04A6=2D2C, 04A7=2F2E`. SNLA261A tables 2-3, 2-6 and 2-7 and TI Linux yield the
  same words; ignore byte labels. The real difference from DP83822 is custom pattern matching, which is not imported." Correct LIBF:127 too. On DP83822: example RXFCFG words clear the bit-12 CRC gate, so compose with bit
  12 preserved rather than copy them as vectors.
- **G4-6** S3 [A] P31:55 omits that WoL Enable is already 1 at reset (`108Xh`, bit 0 unknown; RXFS[12]=1). **Fix:** "Reset state is not WoL-off. Initialization does not write RXFCFG1 unless settings own it. Publish
  observed fields. An explicit disable clears bits 7, 5 and 0 with W0C neutral. Bit 0 is RH: confirm stability in qualification."

#### Prompt 32

- **G5-7** S3 [A] Ledger PHYCR[5] (:4348) and gate `pin_fields` (:13060) treat PHYCR[5] as sourceless, and P32 does not mention it. Table 8-43 names it "LED Configuration" with no description; SNLA344C 3.2.1 labels `0x8020` "enable LED0 config". That note also has values contradicting the datasheet (`0x0A=0x0001`, `0x460=0x0005`), so do not trust it as a recipe. **Fix:** add to P32: "Keep PHYCR[5] out of the typed profile until clarified; name it for the TI gate and Q12."
- **G5-10** S3 [L] P32:24 and :16 omit per-mode pin availability. **Fix:** "Field encodings are mode-independent except polarity: BASIC polarity is printed as fixed active low (tables 6-1, 8-49; 8.3.18) although table 8-42 offers an override, so reject BASIC polarity settings under `pin_fields` and Q12. Profile validation also depends on mode. BASIC exposes LED0/LED1 only. ENHANCED RMII exposes LED0-3. ENHANCED MII exposes LED0/LED1, plus LED2 only if COL is not consumed and `0305[2:0]=0` is written and read back; the printed reset is already 0 while table 5-1 says pin 28 defaults to COL in MII, so do not infer the pin function from the code. LED1 in ENHANCED always depends on CLKOUT selection (Strap6 or IO_CFG1[6]). LED3 on ENHANCED MII has no writable route: table 5-1 promises one, table 8-77 types the mux R, so reject it and keep the conflict under `pin_fields`. Offer no TX_ER on pin 28: table 5-1 names it, table 8-76 has no code."
  Add to `pin_fields`: figure 8-10 shows pin 31 codes 5 and 6 as LOW, while table 8-75 says TX_ER and CLKOUT25M, and reset 0 conflicts with "default in ENHANCED". Do not expose codes 5 and 6 until clarified; `pin_configure` tests use table 8-75 literals only.
#### Prompts 33 and 34

- **G13-3 (P33)** Add: "`mii_configure` and `compatibility_configure` own CR1[1], CR2[1], PHYSCR[4] and GENCFG[3]."
- **G5-2** S2 [A] (also **G13-3**) P34 never names `tx10_configure` or `tx100_configure`, which COV:53-54 assign to it, and no prompt names squelch, 10M_CFG, 10BTSCR, scrambler bypass or FIFO depth. **Fix:** add to scope:
  "`tx100_configure`: PHYSCR[11] scrambler bypass, PHYSCR[9:8] FIFO 0/1/2/3 = 4/5/6/8 nibbles, CR2[2]. `tx10_configure`: 10BTSCR[13] lower threshold; [12:9] squelch codes 0..8 = 200..600 mV in 50 mV steps, reject 9..F; [7] NLP
  disable; [0] jabber disable; `0x002A[14]` 1=short, 0=long preamble. Reading 10BTSCR clears [4] and PHYSTS[12], so every RMW and readback goes through the task 20 collector." `tx10_configure` also owns VOD_CFG3 `0xFFC0`: require the D6 `4A40` contract and do not undo it. ANA_RX10BT_CTRL stays raw under `analog_rx_gain`. Change P34:19 to "tasks 17-20 and 33" and P34:30-31 to
  "Reuse task 33's CR2[1] control; add only the interaction test. Test CR2 bit 2 RX_ER-during-IDLE independently of GENCFG IPG."
- **G5-4** S3 [A] P34:47-49 and :62 "reject RMII use" also blocks a zero-cause disable, the only documented way to turn off a detector on a BASIC RMII board (table 4-3: "No (Always enabled)"). **Fix:** "Reject any nonzero cause set on RMII
  before transfers. A zero-cause request and FLDS collection stay legal. CR3 FLD nibble reset is unknown: observe it and assert no fixed value."
- **G5-5** S3 [A] P34:25-28 omits footnote 5: modes 2, 3 and 5 meet maxima only with RX-error or energy-loss enabled. **Fix:** "Accept those profiles but report no maximum claim otherwise. Selecting MLT3 programs `0117=0417`; descrambler programs
  `0131[5:0]=08`, per 8.3.17. Other codes are explicit raw overrides under gate `fld_threshold_recipe`."

#### Prompt 35

- **G5-1** S2 [L] (with **G13-2**, D7) P35:30-31, :35-36 and :69 order work the ledger blocks and name only two gates. Table 8-50: "Bit [4] in Register 0x0027 = 1, Enables 10Base-T Test Patterns", codes 0000..1101, and SNLA266B table 5-5 writes `0x0013`. **Fix:** replace `compliance_10_harmonic` with 14 exact write actions `compliance_10_pattern_0` to `compliance_10_pattern_D`: register COMPT, mask `0x001F`, value `0x0010|code`, with the existing forced-10M, hard-reset and 4A40 prerequisites. The API rejects 10M codes E and F before I/O; the COMPT field keeps E and F legal as 100M count nibbles (`test/test_remaining_ledger.py`:194-198). Keep COMPT `reservedMask` `0xFFF0` and `compliance_session` owned mask `0x000F`; bit 4 stays an action exception. Narrow `compt_reserved` to bits 15:5 and the 100M `0x0428` branch, and keep that gate id on the 10M actions, marked eligible as `0x0013` is today, because `tools/verify_ledger.py`:120 and :530 require it. Rewrite the `compliance_session` step, the action effect text and the gate text. Update `tools/verify_ledger.py`:105, `test/test_remaining_ledger.py`:273, :286 and :389, RV:334-335 and DB:318-320.

  P35 becomes: "Open gates: `ioctrl_example`, `analog_rx_gain`, `compt_reserved`, `hard_reset_straps`. Eligible: SNLA266B tables 5-1, 5-2 and 5-4 through the composed crossover, BIST and loopback procedures (G5-6), and all COMPT 10M patterns, each with hard-reset stop. The 100M `0x0428` branch keeps typed validation and literal vectors but returns a gated rejection before any I/O." Change "all documented options" to "every eligible option, plus zero-I/O rejection of each gated option".
- **G5-6** S3 [A] P35:33-34: PHYCR `0021` also sets undocumented R/W bit 5 and clears bits 7 and 15; `7101` contains read-only BISCR bit 8. **Fix:** "Forced MDI/MDIX composes only PHYCR[15:14], preserves bit 7, does not write bit 5 from the script, and ignores
  RH [4:0]. In `7101`, write only BISCR [14:12] and [4:0], mask `0x701F`, value `0x7001`; no test expects bit 8 as written." Cite P35:32-34. Sessions that force speed go through `configure_forced`, so D5 applies and CR1[5]=1 although TI's scripts omit it; record this as a difference from the script and check it on the compliance fixture (Q19).
- **G5-8** S3 [A] P35:71-72 "entry/exit preserves" unrelated power/isolation settings is impossible, since every SNLA266B recipe begins `0x1F=0x8000`; the read-only address half stays testable. **Fix:** "Test that no session write sets BMCR[11:10] or targets PHYCR[4:0] except by explicit composition, that the session is refused while IEEE power-down or isolation is requested, that identity and address are re-read after reset, and that after stop/reset a later `startInitialize` (OP-9) restores the persistent profile with one finite reapply and readback."
- **G5-9** S3 [A] P35:53-54 and :74 require calibration "completion" evidence that does not exist. **Fix:** "The contingency is a bounded CDCR read; if bit 14 is high, clear only `0x4000` with bit 15 written 0, then PHYRCR `0x8000`. Completion evidence is the hard-reset
  procedure's own; nothing proves calibration completed."

#### Prerequisites

- **G5-11** S3 [A] P35 needs 24: BISCR `7101` reuses BIST and no chain reaches it. Direct-list tidying only, already transitive: 11, 12, 15, 17 on P36; 17, 19 on P34; 28 on P33.
### 7.2 WP4: prompts 37-42, native

<a id="g6-1"></a>
- **G6-1** S1 [L] (also **G11-1**, **G6-13**, D1) P37:10-17, DD:293 and :300-312 ("required dependency, not an alternative wrapper-only implementation"), ARCH:82-86, AC:1106, SDK:150-151 and :229 require forking `esp_eth` and `esp_hal_emac`, changing the
  public install contract. No document says how a consumer gets this, prompts 69-71 have no delivery path, and it contradicts ENG:22. P37:101-102 also conflicts by allowing a preflight path. Confirmed in pinned source: install ignores `set_mediator` and
  `reset_hw` results and proceeds to `mac->init`; `esp_eth_stop` uses non-blocking `esp_timer_stop`; `esp_timer.h` documents polling `esp_timer_is_active()`; `esp_timer_is_active` is true "if it is armed or its callback is currently running"; task dispatch is
  strictly serialized. Upstream commit `d2ed96d2` changes the stop to `esp_timer_stop_blocking(..., portMAX_DELAY)`, which SDK:208-211 dismissed, but that wait is bounded by the adapter's own `get_link`. **Fix:** replace P37:10-17, :98-105 and DD section 6:

  1. The owner drives board reset and waits readiness by elapsed time, with no MDIO, **before** `esp_eth_driver_install`. SMI is configured in `mac->init`. REF_CLK from the PHY must exist for `mac->init`, whose software reset polls in 10 ms steps up to `sw_reset_timeout_ms` (default 100). Reset failure therefore precedes MAC init. Adapter `reset_hw` returns OK with no action; `set_mediator` only stores.
  2. Adapter `init` is local and `autonego_ctrl(G_STAT)` returns desired AN without MDIO, so neither can fail. Any install error leaves only `mac->del` and `phy->del`. No cleanup-only handle exists.
  3. After install, the owner runs `startInitialize` and configure through the typed route (direct MAC frames, same batch lock). Failure takes the ordinary uninstall path.
  4. `esp_eth_start` error: call `esp_eth_stop`. The timer was never armed, so it sets FSM STOP and returns ESP_ERR_INVALID_STATE, skipping link-down and STOP. Any other `esp_eth_stop` error can also skip them (`esp_eth.c`:338, :341). After step 5(a): if UP is published, call adapter `set_link(DOWN)`. If START was posted (the adapter's `get_link` ran in Starting) and STOP was not, post `ETHERNET_EVENT_STOP` with `&hdl`, `sizeof(hdl)` and finite ticks.
  5. After every `esp_eth_stop`, in this order. (a) Fence: an adapter-owned ESP_TIMER_TASK one-shot created before install; `esp_timer_start_once(fence, 0)`; poll `esp_timer_is_active` with a task delay, not from the esp_timer task, to false under a finite deadline. Retry by polling, never by re-arming. Timeout consumes nothing and resources stay held. (b) If the journal shows UP published, call adapter `set_link(DOWN)`, because an in-flight callback can publish UP after stop's link check and restart the MAC (`esp_eth.c`:151-152, `esp_eth_mac_esp.c`:406). (c) `mac->stop(mac)` to read the HAL result that `emac_esp32_set_link` discards, with finite retry. Each call can busy-wait 2500 us, so hold no lock. Non-OK goes to step 7.
  6. Teardown after step 5, or if never started. Stop every `esp_eth_transmit` caller. If a netif is attached, wait under a finite deadline for the owner's own `ETHERNET_EVENT_STOP` handler, then `esp_eth_del_netif_glue`. `mac->set_mediator(mac, &stub)`, with a static stub and all five members set: `stack_input` and `stack_input_info` free the buffer and return OK, `on_state_changed` returns OK, register hooks return NOT_SUPPORTED. Do not use `on_lowlevel_deinit_done`. Wait for the RX in-flight count to reach zero (G6-8). `esp_eth_driver_uninstall`: its only reachable errors at the pinned source are the FSM and ref-count rejections, which have no side effects and no public getter, so the owner tracks them. `mac->del`; `phy->del`; `esp_timer_delete(fence)`; destroy the netif last. Documented SDK limit: the stub swap is not synchronized with the RX task; a task preempted between loading `emac->eth` and the call can touch the freed driver. No public API closes this window.
  7. A failure this route cannot resolve is terminal for this boot: keep MAC, PHY wrapper and borrowed resources alive, keep admission closed, report the record, and recover by system restart. The HAL FIFO-flush timeout (`emac_hal.c`:680) is unobservable: a documented SDK limit, not a gate.

  Remove the correction requirement at P37:10-17, :98-105, DD section 6, DD:220 (`init` local, G_STAT without MDIO), :222, :234, :286, :347-348, :353, :424, :464, ARCH:82-86, AC:1106, :1374, :1643, SDK:150-151, :229, COV:18, `README.md`:23, `examples/esp_idf/README.md`:39-41. Also change P37:62-63 ("driver installation may perform MDIO/reset") and P38:74-75 (keep only the owner path). WP4 exit check: grep -iE 'corrected|lifecycle correction|verified SDK|SDK correction|cleanup-only'. History in CHANGELOG and progress stays. The fence, the stub, the stop order and the STOP wait are source-derived, not executed: Q13. SDK:208-211's reason for dismissing the upstream fix is also weak: the wait is bounded by `get_link`'s own duration under scheduling assumptions (G6-13).
<a id="g6-2"></a>
- **G6-2** S1 [L] P37:115-118, P38:87-91, P40:73, P42:54-58, VER:75-82 and DD:352-354 require failure injection on real SDK paths, but `esp_eth`'s `CMakeLists.txt` returns on target linux, a linked image executes nothing, and P4 runs belong to 72-73. Stages 37-42 cannot
  complete honestly. **Fix:** add evidence layer "Pinned driver source on host" to VER, reported as host evidence. An opt-in CMake test (off for consumers; NOT RUN if the input is absent) takes a supplied copy of `esp_eth.c`, `esp_eth_driver.h`, `esp_eth_com.h`, `esp_eth_mac.h`, `esp_eth_phy.h`, `esp_eth_spec.h` and `hal/eth_types.h`, checks each git blob hash (same pattern as `tools/probe_sdk_mmd.py`; a CI step may fetch them; not vendored), and compiles them with the adapter against small fakes for esp_err, esp_log, esp_check, esp_event, esp_event_base, esp_heap_caps, esp_timer, FreeRTOS headers, sdkconfig, soc_caps, sys/cdefs and a shadow `esp_eth_mac_esp.h`, with an instrumented `esp_eth_mac_t`. The esp_timer fake must copy the pinned return codes (stop on an unarmed timer returns INVALID_STATE). It proves real install, start, stop, uninstall order and ioctl routing; not timer concurrency, MAC behaviour or timing. A trial compile of `esp_eth.c` to an object with 54 lines of fakes succeeded; it was not linked or run. Acceptance for 37-42 becomes failure injection PASS on this layer plus P4 `idf.py build` link. Keep example
  owner state free of SDK headers so 41 and 42 state tests run on host.
- **G6-3** S2 [A] DD:117-131, AC:1626-1637 and :1660-1721 build poll loops, tokens and six journals for paths the SDK never calls: `phy->reset` and `phy->pwrctl` have no caller in `esp_eth.c`. **Fix:** `reset` and `pwrctl` return ESP_ERR_NOT_SUPPORTED and point at typed operations. Same for
  ETH_CMD_S_PHY_LOOPBACK if its procedure has waits, and for any other SDK-called callback (`autonego_ctrl`, `set_speed`, `set_duplex`, `advertise_pause_ability`): each completes as one terminal batch with no wait, or rejects and points at the typed route, so no callback needs a token or poll loop. Delete `ownedReset`. Reservation becomes one `Phase` enum (Stopped, Control, Starting, Running, Stopping, Maintenance) in a phase-by-method switch. One record: `{stage, firstError, cleanupError, publishedTuple, publicationUncertain, eventLost}`.
- **G6-4** S2 [A] P37:83-89, DD:190-198, DD:346, DD:359-363 and VER:79-80 require hook context resolution for hooks that always reject. **Fix:** two static functions, each `return ESP_ERR_NOT_SUPPORTED;`. Delete the P37:83-89 paragraph and only the context-resolution and two-handle clauses. Keep the shared-controller batch-participant case, the partial install/start/stop/delete effects and callback quiescence. Test: a real ioctl returns the rejection with zero MAC frames.
- **G6-5** S2 [L] P38:49-60 describes admitted forwarding hooks; DD:184-186 and :232 reject everything. Pinned ioctl goes straight through the mediator. **Fix:** "Raw ioctls reach the mediator outside PHY methods. Hooks unconditionally reject them, including from the owner task. The owner-routed typed raw
  API uses the core access policy and calls MAC frame methods inside the batch." Fix P37:78-79 to match.
- **G6-6** S2 [L] P42:34-36 requires TX ownership "through actual completion"; the DMA path copies with `memcpy` and no completion callback exists. **Fix:** "`esp_eth_transmit(hdl, buf, len)` borrows `buf` only for the call. ESP_OK means queued to DMA, not sent. ESP_ERR_NO_MEM is backpressure; ESP_ERR_INVALID_STATE
  means cached link down. If `CONFIG_ETH_TRANSMIT_MUTEX` is off (default n), call from one task only. RX: `stack_input` owns `buffer` on every return, including error, and releases with `free()`."
- **G6-7** S2 [L] P40:40-41 and `examples/esp_idf/README.md`:42 omit data-plane pins; `docs/esp32p4-board.md`:19-20 give no signal mapping; all three carry pre-2026-10-27 control pins (N-2). P4 macro defaults are MDC 31, MDIO 52, TX_EN 49, TXD 34/35, CRS_DV 28, RXD 29/30, CLK 50. Overriding only control and clock pins leaves TXD and RX on wrong pins with no error, on this board taking SDMMC GPIO28-30. **Fix:** require in `BoardConfig.h`: `.smi_gpio={40,39}`, RMII interface, `.clock_config.rmii={EMAC_CLK_EXT_IN,44}`, `.emac_dataif_gpio.rmii={.tx_en_num=49,.txd0_num=41,.txd1_num=42,.crs_dv_num=51,.rxd0_num=52,.rxd1_num=53}`, reset 54. Startup check: every field equals its BoardConfig constant and all pins are unique, because the macro pre-fills fields. Confirm against the final netlist before flashing. A colliding pin makes `esp_eth_mac_new_esp32` return NULL; handle it. State in the board doc that the P4 IO_MUX (`emac_periph.c`) fixes which pads may carry each signal.
- **N-2** S2 [L] `docs/esp32p4-board.md`:9-11, :18-20, DD:397-399 and `examples/esp_idf/README.md`:42 carry superseded pins (MDC 49, MDIO 43, TX 40). The board was re-pinned on 2026-10-27. EasyEDA `docs/ethernet-mcu.md`:133: "TX 49/41/42; reference clock 44; RX 51/52/53; MDC/MDIO/reset **40/39/54**"; `current/2026-10-27-ethernet-pin-order/README.md`: TX_EN 40 to 49, MDC 49 to 40, MDIO 43 to 39, confirmed in the compiled netlist. **Fix:** update all three with the signal-level mapping: MDC 40, MDIO 39, reset 54, TX_EN 49, TXD0 41, TXD1 42, CRS_DV 51, RXD0 52, RXD1 53, REF_CLK 44. Note that `ethernet-mcu.md`:255-263 and :320-321 are stale, and confirm against the netlist before flashing.
- **G6-8** S3 [I] P42:39-41 and :56-57 give RX teardown no mechanism: the RX task keeps draining after stop and dereferences `emac->eth`. **Fix:** application context with static lifetime that outlives `mac->del`; nonblocking callback that drops and frees after session end; order: stub swap (G6-1), then in-flight counter reaches zero under a finite deadline, then uninstall, then `mac->del`. The counter shows the application callback finished; it does not show the RX task is idle or close the pre-call window named in G6-1 step 6. Document that deletion in the pre-handoff window can leak at most one frame buffer.
- **G6-9** S3 [A] DD:87-96, P37:50-64 and P39:36-37 wrap `phy_802_3_t` though all I/O is overridden. **Fix:** `struct PhyHandle { esp_eth_phy_t phy; Phy* owner; };` with recovery by a first-member cast and one `static_assert`. Do not include the base header. Remove "base" from the conformance tables.
- **G6-10** S3 [A] P39:26-27 accounts for "10 ms reset polling, 100 ms AN polling", which the design removed. **Fix:** "These base loops do not run. Bounds come from 100 us MAC steps to nominal 1000 us per frame, `mac->init` reset polling in 10 ms steps up to `sw_reset_timeout_ms`, `mac->stop` up to 2500 us busy-wait per call, the owner's own polls and `check_link_period_ms`."
- **G6-11** S3 [A] P38:70-71 guards "another instance's address", needing shared state that DD:195 and ENG:45 forbid. **Fix:** `set_addr` returns OK only for the same value, otherwise ESP_ERR_NOT_SUPPORTED. Retarget by delete and recreate. Change DD:96 to match.
- **G6-12** S3 [I] DD:281-289 and P39:52-54 force stop/drain on a full event queue. **Fix:** "With the P4 EMAC, whose `set_link` cannot fail at the pinned source: after `on_state_changed(LINK,x)` returns, record published=x and never replay it; if it errored with a valid argument, set eventLost and let the owner re-post. With any other MAC, including an instrumented one: an error leaves publication uncertain and requires stop. The factory selects this; the error code cannot, since it carries both cases. Another MAC must re-audit."
- **N-4** S3 [A] `esp_event_handler_instance_unregister` falls back to `xSemaphoreTakeRecursive(loop->mutex, portMAX_DELAY)` when the mutex is busy, so a blocking default-loop handler makes glue deletion unbounded. **Fix:** add to P40: "Glue deletion has no deadline of its own; its bound is the longest default-loop handler, so handlers must not block."

### 7.3 WP5: prompts 43-55, CLI, runners, regression

- **G7-1** S3 [L] P44:47 cites "address-zero/broadcast" rules no document defines (CLI:35-36 only says "legal PHY addresses"); 826 and AI/AE document no MDIO broadcast. **Fix:** "Discovery reads PHYIDR1/PHYIDR2 only, no writes, over a requested inclusive range under one total frame and deadline budget. The default range is 0 to the declared part's highest strap-reachable address, a part-table value: DP83826I/E and AI/AE 7 (three address straps), DP83825I 3 (SNLS638C 6.3.11, table 6-9), DP83822 31 (SNLS505H 7.5.3). Any other range within 0..31 needs an explicit argument. Address 0 is read like any other. `0xFFFF` or `0x0000` is 'no response evidence', not an error. A transport failure is reported per address. Results are candidates; discovery never binds, initializes or selects. An isolated or powered-down DP83826 still answers on MDIO (table 8-19 bits 10 and 11, 8.3.6.2-3), so discovery says nothing about isolation."
- **G7-2** S2 [I] P43:61 "cannot interleave" is unachievable on P4 unless `esp_log` is routed. **Fix:** P47: "Install `esp_log_set_vprintf` so `ESP_LOGx` lines use the same bounded queue as whole lines. The hook is re-entrant and never blocks: it formats into a small local buffer, then queues under a short try-lock. If the lock or the queue is unavailable it drops the line and counts it. It never uses capacity reserved for a terminal record. Pin `CONFIG_LOG_VERSION_1`, where one call is one line; under V2 a line arrives in pieces. ROM, panic and direct `printf` output still bypass the hook. A machine record is queued as one unit. Records start at column 0 (G7-12). The runner treats a line as a record only if it starts with the prefix; garbled records are invalid, never repaired." Add one host test with log text between queue drains and one with the hook called concurrently.
- **G7-3** S2 [A] P43:45-54, CLI:74-76 and P48:36-48 build a protocol layer with receive windows and reconfiguration; INA228 frames with runner tokens (`hilrun`, `HIL_BEGIN`/`HIL_END`) and TMP1x2 with a runner nonce (`hilmark`). **Fix:** "The application supplies a non-constant u32 `boot` at CLI setup. `session <nonce>` passively prints the nonce verbatim, `boot`, the current `req`, identity and schema version. Each record carries `boot=` and `req=`. `req` is a 64-bit firmware counter from 1, incremented for every complete input line including rejected ones, with no exhaustion branch (as OP-3). The runner has one outstanding command, starts its count from the `session` reply carrying its own nonce, and treats any other `boot` or `req` as stale. After a timed-out command the runner sends only `session <new nonce>` and resynchronizes from that reply; no reply means no further commands. Reconfiguration is not a feature: delete P43:45-46's reconfiguration clause, P43:52-54, 'pending reconfiguration' in P43:75 and 'session replacement with pending work' in P54:72."
- **G7-4** S2 [A] P48:58-72 and CLI:99-107 mandate bundle trees and manifest cross-validation; `EvidenceFiles` in TMP1x2 is 51 lines. **Fix:** "Exactly two files per run: `<run>.json` (verdict, cases, identity, fixture, UTC times, commands, cleanup, transcript sha256 and length, truncation flag) and `<run>.txt`. Keep exclusive create and an INCONCLUSIVE placeholder. Close the transcript first, then write JSON by temp file, flush, fsync and `os.replace`. Offline `verify` checks hash, length, schema, and that the verdict is the worst case verdict in the order FAIL, then INCONCLUSIVE/BLOCKED/NOT RUN, then PASS. Zero cases, skipped-only runs and failed restoration cannot give PASS. Apply the same wording at CLI:99-107, P49:6 and :65, P52:55, P54:73, P72:42 and :57, P73:57, VER:61, COVERAGE.md:31 and ARCH:184."
- **G7-5** S2 [A] P43:72 creates a host fixture, but P47:26, P48:76-80, P50:62-65 and P51:64-67 rely on it and nothing specifies its faults, and P48:5 ties it to 47. **Fix:** P43: "One program `test/cli/dp8382x_cli_fixture` links the shared CLI and the real core over a table-driven simulated MDIO transport, feeding stdin bytes unmodified to stdout, with options `--identity`, `--partner`, `--fail-after N`, `--reboot-after N`, `--partial-record`, and test-only stdin controls `$pause`, `$resume`, `$tick N`, `$output-stall N`, handled before the CLI and absent from firmware. Register values are a plain array with only the behaviours a test needs. Campaign tests use recorded or synthetic transcripts for classification." P48's prerequisite becomes "fixture and schema from 43-46", so an SDK block in 47 does not stop 48-52.
- **G7-6** S2 [L] (D13) P44:70-72, P46:60-63, P47:52-58, P53:36-40 and P55:36-41 maintain an unnamed "exhaustive ledger". **Fix:** "COV holds the one matrix, one row per ledger procedure and feature id (52 and 35 for DP83826I): exact part and mode, CLI command, owner route, record type, host test name, physical case or N/A with reason. A unit test requires every id with non-empty cells, and requires every ledger field's `owner` to resolve to a row whose command or `cfg` path covers that field. 44, 46 and 47 append; 47 closes the CLI columns; 55 closes the matrix for the primary part; 53 adds only gaps. `docs/cli.md` holds grammar and records only."
- **G7-7** S2 [A] P43:26-30 says "framework-free" with 283 fields ahead. **Fix:** "Commands are one `static const` array of `{name, alias, minArgs, maxArgs, flags, handler}`, with a second table of `{group, name, enumValues or min/max, unit}` for `cfg set <group> <name> <value>`. Used by dispatch and help. No dynamic registration or virtuals. Each handler calls one public API, except the G7-8 sequencers."
- **G7-8** S3 [A] CLI rows, P44:44-48, P45:63-68 and P46:33-37, :52-54: discovery, dump, selftest, watch and stress have no API, and P46:46-47 forbids "another diagnostic state machine". **Fix:** "These are CLI-local sequencers over public calls: a small fixed struct (index, count, deadline) advanced by `tick()`, one batch per tick. Reword P46:46-47 to forbid direct uncoordinated MDIO and any second implementation of a chip procedure. Discovery uses a separate probe `DP8382x` object on the same transport: `bind` at each address, `readClause22` of 0x02 and 0x03, `end`. Before each batch it checks the bound instance's `operationSnapshot()` and returns BUSY/OPERATION_ACTIVE while state is RUNNING or `busQuietValid` is true. The bound instance is not otherwise touched." A direct transport callback must not be used: it would bypass the OP-14 floor, report validation and quarantine. The probe object is untested.
- **G7-9** S3 [I] P49:60-64 offers subprocess isolation, and pyserial open and close take no timeout on any platform. **Fix:** "Create the port with `port=None`, `timeout` and `write_timeout`, set DTR/RTS, then open. Never call `flush`/`drain`. Run open and close on a daemon thread joined with a finite timeout; on expiry record INCONCLUSIVE, finalize evidence and end with `os._exit` and a nonzero code. The serial backend adds no child process; driving the compiled fixture (P48:78, P49:75) is unaffected. Test with a fake backend whose open or close never returns."
- **G7-10** S3 [A] P50:62-65, P51:65-66 and P52 imply a hand-written negative set. **Fix:** "Generate negatives by mutating the positive transcript: for each required field, remove, corrupt or duplicate it and assert the verdict is not PASS."
- **G7-11** S3 [A] CLI:12 and P43:25 "freeze grammar" for commands that do not exist yet. **Fix:** "43 freezes tokenization, aliases, numeric parsing, group names, `<group> <verb> [args]`, the record schema and error vocabulary. 44-46 append commands."
- **G7-12** S3 [A] P48:38 and :42 read ambiguously on ANSI; "trailer" at P48:38 differs from "record" at P43:56. **Fix:** "Records are single uncolored lines at column 0 with the prefix. The console tracks its cursor and queues CR LF before a record if a prompt or partial line is open. The runner discards non-record lines and strips nothing from a candidate. Replace 'trailer' with 'record' at P48:38."
- **G7-13** S3 [A] P53:28-30 and :42-44 re-require what `test/mmd.cpp` already asserts (`0x001F`/`0x401F`, per-frame failure), and P54:70-74 re-requires tests that 43, 48 and 49 will ship; P53:7-8 and P54:6-8 state the principle without a mechanism. **Fix:** "List existing tests by name for each item. Add one only when none covers it, and for cross-entry cases."
- **G7-14** S3 [I] P47:35, P49:25-31 and P50:44-46 assume console type and equipment; the board documents show USB Serial/JTAG and no PHY interrupt pin, so INT pin cases on it are BLOCKED per P73:43-45, while polled status cases still run. **Fix:** P47 records console type (UART bridge or USB-Serial-JTAG). P49 records for that type whether baud applies and what DTR/RTS do at open. P50 lists the minimum partner in `docs/hardware-validation.md` and marks cases NOT RUN otherwise.
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

  `bind()` resolves `_part`. Policy functions take `RegisterMap` and search one array per map (DP83826 and DP83826A may share one address array while tables 8-17 and 8-18 match; their field masks differ). Procedures branch only on `map` at real differences. A new revision is a table edit. `requiredMode != UNKNOWN` on a part with `hasOperatingMode=false` is INVALID_CONFIG. No per-part files. Split `validateLinkSettings`: enum, mask and cross-field only. Only the robust check moves to the part-keyed preflight; the validator holds no VOD rule, and the D6 VOD write is keyed to the DP83826 and DP83826A maps. Change `test/foundation.cpp`:121 to `(flags & 30U) == 0U`, and make CFG:97, AC:187 and AC:250 part-specific.

  Rows: DP83826 models {0x11,0x13}, revisionMask 0x0002, interfaceMask MII|RMII, strapAddressMax 7, hasOperatingMode true, forcedRequiresRobust true. DP83826A: same except revisionMask 0x0010. DP83825: {0x14}, 0x0001, RMII, 3, false, false. DP83822: {0x24}, 0x0001, MII|RMII|RGMII, 31, false, false. 825 adds `0x000C, 0x0101, 0x010A, 0x0123, 0x0130, 0x0172, 0x0174, 0x030F, 0x0311, 0x0313, 0x031C, 0x031F, 0x033C, 0x033E, 0x0416, 0x0428, 0x0429, 0x04CD-0x04D2, 0x04D4-0x04D7`, plus table headings `1000h, 1001h, 1014h, 1016h, 203Ch, 203Dh`, whose wire form is governed by D2.

#### Prompts 56-59: DP83826E and AI/AE

- **G8-1** S2 [A] (with **G9-19**) P56:35-41 repeats a complete audit for a part with no documented software-visible difference; E and I differ only in temperature and TA-keyed leakage rows, and AE/AI are alike. **Fix:** "One document-level cited record: one register map and procedure set with no suffix-conditional content. Add `Variant::DP83826E` sharing I's profile. Reopen only if a new TI revision adds a suffix
  condition. Parameterize one identity and bind test." Add the exception to FC:170 and use it for AI/AE. Shrink `docs/variants.md`:77 to "temperature only; no suffix detection from shared ID".
- **G8-3** S2 [L] P59:23-37, P59:46-47 and P57:26-35 say "shared procedures" but omit field moves. Reusing P32's masks makes LED1 writes land on A's LED2 and LED3 writes on reserved. **Fix:** seed P57 with these. LEDCFG (8-86, reset `5565h`): LED3 [11:8], LED2 [7:4], LED1 [3:0], [15:12] reserved and preserved. LEDCFG2 (8-90): bits 10/9/8 are LED3 polarity, drive value and enable where I/E reserves them. LED mux: `0x0303[2:0]` and `0x0306[2:0]` are R on I/E, R/W on A; `0x0304[2:0]` is RH/W with strap default, header reset `000Xh`; `0x0305[2:0]` is RH/W, "default determined by STRAP". IO_CFG1 (8-74): bit 6 `cfg_clkout25m_off` is reserved R. PHYCR[5] (8-44): LED Configuration on I/E, reserved on A. SOR1 (8-88): bit 11 Strap11, pin 19. FLD_CFG2 (8-54): reset `0x2284` to `0x0288`, [15:6] `8Ah` to `Ah`, threshold [5:0] `4h` to `8h`. CR1[5] and CR3[10]: RH/W X, BASIC 0, ENHANCED by strap. PHYSCR[1:0]: RH/W X, ENHANCED 0, BASIC 1, reset `010Xh`. RXFCFG1 (8-91): reset `108Xh` to `10X1h`; bit 7 RH/W X, strap in BASIC, 1 in ENHANCED; bit 0 table reset 1h, "default from strap". FLDS (8-34): RC to RCH, same read-clear handling. FLD: tables 8-5 and 8-6 add Strap11, and BASIC mechanisms depend on it; 8.3.17 Note requires MLT3, low SNR and descrambler to be enabled with energy loss or RX error. 8.3.3.1: A has 27 rows versus 18; 15 identical in value; drops `04D3=4F12`; changes `0416` `1F30` to `1830` and `0466` `FE00` to `FD00`; adds `0126=5D23, 0132=0183, 04D4=3322, 0310=76E1, 0311=01FC, 0313=06E3, 033A=579B, 033C=EC00, 0317=1000` and a second `04F5=2864`; `0466` moves to last, after `0000=3300`. Assign LEDCFG, LEDCFG2, LED mux, IO_CFG1, PHYCR[5], RXFCFG1 and EEE to 59; identity, CR1, CR3, PHYSCR, FLD_CFG2, FLDS, FLD note and SOR1 to 58. Add literal tests.
- **G8-9** S3 [A] P57:59-60, `docs/variants.md`:67-69 and FAM:55: Strap11 wording omits "if FLD is used"; SDAA067A table 3-3 option 5 says Strap8 HIGH while its Figure 3-2 and A table 8-5 say LOW; literal `0x000B=0x0001` clears pair and polarity swaps. **Fix:** reword; record the Strap8 conflict (datasheet governs); "Software writes only mask `0x040F`, preserving bits 6:5. Report Strap11 from SOR1 bit 11. No EMC claim."

#### Prompts 60-61: DP83825I

- **G8-4** S2 [A] P61:25-27 and FAM:79-84: `0x000C` (Register_12, status RC) is unnamed, while table 7-21 marks MISR1 status "R" and table 7-19 says reading clears. **Fix:** "Treat `0x0012`, `0x0013` and `0x000C` as read-consuming, owned only by the collector and excluded from `readable()`. Record the conflict and unknown latch relation. Collect through MISR1/2 only. Test consumed-first-sample then failed second read. Do not trust 825's `R` type where prose describes latching or read-clear: treat FLDS `0x000F` as read-consuming and collector-owned, excluded from `readable()`, and record the conflict with 826 and A."
- **G8-5** S2 [I] P60:30-31, :37-38, :63 and FAM:110 frame RCSR[7] as selectable, but the strap sets the role. Table 7-26 reset 0h conflicts with SDAA081, which reads it as strap confirmation. **Fix:** "RMII role is a board declaration in a new Config field, required on DP83825I (LEADER or FOLLOWER). Observe SOR1[3] directly and RCSR[7] only through the collector, because reading `0x0017` consumes RC bits 3:2; retain those samples. Disagreement is a reported mismatch with no write. Ledger disposition for R/W RCSR[7]: not written, because table 7-26 says the strap determines the reference and SDAA081 2.4 reads it as confirmation. Zephyr's write is observed only. No independent clock selector. Do not assert bit 7 reset equal to 0h; vectors are strap-parameterized."
- **G8-6** S2 [I] P61:36-38 and P60:58-59: in Leader mode (the strap default) the PHY sources the MAC's clock, and generic LED/GPIO setters, isolate or deep power-down can remove it. **Fix:** "With declared role LEADER (G8-5), reject nonzero `0x0305[2:0]`, BMCR[10]=1 and DPDWN `0x0428[2]`=1 before callbacks unless the application passes an explicit clock-loss opt-in. A later SOR1[3] mismatch with the declaration is a reported mismatch that blocks these three writes. Isolate is sourced from table 7-3; `0x0305` and deep power-down are inferred (table 7-61, 6.3.8). Add zero-callback tests and Q15."
- **G8-7** S3 [L] P61:30, :36-38, `docs/variants.md`:79 and P60:69-71: 825 tables 6-5, 6-6 and 6-7 define whole-word modes; `0x04D1` bypass polarity differs from 826. **Fix:** "Functional modes as one finite typed mode procedure; `0x030E=0x4A40` never runs on 825. `0x04D1`: use 6.3.3.2, 6.3.4 and table 6-7 (`0x00C2`, bits 0 and 3 clear), and record the conflict with table 7-92 (bit 0 type R, bit 3 description reversed). `0x030E`, `0x0333`, `0x0218`, `0x031B`, `0x033A` and `0x033D` are exact-address allowlist entries with a source note citing tables 6-5 to 6-7, 8-3 and SDAA081 2.3.3, against table 7-1's reserved statement."
- **G8-8** S3 [A] SNLS638C 6.3.5 and tables 7-83, 7-84 document Secure-ON, but `0x04A5..7` are absent from table 7-1, which calls unlisted addresses reserved. **Fix:** "Named source gap; SNLA261A table 2-1 and 2.2.2 name these addresses for DP8382x and table 2-1 lists DP83825. Implement as exact-address allowlist entries under that cited record, with physical verification required (Q15)."
- **G8-12** S3 [L] CFG:251-252 and P60:45-47: 825 T4 MDC is 20 MHz max; its prose says 24 MHz. **Fix:** "Record the conflict; document 20 MHz as the conservative DP83825I limit."
- **G8-10** S3 [A] Merged into G8-2 and D5. 825 table 7-12 has no robust requirement.
- **G8-11** S3 [A] Merged into G4-5.

#### Prompts 62-67: DP83822

- **G9-1** S2 [L] P65:31-32 and FAM:130: table 7-10: "FX_EN strap must be enabled for SD_EN strap to be functional." **Fix:** "SD is strap-only. `sdActive = SOR1[11:10] in {01,10} AND SOR1[9:8] in {10,11}`. CR2[14] alone never activates SD, and RX_ER keeps its AMDIX_EN meaning unless the FX_EN strap is set. FIBER GENCFG `0x0465[0]` sets polarity only (1=active low). Report FX source and SD state separately. Without active SD, electrical-link loss recovers only by soft reset (7.4.4.2)."
- **G9-2** S2 [L] P65:39-40 and :63-64: SNLA437A 2.3.4, naming DP83822IF/HF: "In Fiber, this bit does NOT toggle when the link status changes. Soft-Reset must be performed (Set Register 0x001F = 0x4000) before reading". **Fix:** "In FX, BMSR[2] is not change-driven. Apply the same rule to PHYSTS[0] as an inference from table 8-16. Mark the value as not change-tracked. Table 8-2 has no fiber exception and table 8-83 says SD causes link drop: record the gap. Provide an explicit maintenance-admitted refresh: `0x001F=0x4000` (digital restart, disturbs the link), then an application-bounded wait (no duration is documented), then two BMSR reads through the collector, using the second and keeping the first as evidence. Never issue it from polling, snapshots or the SDK timer. Fakes model a non-toggling BMSR and a latched-low first read." API shape is a suggestion; Q14.
- **G9-3** S2 [L] FAM:131, P62:32 and P63:29-30 record only a label dispute. Real traps: BMCR says "Straps are re-latched only during power up and pin RESET_N", while table 8-30, 7.5.1 and SNLA437A 2.2.6 add PHYRCR[15]; TI prose says "software reset" and means `0x4000`, while table 8-30 names bit 15 "Software Reset" and table 8-1 names BMCR[15] "PHY Software Reset". **Fix:** five reset mechanisms, with recipes using literal words, never the words "software reset". RESET_N/POR: straps latched, all reset. `0x001F[15]`: pin-equivalent, treat straps as re-latched, record the conflict with table 8-1. `0x001F[14]`: digital restart, registers retained; this is what 7.3.3 and SNLA437A call software reset, and 7.4.4.2's "soft reset" is taken to be the same (INFERRED). BMCR[15]: PCS registers, vendor registers kept, no strap latch; the PCS set in `0x0000..0x001F` is not listed, so re-read owned fields there. `0x3000[15]`: MMD3, MMD7 and PCS, vendor registers kept.
- **G9-4** S2 [L] FAM:138 drops a write and P67:32 mislabels a dependency. 822 7.3.3: "Program (Register 0x0456 = value 0x000A)", "Program (Register 0x04A0 = value 0x1080)", WoL "is not a must change", then `0x4000`. **Fix:** "After IOCTRLx selects RX/TX SFD: write the literal word `0x0456=0x000A` (bit 1 is reserved R/W in table 8-78, so allowlist this value; bit 3 Min IPG stays 1; reject before I/O if an owner holds bit 3=0). Optionally set RXFCFG bit 7, composed with owned WoL fields, bit 12 preserved and bit 11=0; this auto-sets RXFS[12], so MISR2 bit 9 then means WoL (table 8-88). Then `0x001F=0x4000`." P67: "optional WoL-enable accuracy aid". Assign PTPPSEL `0x003E` and PTPCFG `0x003F` to P67.
- **G9-5** S2 [A] Merged into G8-2 and D5. SNLS505H does not apply crossover to FX: 7.4.7 omits it and the AMDIX_EN strap becomes SD_EN under FX_EN (7.4.4.2). Silicon behaviour of those bits in FX is not stated. The FX path writes no CR1[6:5], PHYCR[15:14] or CR3[6:5]. Add a DP83822 revision rule from table 8-4; `DeviceIdentity.mode` stays UNKNOWN there.
- **G9-6** S2 [L] P64:30-35 "source-proved order" does not exist. `0x4300` means RX_D3 = 50 MHz output and LED_1 tri-state, per table 8-81. **Fix:** "Issue the literal `0x0462=0x4300`, then `0x001F=0x4000`, only on a software RCSR[5] 0-to-1 transition. Strap-selected RMII runs no recipe (7.4.1.2 note). Reject before I/O if an owner holds `0x0462[14:12]` or `[10:8]` with other values. Read `0x0462` before the RCSR write. If [6:4] or [2:0] is non-zero, owned or strap-loaded, reapply those bits after the `0x001F` write by masked write, labelled INFERRED, physical NOT RUN. Record OPEN: the word selects the Leader reference even in Follower mode. On a software change away from RMII (RCSR[5] 1-to-0, or RCSR[9] 0-to-1), return `0x0462[10:8]` to 000 by masked write, then `0x001F=0x4000`, labelled INFERRED, physical NOT RUN, because table 5-1 requires RX_D3 as data in MII and RGMII and no reverse recipe is documented."
- **N-6** S3 [L] 822 7.4.2.1 says REGCR function "(10) is not a valid option", while table 8-13 says "10 = Data, post increment on read and write" and 7.4.2.6/7.4.2.7 use `0x801F`. P62:30-31 calls 7.4.2.1 an exact fact to preserve. **Fix:** add to P62: "Record source conflict `regcr_function_10`. Use no-post-increment `01` only; do not admit `10` without a physical check."
- **G9-7** S2 [L] Addressing part merged into G4-1. **Fix for 822:** add to P62 or P67: "Printed `0xDnnn` on 822 maps to DEVAD=D, offset=`0x0nnn` (7.4.2.9). Admit DEVAD 3 and 7 with the part-keyed policy. Literal vector: `0x0007, 0x003D, 0x4007`, read. The DP83826 printed scheme differs: do not share constants."
- **G9-8** S2 [A] Merged into G4-5. Also: "Acknowledge on 822 = write owned config `| 0x0800` (table 2-4 step 6); all other writes keep bit 11=0."
- **G9-9** S3 [L] Merged into G2-12. **Corrected by the re-audit:** RXFS[12] resets to 0 on DP83822 (table 8-88), not 1 as on DP83826, so the 826 rule does not carry over unchanged.
- **G9-10** S3 [A] FAM:100-101 sends the coder to table 8-87 for bit 6, whose whole text is "Bit Mask Flag". **Fix:** "No documented semantics. Preserve it, provide no typed setter, record as OPEN."
- **G9-11** S3 [A] FAM:135-136 and P66:26-28: amplitudes are 7 bits (bit 15 and bit 7 reserved), while SNLA253 says 8. **Fix:** "Mask `0x7F` per tables 8-60..8-64; locations are 8-bit; peak map per tables 8-55..8-59."
- **G9-12** S3 [A] P66:26: CDCR Done resets to 1, so polling Done alone completes immediately. **Fix:** "Manual completion = bit 15 read back clear AND bit 1=1; bit 0 = fail. Test a fake with Done=1 before start."
- **G9-13** S3 [A] FAM:114 and P64:40 omit `0x0462[6:4]` GPIO1 clock output. **Fix:** add it (active with [2:0]=011). Treat it as mutually exclusive with FX SD on shared pin 24 (INFERRED).
- **G9-14** S3 [A] P65:27-28 and FAM:130 omit `0x0040` Fiber Far-End Fault. **Fix:** add bits 13, 6 and 5 with preserve-reserved RMW, plus LDCTRL[11:8] FX swing from P66.
- **G9-15** S3 [A] P62:57-58 and P64:56: "no arbitrary tap range" could hide TXCPSR `0x0042`. **Fix:** scope to "RGMII RCSR[12:11]"; add TXCPSR as an MII TX_CLK phase control owned by 64; record the [4:0] versus [3:0] wording conflict.
- **G9-16** S3 [A] P65:27 and FAM:130: "GENCFG 0x0465" collides with real GENCFG `0x0456`. **Fix:** use `FIBER_GENCFG` and `GENCFG` everywhere, with literal tests.
- **G9-17** S3 [A] `docs/variants.md`:63-64 and FAM:130 cite 7.4.4/7.4.6 for MAC independence, which say nothing about it; P62:58, P64:64 and P65:45 assert it uncited. **Fix:** cite section 4 for FX parts and 7.4.1.1/7.4.1.2 for MII and RMII. Record FX+RGMII as "not stated; no documented exclusion; table 7-10 RX_ER mode 3 can encode it; physical NOT RUN".
- **G9-18** S3 [A] P65:15, :20 and :39 require medium selection but name no shape, and no earlier stage creates one; `Capabilities.linkModeMask` uses the same LINK_100_* bits for copper and FX. **Fix:** outputs of P65: `Config` medium enum (COPPER, FIBER), per-medium link mask, observed medium from CR2[14] with SOR1 evidence, CLI grammar, adapter admission.
- **G9-19** S3 [A] Merged into G8-1. 822 subtype deltas are exactly fiber capability and temperature: "one DP83822 ledger with `parts: [I,H,IF,HF]`, two attributes per Variant, per-part rows only for admission and physical results."

### 7.5 WP7: prompts 68-80, packaging, CI, review

- **G10-1** S2 [L] This host has no `idf.py`, `IDF_PATH` or `framework-espidf`. PlatformIO's Espressif packages (riscv32 GCC 14.2 and 13.2) do not match v6.1's `esp-15.2.0` and are not a substitute. No prompt provisions the SDK, and remote CI is banned for 1-70, so SDK builds cannot defer. **Fix:** add to P37: "If no pinned ESP-IDF is installed, clone tag v6.1 (commit `fff9895c`) outside the repo with submodules, run its `install.ps1 esp32p4`, then `export.ps1` in each build shell, and record path, commit and tool versions. Later prompts reuse it. If installation is impossible, SDK layers are BLOCKED; no header shim."
- **G10-2** S3 [L] P71:46-52 and :62: no `.github` exists on any branch. GitHub documents `workflow_dispatch` as requiring the file on the default branch, and `--ref` only selects the file version, so do not rely on `gh workflow run` before a merge. There are no triggers to "preserve" yet. **Fix:** "`ci.yml` triggers on `push` (all branches), `pull_request` and `workflow_dispatch`. Get the run via `gh run list --commit <sha>`. Do not merge to `main` to enable dispatch. The preserve rule applies only to what 71 creates." Same in `AGENTS.md`:58, COMMON, VER:139-140, prompts README:40 and P77:48-49.
- **G10-3** S3 [L] (D12) `scripts/generate_version.py`:49 and `test_version_tool.py`:113 reject any `library.json` field beyond five, and P10:14 suggests adding some. **Fix:** in P69: "`library.json` keeps exactly its five fields and is version truth only. SDK constraints and dependencies go in `idf_component.yml`, whose version field `sync` generates. Supported parts and interfaces go in `docs/packaging.md`."
- **G10-4** S3 [L] (D12) P70:13-17 and :54-56 and P77:30-31 keep a conditional PlatformIO path with no consumer. **Fix:** "PlatformIO is not a supported consumer in this execution. Do not adopt SCD41's `platformio_configuration`. Create `scripts/pio.cmd` only if a prompt actually runs PlatformIO." Remove the sentence from P77. Keep the conditional rule in `AGENTS.md`.
- **G10-5** S2 [L] `CMakeLists.txt` has no install, no `project(VERSION)` and no IDF branch. P69:40-48 "keep IDF component consumption separate" is ambiguous. **Fix:** "`if(COMMAND idf_component_register)` comes first and selects the IDF component including `src/esp_idf/`. `project()` and every portable rule live in the `else()`. The portable branch adds `$<INSTALL_INTERFACE:include>`, `install(TARGETS ... EXPORT)`, `install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN \"*.h\" PATTERN \"esp_idf\" EXCLUDE)`, and `DP8382xConfig.cmake` via `CMakePackageConfigHelpers`, namespace `DP8382x::`. `sync` writes `project(VERSION)` from `library.json` and `check` verifies it. Do not use `FILE_SET` (needs CMake 3.23; minimum is 3.16)." A scratch build of this form built, installed, relocated and ran a `find_package` consumer under CMake 4.0.1; no real CMake 3.16 run.
- **G10-6** S3 [A] P69:30-36 and :60-67 re-require what prompt 10 ships and tests. **Fix:** "Prompt 10 covers these. 69 adds only new outputs and their fixtures, plus one test for unsupported CLI arguments. Missing-Git identity stays with P70:53-54."
- **G10-7** S3 [A] Files named but not created: `docs/validation.md`, `CONTRIBUTING.md`, "fixture manifest", `ciTestedCommit`. **Fix:** delete "validation" only as a document name at P72:69, P73:68, P74:54, P75:57, P76:56, P77:58, P78:71, P79:74 and P80:66. Keep P72:41, P74:49, P75:6 and P75:45. VER:15 becomes "Keep these separate in each stage report." "Create CONTRIBUTING.md" in P69; P72 "extend the evidence record and fixture selection from 48-52" (G7-4 removes the manifest); reword `ciTestedCommit` to "the tested SHA".
- **G10-9** S3 [L] P68:55-56 and P55:57-58: only MinGW g++ exists. ASan, LSan, TSan and non-trap UBSan do not link. **Fix:** "Run `-fsanitize=undefined -fsanitize-trap=undefined` locally. Record the second compiler and other sanitizers as NOT RUN locally; they run in the Linux job at 71, including the tests-OFF consumer. Do not install toolchains for this."
- **G10-10** S3 [A] P68:24-45 puts assertions in an example. **Fix:** keep `examples/portable/basic/` as the two-instance owner application VER:88-89 requires, reusing `DirectRead.h` and `MmdAccess.h`, and exiting nonzero on failure. Keep `test/consumer` as the link check. Put independence, zero-frame and failure assertions in registered CTest suites.
- **G10-11** S2 [A] (D13) P36, P55, P75, P78 and P80 each reconcile every register and field; P79 repeats P53-P55. **Fix:** "COV holds the one matrix (G7-6). 59, 61 and 67 append their exact-part rows. 36 reviews critical procedures and representative implementations, with no row reconciliation. 75 and 78 verify by sampling and independent re-derivation of high-risk rows only. 80 checks only rows whose inputs changed since last verified. 79 consumes 53-54 results and reviews only the delta. 76 is limited to exported-contract and Doxygen review plus checker adverse tests. Record commands and counts in the stage report only, with a progress-row link." P55:76: replace "test/README.md with the coverage matrix" with a link to COV. Sibling re-audit at each stage is unchanged: it is a recorded owner requirement (AGENTS:11-13, COMMON:20).
- **G10-12** S3 [A] P72:65-68, P73:65-66 and P80:7-8, :67-68: the progress doc has no legend or status for an open physical gate, and P72 says "DONE". **Fix:** add a legend: "72/73: BLOCKED (software and readiness done, physical NOT RUN). 80: COMPLETE when its report and checks exist (COMMON:108), while the execution record and final handoff state the set is PARTIAL." Change P72's "DONE" to "COMPLETE".
- **G10-13** S3 [A] `.gitignore` misses `sdkconfig.<target>`, `dependencies.lock` and `*.log`; repo-wide `*.bin` would hide a fixture. **Fix:** add `sdkconfig.*`, `!sdkconfig.defaults*`, `dependencies.lock`, `*.log`, and `!test/fixtures/**` if binary fixtures are planned. `sdkconfig.*` also hides a future committed `sdkconfig.ci`; revisit the lock-file ignore if an example becomes registry-only, since Espressif recommends committing it for those.
- **G10-14** S3 [L] P70:23-24 and :45 name no archive emitter, and 71 is the first time Linux compilers see the code. **Fix:** P69: "`git archive --format=tar.gz` of HEAD with `.gitattributes` lines of the form `test export-ignore` (not `test/**`, which leaves an empty directory member) for `docs/reference/source`, `docs/reference/text`, `docs/reports`, `docs/prompts`, `docs/library-comparison.md`, `docs/implementation-progress.md`, `test`, `tools`. `tools/check_package.py` validates exactly this and rejects any local user path in the payload." `docs/library-comparison.md`:308-309 holds a local user path. P71 notes that first-run Linux failures are expected and fixed there.
## 8. Physical qualification cases to add

These extend prompts 72-73. They need identified authorized fixtures; none has run. Each protects a decision above.

**No physical board is expected for a long time (owner, 2026-11-02).** No software stage waits for a case below. Implement prompts 13-71 and 74-80 from the decisions in section 3, keep each case NOT RUN, and keep its claim marked source-implied or unmeasured. At 72-73 finish the readiness work and case inventory, leave the physical gate BLOCKED/NOT RUN (P72:65-68, P73:65-66) and continue. The set then ends PARTIAL, not COMPLETE (P80:40-41, G10-12). A missing board does not excuse the other layers: SDK link checks still run (G10-1), and so do host-driver tests (G6-2). When a board arrives, re-check its netlist, its pins and the pinned SDK revision before admitting it, because these records may be old by then.

| Case | What to observe | Protects |
| --- | --- | --- |
| Q1 | EEE wire probe, after hard reset, no partner; log every raw word. Read `(3,0x0014)`, `(3,0x1014)`, `(7,0x003C)`, `(7,0x203C)`. Write `(7,0x003C)=0x0002` and read both. Only if the low readback is not `0x0002`, write `(7,0x203C)=0x0002` and read both. Never read 3.1 or 3.22 here. Decide: `(3,0x0014)=0x0002` and low readback `0x0002`: low works. Low fails both and full works: change the constant table. One low check passes and the other fails: write `(31,0x04D1)=0x008B`, repeat the low reads (822 table 8-133 says these are "ignored" while EEE is disabled), record, stay blocked. Neither: blocked, ask TI. Hard reset last. This closes the gate for `(3,0x0014)` and `(7,0x003C)` only. At 73 against an EEE peer: `(7,0x003D)` bit 1 = 1, the peer reports this PHY advertising EEE, and `(3,0x0001)` bit 10 sets through the collector while the peer sends LPI. `(3,0x0016)` stays source-implied unless a wake fault is produced. | D2, G4-1, N-1 |
| Q2 | LPI on the line or partner after `0x108B` | G4-8 |
| Q3 | RXFCFG1: level mode, valid Magic Packet with asymmetric MAC `11:22:33:44:55:66`, write neutral `0x1981` (level holds?), write `0x1181` (level clears?). If the neutral write cleared the level, send a second Magic Packet before testing `0x1181`. Record RXFCFG1 bit 0 across resets | D16, G4-3, G4-5, G4-6 |
| Q4 | `modifyMmd` on LEDCR, then a direct `readClause22(0x18)` comparison | D16, G12-8 |
| Q5 | PHYRCR hard reset: first valid read time, and strap re-latch against an identified strap change | D14, G1-3, G1-10 |
| Q6 | BMCR[11] after pin release in ENHANCED mode | G4-4 |
| Q7 | Both TDR recipes against known cables, open and short, with measured distance | D3, G3-1, G3-2 |
| Q8 | Reverse and analog loopback with and without restart | G3-3 |
| Q9 | INT pin level versus pulse, MISR latching while its enable is 0, and RXFS[12]=0 retention with RXFCFG1[7]=1 | G3-4, G2-12 |
| Q10 | Next-page exchange and re-AN against an NP-able partner | D8, G2-3 |
| Q11 | Raw PHYSTS in software-forced modes; BMCR[13] and BMCR[8] against the actual line mode with AN strapped off, in BASIC and ENHANCED; CR1[5] with manual MDI; CR3 `0x0060` with PHYCR automatic | D4, D5, G2-1, G2-10 |
| Q12 | Whether LED0 source or polarity depends on PHYCR[5] | G5-7 |
| Q13 | On P4: stop under link flapping with fence before `mac->stop` (MAC stays stopped), `mac->stop` re-call on stopped and never-started MAC, netif STOP delivered before glue deletion, stub mediator, RX teardown under load | D1, G6-1, G6-8 |
| Q14 | DP83822 FX: BMSR on link loss and link return, with SD active and SD inactive; `0x4000` refresh | G9-2 |
| Q15 | DP83825I: `0x000C` latch relation, identity A140 or A150, `0x0305` clock loss in Leader, Secure-ON at `0x04A5..7` | G8-4, G8-6, G8-8 |
| Q16 | RECR with controlled invalid-symbol injection at 7F/80, FF/100 and 7FFF/8000, with read-clear behaviour | G2-13 |
| Q17 | RCSR[3:2] with a controlled RMII clock offset: polarity, read-clear rearm, value after reset | G2-13 |
| Q18 | CR3 low nibble readback after POR in BASIC and ENHANCED for identified strap settings | G13-6 |
| Q19 | Compliance stimulus with CR1[5]=1, which TI's scripts omit | G5-6, D5 |

## 9. Coverage and limits

**Count: 172 findings from 13 scoped auditors, one lead finding (M-1) and six added by the re-audit (N-1 to N-6, with N-5 folded into the section 5 table). Five distinct S1s remain once G11-1 is merged into G6-1; N-1 is S1 but completes G4-1's fix.** Duplicates are merged above, and every id appears in this document. Marks in catalogue entries: 80 [L], 67 [A], 14 [I]. All S1s are [L]. Section 11 gives every finding's re-audit verdict.

**Read completely and compared with primary sources:** prompts 13-80, the shared contracts, AC, DD, ARCH, the ledger, the catalogues and all shipped code. All 80 prompts have working links and anchors; prerequisite references never point forward, though some omit stages (G5-11). 21 prompts use a "Required inputs" sentence instead of a prerequisite line, which is a format difference, not a defect.

**Checked and correct, so do not re-audit without cause:**

- 826 ledger data as stated in section 1. All literal vectors in RV were recomputed by the first audit, apart from G13-7's labelling; the re-audit sampled RV and found no wrong value.
- Identity: I/E `0x2000A111`/`0x2000A131`, revision 1; A model 0x11/0x13, revision 4; 825 `0x2000A140`, revision 0; 822 `0x2000A240`, revision 0. Mode is set by pin, not software.
- Timing: 25 us reset pulse without debounce caps, 2 ms and 50 ms MAX, FLP 1500 ms and EEE values NOM only, FLD 10/11 us MAX, MDC 2.5 MHz NOM and 24 MHz MAX on 826.
- The 18-pair EEE recipe, bypass polarity, REGCR words `0x001F`/`0x401F`, LEDCR mask `0x0692`, the shipped `readable()` classification, and MLEDCR selector mask `0x0078` (owned mask `0x0279`).
- WoL masks, MAC and password packing, BISCR and BIST fields, CDCR masks, MISR pairs, FLD fields, VOD codes, RMII and RCSR fields.
- 822 RGMII: bit 12 1=RX on, bit 11 0=TX on, both 3.5 ns; vectors `0x0800/0x1800/0x0000/0x1000`.
- SDK claims in SDK: flow-control ioctl width, no full-advert ioctl, raw ioctl bypass, `auto_nego_en` cache, peer pause Boolean, repeated UP resets DMA descriptors.
- All 17 shipped enums match AC, and every AC code block type-checks with g++ 15.
- Sibling paths and symbols named by the prompts exist. Prompt 25's exclusion of SNLA330A and SNLA253 stays correct; they are other parts.

**Not checked:**

- Physical behaviour. No PHY or P4 was used, and no case in section 8 has run.
- IEEE 802.3 is not archived in the repository. The re-audit confirmed the next-page behaviour from the IEEE 802.3-2012 mirror copy recorded in `docs/reports/implementation/3-source-audit.md`, by SHA-256; other IEEE claims rest on knowledge and on TI tables that mirror them.
- ESP-IDF was reviewed statically: no build or run, no QEMU, no install on this host. G6-1's sequence is source-derived.
- ASan, LSan and TSan were unavailable on this toolchain; UBSan ran in trap mode only.
- The re-audit required original pages for silicon claims; section 11 lists the known exceptions. The lead personally re-read only the [L] items.
- TI E2E threads 924830 and 792015 returned HTTP 403; TI errata were not searched again. A full 825 versus 826 field comparison belongs to prompt 60.
- Behaviour of sibling code beyond existence of the named symbols.

## 10. Per-prompt index

Before executing a prompt, apply its findings if WP3 to WP7 have not already done so. This table combines direct prompt references in the catalogue with fixes that edit a prompt indirectly. Cross-cutting items apply to every prompt: G11-17, G13-3, the COMMON cleanup glossary, and the ledger tables in section 5. "Cross-cutting only" means no finding targets that prompt's text; it does not mean the prompt was skipped.

| Prompt | Findings |
| --- | --- |
| 13 | G1-1, G1-4, G1-5 |
| 14 | G1-6 |
| 15 | G1-2, G1-8, G1-9, G1-11, G1-13, G1-14, G11-2, G11-20, G11-22, OP rules |
| 16 | G1-3, G1-7, G1-10, G1-11, G1-13, G11-22, OP rules |
| 17 | G1-1, G1-2, G1-4, G2-2, G2-7, G2-8, N-3 |
| 18 | G1-2, G2-4, G2-5, G2-6, G2-10, G2-16 |
| 19 | G1-1, G2-1, G2-5, G2-11, G11-22 |
| 20 | G2-12 |
| 21 | G11-22 |
| 22 | G1-2, G2-15, G2-16, OP rules |
| 23 | G2-16, G3-3, G13-3, OP rules |
| 24 | G3-3, G11-22, OP rules |
| 25 | G1-9, G3-1, G3-2, G3-5, G3-9, OP rules |
| 26 | G3-5, G3-6 |
| 27 | G3-4, G3-8 |
| 28 | G4-4, G4-7, OP rules |
| 29 | G4-1, N-1 |
| 30 | G4-2, G4-8, N-1, OP rules |
| 31 | G4-3, G4-5, G4-6 |
| 32 | G5-7, G5-10, G8-3 |
| 33 | G5-11, G13-3 |
| 34 | G5-2, G5-4, G5-5, G5-11, G13-3 |
| 35 | G5-1, G5-6, G5-8, G5-9, G5-11, OP rules |
| 36 | G2-1, G4-1, G5-11, G10-11 |
| 37 | G6-1, G6-2, G6-4, G6-5, G6-9, G10-1 |
| 38 | G6-1, G6-2, G6-5, G6-11 |
| 39 | G6-9, G6-10, G6-12 |
| 40 | G6-2, G6-7, N-2, N-4 |
| 41 | M-1 |
| 42 | G6-2, G6-6, G6-8 |
| 43 | M-1, G7-2, G7-3, G7-5, G7-7, G7-11, G7-12, G11-20 |
| 44 | G7-1, G7-6, G7-8 |
| 45 | G7-8, OP rules |
| 46 | G7-6, G7-8 |
| 47 | G7-2, G7-5, G7-6, G7-14 |
| 48 | G7-3, G7-4, G7-5, G7-9, G7-12 |
| 49 | G7-3, G7-4, G7-9, G7-14 |
| 50 | G7-5, G7-10, G7-14 |
| 51 | G7-5, G7-10 |
| 52 | G7-4, G7-10 |
| 53 | G7-6, G7-13, G10-11 |
| 54 | G7-3, G7-4, G7-13, G11-22 |
| 55 | G1-5, G5-11, G7-6, G10-9, G10-11 |
| 56 | G8-1, G8-2 |
| 57 | G8-3, G8-9 |
| 58 | G8-2 |
| 59 | G4-5, G8-3 |
| 60 | G8-2, G8-5, G8-6, G8-7, G8-12 |
| 61 | G8-4, G8-6, G8-7 |
| 62 | G9-3, G9-7, G9-15, G9-17, G9-19, N-6 |
| 63 | G9-3, G9-5, G9-9 |
| 64 | G9-6, G9-13, G9-15, G9-17 |
| 65 | G9-1, G9-2, G9-5, G9-14, G9-16, G9-17, G9-18 |
| 66 | G1-5, G9-11, G9-12, G9-14 |
| 67 | G9-4, G9-7, G9-8, G9-9, G9-10 |
| 68 | M-1, G10-9, G10-10 |
| 69 | G10-3, G10-5, G10-6, G10-7, G10-14 |
| 70 | G10-4, G10-6, G10-14 |
| 71 | G10-2, G10-14 |
| 72 | G7-4, G10-7, G10-12 |
| 73 | G7-4, G7-14, G10-7, G10-12 |
| 74 | G10-7 |
| 75 | G10-7, G10-11 |
| 76 | G10-7, G10-11 |
| 77 | G10-2, G10-4, G10-7 |
| 78 | G1-5, G10-7, G10-11 |
| 79 | G10-7, G10-11, OP rules |
| 80 | G10-7, G10-11, G10-12 |

## 11. Re-audit record

On 2026-11-02 thirteen fresh verifiers re-checked this report at commit `7da4b9f`, treating every finding as a claim to falsify: location, evidence, defect, proposed fix and severity. Silicon claims had to be checked on original PDF pages, not text extracts. The lead then re-read the most consequential corrections at source: board pins, DP83822 RXFS[12], the TDR done and sign rules, gate `eee_control_semantics` and the stop race. All corrections are merged above, so the text in sections 3 to 8 is the corrected version.

**Result: of 172 findings, 65 held as written, 101 were real but needed a correction to their evidence or fix, 5 were overstated, 1 was wrong in part, and none was wrong outright.** A few were judged differently by two verifiers; the stricter verdict is recorded. "Corrected" does not mean the defect was false: it means something in the entry would have misled the coder. The ones that would have caused real damage had the coder followed them:

| Finding | What the earlier text would have done |
| --- | --- |
| G6-7, N-2 | Swapped TX_EN and MDC and put MDIO on the wrong pin: the board was re-pinned on 2026-10-27, and the repo's own board doc is stale |
| G6-1 | Called `mac->stop` before the timer fence, so an in-flight link callback could restart the MAC after stop |
| G4-1, N-1 | Left EEE blocked by a second gate; a bit-1 self-check would also have accepted `0xFFFF` |
| G3-1, G3-2 | Treated the previous run's Done bit as fresh; a bit-11 sign test contradicted TI's own text and table 8-71 |
| G9-9 | Applied DP83826's RXFS[12] reset meaning to DP83822, where it is the opposite |
| OP-6, OP-9 | Admitted budgets too small on coarse clocks; blocked `end` after abandon, which deadlocked the broken-clock route |
| G5-1 | Proposed a one-id action family that fails `tools/verify_ledger.py`, and dropped the gate id it requires |
| G7-8 | Routed discovery around the core, bypassing the hard-reset floor and report validation |
| G10-14 | Would have shipped a local user path, and `test/**` patterns would have left empty directory members the package check rejects |
| G13-1 | Listed 7 registers with risky reserved bits; 14 are affected |

**Per-finding verdicts:**

- **Confirmed as written (65):** G1-9 G1-14 G2-2 G2-6 G2-8 G2-9 G2-10 G2-11 G2-15 G3-4 G3-6 G3-8 G4-4 G4-5 G4-6 G4-7 G4-8 G5-3 G5-4 G5-5 G5-6 G5-9 G6-5 G6-6 G6-9 G6-13 G7-10 G7-11 G8-1 G8-4 G8-9 G8-10 G8-11 G8-12 G9-1 G9-7 G9-8 G9-10 G9-11 G9-12 G9-13 G9-14 G9-15 G9-16 G9-19 G10-8 G10-13 G11-1 G11-3 G11-5 G11-6 G11-8 G11-11 G11-12 G11-13 G11-15 G11-16 G11-21 G12-3 G12-4 G12-5 G12-6 G13-4 G13-5 G13-7.
- **Real, corrected (101):** G1-1 G1-2 G1-3 G1-4 G1-5 G1-6 G1-7 G1-8 G1-10 G1-11 G1-12 G1-13 G1-15 G2-1 G2-3 G2-4 G2-5 G2-7 G2-12 G2-13 G2-14 G2-16 G3-1 G3-2 G3-3 G3-5 G3-7 G3-9 G4-1 G4-2 G4-3 G5-1 G5-2 G5-7 G5-8 G5-10 G6-1 G6-2 G6-3 G6-4 G6-7 G6-8 G6-10 G6-11 G6-12 G7-1 G7-2 G7-3 G7-4 G7-6 G7-7 G7-8 G7-9 G7-13 G7-14 G8-2 G8-3 G8-5 G8-6 G8-7 G8-8 G9-2 G9-3 G9-4 G9-5 G9-6 G9-17 G10-1 G10-2 G10-3 G10-5 G10-6 G10-7 G10-9 G10-10 G10-11 G10-12 G10-14 G11-2 G11-4 G11-7 G11-9 G11-10 G11-14 G11-17 G11-18 G11-19 G11-20 G11-22 G12-1 G12-2 G12-7 G12-8 G12-9 G12-10 G12-11 G12-12 G13-1 G13-2 G13-3 G13-6.
- **Overstated, rewritten (5):** G5-11 G7-5 G7-12 G9-18 G10-4.
- **Wrong in part (1):** G9-9, corrected in G2-12.

Four ids could not be checked by the verifier assigned to them because the report held no separate text. G6-13 and G1-15 were checked through their parents, SDK:208-211 and G4-3. R-5 and R-18 had no tag: R-5 is the D6 and G1-2 correction, and R-18 is in the section 6.3 file list.

**Added by the re-audit:**

| Id | Severity | Finding | Where |
| --- | --- | --- | --- |
| N-1 | S1 | Gate `eee_control_semantics` still blocks EEE after G4-1 | Prompts 29-30 |
| N-2 | S2 | Superseded board pins in the repo | Prompts 37-42 |
| N-3 | S2 | Persistent `nextPage` defeats the NP epoch | Prompt 17 |
| N-4 | S3 | Glue deletion unbounded with a blocking handler | Prompts 37-42 |
| N-5 | S3 | CDCR[14] handling needed before prompt 35 | Section 5 table, P25 |
| N-6 | S3 | REGCR function 10 conflict in SNLS505H | Prompts 62-67 |

Other additions are folded into their parent entries: channel-A-only TDR (G3-1), pin 31 LOW conflict (G5-10), RMII reverse recipe (G9-6), 825 FLDS type (G8-4), compliance with robust set (G5-6, Q19), owner-table mechanics (section 5).

**Independent review of the contract (R-1 to R-18), and where each landed:** R-1 OP-5, OP-12, OP-14. R-2 OP-9. R-3 OP-9. R-4 OP-2 and the kept list. R-5 D6, G1-2. R-6 OP-13. R-7 OP-9. R-8 OP-9, OP-14. R-9 OP-9, COMMON sentence. R-10 OP-14. R-11 OP-14, `busQuietUntilUs`. R-12 OP-6. R-13 OP-5. R-14 OP-5. R-15 OP-4. R-16 OP-9. R-17 OP-9, OP-13. R-18 file list.

**Decision taken while merging.** G10-11 proposed reusing sibling audit tables across prompts 68-80. Rejected: "re-audit siblings at each stage" is a recorded owner requirement, so it stays. G7-6 and G10-11 each proposed a coverage matrix; there is one, in COV (D13).

**Limits of the re-audit:**

- Physical behaviour remains unmeasured, and nothing here ran on a PHY or P4.
- Some items were read as text, not rendered pages: parts of SNLS505H pin tables, SNLS638C 6.3.5 and 6.3.11 prose, SNLS783A RXFS and duplex rows, SDAA067A sections 2 and 3.1, SNLA266B tables, SNLA423A and SNLA491 hard-reset wording, and the 826 CR1 row in one check.
- The IEEE copy is a non-publisher mirror of the 2012 edition. ESP-IDF log sources were read through a fetch summary, not as raw files. The EasyEDA netlist itself was not opened, only its records, which state the netlist confirmed the pins.
- COV, RV and AC blocks were sampled in some checks, not fully recomputed again. No real CMake 3.16 run, no ESP-IDF build, and no live `workflow_dispatch` test.
- The state model and refactor prototypes are scratch work and are not retained in the repository.
