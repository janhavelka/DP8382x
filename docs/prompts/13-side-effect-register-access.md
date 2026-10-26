# 13 - Implement side-effect-aware register access

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [12](12-atomic-mmd.md), [4](4-silicon-diagnostic-ledger.md),
[5](5-silicon-power-interface-ledger.md). Verify the concrete inputs below; retain
unrelated open gates under COMMON.

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
Derive metadata from exact tables: MLEDCR bits 6:3 use `0x0078`; LEDCFG LED1/3
fields need independent per-pin vectors; LEDCFG2's reserved fields remain
protected. Treat `0x0F` as destructive FLDS and route it through the collector.
RXFCFG1 `0x04A0` bit `0x0800` is W0C; mirrored PHYSTS flags have clearing dependencies
on other register reads. Derive literal vectors from original tables, compare
the catalogue and maintain per-field source/access provenance. Re-audit current tables and physical evidence;
unresolved GPIO/RC semantics remain named gates, never guessed permissions.

## Documentation, verification and checkpoint

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
