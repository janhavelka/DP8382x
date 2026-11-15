# DP8382x value and direct-call contracts

Public headers own the shipped API. This contract specifies future behavior
introduced with its owning implementation stage. The OP-1 through OP-17 rules
below supersede prompt 8's earlier cooperative design under the remaining-prompts
audit. Stage 13 retains the latest user-requested concrete destructive-acquisition
and mixed-register consumers; no test-only public collector facade is added.

Read the exact-part [ledger](registers/dp83826i.json), [vectors](register-vectors.md)
and [native ownership design](design-decisions.md). Silicon facts belong in the
ledger; shipped names and declarations belong in headers. Core remains C++17,
fixed-size and application-serialized, with no allocation, SDK, clock or RTOS owner.

## Status and error details

[Status.h](../include/DP8382x/Status.h) defines Status, Err and per-code details.
OK alone is success; IN_PROGRESS means admitted work. Preserve signed backend
detail, static non-null messages and raw invalid reports. Local details never
overwrite backend errors. Unknown enums reject before callbacks. Messages are
diagnostic text, never branch keys. CONFIG_MISMATCH uses a separate typed record.
No unused BOARD_PREREQUISITE, LINK_MODE, FEATURE, TOKEN_EXHAUSTED or RETIRE_REQUIRED
detail is introduced. A 64-bit object-lifetime token needs no exhaustion branch.

## Identity, capabilities and link intent

[Config.h](../include/DP8382x/Config.h) defines shipped variant/interface/link
values and pure validation. [DP8382x.h](../include/DP8382x/DP8382x.h) now ships
IdentityAcceptance, DeviceIdentity, Capabilities, readIdentity, identitySnapshot
and capabilities. The [identity guide](identity.md) documents current acquisition,
partial evidence, negative-evidence maintenance admission and invalidation.
These declarations are maintained in the header, not duplicated as future APIs.

`isValidVariant` accepts only `DP83826I`; `UNKNOWN` and other numeric casts are
invalid. Later documented parts are **not** prematurely exposed enum values
returning `UNSUPPORTED` for missing software. Add each at its qualification
stage. Physical marking declares I versus E; shared IDs cannot discover the
ordering suffix. `isValidPhyAddress` accepts the protocol range 0..31; the
DP83826's three physical address straps are separate board evidence.

Current source acceptance is explicitly OUI `0x080028`, model `0x11` BASIC or
`0x13` ENHANCED, revision **1 only**, under a private exact-part accepted-revision lookup. This is the
bounded admission decision supported by SNLS647I tables 8-21/22 and the audited
TI filter, not a claim that other revisions do not exist. A new revision needs
source review and a new policy decision. The raw nibble remains 0..15. Do not
default missing ID bits to `0x2000A111`, `0x2000A131` or revision 1.

Complete reads `(0,0)` or `(0xFFFF,0xFFFF)` produce `DEVICE_NOT_FOUND` with
`ZERO_ID`/`ONES_ID`; the same applies when either required word is 0 or 0xFFFF
(zero check precedes all-one check). This is lack of plausible accepted identity,
not proof of physical absence. Other complete words with wrong OUI/model/revision
produce `DEVICE_MISMATCH` with the first failing criterion in that order, then
required-mode mismatch. A failed read remains its transport error. Both raw words
can be retained as rejected evidence while an ordinary identity output remains
unchanged on an unsuccessful higher-level acquisition. Partial raw evidence has
individual validity; `decodedValid=false`, acceptance UNASSESSED.

Accepted DP83826I capabilities are copper, all four 10/100 modes, MII and RMII;
fiber/gigabit/RGMII are absent. These values describe silicon evidence, not
implemented feature coverage or a physical qualification. Future feature
capabilities are added with typed behavior; the feature coverage guide already
covers every documented family. Interface selection and captured SOR straps
are distinct: Config declares the MAC wiring; decoded identity does not prove
the strapped interface. Raw strap words and reset provenance belong to the
initialization result when implemented, never a guessed identity field.

`validateLinkSettings` checks every enum (including inactive fields), mask and
cross-field condition without transport. It returns `INVALID_PARAM/ENUM_VALUE`
for unknown enums, otherwise `INVALID_CONFIG/LINK_COMBINATION` for inconsistent
intent. In AN mode, any nonempty subset of the four technology bits is legal;
all four symmetric/asymmetric pause combinations remain representable. Pause
advertisement is allowed with half-only technologies but cannot resolve pause
without a full-duplex result. Encoding forces selector 1 and local remote-fault
zero while preserving read-only T4 and reserved bits; these are not user-settable fields. Inactive
forced fields must be UNKNOWN; contradictory inactive intent rejects before admission.

Forced mode requires one of 10/100 × half/full, robust crossover true for both
automatic and manual pair policy, and an inactive advertisement of mask zero,
pause false/false. This canonical inactive form prevents
accidental interpretation as negotiated policy. The swap Booleans must match: both
true select coupled CR3 port mirror, both false disable it. Extended full duplex
is an owner opt-in with no partner-evidence preflight; forced10 plus extension
rejects. No Config Boolean proves the partner. Next-page NP=1 belongs only to one admitted exchange epoch; it is not a persistent Advertisement field. Source-gated operations reject before mutation.
Every startInitialize writes and verifies VOD_CFG3=0x4A40 for every ResetKind, including NONE; binding does not apply it.

## Borrowed transport and direct-call budgets

[Config.h](../include/DP8382x/Config.h) is the declaration authority for Config,
Transport, IoBudget, MdioBatch, MdioFrame and frame/batch results. The terminal
callback and detailed frame-report invariants are documented there and in
[direct access](direct-access.md); do not duplicate their declarations here.

One controller-wide lock covers each complete batch, including indirect selector
traffic and RMW. It is released on every exit before waits or output. No selector
state survives a batch. Every controller participant shares fault/silence admission.
The application owns the instance gate, controller lock, borrowed context, bounded
clock and execution callback. A core snapshot cannot authorize another user.

TransportBounds has three positive values: fixedUs, frameUs and clockLagUs.
fixedUs includes acquisition, release and fixed execution/accounting overhead;
the batch bound is fixedUs + frameCount * frameUs with checked 64-bit arithmetic.
Platform scheduler/controller allowances must be proved separately from nominal
SDK polling loops. A terminal callback cannot be preempted by the core.

Time is nonwrapping uint64_t microseconds. nowUs returns a lower bound; raw plus
clockLagUs is an upper bound at the instant the clock was read. clockLagUs bounds
quantization plus source-update latency, excluding scheduling after the read.
A backward sample or overflow forming the upper bound latches CLOCK_CONTRACT;
later larger samples do not repair it. Effects and old timestamps survive, but
new timed I/O/output timestamps are refused until owner repair and fresh binding.
Bus/clock-silent snapshot, result and lifecycle routes remain available.

Before each batch: reject an expired raw deadline as DEADLINE, insufficient
frames as FRAME_BUDGET, or a complete bound that cannot fit strictly before the
exclusive deadline as BOUND_ADMISSION. Whole transactions are admitted, never
partial MMD operations. Subsequent batches share the original deadline and deduct
attempted physical frames; no hidden retry or renewed allowance exists. One C22
access costs one frame, C22 RMW two, MMD read/write four, MMD RMW five. The latter
is inferred from SNLS647I 8.3.11.1 state semantics, not a printed combined sequence;
physical Q4 remains NOT RUN. Fixed capacity is eight frames.

After a callback, sample again. A raw sample at/after deadline proves late and
denies ordinary output; there is no post-callback publication reserve. Preserve
earlier primary/release errors and effects. Valid frame completion stamps are
conservative upper bounds, ordered within the batch's pre/post sample window;
otherwise the valid post-callback upper bound can anchor a completed frame.
An invalid optional stamp does not erase independently valid data/completion.

Dependent writes use a preceding successful read under the same lock, with
explicit preserve mask and literal bits; never replay an unvalidated sample.
Read results form a validated prefix. Successful consumed words are copied into
the collector even if later frame, release, timestamp or deadline validation
fails. A failed consuming read with possible effect records possible history loss.
Ordinary aggregate outputs publish only complete success. Keep raw reports even
when malformed, while only independently valid prefix data may become evidence.

The trusted backend checks descriptors and terminal reports before reopening
shared admission. Invalid/unknown completion, failed release, or uncertain idle
keeps the whole controller quarantined; future calls return zero-frame
BUSY/CONTROLLER_QUARANTINED until owner repair. Such a report cannot claim reusable.
Core also validates and stops its instance on escaped contract faults; end/bind
cannot clear backend quarantine. Callback statuses are terminal and preserve
signed backend detail; IN_PROGRESS/null messages/illegal code or flag combinations
are MDIO_ERROR with raw diagnostic evidence retained.

