# 06 - Implement bounded MDIO and register transactions

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-05.

## Fresh audit and owned work

Revisit PCA9555 transport normalization/write uncertainty, MB85RC callback accounting,
TMP1x2 timing contracts and their failure tests. Adapt transaction granularity
explicitly.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

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
