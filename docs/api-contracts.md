# DP8382x value and direct-call contracts

Prompts 7–8 freeze this design on 2026-10-29, using the
[prompt-6 ownership decisions](design-decisions.md). These declarations are
documentation, **not shipped headers or implemented methods**. Introduce each
declaration with its working behavior/tests at its owning stage. The cooperative
contract below adds tokens, deadlines and retained cleanup to the audited direct
calls. Its clock-lag allowance makes the same time contract safe under quantization.

All declarations below are in namespace `DP8382x`. Types use PascalCase,
methods/fields camelCase, private fields `_camelCase`, enum/constants uppercase.
There are no `begin`/`init`/`state` aliases. Units and validity are part of each
contract; representation defaults never establish observations or chip defaults.
The [ledger](registers/dp83826i.json) and [independent vectors](register-vectors.md)
own silicon definitions. The [execution report](reports/implementation/7-value-transport-contracts.md)
records source revisions, deliberate peer differences and verification.

## Status and error details

```cpp contract
#include <array>
#include <cstdint>

namespace DP8382x {
/// @brief Library outcomes, not an SDK ABI or an I2C error namespace.
enum class Err : uint8_t {
    OK, IN_PROGRESS, INVALID_PARAM, INVALID_CONFIG, NOT_BOUND, NOT_INITIALIZED,
    UNSUPPORTED, DEVICE_NOT_FOUND, DEVICE_MISMATCH, MDIO_ERROR, MDIO_TIMEOUT,
    OPERATION_TIMEOUT, CONFIG_MISMATCH, BUSY, CANCELLED,
    RESULT_NOT_AVAILABLE, TOKEN_MISMATCH
};

/// @brief Copyable result with a signed, lossless backend detail where specified.
/// @note msg is non-null immutable static-lifetime text, never owned or freed.
struct Status {
    Err code{Err::OK};
    int32_t detail{0};
    const char* msg{""};
    constexpr bool ok() const noexcept { return code == Err::OK; }
    constexpr bool is(Err expected) const noexcept { return code == expected; }
    constexpr bool inProgress() const noexcept { return code == Err::IN_PROGRESS; }
    constexpr explicit operator bool() const noexcept { return ok(); }
    static constexpr Status Ok() noexcept { return {Err::OK, 0, "OK"}; }
    /// @pre code is a known non-OK error; msg has static lifetime and is non-null.
    /// @note Value construction only: transport-boundary validation is separate.
    static constexpr Status Error(Err code, const char* msg, int32_t detail = 0) noexcept {
        return {code, detail, msg};
    }
};
/// @return Uppercase enumerator name, or "UNKNOWN" for any invalid code; no I/O.
const char* errorName(Err code) noexcept;

/// @brief Local detail namespace for INVALID_PARAM, not backend error numbers.
enum class ParamDetail : int32_t {
    ENUM_VALUE = 1, ADDRESS_RANGE, VALUE_WIDTH, ACCESS_POLICY, BUDGET_RANGE
};
/// @brief Local detail namespace for INVALID_CONFIG.
enum class ConfigDetail : int32_t {
    TRANSPORT = 1, HEALTH_THRESHOLD, LINK_COMBINATION, SOURCE_GATE,
    BOARD_PREREQUISITE, CLOCK_CONTRACT, ENUM_VALUE, ADDRESS_RANGE, INTERFACE_REQUIRED,
    DEVICE_INVALIDATED
};
/// @brief Local detail namespace for BUSY.
enum class BusyDetail : int32_t {
    OPERATION_ACTIVE = 1, RESULT_PENDING, OWNER_ADMISSION,
    CONTROLLER_QUARANTINED, CLEANUP_REQUIRED, OBSERVATION_CHANGED,
    TOKEN_EXHAUSTED, RETIRE_REQUIRED
};
/// @brief Local detail namespace for DEVICE_NOT_FOUND / DEVICE_MISMATCH.
enum class IdentityDetail : int32_t {
    ZERO_ID = 1, ONES_ID, WRONG_OUI, WRONG_MODEL, REVISION_NOT_ACCEPTED,
    MODE_MISMATCH
};
/// @brief Local detail namespace for OPERATION_TIMEOUT.
enum class TimeoutDetail : int32_t {
    DEADLINE = 1, FRAME_BUDGET, BOUND_ADMISSION, POLL_BUDGET
};
/// @brief Local detail namespace for source-proved absent silicon behavior.
enum class UnsupportedDetail : int32_t { INTERFACE = 1, LINK_MODE, FEATURE };
}
```

These local detail values are explicitly cast to `int32_t`; the error code
determines the namespace. Zero means no additional detail where permitted;
callback admission/deadline errors require their specific reasons below. `OK` and
`IN_PROGRESS` have detail zero. `NOT_BOUND`, `NOT_INITIALIZED`, `CANCELLED`,
`RESULT_NOT_AVAILABLE`, `TOKEN_MISMATCH` and `CONFIG_MISMATCH` have detail zero;
the operation/verification result carries tokens and mismatch address/mask/
expected/observed values, never a lossy packed integer. Prompt 8 defines the
cooperative uses, not additional meanings for these values.

`MDIO_ERROR`/`MDIO_TIMEOUT` preserve the full signed backend diagnostic detail.
The adapter maps SDK errors to those library codes without exposing SDK types
or constants in portable headers. A callback-contract violation also returns
`MDIO_ERROR`, preserving the reported detail; a separate transport violation
field identifies the local failure and retains the original numeric Err value.
Never overwrite a backend detail with a local reason. Consumers branch on code,
not message text. A default `Status` is a value success; therefore output reports
from callbacks deliberately default to failure below.

Unknown enums are rejected before callbacks. `is` is equality only and does not
validate its argument. `ok`/bool never treat `IN_PROGRESS` or an invalid code as
success. `errorName` and `driverStateName` always return static text. A backend
must provide a valid static message pointer; arbitrary pointer lifetime cannot
be verified at runtime. A null message is detected, replaced with library static
text and reported as a contract violation; it is never dereferenced.

## Identity, capabilities and link intent

```cpp contract
namespace DP8382x {
/// @brief Declared orderable part; other variants are added with qualification.
enum class Variant : uint8_t { UNKNOWN, DP83826I };
/// @brief Observed identity mode, never a software mode selector.
enum class OperatingMode : uint8_t { UNKNOWN, BASIC, ENHANCED };
/// @brief Board declaration; RGMII is representable but absent on DP83826I.
enum class MacInterface : uint8_t { UNKNOWN, MII, RMII, RGMII };
/// @brief Managed admission policy, not the reset value of a revision field.
enum class RevisionPolicy : uint8_t { DP83826I_REV1_ONLY };
/// @brief Acceptance of complete raw identity under the selected source policy.
enum class IdentityAcceptance : uint8_t { UNASSESSED, ACCEPTED, REJECTED };

/// @brief Copied identity evidence; partial raw words never establish acceptance.
struct DeviceIdentity {
    Variant declaredVariant{Variant::UNKNOWN};
    uint16_t id1{0};                         ///< Meaningful only when id1Valid.
    uint16_t id2{0};                         ///< Meaningful only when id2Valid.
    bool id1Valid{false};
    bool id2Valid{false};
    uint32_t oui{0};                         ///< 22-bit OUI; requires decodedValid.
    uint8_t model{0};                        ///< 6-bit model; requires decodedValid.
    uint8_t revision{0};                     ///< Raw revision 0..15, not acceptance.
    OperatingMode mode{OperatingMode::UNKNOWN};
    bool decodedValid{false};                ///< Both raw reads decoded successfully.
    IdentityAcceptance acceptance{IdentityAcceptance::UNASSESSED};
    RevisionPolicy policy{RevisionPolicy::DP83826I_REV1_ONLY};
    Status admissionStatus{Err::NOT_INITIALIZED, 0, "No accepted identity"};
    uint64_t observedAtMs{0};                ///< Completion of complete acquisition.
    bool timeValid{false};
    bool stale{true};                        ///< Invalidation retains historical bytes.
};

/// @brief Link technology mask bits; these are API bits, not ANAR wire masks.
constexpr uint8_t LINK_10_HALF = 0x01;
constexpr uint8_t LINK_10_FULL = 0x02;
constexpr uint8_t LINK_100_HALF = 0x04;
constexpr uint8_t LINK_100_FULL = 0x08;
constexpr uint8_t INTERFACE_MII = 0x01;
constexpr uint8_t INTERFACE_RMII = 0x02;
constexpr uint8_t INTERFACE_RGMII = 0x04;

/// @brief Foundational source-derived capabilities after accepted identity.
/// @note valid=false makes all following capability values unknown, not absent.
struct Capabilities {
    bool valid{false};
    uint8_t linkModeMask{0};                  ///< LINK_* subset; DP83826I is 0x0F.
    uint8_t interfaceMask{0};                 ///< INTERFACE_*; DP83826I is 0x03.
    bool copper{false};
    bool fiber{false};
    bool gigabit{false};
};

enum class LinkSpeed : uint8_t { UNKNOWN, MBPS_10, MBPS_100 };
enum class Duplex : uint8_t { UNKNOWN, HALF, FULL };
enum class LinkMode : uint8_t { AUTONEGOTIATED, FORCED };
enum class PairSelection : uint8_t { AUTOMATIC, MDI, MDIX };

/// @brief AN intent; zero technology mask is intentionally invalid to configure.
struct Advertisement {
    uint8_t technologyMask{0};               ///< Nonempty LINK_* subset, no other bits.
    bool symmetricPause{false};              ///< Independent ANAR pause bit.
    bool asymmetricPause{false};             ///< Independent ANAR asym-pause bit.
    bool nextPage{false};                    ///< Requires the gated exchange owner.
};
/// @brief Forced intent; UNKNOWN fields must be filled before configuration.
struct ForcedLinkSettings {
    LinkSpeed speed{LinkSpeed::UNKNOWN};
    Duplex duplex{Duplex::UNKNOWN};
};
/// @brief Coupled pair/robust/mirror intent; not observed cable routing.
struct CrossoverSettings {
    PairSelection pairSelection{PairSelection::AUTOMATIC};
    bool robust{true};                       ///< Required for every forced mode.
    bool swapPair{false};                    ///< CR3 pair swap, distinct from PHYCR.
    bool swapPolarity{false};                ///< Port mirror requests both swaps.
};
/// @brief Persistent link intent, independent of binding and temporary diagnostics.
struct LinkSettings {
    LinkMode mode{LinkMode::AUTONEGOTIATED};
    Advertisement advertisement{};
    ForcedLinkSettings forced{};
    CrossoverSettings crossover{};
    bool extendedFullDuplex{false};          ///< Explicit partner-coordinated opt-in.
};

/// @brief Pure membership/range checks; no callbacks, clock, allocation or I/O.
bool isValidVariant(Variant variant) noexcept;
bool isValidPhyAddress(uint32_t phyAddress) noexcept;
Status validateLinkSettings(const LinkSettings& settings) noexcept;
}
```

`isValidVariant` accepts only `DP83826I`; `UNKNOWN` and other numeric casts are
invalid. Later documented parts are **not** prematurely exposed enum values
returning `UNSUPPORTED` for missing software. Add each at its qualification
stage. Physical marking declares I versus E; shared IDs cannot discover the
ordering suffix. `isValidPhyAddress` accepts the protocol range 0..31; the
DP83826's three physical address straps are separate board evidence.

Current source acceptance is explicitly OUI `0x080028`, model `0x11` BASIC or
`0x13` ENHANCED, revision **1 only**, under `DP83826I_REV1_ONLY`. This is the
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
capabilities are added with typed behavior; the feature matrix below already
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
without a full-duplex result. Selector 1 and local remote-fault/T4/reserved bits
zero are enforced by encoding; they are not user-settable fields. Inactive
forced fields may be UNKNOWN; valid known values are retained but ignored.

Forced mode requires one of 10/100 × half/full, robust crossover true for both
automatic and manual pair policy, and an inactive advertisement of mask zero,
pause false/false and nextPage false. This canonical inactive form prevents
accidental interpretation as negotiated policy. Each swap Boolean is legal;
port mirroring specifically sets both. Extended full duplex is representable
but source prerequisites require explicit coordinated partner evidence at the
operation owner; no Config Boolean proves the partner. Next-page true similarly
requires the owning exchange contract and unresolved source gate. Pure numeric
validation may pass these intents; operation preflight reports
`INVALID_CONFIG/SOURCE_GATE` when essential evidence is unavailable, before I/O.
10 Mbit operation in either mode requires the shared VOD initialization before
activation. Binding itself never applies these settings.

