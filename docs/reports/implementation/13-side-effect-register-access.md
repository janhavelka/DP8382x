# Prompt 13: side-effect-aware register access

Executed 2026-11-07 on `promptset/dp8382x-implementation-20261028`.
Entry was clean and synchronized at `81ab2d3`; the original execution base is
`5e148fffde074b85fadc15d8ff66fe2c77111036`. Prerequisite evidence checkpoint
`88af952` was committed, pushed and fetched with equal upstream IDs. Final
commit/sync identity is recorded in the handoff, avoiding a self-reference.

## Rules, scope and prerequisite reconciliation

Read COMMON, CONVENTIONS, EVIDENCE, FULL_COVERAGE and the stage-13 prompt against
the working stage-12 source. Retained namespace/class DP8382x, PascalCase types,
camelCase methods, fixed storage, one application instance owner and one
controller-wide terminal batch mechanism. Siblings and vendor payloads remained
read-only; the lead owned shared headers/guides, integration and Git. No SDK code,
allocation, register-manager framework, retry loop or placeholder feature API was
introduced. No workflow exists yet; CI trigger creation and remote checks belong
to prompt 71.

COMMON also requires the remaining-prompts audit before 13. Its WP0–WP7 changes
were integrated in dependency order, with source reconciliation checked before
dependent policy. The latest explicit user request supersedes G1-1's proposed
collector deferral: the collector has real acquisition and mixed-update consumers
now. Later features extend it. The complete named catalog is checked against the
single ledger; there is no second independent numeric specification.

- WP0: [source/ownership reconciliation and fresh sibling review](../prerequisite-wp0.md).
  Original SNLA428 and SNLS603D bytes, hashes and text derivations were preserved.
- WP1: explicit register-policy rows replace compact implicit classification;
  malformed quarantine reports stop admission; procedure-only addresses fail
  uniformly. Shared Check/FakeController helpers remove duplicated transport/MMD
  harnesses. Four negative checks are registered and execute under Release.
- WP2: fixed/frame/clock-lag bounds replace redundant component bounds; publication
  uses the raw post-return deadline check. Removed unused revision/status/next-page
  scaffolding. Forced 10 Mbit/s plus extended-full-duplex intent rejects locally.
  The API guide contains the revised operation barrier/restoration contract;
  operation machinery remains prompt 15. Native design selects public unmodified
  IDF v6.1 APIs with disclosed RX/HAL limitations rather than requiring an SDK fork.
- WP3: [ledger/source corrections](../prerequisite-wp3-ledger.md), including five
  source-implied EEE tuples, SNLA428's exact TDR recipe, forced-mode/reset policy,
  transient next-page ownership and all compliance patterns. Source interpretation
  is distinct from physical proof. Three stable EEE tuples permit raw reads;
  destructive EEE acquisition and feature initialization remain their typed stages.
- WP3–WP7: [future prompt corrections](../prerequisite-prompts-wp3-7.md), one feature
  matrix, exact family deltas, public SDK route, two runner artifacts, packaging
  and first-CI policy. Updated family/variant and board guides against their cited
  sources. Planning changes do not mark later implementation stages complete.

## Delivered behavior and evidence ownership

Existing `CommandTable.h` names all 97 register records and 283 field masks, with
per-field source/access citations. The ledger retains legal/prohibited encodings,
unknown reset masks, exact modes and recipe permissions. Structural tests require
exact catalog coverage and canonical raw-access classification; original-table
literals independently test numeric behavior. All-reserved words, selectors,
procedure-only actions and unresolved mixed semantics cannot bypass policy.

`acquireEvidence` explicitly consumes selected sources in dependency order, with
PHYSTS first when requested. Eleven current sources include FLDS, BMSR, ANER,
MISR1/2, FCSCR, RECR, RCSR, 10BTSCR and RXFS. RXFS always has complete MMD selection.
Whole-request frame admission precedes I/O; batches share the original deadline
and remaining count. Ordinary aggregate output changes only on total success.
Every valid consumed prefix is retained before a later batch or public return.

Each cached sample retains raw data, validity/staleness, timestamp validity,
sample count, observed-one/zero history and sticky history loss. Counter samples
also retain saturating raw arithmetic; RECR width and physical counter totals
remain unqualified. Failed destructive reads record possible lost history, never
zero events. Independently valid words survive release/time/report failure;
invalid timestamps do not become valid observations. Raw/external mutation makes
configuration evidence dirty and cached observations stale. Snapshots/invalidation
perform no callback. End/rebind clears binding-local evidence after the owner has
copied what it needs.

`modifyInterruptEnables` performs one owned two-frame MISR read/update and retains
the consumed event byte even on write failure. `modifyWakeControl` compares stable
state, harvests MISR2/RXFS before mutation, then uses one atomic five-frame RXFCFG1
RMW. Its worst-case admission is 14 frames; unchanged unrelated settings use four.
W0C bit 11 writes one for neutral preservation and zero for acknowledgement.
Reserved/undefined fields remain preserved; unsupported bit-mask semantics cannot
be set. The owner quiesces traffic and excludes external configuration writers;
an event after preharvest can race explicit acknowledgement. This is not lossless
hardware storage or complete WoL/IRQ feature support.

