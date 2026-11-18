# Prompt 7 independent contract audit

Audited 2026-10-29 on `promptset/dp8382x-implementation-20261028`, starting
clean, fetched and equal to its upstream at
`61e2057590951c740a713786b471228f43d6fbe1`. Original execution base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`; origin is
`https://github.com/janhavelka/DP8382x.git`. The final handoff records the
corrective commit and verified sync, avoiding a self-referencing commit.

## Scope and independent review

Re-read [original prompt 7](../../prompts/7-value-transport-contracts.md) in full,
parent/repository AGENTS, COMMON, CONVENTIONS, ENGINEERING, FULL_COVERAGE,
EVIDENCE and VERIFICATION. Reviewed actual files and `49aadf7..61e2057`, not
the earlier completion summary. The [API contract](../../api-contracts.md)
remains a design deliverable: no production headers, successful stubs or unused
operation engine were added. Prompt 8 still owns cooperative lifetimes.
No workflows exist; remote CI dispatch/status/waiting remains prompt 71 only.

The lead owns all edits and Git. Three fresh read-only reviewers worked in
parallel with no writable repository paths:

- `p7_coverage_audit`, gpt-6.1-sol/high: requirements, naming, scope and complete
  feature/field/procedure mapping; capable model for bounded inventory work.
- `p7_evidence_audit`, gpt-6-astra/high: original TI/SDK/sibling evidence,
  declaration syntax and verification limits; stronger source reasoning.
- `p7_state_audit`, gpt-6-astra/xhigh: independent error, release, ownership,
  timestamp and deadline cases, followed by focused correction review.

The lead checked findings against actual contract/source bodies and original
SNLS647I page 56, table 8-20. The first two reviews found no missing family,
procedure or unsupported support claim. The state review identified three
precision gaps; source review identified one incorrect example literal.
Focused rereview confirmed the final corrections are consistent and implementable
without new fields; no blocking finding remains in this prompt-7 audit.

## Confirmed findings and corrections

| Finding in the original commit | Correction |
| --- | --- |
| BMSR examples `0x7809`/`0x7819` clear fixed read-only capability bit 6. | Use `0x7849`/`0x7859`, retaining low link and the latter's remote-fault sample. Table 8-20 and the existing independent BMSR oracle agree. |
| A new health error with invalid time could retain an older error's valid timestamp. | Replace timestamp validity with every counted error; unavailable time is `0` with validity false. Prior success remains historical, and clock-invalid state blocks age comparison. |
| Clock-regression prose preserved only transport failures while the precedence rule preserved any primary failure. | Preserve every normalized primary failure, then release/unsafe-controller failure, then clock failure; retain clockInvalid independently. |
| Health exclusions depended on whether lock acquisition was attempted, a fact absent from BatchResult. They also left BUSY plus release failure ambiguous. | Classify from actual statuses, frame flags and release/controller evidence. Clean zero-frame admission rejects do not count; lock timeout, new release/controller failure, malformed results and attempted/late failures do. Retain the counted health cause separately from the public primary status. No new field is needed. |

Small clarifications specify callback-local detail membership, valid frame
timestamp bounds/order and optional post-callback fallback, and the lastBatch
receipt's need for caller/collector attribution. No descriptor registry or second
transaction mechanism was added. The plain-owner example now returns bind's
Status and relies directly on the core's unchanged-on-failure publication
contract, removing redundant staging.

Focused case review uses the final rules (contract review, not driver execution):

