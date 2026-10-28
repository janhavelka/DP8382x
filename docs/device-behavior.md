# DP83826I core silicon behavior

This is the prompt-3 research contract for **DP83826I / SNLS647I**, including
DP83826IRHBR. No driver is implemented or physically qualified. The
[ledger](registers/dp83826i.json) owns numeric definitions, ordered procedures,
source citations, ownership and gates. [Independent vectors](register-vectors.md)
are the arithmetic oracle; [coverage](feature-coverage.md) assigns later work.
The [source audit](reports/implementation/3-source-audit.md) records actual
manufacturer/upstream and sibling revisions and licenses.

## Identity and admission

Retain the declared orderable part, both raw ID words, OUI, model, revision,
operating mode and accepted capabilities separately. BASIC and ENHANCED are
observable model distinctions. Shared IDs do not establish an I/E temperature
suffix, A-generation support or interchangeable packages. Revision 1 is the
documented original-part baseline; another revision needs an explicit evidence
and acceptance decision, not a wildcard mask copied from Linux.

Both ID reads must succeed before publishing a new identity. Preserve previous
output on a failed second read, record transport failure and make previous
evidence stale. All-zero/all-one samples fail plausibility; they do not prove
that hardware is absent. Reject invalid variants and unsupported interfaces
before I/O from the declaration or valid cached capabilities. Live identity
validation is a separate bounded acquisition. PHY protocol address range and
the three physical strap address bits are different constraints.

## Register and observation ownership

One application owner serializes every call, including cached snapshots. The
controller lock spans each complete indirect or RMW transaction across all bus
participants, including SDK access and other PHYs. REGCR/ADDAR cannot be used by
an independent raw client. Reject invalid Clause 22 register numbers before any
callback; a reset mask is never a register address. Indirect addressing does
not imply native Clause 45 support.

Every selector phase must succeed before the next phase. A failed selector write
can leave the old target selected; do not continue to a data operation. The next
explicit transaction re-establishes all selectors. No selector cleanup write is
needed. A failed or late data write may already have applied; preserve uncertainty
and invalidate affected configuration trust. Do not retry automatically.

Ordinary read results are staged until success. Destructive samples are retained
immediately, even if the aggregate subsequently fails. Failed destructive reads
can lose unreturned history and must say so. One collector owns BMSR latched
link/fault, ANER page events and the related PHYSTS mirrors. PHYSTS read-clear
labels do not mean every flag clears on a PHYSTS read: its field descriptions
name RECR, 10BTSCR, FCSCR, ANER, MISR and BMSR as different clearing owners.
Signal/descrambler RC observations are retained too. A diagnostic dump uses
cached destructive evidence unless explicit collection is admitted.

Mixed-register writes preserve reserved stable state and compose every command's
neutral value. Reset/restart W1S fields are zero except the requested command.
The schema also represents W0C/W1C for the later event/wake audit; W0C neutral
is one, not zero. No current core field is invented as W1C merely to exercise
that vocabulary. ANAR remote fault is prohibited despite R/W notation; partner
remote-fault observations remain legitimate.

## Reset domains and readiness

| Operation | Established effect | Strap treatment | Persistent state to reconcile |
| --- | --- | --- | --- |
| BMCR PCS reset | PCS registers reset; vendor-specific registers retained | Software-reset prose reloads captured values, not a new pin sample | Revalidate/replay owned standard control, advertisement and next-page state. Exact full PCS domain remains `pcs_reset_extent`. |
| PHYRCR hard reset | Full reset, described as equivalent to reset pin | `hard_reset_straps`: software-reset prose versus pin-equivalence wording leaves external resampling ambiguous | Invalidate all managed configuration; re-identify/read straps and replay persistent state, including 10BASE-Te initialization. |
| PHYRCR circuitry restart | All circuitry except registers reset | No new physical resampling established; do not claim a strap change | Retain desired register state, invalidate live link/diagnostic observations, reverify if uncertain. |
| External RESET_N | Application drives the pin; eligible straps resampled | ENHANCED Strap1 and Strap6 are explicitly POR-only | Re-identify/address-check, read straps and replay persistent state. |
| POR | Application owns power and clock sequencing | Loads hardware mode and all applicable straps | Full initialization and replay after documented SMI readiness. |

Do not generalize ENHANCED POR-only exceptions into an unsupported BASIC reset
claim. BASIC speed/isolation straps have their own tables. The BASIC/ENHANCED
pinout choice is a hardware configuration, not a software mode setter. Read SOR
as captured straps/loaded configuration, not as a mirror of later software
changes. Its printed defaults are not invariant values on every board. PHYSTS's
published reset value is also not a universal post-strap observation: its forced
speed/duplex descriptions conflict with those default bits. Retain the published
value as source evidence, never as a persistent configuration readback assertion.

Software reset completion is the relevant self-clearing command becoming zero
at its actual register. Another active reset command still prevents readiness.
AN restart becoming zero means initiation, not completed negotiation or link.
No inspected software-reset field gives a maximum completion time. Use an
application aggregate deadline and a finite poll count, separate from the
specified external-pin/power timing. Nominal signaling times are not maxima.

