# DP8382x value and direct-call contracts

Prompt 7 freezes this design on 2026-10-29, using the
[prompt-6 ownership decisions](design-decisions.md). These declarations are
documentation, **not shipped headers or implemented methods**. Introduce each
declaration with its working behavior/tests at its owning stage. Prompt 8 adds
cooperative operation signatures, tokens, cancellation and retained cleanup;
it must preserve the values and direct-call semantics here.

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
    ENUM_VALUE = 1, ADDRESS_RANGE, VALUE_WIDTH, ACCESS_POLICY
};
/// @brief Local detail namespace for INVALID_CONFIG.
enum class ConfigDetail : int32_t {
    TRANSPORT = 1, HEALTH_THRESHOLD, LINK_COMBINATION, SOURCE_GATE,
    BOARD_PREREQUISITE, CLOCK_CONTRACT, ENUM_VALUE, ADDRESS_RANGE, INTERFACE_REQUIRED
};
/// @brief Local detail namespace for BUSY.
enum class BusyDetail : int32_t {
    OPERATION_ACTIVE = 1, RESULT_PENDING, OWNER_ADMISSION,
    CONTROLLER_QUARANTINED, CLEANUP_REQUIRED, OBSERVATION_CHANGED
};
/// @brief Local detail namespace for DEVICE_NOT_FOUND / DEVICE_MISMATCH.
enum class IdentityDetail : int32_t {
    ZERO_ID = 1, ONES_ID, WRONG_OUI, WRONG_MODEL, REVISION_NOT_ACCEPTED,
    MODE_MISMATCH
};
/// @brief Local detail namespace for OPERATION_TIMEOUT.
enum class TimeoutDetail : int32_t { DEADLINE = 1, FRAME_BUDGET, BOUND_ADMISSION };
/// @brief Local detail namespace for source-proved absent silicon behavior.
enum class UnsupportedDetail : int32_t { INTERFACE = 1, LINK_MODE, FEATURE };
}
```

These local detail values are explicitly cast to `int32_t`; the error code
determines the namespace. Zero means no additional detail. `OK` and
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
    uint64_t completedAtUs{0};               ///< Same clock domain; valid only if timeValid.
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
and after callbacks and before each later batch. A backward sample latches this
binding's clock invalid until successful end/rebind after owner repair; a later
larger sample does not silently restore it. All timed I/O then rejects with
`INVALID_CONFIG/CLOCK_CONTRACT` before batch callbacks. Local snapshots, result
delivery and bus-silent lifecycle remain possible. Retain effects and mark
time-dependent snapshots stale; suppress new ordinary outputs/timestamps from
the affected call. Historical timestamps remain historical, never a valid age
comparison against the broken current timeline. A post-callback regression
preserves an earlier primary/release transport failure; only otherwise return
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
Config with null required callbacks or either required bound zero is
`INVALID_CONFIG/TRANSPORT`. Owners must provide
truthful conservative platform allowances, including scheduler/controller/clock
quantization, not just nominal loop delays. Core cannot prove or preempt a
backend's timing. The pinned SDK's nominal 1000 us/frame is not a proved bound;
its adapter must establish/report the missing platform allowance before a
bounded native-support claim. No guessed default is supplied.

Validate input and admission first. With an otherwise legal call, deadline
expired returns `OPERATION_TIMEOUT/DEADLINE`; zero/insufficient frame allowance
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
below. Boundary `completion >= deadline` is late, even with successful hardware.

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
the frame. The backend must retain successful destructive-read words even when
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
quarantine admission; OPERATION_TIMEOUT covers exhausted aggregate budget, while
MDIO_TIMEOUT means the controller/lock attempt timed out. `IN_PROGRESS`, unknown
Err, other codes, null message or inconsistent frame/effect reports normalize
to `MDIO_ERROR`. Preserve original code/detail and all independently valid
prefix evidence; never trust contradictory values or manufacture completion.
Release codes are only OK/MDIO_ERROR/MDIO_TIMEOUT; other values are violations.
OK with nonzero detail is a STATUS_CODE violation. Primary OK plus release OK
but controllerReusable=false returns MDIO_ERROR/detail 0 with static message
"Controller state uncertain" and CONTROLLER_STATE. Retain raw primary/release
reports independently.

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
| Pure validation, BUSY/silence/quarantine rejection, expired admission with no lock/controller attempt | No managed success/failure. |
| Controller lock acquisition timeout or attempted managed batch with MDIO failure, malformed result or late completion | One managed failure; completed prefix/frames still recorded. |
| Otherwise successful managed batch cannot fit the post-callback publication reserve | One managed timing-contract failure with OPERATION_TIMEOUT/BOUND_ADMISSION, even if the current timestamp is still before the deadline. No second failure count; no fabricated late=true. |
| Clock regression before a batch / after an otherwise successful managed batch | No count before callback; after a callback, one managed timing-contract failure with INVALID_CONFIG/CLOCK_CONTRACT and no new valid timestamp. This is not evidence of PHY failure. An existing primary MDIO failure still counts only once. |
| Successful bus reads yielding wrong identity, CONFIG_MISMATCH, cable-down or policy mismatch | Managed transport success; semantic failure/link state remains separate and visible. |

Because lock acquisition can fail before a frame, the backend must distinguish
MDIO_TIMEOUT from no-attempt admission BUSY/OPERATION_TIMEOUT. Callback invocation
itself increases `totalBatches`; frame totals count reported attempted/completed
frames only. On malformed reports, count only the independently valid prefix
and mark the violation; counts must not be invented from success status alone.

All counters saturate at UINT32_MAX. A managed failure increments totalFailures
and consecutiveFailures, records normalized status and completion-time
`lastErrorMs` if valid. A managed success increments totalSuccess, resets the
streak to zero and updates lastOk/time-valid. It **preserves lastError and its
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
    /// @brief Record loss of trust locally; never reset/recover hardware or clear history.
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

`end` is idempotent OK when already unbound, otherwise BUSY for active work,
pending result or owed cleanup. Successful end clears borrowed binding/local
state only after the owner has delivered or acknowledged retained evidence;
it never frees the application controller or makes a quarantined bus reusable.
Prompt 8 owns exact acknowledgement/token routes; none is fabricated here.
`invalidateDeviceState` rejects invalid reasons unchanged, returns NOT_BOUND
when unbound, otherwise works even during an active operation: mark trust DIRTY,
identity/link/observed settings stale, capabilities invalid and initialized false;
retain desired intent, counters, samples and result/effect evidence. It signals
the active operation to stop normal progression and preserve cleanup obligations
under prompt 8. No invalidation reason repairs SDK caches or controller state.

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
| Prior caller snapshot says UP; BMSR first `0x7809`, second `0x7809`, remaining required reads establish down | OK; replace caller snapshot with explicit valid DOWN and invalid resolution/pause. | Both low samples retained, current link-down timestamp; no MDIO health failure. |
| First BMSR `0x7819` (latched low and fault), second read MDIO_TIMEOUT detail -37 | Caller snapshot unchanged; MDIO_TIMEOUT/-37. | Successful first full word retained, cached aggregate stale; failed destructive second read marks possible lost history. No synthetic second word. |
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
| Detected backward clock | INVALID_CONFIG/CLOCK_CONTRACT unless a primary/release transport failure already takes precedence | Clock-invalid latch blocks timed I/O until owner repair/end/rebind; retain both facts and effects, publish no new valid output/time. |
| No remaining aggregate time/work, bound cannot fit, or successful callback returned late | OPERATION_TIMEOUT with deadline/frame/bound detail | No partial admission; late writes/reads can have effects despite unchanged ordinary output. |
| Successful in-budget stable-bit readback disagrees | CONFIG_MISMATCH/0 | Trust DIRTY; retain expected/observed/mask/space in typed result; transport health can be READY. |
| Link/AN changes during aggregate observation | BUSY/OBSERVATION_CHANGED | Cached history stale, ordinary output unchanged; collector retains completed samples. |
| Active operation/pending result/owed cleanup | BUSY with distinct reason | No overwritten token/result; exact operation mechanics belong to prompt 8. |
| Release failed after primary failure | Preserve primary error; separate release status | Shared controller quarantine, retained evidence; no retry until owner resolution. |
| Raw write accepted or fails after possible acceptance | Actual primary status, including OPERATION_TIMEOUT if otherwise successful but late | Invalidate affected trust/link; desired settings unchanged; no automatic rollback. |
| Cached value is stale/unknown | Return copied validity/stale metadata, not a fake transport error | No I/O; caller decides age/admission policy. |

## Plain-owner and native design examples

These examples use the frozen declarations; `owner` operations describe
application responsibilities, not extra DP APIs. Neither example is a linked
consumer or permits physical fixture operations.

```cpp example
// Application supplies terminal runBatch and monotonic readClock functions.
void bindPortable(DP8382x::DP8382x& phy, void* context,
                  DP8382x::ExecuteBatch runBatch, DP8382x::NowUs readClock,
                  const DP8382x::TransportBounds& provedBounds) {
    DP8382x::Config config{};
    config.variant = DP8382x::Variant::DP83826I;
    config.phyAddress = 0;
    config.interface = DP8382x::MacInterface::RMII;
    config.requiredMode = DP8382x::OperatingMode::ENHANCED;
    config.transport = {context, runBatch, readClock, provedBounds};
    const auto status = phy.bind(config); // Under owner serialization; no callbacks.
    if (!status.ok()) return;              // Existing binding survives rejection.
    // Initialization is explicit: prompt 8 supplies its cooperative signature.
}

DP8382x::Status observePortable(DP8382x::DP8382x& phy,
                              const DP8382x::IoBudget& ownerBudget,
                              DP8382x::LinkSnapshot& published) {
    DP8382x::LinkSnapshot candidate = published;
    const auto status = phy.readLinkStatus(candidate, ownerBudget);
    if (!status.ok()) return status; // published unchanged; owner can copy diagnostics.
    published = candidate;          // Includes a valid DOWN result.
    return status;
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
own exact operation result into the native journal; prompt 8 defines the token
mechanics. The pinned SDK lifecycle correction remains mandatory.

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
