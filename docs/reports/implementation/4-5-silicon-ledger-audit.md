# Fresh independent audit of prompts 4 and 5

Audited on 2026-10-28 from clean, fetched, synchronized checkpoint
`604096c4355765d45a6686a16fabd479be2d2e5e` on
`promptset/dp8382x-implementation-20261028`. The original execution base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`; origin is
`https://github.com/janhavelka/DP8382x.git`.
The final handoff records this audit's verified commit/upstream equality.

Re-read prompts [4](../../prompts/4-silicon-diagnostic-ledger.md) and
[5](../../prompts/5-silicon-power-interface-ledger.md) in full, parent/repository
AGENTS, COMMON, FULL_COVERAGE, ENGINEERING, SOURCE_MAP and EVIDENCE, plus entry
guides and the actual checker, tests, JSON and commits2805cfd/604096c.
Previous summaries were navigation only. Original PDFs and source-first literals
were compared with current artifacts. This remains source research, not driver
implementation or hardware qualification.

The lead owns all shared edits, documentation and Git. Three parallel reviewers
were read-only; no reviewer edited siblings, vendor bytes or shared files:

- `audit45_diagnostics`: gpt-6-astra/high, fresh context, selected for
  destructive access and contradictory tables; independently reviewed loopback,
  BIST, cable, events/counters and quality.
- `audit45_controls`: gpt-6-astra/high, fresh context, selected for coupled
  silicon fields and power/cleanup semantics; independently reviewed power,
  EEE/LPI/WoL, pins, analog and compliance.
- `audit45_checker`: current capable model, fresh context; actual schema/tests,
  malformed-record experiments, requirement coverage, simplicity and current
  sibling declarations/implementation/tests.

## Original-source coverage

| Scope | Original evidence independently inspected | Audit disposition |
| --- | --- | --- |
| Loopback, BIST and freshness | SNLS647I pp41–43,54,68–73; tables8-19/40/45–47; table8-45 also visually rendered | Per-speed loopback, PRBS modes, freeze/clear read-write overlap, IPG and reset-done interpretation correct |
| TDR and quality | SNLS647I pp76–81, tables8-54–72 | Segment controls/words, reserved result banks, raw capture and MSE address correct; derived units/classifications remain gated |
| Events/counters/link loss | SNLS647I pp15,17,44–45,49,63–72,92; tables8-33–41/44/91 | Consumed prefixes, ambiguous read loss, latch/clear sources, finite budgets, RECR/RMII/PHYSTS conflicts retained |
| Diagnostic supporting sources | SNLA423A pp12,18–20; SNLA344C pp9–10; SNLA494 pp9–11; SNLA495 pp6–7 | Conflicting examples and EtherCAT ESC/F28P65/MII applicability remain separate from P4 RMII |
| Power/EEE/LPI/WoL | SNLS647I pp15,26–29,91–93; SNLA261A, SNLA328, SNLU262A | Policy/state separation, NOM timing, wire gates, neutral W0C and exact-part MAC/password mapping correct |
| Pin/electrical/compliance | SNLS647I tables8-42/49–51/73–80/82–86/89–90/92–97; SNLA266B table5-5 | All25 remaining field partitions/access types correct; one split-field procedure issue fixed below |
| Ownership and family scope | Actual JSON feature/procedure records, variants guide, exact family source assignments | All79 detailed entries,17 opaque records,five blocked EEE associations,35 families and52 procedures accounted for; no inferred suffix support |

No confirmed prompt-4 silicon or procedure defect was found. Its register facts
and diagnostic procedure records are unchanged by this audit. Existing core and
diagnostic conflicts remain explicit, including RECR width, RCSR polarity,
MSE address, PHYSTS forced mode, BIST lifecycle, TDR calibration/freshness and
reserved banks. None was hidden by a fake, broadened mask or support claim.

## Confirmed findings and corrections

1. **Split amplitude transition ordering.** SNLS647I tables8-79/80 pp84–85
   divide MDIX-minus between two words and document combined codes28..38.
   The old procedure validated endpoints only. For2F-to30, high-first passes
   through3F and low-first through20; neither intermediate is documented.
   MAC traffic quiescence does not prove inactive analog output.
   The corrected procedure establishes the observed starting code, stages through
   low nibble8, verifies each write before dependent continuation, and applies
   the same rule to restoration. Unknown/invalid current state prevents dependent
   writes and retains recovery-required state. Temporary amplitude changes remain
   explicit; the procedure makes no glitch-free claim.

   The lead independently reopened both original tables and checked the arithmetic.
   Primary maximum is68 frames for5 snapshots,6 writes and6 readbacks, plus16
   prerequisite frames. Cleanup reserves56 frames for2 current VOD rereads,
   6 writes and6 readbacks. These counts require retained-word composition and
   exclusion of conflicting writers/reset, with finite duration limits as before.
   Tests cover289 endpoint pairs and restoration from each modeled prefix.
   These are research arithmetic and future failure-injection contracts, not an
   executed PHY transition implementation.

2. **Aggregate field oracles missed semantic changes.** The former remaining-row
   test combined RW/RHW and aggregate masks. A polarity RHW-to-RW change or
   swapped LED enable/value masks passed those checks.
   Added an independently transcribed semantic-name-to-mask/access oracle for
   every field of all25 remaining detailed rows, including the all-reserved row.
   The source reviewer supplied literals before comparison. Reproduced mutations
   now fail individually; numeric expectations remain outside the structural checker.