Error precedence is normalized primary failure, release/unsafe-controller failure,
clock fault, late deadline, then chip semantic result. A late failed frame remains
its MDIO error with late=true. CONFIG_MISMATCH requires complete in-budget stable
owned-bit readback. Timestamp zero is valid when its flag is true. Current lower
time minus observed upper time gives only a lower bound on age; maximum-age
freshness needs an observation lower bound or a separately proved uncertainty.

## Passive observations, trust and accounting

```cpp contract
namespace DP8382x {
enum class DriverState : uint8_t { UNINIT, READY, DEGRADED, OFFLINE };
/// @return Uppercase name or "UNKNOWN"; no I/O, clock read or mutation.
const char* driverStateName(DriverState state) noexcept;
/// @brief Managed MDIO transaction health only; not link/configuration readiness.
struct HealthSnapshot {
    DriverState state{DriverState::UNINIT};
    bool initialized{false};
    uint32_t consecutiveFailures{0};         ///< Saturating, never an admission latch.
    uint32_t totalFailures{0};               ///< Saturating transaction count.
    uint32_t totalSuccess{0};                ///< Saturating transaction count.
    uint64_t lastOkUs{0};
    uint64_t lastErrorUs{0};
    bool lastOkTimeValid{false};
    bool lastErrorTimeValid{false};
    Status lastError{Err::OK, 0, "No managed transport failure"};
};
// LinkSnapshot, NegotiationState and LinkChange are shipped in DP8382x.h.
enum class ConfigurationTrust : uint8_t { UNKNOWN, VERIFIED, DIRTY };
/// @brief Foundational link settings view; no claim to verify future feature fields.
struct SettingsSnapshot {
    LinkSettings desired{};
    bool desiredValid{false};
    LinkSettings observed{};
    bool observedValid{false};              ///< Complete stable link-setting tuple only.
    bool observedStale{true};
    ConfigurationTrust trust{ConfigurationTrust::UNKNOWN};
    uint64_t observedAtUs{0};
    bool timeValid{false};
    Status lastVerificationStatus{Err::NOT_INITIALIZED, 0, "No settings verification"};
};
}
```

Health counts **one managed batch callback invocation** as one transaction,
not one public method or one physical frame. One four-frame MMD callback is one
transaction; a compound observation can contain multiple transactions. Attribute
traffic internally, never with a caller-writable "managed" Boolean:

Managed traffic is initialization, persistent typed configuration, verification
and live link acquisition. Raw access and temporary session stimulus/stop count
only in transport totals. Restoration through startInitialize is managed.

A callback is not counted if zero frames were attempted, normalized transport
status is BUSY or OPERATION_TIMEOUT, raw release is OK, and either the controller
remains reusable or this is a valid BUSY/CONTROLLER_QUARANTINED report. Otherwise
count a failure if normalized transport status or release is not OK, controller
reuse is newly lost, or the report is malformed. Otherwise count a success.
Late successful transfers and post-callback clock faults remain transport
diagnostics; they do not manufacture MDIO failure. Semantic mismatch/down remains
separate. Count at most once per callback.

These rules use returned statuses, release/controller evidence and frame flags;
there is no hidden acquisition-attempt flag. A controller lock timeout returns
MDIO_TIMEOUT and counts, whereas an aggregate-budget rejection before any frame
with successful release and reusable controller does not. Every callback
invocation increases `totalBatches`; frame totals count reported attempted/completed
frames only.
On malformed reports, count only the independently valid prefix and mark the
violation; counts must not be invented from success status alone.
An invalid optional frame timestamp does not erase independently valid physical
completion/value flags: keep the completion count, reject publication/time use
and retain the raw contradictory stamp for diagnosis.

For a counted failure, health lastError records normalized primary
MDIO_ERROR/MDIO_TIMEOUT first, otherwise the normalized release/unsafe-controller
error, with clock/deadline outcome retained separately. This is the cause of the
health count, not necessarily the public call's primary status. For example,
primary BUSY/OWNER_ADMISSION plus release MDIO_ERROR/-37 remains BUSY publicly
but records one health failure with lastError=MDIO_ERROR/-37. Invalid reports
normalize to MDIO_ERROR as above. A valid OPERATION_TIMEOUT after attempted
frames counts as an incomplete managed transaction: if no primary MDIO or
release error exists, its health cause is MDIO_ERROR with the original detail.
This does not prove a physical wire defect; public timeout and raw report stay
unchanged. An otherwise clean, excluded no-attempt
rejection plus clock regression remains uncounted; clockInvalid still latches.
At most one managed count is produced per callback.

All counters saturate at UINT32_MAX. A managed failure increments totalFailures
and consecutiveFailures, records normalized status and completion-time
`lastErrorUs` if valid. Health timestamps use the post-callback raw clock sample
in microseconds, subject to clockLagUs uncertainty; zero is valid. They are
not exact wire-completion instants or freshness guarantees. Replacing lastError
also replaces its timestamp validity:
if that failure has no valid completion time, set lastErrorUs=0 and
lastErrorTimeValid=false. Never attach an older error's valid timestamp to a
new error. An earlier lastOk/time-valid remains historical evidence; the clock
fault latch prevents using it to compute age in the broken timeline.
A managed success increments totalSuccess, resets the streak to zero and updates
lastOk/time-valid. It **preserves lastError and its
timestamp**. Raw/probe/session-stop traffic cannot change the streak; a later managed initialization can.
Configuration trust is never repaired by a transport success.

`initialized=false` forces health state UNINIT, including before first initialization or after reset invalidation; OP-13 governs reapply and restart. Managed attempts during
initialization still count and record evidence; they do not prematurely expose
READY. `initialized=true` means accepted identity plus required initialization
completed in the current binding/reset context, not link-up or complete desired
configuration trust. Once initialized, streak zero => READY; 1 through
threshold-1 => DEGRADED; streak >= threshold => OFFLINE. Threshold defaults to
3 as configurable application policy, not silicon timing; zero is invalid.
OFFLINE never suppresses a later attempt. Any later successful **managed**
transaction resets the streak and restores READY if still initialized, without
recovery writes, link claims or clearing lastError. Reinitialization may restore
initialized only through its complete procedure, never `resetStatistics`.

`resetStatistics()` clears **only** health totalSuccess and totalFailures. It
preserves consecutiveFailures, state, initialized, lastError, both timestamps/
validity flags, threshold, transport totals, chip counters, desired settings,
trust, collector history and pending results. No I/O, clock read or recovery.
Successful replacement binding/local end starts a new binding's local evidence
after required result delivery and barrier disposition; it is not this statistics reset.
Prompt 21 implements these health methods over prompt 11 transport accounting;
see [passive health](health.md) for the executing classification and lifecycle table.

Cached accessors return value copies without callbacks (including clock), I/O,
polling, acknowledgement or retry. Age is computed externally from a timestamp
and validity in the same time domain; no cached accessor silently expires data
or accepts a zero timestamp as "not observed". `stale` records known invalidation
or failed refresh, not application age policy. A prior valid snapshot can remain
`valid=true, stale=true` as history. A never-observed snapshot has valid=false.
On a successful refresh, stale=false and lastRefreshStatus=OK even for link-down.
LinkSnapshot retains the acquisition interval: observedSinceUs is the start
lower bound and observedAtUs the completion upper bound. A conservative maximum
age subtracts observedSinceUs from the owner-supplied current upper time, with
explicit order/overflow checks. LinkChange and saturating observations/changes
retain first, changed, repeated, stale and regained evidence without consumption.
On failed live refresh (including frame/deadline/bound admission), preserve prior
values/observedSinceUs/observedAtUs/timeValid as historical
evidence, set stale=true and lastRefreshStatus to the returned error. An ordinary
caller output remains unchanged. A clock fault additionally prevents comparing
that history to the now-invalid time domain; it never manufactures a fresh stamp.

Active-profile preflight requires BMCR[11]=0 only when software power intent is
unowned. With explicit `PowerSettings`, every BMCR writer replaces bit11 from
that owned intent, never the software/pin OR. Owned IEEE down is retained across
profile application; negotiation wait/page exchange rejects while down and AN
restart is suppressed. Power reset/replay uses the same intent. See [power
operations](power.md) for separate readback, blocked wake and PLL ordering.

Settings trust VERIFIED means every stable owned bit of this foundational tuple
was compared successfully against validated desired intent in the current
context. UNKNOWN means not established; DIRTY means a known possible mutation,
external change or mismatch invalidated that comparison. Observed fields are
not desired fields merely because equal. Partial words belong in retained
procedure evidence; do not assemble a new observed tuple from mixed epochs.
Desired intent survives invalidation and temporary diagnostics; raw writes never
silently update it. Later feature stages extend typed snapshots/owned fields
alongside behavior, rather than claiming this link tuple covers all chip state.

