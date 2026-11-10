# Live and cached link observations

`readLinkStatus` acquires a bounded DP83826I observation at the configured address.
`linkSnapshot` copies the retained value without MDIO, clock callbacks, event
acknowledgement or age calculation. Serialize both through the application owner.
The [linked example](../examples/portable/LinkStatus.h) separates sampling from
the application's optional 100/full and maximum-age policy.

## Admission and acquisition

Initialization and accepted identity are required; verified settings are not
required merely to observe link. OP-14 admits the call between operation polls
and while a result is pending. It rejects callback reentry, reset WAIT, the
persistent hard-reset quiet floor and an active typed next-page exchange. The
last restriction prevents stealing ANER page evidence from the exchange owner.
The backend must honor the shared controller owner across every PHY address.

Success uses exactly ten C22 reads in two terminal batches:

1. BMSR(first), BMSR(current), PHYSTS(first), BMCR, ANAR, ALNPAR, ANER, PHYCR.
2. BMSR(final), PHYSTS(final).

Admission reserves ten frames and both callback bounds under one absolute
deadline. Each batch rechecks its remaining allowance. The lock is released
when each callback returns; there is no wait, retry or resampling loop. All
destructive receipts use the existing collector. A later failure cannot erase
the successfully consumed prefix, and an ambiguous destructive failure marks
possible history loss. The first BMSR retains latch-low history; the second
supplies current link. PHYSTS precedes ANER's dependent clearing effects.

Current/final BMSR and both PHYSTS words must agree on link and AN completion,
including agreement between BMSR and PHYSTS at each bookend. Applicable resolved
PHYSTS mode must also agree. Detected change returns BUSY/OBSERVATION_CHANGED,
leaves ordinary output unchanged and marks the previous aggregate stale.
The independent [literal vectors](register-vectors.md#prompt-19-link-observation-vectors)
give exact masks. This checks consistency, not simultaneous sampling: a change
which returns to the same state between observations can remain unseen.

## Meaning and validity

| Evidence | Meaning | Separate concern |
| --- | --- | --- |
| `valid && !stale`, `linkUp=false` | Successful current DOWN observation | Cable down is not MDIO failure |
| UP, negotiation IN_PROGRESS | Link bit observed; mode may be unresolved | No implicit negotiation wait |
| `resolvedModeValid` | Applicable local speed and duplex | Forced mode does not prove partner duplex |
| `pauseValid` | Qualified IEEE pause result agrees with PHYCR | MAC flow-control policy is application-owned |
| `stale`, old values/time | Previous coherent observation is historical | No synthetic DOWN or fresh timestamp |
| `lastRefreshStatus` and transport receipt | Acquisition/error evidence | Managed health arrives in prompt 21 |
| Application 100/full/age policy | Owner decides whether the value is usable | DHCP, IP, EtherCAT and machine readiness are not PHY fields |

With AN disabled, speed and duplex come from BMCR; PHYSTS mode bits remain raw.
Forced 100/half is unresolved unless prior whole-profile VERIFIED evidence proves
CR2 extension zero and extension is not desired. Forced full and forced 10 use
their BMCR values. Forced mode never establishes pause. Strap-forced BMCR
readback remains source-implied pending Q11 measurements.

With completed AN, PHYSTS supplies mode unless ANER reports parallel-detection
fault. Parallel full duplex requires verified CR2 extension and a 100 Mbit/s
result; it has no negotiated pause. Normal pause additionally needs an AN-capable
partner, IEEE base-page selectors, and a highest-common full-duplex technology
matching observed speed. IEEE Table 28B-3 determines local RX/TX; PHYCR is a
cross-check. Disagreement invalidates pause while retaining both advertisements,
PHYCR and the independently qualified mode. Half duplex, pending AN and forced
mode keep pause invalid, including when numeric defaults happen to be zero.

During a running operation after an attempted/possible semantic write, or while
temporary settings may be active, a coherent link bit can still be published,
but mode and pause remain unresolved. Writes, reset, external invalidation and
changed identity make prior observations stale. Rebind/end clear binding-local
history. No configuration, SDK notification or automatic recovery is performed.

## Time, retained changes and failure

`observedSinceUs` is the raw clock lower bound before acquisition;
`observedAtUs` is the completion upper bound including clock quantization lag.
Both belong to the binding's monotonic microsecond domain. `timeValid` explicitly
qualifies the interval; zero is a valid time. For a conservative maximum age use
the owner's current upper time minus `observedSinceUs`, after checking order and
overflow. Subtracting the completion upper bound could understate sample age.
The example accepts a supplied current upper time and does no clock reads.

An admitted failed refresh, including insufficient frames or aggregate time,
preserves old values and interval, sets stale and retains its exact error.
Local owner/lifecycle admission rejection does not manufacture an acquisition.
A clock regression/overflow detected anywhere in the driver invalidates cached
link use with CLOCK_CONTRACT; old time remains historical and cannot be compared
to the failed domain. Partial collector samples keep their independent timestamp
validity even when ordinary aggregate publication fails.

`change` reports FIRST_VALID, CHANGED, REPEATED, STALE or REGAINED. The saturating
`observations` count advances on every coherent publication. `changes` counts
first validity, semantic tuple changes and fresh/stale transitions. Repeated
failure while stale adds no transition. Raw latch history and timestamps are
excluded from semantic equality. Cached readers neither consume nor alter these
values. This retains change evidence for later adapter polling; it is not an
event queue or a guarantee that every physical flap was observed.

## Evidence and limits

[Original-source review](reports/implementation/19-source-review.md) covers
SNLS647I tables 8-19/20/23/24/25/29/34/43 and IEEE 802.3 Table 28B-3.
[Host tests](../test/link.cpp) inject each read failure and read-boundary change,
exercise independent pause literals, clock/budget boundaries and shared ownership.
The [stage report](reports/implementation/19-link-observation.md) separates host,
portable-consumer, SDK and physical results. Q11, real cable/partner behavior,
MDIO timing and hardware qualification remain NOT RUN. The `forced_physts` gate
is a nonblocking raw-value question; it does not authorize forced PHYSTS decoding.
