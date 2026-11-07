# 15 - Implement initialization and complete PCS reset operations

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [14](14-identity-capabilities.md), [8](8-operation-contracts.md). Verify
the concrete inputs below; retain unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver initialization and BMCR PCS reset as complete caller-driven operations,
including cancellation, retained results, teardown and bounded restoration of
currently implemented owned state. Use the existing facade and one explicit
operation-kind/phase switch; no standalone generic job engine.

Reopen TMP1x2 `OperationResult`, `OperationSnapshot` and
`src/TMP1x2Operations.cpp::admit`, `poll`, `finishJob`,
`cancel`, `takeResult`. Inspect PCA9555 cancel/required-cleanup,
detach-refusal and shared-remaining-deadline tests; compare 24Cxx retained effects
across waits. Reject EEPROM ACK polling and sensor conversion delays.

Implement `startInitialize` with reset kinds NONE and PCS, `pollOperation`,
`takeResult`, `cancel`, `operationSnapshot`, `abandonCleanup`,
`invalidateDeviceState` and `isInitialized` under the current API operation rules
OP-1 through OP-17. Starts are bus/clock silent. One slot and a nonzero 64-bit
lifetime token protect retained results; there is no exhaustion branch or test.
A short poll slice yields before the whole next batch; only total budget or
`maxStalledPolls` exhaustion ends it.

Use SNLS647I's exact BMCR effects, completion evidence and waits. Anchor minimum
waits after callback completion, or after a documented upper bound on that
completion. A caller timestamp captured before the reset write cannot by itself
anchor a post-write minimum wait. Account for quantization, uint64 overflow, exact expiry and frozen-clock
transfer bounds. Return to the owner instead of busy-spinning. The controller
lock covers one transaction and never spans waits/polls.

Accepted reset invalidates appropriate link/configuration/identity knowledge
even if later reads fail. Restore and verify only implemented owned settings
with one source-backed procedure that subsequent feature tasks extend. Initialization
can succeed with cable down. At this stage initialized means the implemented
identity/reset/readiness contract is complete; link-profile application arrives
in prompts 17-18 and must then become part of initialization when requested.
Keep managed-health methods unavailable until prompt 21 implements their
transitions. Reset success alone proves neither link nor electrical qualification.

Cancel/end/cached access remain bus-silent. Cancellation ends with its retained
result. OP-9's `cleanupOwed` barrier protects accepted/uncertain effects until a
separately admitted restoration operation succeeds, or the owner uses
`abandonCleanup` under OP-10. Restoration has its own token, result and budget;
its success cannot replace the earlier failure. End/rebind obey OP-12 and do not
release resources still used by callbacks. Expose NONE and PCS now; HARD and
RESTART arrive at prompt 16.

Test literal reset traces, sticky/never-clearing reset and every write/read/restore
failure. Cover uint64 overflow, regressing clock with the CLOCK_CONTRACT latch,
quantization, raw deadline expiry, frozen-clock stalls, zero and short poll slices,
and cancellation at each effect boundary. Test wrong/stale tokens, repeated take,
result-pending rebind and reuse after consumption. Prove OP-6's literal admission
formula including the 100 Hz clock case, and no split MMD restoration. Compile
an owner loop and a failed-restoration/abandon example.

## Concrete TI source inputs

Re-audit [TI initialization/reset behavior](../reference/library-ti-core.md#initialization-reset-and-recovery-procedure-audit)
against table 8-19. PCS reset is a single literal `W(00,8000)`, never an RMW;
the earlier check for already-active reset is a separate read. The reset frame's
other zeros are inferred harmless because reset reloads them. Replay desired
fields afterward. Validate the complete request before mutation. Through prompt
32 initialization does not write RCSR; prompt 33 adds collector-routed RCSR
replay because [3:2] are RC. CDCR rescal recovery belongs to prompt 35 and CDCR
remains rejected for raw read.

Every `startInitialize`, for every ResetKind, writes vendor `030E=4A40` using
`W(0D,001F) W(0E,030E) W(0D,401F) W(0E,4A40)` and verifies mask `FFC0`.
There is no verify-only initialization branch: PCS preserves vendor registers,
so a POR default `8400` remains until this mandatory write. Prompts 17/18 require
initialized and add no duplicate VOD write. Prompt 22's `startVerify` checks the
same mask without writing; restoration uses `startInitialize`.

Before first probing, the application waits at least 50 ms after both supplies
reach final level. If clock is absent at the ramp, hold RESET_N low and release
at least 100 us after clock stabilization, or apply an additional RESET_N after
POR completion initiated at least 100 us after both clock stabilization and POR
completion (7.6 footnote 2). SMI remains inactive 2 ms after release. External
readiness is not a PCS reset completion maximum.

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