## Borrowed transport and direct-call budgets

One terminal batch remains the only transport mechanism. The public descriptor
is a bounded data format for a backend, not an unrestricted raw API. Only the
driver constructs/adopts batches after register policy checks.

```cpp contract
namespace DP8382x {
constexpr uint8_t MAX_BATCH_FRAMES = 8;
constexpr uint8_t NO_READ_INDEX = 0xFF;
enum class FrameKind : uint8_t { READ, WRITE };

/// @brief Absolute owner deadline in the same domain as Transport::nowUs.
struct IoBudget {
    uint64_t deadlineUs{0};                  ///< Exclusive deadline; now >= deadline expires.
    uint32_t maxFrames{0};                   ///< Total admitted physical frames, not retries.
};
/// @brief One C22 frame or masked write using an earlier read in this batch.
struct MdioFrame {
    FrameKind kind{FrameKind::READ};
    uint8_t registerAddress{0};              ///< 0..31, already semantically admitted.
    uint16_t value{0};                       ///< Literal write bits; ignored on read.
    uint16_t preserveMask{0};                ///< Earlier read bits to retain for write.
    uint8_t readIndex{NO_READ_INDEX};         ///< Earlier successful READ, or literal write.
};
struct MdioBatch {
    uint8_t phyAddress{0};                   ///< One PHY address for the complete batch.
    uint8_t frameCount{0};                   ///< 1..8; unused frames ignored/zero-initialized.
    std::array<MdioFrame, MAX_BATCH_FRAMES> frames{};
};
/// @brief Evidence for one frame; no successful value is synthesized on failure.
struct FrameResult {
    bool attempted{false};                  ///< Backend invoked/launched this frame.
    bool completed{false};                  ///< Controller reported terminal success.
    bool valueValid{false};                 ///< Successful read has a complete 16-bit word.
    uint16_t value{0};
    bool effectPossible{false};             ///< Failed write/read may have affected hardware.
    uint64_t completedAtUs{0};               ///< Completed frame only; valid if timeValid.
    bool timeValid{false};
};
/// @brief Terminal callback result, preserving primary and release outcomes.
struct BatchResult {
    Status status{Err::MDIO_ERROR, 0, "Batch result not supplied"};
    Status releaseStatus{Err::MDIO_ERROR, 0, "Release not established"};
    bool controllerReusable{false};          ///< Also false if a launched command is still busy.
    std::array<FrameResult, MAX_BATCH_FRAMES> frames{};
};
/// @brief Synchronous whole-batch execution; descriptors/context borrowed until return.
/// @pre Single instance owner; controller-wide serialization includes all other PHYs.
/// @return Terminal evidence for every completed prefix, including on any failure.
/// @note No reentry, retries, retained buffers, allocation or asynchronous completion.
using ExecuteBatch = BatchResult (*)(void* context, const MdioBatch& batch,
                                    const IoBudget& budget) noexcept;
/// @brief Bounded bus-silent monotonic clock supplied by the application.
using NowUs = uint64_t (*)(void* context) noexcept;

/// @brief Conservative per-batch call-segment bounds, supplied by the owner in us.
struct TransportBounds {
    uint32_t acquireUs{0};                   ///< Includes controller lock acquisition.
    uint32_t frameUs{0};                     ///< Positive bound including mandatory wire gap.
    uint32_t releaseUs{0};                   ///< Release/error-finalization allowance.
    uint32_t overheadUs{0};                  ///< Positive bound: fixed backend, clock/core/publication work.
    uint32_t clockLagUs{0};                  ///< Positive bound on sample age at nowUs return.
};
struct Transport {
    void* context{nullptr};                  ///< Borrowed; null is legal for stateless backends.
    ExecuteBatch executeBatch{nullptr};      ///< Required.
    NowUs nowUs{nullptr};                    ///< Required; no implicit platform clock.
    TransportBounds bounds{};
};
/// @brief Binding choices; no pins, hardware reset, SDK objects or retry policy.
struct Config {
    Variant variant{Variant::UNKNOWN};       ///< Required exact part declaration.
    uint32_t phyAddress{0};                  ///< Validate before narrowing to wire width.
    MacInterface interface{MacInterface::UNKNOWN}; ///< Required board declaration.
    OperatingMode requiredMode{OperatingMode::UNKNOWN}; ///< UNKNOWN accepts either valid mode.
    RevisionPolicy revisionPolicy{RevisionPolicy::DP83826I_REV1_ONLY};
    Transport transport{};
    uint32_t offlineThreshold{3};            ///< Application health policy; valid 1..UINT32_MAX.
};
}
```

Clock time is nonwrapping `uint64_t` microseconds, monotonic within one binding;
all deadline additions must be checked for overflow. There is no modular-wrap
comparison. Sample time zero is valid. `nowUs` itself is terminal/bounded and
performs no MDIO, driver reentry or state mutation. Absolute time avoids granting
fresh allowance after waiting for owner admission. The caller charges its own
instance-gate acquisition before entering the direct call. Core samples before
and after callbacks and before each later batch. The raw sample is a lower time
bound; raw + clockLagUs is an upper bound at clock-callback return. The owner
proves this lag including floor quantization, clock-read latency and scheduling;
resolution alone is insufficient. overheadUs covers subsequent dispatch and
publication work. Core uses the upper bound for admission/output allowance and
the raw lower bound for minimum-wait completion. A backward raw sample or overflow
forming its upper bound latches this
binding's clock invalid until successful end/rebind after owner repair; a later
larger sample does not silently restore it. All timed I/O then rejects with
`INVALID_CONFIG/CLOCK_CONTRACT` before batch callbacks. Local snapshots, result
delivery and bus-silent lifecycle remain possible. Retain effects and mark
time-dependent snapshots stale; suppress new ordinary outputs/timestamps from
the affected call. Historical timestamps remain historical, never a valid age
comparison against the broken current timeline. A post-callback regression
preserves any earlier normalized primary failure, then any release failure or
unsafe-controller error; only an otherwise successful callback returns
`INVALID_CONFIG/CLOCK_CONTRACT`. TransportSnapshot.clockInvalid retains this
second fact even when MDIO_TIMEOUT/-37 remains the primary error. Frozen time
cannot extend finite frame/step work.
Prompt 8 adds cooperative scheduling rules in this same domain.

Complete call-segment bound for n frames is
`acquireUs + n * frameUs + releaseUs + overheadUs`, evaluated in checked 64-bit
arithmetic. Both `frameUs` and `overheadUs` must be positive. overheadUs covers
bounded clock calls, fixed backend work, core validation/accounting and final
snapshot/output publication, conservatively per batch even in a compound call.
Acquisition/release zero is legal only if no corresponding time is needed.
Config with null required callbacks or frameUs/overheadUs/clockLagUs zero is
`INVALID_CONFIG/TRANSPORT`. Owners must provide
truthful conservative platform allowances, including scheduler/controller/clock
quantization, not just nominal loop delays. Core cannot prove or preempt a
backend's timing. The pinned SDK's nominal 1000 us/frame is not a proved bound;
its adapter must establish/report the missing platform allowance before a
bounded native-support claim. No guessed default is supplied.

Validate input and admission first. With an otherwise legal call, deadline
expired at the raw lower sample returns `OPERATION_TIMEOUT/DEADLINE`;
an upper bound reaching the deadline while raw remains earlier returns
`OPERATION_TIMEOUT/BOUND_ADMISSION` (not proved late). Zero/insufficient frame allowance
returns `OPERATION_TIMEOUT/FRAME_BUDGET`; insufficient remaining time for the
indivisible batch returns `OPERATION_TIMEOUT/BOUND_ADMISSION`. All three make no
batch callback for that batch. Admission requires the full bound **strictly less
than** remaining time; equality cannot guarantee completion before the exclusive
deadline. After a callback, also reserve overheadUs for remaining core/clock/output
work before publishing: if current time is before the deadline but that reserve
does not fit strictly, return OPERATION_TIMEOUT/BOUND_ADMISSION with unchanged
ordinary output and retained frame effects. This conservative post-check does
not grant more time or perform another batch. Never start a partial MMD/RMW just to spend the
remaining budget. Subsequent batches share the original deadline and subtract
actual attempted frames; no new allowance or hidden retry is created. Small
register calls require one C22 frame or four MMD frames; internal C22 RMW two
and fully reselected MMD RMW eight. Compound observation budgets are specified
below. A raw post-callback sample >= deadline proves late, even with successful
hardware. An upper bound alone reaching it denies publication but does not set
late=true. Cached completion stamps encode the conservative upper bound. The
current lower bound minus the observation upper bound, clamped at zero, is only
a **lower bound on age**, not proof of maximum-age freshness. Millisecond fields
store floor(upperUs/1000); recover a conservative upper endpoint as
storedMs*1000+999 with checked arithmetic before using that lower-age calculation.
A maximum-age policy needs an observation lower bound (for example the owner's
valid pre-acquisition time) and current upper time, or a separately proved
uncertainty allowance including timestamp rounding. The copied timestamp alone
does not supply that proof; cached access never invents a fresh observation.

The backend validates frame count/indices before acquiring/launching. Literal
write requires `readIndex=NO_READ_INDEX` and `preserveMask=0`. A dependent write
uses `(earlier.value & preserveMask) | value`; require an earlier READ in the
same batch and `(value & preserveMask)==0`. No conditional opcode or register
policy is delegated. Reads require literal/dependency fields at defaults.
Validate PHY/C22 ranges and all descriptor enum values, including poisoned
storage. Whole-word MMD construction sets all reserved and upper bits to zero;
DEVAD31 control words are exactly `0x001F`/`0x401F`. Public `uint32_t` inputs
are checked before conversion, including data/offset <=0xFFFF.

Execute under one controller lock through the complete batch and stop on the
first error. Results form an attempted prefix and a successful completed prefix;
at most the final attempted frame fails. Read `valueValid` requires completed
READ; completed READ requires a valid word. Completed WRITE never has a read
value. Unattempted/unused slots must equal default FrameResult values. Primary
OK requires exactly frameCount attempted and completed frames, every requested
READ value valid, and no extra attempted slot. A shorter successful prefix is
never success. Failed attempted frames set effectPossible=true unless the
backend independently proves no hardware write/consumption occurred; absence
of an acknowledgement alone is not that evidence. Timestamps are
recorded at observed completion or a conservative later sample, never before
the frame. A supplied frame timestamp is already a conservative upper bound on
completion, is optional, requires completed=true, and must lie between the core's
pre-callback raw sample and post-callback upper bound in nondecreasing frame order.
With timeValid=false its stored timestamp is zero. Core can use its valid
post-callback upper bound as a conservative fallback for a completed frame. An
out-of-range/out-of-order frame timestamp is a FRAME_REPORT violation; a backward
core clock sample instead follows CLOCK_CONTRACT and invalidates new time
evidence. Retain the raw report, but never use an invalid stamp for age or waits.
The backend must retain successful destructive-read words even when
a later frame fails. Core transfers them to the single collector before other
instance access. Failed destructive reads may lose unreturned history.

Preferred/conforming backends release controller ownership on **every** terminal
return, including error; `releaseStatus=OK` records that fact (or proves no lock
was acquired). If release fails
or is unknown, retain the release error and set `controllerReusable=false`.
An unfinished controller command also sets false even if its software mutex was
released. The application backend owns shared quarantine across all users; any
later batch invocation returns `BUSY/CONTROLLER_QUARANTINED` without frames until
the application proves idle/release through its controller-specific recovery.
There is no driver API that declares an unknown controller safe. The core retains
loss of trust; clearing the backend interlock is not configuration reconciliation.
Local `end`/rebind cannot release or erase application-owned controller resources.