The application establishes valid power/clock/reset history before management
access. The ledger records minimum pin pulse, power-up and deassert-to-SMI waits
with their source qualifiers. Anchor any minimum post-action wait at observed
completion or a conservative later timestamp. Release the controller lock before
waiting. A timeout, cancellation or teardown never retroactively undoes an
accepted reset. Retain the primary result, pending effects and cleanup result.

## Initialization and link configuration

Apply the exact VOD_CFG3 recipe before permitting 10BASE-Te, including through
advertisement. Reapply after a full reset; PCS reset and circuitry restart retain
vendor register state according to their stated domains. Readback and trust must
still reflect external/raw mutations. Table 8-81 marks both termination-value
fields and their enables read/write, consistent with the required initialization.
Reserved bits still cannot be tuned arbitrarily. Other analog-control procedures
remain assigned to prompts 5 and 35; there is no read-only-field conflict here.

Advertise any legal nonempty subset of 10/100 half/full capabilities, subject to
the application policy. The initial board's 100/full-only choice is not a core
restriction. Validate the entire technology selector, local remote-fault
prohibition, pause and next-page settings before mutation. Apply advertisement
and companion settings before enabling/restarting negotiation. Keep extended
full-duplex compatibility opt-in and coordinated with the partner; it can change
the ordinary IEEE result with a forced 100M partner.

Forced 10/100 half/full configuration uses BMCR with AN disabled. CR1 requires
robust crossover in forced operation. PHYCR chooses automatic versus manual
pair selection; CR3 pair and polarity swap are separate controls, with both used
for port mirroring. Apply the complete coupled settings under operation admission,
verify stable masks and retain partial effects if any write fails.

PHYSTS's forced-mode speed/duplex wording conflicts with BMCR's programmable
controls, including BASIC strap-latched descriptions. Preserve verified
**configured** mode and raw status separately. Do not publish these PHYSTS bits
as universally resolved forced mode until `forced_physts` closes. Prompts 6,
18, 19 and 39 must share that disposition across the portable and SDK paths.

Next-page support includes local message/unformatted code, more-pages and ACK2,
hardware-controlled toggle, partner ACK/toggle/page fields and consumed page
evidence. Reserved message codes cannot be transmitted. Each unformatted-page
series requires a preceding message page defining its use; referenced pages must
follow immediately in the defined order. Bound page count and time,
including null pages while the peer continues. Preserve a consumed event with missing
payload if the subsequent partner read fails. Protocol message interpretation
belongs to the relevant IEEE/application contract; receiving an unrecognized
raw code must not silently turn it into a supported operation. EEE page/address
research remains assigned to prompt 5. IEEE's restart state clears the next-page
loaded flag: prepare desired data locally before restart, but load ANNPTR after
fresh base-page receipt. Distinguish BASE and NEXT events, retain received payload
before advancing, and finish only when both sides are done. The
`next_page_handshake` gate requires exact DP83826I confirmation of these event/load
boundaries before enabling custom exchange; the selected IEEE sequence is not a
measured PHY result. A peer declining next pages is not a transport failure.

Pause RX and TX resolve separately, only after AN selects a full-duplex common
technology. The ledger records the independently reviewed complete truth table.
Half duplex, forced mode, incomplete negotiation and stale link evidence do not
establish negotiated pause. The SDK's Boolean projection is a separate adapter
concern; it does not replace the portable two-direction result.

## Finite replay and failure handling

The accepted procedures in the ledger include prerequisites, ordered steps,
owned masks, waits, timeout policy, partial effects, cleanup and restoration.
Their planned API/CLI names express ownership, not already implemented signatures.

After full reset, first establish identity/address/mode and readiness; then apply
required initialization and persistent interface/clock/power dependencies, coupled
crossover/compatibility, advertisement/pages, AN or forced activation, and event
enables. Later power/EEE procedures can require their own reset-bearing ordering;
prompts 4–5 must extend this dependency order explicitly. Do not replay temporary
diagnostic settings, sampled commands or consumed events as desired state.

Each frame is bounded and terminal. The aggregate budget includes admission,
lock acquisition and controller execution; a finite frame/poll limit still applies
if the clock freezes. Zero work budget performs no I/O. Cancellation/cleanup uses
its separately admitted finite allowance and does not overwrite the primary
failure. Incomplete replay is not configuration trust; successful register access
is not link or application readiness. Cached snapshots perform no I/O.

## Gates and future public documentation

The ledger names every conflict, selected conservative behavior, owning stage
and next evidence. A research record can be complete with such a gate; the affected
implementation/qualification remains open. Physical qualification is NOT RUN.
No fabricated measurement or passing schema test closes a silicon conflict.

From the first C++ declaration, Doxygen must cite the exact ledger/source table
for non-obvious access or sequence rules and document units, legal states,
ownership, side effects, output validity, failure uncertainty and bounds.
No placeholder C++ is needed to record this research. Vendor bytes/notices remain
unchanged; factual GPL comparisons do not authorize importing GPL implementation.
