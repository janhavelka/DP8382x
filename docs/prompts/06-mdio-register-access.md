# 06 - Implement bounded MDIO and register transactions

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-05.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Revisit PCA9555 transport normalization/write uncertainty, MB85RC callback accounting,
TMP1x2 timing contracts and their failure tests. Adapt transaction granularity
explicitly.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen
`PCA9555/include/PCA9555/Config.h` and
`PCA9555/src/PCA9555.cpp::_mapTransportResult`, `_i2cWriteReadRaw`,
`_i2cWriteTracked`, `_i2cWriteCleanupTracked`; compare
`MB85RC/src/MB85RC.cpp::_mapTransportResult` and
`TMP1x2/src/TMP1x2.cpp::read`, `write`, `track`. The first two preserve explicit
write effects/counts; TMP1x2 normalizes terminal Status and stages read output.
Inspect short-success, ambiguous-write and deadline tests in
`PCA9555/test/test_basic.cpp`. Adapt byte/ACK counts to MDIO frames and whole
transactions; reject I2C NACK classifications and per-frame ownership.

Extend stage-05 public Config, driver and register headers under
`include/DP8382x/`. Keep the register-header name selected in 04, such as
`CommandTable.h`, rather than adding a second catalog. Put transport/register
behavior in one cohesive source, for example `src/DP8382xRegisters.cpp`, only
when that responsibility warrants separation. A small access table needs an
actual validator consumer; do not add a manager or class per register. Register
the source and real tests in the existing CMake targets.

Use the planned spellings `readClause22`, `writeClause22`, `readMmd`, `writeMmd`
and stage-04 signatures/effect types. Avoid duplicate readReg/readRegister aliases.
Private helpers distinguish acquiring ownership from running inside an existing
transaction, preventing recursive mediator locks. Use the shared `Status`,
`errorName`, static messages and full signed backend `detail`.

## Decisions and independent test oracles

1. Make an access table for PHY address, register space, legal selectors, allowed
   mask, destructive reads, write effects and owner. TI evidence governs each
   row. Procedure-only addresses permit the exact named recipe, not arbitrary
   writes. Distinguish controller-native Clause 45 from Clause 22-mediated MMD.
2. Trace admission, lock acquisition, selector/address/data frames, publication
   and release. Lock failure performs no MDIO. Each callback receives the
   remaining aggregate allowance. A cooperative budget admits the whole indirect
   transaction or yields before starting; it cannot release ownership mid-sequence.
3. Normalize invalid codes, impossible completion counts and nonterminal callback
   results. Preserve actual attempts and conservative accepted-write effects on
   failure. A successful MDIO controller callback is not an I2C presence ACK.
4. Distinguish selector changes from target mutation without discarding selector
   uncertainty. Re-establish full addressing on subsequent attempts. Later read
   success cannot retrospectively validate an ambiguous configuration write.
5. RMW applies only to stable readable writable fields. Commands, reset, W1C and
   read-clear bits need explicit procedures. Define external-writer invalidation
   and primary versus release/cleanup error precedence under the owner contract.

Use independently derived literal PDF traces for both read and write. Fail each
selector/address/data phase, including failure after a write took effect, then
run another operation to prove fresh addressing. Test short success, invalid
result code, deadline before lock and between frames, sentinel output and release
on every exit. A competing fake owner attempts access between every frame;
assert whole MMD and RMW exclusion across two PHY addresses. Check zero budget,
minimum remaining time, exact operation/transaction/frame counts and no retries.

## Documentation shipped in this stage

Doxygen callbacks, methods and effect/result types with units, borrowed lifetime,
terminal-only behavior, lock/controller bounds, address spaces, read side effects
and output-on-error. Cite TI beside selector constants and non-obvious sequences.
Update `docs/api-contracts.md` and the transport guide with a complete indirect
trace and callback requirements. Update README's implemented scope, CHANGELOG,
ledger and validation. Run registered Doxygen generation/warning/link checks;
comment grep is insufficient. SDK and physical timing remain separate evidence.
Keep the execution branch and coherent-unit/per-prompt checkpoints. For this
first set no remote CI dispatch, polling or waiting belongs to this stage.

## Work

Cover all authoritative registers/non-reserved fields in the ledger with named
addresses/masks/encodings and access semantics, rather than the smaller set in
a reference driver's private header. Keep exact manufacturer-documented
procedure-only addresses/values in a separately justified sequence allowlist;
the absence of a summary-table row neither grants arbitrary access nor invalidates
an otherwise documented EEE/power/loopback recipe. Resolve source conflicts first.

Implement the selected borrowed MDIO transport contract and exact DP83826I register
access. Validate address, Clause 22 register, supported MMD mapping, masks and input
buffers before I/O. Use fixed storage and publish read results only after terminal
success. Preserve backend error detail and distinguish local rejection from a transfer.

Implement one complete indirect transaction path, sharing or delegating IDF helpers
later according to stage 03. Include all selector/address/data phases under a single
application-controlled transaction scope and aggregate deadline. Read/modify/write
atomicity, lock failure and release rules apply across every owner. Do not expose native
Clause 45 support merely because an address is called MMD.

Install precise attempt/effect accounting at the actual callback boundary now; later
health views consume it. Separate logical operations from MDIO frames and tracked vs
diagnostic traffic. Record ambiguous write effects even when the callback fails. No
retries or guessed rollback. Preserve selector uncertainty after partial indirect
operations and explicitly re-establish addressing on subsequent transactions.

Use stage 02 access attributes for safe composition: reserved bits, command bits,
read-clear and W1C require appropriate paths. Reject unsupported registers/operations
rather than filling generic all-register arrays. Add source citations and transport
contract documentation.

## Acceptance and handoff

Script independent exact sequences; inject failures at every phase, including writes
accepted before error. Test all address bounds, illegal callback statuses, unchanged
outputs, deadline exhaustion, contention and release on every exit. Assert complete
indirect/RMW atomicity and no extra transfers. Run focused registered host tests.

Expected output: Actual portable MDIO/register implementation with atomicity and fault
evidence. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