Allowed primary callback codes are `OK`, `MDIO_ERROR`, `MDIO_TIMEOUT`, `BUSY`
and `OPERATION_TIMEOUT`. BUSY means no frames attempted and only owner/silence/
quarantine admission: its detail must be OWNER_ADMISSION or
CONTROLLER_QUARANTINED. OPERATION_TIMEOUT requires a defined TimeoutDetail and
covers exhausted aggregate budget, while
MDIO_TIMEOUT means the controller/lock attempt timed out. `IN_PROGRESS`, unknown
Err, other codes, null message or inconsistent frame/effect reports normalize
to `MDIO_ERROR`. Preserve original code/detail and all independently valid
prefix evidence; never trust contradictory values or manufacture completion.
Release codes are only OK/MDIO_ERROR/MDIO_TIMEOUT; other values are violations.
OK with nonzero detail, or an invalid local admission/deadline detail, is a
STATUS_CODE violation. With release OK, controllerReusable=false classifies an
additional MDIO_ERROR/detail 0 with static message "Controller state uncertain"
and CONTROLLER_STATE, except for a valid zero-frame BUSY/CONTROLLER_QUARANTINED
report of a preexisting fault. This additional failure becomes the public return
only when the primary succeeded; otherwise retain primary precedence. Retain
raw primary/release reports independently.

The **trusted application controller wrapper** validates/classifies frame reports
and terminal statuses before making the shared controller available, while
returning the original report/code/detail evidence unchanged for diagnostics.
Core publishes the normalized outcome separately; no invalid numeric Err is lost.
Its shared admission remains closed through unlock/finalization and opens only
after release and report validity are established. Invalid/unknown completion
keeps it quarantined. This is part of ExecuteBatch's backend contract, not an
ability of a core-local snapshot to control another PHY. Core rechecks defensively;
if it detects a violation that escaped that wrapper, report MDIO_ERROR and stop
this instance, and require owner-wide controller shutdown before further use.
Such an out-of-contract backend cannot retroactively guarantee that another
instance did not enter in the gap. No extra lock/fault callback or second
transaction mechanism is introduced to pretend otherwise.

Result precedence after a callback: normalized primary failure first; otherwise
release failure/unsafe-controller error; otherwise detected clock regression;
otherwise late aggregate deadline or insufficient publication allowance;
otherwise chip semantic result. A failed callback remains MDIO_ERROR/TIMEOUT
even if it also overran the deadline; the retained late flag records both facts.
Successful-but-late work returns OPERATION_TIMEOUT with unchanged ordinary output
and retained effects. `CONFIG_MISMATCH` requires a successful in-budget readback
of documented stable owned bits, not a failed/late read or reset default.

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
    uint64_t lastOkMs{0};
    uint64_t lastErrorMs{0};
    bool lastOkTimeValid{false};
    bool lastErrorTimeValid{false};
    Status lastError{Err::OK, 0, "No managed transport failure"};
};
enum class NegotiationState : uint8_t { UNKNOWN, DISABLED, IN_PROGRESS, COMPLETE };
/// @brief A coherent observation, with optional independently qualified resolution.
struct LinkSnapshot {
    bool valid{false};                      ///< Valid link-up/down observation exists.
    bool stale{true};                       ///< Invalidated/failed refresh; history retained.
    bool linkUp{false};                     ///< Ignore unless valid and not stale.
    NegotiationState negotiation{NegotiationState::UNKNOWN};
    LinkSpeed speed{LinkSpeed::UNKNOWN};
    Duplex duplex{Duplex::UNKNOWN};
    bool resolvedModeValid{false};          ///< Both speed and duplex independently resolved.
    bool pauseRx{false};
    bool pauseTx{false};
    bool pauseValid{false};                 ///< Full-duplex AN resolution only.
    uint64_t observedAtMs{0};               ///< Completed coherent acquisition, floor(us/1000).
    bool timeValid{false};
    Status lastRefreshStatus{Err::NOT_INITIALIZED, 0, "No link observation"};
};
enum class ConfigurationTrust : uint8_t { UNKNOWN, VERIFIED, DIRTY };
/// @brief Foundational link settings view; no claim to verify future feature fields.
struct SettingsSnapshot {
    LinkSettings desired{};
    bool desiredValid{false};
    LinkSettings observed{};
    bool observedValid{false};              ///< Complete stable link-setting tuple only.
    bool observedStale{true};
    ConfigurationTrust trust{ConfigurationTrust::UNKNOWN};
    uint64_t observedAtMs{0};
    bool timeValid{false};
    Status lastVerificationStatus{Err::NOT_INITIALIZED, 0, "No settings verification"};
};
enum class TransportViolation : uint8_t {
    NONE, STATUS_CODE, NULL_MESSAGE, FRAME_REPORT, RELEASE_REPORT, CONTROLLER_STATE
};
/// @brief All-source transport diagnostics, separate from managed health/chip counters.
struct TransportSnapshot {
    uint32_t totalBatches{0};                ///< Every executeBatch invocation, saturating.
    uint32_t attemptedFrames{0};             ///< All admitted frame attempts, saturating.
    uint32_t completedFrames{0};             ///< Reported successful physical frames.
    bool lastBatchValid{false};
    BatchResult lastBatch{};                ///< Exact last backend report, not new ownership.
    Status lastStatus{Err::OK, 0, "No transport attempt"}; ///< Normalized batch/time/release outcome.
    TransportViolation violation{TransportViolation::NONE};
    bool late{false};                       ///< Positive late evidence; unusable if clockInvalid.
    bool clockInvalid{false};               ///< Binding latch, including faults before a batch.
    bool controllerReusable{false};         ///< Last known report; never authorizes I/O.
};
enum class InvalidationReason : uint8_t {
    EXTERNAL_RESET, EXTERNAL_MUTATION, RAW_MUTATION, AMBIGUOUS_MUTATION
};
}
```

Health counts **one managed batch callback invocation** as one transaction,
not one public method or one physical frame. One four-frame MMD callback is one
transaction; a compound observation can contain multiple transactions. Attribute
traffic internally, never with a caller-writable "managed" Boolean:

| Traffic/outcome | Managed health accounting |
| --- | --- |
| Initialization identity/configuration, normal typed configuration/readback, live link collector, explicit managed reconciliation | One success/failure per attempted managed batch; source/semantic result is separate. |
| Public raw access, explicit diagnostic probes/sessions, cleanup/restoration after a failed operation, SDK bridge forwarding itself | All-source transport counters only; no extra managed count or accidental recovery. A bridge never double-counts the core batch. |
| Pure validation; valid zero-frame BUSY/OPERATION_TIMEOUT with clean release and reusable controller | No managed success/failure, even if the backend checked admission under the controller lock. |
| Valid zero-frame BUSY/CONTROLLER_QUARANTINED with clean current-call release | No managed success/failure; controllerReusable=false reports the preexisting quarantine, not a new attempt or recovery. |
| Controller lock acquisition timeout, MDIO failure, malformed report, failed release or newly unsafe controller state | One managed failure even with no frames. Preserve completed prefix/frames; a primary BUSY does not hide a release failure. |
| OPERATION_TIMEOUT with one or more attempted frames | One managed failure; accepted/uncertain effects remain retained. |
| Otherwise successful managed callback returns at or after its deadline | One managed timing failure with OPERATION_TIMEOUT/DEADLINE and late=true; completed effects remain retained. |
| Otherwise successful managed batch cannot fit the post-callback publication reserve | One managed timing-contract failure with OPERATION_TIMEOUT/BOUND_ADMISSION, even if the current timestamp is still before the deadline. No second failure count; no fabricated late=true. |
| Clock regression before a batch / after an otherwise successful managed batch | No count before callback; after a callback, one managed timing-contract failure with INVALID_CONFIG/CLOCK_CONTRACT and no new valid timestamp. This is not evidence of PHY failure. An existing primary MDIO failure still counts only once. |
| Successful bus reads yielding wrong identity, CONFIG_MISMATCH, cable-down or policy mismatch | Managed transport success; semantic failure/link state remains separate and visible. |

These rules use returned statuses, release/controller evidence and frame flags;
there is no hidden acquisition-attempt flag. A controller lock timeout returns
MDIO_TIMEOUT and counts, whereas an aggregate-budget rejection before any frame
with successful release and reusable controller does not. Every callback
invocation increases `totalBatches`; frame totals count reported attempted/completed
frames only.
On malformed reports, count only the independently valid prefix and mark the
violation; counts must not be invented from success status alone.

For a counted failure, health lastError records normalized primary
MDIO_ERROR/MDIO_TIMEOUT first, otherwise the normalized release/unsafe-controller
error, otherwise the operation/clock timing failure. This is the cause of the
health count, not necessarily the public call's primary status. For example,
primary BUSY/OWNER_ADMISSION plus release MDIO_ERROR/-37 remains BUSY publicly
but records one health failure with lastError=MDIO_ERROR/-37. Invalid reports
normalize to MDIO_ERROR as above. An otherwise clean, excluded no-attempt
rejection plus clock regression remains uncounted; clockInvalid still latches.
At most one managed count is produced per callback.

All counters saturate at UINT32_MAX. A managed failure increments totalFailures
and consecutiveFailures, records normalized status and completion-time
`lastErrorMs` if valid. Replacing lastError also replaces its timestamp validity:
if that failure has no valid completion time, set lastErrorMs=0 and
lastErrorTimeValid=false. Never attach an older error's valid timestamp to a
new error. An earlier lastOk/time-valid remains historical evidence; the clock
fault latch prevents using it to compute age in the broken timeline.
A managed success increments totalSuccess, resets the streak to zero and updates
lastOk/time-valid. It **preserves lastError and its
timestamp**. Successful cleanup/raw/probe traffic cannot change the streak.
Configuration trust is never repaired by a transport success.

`initialized=false` forces health state UNINIT, including while first/repeated
initialization is active or after reset invalidation. Managed attempts during
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
after required result/cleanup acknowledgement; it is not this statistics reset.
Health methods are introduced only in prompt 21. Prompt 11 introduces actual
transport accounting. A working initialized PHY must not expose placeholder
UNINIT/READY methods before those behaviors exist.

Cached accessors return value copies without callbacks (including clock), I/O,
polling, acknowledgement or retry. Age is computed externally from a timestamp
and validity in the same time domain; no cached accessor silently expires data
or accepts a zero timestamp as "not observed". `stale` records known invalidation
or failed refresh, not application age policy. A prior valid snapshot can remain
`valid=true, stale=true` as history. A never-observed snapshot has valid=false.
On a successful refresh, stale=false and lastRefreshStatus=OK even for link-down.
On failed live refresh, preserve prior values/observedAtMs/timeValid as historical
evidence, set stale=true and lastRefreshStatus to the returned error. An ordinary
caller output remains unchanged. A clock fault additionally prevents comparing
that history to the now-invalid time domain; it never manufactures a fresh stamp.

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

## Cooperative values

These fixed-size values are copied. They contain no caller buffers, handles,
allocator, callback scheduler or generic feature registry. Add operation kinds
and payloads with real feature consumers, not speculative union alternatives.

```cpp contract
namespace DP8382x {
/// @brief Nonzero serial scoped to one nonmovable driver's object lifetime.
struct OperationToken { uint64_t value{0}; };
constexpr OperationToken INVALID_OPERATION_TOKEN{};
enum class OperationKind : uint8_t { NONE, INITIALIZE, RESET, CONFIGURE, RECOVER };
enum class OperationState : uint8_t { IDLE, RUNNING, CLEANUP, RESULT_PENDING };
enum class OperationPhase : uint8_t {
    NONE, PREFLIGHT, IDENTIFY, QUIESCE, COMMAND, WAIT, RESTORE, VERIFY, COMPLETE
};
enum class ResetKind : uint8_t { PCS, HARD, RESTART };
enum class RecoveryMode : uint8_t { VERIFY_ONLY, REAPPLY, PCS_RESET, HARD_RESET };
enum class CleanupState : uint8_t { NOT_REQUIRED, PENDING, SUCCEEDED, FAILED, DISCHARGED };
/// @brief Application attestation, never a PHY readback or core cleanup success.
enum class QuiescenceDisposition : uint8_t {
    NONE, EXTERNALLY_RESET_OR_POWERED_OFF, ISOLATED, TRANSFERRED_TO_OWNER
};

/// @brief Two immutable absolute allowances reserved together at admission.
struct OperationBudget {
    IoBudget primary{};                    ///< Absolute deadline and total primary frames.
    IoBudget cleanup{};                    ///< Later absolute deadline; separate total frames.
    uint32_t maxPrimaryPolls{0};            ///< Positive; includes zero-work/wait-only polls.
    uint32_t maxCleanupPolls{0};            ///< Positive; no renewed allowance on cancel.
    uint32_t pollIntervalUs{1000};          ///< Positive scheduling policy, not silicon timing.
};
struct InitializeOptions {
    bool pcsReset{true};                    ///< Explicit PCS command after accepted identity.
};
struct ResetOptions { ResetKind kind{ResetKind::PCS}; };
struct RecoverOptions { RecoveryMode mode{RecoveryMode::REAPPLY}; };

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
/// @brief One attributed transport receipt, copied before a later batch overwrites it.
struct OperationBatchEvidence {
    bool valid{false};
    OperationPhase phase{OperationPhase::NONE};
    MdioBatch request{};
    BatchResult report{};
    Status normalized{Err::RESULT_NOT_AVAILABLE, 0, "No batch evidence"};
    TransportViolation violation{TransportViolation::NONE};
    bool late{false};
};
/// @brief Retained operation evidence, including failed/uncertain mutation and cleanup.
struct OperationResult {
    OperationToken token{};
    OperationKind kind{OperationKind::NONE};
    ResetKind resetKind{ResetKind::PCS};     ///< Meaningful only for RESET.
    RecoveryMode recoveryMode{RecoveryMode::REAPPLY}; ///< Meaningful only for RECOVER.
    Status primaryStatus{Err::RESULT_NOT_AVAILABLE, 0, "No operation"};
    Status cleanupStatus{Err::RESULT_NOT_AVAILABLE, 0, "No cleanup decision"};
    CleanupState cleanupState{CleanupState::NOT_REQUIRED};
    bool mutationAttempted{false};          ///< Semantic target write launched; excludes selectors.
    bool mutationCompleted{false};          ///< At least one semantic target write acknowledged.
    bool mutationPossible{false};           ///< Failed target write may have applied.
    bool commandRequired{false};            ///< Qualifies commandAccepted/protocolComplete.
    bool commandAccepted{false};            ///< Requested reset/activation command acknowledged.
    bool protocolComplete{false};           ///< Required completion explicitly observed in budget.
    bool temporaryMayBeActive{false};       ///< Pending reset/stimulus/power transition evidence.
    bool persistentRestored{false};         ///< All currently owned requested persistent state proved.
    bool clockInvalid{false};
    bool primaryLate{false};
    bool cleanupLate{false};
    bool retiredByOwner{false};             ///< Disposition prevents any further binding I/O.
    QuiescenceDisposition disposition{QuiescenceDisposition::NONE};
    DeviceIdentity identity{};             ///< Includes partial/rejected identity evidence.
    StrapEvidence straps{};                ///< Raw captured words; reset provenance stays explicit.
    SettingsSnapshot settings{};           ///< Operation evidence; historical once terminal.
    ConfigurationMismatch mismatch{};
    OperationBatchEvidence primaryBatch{};  ///< First failing batch, else last primary batch.
    OperationBatchEvidence cleanupBatch{};  ///< First failing batch, else last cleanup batch.
};
/// @brief Bus/clock-silent copy; never advances work, acknowledges or consumes evidence.
struct OperationSnapshot {
    OperationState state{OperationState::IDLE};
    OperationPhase phase{OperationPhase::NONE};
    OperationResult result{};              ///< Live evidence; terminal primary may await cleanup.
    uint64_t nextPollAtUs{0};                ///< Conservative due time; valid only with nextPollValid.
    bool nextPollValid{false};
    IoBudget primaryRemaining{};
    IoBudget cleanupRemaining{};
    uint32_t primaryPollsRemaining{0};
    uint32_t cleanupPollsRemaining{0};
};
}
```

Only PCS options/kinds enter shipped headers at prompt 15; HARD/RESTART arrive
with prompt 16, CONFIGURE with 17?18 and RECOVER with 22. This design shows the
complete foundational choices without publishing unsupported software methods.
`ConfigurationMismatch` holds the first actual stable-bit disagreement; a later
read failure keeps it while the primary error follows transport precedence.

## Exact local and direct declarations

```cpp contract
namespace DP8382x {
/// @brief One owner-private PHY instance with borrowed terminal transport.
/// @note All calls, even const snapshots, require application serialization.
/// @pre Destructor runs only after successful end and native callback quiescence.
class DP8382x {
public:
    DP8382x() noexcept;
    ~DP8382x() noexcept;
    DP8382x(const DP8382x&) = delete;
    DP8382x& operator=(const DP8382x&) = delete;
    DP8382x(DP8382x&&) = delete;
    DP8382x& operator=(DP8382x&&) = delete;

