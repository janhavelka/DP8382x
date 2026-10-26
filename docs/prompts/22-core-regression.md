# 22 - Close protocol, ownership and portability test gaps

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-21.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

## Fresh audit and owned work

Inspect current peer fault injection, native source registration, core timing/allocation
guards and CLI negative tests. Review both tests and the production functions they
actually exercise.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Reconcile the full public-source register/field/procedure ledger against actual
symbols, typed operations, CLI and tests. Check for omissions hidden by a smaller
reference-driver map, a runtime-optional label or the first board's capabilities.
Add independently sourced coverage checks; self-consistent production metadata
does not prove correct field extraction or silicon behavior.

Review implemented primary behavior against every requirement in ENGINEERING, API
contracts and the source/feature matrix. Add missing meaningful regressions now; earlier
stages already own their focused tests. The goal is cross-cutting confidence, not
repeating implementation expressions as assertions.

Exercise all MDIO failure boundaries, lock contention and release, illegal interleaving,
partial write effects, reset/config trust, negotiation/latch handling and optional
diagnostics. Test one active owner plus IDF/base/raw/CLI entrants; no path may bypass
arbitration. Confirm zero budget has no I/O and atomic sequences respect their
documented scheduling unit.

Use independent register vectors, both optimized and debug test builds, applicable
sanitizers/strict warnings and public-header standalone compilation. Prove no core
hot-path allocation or hidden platform timing, and no framework includes through
transitive headers. Avoid broad text regexes as the only proof.

Review operation deadlines, quantized/frozen/wrapping clocks, callback completion
timing, pending results, cancellations and repeated lifecycle reuse. Stress CLI queue
limits and parser/runner false positives. Validate registered suite counts and whether
asserts survive Release. Keep reproducible failures as regression tests and remove
accidental scaffolding.

## Concrete regression closure matrix

Apply [CONVENTIONS](CONVENTIONS.md). Compare
`TMP1x2/CMakeLists.txt` explicit source/test lists and
`test/core_no_arduino/compile_main.cpp` public-header boundary tests with
`24Cxx/CMakeLists.txt` host CLI executable and Python transcript registration.
Inspect production functions with their tests and confirm enabled assertions.
Peer workflow commands are local verification references; do not dispatch or
inspect remote runs here. Do not copy their I2C/S2/S3 firmware target matrix.

Build a table of each public operation/family with independently derived source
vectors, success behavior, failure boundary, state/output invariant, owner/frame
budget, test target and remaining silicon/SDK/fixture gate. Keep production
constants, feature-ledger checker and the independent literal expected values as
separate evidence. A matching generated register list is not a silicon oracle.

Cover at least these cross-module regressions using the real core and adapters
where their corresponding evidence layer permits it:

- Every frame of indirect access fails in turn, including an accepted address/
  data write followed by timeout. Reads keep caller output unchanged until valid
  completion; uncertain writes preserve untrusted configuration and no retry.
- An attempted base/raw/CLI/second-PHY entrant during each transaction boundary
  cannot interleave selector/data frames. Lock failure releases nothing it did
  not acquire; callback reentry is rejected without deadlock.
- Zero poll budget, exhausted aggregate deadline, frozen/quantized/wrapping
  clocks and illegal callback progress cannot create extra transfers or busy
  loops. Test exact timeout edges and separate primary/cleanup deadlines.
- Repeated init/end/bind, pending result, active cancellation and context teardown
  preserve ownership and visible residual effects. Output/result consumption
  must not overwrite a different operation's retained completion.
- Reset and 10BASE-Te restoration use independent expected ordering; failed
  stable-bit verification does not restore trusted configuration. Latch-low and
  read-clear events have one collector despite multiple cached consumers.
- Cable loss, negotiation pending, stale link, policy mismatch and backend fault
  produce different evidence. Successful unrelated reads never certify repaired
  configuration. Driver health excludes argument rejection and passive getters.
- Full/slow console, fragmented input and malformed automation records cannot
  produce success, drop completion silently or starve bounded owner cleanup.
  Python aggregate verdicts reject zero-case, skip-only and incomplete campaigns.

Organize tests by protocol/operations/health/CLI/runner responsibility within
the established `test/` layout; extend current fixtures before creating more
harnesses. Use readable case names and small arrange/action/assert sections, not
one enormous scripted test or a file/class per field. Keep sanitizer/compiler
support explicit; a missing tool is NOT RUN/BLOCKED for that layer. Prove the
test system fails on a deliberately failing assertion in a temporary validation
experiment, without committing broken code or leaving production fault toggles.

Run registered debug and optimized host suites with test counts, selected strict
warnings/sanitizers, standalone public-header consumers and the actual plain
consumer. Track allocation in representative core execution after caller setup;
source grep is supplementary. Rebuild actual SDK images when integration changes
affect them. Resolve confirmed defects in their owning module and update the
relevant focused regression, not just a broad text guard.

Update Doxygen for clarified behavior, `docs/api-contracts.md`/applicable maintained API
guide, ownership/time-bound guides, `test/README.md`, root README and CHANGELOG
for actual fixes. Record remaining limitations and exact local commands in the
stage report. Run established documentation checks for changed contracts. Assign
independent read-only protocol and ownership reviews when useful, with explicit
acceptance cases; select available agents for the reasoning required. The lead
integrates fixes and maintains the cross-module coverage table.

## Acceptance and handoff

Run the affected complete host/CLI/Python suites and plain portable consumer. Rebuild
SDK examples if public/adapter paths change. Record coverage by behavior rather than an
unsupported percentage. List open silicon interpretations or SDK bounds honestly and
block affected claims.

Expected output: Primary implementation regression evidence and closed ownership gaps.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
