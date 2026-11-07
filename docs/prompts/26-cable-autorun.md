# 26 - Implement cable autorun and shared result acquisition

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [25](25-cable-manual.md), [20](20-event-counter-acquisition.md),
[4](4-silicon-diagnostic-ledger.md). Verify the concrete inputs below; retain unrelated
open gates under COMMON.

## Scope, current evidence and work

Deliver explicitly configured silicon cable autorun on link down, exact
readback/status/result behavior and interaction with manual capture. Reuse task
25's verified controls, decoding, collector and restoration; no autonomous
software recovery loop or duplicate diagnostic acquisition path.

Reopen SNLS647I 8.3.16 / table 8-28 and the
[autorun/result facts](../reference/library-ti-features.md#cable-controls-and-remaining-interpretation-gates):
CR1 `0x0009[8]` (`0x0100`) enables hardware autorun after link down.
CDCR done is set in reset value `0x0102`; neither that bit nor unchanged
peak values prove a new trigger. Include literal autorun enable/disable
preserving CR1's other controls, initial-done/stale-result cases, manual-start
contention and reads spanning a hardware update. Do not manufacture a generation
counter, freshness guarantee or maximum silicon completion time absent evidence.

Reopen TMP1x2 operation result-retention/public lifetime contracts and fixture
runner/parser cases; compare PCA9555 cancel/cleanup and deferred observation
tests. Inspect current registrations and examples. Reopen all SNLS647I hardware
autorun, link-down trigger, cable status and shadow/result descriptions. Do not
infer generations or timing guarantees the device does not expose.

The documented autorun policy is one CR1[8] enable flag. Validate and apply it
once with owned-bit readback and persistent desired/observed/trusted state.
Do not invent additional settings or silicon quiescence evidence. Replay only
explicitly owned policy after applicable reset/power transitions.

Separate configured policy from a physical trigger and fresh capture. To hand
off to manual capture, clear CR1[8], verify it, and report engine quiescence as
unknown; otherwise reject manual start while enabled. There is no documented
quiesce sequence. Test hardware updates during each handoff phase. A captured
autorun result is at most one run under the current settings, never SNLA428's
six-run result, and has no derived distance without known recipe context.

Bound acquisition and any manual-session handoff. Never drain until stable or
poll until the environment triggers. Stop/reconfigure with explicit maintenance
admission where disruptive; preserve accepted settings and partial samples on
failure. Cancellation and cleanup keep their separate finite budget, retained
effects and status rules. Hardware autorun does not authorize software reset,
link recovery or application scheduling policy.

Test enable/disable/readback, invalid requests, old done/results, trigger during
acquisition and manual handoff, each mutation/read/restore failure and exact bounds.
CDCR/0180/0185/018A are non-destructive: failed words become invalid without
history-loss flags; bounded re-read is legal. Destructive prerequisite reads
remain collector-routed. Cached views do no I/O.

Extend the diagnostics guide with manual-versus-autorun ownership, persistence,
freshness limits and fixture scenarios. Compile an owner example that explicitly
configures policy and later acquires evidence. Physical trigger timing and
distance accuracy remain separate from host protocol tests.

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