    /// @brief Validate and replace a quiescent binding locally; never touch PHY/clock.
    /// @retval INVALID_CONFIG Invalid transport/health/board declaration; old state unchanged.
    /// @retval BUSY Active operation, retained result or owed cleanup; old state unchanged.
    Status bind(const Config& config) noexcept;
    /// @brief Local detach only, after operation/result/cleanup obligations are resolved.
    /// @note No reset, power-down, SDK uninstall, backend release or implicit cancel.
    Status end() noexcept;
    bool isBound() const noexcept;           ///< Local binding validity; not bus readiness.
    bool isInitialized() const noexcept;     ///< Accepted current initialization, not link.
    /// @brief Invalidate current device evidence locally; preserve terminal operation receipts.
    /// @note Stops active primary work and rechecks remaining cleanup; never reopens a result.
    Status invalidateDeviceState(InvalidationReason reason) noexcept;

    /// @brief Read one policy-admitted stable C22 register under the whole-batch owner.
    /// @param[out] value Assigned only on complete, in-budget success; otherwise unchanged.
    /// @param[in] budget Original owner deadline and remaining frame allowance.
    Status readClause22(uint32_t registerAddress, uint16_t& value,
                        const IoBudget& budget) noexcept;
    /// @brief Write one admitted C22 value; accepted/late effects survive failure.
    Status writeClause22(uint32_t registerAddress, uint32_t value,
                         const IoBudget& budget) noexcept;
    /// @brief Read one admitted DEVAD/16-bit offset via a complete nonincrementing batch.
    /// @param[out] value Unchanged unless every frame and release succeed before deadline.
    Status readMmd(uint32_t devad, uint32_t registerAddress, uint16_t& value,
                   const IoBudget& budget) noexcept;
    /// @brief Write one admitted MMD value; never leave selectors exposed between calls.
    Status writeMmd(uint32_t devad, uint32_t registerAddress, uint32_t value,
                    const IoBudget& budget) noexcept;
    /// @brief Acquire current link through the sole destructive-evidence collector.
    /// @param[out] snapshot Fully assigned on OK, including down; unchanged on failure.
    /// @note Consumed samples remain retained even when this aggregate fails.
    Status readLinkStatus(LinkSnapshot& snapshot, const IoBudget& budget) noexcept;

    DriverState driverState() const noexcept;       ///< Cached; introduced with prompt 21.
    HealthSnapshot healthSnapshot() const noexcept; ///< Cached; introduced with prompt 21.
    void resetStatistics() noexcept;                ///< Clear only managed cumulative totals.
    LinkSnapshot linkSnapshot() const noexcept;     ///< Cached; no I/O/clock/consumption.
    SettingsSnapshot getSettingsSnapshot() const noexcept; ///< Cached desired/observed/trust.
    TransportSnapshot transportSnapshot() const noexcept;  ///< Cached all-source batch evidence.


