# Prompt 8: fresh independent contract audit

Audited 2026-10-29 on `promptset/dp8382x-implementation-20261028`, starting
clean and fetched at `2b570bb5dcf52b2eacbf97055babb8d17f944fbd`, equal to its
same-name origin upstream. Original base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`; configured origin is
`https://github.com/janhavelka/DP8382x.git`. The final handoff records the
corrective commit and verified sync without a self-referencing commit loop.

## Scope and independent review

Re-read the complete on-disk prompt 8, including its explicit broken-clock /
owed-cleanup dependency, COMMON, ENGINEERING, CONVENTIONS and relevant
EVIDENCE/VERIFICATION rules. Audited actual declarations, examples, scenarios,
timing probes and the `382462c..2b570bb` diff, then reviewed the corrective diff.
Prompt 8 remains a design deliverable: no production operation engine, future
facade, SDK stub or successful hardware placeholder was added. Shared API/docs
and Git remained lead-owned; all agents and sibling references were read-only.

Parallel reviewer assignments and rationale:

| Reviewer | Model / scope |
| --- | --- |
| `p8_audit_lifetime` | gpt-6-astra/xhigh for independent state, ownership, deadlines, quiescence and retained-result reasoning; reviewed corrections to closure. |
| `p8_audit_time` | gpt-6-astra/high for SDK/silicon timing, budget arithmetic, callback duration and evidence limits. |
| `p8_audit_coverage` | gpt-6.1-sol/high for requirements, vocabulary, examples, simplicity and consumers 15/16/22; independently compiled corrected owner types. |

## Confirmed findings and corrections

1. Invalidation did not distinguish active work from an already terminal result.
   Its unconditional wording could overwrite a successful primary or restart
   failed/discharged cleanup. The contract now defines all four states: IDLE
   creates no operation; RUNNING freezes invalidation and preserves obligations;
   CLEANUP rechecks only its remaining safe route within original allowances;
   RESULT_PENDING retains its historical receipt and never reopens work. Current
   device snapshots become invalid independently. New external effects remain
   the notifying owner's responsibility. Added explicit acceptance scenarios
   for invalidation during cleanup and after success, failure or discharge.
2. Both design owner structs were implicitly copyable/movable. The plain owner
   also accepted an arbitrary PHY argument alongside its token, allowing equal
   token numbers on two instances to target the wrong object. Both owners now
   delete copy/move operations. OwnerSession retains its exact core reference;
   its helpers use that reference and cannot select another instance. Copied
   snapshots/results remain ordinary publication values.
3. The native callback delivered a full terminal result but could leave
   nativeStatus holding an earlier OK/IN_PROGRESS. It now stores the selected
   terminal primary/cleanup status before mapping and returning the native error.

The lead verified each finding in the actual contract/example and reviewed all
edits. Independent lifetime and coverage closure found no remaining blocking
defect. No extra lifecycle, retry mechanism, generic registry or operation model
was needed. The deadline/clock design, source gates and selected owner boundary
remain intact.

## Fresh source checks

The four relevant sibling checkouts are still clean at the original stage's
recorded revisions:

| Reference | Revision / reopened paths relative to Projects |
| --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9`; `include/TMP1x2/TMP1x2.h`, `src/TMP1x2Operations.cpp`, `src/TMP1x2.cpp`. Lead rechecked poll/cancel/take at lines 192–250. |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a`; `include/PCA9555/PCA9555.h`, `src/PCA9555.cpp`. Lead rechecked terminal poll/take at lines 896–938. |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`; `src/MB85RC.cpp:1738–1789`, accepted/uncertain write and verification evidence. |
| FieldCore-node | `5ad9292dc8481cf08be9a61f662495a6474d1da0`; `src/devices/i2c/Ina228Module.cpp:551–598`, cached result and failed-take identity clearing. |

Adopt copied receipts and accepted/uncertain effects; preserve the deliberate
rejection of sibling result erasure, stale callback time and token loss after a
failed take. MDIO atomic batches, cleanup admission and passive OFFLINE semantics
retain their own contract. The [original report](8-operation-contracts.md) owns
the fuller unchanged source/test/example/runner inventory; this audit does not
claim to have rerun sibling test suites or re-reviewed every tool path.

The timing reviewer reopened the actual SDK BASE/MAC sources pinned at
`fff9895c82d744c7237be8847347bdd1b07c6643`; Git blobs still match
`7d5d407d029c63494dffaaa3d82f8701b6b73790` and
`6448fcd6de4669ddf1e836c992ca8a1accc9dd96`. Reset-before-budget, AN expiry,
power-bit verification, tick truncation and nominal frame loops remain accurately
described. Native callback shape matches `esp_eth_phy.h`. Original SNLS647I PDF
pages 15, 17 and 34 were visually rechecked for timing columns and bus-silence
requirements. No silicon fact, SDK baseline or vendor payload changed.

## Verification and documentation

Environment: Windows NT 10.0.26300.0, PowerShell 5.1.26100.9711,
Python 3.12.10, MinGW-w64 UCRT g++ 15.1.0.

| Evidence layer / command | Result |
| --- | --- |
| Host research/design: `python -m unittest discover -s test -p "test_*.py" -v` | PASS: 85 tests in 14.847 s, including eight timing probes. |
| `python tools/verify_ledger.py` | PASS: 96 registers, 52 procedures, 35 features; research structure/coverage. |
| `python tools/verify_references.py` | PASS: 27 artifacts (43,240,385 bytes), 25 extracts, 1,307 local links in 151 Markdown files. |
| Strict C++17 documentation syntax and ownership traits | PASS: all eight portable blocks plus the exact core-only NativeResetContext struct; 23 compile-time assertions. |
| `git diff --check` | PASS. |
| Host driver execution | NOT RUN: no working operation implementation in this stage. |
| Portable/package consumer | NOT RUN: documentation syntax is not a linked/exported driver consumer. |
| SDK compilation/link | NOT RUN: native callback source/signature review only. |
| Physical qualification | NOT RUN: no fixture action or timing measurement. |
| Remote CI | NOT RUN: dispatch/status/wait remains prompt 71; no workflow definitions were changed. |

The syntax probe extracts `cpp contract` and `cpp example` blocks, appends the
unchanged text of the NativeResetContext struct alone, and invokes
`g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsyntax-only -x c++ -`.
Assertions check core/owner noncopyability and nonmovability, owner construction
from a persistent core reference only, exact reference member types and copyable
publication values. No SDK declaration substitute or runtime implementation is
used. Native callback bodies remain source-reviewed, not host-linked.

The eight arithmetic tests are executable design vectors, not state-machine
tests. Invalidation, token and cleanup scenarios remain explicit future-driver
acceptance recipes; actual execution belongs to the first real consumer. No
fake operation implementation was added merely to make those recipes pass.

Updated API declaration comments, state/scenario tables, architecture, native
example plan, CHANGELOG, report index and progress together. Reviewed README,
design decisions, conventions, feature map, public-header and test guides: their
scope, ownership rules, counts and future implementation gates remain accurate,
so no edits were needed. Existing source gates and the prompt-37 SDK lifecycle
correction remain separate implementation dependencies; none blocks this audited
contract's completion.
