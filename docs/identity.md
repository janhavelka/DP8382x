# Exact identity and foundational capabilities

DP83822I, H, IF and HF accept the same two words, `2000 A240`, at
revision 0. Declare copper, MII/RMII/RGMII, and UNKNOWN operating mode.
The portable probe retains partial reads and permits an explicit retry.
A successful probe also admits `captureDp83822Straps` for both strap words.
`strapSnapshot()` exposes word validity, staleness and the completion timestamp.
The probe reports documented 10/100 modes and interfaces. Only
the IF and HF declarations report fiber capability. Startup, raw register
access, the collector and native adapter remain unavailable for these
declarations. The probe requires board-established SMI readiness and a
declared MDC rate of at most 25 MHz, or zero to use the board timing
contract. [SNLS505H sections 4, 6.9 and tables 8-3/4](reference/source/TI-DP83822-SNLS505H.pdf)

DP83825I requires ID1 `2000` and ID2 `A140` (SNLS638C table 7-6).
Its model is 20, revision is zero, and the operating-mode field stays
`UNKNOWN`. The two other revision-1 models, the A grades and the Linux label
`A150` all fail this declaration. A partial or rejected probe retains each
returned word. Only direct ID observation can proceed until a successful
probe or a new binding. Strap discovery covers addresses 0 through 3.
[Exact-part sources and discrepancy](variants.md)

DP83826AI and DP83826AE require ID1 `2000` with ID2 `A114` (BASIC) or
`A134` (ENHANCED), as specified in SNLS783A tables 8-22/23. A contradictory
mode or revision retains the observed identity and ends initialization before
selector writes. If a later probe times out after ID1, the identity snapshot
retains that partial read. The earlier rejection still blocks feature reads and
MMD selector writes until a complete matching probe or a new binding.
The application declaration distinguishes the grades.

During a running initialization, identity reads use OP-14 admission: reset WAIT
rejects them locally; other between-poll reads remain admitted. A failed or
changed live identity freezes that initialization before another mutation.
Reset/invalidation withdraws old permissions. Initialization refreshes identity
after PCS and only publishes readiness after mandatory verification. See
[initialization](initialization.md).

`readIdentity(output, budget)` reads PHYIDR1 then PHYIDR2 at the bound address
in one owned two-frame Clause 22 batch. It requires binding, an aggregate budget
of at least two frames and application-established clock/power/reset readiness.
It performs no scan, reset, retry, selector write or implicit address selection.
Use the [linked portable probe example](../examples/portable/IdentityProbe.h).