    /// @brief Admit initialization locally; no clock/MDIO call or output buffer retention.
    /// @param[out] token Assigned only on IN_PROGRESS; unchanged on rejection.
    Status startInitialize(const InitializeOptions& options, const OperationBudget& budget,
                           OperationToken& token) noexcept;
    /// @brief Admit exact software reset and persistent replay, never GPIO action.
    /// @param[out] token Assigned only on IN_PROGRESS; unchanged on rejection.
    Status startReset(const ResetOptions& options, const OperationBudget& budget,
                      OperationToken& token) noexcept;
    /// @brief Validate/copy new persistent link intent and admit its verified application.
    /// @param[out] token Assigned only on IN_PROGRESS; unchanged on rejection.
    Status startConfigure(const LinkSettings& settings, const OperationBudget& budget,
                          OperationToken& token) noexcept;
    /// @brief Admit verify/reapply/explicit reset recovery even when current trust is dirty.
    /// @param[out] token Assigned only on IN_PROGRESS; unchanged on rejection.
    Status startRecover(const RecoverOptions& options, const OperationBudget& budget,
                        OperationToken& token) noexcept;
    /// @brief Advance the exact token by at most one indivisible terminal batch.
    /// @param[in] slice Original owner deadline and this call's physical-frame allowance.
    /// @return IN_PROGRESS through primary/cleanup work; terminal status once result pending.
    Status pollOperation(OperationToken token, const IoBudget& slice) noexcept;
    OperationSnapshot getOperationSnapshot() const noexcept;
    /// @brief Consume one resolved/discharged terminal result; does no clock or bus I/O.
    /// @param[out] result Fully copied on OK, unchanged on any error.
    /// @retval BUSY Cleanup remains owed; copying a result cannot discharge it.
    Status takeResult(OperationToken token, OperationResult& result) noexcept;
    /// @brief Freeze primary cancellation locally; later admitted polling may clean up.
    /// @return OK acknowledges the request, not hardware rollback or operation success.
    Status cancel(OperationToken token) noexcept;
    /// @brief Transfer owed hardware cleanup to a quiescent external owner; no callbacks.
    /// @pre Caller has stopped/drained callbacks and established the declared disposition.
    /// @note Preserves failed/unrun cleanup evidence and makes this binding retire-only.
    Status acknowledgeQuiescence(OperationToken token,
                                 QuiescenceDisposition disposition) noexcept;
};
}
```

All drivers and native/application operation owners are non-copyable and
non-movable, **even unbound**. There is no conditional bound-object move, hidden
second owner, automatic cancellation in a destructor or transferred native
callback address. Values such as Config and snapshots are copyable; copying a
Config copies borrowed pointers, not resource ownership. The application must
retain their context, clock and controller resources through callbacks and any
pending cleanup. A driver destructor has no I/O and cannot fix a violated
lifetime precondition. Prompt 8 makes the retained-operation obligations exact.

`bind` validates a temporary Config before touching current state. It rejects
invalid Variant/interface/mode/policy enums with INVALID_CONFIG/ENUM_VALUE,
address >31 with INVALID_CONFIG/ADDRESS_RANGE, missing callbacks/bounds with
INVALID_CONFIG/TRANSPORT, zero threshold with INVALID_CONFIG/HEALTH_THRESHOLD
and UNKNOWN interface with INVALID_CONFIG/INTERFACE_REQUIRED;
RGMII yields UNSUPPORTED/INTERFACE for DP83826I. Null context alone is legal.
It does not call executeBatch/nowUs, inspect hardware or enforce a guessed strap
address. Only an idle, acknowledged instance can be rebound; even an identical
valid Config begins a new uninitialized binding. Accepted bind resets local
identity/capabilities/observations/statistics and default intent validity, never
hardware or the shared controller's fault/silence state. Invalid/busy bind leaves
the previous binding and all evidence unchanged. Ordinary settings changes use
the later configuration operation, not rebinding.
The lifetime token counter is excluded from those resets. An acknowledged
retire-only binding rejects bind with BUSY/RETIRE_REQUIRED after slot backpressure
checks; successful end is required before its fresh binding.

`end` is idempotent OK when already unbound, otherwise BUSY for active work,
pending result or owed cleanup. Successful end clears borrowed binding/local
state only after the owner has delivered or acknowledged retained evidence;
it never frees the application controller or makes a quarantined bus reusable.
The cooperative contract below defines exact acknowledgement/token routes.
`invalidateDeviceState` rejects invalid reasons unchanged, returns NOT_BOUND
when unbound, otherwise works even during an active operation: mark trust DIRTY,
identity/link/observed settings stale, capabilities invalid and initialized false;
retain desired intent, counters, samples and result/effect evidence. It signals
the active operation to stop normal progression and preserve cleanup obligations
under the state-specific rules below. A pending terminal result remains historical;
current device snapshots show the invalidation. No invalidation reason repairs
SDK caches or controller state.

Direct calls return terminal status, never IN_PROGRESS. No token is needed for
these bounded short calls. Admission rejects incompatible active work, pending
cooperative result or cleanup as BUSY; the native owner also admits lifecycle/
maintenance state. Raw writes require maintenance and invalidate affected trust
on accepted **or uncertain** mutation. The portable caller owns maintenance
serialization; native wrappers enforce their reservation. No argument can bypass
access policy. Explicit raw read access is available after binding for legal
stable addresses before initialization; writes and feature/link operations need
accepted current initialization. Initialization's internal procedures have their
own admitted bootstrap path, not a public unsafe-write flag.

The direct raw methods reject C22 holes, selectors REGCR/ADDAR, read-clear/latch
registers and recipe-only/gated accesses with INVALID_PARAM/ACCESS_POLICY, or
INVALID_CONFIG/SOURCE_GATE for an otherwise defined source-gated operation.
MMD alias access inherits the target's policy, so it cannot bypass these rules.
Reads of destructive locations use typed collector APIs (such as readLinkStatus),
not a raw flag. Writes reject reserved/prohibited values and command/session
actions requiring a typed procedure. A register labelled R/W is not automatically
safe for full-word writes: use the exact ledger composition policy. Mixed or
dependent settings without a proved direct-write policy reject. No auto-retry,
scan, reset, implicit recovery or selector cleanup occurs. On rejection or failed
read, the ordinary output reference is untouched. `transportSnapshot` retains
the last batch report for owner copying; its receipt is replaced by the next
batch, while destructive/operation evidence has separate retention.
The raw lastBatch receipt contains no register descriptor: the initiating call
or typed collector supplies address/phase attribution. It is not a standalone
transaction trace; no generic operation registry is introduced.

### Live link result and failure examples

`readLinkStatus` requires initialized identity and owner admission, but not
verified desired configuration merely to observe. It always collects two BMSR
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
automatic retry**. The owner may request a new bounded observation. Prompt 19
implements the detailed masks and source gates; new evidence changing this order
or cap requires a coordinated contract/caller update, not a silent extra loop.

Valid UP with pending negotiation may have unresolved mode/pause. An unresolved
forced PHYSTS interpretation returns a valid link observation with
resolvedModeValid=false, never invented speed/duplex from BMCR configuration.
Complete AN resolution requires current link, AN complete and sufficient coherent
source evidence. Pause RX/TX are independently resolved only for full-duplex AN;
half duplex, forced mode and pending AN leave pauseValid=false. The SDK cannot
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

## State and error precedence

Validate syntactic inputs/enums/widths first, then binding/capability/admission,
then budget, then callbacks and semantic readback. A rejected call changes no
ordinary output, binding, desired intent or managed health; passive diagnostic
recording must not fabricate a transport attempt.

| Situation | Result / detail | State and evidence |
| --- | --- | --- |
| Invalid enum, C22 >31, DEVAD >31, offset/data >65535, illegal access | INVALID_PARAM with ENUM_VALUE/ADDRESS_RANGE/VALUE_WIDTH/ACCESS_POLICY | Zero clock/batch callbacks for pure input failure; output unchanged. |
| Invalid binding declaration, missing transport/bounds/threshold | INVALID_CONFIG with typed reason | bind is bus/clock silent and preserves prior state. |
| Direct call unbound / managed call not initialized | NOT_BOUND / NOT_INITIALIZED | No batch; no implicit init. Legal pre-init stable raw reads remain available. |
| RGMII requested on accepted/declared DP83826I | UNSUPPORTED/INTERFACE | Source-proved absent silicon, rejected before I/O. |
| Required software not yet implemented | No published method/declaration in shipped facade | Never successful stub or UNSUPPORTED claim about silicon. |
| Known feature branch has unresolved essential source evidence | INVALID_CONFIG/SOURCE_GATE at pure typed preflight | Keep exact named ledger gate in feature result/diagnostics; no speculative I/O. |
| Plausibility fails after valid ID reads | DEVICE_NOT_FOUND/ZERO_ID or ONES_ID | Not proof of absent hardware; raw successful reads are transport successes. |
| Observed wrong OUI/model/revision/mode | DEVICE_MISMATCH with exact IdentityDetail | Preserve raw evidence, no managed identity acceptance. |
| Controller failure / controller or lock timeout | MDIO_ERROR / MDIO_TIMEOUT, signed backend detail | Preserve successful prefix, uncertain effects and release result. |
| Detected backward clock | Preserve any normalized primary failure, then release/unsafe-controller error; otherwise INVALID_CONFIG/CLOCK_CONTRACT | Clock-invalid latch blocks timed I/O until owner repair/end/rebind; retain both facts and effects, publish no new valid output/time. An otherwise clean no-frame admission rejection remains uncounted in managed health. |
| No remaining aggregate time/work, bound cannot fit, or successful callback returned late | OPERATION_TIMEOUT with deadline/frame/bound detail | No partial admission; late writes/reads can have effects despite unchanged ordinary output. |
| Successful in-budget stable-bit readback disagrees | CONFIG_MISMATCH/0 | Trust DIRTY; retain expected/observed/mask/space in typed result; transport health can be READY. |
| Link/AN changes during aggregate observation | BUSY/OBSERVATION_CHANGED | Cached history stale, ordinary output unchanged; collector retains completed samples. |
| Active operation/pending result/owed cleanup | BUSY with distinct reason | No overwritten token/result; cooperative rules below define the exact transitions. |
| Release failed after primary failure | Preserve primary error; separate release status | Shared controller quarantine, retained evidence; no retry until owner resolution. |
| Raw write accepted or fails after possible acceptance | Actual primary status, including OPERATION_TIMEOUT if otherwise successful but late | Invalidate affected trust/link; desired settings unchanged; no automatic rollback. |
| Cached value is stale/unknown | Return copied validity/stale metadata, not a fake transport error | No I/O; caller decides age/admission policy. |

## Plain-owner and native design examples

These examples use the frozen declarations; `owner` operations describe
application responsibilities, not extra DP APIs. Neither example is a linked
consumer or permits physical fixture operations.

```cpp example
// Application supplies terminal runBatch and monotonic readClock functions.
DP8382x::Status bindPortable(DP8382x::DP8382x& phy, void* context,
                  DP8382x::ExecuteBatch runBatch, DP8382x::NowUs readClock,
                  const DP8382x::TransportBounds& provedBounds) {
    DP8382x::Config config{};
    config.variant = DP8382x::Variant::DP83826I;
    config.phyAddress = 0;
    config.interface = DP8382x::MacInterface::RMII;
    config.requiredMode = DP8382x::OperatingMode::ENHANCED;
    config.transport = {context, runBatch, readClock, provedBounds};
    // Explicit startInitialize/pollOperation/takeResult follows successful binding.
    return phy.bind(config); // Owner-serialized; failure preserves the prior binding.
}

