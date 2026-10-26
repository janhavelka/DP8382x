# 22 - Close protocol, ownership and portability test gaps

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-21.

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
