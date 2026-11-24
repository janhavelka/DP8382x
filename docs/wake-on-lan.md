# Wake-on-LAN controls and retained indication

The [typed control CLI](cli.md#typed-controls) exposes policy, explicit observation and acknowledgement. Machine records carry validity while withholding match credentials.

`startConfigureWake` admits a complete `WakeSettings` into the existing operation
slot. Starts are bus/clock silent; the application polls with finite frame slices
and consumes the retained `OperationResult::wake`. `acknowledgeWake` starts a
separate operation with a separate budget/result. `readWakeStatus` acquires live
evidence once; `wakeSnapshot` is passive. The application serializes every call
and all participants honor complete MDIO transaction exclusion.

The implementation follows exact DP83826I SNLS647I 8.3.5 and tables 8-90..97.
Settings cover six-octet MAC matching, optional six-octet Secure-ON, CRC gate,
level or pulse indication, all four pulse widths, and explicit output routing.
API octet 0 is first transmitted. All 48-bit MAC/password values are representable;
Secure-ON is a packet password, not cryptographic authentication. A dormant
secureOn request while disabled does not set the hardware flag. Unknown bit 6 is
preserved; no custom pattern engine is exposed or inferred from its name.

| Route | Admission and behavior |
| --- | --- |
| STATUS_ONLY | No physical pin or IRQ wiring required; matching/status remains useful |
| INTERRUPT | An explicitly owned verified WAKE interrupt policy, global enable, MISR2 wake enable and applicable output direction; existing pin contracts apply |
| LED1 | Declared unused/available pin; ENHANCED additionally needs unused clock consumer and live clock-off evidence; narrow mux 2 ownership |
| LED2 | ENHANCED only, available pin and unused MII COL where applicable; narrow mux 2 ownership |
| LED0 / LED3 on original I/E | Read-only mux fields preclude direct wake routing |
| LED0 on AI/AE | Both modes allow direct wake with board ownership and a free pin 30 clock route |
| LED3 on AI/AE | ENHANCED allows direct wake. MII requires the owner to release CRS |

AI/AE uses the SNLS783A RXFCFG1 W0C indication and the same wire-octet
ordering. MAC bytes `02:12:34:56:78:9A` produce the three words `1202`,
`5634` and `9A78` in ascending register order. The collector retains an
indication before an unrelated control update writes neutral one.

Enable cannot silently replace an owned polarity-source IRQ policy. Direct WoL
and IRQ functions cannot share one LED mux. An owned wake IRQ prevents later
interrupt settings from disabling its delivery. EEE's clock recipe conflicts
with direct LED1 wake routing; LED2 retains its own source-backed admission.
The narrow route fields are restored in the same WoL procedure, including mux 4
when the selected IRQ owner uses an LED. General pin/clock control remains a
later stage. The selected route is the path this procedure verifies, not a promise
that other pre-existing pin functions are disconnected; their disposition remains
with the pin owner. Disabling detection retains the selected route requirement,
so conflicting IRQ or EEE requests still reject before changing desired intent.
Select STATUS_ONLY explicitly to release that requirement; this does not rewrite
the old pin mux. Shared WoL reset replay also restores a separately owned IRQ
LED mux after disabling global IRQ delivery. A clock that remains active on LED1 is rejected; no guessed clock
write is added. Board wiring and host wake logic remain external.

## Configuration and restoration

Whole-request validation precedes I/O and desired-state change. Configuration
requires explicit maintenance and receive-traffic-quiesced admission. That is an
application statement; detector-disable readback alone cannot prove silicon
quiescence. Safe continuous live reconfiguration remains Q3 evidence work.

1. Recheck identity and route prerequisites, then read control and all six match
   words. Skip unchanged configuration under stable mask 17E1, including preserved
   bit 6; there is no implicit acknowledgement.
2. If a change is needed while any enable/Secure-ON/magic-match bit is set,
   harvest RXFS/MISR2, clear those three bits with W0C neutral one, and verify.
3. Apply and verify a needed narrow pin mux while the detector is disabled.
   Write and verify all six credential words when any credential differs.
4. Harvest RXFS/MISR2 before the final RXFCFG1 update, preserve reserved/unknown
   bits and compose W0C neutral one, then enable last and verify stable fields.
5. Capture RXFS and drain the crossing MISR2 sample before establishing its
   attribution. Restore/verify any owned interrupt policy afterward; final power
   policy follows it in shared initialization.

Every indirect RMW is one indivisible five-frame callback. No controller lock
spans polls, owner scheduling or output. Short poll slices yield before an entire
batch. Failed or late writes retain possible effects and OP-9 restoration debt;
there is no rollback, automatic retry or extra cleanup write after failure.
Cancellation freezes its own result. Explicit `startInitialize` is the separately
budgeted restoration path and never replays an acknowledgement.

Reset is not WoL-off: the reset control has enable set and magic bit 0 unknown;
RXFS selects WoL. Initialization writes no WoL settings unless explicitly owned.
Owned NONE/PCS/HARD/RESTART paths inspect and restore requested controls, credentials
and route. `startVerify` reads without mutation. A reset can lose external clock
prerequisites; restoration then fails honestly with debt rather than claiming a
usable pin. IEEE power-down/PLL disable may prevent reception even with valid
stored WoL intent. No PHY indication proves that a MAC, MCU or application woke.

## Acknowledgement and observations

Exact table 8-90 makes bit 11 W0C: one is neutral, zero acknowledges the level
indication. Both settings and acknowledgement harvest the single collector
before every RXFCFG1 write. Acknowledgement requires owned verified level mode,
preserves all stable bits, writes once even if nothing appears pending, and
verifies the stable configuration. It does not poll until a line deasserts.
SNLA261A/DP83822 and shared TI Linux use write-one family behavior; that difference
remains Q3, without overriding the exact-part table. Harvesting preserves evidence
under either physical polarity, but cannot promise lossless multiplicity.

`readWakeStatus` reads control and six credential words followed by one RXFS/MISR2
harvest: 33 frames/eight callbacks. The whole pass is admitted before the first
frame. Ordinary output stays unchanged on failure. Cached valid words retain
their values; failed requested words lose their current validity. Every successful
destructive sample is retained immediately by the existing collector, even when
the next read fails. A read-after-effect error records possible history loss,
never zero events. RXFS bit 7/6/5/0 and MISR2's simultaneous events remain raw
source evidence; their collector samples retain history and timestamps.

A complete view is an ordered software acquisition, not an atomic silicon
snapshot. The 33-frame status pass does not reacquire the pin mux, so it clears
pinControlValid and retains rawPinControl only as historical diagnostic data. Later partial/IRQ reads make that aggregate historical; observedStale
can therefore be true after a successful configuration followed by IRQ verification,
while persistent trust is VERIFIED. Settings, raw fields and event history have
distinct validity. Valid contradictory controls, credentials, mux or RXFS source
withdraw trust immediately and survive a later error. Cached rendering never
consumes events again; host history clearing does not acknowledge the PHY.
An independent read that contradicts an already verified field between operation
polls freezes the WoL result immediately. No later poll may enable a detector
whose credentials or route are already known to be wrong.

The default status-only full change needs 111 frames/28 callbacks; an identical
request 35/10; acknowledgement 25/7. Route checks and any required mux update,
plus existing IRQ verification, add explicit bounded transactions under the same
budget. The maximum individual batch is five frames. These are software bounds,
not silicon trigger or host-wake timing limits. Persistent configuration/replay
is managed traffic; acknowledgement and live status are raw evidence traffic.

[The compiling owner example](../examples/portable/Wake.h) separates configuration,
observation, passive rendering and explicit acknowledgement. Keep the original
failed result while admitting any restoration with a new token/budget.

## Physical qualification

Before configuring an LED2 output, the source owner reads RCSR as the fifth frame
of its mux-read batch and requires observed mode to match declared wiring. This
also covers WoL through an LED2 IRQ route. No pin profile is required, and none
can bypass this prerequisite. Consumed RC evidence stays with the collector;
later mode contradictions withdraw route trust before further mutation.

Q3 remains NOT RUN: record exact part/revision/straps/interface, firmware identity,
MAC/Secure-ON packet octets, good/bad CRC and passwords, each route/pulse code,
line measurements, simultaneous interrupt sources and receive-path power state.
Compare neutral versus acknowledgement on actual pending level, confirm RH bit 0
stability, reset-loaded enable/source, quiesced reconfiguration and races with
new arrivals. Pulse widths derive from 8/16/32/64 cycles at 125MHz:
64/128/256/512ns; host tests do not measure those pins. Measure the external MCU
and application wake separately. SDK, packaging and physical timing remain
separate evidence from the portable host tests.