`observed` is a decoded canonical **active-policy** view. Inactive forced members
in AN mode and the canonical inactive advertisement in forced mode make no
hardware-observation claim. Raw inactive bits remain separate procedure evidence.
Trust compares only the mode-specific active owned masks; a retained ANAR in
forced mode is neither a mismatch nor a reason to clear it. A complete in-budget
readback mismatch publishes the fresh observed tuple/time, sets trust DIRTY and
lastVerificationStatus=CONFIG_MISMATCH. Partial/failed verification preserves the
old tuple/time as stale, records its error and never promotes trust.
The additional rawVod/vodValid fields retain an independently returned mandatory
word even when that batch later fails release or timing. Like other raw-field
validity flags, they describe historical capture, not whole-profile freshness.
VERIFY without desired settings checks identity and VOD only and never fabricates
verified profile trust. An accepted changed identity returns
INVALID_CONFIG/DEVICE_INVALIDATED and requires explicit reinitialization;
actual declared identity mismatch remains DEVICE_MISMATCH. An observed active
BMCR reset invalidates initialization before any release/deadline error is
returned; otherwise VERIFY returns BUSY without waiting. AN-restart is ignored
by stable comparison. Successful VERIFY may set persistentRestored as stable
proof while cleanupOwed remains unchanged under OP-9.

```cpp contract
namespace DP8382x {
struct OperationToken { uint64_t value{0}; };
constexpr OperationToken INVALID_OPERATION_TOKEN{};
enum class OperationPhase : uint8_t {
    NONE, PREFLIGHT, IDENTIFY, QUIESCE, COMMAND, WAIT, RESTORE, VERIFY, COMPLETE
};
/// @brief Captured strap words, not current pin levels or inferred reset defaults.
struct StrapEvidence {
    uint16_t sor1{0};
    uint16_t sor2{0};
    bool sor1Valid{false};
    bool sor2Valid{false};
    uint64_t observedAtUs{0};                ///< Conservative complete-acquisition stamp.
    bool timeValid{false};
};

/// @brief Exact first failed stable-owned-bit comparison; value validity is explicit.
struct ConfigurationMismatch {
    bool valid{false};
    bool mmd{false};                        ///< false: C22; true: MMD DEVAD/offset.
    uint8_t devad{0};                       ///< 0..31 only when mmd.
    uint16_t registerAddress{0};            ///< C22 0..31, MMD 0..65535.
    uint16_t mask{0};                       ///< Stable owned bits only.
    uint16_t expected{0};
    uint16_t observed{0};
};

}
```

## Exact local and direct declarations

[DP8382x.h](../include/DP8382x/DP8382x.h) owns shipped signatures and preconditions;
[direct access](direct-access.md) owns their wire traces and failure examples.
All driver and application/native owner objects are noncopyable and nonmovable,
even unbound. Copies of Config borrow the same resources. Destructors do no I/O;
resources outlive all callbacks, results and externally assumed obligations.

bind validates a temporary Config before replacing state and calls neither clock
nor transport. Invalid enum/address/transport/threshold/interface declarations
leave the old binding intact. Null context is legal. bind establishes no identity,
readiness or hardware configuration; end is local and idempotent while unbound.
OP-12 governs both once operations exist. The lifetime token counter survives both.

Direct methods terminate synchronously and never return IN_PROGRESS. Width and
policy checks occur before callbacks. Plain raw reads may run before initialization;
the existing LEDCR maintenance subset additionally permits pre-initialization
writes under exact-part/address/electrical owner admission. Stage 13 adds explicit
destructive acquisition and narrow mixed-register maintenance consumers through
the single collector. It does not expose unrestricted reset, session or raw
destructive access. Other typed features acquire their admission with their code.
Ordinary outputs remain unchanged on failure. OP-14 defines operation interleaving.

Selectors REGCR/ADDAR, holes, reserved-only words, procedure-only words and CDCR
remain refused through raw methods and MMD aliases. CDCR has a mandatory recovery
obligation when bit 14 reads high. A field's R/W type alone grants no write API.
Reserved handling is per target: LEDCR forces zero; elsewhere preserve read-back
reserved bits outside the caller mask unless a sourced whole-word recipe says
otherwise. Preserve command-neutral values and collect consumed status first.
RXFCFG1[11] is W0C: unrelated writes use one, explicit acknowledgment uses zero;
reserved and undefined-feature fields are not caller controls. Its polarity and
the five-frame MMD inference still require physical cases Q3/Q4.

Raw/external/ambiguous semantic writes invalidate affected observed trust without
changing desired intent. The immediate transport receipt is replaced by the next
batch; collector and operation evidence have independent retention. No hidden
retry, selector cleanup, scan or reset repairs a failed call.

The stage 13 maintenance routes are acquireEvidence(sourceMask, output, budget),
evidenceSnapshot(), modifyInterruptEnables(register, mask, value, budget) and
modifyWakeControl(mask, value, acknowledge, budget), with exact declarations owned
by the shipped header. Acquisition accepts only the eleven explicit source bits,
orders PHYSTS before requested mirror-clearing sources and retains partial results.
MISR changes read and modify the low enable byte atomically while retaining the
consumed high event byte. Wake changes first harvest RXFS and MISR2; unrelated
updates preserve the W0C indication and bit6, and explicit acknowledge uses zero.
Both maintenance routes require the owner's exact-part/address and safe-use
admission even before typed initialization; raw access remains restricted.

Stage20 ships `collectEvents(CounterSnapshot&, const IoBudget&)`, passive
`counterSnapshot()` and bus-silent `clearEvidence()`. Public headers own the
value declarations; the [event/counter guide](event-counters.md) owns their
partial-history and reset/discard contract. Collection reuses `acquireEvidence`
for one PHYSTS/FLDS/MISR1/MISR2/FCSCR/RECR batch. Ordinary output is unchanged
on failure, separately retained valid prefixes survive, and any subsequent
collector read invalidates the earlier complete aggregate. No RXFS read is
added to decode MISR2's ambiguous shared event.

Host history spans binding or explicit discard, with sticky discontinuity
across reset/external/configuration changes; latest samples can become fresh
without assigning retained event unions to a reset epoch. Event masks coalesce
occurrences. Sample count and host arithmetic overflow are explicit. Only FCSCR
is accumulated, with hardware saturation/lost history flagged as lower bounds;
RECR remains raw and its arithmetic field stays zero. Discard does not access
hardware, clear transport/managed-health counters, alter configuration trust,
or consume a retained operation result. Running work/callbacks refuse discard.

The following class-member signatures are shipped for settings/lifecycle and,
from prompt 19, link observation. Passive health is implemented in prompt 21; the
enclosing class is omitted from this declaration excerpt:

```cpp interface
Status readLinkStatus(LinkSnapshot& snapshot, const IoBudget& budget) noexcept;
DriverState driverState() const noexcept;
HealthSnapshot healthSnapshot() const noexcept;
void resetStatistics() noexcept;
LinkSnapshot linkSnapshot() const noexcept;
SettingsSnapshot settingsSnapshot() const noexcept;
Status invalidateDeviceState(InvalidationReason reason) noexcept;
Status startVerify(const OperationBudget& budget, OperationToken& token) noexcept;
```

### Live link result and failure examples

`readLinkStatus` requires initialized identity and owner admission, but not
verified desired configuration merely to observe. Local lifecycle/owner rejection
is not an acquisition and does not change a snapshot; ordinary acquisition-budget
failure does mark prior history stale. An active typed next-page exchange rejects
link acquisition to preserve its sole ANER consumer. It always collects two BMSR
samples in order through the collector, including when the first link bit is
low. Do not copy TI `GenericPhy_isLinkUp`'s first-low success with untouched
output. A successful query assigns all fields from a staged snapshot; a valid
down result has valid=true, stale=false, linkUp=false, UNKNOWN speed/duplex,
resolvedModeValid=false, pauseValid=false, and meaningful time/refresh status.