3. **Procedure ownership was not fully guarded by numeric tests.** Removing all
   ordinary WoL configuration masks passed the old remaining oracles.
   Added independently reviewed exact ownership maps for all24 stage-5 procedures.
   Empty ordinary ownership remains correct for legacy LPI because its required
   opaque write actions carry ownership. The missing-WoL-mask mutation now fails.

4. **Optional explanatory text was not type-checked.** The schema documents
   printed-address `disposition` as text, but malformed containers, flags,
   numbers or empty values passed. The checker now validates nonempty text when
   present; omission remains valid. Malformed temporary-record tests cover these
   cases without introducing a general schema framework.

The independent reviewers rechecked the actual corrections and found no further
confirmed issue in their scopes. A representative931-case structural mutation
sweep also found no uncaught exceptions. That experiment is robustness evidence,
not proof that arbitrary text accurately describes silicon.

## Fresh sibling and current-source audit

Read-only source review reopened relevant rules, headers, actual failure/cleanup
paths, rejection tests, CLI examples and Python runners:

- TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9`, clean:
  `include/TMP1x2/Config.h`, `include/TMP1x2/TMP1x2.h`, configuration/
  operation source (`src/TMP1x2Configuration.cpp`, `src/TMP1x2Operations.cpp`),
  `test/variants/test_variants.cpp`, `test/jobs/test_jobs.cpp`,
  `examples/common/Tmp1x2CliDiagnostics.cpp` and `tools/hil_tmp1x2_runner.py`.
  Retain explicit capability rejection, accepted/uncertain writes and retained
  results; sensor conversion timing is not PHY timing.
- PCA9555 `498178e4d9e4acb6fab7d8b816ad5021efe8806a`, clean:
  `include/PCA9555/PCA9555.h` result/snapshot declarations,
  `src/PCA9555.cpp` input/cleanup paths,
  `test/test_basic.cpp` failure cases around line795,
  `examples/01_basic_bringup_cli/main.cpp` and `tools/run_i2c_hil.py`.
  A consumed valid observation survives later cleanup failure;
  primary/cleanup results remain separate.
- MB85RC `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`, clean:
  `include/MB85RC/CommandTable.h`, public power contracts, sleep/wake
  implementation in `src/MB85RC.cpp`, `test/test_basic.cpp` rejection/uncertainty
  tests, `examples/common/DiagnosticCore.h` and `tools/hil_runner.py`.
  Explicit variant capabilities and UNKNOWN on ambiguous power operations fit
  the PHY; memory wake timings and command framing do not.
- TUSB32x now `aca463158485d13fe8d4437e1962dda4a4c32516`, with unrelated dirty
  work preserved: current AGENTS and `docs/prompts/2-family-capabilities.md`
  reopened. Shared identity does not detect ordering suffix/capability; this is
  still a planning reference, not a full implementation claim.

The lead re-resolved current manufacturer branches through GitHub:
[MCU+ next](https://github.com/TexasInstruments/mcupsdk-core/commit/4b9a66f4e18e02a834961f5f834155c846cab041),
[RTOS main](https://github.com/TexasInstruments/ethernet-rtos-drivers/commit/94bf4343004732983b0b8e1eec123d88ff70453b)
and [combined TI Ethernet main](https://github.com/TexasInstruments/ti-ethernet-software/commit/450af1144b8caeb58724bcbae081b409f6bf5c3a)
are unchanged from the previous audit. The complete pinned MCU+
`source/board/ethphy/icss_emac/ethphy_dp83826e.c` was re-read, including its
BSD-3-Clause notice and LED/odd-nibble/power helpers. Its observed defects do
not override original tables. No vendor implementation was copied.

Targeted TI support searches for DP83826 BICSR1 freeze, EEE203C/1001 and CDCR/TDR
did not produce an inspected exact-part correction closing the current gates.
The [TI product listing](https://www.ti.com/product/DP83826E) still exposes Rev.I.
These are bounded source checks, not an assertion that no clarification exists.
Vendor bytes/hashes and all24 named source gates remain unchanged.

## Verification and completion

Windows/PowerShell, Python3.12.10; standard-library tests/checkers. PyMuPDF was
used only to inspect originals. No sibling tests, firmware or fixture was run.
No CI workflow was added or changed; remote CI is reserved for prompt71.

| Layer | Final result |
| --- | --- |
| Ledger structure/coverage | PASS: `python tools/verify_ledger.py`;96 registers,52 procedures,35 features |
| Registered host research tests | PASS: `python -m unittest discover -s test -p "test_*.py" -v`;77 tests |
| Vendor/reference/local-link integrity | PASS: `python tools/verify_references.py`;27 artifacts,25 text extracts; local links checked |
| Task diff/independent final review | PASS: `git diff --check`; confirmed findings corrected, no unrelated changes |
| Driver host / portable-package / SDK link | NOT RUN: no such implementation added in this research audit |
| Remote CI / physical qualification | NOT RUN: prompt71 / identified later fixture campaigns |

Updated behavior, independent vectors, schema/test guides, coverage navigation,
README/CHANGELOG and progress. Reviewed architecture and variant boundaries;
their responsibilities remain unchanged. Future analog Doxygen must explain
intermediate profiles, current-state admission, per-write verification, partial
effects and finite restoration, citing the exact two VOD tables.
Research completion remains valid with named source gates; driver support and
physical qualification are not inferred from passing research tests.
