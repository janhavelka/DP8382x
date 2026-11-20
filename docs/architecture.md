# Scope and architecture

Stages 15-16 implement one initialization phase switch in the existing facade and
transport implementation. [Initialization](initialization.md) documents NONE/PCS/HARD/RESTART,
retained operation evidence and separately admitted restoration. There is no
generic job engine or desired profile inferred from raw maintenance writes.

The framework-independent C++17 DP8382x core begins with exact DP83826I and is
shared by portable applications and a native ESP-IDF adapter. Shipped behavior
is recorded in [implementation progress](implementation-progress.md); design
contracts are not placeholder APIs or claims of physical support.

Read [API contracts](api-contracts.md) for transport, values and OP-1 through OP-17,
[ownership decisions](design-decisions.md) for native execution/lifecycle, and the
[ledger](registers/dp83826i.json) for silicon access/evidence. Those documents own
the rules; this guide does not duplicate operation declarations or state tables.

## Initial component

| Layer | Responsibility |
| --- | --- |
| Portable core | Identity, source-backed access, chip procedures, desired/observed/trust, operations and one destructive-evidence collector |
| Native PHY adapter | Owned esp_eth_phy_t wrapper, same core, SDK cache reconciliation and retained publication/lifecycle record |
| ESP-IDF MAC/framework | DMA, Ethernet frame I/O, public framework integration and actual MDIO frame transport |
| Application/board | Pins/address/clocks/reset, supplies, controller setup, serialization, scheduling, budgets and retry/recovery policy |
| Network/EtherCAT stack | Protocol behavior, application readiness and timing |

The [native owner](esp-idf.md) implements factory/install, explicit board readiness,
typed AN/forced configuration, all reset choices under shared logical silence,
public SDK reconciliation, owner-routed raw/collector access, separate MAC/PHY flow
policy, terminal framework restart, link notifications and bounded stop/fencing. Real-driver host and P4 link tests
cover their own layers. RX task lifetime still prevents reusable destruction of
installed resources; terminal retirement retains them through system restart.
Transient live-query misses use a validated count limit and recover on later
evidence; reset detected by the live read, FAILED_CLOSED board readiness and
controller quarantine require owner stop. Gate-denied polls do not advance the
miss count or bound stale UP in wall time. Event repair debt cannot withhold
the native hardware-stop attempt. Core result delivery remains a separate
obligation, including after a forbidden raw uninstall closes normal admission.

The first board advertises 100 Mbit/s full duplex, but the reusable driver supports
all source-qualified 10/100 modes. Configured policy, resolved link and application
readiness are separate. BMCR readback supplies forced speed/duplex when AN is
disabled; PHYSTS low bits remain raw evidence under the named source uncertainty.

Core has no SDK/RTOS/clock/logging/allocation owner. Borrowed terminal batches
serialize each complete indirect or RMW transaction across all controller users.
The application also serializes all instance calls, even snapshots. Release the
controller lock before hardware waits, SDK callbacks, notifications or output.
Shared controller quarantine and SMI silence are application-owned admission,
distinct from passive transport health and from an operation's cleanup barrier.
The native shared admission enum prevents board preparation and initialization
from replacing each other's reservation. MAC installation has its own logical
silence reservation across reset and CSR-clock setup, with no mutex across waits.
A pre-GPIO failure with known timing and controller release restores prior
admission while preserving the shared POR floor. Partial/unknown board effects
stay closed independently of the adapter lifetime. Applications invalidate
every peer affected by shared RESET_N through its owner. Explicit address discovery uses the core's
raw two-ID batch audit and counters without accepting or retargeting a device.

The collector retains every validated consumed sample and possible history loss;
ordinary aggregates publish only complete observations. Safe raw dumps exclude
destructive addresses and show cached evidence. Explicit acquisition and narrow
mixed-register maintenance are stage 13 consumers; stage 19 link acquisition uses
the same path, extended later by IRQ and diagnostics. Unknown register semantics remain named gates, not guessed permissions.

Stage 14 adds one exact-address two-read identity batch. Partial/rejected raw
words remain diagnostic evidence; only fresh accepted identity supplies
foundational capabilities. Mode changes invalidate older observations. See
[identity admission](identity.md); no factory, scan or suffix inference is used.

## Operations and native integration

One core operation slot holds IDLE, RUNNING or RESULT_PENDING. Starts copy input
without I/O; polls execute at most one admitted batch. A short frame slice defers
work; aggregate deadline/frames and a stall counter keep work finite. Failure or
cancellation freezes its result. cleanupOwed admits only defined restoration and
observation routes; later restoration receives a separate token, result and budget.
takeResult can deliver evidence while the barrier remains. abandonCleanup requires
application-established external disposition and never claims restoration.

