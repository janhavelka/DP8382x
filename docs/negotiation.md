# Negotiated link settings and next pages

Prompt 17 implements DP83826I autonegotiated profiles through `startConfigure`
and the existing caller-driven operation slot. It supports every nonempty subset
of documented 10/100 half/full technologies and all four symmetric/asymmetric
pause combinations. MAC flow-control policy remains application-owned. Stage 18 also implements [forced settings and coupled routing](forced-settings.md),
including legal robust/manual/mirror and extension choices.

The [public header](../include/DP8382x/DP8382x.h) owns the shipped declarations.
The [ledger](registers/dp83826i.json),
[TI negotiation audit](reference/library-ti-core.md#negotiation-forced-modes-and-observations)
and [independent vectors](register-vectors.md) supply field/access provenance.
SNLS647I section 8.3.1 and tables 8-19, 8-20, 8-23 through 8-27 and 8-34 define
the underlying PHY controls and observations. Protocol completion, register
verification and physical link qualification are separate evidence.

## Admission and persistent intent

Bind and successfully [initialize](initialization.md) the exact declared PHY
before configuring it. Initialization always performs the mandatory 10BASE-Te
VOD write and verification. Configure does not duplicate that write. Generic
core has no implicit 100/full preference; the
[portable example](../examples/portable/Configure.h) provides that initial board
choice as application data alongside an all-technology profile.

`startConfigure(settings, options, budget, token)` validates the entire request
without a clock or MDIO callback. The shorter overload uses default options:
verified register application without waiting for AN completion. Invalid or
unimplemented settings leave desired state and the caller's token unchanged.
An accepted start copies the validated desired settings and marks trust DIRTY.
Cancelling before the first write retains that intent but creates no hardware
cleanup obligation merely from admitting it.

One application owner serializes every call, including passive snapshots. Each
complete indirect or RMW batch is exclusive across all MDIO participants. The
controller lock is released before scheduled waits and output. `pollOperation`
advances within the absolute deadline, total frames and per-poll slice. A short
slice yields before the entire next batch; the owner returns to its scheduler.
Frozen-clock stalls, total allowance and page limits keep operations finite.

The driver replaces all owned ANAR selector and advertisement fields, including
repairing a prior non-IEEE selector, and prohibits local remote fault. It
preserves target-specific reserved bits and neutralizes commands according to
their exact access rules. It applies the profile before enabling/restarting AN
and verifies stable owned fields. Command-clear is not AN-complete. Ordinary
configuration never enables EEE, WoL or next pages.

`settingsSnapshot()` is bus/clock silent and separates desired intent, complete
observed settings, staleness, trust and the last verification status. At this
stage verification covers ANAR, BMCR, CR1/PHYCR/CR3/CR2 and mandatory
initialization VOD; these are the current owned profile fields.
An earlier mismatch remains evidence even if a later readback fails. An interrupted
verification returns that interruption's status and backend detail; CONFIG_MISMATCH
is terminal only after the complete readback pass. Matching ordinary observations
do not silently become new desired intent.

`waitForCompletion=true` requests a bounded AN-completion wait. The result's
`negotiationComplete` is true only after observing BMSR AN-complete. Successful
application without that option can return while negotiation remains pending.
The reset-scoped `commandRequired`, `commandAccepted` and `protocolComplete`
flags remain false for CONFIGURE and initialization NONE, including AN replay.
Neither outcome alone proves link-up, resolved pause policy, usable network
traffic or electrical qualification. Current link publication belongs to later
link-observation work.

## Typed next-page epoch

`ConfigureOptions.nextPages` contains a fixed array of up to 16 typed `NextPage`
values, a `pageCount` and a finite `maxPartnerPages`. Each page carries an
11-bit `data` value, `message` and `acknowledge2`. Toggle is read-only hardware
evidence, not caller input. Nonzero `pageCount` enables NP only for that exchange
and requires bounded AN completion regardless of `waitForCompletion`.

Supported local message sequences are:

| Message | Following unformatted payload |
| --- | --- |
| Null, code 1 | None |
| Remote fault, code 4 | One page, value 0 through 3 |
| OUI, code 5 | Four pages |
| PHY identifier, code 6 | Four pages |

Invalid payload widths, incomplete message groups, reserved/unsupported messages
and excessive page counts are rejected together before mutation. Technology
specific and extended-next-page message codes are not exposed as guessed
generic payload support. The hardware transmits the page toggle; the driver
does not write a sampled toggle back as a command.

The exchange owns destructive ANER acquisition so another public collector
request cannot steal page evidence while it is active. Each consumed valid
sample is retained immediately. `OperationResult.nextPage` retains requested,
base-page event receipt, partner refusal, completion and possible remaining NP
flags. `baseReceived` records the consumed event; `basePageValid` separately
records a returned base-page payload. `receivedPages` counts consumed next-page
events, including duplicates and missing payloads; each retained page has its
own validity/time. `transmittedPages` counts completed local/generated page-load
writes, even if release or the post-callback deadline later fails. It does not
prove partner acceptance. `duplicatePages` counts repeated-toggle payloads. A failed later read does not erase earlier
consumed pages. Ambiguous destructive reads may have lost history and are not
reported as zero received events. Missing/duplicate page evidence cannot be
invented into a completed exchange.

The partner may decline next pages. That is a protocol outcome, not a transport
ACK claim. The finite exchange reports the observed refusal and completion
evidence without pretending that requested pages were accepted: ordinary AN can
complete while `nextPage.complete` remains false and `partnerRefused` is true.
Re-AN while
NP remains set and no new page is loaded is pending/stalled negotiation, not a
silently inferred down state. Physical Clause 28 handshake case Q10 remains
NOT RUN until an admitted peer fixture and MDIO traces establish it.

## Partial effects and restoration

On complete exchange success, the driver clears ANAR NP without restarting AN.
Persistent `Advertisement` never stores NP. Initialization/reset replay writes
the persistent profile with NP clear and restarts AN; it never resumes an old
page epoch. Raw ANAR writes cannot manufacture persistent desired settings.

A failure, timeout, late write or cancellation terminates immediately without a
hidden cleanup write. The retained result records accepted/possible effects,
partial observations and whether NP may remain set. OP-9's `cleanupOwed` barrier
survives result consumption. The owner retains that result and admits a separate
`startInitialize`, or an eligible configure retry when CONFIGURE caused the
barrier, with a fresh budget/token. That operation replays NP clear and restarts
AN. Its success cannot replace the original failure. If restoration is
impossible, follow initialization's explicit external-disposition and
`abandonCleanup` contract.

The [portable helpers](../examples/portable/Configure.h) show local profile
creation, admission, one scheduled poll and single result consumption. Use a
separate [initialization attempt](../examples/portable/Initialize.h) for
restoration. Host tests and a linked portable consumer establish software
behavior only; SDK/controller timing, real advertisement readback and physical
negotiation remain separate qualification layers.
