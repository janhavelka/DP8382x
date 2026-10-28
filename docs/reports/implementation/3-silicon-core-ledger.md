# Prompt 3 — primary register ledger and core silicon behavior

Completed 2026-10-28 as **core research and offline tooling**, not driver,
full-ledger or physical completion. Prerequisites are the prompt-1
[API/ownership audit](1-core-api-state-ownership.md) and prompt-2
[tooling audit](2-ecosystem-tooling-audit.md), including their selected conventions.

## Execution and ownership

Resumed `promptset/dp8382x-implementation-20261028` from clean entry checkpoint
`e89c024aa4a149bcf56e7ba5dd7bcf23d4966d40`. Fetch confirmed zero ahead/behind
against the same-name upstream. Original base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036` (`main` / `origin/main`). The lead alone
owns schema/shared guides/progress, integration, staging, commit and push.
Final commit and verified sync IDs are in the handoff; the commit does not
attempt to contain its own hash. No release or hardware operation was requested.

Applied root/parent rules, COMMON, FULL_COVERAGE, ENGINEERING, SOURCE_MAP,
EVIDENCE, CONVENTIONS and VERIFICATION. Kept exact-part evidence, namespace/mask
separation, one owner/collector, finite operations and separate evidence layers.
No C++ scaffolding, SDK build metadata or fabricated successful hardware stubs
were added. The repository has no workflow definitions to dispatch; sibling
workflow definitions were reopened as read-only precedents. Remote CI remains
prompt 71 only, with no dispatch, run-status query or wait in this stage.

## Delivered files and acceptance

| Output | Completed content |
| --- | --- |
| [Ledger](../../registers/dp83826i.json), [schema](../../registers/README.md) | 79 summary entries, 17 vendor recipe offsets, five separately gated printed EEE addresses; 20 complete core records containing 129 fields; reset knowledge, reserved partitions, access, legal/prohibited values, source refs and ownership. |
| [Behavior](../../device-behavior.md) and [register guide](../../register-map.md) | Exact identity/declaration, selector ownership, reset/strap domains and uncertainty, required 10BASE-Te initialization, AN/forced/MDIX/pause and per-source read effects. |
| [Vectors](../../register-vectors.md) | Independently derived ID/field/reset partitions, reset polling address-versus-mask, C22 rejection, VOD literal frames, negotiation/crossover and full pause matrix. |
| [Coverage](../../feature-coverage.md) | All 35 feature families assigned research/API/CLI/test/implementation owners; documented, implemented, host-tested, SDK-built and hardware-qualified evidence remain distinct. |
| [Validator](../../../tools/verify_ledger.py), [tests](../../../test/test_ledger.py) | Offline standard-library checks and 36 tests registered by unittest discovery; malformed temporary JSON files exercise the CLI. |
| [Fresh source audit](3-source-audit.md) | Current sibling paths/revisions/conventions, manufacturer/upstream files and licenses, EVM/SDK/support searches, original IEEE pause-table provenance and limitations. |

The 12 accepted procedure records cover identity, direct access, complete indirect
transactions, PCS reset, hard reset, circuitry restart, external reset/POR,
10BASE-Te initialization, negotiated configuration, forced/crossover configuration,
next-page exchange and live/retained link observation. Each has explicit ownership,
prerequisites, ordered steps, wait/deadline rules, accepted/uncertain effects,
finite cleanup and persistent restoration. Next-page exchange explicitly contains
an implementation gate for exact-part handshake confirmation; its IEEE-derived
phase order is recorded without claiming measured silicon behavior. Exact API
signatures remain prompts7–8.

Fields shared with diagnostics/LED/interface controls have complete core numeric
partitions but their feature procedures stay assigned to prompts4–5. No partial
feature is reclassified as unsupported merely because it is not implemented.
Hardware-only straps and outside-PHY work are separately classified. All-reserved
entries are retained without writable permission.

## Delegation and independent review

The independent PDF reviewer used **gpt-6-astra, high reasoning**: stronger
reasoning was explicitly requested for original silicon arithmetic/access review.
It was read-only and derived its oracle before reading the new ledger. Original
PDF pages were rendered and visually checked: access/core tables8-18..34
(pp54–65), PHYCR8-43 (p71), PHYRCR8-48 (p73), VOD8-81 (pp85–86), SOR8-87/88
(pp89–90), plus feature/reset/strap/timing sections. Its pause oracle came from
the original IEEE table identified in the source audit. The review subsequently
matched all20 core address/field/reserved/access records against that oracle.

The source auditor and validator implementer used the inherited capable model:
source comparison and part-specific Python validation were disjoint bounded
tasks. Writable paths were respectively `3-source-audit.md`, and
`tools/verify_ledger.py` plus `test/test_ledger.py`. Neither changed Git, shared
contracts, siblings or archived payloads. The source auditor then independently
reviewed validator/tests read-only. No model-selection capability was assumed
for the inherited workers beyond the current available model.

Review corrections integrated:

- Excluded prohibited local remote-fault bit from advertisement ownership.
- Corrected partner next-page reset prose, distinct from local null-message code.
- Rechecked SNLA261A front page and corrected its revision date to March2024.
- Preserved separate delayed-clock timing statements and explicit CR3 default gate.
- Added independent assertions for every root identity number, not just field masks.
- Rejected enormous timing JSON integers cleanly without float-conversion overflow;
  added a temporary-file regression and positive all-reserved-register validation.
- Checked original IEEE next-page state-machine pages: replaced pre-restart
  loading with a fresh-base/post-restart load contract, distinguished BASE/NEXT
  events, required bilateral/null-page termination, and added the exact-part
  handshake gate rather than guessing management sequencing.

The initial integrated validator caught a missing BMCR membership in the MII
feature's ownership list. That was fixed in the data, and all negative tests
then exercised their intended failure rather than failing on a bad baseline.

## Verification results

Host: Windows, PowerShell, Python **3.12.10**, repository cwd
`C:/Users/Honza/Documents/Projects/DP8382x`. Standard library only at runtime;
PDF rendering used an already available local PDF reader for research, not a
consumer dependency. Sibling/source paths and exact Git identities are in the
source audit. Archived vendor payloads/provenance were not changed.

| Evidence layer | Status | Actual scope |
| --- | --- | --- |
| Host research tools | PASS | `python tools/verify_ledger.py`:96 registers/12 procedures/35 features; `python -m unittest discover -s test -p "test_*.py" -v`:36 tests, including malformed temporary files and independent numeric literals. This is not host driver behavior evidence. |
| Reference integrity and local links | PASS | `python tools/verify_references.py`:27 unchanged vendor artifacts,43,240,385 bytes,25 text extracts; local Markdown links checked. |
| Diff/format | PASS | `git diff --check`; final task-only diff and new files reviewed. |
| Portable/package consumer | NOT RUN | No portable implementation/package exists at this research stage. |
| Native SDK link | NOT RUN | ESP-IDF v6.1 source baseline retained; source review is not compilation/linking. |
| Remote CI | NOT RUN | First-execution exception: prompt71 owns this evidence. No historical CI SHA invented. |
| Physical | NOT RUN | No identified/admitted fixture, flashing, reset-pin operation, MDIO trace or electrical measurement. |

Unittest assertions are active independently of Python's plain `assert` removal;
these tests do not depend on Release compiler behavior. Future C++ suites must
still establish their own assertion/build registration contract. No checked
file claims that these research tests execute reset/driver behavior on hardware.

## Remaining evidence and stage handoff

Fifteen named gates in the ledger have owners and next evidence. Core material
ones are `forced_physts` (coordinated portable/SDK link interpretation),
`hard_reset_straps` (software-hard-reset resampling), `pcs_reset_extent`
(complete PCS-domain enumeration), `vod_access` (general tuning versus exact
sanctioned initialization), and `next_page_handshake` (exact-part event/load
sequencing). The selected conservative behavior permits independent
implementation work while withholding the disputed claims. Exact TI clarification
or identified measurements are needed to close the affected ambiguity.

Prompt4 owns diagnostic access/procedures, CR3 defaults, MSE, counter saturation,
RMII RC polarity, TDR reserved results, loopback and calibration. Prompt5 owns
EEE wire notation/recipe completion, power/wake, per-pin controls, IOCTRL/compliance
exceptions and remaining interfaces/analog fields. Physical qualification remains
prompts72–73. These are genuine remaining gates, not missing prompt-3 outputs.

Reviewed and updated README, CHANGELOG, variants, architecture, source-navigation
guides, verification instructions, future Doxygen citation requirements and
implementation progress. Existing provenance/license notices remain authoritative.
No vendor implementation was copied; GPL material was used only for factual
comparison. No new package/driver support is claimed by a completed research row.
