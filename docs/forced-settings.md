# Forced modes and coupled crossover settings

Prompt 18 extends the same `startConfigure` operation and initialization replay
used by [negotiated profiles](negotiation.md). Forced mode configures the complete
local speed/duplex/routing profile. A BMCR-only write does not establish Force
Mode, and a successful apply does not prove partner duplex or physical link.

The [public values](../include/DP8382x/Config.h) describe persistent intent;
the [driver header](../include/DP8382x/DP8382x.h) defines admission, snapshots and
retained results. The [ledger](registers/dp83826i.json) is the numeric/access
catalogue. Original SNLS647I sections 8.3.2 and 8.5.1.10, tables 8-19 and
8-28 through 8-30, and the PHYCR definition govern the coupled settings.
Independent literals belong in [register vectors](register-vectors.md).

## Legal profile matrix

| Profile | Speed/duplex | Robust crossover | Pair selection | Port mirror | Extended full duplex |
| --- | --- | --- | --- | --- | --- |
| Negotiated | Nonempty advertisement of documented 10/100 half/full technologies | Enabled or disabled | Automatic, MDI or MDIX | Off or on | Explicit off/on opt-in |
| Forced 10 | Half or full | Required enabled | Automatic, MDI or MDIX | Off or on | Must be off |
| Forced 100 | Half or full | Required enabled | Automatic, MDI or MDIX | Off or on | Explicit off/on opt-in |

`swapPair` and `swapPolarity` encode one port-mirror request: both false means
normal and both true means mirror. Mismatched values reject before callbacks.
The source supports the coupled mirror control, not two independently justified
corrections. Automatic PHYCR selection preserves its dormant manual-select bit;
manual selection writes the complete MDI/MDIX selector. Robust remains enabled
for every forced profile, including manual routing.

Extended full duplex is an application opt-in. It changes the IEEE result
against a forced-100 partner by permitting the extended full-duplex behavior;
it does not prove the partner's duplex. The driver does not invent a partner
observation prerequisite. With extension disabled, parallel detection does not
gain that extension. Forced operation and parallel detection do not establish
negotiated pause; MAC flow-control policy remains outside the PHY core.

## Admission, application and restoration

Initialization is required and already performs mandatory 10BASE-Te VOD setup
for every selected reset kind. Configure does not issue a second VOD write.
The full request is validated before bus/clock callbacks or desired-state change.
Forced profiles carry no advertisement, pause, next-page exchange or AN-completion
wait. A forced operation neither writes nor verifies ANAR. Returning to AN needs
a newly supplied complete advertisement; the core never silently falls back to
a previous speed or invents missing negotiation intent.

Application saves touched fields, applies the complete source-ordered coupled
profile and verifies stable owned bits. RMW preserves each target's reserved and
unrelated controls with its documented neutral composition. Robust enable
precedes forced-mode activation. A requested robust disable is deferred
until AN has been enabled, so a forced-to-AN transition does not disable it while
the prior forced profile still controls BMCR. Forced verification does not wait
for an ignored AN-restart bit to clear.

The controller lock covers an indivisible transaction, not the whole asynchronous
operation. A short
poll slice yields before a complete batch; all work shares the absolute operation
deadline and frame allowance. No retry or lock retention crosses a scheduled wait.

Admitted desired settings remain authoritative after cancellation or failure.
Observed values and verified trust stay separate. One successful read cannot
clear uncertainty or an earlier mismatch in another field. Partial apply,
accepted-write timeout, cancellation and readback failure retain effect evidence
and OP-9's cleanup obligation. Restoration is separately admitted through
`startInitialize`, with a new token/budget/result, and replays every owned routing,
mirror, extension and mode field through the same implementation. Reset replay
does not restore a transient next-page epoch.

## Owner example and qualification

[Forced.h](../examples/portable/Forced.h) provides a manual-MDI forced-100/full
application profile and uses the existing configuration owner loop. It is
separate from the initial board's 100/full-only AN profile: AN advertises a
capability, whereas forced mode selects local controls without partner negotiation.
The caller must check terminal Status and retain failures before deciding on
restoration. Cached settings and operation snapshots do no bus or clock I/O.

Host tests establish literal composition, all legal routing combinations,
transitions, failure/cancel boundaries, complete readback and reset replay.
They do not establish hardware partner compatibility, robust manual routing,
mirror interaction, cable wiring or electrical timing. These remain physical
Q11 evidence. Loopback restoration tests belong to prompt 23 when that feature
exists; no loopback support is implied by this configuration procedure.