| Case | Public result and retained outcome |
| --- | --- |
| Valid zero-frame BUSY/OWNER_ADMISSION; clean release; backward post-sample | BUSY remains primary; clockInvalid latches; no managed count or output. |
| Same admission rejection with release MDIO_ERROR/-37 | Public BUSY; one managed failure with health lastError MDIO_ERROR/-37; shared quarantine. A clock fault additionally invalidates the new error timestamp. |
| Zero-frame OPERATION_TIMEOUT; clean release and reusable controller | No managed count even if admission was checked under a lock; totalBatches increments. |
| Valid zero-frame BUSY/CONTROLLER_QUARANTINED; clean current-call release | Existing quarantine is preserved; no new managed count or implied recovery. |
| Primary OK, release OK, controller unexpectedly unusable | MDIO_ERROR/0; one managed failure, unchanged ordinary output and shared quarantine. |
| Managed batch succeeds but returns at/after deadline | OPERATION_TIMEOUT/DEADLINE; one managed failure, late=true, effects retained. |
| Previously timestamped error followed by a counted clock failure | New error has lastErrorMs=0 and lastErrorTimeValid=false; old time is not attached to the new cause. |
| Optional frame timestamp omitted / contradictory stamp | Valid post-callback time can conservatively timestamp a completed sample / FRAME_REPORT normalizes the malformed report and retains raw evidence. |
| Low BMSR `0x7859`, then MDIO_TIMEOUT/-37 | Ordinary output unchanged; successful destructive word retained; possible lost history remains explicit. |

## Fresh source and coverage evidence

