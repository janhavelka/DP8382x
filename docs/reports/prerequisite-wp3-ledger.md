# WP3: source decisions, ledger and independent regressions

Date: 2026-11-07. Execution branch: `promptset/dp8382x-implementation-20261028`.
Base checkpoint: `88af952` (WP0, synchronized by the lead). This contribution
changes research records, their validator/tests and assigned guides. It does not
implement feature procedures or change production headers, siblings or Git.
The original prompt13 collector and its current destructive-acquisition/mixed-write
consumers remain required; the later audit's conflicting deferral is not adopted.

The same capable current model performed this disjoint source/test review; the
lead independently integrates production policy and the shared contracts. Original
PDFs were reopened rather than using production constants as expected values.
The fresh sibling audit, revisions, selected/rejected principles and vendor hash
verification are preserved unchanged in [WP0](prerequisite-wp0.md).

## Source selection and changes

- **D2 / G4-1 / N-1:** the five EEE associations now retain both the printed
  address and an exact source-implied wire address, with explicit safe-read or
  collector routing. SNLS638C tables7-99..103, SNLS505H7.4.2.9 and tables8-138..142,
  SNLS647I8.3.11.1 and SNLS603D p47 provide the selected chain. SNLS647I's full
  numbers and SNLA328 are retained counter-readings. These are inferences, not
  exact-part tables or measured wire proof. Whole-word capability/readback checks
  fail closed, including FFFF. The sourced clock row uses ownership preflight,
  not a blanket pin gate. Opaque disable remains hard reset and reapply.
- **D3 / G3-1/2/5/6:** SNLA428 table2-1 is selected explicitly over the differing
  datasheet tuning for the derived transmit-A helper. The ledger adds one opaque
  row, exact0411/0416/GENCFG actions, six runs, write order, matching-row threshold
  and formula, whole-word classification, NO_PEAK/NOT_RUN and raw-result validity.
  Table2-5's100m example is independently evaluated. The MATLAB indexing,
  undefined temporary and differing delay constant are not copied. PairB and
  arbitrary raw tuning do not acquire a derived interpretation. The raw datasheet
  tables and reserved banks remain unchanged. Q7 still needs known cable fixtures.
- **D4 / G2-1:** coherent BMCR plus current link establishes forced mode under
  the selected interpretation; the extended100half exception keeps duplex
  unknown. Raw PHYSTS remains available under Q11. Pause uses IEEE resolution
  with a PHYCR cross-check. Extended duplex does not require invented peer
  preflight, and port mirroring uses both controls.
- **D7 / G5-1/6/8/9:** all14 table8-50 10M pattern actions are enumerated under
  mask001F, preserving the ordinary000F/reservedFFF0 split.100M0428 remains
  gated. Compliance composes writable PHYCR/BISCR masks, refuses power-down or
  isolation, stops by hard reset and requires initialization/reapply. Resistor
  contingency completion is hard-reset completion, not invented calibration status.
- **D8:** next-page intent is transient; ordinary advertisement excludes it.
  Success clears intent without restart, failure/cancel retains cleanupOwed,
  and separately admitted initialization restores ordinary AN. Q10 remains open.
- **Other applicable ?7.1 corrections:** attempted hard reset imposes the shared
  frame-free2ms policy and possible eligible-strap resampling; reverse/analog
  loopback add the sourced restart while the illegal analog word is rejected;
  near-end modes restore crossover; RXFS shared-event routing and dependent
  clearers preserve partial evidence; IRQ service drains both MISRs with bounded
  more-pending evidence; BMCR request versus pin observation stays distinct;
  WoL neutral/acknowledgement follows exact W0C with Q3 retained, harvests first,
  and no longer claims differing DP83822 wire packing; mode-specific LED and
  clock conflicts remain explicit; RMII permits a zero-cause Fast Link Drop
  disable, and response maxima retain their detector prerequisites.

The ledger has97 register records:79 detailed rows and18 opaque recipe rows,
283 detailed fields,52 procedures and35 features. Five EEE associations are kept
outside the exact-part detailed map so cross-part field inference cannot silently
become an exact-part reset/access assertion. No unknown reset bits were filled.
The small validator remains part-specific. It checks exact exception inventories,
EEE tuple/route associations and TDR structure; independent source literals check
numeric content in the tests and [vectors](../register-vectors.md).

## Verification

Windows PowerShell, repository Python interpreter. `python tools/verify_ledger.py`:
PASS,97 registers/52 procedures/35 features. `python -m unittest discover -s test
-p "test_*ledger.py" -q`: PASS80 tests, including malformed EEE routes/offsets,
missing TDR runs/order/gates, the full original-source TDR matrix and100m example,
all14 compliance values and unchanged detailed-field checks. `git diff --check`:
PASS (line-ending warnings only). After the lead relocated temporary draft copies,
`python tools/verify_references.py`: PASS29 vendor artifacts (46,886,437 bytes),
27 text extracts and1361 local links in164 Markdown files. The earlier failures
were confined to those temporary copies, not source or archive integrity.

SDK/controller/physical tests: NOT RUN in this research unit. Q1?Q19 remain
identified physical questions as applicable; no runtime result or hardware support
follows from these source decisions. Vendor PDF payloads and provenance remain
byte-for-byte unchanged from WP0. Commit/sync and final integrated checks belong
to the lead; this agent did not operate Git.