The native target is unmodified ESP-IDF v6.1 at
`fff9895c82d744c7237be8847347bdd1b07c6643`. Use an owned esp_eth_phy_t wrapper
without the generic IEEE base. Board reset/readiness precedes install, install
callbacks are local, and typed core initialization follows MAC setup. SDK-called
mutators complete one no-wait terminal batch or reject in favor of the typed route.
Raw mediator hooks always reject; approved owner traffic calls actual MAC frames.

The native public lifecycle route fences timer callbacks before final DOWN/MAC
stop, coordinates STOP delivery/glue and RX ownership, and retains resources on
unresolved failure until reboot. The SDK's remaining RX handoff race and hidden
HAL flush errors are documented limits. Source review, executing pinned-driver
host tests, P4 link builds and physical concurrency evidence remain separate.

## Source, feature and verification coverage

The [feature matrix](feature-coverage.md) owns one row per ledger feature/procedure,
including exact-part support, CLI route, host test and physical case. The ledger
owns per-field encodings/access effects and exact procedure-only actions. Add
later variants only with their individual register/procedure and qualification
evidence; family names never grant compatibility.

Examples own board resources and actual SDK links. The [shared CLI](cli.md)
routes identity, configuration and register commands through the application
owner. Typed profile apply uses the existing core operation and native public
SDK reconciliation. Discovery and dumps advance one address per tick. The CLI holds a complete
terminal record when the console queue fills. The host fixture runs this
processor and the core on simulated MDIO. Planned Python sessions retain bounded raw
transcripts and correlated identities/outcomes. Runs use two artifacts: a text
transcript and an atomically finalized JSON record with verdict, case results,
fixture/identity, cleanup, hash/length and truncation. Zero/skipped-only or failed
restoration evidence cannot be PASS. Physical fixture actions need separate scope.

Ordinary consumers use committed generated version metadata and build without
Git/Python/Doxygen or sibling paths. Keep vendor archives outside firmware and
packages. Use [verification](../test/README.md) and COMMON for host, package,
SDK-link, remote CI and physical evidence; first-execution remote CI belongs to 71.

## Current negotiated configuration responsibility

`src/DP8382xConfiguration.cpp` owns validated negotiated profile composition,
stable verification and finite Clause 28 page service. The existing driver still
owns one token/result slot, aggregate admission, transport accounting and the
single destructive collector. Initialization calls the same profile procedure
for replay. There is no second transaction owner, page registry or job engine.
[Negotiation](negotiation.md) documents the currently implemented scope; SDK
projection and MAC flow control have later owners. Stage 18 extends
the same profile phases for forced modes and coupled routing/extension, including
shared restoration; no separate Force Mode implementation exists.

`src/DP8382xLink.cpp` owns the fixed two-batch link consistency pass and retained
semantic change evidence. It reuses the same transport, collector and invalidation
hooks; cached access has no callbacks. [Link observation](link-observation.md)
keeps current link, consumed history, mode/pause validity and application age
policy distinct. There is no core notification scheduler or second collector.

Stage 20 extends that same collector in `DP8382x.cpp`: `collectEvents` publishes
one six-source aggregate, while per-source evidence survives partial failure.
`DP8382xEvidence.h` is a private arithmetic helper used by the collector and
independent limit tests; it owns no transport or storage. Explicit host-history
discard is bus-silent. [Event/counter semantics](event-counters.md) separate
current words, coalesced history, FCS lower bounds and source-gated raw RECR/RCSR.

Stage 21 projects [passive health](health.md) at the existing terminal callback
boundary. Managed initialization/configuration/link batches count once each;
raw/probe/collector traffic only contributes transport totals. Cached state is
derived from initialization and the failure streak, with no admission latch or
automatic recovery. A private arithmetic helper owns saturating accumulation
and retained timestamps only; it adds no transport or state-machine owner.

Stage 22 adds [VERIFY](reconciliation.md) to that same operation slot: identity
and mandatory-state acquisition feed the existing mode-specific configuration
comparisons. Shared slot setup preserves prior obligations; verification cannot
clear them. Explicit restoration uses the same initialization/replay path.

Stage 23 adds `DP8382xDiagnostics.cpp` for [loopback sessions](loopback.md).
Its explicit phase switch feeds the existing poll/transport/result machinery.
Temporary profile targeting reuses configuration without replacing desired intent;
stop/HARD reuse initialization and one saved-field verification tail. There is
no second collector, reset engine, background dwell or automatic cleanup owner.

