# 12 - Implement complete atomic MMD transactions

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [11](11-clause22-transport.md), [3](3-silicon-core-ledger.md),
[6](6-framework-ownership.md). Verify the concrete inputs below; retain unrelated open
gates under COMMON.

## Scope, current evidence and work

Deliver `readMmd` and `writeMmd` through the exact DP83826I Clause
22 indirect mapping, including safe stable-field read/modify/write. Keep one
register procedure implementation; no native Clause 45 claim or alternate
transaction mechanism.

Reopen PCA9555 transport/cleanup wrappers and failure tests,
MB85RC callback completion accounting, and TMP1x2 timing contracts/source.
Inspect SNLS647I REGCR/ADDAR tables and independently derive literal read/write
traces; production masks are not their own test oracle.

Validate supported device selectors, address holes, operation and masks before
I/O. A manufacturer procedure-only address authorizes its named recipe, not
arbitrary raw writes. One application-controlled ownership scope covers selector,
address and data frames and the full stable-field RMW. Every participant,
including other PHY addresses and future SDK/CLI paths, must honor it. Distinguish
acquiring ownership from execution inside an existing transaction so mediator
callbacks cannot recursively lock.

Pass the remaining aggregate allowance to each terminal callback. A cooperative
budget either admits the entire transaction or yields before starting; it never
releases an indirect sequence halfway through. Release before any later scheduled
wait. Keep frame attempts, logical outcome, selector effects and target mutation
distinct. Partial failure leaves selector uncertainty; re-establish the complete
address sequence on the next explicit operation. A later read cannot retrospectively
validate a previous ambiguous target write. Apply one canonical access policy to
Clause 22 registers and their documented vendor-MMD aliases. Exclude REGCR/ADDAR
from indirect addressing as SNLS647I 8.3.11.1 specifies; aliases cannot bypass
reserved-field, destructive-read or mutation ownership checks.

Use fixed storage and staged ordinary read output. Preserve primary/release
statuses and accepted effects. Generic RMW is limited to stable readable writable
fields; mixed/destructive access remains task 13's source-specific procedure.
No automatic retry, nested lock, per-PHY lock registry or retained caller buffer.

Inject failure at every selector/address/data phase, including writes accepted
before error, then prove a subsequent call re-addresses fully. Attempt competing
access between every frame and across two PHY addresses; demonstrate exclusion
for both MMD and RMW. Test zero budget, minimum remaining allowance, deadline
between frames, invalid statuses/counts, sentinel output, all exit releases and
exact counts. An admitted indivisible sequence must never be divided by a poll
budget. Add a complete trace and portable consumer example to the transport guide;
keep actual SDK/controller timing separate from host protocol evidence.

## Concrete TI source inputs

Reopen the [pinned TI MMD procedures](../reference/library-ti-core.md#transport-and-mmd-procedure-audit)
and [SDK comparison](../reference/library-sdk.md) against current source and
SNLS647I 8.3.11.1-.5/tables 8-31/32. Vendor DEVAD31 read is literally writes
`0x0D=0x001F`, `0x0E=address`, `0x0D=0x401F`, then read `0x0E`; a write replaces
the last read with target data. Keep REGCR reserved bits 13:5 zero, including
when SDK bitfields contain poisoned/uninitialized storage. Keep EEE DEVAD3/7 wire offsets gated until their source mapping is established.
Check every RMW frame result and stop after the first failed phase; prove stale
addressing cannot produce a trusted target update. Select the minimum complete
source-backed RMW sequence and test its actual frame count, including any
required re-selection. Compare
real admitted MDIO traces later; mock success cannot establish wire correctness.

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