A current-link pass uses exactly **10 physical C22 reads on success**. First
batch: BMSR(first), BMSR(current), PHYSTS(first), BMCR, ANAR, ALNPAR, ANER,
PHYCR. Second batch: BMSR(final), PHYSTS(final). Reserve all ten frames and the
sum of both callback bounds before the first callback, while retaining one
deadline; stop at any failure. Every destructive sample belongs to the collector.
The final reads check current link/AN against the prior current BMSR/PHYSTS,
and qualified resolved mode against prior PHYSTS when used. This is a bounded
consistency check, not an atomic hardware snapshot. ANAR/ALNPAR/PHYCR pause
evidence is usable only with a coherent full-duplex AN result and the ledger's
exact interpretation. A detected change returns BUSY/OBSERVATION_CHANGED,
keeps ordinary output unchanged and marks cached history stale; there is **no
automatic retry**. The owner may request a new bounded observation. The [implemented guide](link-observation.md)
defines the detailed masks and source gates; new evidence changing this order
or cap requires a coordinated contract/caller update, not a silent extra loop.

Valid UP with pending negotiation may have unresolved mode/pause. With AN disabled, use observed BMCR bits 13/8 for forced speed/duplex; retain
PHYSTS low bits raw. Forced 100/half with extended full duplex requested or
unverified remains unresolved because the result depends on its partner.
Complete AN resolution requires current link, AN complete and sufficient coherent
source evidence. Pause RX/TX are independently resolved only for full-duplex AN;
half duplex, forced mode and pending AN leave pauseValid=false. Pause also
requires IEEE selectors, an AN-capable partner, highest-common full-duplex
technology matching observed speed and agreement with PHYCR. A mismatch retains
raw advertisements and independently valid mode, while withdrawing pause. The SDK cannot
project UP without the required resolved tuple; that restriction does not force
the portable query to invent one or label valid link-down a failure.

| Input/failure sequence | Ordinary output and status | Retained evidence |
| --- | --- | --- |
| Prior caller snapshot says UP; BMSR first `0x7849`, second `0x7849`, remaining required reads establish down | OK; replace caller snapshot with explicit valid DOWN and invalid resolution/pause. | Both low samples retained, current link-down timestamp; no MDIO health failure. |
| First BMSR `0x7859` (latched low and fault), second read MDIO_TIMEOUT detail -37 | Caller snapshot unchanged; MDIO_TIMEOUT/-37. | Successful first full word retained, cached aggregate stale; failed destructive second read marks possible lost history. No synthetic second word. |
| First low, second high, coherent remaining observation | Valid current UP if sufficient evidence; the first low remains historical. | Both samples retained; never claim the pair is an atomic history sample. |
| ID1 `0x2000`, ID2 transport failure | No newly accepted identity; ordinary identity result unchanged. | First raw word may remain valid; decoded/accepted false. |
| IDs `0x2000/0xA132` | DEVICE_MISMATCH/REVISION_NOT_ACCEPTED. | Both words/model ENHANCED/revision 2 retained; no suffix inference or replacement revision. |
| PHYRCR callback fails while checking reset | MDIO error; no reset-complete claim or default zero. | Address is 0x1F, command mask 0x8000; mask 0x8000 as C22 address is rejected before callback. |

## Complete typed feature map

The single [feature coverage matrix](feature-coverage.md) owns feature/procedure
IDs, implementation owners, CLI routes, tests and physical cases. Do not create
a second field catalogue in C++ or duplicate the matrix here. Stage 13's policy
table classifies every ledger address; masks and legal encodings remain sourced
and mechanically checked, with destructive status owned by one collector.

## Cooperative values

[DP8382x.h](../include/DP8382x/DP8382x.h) owns the shipped operation declarations:
OperationToken, OperationKind/State/Phase, ResetKind, OperationBudget,
InitializeOptions, StrapEvidence, ConfigurationMismatch, BatchSummary,
OperationResult and OperationSnapshot. Stage 15 implements startInitialize,
pollOperation, takeResult, cancel, abandonCleanup, invalidateDeviceState,
operationSnapshot and isInitialized. See the [initialization guide](initialization.md)
and compiling [owner example](../examples/portable/Initialize.h).

INITIALIZE with NONE/PCS/HARD/RESTART and the retained
busQuietUntilUs/busQuietValid floor are implemented through stage 16. Stage 17
implements CONFIGURE and SettingsSnapshot for negotiated ANAR/BMCR plus mandatory
VOD; stage 18 adds forced modes and all owned CR1/PHYCR/CR3/CR2 controls.
Stage 22 implements VERIFY in the same slot and OperationBudget. EXTERNAL_POWER_ON
remains a local invalidation attestation, not a hard-reset operation. See
[reconciliation](reconciliation.md) for the exact finite read plan, retained
mismatch semantics and explicit initialization replay. No separate reset/recover
alias or automatic escalation exists.

StrapEvidence retains raw partial/historical words, with independent validity
and staleness. A trustworthy completed read survives a later release/deadline
failure; it does not establish a complete current strap capture. BatchSummary
retains normalized release evidence independently of the operation status.

## One cooperative operation and retained result

- **OP-1 One slot.** One instance holds one operation, with no task, scheduler or registry. Starts copy their inputs,
  return IN_PROGRESS and assign a token. They call neither clock nor MDIO.
- **OP-2 Start rejection order.** Syntax and budget shape (`pollIntervalUs` or `maxStalledPolls` of zero is
  INVALID_PARAM/BUDGET_RANGE); NOT_BOUND; slot busy (BUSY/OPERATION_ACTIVE, then BUSY/RESULT_PENDING); known clock fault
  (INVALID_CONFIG/CLOCK_CONTRACT); barrier (OP-9, BUSY/CLEANUP_REQUIRED); NOT_INITIALIZED; capability and source admission; zero budget
  (OPERATION_TIMEOUT/FRAME_BUDGET or DEADLINE). `startConfigure`, `startVerify`, feature starts, session starts and session acquire require
  initialized. `startInitialize` and the owning stop do not, and use the bootstrap identity path. OFFLINE never denies a start. `startVerify` issues no semantic or command write.
  A rejected start changes nothing.
- **OP-3 Tokens.** A 64-bit counter for the object's lifetime, from 1, never reset by end or bind. There is no exhaustion
  branch, code or test.
- **OP-4 States.**

  | State and event | Next |
  | --- | --- |
  | IDLE, accepted start | RUNNING |
  | RUNNING, poll with work remaining | RUNNING, IN_PROGRESS |
  | RUNNING, full success | RESULT_PENDING, status OK |
  | RUNNING, error, timeout or clock fault | RESULT_PENDING, status frozen; set barrier per OP-9 |
  | RUNNING, cancel | RESULT_PENDING, status CANCELLED unless an earlier error froze; set barrier per OP-9 |
  | RUNNING, `invalidateDeviceState` | RESULT_PENDING, INVALID_CONFIG/DEVICE_INVALIDATED unless an earlier error froze; set barrier per OP-9, then apply OP-15. `result.cleanupOwed` is the value after both |
  | RESULT_PENDING, poll or repeated cancel | Unchanged; poll repeats the status silently; cancel returns OK |
  | RESULT_PENDING, exact take | IDLE; result copied once; barrier unchanged |

  The status never changes once frozen. `takeResult` before terminal returns RESULT_NOT_AVAILABLE. Take always succeeds
  once pending, so evidence can always be delivered.
- **OP-5 Poll.** At most one batch per call, in this order:
  1. Sample time. A regression latches CLOCK_CONTRACT and freezes.
  2. If raw time >= deadline, freeze OPERATION_TIMEOUT/DEADLINE before any I/O.
  3. If the next action is not due, or a hard-reset floor is pending, return IN_PROGRESS with no I/O. The floor applies to every
     operation kind.
  4. If the next batch exceeds remaining frames, freeze FRAME_BUDGET. If its bound cannot fit before the deadline, freeze
     BOUND_ADMISSION.
  5. If the next batch exceeds `maxFrames`, return IN_PROGRESS with no I/O. So `maxFrames=0` is bookkeeping only, and a short
     slot is never fatal.
  6. Otherwise run it.

  A poll that issues no batch and sees the same raw clock value as the last sample any poll took, including a post-callback sample,
  increments `stalledPolls`. Any batch or clock advance resets it, and the first poll of an operation never stalls. Reaching
  `maxStalledPolls` freezes OPERATION_TIMEOUT/POLL_BUDGET. Frames bound batches and stalls bound waits, so a frozen clock always
  terminates, within `(frames + 1) * maxStalledPolls + 1` polls. The
  default of 100000 is deliberately large, because this is a fault path: a tight loop on a 100 Hz tick clock must not trip it, while a
  frozen clock still ends in about 100 s at 1 ms polls. Lower it only if the clock's tick is shorter than poll spacing times the value.
