# Fresh prompt-3 audit

The user requested a fresh independent audit on 2026-10-28. Entry was clean and
synchronized at `01ce198a2ea2bf1284793b3876b90ea8ca5a1d86` on
`promptset/dp8382x-implementation-20261028`. Base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`. The lead re-read the complete original
prompt and applicable rules, and inspected actual files and committed/working
diffs. Three new reviewers worked in parallel; all were read-only. The lead
implemented and reviewed every correction and retained sole Git ownership.

## Findings and corrections

| Confirmed finding | Correction and regression evidence |
| --- | --- |
| VOD_CFG3's upper termination field was wrongly recorded as R | Original table 8-81 p86 explicitly says R/W. The lead separately rendered and checked it. Corrected the field and encoding descriptions, deleted the false `vod_access` gate and special validator exception, and corrected guides/tests. The earlier review missed this; this audit supersedes that conclusion. |
| Two nominal timings wrongly included power-up | Section 7.6 p15 specifies reset-to-100M signaling and reset-to-RMII leader clock only. Corrected start scope without inventing a reset edge; literal timing tests preserve values, units and NOM qualifiers. |
| Unformatted next-page framing was underspecified | IEEE Annex 28B.4 p732 and 28.2.3.4.13g p298 require a defining message page followed immediately by its referenced unformatted pages in order. Added this explicit requirement while retaining the separate exact-part handshake gate. |
| Invented MMD aliases bypassed address policy | Reject aliases on MMD-primary records until a source-backed allowance exists; compare unresolved printed tuples against all occupied addresses. Temporary-file regressions cover EEE tuples, direct-only selectors, holes and unresearched addresses. |
| Required core procedures could be removed with their backlinks | Added a small mapping of the 12 required procedure IDs to features, allowing later additions. Each removal is tested independently. |
| A procedure could claim a register outside its feature's coverage | Added the ownership cross-check and a targeted negative test. Shared field owners remain permitted. |

Also clarified that PHYSTS's published reset is not a universal post-strap
readback assertion. Added an independent access-mask oracle for all 20 core
registers, supplementing existing partition/reset/pause literals. Corrected the
old VOD test oracle rather than preserving its false expectation. Removing the
special VOD bypass simplifies the checker; no generic register framework, driver
stub or unrelated feature implementation was introduced.

## Independent review and scope

The new silicon reviewer used **gpt-6-astra, high reasoning**, explicitly requested
for original-PDF review. It derived access/masks from the original tables before
comparing the ledger and inspected original IEEE next-page text. The code and
coverage reviewers used the current inherited capable model for bounded Python
and requirements review. The code reviewer tried 1,020 malformed scalar/container
mutations without an uncaught exception; targeted valid-shape mutations exposed
the three coverage/ownership gaps above. Baseline success alone did not establish
correctness. Reviewers rechecked the actual fixes and found no remaining blocker.
A synthetic later-stage procedure still passes, confirming that the fixed core
subset does not prevent future additions.

Fresh coverage inspection found the original inventory, feature ownership,
source/license records, gates and evidence-layer separation complete for prompt 3.
The sibling re-audit and current revisions are in [source evidence](3-source-audit.md).
Existing external retrieval records were assessed, not misrepresented as all
re-downloaded. Archived payloads and hashes remain unchanged. Fourteen genuine
source gates remain explicit, with owners and next evidence; research completion
does not close disputed implementation or physical behavior. Prompts 4–5 retain
diagnostic and remaining-control research ownership.

## Verification and handoff

Windows / PowerShell / Python 3.12.10, repository root, standard-library tool runtime:

- PASS: `python tools/verify_ledger.py` — 96 registers, 12 procedures, 35 features.
- PASS: `python -m unittest discover -s test -p "test_*.py" -v` — 41 tests.
- PASS: `python tools/verify_references.py` — unchanged 27 vendor artifacts and
  25 extracts; local links checked.
- PASS: `git diff --check` and review of task-only changes.
- NOT RUN: portable/package consumers, native SDK link, remote CI and physical
  evidence. These are research-tool tests. Remote CI remains prompt 71 only.

Reviewed README (its claims remain accurate), updated CHANGELOG, behavior/map/
vectors/schema/coverage guides, test instructions, source/report corrections and
implementation progress. No public C++ API was added, so the existing future
Doxygen requirements remain unchanged. Final commit/sync identity is supplied
in the handoff without a self-referencing commit; the same-name upstream must
match with zero ahead/behind before reporting sync complete.


## 2026-11-17 current-code response

Every finding above was checked against current `main` at
`77c544f415edb26a0e23de76591a0163b9b73147`. The archived original SNLS647I
pages 15,56,84-86 were reopened; page 86 was rendered and visually checked.
The response preserves the earlier audit rather than rewriting its history.

| Original finding | Current verdict and evidence |
| --- | --- |
| VOD_CFG3 upper termination field incorrectly R | Corrected and retained. Original table 8-81 p86 explicitly R/W; ledger `VOD_CFG3` field `mdi_termination` is RW mask F000. `test/test_ledger.py::test_independent_mmd_and_vod_vectors` and the core access/field oracles cover it. Shipped initialization composes/verifies the mandatory 4A40 policy; `src/DP8382xAnalog.cpp::_analogVod3` supports explicit termination ownership. Reserved 5:0 remain excluded from ordinary writes. |
| Nominal signaling/leader-clock intervals incorrectly included power-up | Corrected. Original 7.6 p15 puts 0.5ms and 0.2ms in reset rows. Ledger timings retain reset-only scope and NOM; independent timing oracle retains values/units/qualifiers. Native `prepareBoard` uses separately sourced 50ms POR and 2ms reset silence, not these nominal values as maxima. |
| Missing defining-message/immediate-order requirement for unformatted next pages | Corrected, and now implemented under selected D8. The original IEEE 2012 PDF was retrieved with the report's identical SHA256 and printed 298/732 rechecked. `src/DP8382xConfiguration.cpp` and `test/negotiation.cpp` implement bounded post-restart loading, framing/null/termination rules. Current ledger `next_page_handshake` is physical Q10, superseding this report's historical software gate. |
| Invented MMD aliases and unresolved tuple collisions | Retained enforcement in `tools/verify_ledger.py` occupied-address/alias checks; `test_unproved_mmd_aliases_are_rejected` and tuple tests mutate aliases and printed addresses. Current source-implied EEE low-offset tuples are individually selected D2, not a general digit-stripping rule. |
| Core procedures removable together with backlinks | Retained `CORE_PROCEDURES` twelve-ID mapping in the checker and `test_each_core_procedure_is_required`; later procedures remain permitted. |
| Procedure could own a register outside its feature | Retained feature/register ownership cross-check and `test_procedure_owned_register_requires_feature_coverage`. Shared field owners do not bypass it. |
| PHYSTS reset overclaim and missing independent access oracle | Current ledger resetNotes preserve strap/mode uncertainty; `test_independent_all_core_access_masks` and semantic-field literals remain independent of production masks. Forced mode uses selected D4 BMCR decoding, retaining PHYSTS raw, with physical Q11 open. |

PASS in this response: ledger structure: 97 registers/52 procedures/35 features;
all 80 ledger tests (15.870s); generated version check. These validate research
records and source-backed arithmetic, not hardware. No additional production or
checker correction is required for the enumerated findings. Physical, package,
remote CI and a new SDK image link were NOT RUN here. Shared source gates retain
the current ledger decisions rather than the historical count of fourteen.
