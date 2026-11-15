# Quality and hardware link-loss controls

`startConfigureQuality` admits an explicit `QualitySettings` profile into the
existing operation slot. Starts perform no MDIO or clock calls. The application
stops MAC traffic, admits maintenance, then polls with a finite deadline, frame
budget and stall limit. Each complete indirect or read/modify/write batch holds
the shared controller owner; no lock survives a poll. Configuration success
proves owned register readback, not link recovery or physical signal quality.

Profiles independently own Fast Link Drop, 100BASE-TX controls, 10BASE-Te controls
and hardware recovery. Unowned groups are untouched. Once admitted, an owned
group remains in the persistent profile; change its values explicitly instead
of silently dropping restoration responsibility. Desired settings, partial raw
words and whole-profile trust are separate in `qualitySnapshot()`.

| Group | Controls and admission | Evidence limits |
| --- | --- | --- |
| Fast Link Drop | Named cause subset, MII for nonzero causes; zero disable legal on RMII | Unknown reset nibble is observed, never assumed. MLT3 and descrambler use the documented recipes unless raw threshold override is explicitly requested. |
| 100BASE-TX | Scrambler bypass, far-end-loopback FIFO depth and RX_ER during idle | Bypass requires coordinated traffic/partner use. FIFO depth is not a general MAC buffering control. |
| 10BASE-Te | Lower threshold, legal squelch codes, NLP/jabber disables, short/long preamble with partner admission | No cable-length or packet-acceptance guarantee. Initialization retains sole ownership of mandatory VOD setup. |
| Link-loss recovery | One explicitly owned hardware policy flag | Partner events govern behavior; this is not software retry or automatic reset policy. |

The [ledger](registers/dp83826i.json) owns the numeric field map. Original-source
[literal vectors](register-vectors.md) and `test/quality.cpp` check encodings and
preservation independently. CR2 odd-nibble detection remains
[`configureInterface`](interface-clock.md) ownership: set disables detection.
Quality owns only its separate RX_ER-idle bit; GENCFG IPG is another control.
Next-page exchange remains [`startConfigure`](negotiation.md) ownership.

Fast Link Drop captures PHYSTS before FLDS, disables detector causes before
changing thresholds, verifies each prerequisite, then enables and verifies the
requested cause set. Nonzero activation checks both the declared and observed
MII mode. MLT3 window/threshold zero codes are prohibited. Raw overrides retain
`fld_threshold_recipe`; they do not inherit the recipe's physical claims.
For source-recipe combinations, modes 1–4 have a printed maximum of 10 us,
descrambler 11 us. Modes 2, 3 and 5 require energy-loss or RX-error detection
enabled for those maxima. Other legal combinations remain configurable without
that claim. An unexpected preserved reserved nibble also withholds recipe-based
maximum applicability without authorizing a reserved-bit write. These silicon bounds exclude host observation latency and are never
converted into driver completion deadlines.

All 10BTSCR prerequisite, RMW and verification reads pass through the single
collector. Consumed polarity and PHYSTS mirror evidence survives a later failed
frame. A possibly consumed read without returned data marks possible history
loss. `collectEvents` continues to own FCSCR/RECR and MISR evidence; raw RECR
arithmetic and RMII status polarity remain gated. Cached readers consume nothing.

Cancellation or failure ends the operation immediately, retaining accepted or
uncertain effects and the original error. After mutation, OP-9 blocks new
configuration until a separately admitted `startInitialize` restores the owned
profile, or the application explicitly abandons cleanup. No retry, hidden
rollback or post-failure write is issued. Initialization, diagnostic restoration
and reset-bearing power/EEE recovery share the same quality pass, after interface
restoration and before output activation/final power policy. `startVerify` reads
owned fields without writing controls and cannot discharge a cleanup obligation.

`readSignalQuality` performs one bounded, four-frame DEVAD31 read of the
datasheet's MSE word. It stages ordinary output; failure leaves it unchanged and
marks the retained prior sample stale at its original timestamp. The snapshot
includes cached identity and link context, whose own validity/time remain
independent: this is not simultaneous sampling. Zero is a valid raw value, not
infinite SNR. No conversion to dB, distance or quality class is exposed. The
datasheet/app-note address discrepancy, scaling and update-time questions remain
under `mse_address`; no speculative alternate-address probe is made.

The [portable example](../examples/portable/Quality.h) shows explicit MII policy
and one acquisition rendered through multiple cached views. CLI implementation
later uses these same owners; formatting must not add live destructive reads.

Physical qualification requires an identified MII fixture, controlled energy,
symbol/error/descrambler stimuli, a partner and oscilloscope timing correlated
with actual link indication. Test each detector and combination, threshold
override, scrambler/FIFO setting, polarity, preamble and jabber/NLP behavior;
record part, straps, clock, cable and firmware identity. MSE needs authoritative
address/scaling evidence and calibrated measurements before derived units.
Host tests establish protocol and evidence retention only. Physical latency,
signal quality, partner compatibility and unavailable MII fixtures remain
NOT RUN; the initial RMII board does not remove the MII software controls.

Quality prerequisite reads retain active-reset and contradictory BMCR mode
evidence before a later read or release can fail. An interleaved admitted read
that observes reset/restart terminates the current maintenance pass before any
next dependent write. Raw observation success and interrupted-operation failure
remain distinct. A quarantined-controller MSE request performs no I/O, leaves
caller output unchanged, and marks the previous cached value stale at its old
timestamp with the rejection status.