- **OP-6 Reset admission.** This is the only look-ahead. Before issuing a reset command, require frames for the command batch and
  the first completion read, and `raw + 4 * clockLagUs + bound(command) + wait + bound(read) < deadline` with checked addition. `wait` is
  `pollIntervalUs`, or `max(2000, pollIntervalUs)` us for HARD. Otherwise freeze BOUND_ADMISSION with no mutation. Every other step
  checks only its next batch. The four lags are: upper bound at entry, anchor at completion, overshoot of the first raw sample >= Due on a quantized clock, and upper bound at the read. This assumes the raw step does not exceed
  `clockLagUs`. It rejects budgets too small under prompt polling; a late owner poll can still time out after the command, and OP-9 covers that.
- **OP-7 Time.** `clockLagUs` bounds quantization and source-update latency at the instant of reading the clock; scheduling after that read is excluded. Only a raw post-sample >= deadline denies success; there is no publication reserve.
- **OP-8 Primary and cleanup stay separate.** Each operation has one status and one result. Restoration is a later
  operation with its own token, result and budget, so its success can never rewrite the earlier failure. There is no
  automatic retry; the application may retry.
- **OP-9 Barrier.** Two pieces of knowledge, plus one private field recording what set them:
  - `temporaryMayBeActive` becomes true when the first write of a session or temporary profile is **attempted**, including companion
    writes. It becomes false only on a successful owning stop, a successful HARD, EXTERNAL_RESET/EXTERNAL_POWER_ON invalidation, or a successful `bind` (OP-12). A
    reset command alone never sets it; a pending reset is `commandAccepted && !protocolComplete`.
  - `cleanupOwed` is set when an operation ends without `persistentRestored` after a semantic effect (`mutationCompleted` or
    `mutationPossible`), and when `temporaryMayBeActive` becomes true. Selector writes do not count.

  | While `cleanupOwed` or `temporaryMayBeActive` | Admitted |
  | --- | --- |
  | Always, except where OP-14 blocks them | Admitted raw reads and the collector |
  | Temporary active | The owning session's own acquire operations and its stop, even if its start failed; `startInitialize({HARD})` |
  | No temporary active | `startInitialize` with any reset; `startVerify`; `startConfigure` only if a CONFIGURE operation set the barrier |
  | Everything else | Other starts and raw and typed direct writes return BUSY/CLEANUP_REQUIRED. `end` and `bind` return BUSY/CLEANUP_REQUIRED only while `cleanupOwed`; after `abandonCleanup` they are admitted even with `temporaryMayBeActive` set |

  `cleanupOwed` clears only on: a successful owning stop; a successful `startInitialize` with `persistentRestored`; a successful
  `startConfigure` admitted by the row above; `invalidateDeviceState` with EXTERNAL_RESET or EXTERNAL_POWER_ON; or `abandonCleanup`.
  **`startVerify` never clears it**, because stable bits cannot prove opaque recipe state, and it is rejected while a
  temporary is active, because overridden fields would give false mismatches. A failed restoration leaves the barrier set. PCS
  reset and RESTART cannot be assumed to stop a stimulus: they keep vendor registers (table 8-19, table 8-48).

  **Sessions.** A stimulus that stays active across owner decisions is two operations with separate budgets; its duration is
  application policy, and the core only blocks other work. A self-contained test (cable capture, a counted BIST run) may be one operation
  that must stop and restore before success. A start or stop with no wait may be a direct bounded call: it follows the same set and clear
  rules at return, requires slot IDLE, and on failure leaves the barrier set. Loopback, BIST, compliance, forced LPI and collision test
  are sessions.
- **OP-10 Abandon.** `abandonCleanup()` is bus and clock silent. While RUNNING it returns BUSY/OPERATION_ACTIVE. With no
  obligation it returns OK and changes nothing. Otherwise it clears `cleanupOwed`, sets initialized false and trust DIRTY,
  and keeps `temporaryMayBeActive` and all evidence. It never claims restoration, clears controller quarantine or authorizes freeing
  borrowed resources. Before calling it the owner must close admission, drain callbacks with no lock held across SDK calls, and
  establish the external disposition: reset, power removal, isolation or transfer to another owner. A later `startInitialize` on
  this binding still needs HARD while `temporaryMayBeActive` is set.
- **OP-11 Broken clock.** Timed work is refused once latched, so restoration cannot run. The finite route is: `takeResult` if pending, `abandonCleanup` if owed, `end`, repair the clock, wait out any OP-12 floor using a time source the owner trusts, `bind`.
- **OP-12 End and bind.** Both reject BUSY/OPERATION_ACTIVE, BUSY/RESULT_PENDING or BUSY/CLEANUP_REQUIRED, with no callbacks. A
  successful `bind` resets local knowledge, including `temporaryMayBeActive`; the owner carries any known hardware state across it. `end`
  and a replacing `bind` discard a pending hard-reset floor, so the owner must wait at least 2000 us after the last possible PHYRCR[15] write before the new
  binding's first frame. Neither touches hardware or controller quarantine.
- **OP-13 Initialized.** A successful `startInitialize` sets it. It clears when a PCS or HARD reset is attempted or possibly applied,
  or independently observed active, even if that read's release or timing later fails,
  when `invalidateDeviceState` runs, or on abandon. `startInitialize({NONE})` on an initialized object leaves it set unless identity or
  verification fails, or a mandatory-initialization write is possibly applied and unverified, so link polling continues through a
  reapply. RESTART never clears it, because table 8-48 keeps registers; a HARD inside a session recipe does. An owning stop that
  completes the full identity, mandatory and replay procedure sets it.
- **OP-14 Direct calls during operations.** "Raw writes" here and in OP-9 means raw and typed direct writes, except the owning session's direct stop (OP-9).

  | Situation | Admitted raw reads and collector | Raw writes |
  | --- | --- | --- |
  | IDLE, no barrier | Yes | Per access policy |
  | IDLE, barrier or temporary | Yes | BUSY/CLEANUP_REQUIRED |
  | RUNNING, between polls | Yes: no selector state survives a batch | BUSY/OPERATION_ACTIVE |
  | RUNNING, reset wait | BUSY/OPERATION_ACTIVE | BUSY/OPERATION_ACTIVE |
  | RESULT_PENDING | Yes, so link polling continues | BUSY/RESULT_PENDING |
  | Hard-reset floor pending, in any state | BUSY/OPERATION_ACTIVE | BUSY/OPERATION_ACTIVE |

  **Hard-reset floor.** It starts whenever any operation attempts a PHYRCR[15] write, including a failed or late write, anchored at
  the post-callback upper bound plus `max(2000, pollIntervalUs)`. It survives cancel, take, abandon and invalidation. Publish it as
  `busQuietUntilUs`. A shared-bus owner holds other PHYs from before the poll that can issue PHYRCR[15] until that time. If the post-callback sample regressed, no anchor exists: `busQuietValid` is false, timed I/O is already refused, and the owner applies OP-12's 2000 us.

  A malformed submitted HARD receipt cannot prove no write: it also retains possible
  effects and the floor, even if attempt flags are missing. Valid zero-attempt
  denials remain effect-free. Overflow forming this mandatory floor leaves its
  anchor invalid and latches CLOCK_CONTRACT because the nonwrapping timeline
  cannot prove the required wait; preserve an existing primary error, otherwise
  terminate BOUND_ADMISSION. Ordinary nonfloor due overflow does not add this latch.
  Cached floors expire only when a timed call observes their endpoint. Safe raw
  reads can sample the clock to reject/expire a floor before any MDIO; snapshots,
  notifications, cancellation and result delivery remain clock-silent.

  **Link evidence during writes.** If a RUNNING operation has attempted a semantic write, or `temporaryMayBeActive` is set,
  `readLinkStatus` publishes the link bit only, with `resolvedModeValid=false` and `pauseValid=false`, and copies
  `temporaryMayBeActive` into `LinkSnapshot`. Otherwise a reader could combine new ANAR with old ALNPAR, or report an internal loopback
  path as a normal link.
- **OP-15 Invalidation.** Bus and clock silent in every state. It marks trust DIRTY, observations stale, capabilities invalid and
  initialized false, and keeps desired intent, samples and evidence. IDLE stays IDLE. RUNNING freezes per OP-4. RESULT_PENDING keeps its
  historical result. Only EXTERNAL_RESET and EXTERNAL_POWER_ON clear the barrier and `temporaryMayBeActive`; this is the owner's
  attestation that hardware was reset.
- **OP-16 Health.** `startInitialize`, `startConfigure`, `startVerify`, typed persistent feature operations and live link reads are managed traffic under the managed-health predicate above. A
  successful `startInitialize` may restore READY. Session stimulus, session stop and raw access count only in transport totals.
- **OP-17 Result flags.** `result.cleanupOwed` and `temporaryMayBeActive` are copies at termination. The current truth is in
  `operationSnapshot`.

### Result evidence and wait anchors