DP8382x::Status observePortable(DP8382x::DP8382x& phy,
                              const DP8382x::IoBudget& ownerBudget,
                              DP8382x::LinkSnapshot& published) {
    // Core stages output: failure preserves published; OK includes valid DOWN.
    return phy.readLinkStatus(published, ownerBudget);
}
```

The owner holds instance serialization for each call, supplies a deadline formed
before waiting for admission with checked arithmetic, and reserves ten frames
for the link pass. The application backend shares one controller lock and
silence/quarantine state with other PHYs. It releases the lock on callback return;
neither sleep nor console output is inside it. On failure the owner copies
`transportSnapshot` and collector/operation evidence before another request;
successful diagnostics do not implicitly restore trust. Two instances use
separate objects and contexts or explicitly shared controller context, never
copies/moves of a bound object.

Native design chain remains: owner reservation → public stopped-driver SDK
control → expected owned PHY callback → same core procedure/batch → actual MAC.
No instance gate is held across an SDK call that synchronously reenters. Full
advertisement is a shared PHY procedure; AN enable/disable, speed and duplex
use the public SDK controls and verify `G_AUTONEGO` agreement. Example AN-to-
forced transition: validate the entire `LinkSettings`, stop/drain, prepare
coupled PHY fields, use `ETH_CMD_S_AUTONEGO(false)`, speed and duplex controls,
verify stable settings/cache, then allow start. A failed step retains partial
effects; do not patch caches or replay UP. `readLinkStatus` stages output exactly
as in the portable example, while native notifications require a valid resolved
tuple and journal their own possible effects.

The incoming hook handle resolves the actual owned PHY/MAC during install;
external raw mediator/ioctl frames remain rejected even when another request is
admitted. Core raw methods are used only through the typed native owner route.
SDK peer pause is a Boolean projection, not the portable RX/TX result. Raw BMCR
mutation, invalidation and accepted-then-failed writes require public stopped
reconciliation before restart. Native synchronous callbacks consume only their
own exact operation result into the native journal using the token rules below.
The pinned SDK lifecycle correction remains mandatory.

## Complete typed feature map

The following is a responsibility map, **not frozen future function signatures**.
Every row includes all ledger fields with that feature `owner`, plus the fields
and actions explicitly used by its linked/delegated procedures. Legal values
are exactly each field ID's `mask`, `legal`, `prohibited`, `access`, `validity`
and register/procedure/action gates. A `range` is the unshifted field encoding
excluding prohibited values; it does not authorize writes to read-only fields
or invent physical meanings for unspecified codes. Procedure prerequisites
constrain combinations. This rule covers all 283 field records, opaque recipe
actions and shared dependencies without copying a second numeric register map.
All current gates in [feature coverage](feature-coverage.md) apply to the named
branch; none removes the whole documented feature from implementation scope.

| Feature ID | Planned typed operation/configuration | Observable result | Ledger procedure IDs |
| --- | --- | --- | --- |
| identity | Declared Variant; identity/capability acquisition | Raw/decoded IDs, acceptance and source/physical evidence separately | identify |
| access | C22/MMD direct access and internal side-effect-aware update | Value or retained frame/effect report | direct_access, mmd_access |
| reset | PCS/hard/circuitry reset and external-reset reconciliation | Completion, lost trust, accepted effects, cleanup | pcs_reset, hard_reset, restart, external_reset |
| initialization | Mandatory 10BASE-Te preparation | Verified initialization evidence | initialize_10base |
| negotiation | LinkSettings, advertisement and AN controls | Desired/observed/trust, negotiation and directional pause | configure_an |
| next_page | Typed local pages and bounded exchange | Partner pages, partial handshake and completion | next_page_exchange |
| crossover | ForcedLinkSettings, crossover/mirror/extended duplex | Configured mode versus observed routing/link | configure_forced, extended_link_configure |
| link | readLinkStatus and cached LinkSnapshot | Link/current-history validity, age and resolution | observe_link |
| eee | EEE initialization/settings/status | Local/partner capability, request and negotiated state | eee_initialize, eee_negotiation_status |
| lpi | Finite legacy-MAC LPI session | Accepted/maybe-active state, wake/exit readiness | legacy_lpi |
| wol | Wake MAC/password, pulse/level/CRC and acknowledgement | Verified settings and retained wake/error evidence | wol_configure, wol_acknowledge |
| power | IEEE power, active-sleep, PLL and deep-power policy | Desired policy, observed power and exit/trust | power_ieee, power_active_sleep, power_pll_control, power_deep |
| repeater | Coordinated two-PHY topology | Both owners' results, partial effects/restoration | repeater_configure |
| clock | Clock output selection and dependencies | Configured policy and qualified observed state | clock_configure |
| mii | MII controls/isolation | Verified interface settings/isolation | mii_configure, isolation_configure |
| rmii | Leader/follower, revision, elasticity, RX_DV and shift | Verified settings and collected status | rmii_configure |
| tx100 | Scrambler and idle RX-error policy | Verified data-path settings | tx100_configure |
| tx10 | Squelch/threshold/NLP/jabber/preamble/gain | Verified settings with physical interpretation limits | tx10_configure |
| loopback | All documented near/digital/reverse/external modes | Active/maybe-active mode and restoration | loopback_near, loopback_digital, loopback_reverse, loopback_external |
| bist | Generator/checker, packet/IPG/counter modes | Activity/lock, retained finite error sample | bist_run, bist_capture |
| cable | Manual/autorun settings and finite capture | Freshness-qualified raw peaks/amplitude/polarity | cable_configure, cable_manual, cable_autorun, cable_capture |
| fast_drop | MII-only causes/thresholds | Verified settings and retained causes | configure_fast_drop |
| link_recovery | Hardware link-loss recovery | Verified policy | link_recovery_configure |
| led | Per-pin sources/polarity/blink/stretch/manual drive | Verified per-pin values | led_configure |
| pin_mux | Exact per-pin mux and conflicts | Valid roles and read-only observations | pin_configure |
| interrupts | Global routing/polarity/enables/test/service | Verified masks, retained causes | interrupt_configuration, interrupt_test |
| counters | Single destructive acquisition owner | Per-sample time/validity, partial counts/history loss | collect_event_counters |
| quality | Raw MSE and source-proved interpretation | Raw value/validity, no invented SNR | signal_quality_sample |
| compliance | Collision test and finite admitted stimulus | Applied/maybe-emitting state, stop/cleanup | collision_test, compliance_session |
| analog | Amplitude/termination/slew/output profile | Verified settings and legal intermediate restoration | analog_configure |
| compatibility | IPG and compatibility policy | Verified settings | compatibility_configure |
| calibration | Calibration contingency recovery | Recovery/reset/reinitialization evidence | calibration_recovery |
| straps | Read-only captured strap observation | Raw/decoded words, mode/reset provenance | Hardware observation; no strap setter |
| outside_phy | Application pins/clocks/power/MAC/stack/scheduling | Application-owned evidence | Outside core |
| absent | Exact-part capability validation | Explicit source-backed rejection | No invented operation |

Required legal-value coverage includes every mask/range above, notably all four
10/100 modes and four pause advertisements; loopback encodings {0,1,2,4,8,16}
as alternatives, not combinable flags; four BIST PRBS modes; TDR averages
{1,2,4,8,16,32,64}, segments 1..5 and uninterpreted raw polarity; RMII elasticity
codes 0..3 mapping to 14/2/6/10 bit tolerance; LED sources 0..E (F reserved),
LED0 route-enable only 1 and read-only LED0/3 mux distinctions; power-save codes
0/2 only; 10BASE-Te squelch codes 0..8 for 200..600 mV; analog minus codes
0x28..0x38 and plus 0x08..0x18, with legal split-field transition prefixes; and
six-byte WoL MAC/password values with their different exact word packing.
EEE/LPI opaque/gated actions remain explicit, not guessed fields. Typed owning
stages freeze feature-specific signatures/results alongside actual behavior.

Raw access alone completes none of these multistep features. Hardware-only
straps, source-proved absent capabilities and application responsibilities stay
separate from unimplemented software. Physical qualification remains external
evidence identified by exact board/part/revision/firmware/fixture; no bool in a
source-derived Capabilities value grants it.


## One cooperative operation and retained result

All calls use the same instance owner as direct I/O. The instance owns one slot;
there is no task, scheduler, registry, second configuration engine or retained
caller buffer. Starts copy options, budgets and requested intent, return
IN_PROGRESS on admission and assign one token. They call neither nowUs nor MDIO.
Admission is not command acceptance, initialization, link or restoration.

### Admission and token identity

Start rejection precedence is: syntactic options/settings and structural budget
shape; NOT_BOUND; slot BUSY (active before result); retire-only; known clock fault;
identity/capability/source admission; zero budget; token exhaustion. Structural
budget checks are positive-deadline ordering and pollIntervalUs, while zero
allowances are semantic budget failures. Thus unbound plus zero budget returns
NOT_BOUND, active plus broken clock returns BUSY/OPERATION_ACTIVE, and retired
plus a pending result returns BUSY/RESULT_PENDING. Rejected
starts leave the output token, desired settings and all evidence unchanged.
Primary/cleanup deadlines and frame/poll allowances must be positive;
pollIntervalUs must be positive. Zero frame/poll allowance returns respectively
OPERATION_TIMEOUT/FRAME_BUDGET or POLL_BUDGET; zero deadline returns
OPERATION_TIMEOUT/DEADLINE. Cleanup deadline must exceed primary deadline;
invalid ordering or zero pollIntervalUs returns INVALID_PARAM/BUDGET_RANGE.
Absolute times are caller-created with checked addition. A nonzero deadline
already in the past cannot be detected without a clock call at start; the first
poll freezes timeout before I/O. Neither start nor bind proves board readiness.

RUNNING/CLEANUP rejects new starts as BUSY/OPERATION_ACTIVE. RESULT_PENDING
rejects them as BUSY/RESULT_PENDING even when its primary failed; owed cleanup
is visible in the snapshot. Retire-only rejects all timed I/O/starts as
BUSY/RETIRE_REQUIRED. A known broken clock rejects as INVALID_CONFIG/CLOCK_CONTRACT.
Reset/configure and VERIFY_ONLY recovery require accepted current initialization.
Initialize and other recovery modes have an internal identity/readiness bootstrap
path and do not require verified hardware trust; OFFLINE never denies them.
No desired link profile is a valid empty owned-profile choice: recovery still
restores mandatory initialization without inventing link intent. Invalid stored
intent rejects as INVALID_CONFIG/LINK_COMBINATION before writes. Source-gated requests reject the whole
plan before mutation, including a recipe's preliminary reset.

Accepted startInitialize immediately sets initialized=false and marks prior
identity/observations stale; even cancel-before-mutation leaves UNINIT until a
new initialization succeeds. Other starts retain initialized until a reset is
attempted or possibly applied; then they clear it immediately, including on a
failed/late command, without waiting for acknowledgement. Repeated initialization
never exposes placeholder READY while active. Desired/counter/result evidence
follows its separate rules.

The counter belongs to the **object lifetime**, outside binding state. First
admission allocates 1; UINT64_MAX is allocated once and the next start returns
BUSY/TOKEN_EXHAUSTED, unchanged output, no callbacks. No wrap, token reset on
end/bind or reuse after take. Tokens are scoped to the exact object: the owner
retains the object association with each request. Equal numbers on two instances
are not transferable credentials. Destroying/reconstructing an object requires
invalidating its application request identities; old tokens are not valid for
the new lifetime, even at a reused address. No global uniqueness is promised.

Zero/future tokens return TOKEN_MISMATCH. With an occupied slot any token other
than its exact token returns TOKEN_MISMATCH. With an empty slot a previously
issued nonzero token returns RESULT_NOT_AVAILABLE; only a lifetime counter is
needed, not a history registry. Wrong/absent-token calls preserve state, counters,
outputs and retained evidence and make no callbacks. Local input validation
precedes this lookup; ordinary end/rebind never rewinds the counter.

### States, publication and consumption

| Current state and event | Next state / result |
| --- | --- |
| IDLE, accepted start | RUNNING/PREFLIGHT; token allocated; primary IN_PROGRESS, cleanup undecided, no callbacks. |
| RUNNING, legal poll finishes a bounded step or remains before a wait | RUNNING/IN_PROGRESS; copy effects and nextPollAtUs. No lock survives the call. |
| RUNNING, full in-budget success | RESULT_PENDING; primary OK, required protocol/verification evidence published, cleanup NOT_REQUIRED/OK. |
| RUNNING, primary error or cancel, no cleanup obligation | RESULT_PENDING; frozen primary error, cleanup NOT_REQUIRED/OK. |
| RUNNING, primary error or cancel after cleanup-requiring effect | CLEANUP; freeze primary error, cleanup PENDING/IN_PROGRESS, retain readiness barriers and effects. |
| CLEANUP, admitted bounded cleanup progress | CLEANUP/IN_PROGRESS; primary immutable, no renewed budget. |
| CLEANUP, restoration/stop verified | RESULT_PENDING; cleanup SUCCEEDED/OK; preserved primary failure and historical effects. |
| CLEANUP, error, exhausted allowance, invalid clock or unavailable safe route | RESULT_PENDING; cleanup FAILED with exact cause, obligation still owed; no automatic retry. |
| CLEANUP or RESULT_PENDING with owed cleanup, valid owner acknowledgement | RESULT_PENDING; cleanup DISCHARGED, retire-only binding; unrun/failed cleanup status and effects preserved. |
| RESULT_PENDING, exact take with NOT_REQUIRED/SUCCEEDED/DISCHARGED cleanup | IDLE; full result copied once, then slot cleared. Binding quarantine/retire/clock/trust state is not repaired. |
| RESULT_PENDING, exact take while cleanup FAILED | BUSY/CLEANUP_REQUIRED; output and result unchanged. |

While RUNNING/CLEANUP, takeResult returns RESULT_NOT_AVAILABLE. To inspect a
frozen primary error during cleanup, copy getOperationSnapshot; it never consumes
or advances work. pollOperation returns IN_PROGRESS while either primary or
cleanup remains active. Once RESULT_PENDING it returns primaryStatus if failed,
otherwise failed cleanupStatus, otherwise OK. Thus expiry freezes the promised
OPERATION_TIMEOUT and can never later become successful completion: cleanup may
continue under its separate allowance and its IN_PROGRESS is explicitly not
primary success. Polling an already pending result is bus/clock silent, repeats
that terminal return and consumes no additional allowance. takeResult's OK means
successful **delivery**; the caller must inspect both statuses in the copied
result. A failed take must not clear the owner's token or native journal.

The result keeps accepted/uncertain effects even after verified restoration.
mutationAttempted, mutationCompleted and mutationPossible are historical ORs;
restoration does not clear them. commandAccepted requires acknowledged command
write, protocolComplete requires its documented completion observation before
the primary deadline, and persistentRestored requires all applicable owned
persistent state proved. temporaryMayBeActive is cleared only by proved stop/
completion, not by cancellation or owner transfer. These are independent of
LinkSnapshot: cable-down is compatible with successful initialization/reset.
REGCR/ADDAR selector writes in an MMD read remain physical frame evidence but
do not set semantic mutation flags or create a restoration obligation. Cleanup
is determined by actual target/control effects, never by the presence of any
WRITE descriptor. commandRequired=false makes commandAccepted/protocolComplete
inapplicable (both false), for example VERIFY_ONLY or initialization without a
requested command; such an operation can still succeed. It never synthesizes
reset/negotiation completion from a no-command path.

Before another batch replaces transportSnapshot, copy primary evidence into
primaryBatch. Keep the first failing receipt, otherwise the last primary receipt;
cleanup uses cleanupBatch independently. OR late/clock/effect history into the
operation result, retaining signed primary and release details. A bounded result
is not a complete trace. The single destructive collector retains every consumed
partial event/counter sample separately; result backpressure cannot erase it.
A mismatch record survives a subsequent transport failure even though the latter
becomes the primary result **within the same finite verification pass**: record
the first mismatch, continue only its remaining planned reads, then freeze
CONFIG_MISMATCH if all transport succeeds. Do not freeze the primary until that
pass finishes/fails. Once frozen, no cleanup error can replace it. Identity
evidence follows prompt 7's partial validity.

nextPollValid is false in IDLE, newly admitted PREFLIGHT, RESULT_PENDING and any
clock-invalid state. False during active PREFLIGHT means no wait is scheduled;
the owner may poll immediately. True names the earliest legal hardware-action
time, never a promise that the owner will run then. Terminal snapshots do not
offer a stale deadline for rescheduling.

### Poll work, time and finite limits

A poll advances at most **one** indivisible MDIO batch (1..8 frames). There is
no same-call transition from primary I/O into cleanup I/O. Any bounded local
phase transitions stop at the next I/O, wait, terminal state or cleanup boundary;
the procedure's fixed acyclic phase plan bounds that work. No recursive replay.
Every exact-token call entering RUNNING or CLEANUP consumes one poll allowance
for that phase, including zero-frame and too-early polls. The final allowed poll
may do one legal batch; if still unfinished at its return, freeze
OPERATION_TIMEOUT/POLL_BUDGET immediately. A subsequent call is not required to
reveal exhaustion. Primary exhaustion schedules cleanup where owed; cleanup
exhaustion publishes FAILED, with no further library attempts.

Sample time at poll entry and after each terminal callback, then classify the
result before another phase can claim completion. Use the original phase deadline
and remaining frames; never regenerate allowances from the current time. A
positive slice uses min(slice.deadlineUs, phase.deadlineUs) and min(slice.maxFrames,
phase.remainingFrames). Admission includes the full selected batch bound and
publication reserve from the direct contract. A zero slice.maxFrames is explicitly
bookkeeping-only: ignore its slice deadline, issue no batch, but check the global
phase deadline, clock fault and poll exhaustion. An expired positive slice or
insufficient positive slice bound ends the current phase with OPERATION_TIMEOUT;
it is not a terminal-looking error while quietly continuing primary work.
If cleanup becomes active the poll returns IN_PROGRESS with that primary timeout
frozen in its snapshot, as specified above.

Before the first mutation, reserve the chosen successful path's minimum frames,
polls and time (command, at least one completion check, all applicable minimum waits,
verification/restoration). The primary budget's finite polling allowance limits
additional completion checks. pollIntervalUs is a positive owner policy between
incomplete completion reads, default 1000 us, **not** a silicon minimum or maximum.
Reset command admission also reserves one interval plus its first completion
read, even when the command might clear sooner. A positive primary allowance
below this required interval/batch/restore sum returns
OPERATION_TIMEOUT/BOUND_ADMISSION before mutation. A small per-call slice need
only fit its batch because hardware waits span calls; it never shortens the
operation's required interval. Each later incomplete poll schedules its next
read from fresh completion time, not an old caller timestamp. If the next legal
read plus its bound cannot fit before the primary deadline, freeze timeout now.
No remaining frame allowance admits a partial MMD/RMW. Cleanup obeys the same
rules under its independently reserved deadline, frame and poll counters.
With one batch per poll, the available poll count includes the current call
(one plus its remaining count after entry). A command cannot be admitted on
the final poll if its mandatory completion/verification requires another poll.
Before mutation, the reserved cleanup frames/polls and the cleanup window after
the primary deadline must also fit the plan's minimum safe stop/settle/restore
route. Reject insufficient cleanup capacity before mutation with FRAME_BUDGET,
POLL_BUDGET or BOUND_ADMISSION as appropriate. All reserve arithmetic includes
clock-lag padding for conservative wait anchors and uses checked wide additions.
These are necessary capacity checks, not guarantees of hardware completion or
timely owner scheduling; actual late callbacks can exhaust even reserved cleanup.

For raw sample L and declared lag E, use U=L+E (checked) as conservative upper
now. A validated FrameResult timestamp already bounds completion from above;
otherwise use the refreshed post-callback U. Due = completionUpper + minimumUs,
checked for overflow. Only a later raw lower sample >= Due permits the next
hardware action. Overflow forming U is CLOCK_CONTRACT and latches invalid time;
overflow forming a requested Due is OPERATION_TIMEOUT/BOUND_ADMISSION, retaining
effects without pretending the clock broke. Delay calls merely schedule owner
wakeups; elapsed-time checks decide readiness. A floor RTOS tick multiplied into
microseconds is not a sufficiently precise completion anchor by itself.

A proven late callback has raw post-sample >= effective deadline. Otherwise,
upper/post-publication allowance crossing the deadline denies success with
BOUND_ADMISSION and late=false. Preserve an earlier normalized transport error,
then release/unsafe-controller failure, then clock fault, then deadline/bound
failure as in direct calls. IN_PROGRESS from ExecuteBatch is MDIO_ERROR, never
an asynchronous hardware operation. A synchronous terminal backend cannot be
preempted. Every claimed bound depends on its actual controller, lock, clock
and scheduling contract; the uncorrected SDK's nominal loop limit is insufficient.

Frozen time cannot increase total frames or polls. It also cannot satisfy a
future minimum wait: primary and cleanup may both exhaust without any further
MDIO, retaining a pending reset/stimulus. If the caller stops polling, no library
step executes and no unconditional wall-clock completion is promised. External
owner shutdown can still inspect/cancel/acknowledge locally. Physical readiness
and finite software work are different claims.

### Readiness and source-specific procedure anchors

External RESET_N/POR stays with the board/controller coordinator: SNLS647I 7.6
and 8.3.11 require at least 25 us reset assertion without debounce capacitors,
2 ms shared-bus inactivity after release and 50 ms after power-up. The latter
readiness numbers occupy MAX columns; the prose makes them mandatory waiting
windows. A delayed external clock requires additional reset after POR and at
least 100 us clock stability (7.6 footnote 2). Other PHYs cannot issue MDC during
that silence. Core start/bind does not touch pins or establish this prerequisite.
BMCR/PHYRCR software reset instead polls its actual self-clearing command; no
exact source maximum was found and external-pin intervals are not substituted.
FLP 1500 ms and EEE wake/sleep figures are nominal, not operation deadlines.

The first implementation's phase plan is explicit: accepted identity from two
C22 reads; capture SOR1/SOR2 through their admitted MMD paths when required;
read BMCR/PHYRCR command state and wait for any already-active reset before
configuration rather than blindly issuing another command; optionally issue PCS
reset via neutral BMCR composition; wait/read BMCR until
reset clears; apply/verify mandatory 10BASE-Te state; replay/verify currently owned
desired settings; publish. HARD uses PHYRCR 0x1F command 0x8000 and re-identifies;
RESTART uses 0x1F command 0x4000 and its retained-register scope. Never poll C22
address 0x8000. Raw strap acquisition does not prove a new pin sample; operation
kind/options and external board history supply provenance, retaining the
hard-reset strap gate. Each register access remains under its ledger policy.

The pinned SDK's reset/power loops write before evaluating timeout/10 ms, and
its AN loop can return success after timeout/100 ms polls without completion.
Neither loop is reused. Native scheduling checks elapsed high-resolution time;
tick rounding may delay the next eligible call but never advances its due time.
An owner can use a 10 ms interval, but a deadline shorter than that required
interval cannot justify issuing the reset. The executable synthetic budget
vectors cover this admission failure separately from measured hardware timing.

### Cancellation, incomplete cleanup and teardown

cancel is bus/clock silent. In RUNNING it freezes CANCELLED/0 unless an earlier
primary error has already frozen, stops normal progression and schedules only
owed cleanup. Before any effect, cancellation yields RESULT_PENDING immediately.
In CLEANUP or RESULT_PENDING, repeated cancel is idempotent OK and does not
restart allowances, overwrite primary failure or cancel required cleanup.
A concurrent call while a terminal callback executes is prohibited by the owner;
there is no interrupt-cancel or software preemption claim.

Cleanup is **reserved at start**, not allocated a fresh timeout when primary
fails. It contains only source-permitted stop/settle/replay/verification steps
and runs solely through subsequent authorized owner polls. First cleanup failure
is terminal; remaining effects and cleanup FAILED are retained. A primary deadline
cannot be borrowed or extended by cleanup. A controller quarantine, board silence
or outstanding reset readiness barrier also applies to cleanup; cleanup cannot
issue an early frame to make teardown convenient. A denied attempt yields its
actual failure without hidden retries. Unknown identity after reset forbids
configuration writes until re-identification succeeds. Failed cleanup never
restores health by itself; independent verification is needed for trust.

end remains local and rejects RUNNING/CLEANUP, RESULT_PENDING, or an owed
obligation without callbacks. Rebind has the same backpressure and never cancels.
Result consumption alone does not resolve failed cleanup. The explicit escape is:

1. Owner closes new admission and drains native/application callbacks, with no
   instance/controller lock held across SDK drain calls. Inspect/copy full evidence.
2. Owner establishes the declared external disposition: reset/power removal with
   applicable readiness responsibility, physical isolation, or transfer to a
   continuing controller/board owner that retains recovery obligations and resources.
   Quiescence alone is not proof that an active stimulus stopped.
3. For the exact token with frozen primary and PENDING/FAILED cleanup, call
   acknowledgeQuiescence with a non-NONE disposition. It invokes no clock/bus/SDK
   callback, sets cleanup DISCHARGED and retiredByOwner, preserves effects and the
   earlier cleanup error. Unattempted cleanup is recorded RESULT_NOT_AVAILABLE,
   never OK. A pending cleanup's IN_PROGRESS becomes RESULT_NOT_AVAILABLE with
   static text stating that the owner took responsibility.
4. Copy/consume the result successfully, then end. The binding is retire-only
   after acknowledgement: even startRecover cannot run until successful end and
   fresh bind. The new binding may use a repaired clock; token allocation still
   continues from the object's lifetime counter.

acknowledgeQuiescence rejects invalid/NONE disposition before state changes,
wrong tokens as above, RUNNING as BUSY/OPERATION_ACTIVE, and an already resolved
cleanup as INVALID_PARAM/ACCESS_POLICY. Repeating the same acknowledgement before
take is idempotent OK; a different disposition is INVALID_PARAM/ACCESS_POLICY.
It cannot clear controller-wide quarantine, free borrowed native resources or
claim persistentRestored. A continuing external owner must retain those resources
until actual release/recovery is proved. Destruction still requires successful
end and native callback quiescence. This resolves the clock-fault/cleanup cycle
without permitting timed I/O on a broken clock or erasing unperformed cleanup.

### Desired state, invalidation and acyclic restoration

At accepted startConfigure, copy validated requested settings into the persistent
desired view and mark its relationship to hardware unverified. Rejected admission
changes nothing. Cancellation/failure preserves that accepted intent; cleanup may
restore it under the cleanup allowance even though the primary remains CANCELLED
or failed. Before any mutation no cleanup is owed merely because desired changed:
the previous observed tuple stays historical and trust remains DIRTY until the
new intent is applied/verified. This choice makes cancellation stop primary work,
not silently replace the owner's requested policy with an older one.

Temporary diagnostic overrides never change persistent desired intent. Capture
every touched readable documented owned field, with masks/access semantics, before
mutation; save all pieces of split fields and verify legal intermediate values.
For recipe-only/unreadable fields use only an exact source-backed stop/reset/reapply
route. A missing restoration route rejects the entire plan at pure preflight;
never guess a saved word or add a speculative read. Partial reset/configuration
invalidates affected trust and link observations as soon as mutation is possible.
Accepted reset sets initialized=false until the required bootstrap and replay
complete. Raw public mutation is denied during a slot. A valid external mutation
notification through invalidateDeviceState always invalidates current device
snapshots as specified above, then follows the operation state:

| Operation state | Effect of invalidation on the slot |
| --- | --- |
| IDLE | Remain IDLE; no token, result or cleanup is created. External effects belong to the notifying owner until an explicit admitted recovery. |
| RUNNING | Freeze primary INVALID_CONFIG/DEVICE_INVALIDATED; preserve consumed samples and effects. Enter CLEANUP only for an existing operation obligation, otherwise RESULT_PENDING with cleanup NOT_REQUIRED/OK. |
| CLEANUP | Preserve the frozen primary and accumulated effects. Recheck the remaining safe cleanup route on the next authorized poll, keeping original deadlines and remaining frame/poll allowances; never restart or extend cleanup. If no safe route remains, publish cleanup FAILED with INVALID_CONFIG/DEVICE_INVALIDATED. |
| RESULT_PENDING, including FAILED or DISCHARGED cleanup | Preserve the entire terminal receipt, statuses, effects and disposition. Do not reopen work, change a successful historical outcome or retry failed/discharged cleanup. Failed cleanup still requires owner disposition before take; DISCHARGED remains retire-only. |

Active snapshots copy the invalidated current evidence; terminal result fields
describe the completed operation's historical observation, not current trust.
Current getSettingsSnapshot/linkSnapshot/isInitialized expose the invalidation
even while that receipt awaits delivery. New external hardware effects and their
cleanup belong to the notifying owner; they cannot be folded into an already
terminal operation or cleared by consuming its result. Invalidation itself is
bus/clock silent in every state and cannot perform or prove restoration.

All four operations share private chip procedures, not public start calls:

1. Validate the complete request and compile its finite order from currently
   implemented owned fields and source gates; establish board/controller readiness.
2. Stop/quiesce any admitted temporary stimulus before reset or restoration.
3. Execute the selected reset primitive once, if requested/required, and observe
   the actual self-clearing command at BMCR or PHYRCR. The primitive never calls
   the restore dispatcher. No unspecified reset maximum is invented.
4. Re-identify/address/mode as required, retain strap evidence, and apply mandatory
   initialization such as source-backed 10BASE-Te preparation.
5. Execute qualified persistent recipe actions, then interface/clock/power
   dependencies, coupled crossover/compatibility, advertisement/page state and
   AN/forced activation, then event enables. Verify stable owned bits and qualified
   procedure completion. A reset-bearing recipe contributes its reset at step 3,
   not a recursive call back to this complete plan.
6. Publish complete initialized/restoration evidence; observe current link separately.

Initialization requires complete accepted identity and currently implemented
mandatory initialization; it can succeed unplugged. InitializeOptions.pcsReset
selects the PCS primitive, not hard reset or pins. No persistent link profile is
invented if none was supplied; when a valid desired profile exists it is included
as its features become implemented. startReset always restores currently owned
persistent state after its exact reset kind; protocol completion alone is not
operation success. startRecover VERIFY_ONLY performs bounded identity/stable-bit
checks and never initializes an uninitialized object or mutates hardware. REAPPLY
may bootstrap and replay dirty hardware; reset modes explicitly select their
primitive. No recovery mode requires current configuration trust before repair.

EEE/compliance are future consumers of this same order. Their essential gates
(EEE wire/control/exit semantics and opaque compliance preservation) reject before
any recipe reset. When qualified, an EEE reset runs once and its post-reset recipe
then precedes ordinary persistent replay. Compliance cleanup stops/reset-clears
stimulus before replay; it never replays temporary COMPT/loopback/BIST/TDR triggers.
Command/status bits and consumed events are not desired fields. No reset inside
recipe setup can recursively invoke that same whole recipe.

## Operation acceptance scenarios and examples

The following are exact future-driver test recipes. The companion arithmetic
probe in test/test_operation_timing.py executes timing/finite-limit design cases
now; these tables do not claim execution of an unimplemented driver.

| Scenario / owner calls | Required result, effects and finite next action |
| --- | --- |
| bind; startInitialize; legal polls; takeResult | IN_PROGRESS admission; one batch per poll; accepted identity/bootstrap/replay yields pending primary OK, then take OK once. No link-up requirement. |
| Same sequence with cable unplugged | Initialization still OK if MDIO/procedures complete; separate readLinkStatus succeeds with valid DOWN. |
| First identity read MDIO_TIMEOUT/-37 | No invented identity, primary MDIO_TIMEOUT/-37; no mutation means cleanup NOT_REQUIRED; take returns full failure result. |
| Successful ID reads yield wrong OUI/model/revision | Primary DEVICE_MISMATCH with exact reason; successful bus reads remain transport successes; no configuration/reset write before acceptance. |
| start with zero frame or poll budget | OPERATION_TIMEOUT/FRAME_BUDGET or POLL_BUDGET; no token, clock or MDIO call. |
| Accepted start; zero-frame poll slice | No MDIO, slice deadline ignored; spend one phase poll, enforce global deadline; IN_PROGRESS or finite terminal/cleanup transition. |
| Positive primary time below selected reset interval plus batch/restore reserve | OPERATION_TIMEOUT/BOUND_ADMISSION before reset mutation; no pretend reset completion. |
| Last legal completion poll finds reset still asserted | Primary timeout (deadline/frame/poll reason), never OK; retain accepted reset and applicable readiness barrier for cleanup. |
| Expired poll when hardware would now read complete | Deadline classification occurs before read; no I/O or retrospective success. |
| Callback spans a tick boundary | Use fresh conservative completion upper bound plus full interval; early owner poll returns IN_PROGRESS without I/O. |
| Raw clock regresses / upper addition overflows | CLOCK_CONTRACT unless earlier error; latch clockInvalid; no further timed I/O; cleanup FAILED and retained; external disposition/ack/take/end/bind is the finite route. |
| Frozen clock with accepted reset and future wait | No early cleanup read; finite primary then cleanup poll caps expire; retained pending effect requires owner disposition. No wall-clock completion claim. |
| Cancel before mutation; end before take | Cancel OK, pending primary CANCELLED, no cleanup I/O; end BUSY/RESULT_PENDING; exact take then end. |
| Cancel after accepted reset/partial configure | Freeze CANCELLED; CLEANUP only, separate reserved allowance; reset completion/restore do not change primary. |
| Accepted write returns late or MDIO error | Retain mutationPossible/completed evidence and stale trust; primary timing/transport error with separate cleanup receipt; no automatic retry. |
| Cleanup read/write fails | Pending cleanup FAILED and primary unchanged; take BUSY/CLEANUP_REQUIRED; owner disposition/ack permits delivery without claiming restoration. |
| Primary failure, cleanup succeeds | take OK delivers original primary failure plus cleanup SUCCEEDED and historical effects; raw cleanup does not repair managed health. |
| New start while result pending; wrong token take | BUSY/RESULT_PENDING; TOKEN_MISMATCH respectively; no overwritten output/result or callbacks. |
| Consumed token after end/rebind | RESULT_NOT_AVAILABLE when slot empty, or TOKEN_MISMATCH against newer occupied slot; never aliases a newly issued token. |
| Lifetime counter has issued UINT64_MAX | Next start BUSY/TOKEN_EXHAUSTED; object replacement requires explicit old-request invalidation, never wrap. |
| External mutation during wait | Invalidate locally, freeze primary invalidation unless an earlier error exists; retain samples/effects and re-evaluate safe cleanup prerequisites. |
| External mutation during CLEANUP | Preserve primary; spend only the original remaining cleanup allowance on a revalidated safe route, or retain FAILED/DEVICE_INVALIDATED if none exists. No renewed deadline or hidden retry. |
| External mutation after successful RESULT_PENDING | Current trust/initialization becomes invalid; retained primary OK and verified historical receipt stay unchanged. Take delivers history, then explicit initialization/recovery addresses the new context. |
| External mutation after failed cleanup / owner discharge | Preserve FAILED and take BUSY/CLEANUP_REQUIRED, or preserve DISCHARGED and retire-only respectively. No cleanup reopening; owner disposition/take/end remains the finite route. |
| Hard reset requested with untrusted hardware | Initialized owner may startReset; otherwise startRecover(HARD_RESET) uses bootstrap. Trust is not a prerequisite for repair; source gates still apply. |

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
                        const DP8382x::IoBudget& slice) {
    const auto progress = owner.core.pollOperation(owner.token, slice);
    owner.retained = owner.core.getOperationSnapshot(); // Includes a failed primary during cleanup.
    if (owner.retained.state != DP8382x::OperationState::RESULT_PENDING)
        return progress;
    DP8382x::OperationResult result{};
    const auto taken = owner.core.takeResult(owner.token, result);
    if (!taken.ok()) return taken; // Keep token and evidence for explicit disposition.
    owner.delivered = result;
    owner.deliveredValid = true;
    owner.token = DP8382x::INVALID_OPERATION_TOKEN;
    return !result.primaryStatus.ok() ? result.primaryStatus : result.cleanupStatus;
}
```

