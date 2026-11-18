# Loopback sessions

`startLoopback(options, budget, token)` and `stopLoopback(budget, token)` are
separate bounded operations in the existing slot. They return IN_PROGRESS on
bus/clock-silent admission. Poll at most one complete batch per owner turn and
consume each result once. The application chooses dwell, produces/correlates
traffic and stops that traffic before requesting restoration. A successful
activation proves the selected control path, never delivered packets or signal
integrity. BIST generator/checker support belongs to prompt 24.

Start requires fresh DP83826I identity, initialization and a VERIFIED desired
profile. The caller explicitly attests maintenance admission; ordinary MAC/SDK
traffic must be quiescent. Starts reject active reset, pre-existing loopback/BIST,
power-down, isolation and collision-test controls before mutation. External
changes must be reported through `invalidateDeviceState`; no read sequence can
prove that an uncoordinated external writer is absent.

## Mode and fixture matrix

All modes support configured MII and RMII. The historical MII-loopback name does
not exclude RMII. There is no supported RGMII binding.

| Path | Speed | Additional admission / activation | Restoration |
| --- | --- | --- | --- |
| MII | 10 or 100 | Speed-specific detailed BMCR companion; optional line transmission only at 100 | Ordinary stop and profile replay; no restart on start |
| PCS input | 10 | Internal forced/full profile | Ordinary stop and profile replay; no restart on start |
| PCS output | 10 or 100 | Internal forced/full profile | Ordinary stop and profile replay; no restart on start |
| Digital | 100/full | Exact companion recipe; `allowHardResetExit` required; excludes overlapping EEE use | HARD, initialization/profile replay and saved ordinary fields |
| Analog | 10 or 100 | `termination100Ohm` attestation; restart after activation | Ordinary stop and profile replay |
| Reverse/far-end | 10 or 100 | `partnerReady`, compatible live link; caller FIFO code; restart then bounded relink at the requested speed | Saved FIFO, selectors and persistent profile, including manual MDIX |
| External | 10 or 100 | `externalFixture`; internal selectors clear; existing profile must allow the speed; AN need not be disabled | Ordinary stop and profile replay; owner removes fixture only after stopping traffic |

Near-end paths use the same validated forced-profile procedure as normal
configuration: full duplex, robust manual MDI, no mirror or duplex extension.
The persistent desired profile is never overwritten. Reverse and external
retain it. Start does not rewrite mandatory VOD; initialization already proved
it. Every stop reuses initialization, so VOD is rewritten and verified there.
For MII loopback, BMCR 14 is set before optional BISCR 6 line transmission;
stop clears BISCR 6 before clearing BMCR 14 (SNLS647I table 8-40).

The source ledger owns field encodings. [Independent vectors](register-vectors.md)
and `test/loopback.cpp` contain literal recipes and bounds derived from the
original tables, not production masks.

## Effects, failure and restoration

The first attempted semantic write sets `temporaryMayBeActive` and
`cleanupOwed`, even if the backend reports an effect-free failure. A failed
selector-only prefix does not imply a target mutation. Failures/timeouts/cancel
freeze the current result immediately: no automatic cleanup, retry or escalation
follows. Take releases the operation slot while retaining the session and barrier.

During dwell, admitted raw reads and the single collector remain available.
Cached `loopbackSnapshot`, health and settings access performs no I/O.
Live link sampling reports the link bit with temporary-state evidence and
withholds normal speed/duplex/pause resolution. Reset waits retain the normal
OP-14 guard. Every batch releases controller ownership before returning.

Only the owning stop or explicit HARD initialization can restore a possibly
active session. Other initialization kinds, configuration, verification and raw
mutations reject. Abandon clears the cleanup obligation but retains temporary
knowledge; it requires the owner's external disposition and is not restoration.
External reset/POR notification clears temporary knowledge without I/O. Historical
results remain immutable. A new bind discards local knowledge only after the
existing teardown contract has been honored.

Stop rechecks identity, clears the ordinary loopback controls, then uses the
existing NONE initialization and persistent profile replay. Digital instead
uses HARD because its opaque companion registers have no documented arbitrary
save/restore operation. No opaque register is read for replay. HARD may resample
eligible straps (Q5); it can reset unrelated unowned hardware state. This is an
explicitly admitted reset, not a promise to preserve every register image.

The common final restoration steps restore/verify saved ordinary BISCR controls
and PHYSCR FIFO. A HARD exit also restores saved BMCR extras, including isolation
that straps may reload, while keeping commands and software power-down neutral.
Saved active BIST/loopback is never admitted, so old traffic is not replayed.
CR1/PHYCR routing, mirror, duplex extension, speed and AN intent are restored
through their existing profile owner. The barrier clears only after the complete
tail passes. A readback mismatch remains in the result even if release or timing
then fails; the later error is primary. HARD readiness stays invalid until that
tail succeeds. An explicitly requested HARD uses the same saved-field tail.
Each new HARD replays that tail even if an earlier stop verified it before
failing in a later BIST restoration step: reset invalidates the earlier proof.

Session start/stop callbacks, including their replay, count only in transport
totals. A separately admitted `startInitialize({HARD})` is managed traffic.
Neither stop success nor a later retry overwrites an earlier failed result or
managed lastError.

## Bounds and owner example

Absolute deadlines, total frames, callback bounds, polling interval and stalled
poll limit are required independently for start and stop. Small poll slices
yield before the whole RMW/MMD batch. Reverse relink and restart completion use
the remaining aggregate allowance; no completion maximum is inferred from the
datasheet. Digital/HARD exits publish the persistent shared-bus quiet floor.
The owner holds every PHY/controller participant silent until it expires.

With immediate restart/relink completion, the current start bounds are:

| Path | Frames / callbacks |
| --- | --- |
| MII | 34 / 25 |
| PCS input or output | 32 / 24 |
| Analog | 34 / 26 |
| Digital | 46 / 30 |
| Reverse | 19 / 15 |
| External | 12 / 9 |

For a persistent AN profile, ordinary stop uses 57 frames/33 callbacks; digital
stop uses 71/39. A forced persistent profile uses four fewer frames and three
fewer callbacks. Each additional wait observation consumes one frame/callback,
bounded by the supplied budget. These are protocol bounds, not measured latency.

[Loopback.h](../examples/portable/Loopback.h) reuses the shared owner poll helper
and hard-reset-floor value. The linked consumer runs a 10-Mbit PCS session over
an unchanged 100/full desired profile, retains a failed stop, then explicitly
admits a separate successful stop. It performs no hardware fixture action.

## Source conflicts and physical evidence

SNLS647I table 8-19's speed-specific MII companion overrides the general prose
in 8.3.14.2. Digital's full companion sequence comes from 8.3.14.4. The selected
SNLA423A scripts on pages 18/20 add restart to reverse/analog; no extra restart
is imposed on MII/PCS. Table 8-40's reverse selector wins over the note's reserved
bit example. Analog `030B=3380` remains unadopted because its field is outside
the documented range. Lack of later BIST lock will be inconclusive, not grounds
to invent a gain value. The `loopback_recipe` conflict and Q8 remain open for
exact-part clarification or measured traces. Source/host coverage proves no
termination, MAC, peer, packet delivery or electrical quality.

Stage 24 now composes BIST with these same digital/analog/external paths; other
BIST interactions remain source-gated. Its owning stop additionally disables and
checks generator activity and restores saved packet/IPG. Plain stopLoopback
cannot bypass that owner. See [BIST sessions](bist.md).