Admission does not mean command acceptance, initialization or restored hardware.
Tokens are interpreted only with their original nonmovable instance: zero/future
or another active token is TOKEN_MISMATCH; with an empty slot any previously issued nonzero token returns RESULT_NOT_AVAILABLE.
A newer occupied slot rejects every other token as TOKEN_MISMATCH. No history
registry is needed, and equal token numbers on different objects are unrelated.
Rejections preserve caller outputs and state. No-token output buffer is retained.

mutationAttempted/completed/possible are historical ORs for semantic target writes;
REGCR/ADDAR selector writes do not set them. commandAccepted requires acknowledgment;
protocolComplete requires the documented observation before deadline. When no
command is required both are false, even on success. persistentRestored means
all currently owned persistent state was proved. OP-9 defines temporary/barrier
state, independently from these historical effect flags and physical link.

Before overwriting the transport receipt, keep the first failing BatchSummary,
otherwise the last; OR late/clock/effect history into the operation result.
The raw batch stays in transportSnapshot, and the collector separately keeps
consumed partial samples. Keep the first mismatch while finishing only the
finite verification pass; a later transport error wins within that pass. Freeze
CONFIG_MISMATCH only after all planned reads succeed. A later restoration result
cannot replace the frozen outcome.

nextPollValid is false in IDLE, new PREFLIGHT, RESULT_PENDING or invalid time;
false in PREFLIGHT permits an immediate owner poll. A true due time is the
earliest legal action, not a scheduling promise. Anchor Due to validated completed
frame upper time, or conservatively the refreshed post-callback upper bound, plus
the minimum wait. Only a later raw sample >= Due admits the next action.
Overflow forming Due is BOUND_ADMISSION with effects retained, unlike an upper
clock-bound overflow which latches CLOCK_CONTRACT. Sleeping only schedules polls.
If the owner stops polling no core work runs or wall-clock completion is promised.

### Readiness and source-specific procedure anchors

Board ownership covers RESET_N, supplies and clocks. SNLS647I 7.6 and 8.3.11
require at least 25 us assertion without debounce capacitors, at least 2 ms
shared-SMI silence after actual reset release, and at least 50 ms after both
supplies reach their final level. The latter readiness figures are MAX table
values used as waiting windows by the prose. A clock that arrives late requires
reset after POR and at least 100 us clock stability. No other PHY may generate
MDC during the shared silence. bind/start cannot establish these board facts.

PHYRCR hard reset uses OP-14's floor after every attempted command. PCS and
RESTART poll their actual self-clearing bit; no undocumented completion maximum
is invented. FLP 1500 ms and EEE timing figures are nominal, not deadlines.

Initialization reads both IDs, captures applicable SOR1/SOR2, checks live command
state and waits out an already active reset, then issues the selected whole-word
command once: BMCR=0x8000 for PCS, PHYRCR=0x8000 for HARD or 0x4000 for RESTART.
Never compose reset commands from sampled command bits or poll address 0x8000.
After required identity/readiness, every startInitialize writes and verifies
VOD_CFG3=0x4A40 for every ResetKind. Unknown identity forbids configuration writes.
The best reading permits hard reset to resample eligible straps, but software-reset
wording remains arguable: retain `hard_reset_straps` and Q5, NOT RUN. ENHANCED
Strap1/6 are not re-latched by reset; no POR-only BASIC restriction is inferred.

### Desired state, invalidation and acyclic restoration

Accepted startConfigure copies validated desired intent and marks trust DIRTY;
rejection changes nothing. Cancellation/failure preserves accepted intent. Intent
alone, before effects, creates no cleanup obligation. Later restoration receives
its own budget/result. Temporary profiles never replace persistent desired state.
Save every touched readable documented owned field before mutation, including
all split-field pieces and legal transition order. Recipe-only/unreadable fields
require an exact sourced stop/reset/reapply route; absent routes reject preflight.

Shared procedures use this finite order, never recursive public starts:

1. Validate the full request against implemented owned fields/source gates and
   establish board/controller readiness.
2. Stop temporary stimulus before restoration; HARD itself is the stop for a
   hard-reset exit.
3. Execute the selected reset primitive once and observe actual completion.
4. Re-identify/address/mode as required, retain straps, apply mandatory 0x4A40.
5. Apply qualified persistent recipes; interface/clock/power dependencies;
   coupled crossover/compatibility; advertisement/exchange state and activation;
   event enables. Verify stable owned state and required protocol completion.
6. Publish initialized/restored evidence. Observe live link separately.

startInitialize can succeed unplugged and replays valid existing desired settings
as their owning features ship; it invents no default profile. NONE replays without
reset; PCS/HARD/RESTART select their primitives plus mandatory initialization and
replay. startVerify requires initialized, checks identity/stable owned bits without
semantic writes, and never clears an opaque-state cleanup barrier. Temporary
COMPT/loopback/BIST/TDR triggers and consumed events are not persistent settings.
Reset-bearing EEE or compliance procedures reuse the same finite order once
their exact source gates are admitted, with no recursive recipe invocation.

## Operation acceptance scenarios and examples

| Scenario | Required |
| --- | --- |
| Start, polls, take, cable unplugged | Status OK; separate `readLinkStatus` gives valid DOWN |
| Short slot, `maxFrames=1`, next batch needs 4 | IN_PROGRESS, no I/O while the batch still fits; then OPERATION_TIMEOUT/BOUND_ADMISSION, or DEADLINE if the first poll after that is already at or past the deadline |
| Polling every 100 us through a 2 ms wait on a 1 us clock | No POLL_BUDGET; first read not before due |
| Frozen clock during a wait | No I/O; POLL_BUDGET on stall count `maxStalledPolls`, where the first poll never stalls; barrier set if a reset was issued |
| Tight loop on a 100 Hz tick clock through a 5 ms wait | No POLL_BUDGET; first read only once a raw sample reaches due |
| 100 Hz clock, lag 10000 us, HARD, deadline now + 5 ms plus bounds | BOUND_ADMISSION before any write, per OP-6's literal formula |
| Budget too small for reset command, wait and first read | BOUND_ADMISSION, zero writes, no barrier |
| Budget just enough for the reset, restore runs out | Timeout at the first non-fitting batch; barrier set; restoration later succeeds with a new budget and clears it |
| Hard reset with 1000 us interval | No frame before command-completion upper bound + 2000 us |
| Hard-reset write fails or returns late | Floor still applies; `busQuietValid` true; status frozen |
| Cancel after a hard-reset write, take, immediate new `startInitialize({HARD})` | Polls return IN_PROGRESS with no frame until the floor ends. `startVerify` in the same state returns NOT_INITIALIZED with no token |
| Abandon, end and bind within the floor | Owner waits out 2000 us; a contract test documents this, and the core cannot enforce it |
| Cancel before any effect | Pending CANCELLED, no barrier, `end` after take succeeds |
| Cancel after accepted reset | Pending CANCELLED, barrier set; take OK; other starts BUSY/CLEANUP_REQUIRED; `startInitialize` clears it |
| Restoration fails | Barrier remains; application may retry with a new token; the first result is unchanged |
| Loopback active, `startInitialize({PCS})` | BUSY/CLEANUP_REQUIRED; `stopLoopback` or `{HARD}` admitted |
| Cancel after a loopback start's first write | `temporaryMayBeActive` true; `{PCS}` rejected; `stopLoopback` admitted though the start failed |
| BIST session active, capture requested | The session's acquire operation is admitted; other starts BUSY/CLEANUP_REQUIRED |
| Power operation fails after a write, then `startConfigure` | BUSY/CLEANUP_REQUIRED; `startInitialize` restores and clears |
| Configure fails after a write, then `startConfigure` succeeds | Barrier clears |
| `startVerify` succeeds while a barrier is set | Barrier stays set; evidence only |
| `readLinkStatus` during reapply after ANAR write | Link bit only, `resolvedModeValid=false`, `pauseValid=false` |
| External reset notified while barrier set | Barrier and temporary cleared; initialized false; pending result unchanged |
| Clock regression mid-operation, with and without a loopback session active | Frozen CLOCK_CONTRACT; route of OP-11 completes with no timed call |
| Link poll with result pending | `readLinkStatus` succeeds; start and raw write BUSY/RESULT_PENDING |
| First identity read MDIO_TIMEOUT/-37 | Original error and partial identity retained; no semantic mutation, no cleanup barrier |
| Complete wrong OUI/model/revision | DEVICE_MISMATCH with exact reason; no configuration/reset write; successful transport remains successful |
| Last completion poll still sees reset asserted | Timeout, never OK; keep accepted command and applicable floor/barrier |
| Expired poll when hardware could now be complete | Deadline before read, zero frames, no retrospective completion |
| New start while result pending | BUSY/RESULT_PENDING, prior token/output/evidence preserved |
| Wrong token, stale token, double take | TOKEN_MISMATCH or RESULT_NOT_AVAILABLE; no state or output change |

