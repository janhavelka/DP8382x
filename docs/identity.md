# Exact identity and foundational capabilities

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

SNLS647I tables 8-21/22 define the identity fields. The declared part must be
DP83826I; observed OUI/model/revision must match its private accepted-revision
table. The current policy accepts revision 1 only. This is a reviewed admission
policy, not proof that no other revision exists. BASIC and ENHANCED IDs do not
identify I/E suffix or temperature grade: the application supplies the ordering
part. Linux C/NC labels are not ordering-part aliases.

| Declared part / required mode | Observed words | Admission | Implemented facts / hardware evidence |
| --- | --- | --- | --- |
| DP83826I / UNKNOWN or BASIC | `2000 A111` | Accepted BASIC revision 1 | Two-frame acquisition, passive snapshot/capabilities host-tested; physical NOT RUN |
| DP83826I / UNKNOWN or ENHANCED | `2000 A131` | Accepted ENHANCED revision 1 | Same implemented path; physical NOT RUN |
| DP83826I / contradictory required mode | Either valid ID above | DEVICE_MISMATCH / MODE_MISMATCH | Reads retained; output unchanged, capabilities invalid |
| DP83826I | Recognized model with revision 0 or 2–15 | DEVICE_MISMATCH / REVISION_NOT_ACCEPTED | Raw revision retained; no guessed support for A-generation |
| DP83826I | Wrong OUI or model | DEVICE_MISMATCH / WRONG_OUI or WRONG_MODEL | OUI checked before model, then revision and required mode |
| DP83826I | Either required word zero | DEVICE_NOT_FOUND / ZERO_ID | Plausibility failure, not evidence of physical absence |
| DP83826I | Either word FFFF, neither zero | DEVICE_NOT_FOUND / ONES_ID | Controller data retained; no guessed MDIO acknowledgement |
| UNKNOWN or unimplemented relative represented by an invalid enum cast | No read | INVALID_CONFIG / ENUM_VALUE at bind | No automatic family selection, no callbacks |
| DP83826I / RGMII declaration | No read | UNSUPPORTED / INTERFACE at bind | Exact part has no RGMII; MII and RMII are both legal in both modes |

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

Fresh accepted DP83826I reports all four 10/100 half/full link modes, MII and RMII,
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

Identity uses the same `_execute` admission path as raw reads. Reset operations
do not exist at stage 14: the application currently owns readiness and bus silence.
Stage 15 must add OP-14 reset-wait/hard-reset-floor admission and its tests at this
shared boundary when it introduces real reset operations. No unused reset-state
scaffolding or claim of enforced future operation states is supplied here.