Pure source-specific compositions cover current neutral rules and BMCR command
suppression using desired power-down intent. No invented W1C behavior or generic
zero-command policy is introduced. Reset/BIST/CDCR commands remain later exact
procedures. Five-frame indirect RMW is inferred from 8.3.11.1 state retention,
not a printed combined sequence; physical Q4 remains NOT RUN.

## Independent source, peer and adversarial reviews

`access13_sources` used gpt-6-astra with high reasoning for destructive semantics,
contradictory original tables and independent literals. It reopened SNLS647I
original pages 54–56, 59, 63–64, 66–73 and 91–92; its current TI fetch still
identified revision I. It independently established MISR event clearing, PHYSTS
dependencies, FLDS/10BTSCR handling, W0C neutral/ack values and LED field placement.
Its writable scope was contract drafts and then `test/access.cpp`; the lead merged
shared contracts and implemented production code. It never used production masks
as the expected-value oracle.

`access13_peer` used the current capable model for fresh PCA9555/INA228 review,
WP0/WP3 ledger work and final read-only production review. Exact clean revisions,
headers, implementations, primary/cleanup tests, cached/live CLI, Python runners
and agent/workflow paths are recorded in WP0. Adopted immediate retained evidence,
explicit cached display and primary-error preservation; rejected I2C ACK/byte
accounting, pointer parking and sensor-specific timing/polarity as MDIO evidence.

`prerequisite_prompts` used the current capable model for disjoint future-prompt
corrections, shared test-harness factoring and later-family source reconciliation.
Original family PDF tables were reopened; support is not claimed from that work.
Separate build directories prevented test-build interference. No subagent edited
sibling repositories or Git. The lead reviewed actual source, tests and diffs.

Confirmed review findings were fixed and regression-tested:

- Counter retention originally lacked raw arithmetic across samples; added sums
  without asserting an unproved RECR width or physical event total.
- A failed read with explicit no-effect evidence incorrectly implied history
  loss; it now marks only stale state.
- A contradictory completed-but-not-attempted receipt could hide consumed history
  or a write effect. Report normalization still rejects it, while conservative
  history-loss/configuration invalidation now survives the malformed flags.
- Clarified cached lastStatus scope and corrected stale catalog/source wording.

## Verification and documentation

Environment: Windows PowerShell, GNU MinGW-w64 15.1.0 x86-64, CMake 4.0.1,
Python 3.12.10, Doxygen 1.13.2. Release uses `NDEBUG` with explicit active CHECKs
and strict conversion/sign warnings as errors. Registered suites have finite
timeouts. Reproduction commands from the repository root:

```sh
cmake -S . -B build/access13-release -DDP8382X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/access13-release
ctest --test-dir build/access13-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/access13-consumer -DCMAKE_BUILD_TYPE=Release
cmake --build build/access13-consumer
ctest --test-dir build/access13-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/verify_references.py
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

| Evidence layer | Result and scope |
| --- | --- |
| Compiled portable host | PASS: 27 CTests, including nine access suites and four deliberate-failure probes. Literal neutral values, all destructive phases, partial multi-batch capture, accepted writes followed by error, malformed reports, deadline/release, exact counts, no retries, sentinels and cross-owner RMW exclusion. |
| Portable build-tree consumer | PASS: tests OFF, real linked driver and direct/MMD/evidence examples; explicit simulated backend, no SDK/Python/Git build dependency. |
| Python research/tool/policy | PASS: 102 tests; 80 ledger, seven timing-design, two policy/catalog and 13 version-tool checks. |
| Ledger | PASS: 97 register records, 283 fields, 52 procedures, 35 feature families. Structure/source coverage does not prove physical behavior. |
| Reference/link integrity | PASS: 29 preserved artifacts, 46,886,437 bytes and 27 text extracts; local links checked after final integration. |
| Public documentation | PASS: 82 Doxygen HTML pages, local assets/links and two rejected negative fixtures. |
| Version and whitespace | PASS: generated header current; diff check clean. Version unchanged. |
| Installed/exported package | NOT RUN: later package stages, not supplied by this build-tree consumer. |
| SDK/controller image | NOT RUN: native implementation remains prompts 37–39. |
| Remote CI | NOT RUN: intentionally reserved for prompt 71. |
| Physical qualification | NOT RUN: no fixture operated; W0C/Q3, RMW/Q4, EEE/Q1, GPIO/RC/RECR and other documented physical gates remain explicit. |

Updated public Doxygen, direct/side-effect guides, API/native/architecture
contracts, source/feature/variant guides, test instructions, portable example,
README, CHANGELOG and progress. Version-tool commands need no behavior change;
no CLI/SDK/package implementation exists to document as supported. No ledger
procedure has primary implementationStage 13; this stage closes the stated access
foundation while full counters/IRQ/WoL procedures keep their later owners.

Final maintainability review keeps one source file for the cohesive transport and
access responsibility, explicit register rows, small source-specific helpers and
fixed storage. The independent destructive fake remains separate from the shared
protocol fake because it is an independent side-effect oracle. No generic plan
engine or generated register framework was added. Source conflicts and physical
limits remain named; they do not turn completed host software into hardware proof.