```cpp example
// Owner state, never shared without application synchronization.
struct OwnerSession {
    explicit OwnerSession(DP8382x::DP8382x& phy) noexcept : core(phy) {}
    OwnerSession(const OwnerSession&) = delete;
    OwnerSession& operator=(const OwnerSession&) = delete;
    OwnerSession(OwnerSession&&) = delete;
    OwnerSession& operator=(OwnerSession&&) = delete;

    DP8382x::DP8382x& core; // Token identity is permanently paired with this instance.
    DP8382x::OperationToken token{};
    DP8382x::OperationResult delivered{};
    DP8382x::OperationSnapshot retained{};
    bool deliveredValid{false};
};
DP8382x::Status admitInitialize(OwnerSession& owner,
                              const DP8382x::OperationBudget& budget) {
    DP8382x::OperationToken candidate{};
    const auto status = owner.core.startInitialize({}, budget, candidate);
    if (status.inProgress()) owner.token = candidate;
    return status; // Rejection preserves the previous owner identity/evidence.
}
DP8382x::Status ownerPoll(OwnerSession& owner,
                        uint32_t maxFrames) {
    const auto progress = owner.core.pollOperation(owner.token, maxFrames);
    owner.retained = owner.core.operationSnapshot(); // No bus or clock access.
    if (owner.retained.state != DP8382x::OperationState::RESULT_PENDING)
        return progress;
    DP8382x::OperationResult result{};
    const auto taken = owner.core.takeResult(owner.token, result);
    if (!taken.ok()) return taken; // Keep token and evidence for explicit disposition.
    owner.delivered = result;
    owner.deliveredValid = true;
    owner.token = DP8382x::INVALID_OPERATION_TOKEN;
    return result.status;
}
```

Construct one OwnerSession for each core instance; the core outlives that session.
Neither the session nor the native owner below can be copied or moved. Only their
snapshot/result values are published by copy. Equal token numbers from two driver
instances cannot redirect these helpers to a different PHY.

The owner schedules finite calls at nextPollAtUs or a later application time.
A short maxFrames slice postpones work without terminating it. After failure,
deliver the result; if cleanupOwed remains, separately admit restoration, or
establish external disposition and abandon before end. Synthetic timing tests
are software evidence and do not qualify ESP-IDF or physical timings.

### Implemented negotiated operation and page contract (stage 17)

The shipped [configuration API and guide](negotiation.md) define `ConfigureOptions`,
`NextPageExchange`, `NextPage`, `SettingsSnapshot` and `NextPageResult`. Fixed page
storage is copied on admission. All legal nonempty technology subsets and pause
combinations are admitted. Stage 18 adds the complete forced/crossover/extension
profile with the same admission and replay contract. Initialization replays admitted
persistent intent only after mandatory VOD setup, using the same procedure.

Raw safe reads remain available between configuration polls. During a typed
exchange, public acquisition of ANER rejects BUSY/OPERATION_ACTIVE to protect the
single page-event consumer; the owning operation uses the existing private
collector. Reset WAIT and persistent hard-reset floors retain their OP-14 guard.
A base event, returned base payload, each consumed next event, returned page and
AN completion have separate evidence. `negotiationComplete` means observed BMSR
completion; restart-clear in `protocolComplete` does not imply it. An earlier
verification mismatch survives a later read/deadline failure in the primary result;
`failingBatch` retains the separate later transport evidence.

### Complete coupled profiles (stage 18)

[Forced settings](forced-settings.md) documents the matrix. Both AN and forced
profiles save/apply/verify CR1 robust routing, PHYCR pair selection, CR3 coupled
mirror and CR2 extension. Forced profiles never read/write/verify ANAR and reject
AN completion/page options. Active observed settings are decoded from successful
readbacks, not copied from desired intent. A complete mismatch publishes actual
fields with DIRTY trust; partial failure keeps the older tuple/time stale. If a
forced request observes AN unexpectedly enabled, no fresh ANAR exists; preserve
any prior tuple/time as stale history, never reuse historical ANAR to complete it.
A non-IEEE ANAR selector likewise cannot establish decoded IEEE technologies.

All controls normally precede BMCR activation. Disabling robust routing for AN
is deferred until after enabling AN, preserving robust routing throughout any
previous forced interval. Forced readback ignores AN-restart because it has no
meaning with AN disabled. No physical partner or pause resolution is inferred.

### Shipped loopback sessions (stage 23)

`startLoopback(const LoopbackOptions&, const OperationBudget&, OperationToken&)`
and `stopLoopback(const OperationBudget&, OperationToken&)` use the same slot;
`loopbackSnapshot()` is passive. Starts require fresh identity, initialized and
VERIFIED desired settings, explicit maintenance and mode-specific fixture
attestations. No callback runs on admission. Loopback start/stop are RAW traffic,
including stop's initialization/profile replay. Explicit initialization HARD
remains managed. The [mode/procedure guide](loopback.md) defines exact scope.

OP-9 temporary state survives take/cancel/abandon. Only owning stop, complete
HARD restoration, external reset/POR attestation or a fresh binding clears it.
A first attempted session write sets it even if the backend reports no target
effect; selector-only frames do not. Stop rechecks identity, clears selectors,
replays the persistent desired profile and verifies saved ordinary fields.
Digital uses HARD for opaque companions; no arbitrary opaque snapshots are
replayed. Failed stop leaves the session available for a new explicit attempt.
Each result retains mode/speed, verified-control/stop flags and any acquired
raw PHYSTS evidence; no packet result is implied.

### Shipped BIST sessions (stage 24)

`startBist`, `startBistAcquire`, `stopBist` and passive `bistSnapshot` extend
the same OP-9 owner. All three operations are RAW traffic. Each uses a new token
and budget; start/acquire require initialized, owning stop does not. Saved packet/
IPG precede loopback mutation; stop/HARD reuse shared restoration. The single
collector retains BISCR/BICSR1 partial words and capture-loss uncertainty.
Stop rechecks identity before PRBS mutation. Dwell collector reads contribute
to retained session activity/loss even if host history is later cleared. A new
HARD replays both saved-field tails regardless of earlier partial stop progress.
See [BIST](bist.md) for the typed mode matrix, exact bounds, per-field validity,
external observation contract and source-gated INCONCLUSIVE outcome.

### Shipped manual cable capture (stage 25)

`startCableTest`, `startCableAcquire`, `stopCableTest` and passive `cableSnapshot`
share the operation slot and OP-9 barrier. Start validates CableSettings, exact
initialized capability and VERIFIED profile before I/O. Live preflight precedes
all mutations. Start executes the finite selected plan and retains temporary
effects; success does not imply restoration. Explicit acquire rereads only the
current accepted trigger, never retriggers or resumes unperformed rows.

Stop has a new token/budget/result and requires HARD through the shared replay
path, followed by saved documented fields. Failure/cancellation freeze the
primary result without cleanup callbacks. Diagnostic traffic is RAW; explicit
initialization remains managed. Nondestructive cable words retain successful
partial reads without inventing history loss. `interpretCable` stages its output
and accepts only complete fresh SNLA428 rows. See [cable contracts](cable.md)
for freshness, raw controls, helper limits and the physical fixture plan.

### Shipped cable autorun (stage 26)

`startConfigureCableAutorun`, `startCableAutorunAcquire` and passive
`cableAutorunSnapshot` extend the existing slot/cable phases. One explicit
maintenance-admitted flag owns CR1 autorun. Admission copies desired intent;
readback establishes only owned policy trust. Disable is the manual handoff and
never proves engine quiescence. Configuration is MANAGED, acquisition RAW.
Failed semantic effects retain OP-9; restoration is a later `startInitialize`,
not a retry exception or hidden cleanup operation.

Shared initialization replays owned policy last after saved diagnostic fields;
failed/cancelled final replay cannot leave initialization complete. `startVerify`
and link `startConfigure` verify the owned bit without writing. Direct/raw reset
invalidation keeps desired policy and stales its independent trust. Unowned
policy never adds replay writes. Power-state transitions use this owner and shared restoration.

Autorun acquisition is one fourteen-frame pass with nondestructive partial words,
no trigger and no environmental polling. `complete` means read-plan completion;
`autorun` plus aggregate `stale` withholds fresh/coherent-measurement claims, even
with equal bookends. No source-backed generation counter exists. The
[cable guide](cable.md) specifies bounds, current settings, handoff and fixtures.

## Prompt 27 interrupt policy, stimulus and service

