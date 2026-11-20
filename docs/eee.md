# EEE configuration and observations

The [typed control CLI](cli.md#typed-controls) exposes EEE configure, explicit read and cached status under the existing reset and collector ownership.

`startConfigureEee` owns a complete, explicit `EeeSettings` policy in the existing
operation slot. `readEeeStatus` acquires bounded live evidence;
`eeeSnapshot` and `evidenceSnapshot` are passive. No default initialization
activates EEE. The exact DP83826I procedure is SNLS647I 8.3.3.1, with the
[source-implied mapping and independent literals](register-vectors.md).
The `eee_wire` and `eee_control_semantics` gates remain physical-check pending
under Q1. Host success does not qualify the wire mapping or interoperability.

## Admission and effect ownership

Enable requires an initialized accepted device, an explicitly owned verified
AN profile and maintenance admission. Both MII/RMII and every legal AN technology
subset remain supported. The 10BASE-Te amplitude behavior does not create a
10-Mbit EEE advertisement bit or prove negotiated LPI100. Advertisement accepts
only zero or 100BASE-TX EEE; reserved technologies reject before I/O.

`bypassBit0` and `bypassBit3` name the two exact-part capability-bypass controls.
True clears the respective bit; false disables bypass by setting it. The source
does not establish independent TX/RX directional meanings for these controls.
Only the admitted control mask is modified; opaque recipe bits are preserved.
EEE configuration keeps forced TX LPI clear. [Explicit LPI calls](lpi.md) own its temporary bit 12 session.

Host-controlled next-page exchange cannot run while EEE is enabled. Forced link
profiles, explicit active-sleep/IEEE-down/PLL-disable policies, alternate LED IRQ
routes and temporary loopback/BIST/manual cable overrides must first disable EEE.
Those choices conflict with this recipe's AN, clock or opaque diagnostic state;
the library does not silently turn them off. A known conflict rejects without
callbacks or desired-state changes. The exact clock row remains in the recipe;
its explicit sequence permission does not authorize generic reserved-bit writes.

## One reset-bearing procedure

Starts are bus/clock silent. Polls perform one complete terminal batch, yielding
before an indivisible MMD transaction if the slice is too short. The shared MDIO
owner includes every PHY and observes the persistent HARD silence floor.

1. Read identity/straps and reset state through existing initialization admission.
2. Require the whole capability word to equal the Q1 literal before HARD. Selector
   frames are physical writes, but this check performs no target mutation.
3. Issue the single literal HARD command, wait cooperatively through its minimum
   silence, poll completion and reselect identity/straps.
4. Execute row 2 and require its whole advertisement word to match before the
   other sixteen rows. Every row uses its exact DEVAD and a complete fresh
   selector/address/data sequence. Opaque words are not individually certified by
   invented readback masks.
5. Apply the requested legal advertisement and bypass controls, verify their
   stable owned bits, then replay mandatory VOD and the owned link profile.
   Thus the final profile restart uses the desired EEE advertisement.
6. Verify EEE stable controls, then restore/verify owned autorun, IRQ and power
   policies in the existing order. Only complete proof publishes restoration.

The eighteen recipe rows plus two self-checks require at least 74 frames.
Admission reserves 75 including the first reset poll, with 21 bounded callbacks,
quantization and HARD silence; identity/profile replay needs additional allowance.
After the precheck, the remaining recipe floor is 71 frames/20 callbacks.
Actual late owner polling can still exhaust the aggregate deadline. No nominal
EEE sleep/wake interval is used as a completion maximum.

Disable uses HARD plus shared persistent replay, then verifies advertisement
clear. There is no inverse opaque recipe. Once EEE policy is owned, each
`startInitialize` restores it through exactly one effective HARD, including a
caller request of NONE, PCS or RESTART. `OperationResult.reset` reports that
actual kind; requests without owned EEE retain the earlier reset semantics.
This explicit upgrade ensures interrupted opaque setup is never repaired by
stable-register checks alone. Ordinary CONFIGURE/VERIFY read the EEE controls;
they do not recursively restart the recipe. VERIFY cannot clear restoration debt.

Failures and cancellation freeze their own result, possible effects, primary and
release statuses, and OP-9 restoration obligation. Another initialization has a
new token/budget/result. It does not rewrite the earlier failure. Failed HARD
attempts retain the shared-bus silence floor through cancellation and result take.
No lock spans a wait. End/rebind obey the existing debt/lifetime rules.

## Acquisition and cached meaning

A complete status pass is exactly 31 frames in eight terminal callbacks: leading
BMCR/two BMSR/PHYSTS reads, six separately addressed EEE words, and trailing
BMSR/PHYSTS/BMCR checks. No drain, retry or wait for negotiation occurs. A changing
bookend returns OBSERVATION_CHANGED with ordinary output unchanged. This bounded
check is not simultaneous silicon sampling and cannot exclude an unobserved ABA
change. The selected raw 3/7 allowlist remains only the three non-destructive
capability/local/partner words; raw writes and other offsets reject.

The single collector owns latched EEE status and the read-clear wake counter,
qualified by DEVAD so they cannot alias vendor BMSR/BISCR. Each successfully
consumed sample survives any later error. An ambiguous destructive-read failure
marks possible history loss. Saturated wake-error raw data is a lower bound,
not an exact accumulated error rate. Host sample/history consumption remains
separate from hardware clearing and MDIO/managed-health counters.

| Evidence | Meaning |
| --- | --- |
| desired/trust | Requested policy and stable owned verification; opaque setup additionally needs a completed recipe. |
| capability/local/partner raw words and validity | Distinct observed support/advertisements, including valid negative evidence. |
| status/wakeErrors | Source-implied current/latched/RC words with history retained by the collector. |
| negotiatedValid/negotiated | Applicable coherent AN 100/full proof and local/partner EEE agreement; no current-LPI or power-saving guarantee. |
| stale/timeValid/observedAtUs | Freshness is explicit; old words and time survive incomplete acquisition. |

Observed reset, loopback, isolation, restart-pending state, disabled AN, unknown
capability bits or inconsistent bookends cannot publish negotiated success.
A later link loss/change or contradictory raw advertisement withdraws cached
negotiated validity. Valid contradictory control prefixes dirty trust even if
release or a later read fails. No later independent policy completion can bless
that dirty EEE proof. Cached readers cause no callbacks.
Controller-quarantined refreshes also mark cached evidence stale and negotiated
validity false, retaining raw words and their old timestamp. The BUSY status is
retained without callbacks or ordinary output publication.

[The portable example](../examples/portable/Eee.h) uses the shared owner-loop and
quiet-floor helper. Check Status before using ordinary output, then render any
number of passive copies. Applications coordinate MAC queues, latency and clock
ownership before explicit LPI entry; the PHY has no implied packet buffer.

Q1 remains NOT RUN: record exact part/revision/straps and real DEVAD 3/7 traces,
self-check words, partner agreement, bypass behavior and wake-error access.
Measure MAC/PHY interoperability, wake latency and energy separately. SDK/package
and physical evidence are separate from portable protocol tests.