Stage 24 extends the same diagnostics source and operation slot for BIST. It
chains the existing loopback/configuration/reset procedures and saved-field exit;
BISCR/BICSR1 retention extends the existing collector. No test framework, second
collector or automatic cleanup engine is added. See [BIST](bist.md).

Stage 25 adds `src/DP8382xCable.cpp` for the cohesive manual cable procedure,
raw control composition and pure interpretation. It extends the existing slot,
terminal batch execution and shared HARD/profile restoration; it adds no worker,
collector or generic job engine. Six bounded result rows retain nondestructive
per-word evidence. See [manual cable capture](cable.md).

Stage 26 extends the same cable phase switch with persistent policy write/readback
and reuses the manual result-reading phases for a fixed autorun observation.
Shared initialization restores owned policy last; no second acquisition engine,
collector, scheduler or autonomous link-recovery loop is introduced.

## Interrupt ownership

`src/DP8382xInterrupts.cpp` adds policy/test phases to the existing operation slot
and one finite deferred service pass. All destructive reads still enter
`_retainEvidence`, including selector attribution and pending half-full counter
requests. No second IRQ collector, event queue, scheduler or generic manager is
introduced. The shared initialization tail replays explicitly owned IRQ policy
after diagnostic/autorun restoration. The [interrupt guide](interrupts.md) defines
pin conflicts that later LED/GPIO owners must preserve.

Stage 28 adds `DP8382xPower.cpp` for [power policy and observations](power.md),
feeding the same slot/transport/collector. One software power intent is shared
by every BMCR writer; live pre-reset PLL preparation and final power replay
extend initialization. There is no second power engine, scheduler or clock owner.

Stage 29 adds `DP8382xEee.cpp` for [EEE](eee.md). Its exact recipe is a once-only
prefix to shared initialization; it never re-enters reset from a replay tail.
The existing collector gains DEVAD-qualified EEE history, with no second event
owner. Ordinary profile/VERIFY tails compare EEE stable controls without reset.


Forced TX LPI extends the existing EEE source with two direct bounded calls.
It uses the shared transaction executor and OP-9 temporary barrier while leaving
the operation slot idle during application dwell; see [LPI ownership](lpi.md).
No additional job engine, collector or MAC scheduling responsibility is added.


[Wake-on-LAN](wake-on-lan.md) uses a cohesive source module within the same
operation-kind/phase dispatch and collector. Its route/control restoration runs
before IRQ attribution and final power policy. There is no second event owner or
wake scheduler; physical MAC/application wake belongs to the application.


[LED/GPIO profiles](led-gpio.md) use `DP8382xPins.cpp`: one fixed row/phase pass
serves bounded direct configuration and the existing restoration operation.
It prepares source/value, clock/mux and final force enable, preserving unowned
fields. Pin dependencies precede WoL/IRQ replay and final power completion.
There is no pin registry, independent collector, scheduler or per-pin object.


Stage 33 adds `DP8382xInterface.cpp` for one fixed interface/clock/compatibility
pass. The direct call and existing cooperative restoration share its rows; no
new operation engine or bus owner exists. It runs before pin/source/power replay
and uses the existing collector for RCSR. Repeater coordination is two explicit
application calls with independent results, not a core device registry.

Stage 34 adds one explicit quality/TX/link-loss pass in the existing operation
slot. It shares controller transactions, destructive collector and initialization/
verification tails; no scheduler or second diagnostic reader is introduced.
See [quality controls](quality.md) for independent raw MSE and profile evidence.

Stage 35 keeps persistent analog sequencing in `DP8382xAnalog.cpp` and finite
electrical sessions in `DP8382xElectrical.cpp`. Both extend the existing slot,
collector and shared reset/configuration/replay paths. No controller lock spans
a wait; no independent job framework or raw reserved-write route is introduced.

Prompt 39 closes the native [entry/notification/timing table](esp-idf.md).
Acquired core link snapshots remain independent of accepted SDK field publication;
terminal receipts prevent callback gate contention from losing effects or stranding
admission. No base polling or second chip procedure is introduced. Finite native
wall time requires the explicit workload qualification described by native_wall_time;
installed reusable destruction remains gated separately by native_rx_quiescence.
The [2026-11-18 correction review](reports/implementation/37-39-native-fixes-2026-11-18.md)
adds retryable failed retirement, independent failed-start cleanup, typed native
error receipts, administrative DOWN after lost observation, resolved pause
projection and validated callback/period bounds. This changes no stage 40 scope.
