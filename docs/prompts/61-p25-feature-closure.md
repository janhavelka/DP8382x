# 61 - Close complete DP83825I feature and integration coverage

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are the complete source ledger
and verified core paths from [60](60-p25-source-core.md), existing common feature
procedures and affected adapter/CLI/runner contracts. Physical DP83825I evidence
is separate; missing fixtures cannot excuse required software coverage.

## Current evidence and ownership

Reopen TMP1x2 variant capability/configuration tests, MB85RC metadata and actual
identity selection, plus peer diagnostic and runner restoration tests relevant
to the assigned features. Inspect public headers/source, current CLI/help,
Python fixtures, check registration and AGENTS. Record revisions, dirty state,
paths and adopted/adapted/rejected rules; return to exact SNLS638C and applicable
TI notes for the implemented sequences.

The lead owns shared declarations, capabilities and integration. A useful
delegation is a disjoint feature test section or read-only source-to-API coverage
review after signatures freeze. Assign explicit writable/forbidden files,
independent failure vectors and available models by task complexity; review all
contributions. Agents do not mutate Git or sibling sources.

## Concrete source checks

Compare [Linux IRQ/wake](../reference/library-family.md#interrupt-and-wake-facts-from-ti-linux)
with exact SNLS638C and the upstream Zephyr pin in the catalogue. Zephyr uses
PHYSCR `0x11[1:0]` for IRQ/global output and MISR `0x12[5]` link enable, and
reads status to clear it; this limited path cannot close all event sources.
RCSR `0x17[3:2]` FIFO overflow/underflow are RC: one collector retains samples
even if later diagnostics fail. Confirm bit 8 TX shift applies only to follower.
Use exact DP83825I TDR/EEE/compliance notes; DP83822's calibration and custom
filter map require independent applicability and cannot be inferred. Correct
conflicts before dependent implementation; missing fixtures remain physical gates.

## Work and acceptance

Implement the finite feature-family differences assigned by 60: every applicable
interrupt/event/counter behavior, loopback/BIST/PRBS/cable or other diagnostic,
PHY low-power/EEE/LPI/wake/filtering, LED/GPIO and documented test/compliance/
analog control. Validate shared inherited paths where source proves equality;
do not copy the driver or reimplement all common procedures. If research reveals
a substantial new feature family absent from the assigned list, record and
repair the task boundary explicitly instead of hiding unbounded work in closure.

Reconcile every exact DP83825I register/field/procedure with legal values,
source citations, production symbol/typed operation, CLI and independent tests.
An upstream API omission or unavailable pin is not unsupported silicon. Give
hardware-only or exact-silicon-inapplicable rows cited dispositions; required
unimplemented or ambiguous software stays OPEN. Preserve one transaction owner,
one destructive-status collector and one CLI grammar. Keep capability getters,
adapter admission, command outcomes and runner suite selection consistent.

Register source-independent literal vectors for all changed fields/procedures
and inherited applicability. Exercise every legal mode and boundary, invalid
masks/enums, inaccessible registers, unsupported combinations with zero preflight
callbacks, no-link versus transport failure and cached bus-silent state. Inject
every changed read/write/cleanup failure, hardware effect followed by error,
partial acquisition, reset mid-operation, cancellation and aggregate/cleanup
deadline exhaustion. Verify output sentinels, complete traces, reserved/neutral
write values, retained consumed events, configuration uncertainty and preserved
backend/primary/cleanup detail. Restore only owned settings with a finite budget.

Close exact-I software support only with all required rows implemented/tested;
do not imply undocumented S/CM/CS support. Run earlier-part and focused DP83825I
host regressions, CLI/Python/Doxygen checks and actual affected SDK links.
Recheck packages only when exported inputs change. Ship public/source Doxygen
and README/CHANGELOG/variants/API/feature/adapter/RMII fixture guides together,
recording no-change reasons for inspected unaffected guides. Record commands,
counts, source audit and separate host/SDK/physical evidence in report, validation
and progress. Check final diff against entry rules; commit/sync verified units
and prompt completion on the execution branch. Remote CI belongs only to 71.