Construct one OwnerSession for each core instance; the core outlives that session.
Neither the session nor the native owner below can be copied or moved. Only their
snapshot/result values are published by copy. Equal token numbers from two driver
instances cannot redirect these helpers to a different PHY.

The owner schedules another finite call at nextPollAtUs or its own later policy
instant, supplies slices from the original deadlines, and publishes copied
snapshots. It does not spin until a wait expires. After failed cleanup it retains
resources, establishes a valid external disposition, calls acknowledgeQuiescence,
then uses takeResult and end in that order. The executable arithmetic probe uses
synthetic backend bounds; they are not ESP-IDF timing qualifications.

Native callback design uses the **same exact methods**, under the prompt-6
control reservation. An owned `esp_err_t reset(esp_eth_phy_t* phy)` callback
starts `startReset(ResetOptions{ResetKind::PCS}, budget, token)` and drives
`pollOperation(token, slice)` with finite owner scheduling between calls; no B
lock is held while sleeping. A callback-local/native-journal token identifies
only that callback's operation, never an application result. Every loop copies
getOperationSnapshot before deciding whether the result is pending; it does not
stop merely because a frozen primary timeout is visible while cleanup proceeds.
On pending result it calls `takeResult(token, result)`, stores the **entire**
OperationResult in the native journal, then maps primary/cleanup error to esp_err_t.
Failed take preserves token/journal/borrowed adapter lifetime, returns a native
failure and keeps the reservation quarantined for the owner-assisted route.
No output Boolean/cache is assigned until the shared operation proves success.

