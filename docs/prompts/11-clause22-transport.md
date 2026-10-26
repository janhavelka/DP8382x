# 11 - Implement bounded Clause 22 transport

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [6](6-framework-ownership.md), [7](7-value-transport-contracts.md),
[8](8-operation-contracts.md), [9](9-portable-source-foundation.md). Verify the concrete
inputs below; retain unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver a usable portable driver binding and direct Clause 22 reads/writes.
Own the existing Config/driver/register headers, corresponding source and focused
tests; add a source file only for a real responsibility.

Reopen PCA9555 `include/PCA9555/Config.h`,
`src/PCA9555.cpp::_mapTransportResult`, `_i2cWriteReadRaw` and
`_i2cWriteTracked`; compare MB85RC `src/MB85RC.cpp::_mapTransportResult`
and TMP1x2 `src/TMP1x2.cpp::read`, `write`, `track`. Inspect their
ambiguous-write, short-success and deadline tests and registration. Record which
byte/ACK rules are rejected for MDIO.

Implement the chosen terminal callback/transaction mechanism, `bind`,
`isBound`, local lifetime guards, `readClause22` and `writeClause22`.
Validate PHY/register addresses, readable/writable access and inputs before any
callback. Publish ordinary outputs only on complete terminal success. The bound
driver may retain its configured context, which must outlive every callback.
Per-call buffers remain borrowed only until the synchronous callback returns;
the transport neither retains those buffers nor re-enters the driver. Binding and
cached access are bus-silent; reject teardown that would abandon live users.

Account for lock acquisition and controller execution within the aggregate
allowance; measuring an overrun after return cannot prove bounded execution.
Normalize illegal/nonterminal callback statuses and impossible completion counts,
preserving full signed backend detail. Count actual attempted frames/transactions
at this boundary, separately from managed health. Failed or late writes retain
possible hardware effects without retry or cache rollback. Release ownership on
every exit according to the frozen transport contract. If release is fallible,
retain its ownership/effect evidence and block affected transactions until the
application resolves ownership; preserve the primary error separately.

Restrict this unit to source-proved safe Clause 22 access. Treat REGCR/ADDAR as
internal transaction primitives, not independent public writes that can establish
or use an unlocked selector session. Their public route is task 12's complete
owned MMD operation. Destructive reads and mixed command/status mutations needing
the collector are rejected until task 13;
do not ship placeholder MMD calls or advertise their support. This is a complete
direct-access subset, not a successful shell for later operations.

Test every address bound, local zero-I/O rejection, terminal-status normalization,
short completion, unchanged sentinel outputs, accepted-write-then-error,
deadline-before-lock and contention. Test release failure only when exposed by
the selected contract, including subsequent admission; otherwise test the
backend's release-on-return guarantee. Prove no automatic retries,
exact frame counts, two-instance isolation and no SDK dependency with a linked
portable consumer. Test driver copy/move restrictions and two owners attempting
selector/data access around one transaction. Document the callback contract and a compiling direct-read
example that checks Status before using output.

## Concrete TI source inputs

Use the [TI core transport audit](../reference/library-ti-core.md#transport-and-mmd-procedure-audit)
and [timing table](../reference/library-ti-core.md#timing-silicon-limits-versus-library-policy).
Validate at the public boundary before narrowing: Clause 22 register numbers
are five bits and data is 16 bits. Test rejection of register `0x8000` before
any backend can truncate it to BMCR, and check aggregate deadlines and staged
output publication independently of the transport callback. Keep protocol PHY-address range distinct from the part's
three strap bits. SNLS647I 7.6/8.3.11 and table 8-20 give MDC 2.5 MHz nominal,
24 MHz maximum, a 500 ns minimum suppressed-preamble transaction gap plus the
specified MDC/MDIO idle edge, and preamble/readiness requirements. Carry these
into backend evidence; host callbacks cannot prove physical bus timing.
Recheck current source/datasheet and correct discrepancies before coding; retain
NOT RUN hardware timing evidence until admitted measured traces exist.

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