`startConfigureInterrupts` copies complete explicit policy into the existing
operation slot. `InterruptSettings` owns both enable bytes, PHYSCR polarity/global
and ENHANCED output, plus the RXFS selector; BASIC OE is preserved. Pin permissions
cover both the selected route and pin21. Stage32 adds source-owned alternate
mux configuration/restoration after delivery disable, with masked readback.
`interruptSnapshot` separates desired intent, independent raw validity/time,
policy trust and temporary test effects. Initialization replays owned policy after
other restore tails; verification/configuration only checks it. A changed mode in
the final identity batch invalidates prior proof and rejects before IRQ mutation.

`startInterruptTest` / `stopInterruptTest` are two operations with independent
budgets/results. A possible stimulus leaves OP-9 temporary state. Cancellation
is local; explicit stop verifies clear and preserved stable policy. Full HARD
restoration never replays test1. Stable-policy mismatch cannot remain VERIFIED.

`serviceInterrupts` reads PHYSTS/MISR1/MISR2 once and optionally FCSCR/RECR in a
second callback: at most five samples/frames. Transaction/sample/frame/deadline
limits are explicit. The ordinary output is staged; per-source consumed prefixes
and history loss are retained immediately by the one collector. Nonzero events
or exhausted allowance report morePossiblyPending. Pending half-full requests
survive until a valid collector counter read or explicit host-history discard.
Selector context is captured at MISR2 consumption, with separate sticky ambiguous
history. A read alone cannot establish an old epoch. Cached service/policy getters
do no I/O; ISR only signals the application owner. See [the complete procedure and
board constraints](interrupts.md). Q9 is NOT RUN.

## Stage 28 power ownership

`startConfigurePower`, `readPowerStatus` and passive `powerSnapshot` expose one
complete opt-in policy, source-defined indications and retained partial evidence.
`POWER_CONFIGURE` uses OP-1 through OP-17; starts/cancel/end/cached access remain
bus-silent. Failed mutation requires separate `startInitialize` restoration.
`PowerSettings` owns active-sleep policy, software power-down intent and PLL
control. BMCR isolation remains independent. The [power guide](power.md) defines
finite sequences, live reset/partial invalidation, profile interactions and
non-HARD PLL preparation. DPD is explicitly rejected under `deep_power_exit`;
control readback never proves physical wake or generated-clock readiness.

## Implemented EEE policy (stage29)

`startConfigureEee`, `readEeeStatus` and passive `eeeSnapshot` use the existing
slot, finite transport and single collector. [EEE](eee.md) specifies admission,
source-implied fields and partial publication. EEE ownership is explicit; no
ordinary initialization enables it. Enable/disable and owned EEE restoration
select one effective HARD. This is the documented exception to NONE/PCS/RESTART
initialization intent: `OperationResult.reset` records HARD, and the persistent
shared-bus floor applies. The recipe precedes VOD/profile and other owned replay;
read-only VERIFY cannot prove opaque state or discharge an interrupted recipe.
Host next-page and hardware EEE ownership are exclusive. No TX LPI is requested.


## Stage30 shipped forced TX LPI

[The LPI contract](lpi.md) defines synchronous `startLpi(LpiOptions, LpiResult&,
IoBudget)` and `stopLpi(LpiResult&, IoBudget)`. These are OP-9's no-wait direct
calls: at most9frames/2callbacks, slot IDLE, owned temporary barrier during dwell.
No token cancellation point is invented inside a synchronous callback. Caller
cancellation before entry performs no call; after entry it requests a separate
stop. Admitted diagnostic results publish even on error; local rejection preserves
output. Failed target writes retain UNCERTAIN and explicit frame-loss warning.
The owner example and guide cover post-exit coordination and retained debt.


## Stage31 shipped Wake-on-LAN

[The WoL guide](wake-on-lan.md) defines `WakeSettings`, `WakeSnapshot`,
`startConfigureWake`, cooperative `acknowledgeWake`, live `readWakeStatus` and
passive `wakeSnapshot`. All use existing OP-1..17 lifetime and collector rules.
Configuration and acknowledgement have distinct tokens/results. Settings never
acknowledge pending level implicitly; source-specific neutral one and explicit
zero share the existing RXFCFG1 composer. Quiesced change/readback and shared
reset restoration own the narrow required mux field, with general pin controls
remaining separate. Unsupported custom patterns and LED0/3 access remain gated.


## Stage32 shipped LED/GPIO profiles

[The pin guide](led-gpio.md) defines `PinSettings`, bounded direct
`configurePins(PinSettings, PinResult&, IoBudget)` and passive `pinsSnapshot`.
A complete request and its full frame/deadline allowance are admitted before
publishing desired intent. Finite prerequisite reads can still reject unsupported
live routing. Local rejection preserves caller output; admitted partial results
retain every successful ordinary word, first mismatch and primary/release status.
Accepted/uncertain failure establishes OP-9 debt, restored by a separate
`startInitialize`; no synchronous token/cancel point or automatic rollback exists.
An empty profile releases intent without changing the old physical mux.

The same fixed pass supplies initialization and diagnostic/power recovery, and
read-only VERIFY. Shared WAKE/IRQ pin entries depend on the source owner's route
and verification; passive views withdraw trust if that dependency is lost.
The source owner alone writes its mux after quiescing delivery. Clock, COL and
MAC/board availability remain explicit application attestations.
For LED2/3, live collector-routed RCSR mode must match declared wiring before
mutation; the mux reset value is not availability proof. Direct LED2 IRQ/WoL
source routes enforce the same prerequisite without requiring a pin profile.
Pin-owned and interface-owned LED1 clock uncertainty both invalidate dependent
IRQ/WoL views. Initialization readiness waits for every owned restoration tail.


## Stage33 shipped clock/interface profile

[Interface and clock controls](interface-clock.md) define `InterfaceSettings`,
bounded direct `configureInterface`, `InterfaceResult` and passive
`interfaceSnapshot`. Requests must match `Config.interface`, the immutable
application wiring declaration. Stopped MAC, pin availability, reference frequency
and stability, management clock and permission for both removal/reconnection are
explicit attestations, not measurements. The complete finite allowance is
admitted before desired intent changes. Live preflight may still reject active
commands or unowned power before mutation.

Every RCSR read passes through the sole collector, including atomic RMW and
readback. Observed RCSR mode has independent validity; it is never copied from
desired mode. RC history/polarity remains raw. Failed accepted mutation leaves
OP-9 debt with its original direct result; `startInitialize` is the separately
budgeted restoration. Read-only VERIFY uses the same owned-mask pass.
Isolation is optional independent ownership of BMCR[10], and writes use the
power owner's software intent rather than the hardware/software OR readback.
No direct-call token, scheduled silicon wait or retry is introduced.

## Quality and hardware link-loss ownership (stage34)

`startConfigureQuality(QualitySettings, OperationBudget, OperationToken&)`
uses OP-1 through OP-17 with one admitted profile, cooperative complete batches
and separate restoration. `qualitySnapshot()` retains desired, partial raw and
verified evidence. `readSignalQuality` stages a raw MSE word with original time
and cached identity/link context; `signalQualitySnapshot()` is bus-silent.
The [quality guide](quality.md) owns field admission, interface restrictions,
read effects and physical limits. These routes complete the five stage34 ledger
procedures without another collector or automatic recovery policy.

## Stage35 electrical and compliance contracts

`AnalogSettings` owns explicit amplitude, termination and slew groups;
`startConfigureAnalog` uses the existing slot and keeps desired/raw/trust separate.
Mandatory initialization setup precedes an admitted termination override; VERIFY
compares that desired mask. `ComplianceOptions`, `CollisionOptions` and
`CalibrationOptions` require explicit maintenance/fixture admission. Electrical
sessions use separate starts/stops and retained results, with no automatic dwell
or cleanup work. HARD stop/calibration completion leaves persistent restoration
to a new `startInitialize`. See [electrical procedures](electrical.md) for
source gates, legal transition ordering, finite bounds and physical limits.

The [prompts34-35 independent audit](reports/implementation/34-35-independent-audit.md)
adds regressions for OP-9 reset-versus-stimulus evidence, OP-15 current electrical
obligation clearing, combined-owner replay and interleaved reset observations.
A valid raw reset observation can terminate the pending maintenance operation
without turning that successful raw read into a transport failure.

The stage-36 integration review applies reset observation to every direct and
vendor-alias read, independent of the active feature. PCS/HARD withdraw current
identity/readiness/configuration; RESTART retains configuration and invalidates
link/history continuity. An interleaved operation freezes; owned reset polling
continues through its existing bounded phases. RESULT_PENDING remains historical,
including its identity/strap fields, after a later raw observation.
