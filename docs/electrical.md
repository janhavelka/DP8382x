# Electrical controls and finite maintenance sessions

The [typed diagnostic CLI](cli.md#typed-diagnostics) exposes explicit compliance, collision and calibration requests. [Typed controls](cli.md#typed-controls) exposes analog intent.

`startConfigureAnalog` applies an explicitly owned `AnalogSettings` profile.
`startCompliance`/`stopCompliance` and `startCollisionTest`/`stopCollisionTest`
are separate bounded calls around application-controlled instrument dwell.
`startCalibrationRecovery` executes the conditional resistor-calibration
contingency. All use the existing operation slot, retained token/result and
application-wide MDIO owner. Starts are bus/clock silent; each poll submits at
most one complete batch. No controller lock spans a wait or a later poll.

The [ledger](registers/dp83826i.json) remains the numeric/access catalogue;
[original-source vectors](register-vectors.md) and independent host literals
verify the implementation. These controls require exact DP83826I identity,
maintenance admission and a stopped MAC. They do not establish electrical
compliance, calibrated impedance, EMC or analog signal integrity.

## Persistent analog profile

Ownership groups cover positive/negative MDI and MDIX amplitude with raw zero
DAC code, MDI/MDIX termination and enables, MAC output slew, and IO slew.
Unowned groups remain untouched. A replacement profile retains every previously
owned group so reset restoration cannot silently drop responsibility. Invalid
encodings or unowned nondefault values reject before desired-state change or I/O.
Receiver-gain interpretation stays gated; no write to the wholly reserved analog
register or the conflicting IOCTRL example is exposed.

Amplitude changes snapshot the current split MDIX code before mutation. When
its high bits change, a verified low-nibble bridge keeps every intermediate
code legal. Every write is followed by readback before dependent continuation.
Unavailable or undocumented current codes retain a cleanup barrier and permit
no guessed split-field write. There is no claim of glitch-free analog output.

Initialization always performs and verifies its mandatory VOD setup first.
An explicitly admitted termination override follows in the shared analog tail.
`startVerify` compares that owned desired override; otherwise it checks the
mandatory baseline. Link configuration does not duplicate the setup write.
Persistent replay follows interface and quality restoration, before pin/source
activation and final power policy. Re-observe current VOD words on every replay.

`analogSnapshot()` separates desired settings, each retained raw word/validity,
whole-profile trust, timestamp and stale evidence. Full application uses at most
70 frames in 18 callbacks: two prerequisite reads, five four-frame snapshots,
six writes and six readbacks, including the optional split-code bridge. A
read-only pass uses at most 22 frames in six callbacks. Reset/profile restoration
adds its own bounded work under the same operation budget. Short poll slices
yield before the entire next batch. Failure/cancel stops immediately; a later
`startInitialize` has its own token, budget and result, preserving the earlier
failure. An invalid current code may require explicitly admitted HARD recovery.

## Compliance and collision ownership

If collision entry/exit consumes an active reset during its BMCR RMW, it stops
with INVALID_CONFIG/DEVICE_INVALIDATED unless a primary callback/clock failure
already occurred. The same batch may have accepted the write before returning;
temporary uncertainty and restoration debt remain, with no successful readback
or implicit retry claimed.

| Session | Admitted behavior | Stop and evidence |
| --- | --- | --- |
| TX100 | SNLA266B table 5-1, shared forced 100/full and selected manual pair, then restart | HARD stop; later initialization restores persistent ownership |
| NLP10 | Table 5-2, forced 10/full and selected manual pair, then restart | Same |
| PRBS10 | Table 5-4, speed-specific digital loopback/generator fields and restart | Same; traffic is not a passing measurement |
| PATTERN10 | Fourteen exact table 8-50 patterns; recipe-specific COMPT bit 4 exception | HARD stop; no generic reserved-bit permission |
| COUNT100 | Typed count 0..31, including E/F in that distinct context | Rejects before I/O under `compt_reserved`; no speculative 0428 read |
| Collision | MII, explicit COL mux and owner-declared TX_EN stimulus/frequency | Clear/verify collision bit; incomplete stop retains possible stimulus |

Compliance uses the existing forced profile sequence, including robust crossover
and reserved/strap preservation. It never imports script power-down/isolation
or undocumented PHYCR low bits. Requested power-down/isolation rejects before
entry, with live preflight before mutation. Persistent desired link settings
are not replaced by the temporary stimulus. HARD start destroys hardware
configuration; register success does not restore other persistent owners.
HARD stop verifies completion and identity, then leaves `cleanupOwed` until a
separately admitted initialization replays the persistent profile. Application
dwell does not hold the operation slot after its result has been consumed.

Collision verifies an explicit COL route; the ambiguous default mux label is
not treated as proof. The requested 10/100 rate is an owner fixture-frequency
policy, not an additional datasheet restriction. Forced speed comes from BMCR;
AN status is used only with observed link/completion. The documented COL
assert/deassert bounds require measured TX_EN and COL traces. Normal stop can
discharge its own barrier only while unrelated persistent proof remains valid.

Cancellation performs no bus work. A failed or cancelled start may leave stimulus
active. Consume and preserve that result, then explicitly stop with a fresh
budget. Failed stop remains visible, blocks teardown/rebind under OP-9, and never
becomes instrument PASS. `electricalSnapshot()` is passive; cached rendering
cannot repeat destructive prerequisite reads. The single collector retains
consumed RCSR/BMSR/BISCR evidence even if a later acquisition fails.

## Calibration contingency

Read CDCR once. Low calibration bit returns no recovery needed. High bit admits
one indivisible two-write batch: clear only the calibration request with manual
start neutral, then immediately issue literal HARD. Reserve the complete pair,
minimum quiet period and first completion poll before mutation. The backend
stops at its first failure. An ambiguous clear cannot authorize an automatic
post-error continuation: retain its effect and cleanup obligation, then let the
owner admit a new HARD recovery. Any attempted or unprovably omitted HARD frame
establishes the persistent bus-wide quiet floor, including error/cancel/take.

Successful HARD completion and re-identification prove reset completion only.
They do not prove resistor calibration completed or achieved an accurate value.
Later initialization restores persistent controls with a separate retained
result. No calibration polling loop or automatic reset retry exists.

## Fixture and application responsibilities

The compiling [owner example](../examples/portable/Electrical.h) separates
entry, application dwell, explicit stop and later initialization. Identify the
PHY ordering suffix/revision, board, supplies/VDDIO, clock, magnetics, cable,
termination/load and routing. Use a calibrated differential MDI probe and
suitable compliance fixture/reference conditions, plus MAC-pin probes for slew
and TX_EN/COL timing. Record instrument calibration, bandwidth/loading, selected
pair/rate/pattern, raw traces, limits, ambient conditions and each entry/exit
result. Keep the MAC stopped until verified restoration and application approval.
A missing or failed exit invalidates the session verdict even if a stimulus
looked correct. Do not operate a fixture from these host examples automatically.

`ioctrl_example`, `analog_rx_gain`, `compt_reserved` and `hard_reset_straps`
remain explicit source/qualification limits. Q19 covers the sourced script
adaptations and physical compliance; hardware results, electrical timing,
resistor accuracy and energy measurements are NOT RUN.

A reset command alone does not mean a compliance stimulus is active: reset
acceptance, completion, quiet floor and restoration debt remain separate from
the first attempted companion/profile/stimulus write. Cancellation immediately
after the initial HARD therefore permits ordinary bounded reconciliation once
the floor expires. An owner-attested external reset or POR clears current
electrical sessions and calibration HARD debt without changing a frozen result.

Shared replay leaves the analog pass before handing control to owned pins, WoL,
interrupts and power. Combined profiles retain readiness and cleanup barriers
until those later passes also finish. An admitted read observing an active reset
interrupts a maintenance pass before another dependent snapshot-based write.
