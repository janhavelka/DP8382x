# 61 - Close complete DP83825I feature and integration coverage

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are the complete source ledger and verified core paths from
[60](60-p25-source-core.md), existing common feature procedures and affected
adapter/CLI/runner contracts. Physical DP83825I evidence is separate; missing
fixtures cannot excuse required software coverage.

## Current evidence and ownership

Reopen TMP1x2 variant capability/configuration tests, MB85RC metadata and actual
identity selection, plus peer diagnostic and runner restoration tests relevant
to the assigned features. Inspect public headers/source, current CLI/help,
Python fixtures, check registration and AGENTS. Record revisions, dirty state,
paths and adopted/adapted/rejected rules; return to exact SNLS638C and
applicable TI notes for the implemented sequences.

The lead owns shared declarations, capabilities and integration.

## Concrete source checks

Compare [Linux
IRQ/wake](../reference/library-family.md#interrupt-and-wake-facts-from-ti-linux)
with exact SNLS638C and the upstream Zephyr pin in the catalogue. Zephyr uses
PHYSCR `0x11[1:0]` for IRQ/global output and MISR `0x12[5]` link enable, and
reads status to clear it; this limited path cannot close all event sources. RCSR
`0x17[3:2]` FIFO overflow/underflow are RC: one collector retains samples even
if later diagnostics fail. Confirm bit 8 TX shift applies only to follower. Use
exact DP83825I TDR/EEE/compliance notes; DP83822's calibration and custom filter
map require independent applicability and cannot be inferred. Correct conflicts
before dependent implementation; missing fixtures remain physical gates.

## Exact access and mode corrections

Treat Register_12 `0x000C`, MISR1/2 `0x12/0x13` and FLDS `0x0F` as consuming,
collector-owned and excluded from ordinary `readable()`. Record R-versus-RC
prose conflicts and unknown Register_12 latch relation; collect IRQs through
MISR1/2 only. Test consumed first sample followed by failed second read and
prompt 60's RMII role/clock-loss admission.

Implement tables 6-5..6-7 functional modes as one finite typed procedure.
For `0x04D1`, sections 6.3.3.2/6.3.4 and table 6-7 (`0x00C2`, bits 0/3 clear)
govern over table 7-92's R marking/reversed bit 3 wording. Recipe addresses
`0x030E,0x0333,0x0218,0x031B,0x033A,0x033D` need exact allowlists citing those
tables, table 8-3 and SDAA081 2.3.3 against table 7-1's reserved statement.
SecureON `0x04A5..0x04A7` is absent from that map, but SNLA261A table 2-1/2.2.2
names DP83825; allowlist those addresses with the source gap and physical Q15.
Append exact-part rows to the single feature-coverage matrix.

## Work and acceptance

Implement the finite feature-family differences assigned by 60: every applicable
interrupt/event/counter behavior, loopback/BIST/PRBS/cable or other diagnostic,
PHY low-power/EEE/LPI/wake/filtering, LED/GPIO and documented
test/compliance/analog control. Validate shared inherited paths where source
proves equality; do not copy the driver or reimplement all common procedures. If
research reveals a substantial new feature family absent from the assigned list,
record and repair the task boundary explicitly instead of hiding unbounded work
in closure.

Reconcile every exact DP83825I register/field/procedure with legal values,
source citations, production symbol/typed operation, CLI and independent tests.
An upstream API omission or unavailable pin is not unsupported silicon. Give
hardware-only or exact-silicon-inapplicable rows cited dispositions; required
unimplemented or ambiguous software stays OPEN. Preserve one transaction owner,
one destructive-status collector and one CLI grammar. Keep capability getters,
adapter admission, command outcomes and runner suite selection consistent.

Register source-independent literal vectors for all changed fields/procedures
and inherited applicability. Exercise every legal mode and boundary, invalid
masks/enums, inaccessible registers, unsupported combinations with zero
preflight callbacks, no-link versus transport failure and cached bus-silent
state. Apply VERIFICATION's transfer, partial-acquisition, reset,
deadline/cancel and cleanup cases to changed and inherited DP83825I paths. Check
exact reserved/neutral write values and retained read-clear evidence.

Close exact-I software support only with all required rows implemented/tested;
do not imply undocumented S/CM/CS support. Run earlier-part and focused DP83825I
host regressions, CLI/Python/Doxygen checks and real affected SDK links; recheck
packages for changed exports. Update variants/API/feature/adapter and RMII
fixture guidance. Record software closure and separate physical gates, then
complete COMMON's report and checkpoint.
