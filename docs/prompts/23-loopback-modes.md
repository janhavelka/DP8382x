# 23 - Implement all supported loopback modes

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [22](22-reconcile-recover.md), [4](4-silicon-diagnostic-ledger.md),
[20](20-event-counter-acquisition.md). Verify the concrete inputs below; retain
unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver complete exact-part loopback configuration and finite owner-driven
sessions, including stop and verified restoration. Persistent configured
loopback and a finite diagnostic session need distinct semantics. A persistent
setting remains explicitly owned until disabled; its lifetime is not an unfinished
operation token. A finite session must include disable and verified restoration. Extend the
existing operation/result mechanism and cohesive diagnostic source; no new
scheduler or autonomous maintenance worker.

Revalidate the [TI loopback facts](../reference/library-ti-features.md#loopback-and-bist-anchors)
against SNLS647I tables 8-19/8-40 and current source. BISCR `0x0016[4:0]`
uses `01/02/04/08/10` for PCS input/output, digital, analog and reverse;
PCS input is 10 Mbit/s only, digital 100 Mbit/s only, and analog requires 100-ohm
termination. Include the complete 100 Mbit/s digital sequence
`0000=2100,0016=0104,0122=2000,0123=2000,0130=47FF,001F=4000`
as a sourced literal example with owned-state restoration, not general whole-word
writes. Use reverse-loopback selection `0x10` under table 8-40 and apply BISCR
bit 6 line transmission only to 100 Mbit/s MII loopback. Test the one-hot mode
encodings independently. Track `0x0123` overlap with EEE explicitly.

Reopen PCA9555 owner cleanup/result types and
`test_failed_cooperative_input_read_preserves_primary_and_cleanup_evidence`.
Inspect TMP1x2 `examples/common/Tmp1x2CliDiagnostics.cpp` restoration/
result handling and `tools/test_hil_tmp1x2_runner.py` cleanup cases.
These supply ownership precedents, not PHY loopback recipes.

Complete the source matrix for MII, PCS input/output, digital, analog,
reverse/far-end and every other documented loopback mode, with speed/interface,
companion writes, termination/peer prerequisites and restore effects. Procedure-only
addresses permit exact sourced recipes. For each touched procedure-only field,
prove readable/saveable state or use a documented reset/reapply exit; an allowlisted
write alone does not establish that an arbitrary previous value can be restored.
Generator/checker sessions are task 24; loopback itself must work completely now.

Resolve the SNLS647I speed-specific discrepancy: section 8.3.14.2 gives the
general BMCR.14/BISCR.2 recipe, while table 8-19 specifies
`BISCR[4:0]=00100` at 100BASE-TX and `00001` at 10BASE-Te.
Use the detailed speed-specific definition absent stronger exact-part evidence,
record the conflict and test both independent literal vectors. Do not copy the
100-Mbit recipe into the 10-Mbit path or block all loopback modes.

Validate capability, mode, interface, maintenance admission and finite-session
bounds before I/O. Capture only required owned state and explicitly handle/reject
an untrusted starting profile. Start, wait/acquire where applicable, stop and
verify-restore use one operation slot and aggregate deadline; release the bus
lock between polls. Persistent configuration extends reset/recover ownership.
No implicit loopback activation during initialization, passive status or recovery
except replay of explicitly requested persistent settings.

Cancel/timeout preserves accepted effects. Cleanup runs through later owner
execution under its finite allowance; primary and cleanup statuses remain
separate. A completed diagnostic with failed restore is not overall success.
Report what internal PHY path was selected without claiming MAC-frame, cable or
peer validation from a register write.

Test every supported mode/speed/interface with independent exact sequences,
unsupported zero-I/O rejection and invalid prior state. Fail/cancel after every
mutation including stop and each restore/readback. Assert transaction bounds,
stale-result protection, two sessions after consumption and unchanged unrelated
state. Compile a maintenance-owner example and document the complete mode matrix,
external fixture prerequisites and failed-restoration evidence.

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