SNLS647I tables 8-21/22 define the identity fields. Bind the ordering part
from the board BOM. Both DP83826I and DP83826E accept the original model words
at revision 1. Each successful observation carries the supplied declaration,
including when two instances read identical words. Linux C/NC labels describe
models. The shared source record identifies the ordering-grade differences.
[SNLS647I evidence](reports/implementation/56-e-support.md#shared-silicon-record)

| Declared part / required mode | Observed words | Admission | Implemented facts / hardware evidence |
| --- | --- | --- | --- |
| DP83826I or E / UNKNOWN or BASIC | `2000 A111` | Accepted BASIC revision 1 | Two-frame acquisition, passive snapshot/capabilities host-tested; physical NOT RUN |
| DP83826I or E / UNKNOWN or ENHANCED | `2000 A131` | Accepted ENHANCED revision 1 | Same implemented path; physical NOT RUN |
| DP83826I or E / contradictory required mode | Either valid ID above | DEVICE_MISMATCH / MODE_MISMATCH | Reads retained; output unchanged, capabilities invalid |
| DP83826I or E | Recognized model with revision 0 or 2–15 | DEVICE_MISMATCH / REVISION_NOT_ACCEPTED | Raw revision retained; no guessed support for A-generation |
| DP83826I or E | Wrong OUI or model | DEVICE_MISMATCH / WRONG_OUI or WRONG_MODEL | OUI checked before model, then revision and required mode |
| DP83826I or E | Either required word zero | DEVICE_NOT_FOUND / ZERO_ID | Plausibility failure, not evidence of physical absence |
| DP83826I or E | Either word FFFF, neither zero | DEVICE_NOT_FOUND / ONES_ID | Controller data retained; no guessed MDIO acknowledgement |
| DP83825I / UNKNOWN | `2000 A140` | Accepted revision 0 | Model 20, copper RMII, address straps 0..3, physical pending |
| DP83825I / UNKNOWN | `2000 A141` through `A14F`, or Linux I label `A150` | DEVICE_MISMATCH | Preserve raw ID and allow direct identity reprobe |
| UNKNOWN or unimplemented relative represented by an invalid enum cast | No read | INVALID_CONFIG / ENUM_VALUE at bind | No automatic family selection, no callbacks |
| DP83826I or E / RGMII declaration | No read | UNSUPPORTED / INTERFACE at bind | Exact part has no RGMII; MII and RMII are both legal in both modes |

Numbers above are hexadecimal. The OUI is `(ID1 << 6) | ((ID2 & FC00) >> 10)`,
model is `(ID2 & 03F0) >> 4` and revision is `ID2 & 000F`. The independent literal
tests include both OUI portions and every model/revision encoding. Protocol PHY
addresses 0–31 are valid inputs, separately from the part's three address straps.
Reading a plausible ID at a configured address does not locate or select a device
elsewhere on the bus, nor prove how that address was strapped.

## Ordinary output and retained diagnostics

The caller's output changes only after complete, accepted, in-budget success.
A failed second read preserves the first in `identitySnapshot()` with individual
validity flags. No call combines an old first word with a new second word.
Transport errors and signed backend details take precedence over identity
classification. Release, report and deadline failures cannot become successful
identity merely because two plausible words were returned.

Two independently valid words can be decoded on terminal failure, but acceptance
remains UNASSESSED and stale. On complete transport success, semantic rejection
is REJECTED with the exact reason, while the fresh rejected data remains visible.
The complete-acquisition time is the conservative post-callback upper timestamp;
time validity is separate from acceptance and invalid timing is never stamped.
Malformed prefix continuation cannot produce trusted later words.

Local admission rejection and valid zero-frame BUSY/OPERATION_TIMEOUT with clean
release preserve the previous identity. Other attempted acquisitions replace it
with their own partial/rejected evidence. A failed probe or changed complete ID
marks prior device observations stale and configuration dirty. An accepted new
ID can establish new capabilities but cannot verify old settings or event history.

`identitySnapshot()` and `capabilities()` copy cached values without MDIO or clock
callbacks. Rebind/end clear identity. External invalidation and semantic writes
retain historical bytes/assessment but mark identity stale, withdrawing capability
validity. The owner must report external resets, power cycles or mode changes
through `invalidateDeviceState`; the driver cannot discover an unreported event
from a bus-silent snapshot.

## Capability and maintenance boundaries

A fresh accepted DP83826I or DP83826E reports all four 10/100 half/full link modes, MII and RMII,
and copper medium. Fiber, gigabit and RGMII are absent. Table 4-3 and BASIC strap
table 8-12 support both interfaces in both identity modes. Actual wiring/strap
selection is separate evidence; identity alone does not read or qualify it.

These foundational fields describe source-supported silicon after implemented
identity admission. They do not claim complete typed feature software, initialized
hardware, link-up, managed recovery, SDK support or qualification. Invalid
capabilities mean unknown, not unsupported. The [feature matrix](feature-coverage.md)
tracks implemented and qualified layers separately.

Existing owner-attested LEDCR/MISR/WoL maintenance before the first probe remains
available. Once a probe fails or is rejected, all five maintenance write routes
return its retained status before callbacks, including WoL's preharvest reads.
Invalidation does not erase that rejection; accepted reprobe or explicit rebind
does. A stale previously accepted identity still allows this narrow maintenance
exception, while capabilities remain invalid. It cannot authorize future typed
feature writes that require fresh identity and initialization.

Identity uses the same `_execute` admission path as raw reads. OP-14 rejects it
during a reset wait or a pending hard-reset quiet floor, including after a failed
or cancelled reset. The application owns external readiness and shared-bus silence;
the core enforces these operation guards at its transaction boundary.

Prompt 16 extends the same admission guard with a persistent hard-reset floor.
HARD withdraws mode-derived permissions and reaccepts identity after completion;
RESTART retains register identity while refreshing live observations. An observed
already-active configuration reset also requires refreshed identity before the
requested reset command. Reconciliation stays at the configured PHY address;
changed reachability returns its actual failure without implicit scanning.

Native explicit discovery uses core `readIdentityAt(address,id1,id2,budget,invoked)`.
This raw two-word probe accepts addresses 0..31, requires an idle binding without
pending results/cleanup/temporary state, and never retargets or accepts identity.
Both ID outputs remain unchanged on failure. The optional invoked flag is always
set, including local rejection; only true identifies a new transport lastBatch.
Cumulative batch/frame statistics and clock faults use the normal core boundary,
while semantic collectors and managed health remain unchanged. Discovery may
retain successful partial raw ID words separately from ordinary outputs.
