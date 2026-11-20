# Manual cable capture

The [typed diagnostic CLI](cli.md#typed-diagnostics) exposes manual recipe and raw tuning, cached capture and interpretation, and separate autorun controls.

`startCableTest`, `startCableAcquire` and `stopCableTest` use the existing
operation slot, terminal MDIO batches and application owner. Admission calls
neither the clock nor the bus. Poll at most one complete batch per owner turn;
consume each retained result before admitting another operation. `cableSnapshot()`
copies evidence without I/O. These diagnostic operations are RAW transport
activity, separate from managed health and chip event/error counts.

Start requires exact accepted DP83826I identity, initialization and a VERIFIED
persistent link profile. The caller admits disruptive maintenance, quiesces
normal MAC/EtherCAT traffic, disconnects or silences the peer, and accepts the
HARD-reset exit. Those are application responsibilities, not inferred fixture
facts. Live preflight verifies identity and rejects reset/restart, loopback/BIST,
power-down, isolation, collision test, enabled CR1 autorun or an active CDCR
command. PHYSTS must report link down before any diagnostic write. Link down
does not by itself prove that a connected peer is quiet.

## Selected recipe and raw controls

The default `CableRecipe::SNLA428` plan selects all six runs, ordered 1A, 1B, 2,
3, 4, 5. `runMask` selects an explicit nonempty subset in that order. Skipped
rows remain `NOT_RUN`; zero-initialized storage is never a zero measurement.
SNLA428 table 2-1 is the selected recipe under D3 and `tdr_recipe`. Its register
words remain in the [factual ledger](registers/dp83826i.json) and
[independent vectors](register-vectors.md), rather than another numeric guide map.
The recipe transmits on A and captures on A only.

The six-run path writes the selected tuning and opaque companion words in the
note's order, verifies documented owned controls, then issues the manual trigger.
GENCFG is the explicit exception: a complete atomic RMW owns only bits 10:9,
preserving compatibility bit 3 and unrelated fields. Thus an old value `0008`
becomes `0608`, while `0000` becomes `0600`. The latter composition is inferred
and remains part of Q7. No general raw-write permission follows for those bits
or the opaque companion addresses.

`CableRecipe::RAW` runs one capture in the first result slot and requires
`runMask=1`. Its copied `CableControls` expose all documented controls:

| Control | Legal request |
| --- | --- |
| Channel | A to A, A to B, B to A, B to B |
| DC removal | Explicit no-initialization policy |
| Averaging | Codes 0 through 6, meaning 1 through 64 cycles; 7 rejects |
| Segment | Codes 1 through 5; these select tuning, not measured distance |
| Duration and initial skip | Independent byte codes |
| Shadow average / forward shadow | Three-bit / four-bit codes |
| Location / short-cable threshold | Independent five-bit codes |
| Pulse width | Three-bit code |

Raw configuration uses six complete field RMWs and readbacks, preserving reserved
bits. It does not access opaque recipe companions. The values from SNLS647I
tables 8-56/57 remain available through these typed fields and are distinct from
SNLA428 tuning. Raw B/cross-pair capture has no derived distance/classification
claim. Explicit link-down autorun and its manual handoff are described below.

## Freshness, raw data and explicit reacquisition

Preflight retains the full CDCR word. If its calibration bit is set, the operation
ends with an explicit error before cable writes. Prompt 35 owns the required
clear-then-HARD recovery; manual cable code does not clear calibration as an
incidental register update. Observing calibration also blocks initialization
until owning stop observes it clear or an external reset/POR is reported; this
prevents ordinary initialization from bypassing the known clear-then-HARD requirement.

A successful trigger alone is insufficient. Each run requires an observed
post-trigger start/busy or done-clear indication, followed by start clear and
done set. Initial/reset done and a previous run's done cannot satisfy this rule.
The source gives no trigger-to-progress minimum or completion maximum. If a fast
transition is missed, the finite owner budget can expire with freshness unproved;
the library does not invent success or a silicon failure classification.

After fresh completion, the driver retains fail status, then the supported
location, amplitude and peak words, followed by a CDCR consistency check. The
other result banks are reserved and are not read as additional decoded peaks.
Fail status is retained even when transport and raw capture complete successfully.

Each `CableWord` has independent raw, validity, staleness and timestamp evidence.
These reads are non-destructive: an unsuccessful read leaves data absent or older
data stale, not zero and not lost hardware history. Successfully returned words
survive a later transport/release/deadline failure. The complete aggregate stays
stale until the requested rows finish. Only applicable prerequisite status reads
use the single destructive-evidence collector; cable result words do not create
another collector.

After a failed result read, `startCableAcquire` can explicitly reread the retained
current run under a new budget and token, without rewriting a trigger or tuning.
It requires an accepted current trigger and valid device generation. It cannot
resume unperformed later runs after another run's programming failed, recover an
ambiguous trigger by pretending it succeeded, or make pre-reset samples current.
There is no automatic retry. Preserve the original result separately.

## Stop, cancellation and restoration

The first attempted semantic mutation retains `temporaryMayBeActive` and
`cleanupOwed`. Cancellation is bus-silent and freezes its own result. Failure,
timeout and cancellation issue no cleanup frames; the owner admits a later stop
or eligible HARD initialization with a separate finite allowance and result.

Stop rechecks identity and live CDCR, refuses the calibration contingency, clears
manual start and verifies the command control. That bit readback alone is not
proof that the analog engine has stopped. Stop then performs HARD through the
existing initialization path, reselects identity, replays the persistent profile
and restores/verifies saved documented tuning and BMCR extras. SNLA428 also
restores GENCFG bits 10:9; RAW never modifies or owns those exceptional bits and
does not replay them after HARD. Unowned settings may reset.
Both raw and SNLA428 paths use this admitted HARD exit. Opaque recipe words have
no documented arbitrary snapshot/restore operation; restart retains registers and
cannot discharge their effects.

Trust and the temporary-state barrier clear only after complete replay and
readback. A failed restoration remains a separate failure; it cannot overwrite
the capture outcome. Every newly admitted HARD must replay the saved tail again,
even if an earlier restoration completed some fields. Later implemented persistent
features, including EEE, extend this same initialization/replay path. Current
software does not claim restoration of unimplemented features or every unowned
register image.

HARD may resample eligible straps and affects all normal chip activity. The shared
bus owner holds every PHY silent for the published hard-reset floor and retains
that obligation through cancellation and result consumption. No controller lock
spans a wait. External reset/POR notification is bus-silent, invalidates current
capture knowledge and retains historical result evidence.

`OperationBudget` bounds the aggregate deadline, actual frames, polling interval
and stalled polls independently for capture and restoration. A vendor read/write
uses four frames, and a stable-field RMW uses five under one transaction owner.
Small poll slices yield before a complete batch. Additional status polls consume
the remaining finite allowance; these software bounds are not silicon timing
qualification.

## Source-implied interpretation

`interpretCable` is pure and leaves ordinary output unchanged on rejection. It
accepts complete, fresh, nonfailed selected-recipe rows and a positive finite
propagation delay in ns/m. It searches performed runs 2, 3, 4, 5, 1B, 1A,
selecting the first low-seven-bit amplitude above decimal 10 for runs 2 through 5
or decimal 24 for segment 1. Index and peak come from that same row. It computes
the magnitude of `(index - 5) * 8 / (2 * propagationDelay)` using the low index
byte. Whole peak word zero means source-implied OPEN; nonzero means SHORT.
Raw peak bits 11 and 5 remain separately visible without an invented bit-polarity
meaning. An amplitude at the maximum code is flagged; no undocumented saturation
behavior or exact accuracy follows.

No qualifying peak produces `NO_PEAK` with `distanceValid=false`, never “good
cable” or an asserted zero distance. The conclusion describes only the selected
transmit-A path. It does not establish pair-B condition, whole-cable health,
connector quality or application readiness.

SNLA428's suggested propagation values are Cat5/5e 5.35, Cat6 5 and Cat7 4.6 ns/m.
They are caller inputs requiring fixture calibration. Its complete table 2-5
sample selects run 5: index `78`, amplitude `11`, peak `0000`, giving 100 m at
4.6 ns/m. The note's MATLAB example has indexing and variable inconsistencies;
the helper is independently derived from the selected tables/interpretation, not
copied from that code. `tdr_recipe`, `tdr_calibration`, `tdr_freshness_abort` and Q7
remain explicit physical/source gates.

## Planned known-cable fixture

This is an actionable qualification procedure, not an existing fixture or
authorization to operate hardware. Physical distance accuracy is NOT RUN/BLOCKED
until an identified campaign supplies measurements.

1. Record exact board/netlist, PHY ordering code/IDs/revision, MII/RMII mode,
   straps, supply/clock arrangement, firmware commit, controller timing and
   instruments. Use a sacrificial isolated test link, with machine/EtherCAT
   activity stopped and ordinary MAC traffic quiesced. Disconnect any peer or
   independently establish it is quiet; verify live link down. Disable autorun
   through its owner when that feature is available.
2. Measure and label known cables of representative lengths in each selected
   segment, including short cables below 10 m and a 100 m Cat7 reference where
   available. Record actual cable category, construction, connectors, measured
   length and propagation-delay source; do not calibrate against its label alone.
3. At the remote end, manually select a known open, a known short across the
   selected pair, and a characterized matching differential termination (typically
   100 ohms for the cable). Identify the pins/pair and measure the termination;
   do not short a powered peer or treat Ethernet magnetics as a known resistor.
   A “known-good” terminated cable is a baseline: absence of a peak does not prove
   all pairs or connectors good.
4. Admit one six-run capture, retaining every raw word, validity, timestamp,
   selected row and completion/fail evidence. Record any missed freshness
   transition as unproved. Correlate MDIO traces with physical pulse/termination
   observations where instrumentation permits, without assuming host timing
   proves analog timing.
5. Stop and verify HARD replay before changing wiring or returning the link to
   service. Retain capture and each restoration attempt separately. Failed stop
   requires explicit owner recovery/disposition; do not continue a campaign on
   an uncertain configuration.
6. Repeat open/short/terminated cases with independent lengths and categories,
   then test raw A/B and same/cross-pair routing with identified pair fixtures.
   Raw cross-pair results do not reuse the transmit-A derived helper. Exercise
   both GENCFG bit-3 states and record the inferred `0600` case separately.
7. Compare source-implied distances against measured lengths and the declared
   propagation delay; retain offset, error, limits, noisy/maximum-code samples,
   missing peaks and restoration failures. Do not claim the datasheet's stated
   diagnostic accuracy or universal cable classification from a passing host
   test, a single length or one correctly identified open circuit.

The [portable raw-capture owner example](../examples/portable/CableCapture.h) is
compiled and executed by the separate tests-OFF cable consumer.
Host tests establish literal traces, bounded state transitions and retained
evidence. SDK linking, physical timing and cable accuracy are separate layers.

## Reproducible host bounds

With one busy and one complete status read per run, six SNLA428 runs use
426 frames in 128 callbacks; one selected run uses 101/33. RAW uses 102/32.
Each extra pending poll adds one frame/callback under the same deadline/budget.
With the test's negotiated profile, successful separate stop/replay uses 127/50
for SNLA428 and 118/48 for RAW. Other owned profiles can change replay counts;
the finite caller allowance remains authoritative. No sequence is split across
an indirect transaction and no controller ownership survives a wait.

## Hardware link-down autorun

`startConfigureCableAutorun({enabled, maintenanceAdmitted}, budget, token)`
owns only the documented CR1 enable bit. It requires initialized accepted
identity and, when present, a VERIFIED link profile. Starts are bus/clock silent;
syntax/lifecycle/barrier/initialization checks precede zero-budget outcomes.
Admitted intent is copied immediately into `cableAutorunSnapshot().desired` and
remains authoritative after cancellation. Rejected requests change no intent.
Observed value, validity/time, staleness and trust are independent fields.

Live identity/reset/BIST/manual-command/calibration checks precede a single
two-frame CR1 RMW and one owned-bit readback. Policy configuration takes exactly
10 frames in eight terminal callbacks on success. All unrelated CR1 controls and
reserved bits are preserved. Setting the flag is not a physical trigger; neither
link-down nor CDCR's initial done bit proves a fresh diagnostic measurement.
Policy configuration is MANAGED traffic; raw acquisition is RAW.

Disable with the same explicit operation and consume its successful result before
starting manual capture. **Engine quiescence remains unknown**: readback proves
only disabled policy. Hardware may update status/results during any handoff phase.
Manual capture still performs its own live preflight and rejects enabled autorun,
an active manual command or calibration. No automatic disable/re-enable, hardware
idle claim, generation counter or drain-until-stable loop is added. Keep normal
application traffic quiesced under the same maintenance owner.

Every shared initialization replays an explicitly owned flag, including disabled
intent, after persistent profile and temporary diagnostic saved-field restoration.
It adds three frames in two callbacks. Initialization/restoration cannot become
complete until this final replay/readback succeeds. Unowned policy adds no frames
and is never implicitly enabled. `startVerify` and ordinary `startConfigure`
check owned policy with one additional read, without rewriting it. Reset/raw
invalidation keeps desired intent while staling observed/trusted evidence; rebind
or end discards local ownership. Future power operations extend this same replay.

Failed/late policy mutation retains possible effects and the OP-9 cleanup barrier.
Cancellation performs no cleanup I/O. Take the frozen result, then explicitly
admit `startInitialize` with a new budget/token to restore all owned state.
A later successful read cannot retroactively validate the failed write. Policy
configuration cannot bypass another diagnostic's temporary-state barrier.

## One bounded autorun observation

`startCableAutorunAcquire(budget, token)` uses the same result-read phases as
manual capture: CDCR, the three documented result words, then CDCR once more.
This is exactly 14 frames in five terminal callbacks, without a trigger, reset,
environmental wait or retry. The initialized owner can read even without owning
policy, or while a nontemporary cleanup obligation exists, as raw observation;
temporary manual/other diagnostic sessions reject this separate acquisition.

The result has `autorun=true`, RAW provenance, one row, and `complete=true` only
when the software read plan finishes. Aggregate `stale=true` explicitly withholds
current-measurement interpretation. No accepted manual trigger or freshness flag
is fabricated. Cached `settingsValid=false` means no tuning/recipe context was
captured; default control values are not observed hardware settings.
`statusChanged` reports unequal CDCR bookends; equal bookends
cannot prove coherent generation, freshness or unchanged intermediate results.
Each word can still be valid and timestamped as an individual observation.
The derived SNLA428 distance helper rejects this result.

On failure, each successfully returned nondestructive word remains in the retained
result; missing words stay invalid and no history-loss flag is invented. A new
explicit acquisition can reread under a new budget. Cache storage represents the
latest cable procedure; retain prior operation results separately. Two cached
readers do no I/O. Destructive preflight words remain with the existing collector.
The [portable autorun example](../examples/portable/CableAutorun.h) is compiled
and executed by the tests-OFF cable consumer.

For physical qualification, extend the identified fixture plan above: capture
CR1 readback and MDIO traces before/after controlled peer disconnects; correlate
actual link drops and cable changes with observations. Exercise updates during
disable, every result read and manual admission. Repeat identical-result and
missed-transition cases without assigning a new generation. Record peer activity,
termination and restoration independently. These are planned scenarios, not
executed measurements; physical trigger timing and distance accuracy remain NOT RUN.
