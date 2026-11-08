# Scope and architecture

Stage 15 implements one initialization phase switch in the existing facade and
transport implementation. [Initialization](initialization.md) documents NONE/PCS,
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

The collector retains every validated consumed sample and possible history loss;
ordinary aggregates publish only complete observations. Safe raw dumps exclude
destructive addresses and show cached evidence. Explicit acquisition and narrow
mixed-register maintenance are stage 13 consumers, later reused by link, IRQ and
diagnostics. Unknown register semantics remain named gates, not guessed permissions.

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

Examples own board resources and actual SDK links. The shared CLI routes every
request through the same application owner. Python sessions retain bounded raw
transcripts and correlated identities/outcomes. Runs use two artifacts: a text
transcript and an atomically finalized JSON record with verdict, case results,
fixture/identity, cleanup, hash/length and truncation. Zero/skipped-only or failed
restoration evidence cannot be PASS. Physical fixture actions need separate scope.

Ordinary consumers use committed generated version metadata and build without
Git/Python/Doxygen or sibling paths. Keep vendor archives outside firmware and
packages. Use [verification](../test/README.md) and COMMON for host, package,
SDK-link, remote CI and physical evidence; first-execution remote CI belongs to 71.