Native polls use esp_timer_get_time plus proved clock lag and actual backend
bounds; RTOS ticks schedule wakes only. Remaining cleanup time is reserved inside
the enclosing native callback/lifecycle allowance, not a fresh budget after SDK
expiry. A native caller unable to complete safe cleanup retains the cleanup-only
resource path selected in prompt 6; deinit/del cannot discard the token. Native
AN enable/speed/duplex still use public stopped-driver controls. Portable
startConfigure is not a bypass around SDK-owned settings; the native owner shares
its validated plan and private procedure steps through those authorized callbacks,
then commits the full result once. No duplicate public operation slot or recursive
SDK call under the instance lock is introduced.


The native example below uses the real SDK callback signature. Application helper
**declarations** identify the existing adapter/owner responsibilities; they are
not stub implementations or additional portable APIs. `nativeWaitForNextPoll`
has a finite enclosing native allowance and returns an error on its own scheduling
failure. It never holds the instance/controller gate while waiting. The ordinary
context lookup and admission must follow prompt 6's incoming-handle/owned-PHY
rules. These declarations are SDK-only and not part of the portable syntax probe.

```cpp native-example
#include "esp_eth_phy.h"
#include "esp_err.h"
struct NativeResetContext {
    explicit NativeResetContext(DP8382x::DP8382x& phy) noexcept : core(phy) {}
    NativeResetContext(const NativeResetContext&) = delete;
    NativeResetContext& operator=(const NativeResetContext&) = delete;
    NativeResetContext(NativeResetContext&&) = delete;
    NativeResetContext& operator=(NativeResetContext&&) = delete;

    DP8382x::DP8382x& core;
    DP8382x::OperationBudget budget;
    DP8382x::OperationToken token{};
    DP8382x::OperationSnapshot retained{};
    DP8382x::OperationResult delivered{};
    DP8382x::Status nativeStatus{DP8382x::Err::RESULT_NOT_AVAILABLE, 0, "No native call"};
    bool deliveredValid{false};
    bool quarantined{false};
};
NativeResetContext& nativeResetContext(esp_eth_phy_t*) noexcept;
DP8382x::IoBudget nativeSlice(NativeResetContext&) noexcept;
DP8382x::Status nativeWaitForNextPoll(NativeResetContext&) noexcept;
esp_err_t nativeError(const DP8382x::Status&) noexcept;

esp_err_t ownedReset(esp_eth_phy_t* phy) {
    auto& owner = nativeResetContext(phy); // Existing stopped-driver reservation.
    if (owner.token.value != 0) return ESP_ERR_INVALID_STATE;
    DP8382x::OperationToken candidate{};
    const auto admitted = owner.core.startReset(
        {DP8382x::ResetKind::PCS}, owner.budget, candidate);
    owner.nativeStatus = admitted;
    if (!admitted.inProgress()) return nativeError(admitted);
    owner.token = candidate;
    const uint64_t calls = uint64_t{owner.budget.maxPrimaryPolls} +
                           owner.budget.maxCleanupPolls;
    for (uint64_t call = 0; call < calls; ++call) {
        const auto progress = owner.core.pollOperation(owner.token, nativeSlice(owner));
        owner.retained = owner.core.getOperationSnapshot();
        if (owner.retained.state == DP8382x::OperationState::RESULT_PENDING) {
            DP8382x::OperationResult result{};
            const auto taken = owner.core.takeResult(owner.token, result);
            if (!taken.ok()) {
                owner.nativeStatus = taken;
                owner.quarantined = true;  // Keep token, receipt and borrowed resources.
                return nativeError(taken);
            }
            owner.delivered = result;      // Journal the full result before returning.
            owner.deliveredValid = true;
            owner.token = DP8382x::INVALID_OPERATION_TOKEN;
            owner.nativeStatus = !result.primaryStatus.ok() ?
                                 result.primaryStatus : result.cleanupStatus;
            return nativeError(owner.nativeStatus);
        }
        owner.nativeStatus = progress;
        if (!progress.inProgress()) break;
        owner.nativeStatus = nativeWaitForNextPoll(owner);
        if (!owner.nativeStatus.ok()) break;
    }
    (void)owner.core.cancel(owner.token);   // Local only; does not pretend cleanup ran.
    owner.retained = owner.core.getOperationSnapshot();
    owner.quarantined = true;
    if (owner.nativeStatus.ok() || owner.nativeStatus.inProgress())
        owner.nativeStatus = DP8382x::Status::Error(DP8382x::Err::OPERATION_TIMEOUT,
            "Native poll allowance exhausted", int32_t(DP8382x::TimeoutDetail::POLL_BUDGET));
    return nativeError(owner.nativeStatus);
}
```

`nativeError` maps library statuses to SDK codes and leaves full signed details
in the journal. The helper selects the correct primary/cleanup slice from the
retained snapshot and original reservation. An abnormal scheduler/loop exit has
its own native error receipt; later owner handling polls authorized cleanup or
uses explicit external disposition before taking the result. No success return
is possible merely because a callback was admitted or a reset bit was written.
