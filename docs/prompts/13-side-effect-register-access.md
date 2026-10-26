# 13 - Implement side-effect-aware register access

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Use [SOURCE_MAP](SOURCE_MAP.md) for current
source locations; historical reports do not replace a fresh audit.

At entry record owned paths, applicable rules, current source revisions/dirty
state, selected names, one MDIO owner, local checks and the recorded execution
branch. Resume that branch. Use disjoint implementation/test work or independent
read-only review when useful; choose available models/effort by task complexity
and record the rationale. The lead owns shared contracts, integration and Git.
Siblings remain read-only.

Prerequisites: [12](12-atomic-mmd.md), [4](4-silicon-diagnostic-ledger.md), [5](5-silicon-power-interface-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Complete the source-backed register/mask/access catalog and safe procedures for
mixed stable/command/status fields. Establish the single destructive-evidence
collector that later link, IRQ and diagnostic features extend. Own existing
register/driver source and focused tests; no generic register-manager framework.

Reopen PCA9555 `_i2cWriteCleanupTracked`, shadow invalidation and
primary/cleanup tests; inspect INA228 `DiagAlert`/`readDiagAlert`
declarations, implementation and tests. Read actual sibling CLI rendering to
distinguish cached display from live destructive reads. Recheck the exact
SNLS647I access tables and the authoritative ledger.

Cover all documented non-reserved register fields with named addresses, masks,
legal encodings and access attributes; independently justified procedure-only
addresses remain an exact-sequence allowlist. Unknown reset bits remain unknown.
Define safe-dump reads and explicit destructive acquisition. Raw access cannot
bypass addressing, ownership, maintenance or invalidation rules.

Implement source-specific composition for W0C/W1C/W1S, self-clearing commands,
read-clear fields and reserved bits. There is no universal zero-all-command-bits
policy. Preserve consumed status before updating owned stable controls and use
each command's documented neutral value. In RXFCFG1, bit 11 is W0C under
SNLS647I table 8-90: unrelated updates preserve pending WoL and an intentional
zero acknowledges it. Do not import SNLA261A's DP83822 acknowledgement recipe.

Retain every successfully consumed event/counter sample immediately with source
validity/time, even if a later read fails. Keep aggregate output unchanged or
explicitly stale on incomplete acquisition. A destructive read that may have
cleared hardware without returning valid data records possible history loss,
not zero events. Avoid a public collector API without a current consumer: safe
destructive acquisition and mixed-register mutation are its consumers now.
Raw/external writes invalidate appropriate configuration evidence.

Test literal neutral compositions for every used access type, pending WoL during
unrelated updates, explicit W0C acknowledgement, stable-bit preservation and
illegal/reserved requests before I/O. Fail each phase, including destructive
read-after-effect timeout and partial multi-source acquisition. Assert retained
samples/history-loss flags, aggregate sentinel output, exact counts, no retries
and full RMW exclusion. Test cached readers do no I/O. Document the access table,
safe dump limits and partial-evidence contract; typed feature completion remains
with the later feature's full procedure.

## Concrete TI source inputs

Use the [complete TI header comparison](../reference/library-ti-core.md#complete-dp83826-private-header-field-inventory)
and [destructive-read table](../reference/library-ti-core.md#destructive-reads-counters-and-safe-diagnostic-output).
Correct source-derived metadata against exact tables: TI's MLEDCR mask `0x0038`
must not truncate bits 6:3 (`0x0078`); LEDCFG LED1/LED3 fields are swapped in its
header, and its LEDCFG2 LED3 controls name datasheet-reserved bits. Generic
`GIGESR=0x0F` is this chip's destructive FLDS, not gigabit status. RXFCFG1
`0x04A0` bit `0x0800` is W0C; mirrored PHYSTS flags have clearing dependencies
on other register reads. Use literal vectors from the catalogue and maintain
per-field source/access provenance. Re-audit current tables and physical evidence;
unresolved GPIO/RC semantics remain named gates, never guessed permissions.

## Documentation, verification and checkpoint

Ship accurate public Doxygen and procedure citations with this behavior: units,
legal states, ownership, output validity, I/O effects, deadlines and cleanup.
Update API contracts, affected guides, restoration/feature evidence and the stage
report/progress. Review README, CHANGELOG and relevant example/test/tool guides;
update affected claims and record why others need no change. Future APIs and
physical qualification remain explicitly unimplemented/unverified.

Run registered focused host tests with active Release checks, relevant portable
consumer/source-documentation/link checks, the reference checker and
`git diff --check`. Record actual commands and separate PASS/FAIL/NOT RUN/BLOCKED
for host, SDK, packages and hardware where relevant. Missing source evidence
blocks the affected requirement; it cannot become unsupported silicon or close
a required gate. Continue independent eligible work.

Review the final diff for simple control flow, stale comments, duplicated
procedures and undocumented errors. Commit and sync checked tests/docs/code on
the same execution branch after this task and each coherent verified unit.
The lead fetches, safely integrates, pushes and verifies upstream equality;
report exact sync failures. First-execution remote CI dispatch, polling and
waiting belong only to checkpoint **71**, never this task.