All sibling worktrees were rechecked clean at these unchanged revisions. Exact
audited paths and selected adaptations remain in the
[original report](7-value-transport-contracts.md#fresh-sibling-evidence):
TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9`,
PCA9555 `498178e4d9e4acb6fab7d8b816ad5021efe8806a`,
INA228 `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b`,
MB85RC `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`, and
SHT3x-main `f65f15573e3c9b1ef04cce72ad08f307d021e563`.
Reopened Status/Config and state code, INA health/trust, MB capability definitions
and SHT cached/live publication. In particular TMP1x2 `src/TMP1x2.cpp:130–132`
assigns lastError and both timestamp fields together; PCA9555's transport mapping
preserves signed detail; SHT live readSettings can publish before later failure.
Their differing policies are not silently inherited.

The source reviewer freshly fetched TI's pinned `dp83826.c` and `generic_phy.c`
at `94bf4343004732983b0b8e1eec123d88ff70453b`, checked the identity/link failure
paths, and visually reviewed original SNLS647I pages 56–57. The lead also viewed
page 56. SDK `esp_eth.c` at `fff9895c82d744c7237be8847347bdd1b07c6643` still
supports the documented stopped-driver control/cache chain and rejects the idea
that raw frame hooks could own a whole MMD sequence. No SDK baseline, vendor
payload, silicon ledger or existing source gate changed.

Independent mapping checks found exactly 35 unique feature IDs, all 52 procedure
IDs, zero procedure/feature ownership mismatches, and all 283 fields assigned
to mapped owners. Their normative legal-value source remains the ledger;
unfinished feature-specific signatures remain with their implementation stages.

## Verification and documentation

Environment: Windows NT 10.0.26300.0, PowerShell 5.1.26100.9711, Python 3.12.10,
MinGW-w64 UCRT g++ 15.1.0.

| Evidence | Final local result |
| --- | --- |
| `python tools/verify_references.py` | PASS: 27 artifacts (43,240,385 bytes), 25 extracts, 1,294 local links in 149 Markdown files. |
| `python tools/verify_ledger.py` | PASS: 96 registers, 52 procedures, 35 features; research structure/coverage only. |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS: all 77 research tests, 15.806 s. |
| Strict C++17 documentation syntax/static assertions | PASS: all six declaration/example blocks. |
| Feature/procedure and BMSR example checks | PASS: exact mapping and independently specified capability/link/fault bits. |
| `git diff --check` | PASS. |
| Host driver execution / portable-package consumer / SDK link | NOT RUN: no working driver is claimed in this design stage. |
| Remote CI / physical evidence | NOT RUN: prompt 71 / no fixture action. |

The six documentation C++ blocks were extracted into a temporary translation
unit and checked with `g++ -std=c++17 -Wall -Wextra -Werror -pedantic
-fsyntax-only`. Static assertions cover deleted copy/move, OK versus IN_PROGRESS
and invalid enum, signed detail preservation, and invalid default result/time
evidence. Separate checks compare exact feature/procedure membership and BMSR
example bits. These checks prove syntax/records, not implementation behavior.

Architecture and example notes now reflect health-cause attribution and direct
output publication. README, feature coverage, variants, ownership design and
public-header plan remain accurate: no scope, lifecycle architecture, feature
mapping or introduction stage changed. CHANGELOG, reports and progress link this
audit. Existing test/tool commands need no change. No new tests purport to execute
unimplemented PHY behavior.

Portable/package consumers, host driver execution, SDK linkage and physical
qualification remain NOT RUN. Remote CI remains NOT RUN under prompt 71's
exception. Existing silicon gates, native lifecycle correction and prompt 8's
clock-fault cleanup dependency remain explicit later-stage work.


## 2026-11-17 current-code response

Rechecked all four original findings and all nine focused-case rows against
current shipped headers, implementation/tests and maintained OP contracts at
`main` `77c544f415edb26a0e23de76591a0163b9b73147`.

| Finding or case group | Current verdict and evidence |
| --- | --- |
| BMSR 7849/7859 capability examples | Corrected. Original table 8-20 p56 was reopened and bit 6=1 confirmed. `test/test_ledger.py` independent core access/reset literals and `test/link.cpp` use source-backed capability/link words. Bit 4 remains clear-on-read evidence. |
| New health error inherits old valid timestamp | Corrected in `src/DP8382xHealth.h::retainHealth`: replaces cause, lastErrorUs and validity together. `test/health.cpp` clock and saturation cases explicitly test zero/false on the new untimed error while retaining prior opposite-outcome evidence. Current units are microseconds, superseding this report's Ms field spellings. |
| Clock fault overwrites specific primary/release failure | Corrected in `_execute`: normalized primary wins, then release, then post-clock/deadline outcome; raw report/clockInvalid/late remain separately retained. `test/transport.cpp` reports/timing and `test/health.cpp` clock cases cover primary detail and release preservation. |
| Admission/health classification depended on unreported lock attempt | Corrected. `_execute` classifies actual statuses/frame/release/reuse evidence. Clean zero-frame BUSY/OPERATION_TIMEOUT and pre-existing quarantine are excluded; positive native controller-wait expiry maps MDIO_TIMEOUT, not BUSY. New release/controller faults count and quarantine. `health_reports`/`health_timing` and native controller-wait tests exercise these cases. |
| BUSY plus release failure/backward clock; zero-frame timeout/quarantine; unexpected unusable controller | Current predicate preserves primary BUSY while retaining release health cause, invalidates the new error's time on clock failure, and excludes clean admission denial. The corresponding `test/health.cpp` table covers exact causes and counts. |
| Historical row: successful callback returned after deadline counts managed failure | **Superseded design**, deliberately not restored. Current `docs/api-contracts.md` health predicate, `docs/health.md` and `_execute` distinguish transport success from owner timing. `test/health.cpp` lines 429-433 requires one success, zero failures and public OPERATION_TIMEOUT/late. A clock fault similarly does not turn sound MDIO into OFFLINE. This reconciles the old case table with implemented stage 21/G11-7. |
| Optional/malformed frame stamps; low BMSR followed by read failure | Current frame audit validates optional stamps/order; collectors retain validated consumed prefixes even if publication fails. `test/transport.cpp`, `test/access.cpp` and `test/link.cpp` check malformed reports, unchanged ordinary outputs and retained partial/destructive evidence. |
| Owner example bind status and copied receipts | Current API examples retain an exact core reference and return bind Status. Shipped facade deletes copy/move; snapshots remain value copies. `test/foundation.cpp` and isolated header translation units exercise public contracts. |

The 35-feature/52-procedure mapping remains guarded by the ledger checker; it
does not imply every source-gated branch is implemented. Historical no-driver
claims and private native sketches are not the current API. Current operation
state and cleanup are governed by OP9 and the shipped headers.

PASS here:80 ledger Python tests,97/52/35 checker, version check and independent
native 63/63 Release CTests with 45 pinned SDK blobs. No new production defect
was established. Portable consumer/new SDK image/remote CI/physical checks were
NOT RUN by this reviewer; this response corrects interpretation of old evidence.
