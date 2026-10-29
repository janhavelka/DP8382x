# Scope and architecture

Prompt 6 selected this architecture on 2026-10-29. The complete
[ownership decisions and call-chain matrix](design-decisions.md) are the design
contract for portable and native operation. No driver API is implemented;
prompts 7–8 freeze exact signatures and value layouts. The
[source comparison](library-comparison.md) and
[prompt-6 audit](reports/implementation/6-framework-ownership.md) record precedents.
The [core silicon behavior](device-behavior.md) and [machine-readable ledger](registers/dp83826i.json)
now close primary-part source research through prompt 5, with explicit unresolved
implementation gates. Destructive status
and counter acquisition belongs to one collector; completed samples survive later
failures and cleanup. Exact recipe actions do not grant ordinary register access.
Requested power policy, observed power state and trust remain separate.
Source-gated EEE/opaque accesses reject before I/O; finite stimulus cleanup
stops emission before persistent configuration is restored.
In particular, configured forced
mode is distinct from resolved link evidence while the PHYSTS conflict remains
open; the selected native projection cannot manufacture resolved forced-mode
evidence. Prompts 18, 19 and 39 retain that gate.
The [implementation promptset](prompts/README.md) establishes exact contracts
before coding and requires a fresh relevant sibling audit before every stage.

## Initial component

Start with a framework-independent C++17 DP83826I library and an adapter through
ESP-IDF's Ethernet PHY interface. Ordinary C++ MCU owners must be able to use the
real chip implementation through borrowed MDIO transport without SDK headers,
RTOS, Arduino or FieldCore dependencies. The ESP32-P4 MAC driver owns frame
transmission/reception and MDIO controller transport; the PHY library configures
and observes the external transceiver. Reuse Espressif's IEEE 802.3 PHY base for
common behavior where appropriate, with one executing owner per operation.
See the [pinned official source references](reference/source-code.md).

| Layer | Responsibility |
| --- | --- |
| Portable DP8382x core | Identity checks, supported chip configuration, PHY status, register access, reset/reinitialization primitives and diagnostics through application-owned transport |
| ESP-IDF PHY adapter/base | Owned callback wrappers, qualified local base storage, shared core execution, SDK cache reconciliation and MAC notification journal |
| ESP-IDF MAC driver | ESP32-P4 MAC, DMA, frame I/O and management bus transport |
| Board firmware | Pins, PHY address, clock wiring, power sequencing, link policy, retries and recovery decisions |
| Application/network stack | IP networking or EtherCAT master operation, scheduling and application protocol behavior |

Use one core chip implementation in both modes. The adapter reuses the pinned
base's local object/mediator/address storage under guards, overriding all I/O,
notification and lifetime callbacks. The generic base's MMD, reset, discovery,
power and link procedures do not satisfy the selected contract. Framework reuse
means retaining the supported MAC/PHY interface and mediator, not creating a
second generic PHY stack. Actual standalone and SDK links remain future evidence.

Application code owns controller setup, callback lifetime, transaction locks,
monotonic time, scheduling and recovery decisions. Complete indirect MDIO and
read/modify/write transactions must be serialized across the core, base driver,
ioctl, polling and CLI. Select one borrowed synchronous terminal batch callback:
fixed C22 steps with preceding-read masks for RMW, one controller-wide lock
through the complete batch, retained partial samples/effects and release on
every exit. No second transaction-scope API is planned. Give read-clear
diagnostics one collector and expose passive cached observations to consumers.
External power-up/hardware-reset readiness closes controller-domain admission for every PHY
before transition, through the actual-release/clock-qualified SMI silence window.
The mutex is released during that wait; logical admission still prevents frames.

The application serializes access to each driver instance, including cached
snapshots, and publishes copies to other tasks. Lock order is instance gate,
controller lock, then controller internals. Native lifecycle/control reserves
the operation under the gate, releases it before entering the SDK, and admits
only expected synchronous callbacks that reacquire the gate. Notifications and
callback drain occur without these locks. The framework timer alone polls native
running-link state; application and IRQ requests use the same collector.

Resolve frame-hook context from the incoming handle using public PHY/MAC getters,
including before installation publishes its output handle. Reject external raw
mediator/ioctl frames; approved typed owner requests use complete core batches
and the actual MAC's frame methods directly. No recursive mediator transport,
global singleton or multi-call raw selector authorization is used.

Selected ESP-IDF source baseline is v6.1 at
`fff9895c82d744c7237be8847347bdd1b07c6643`. Prompt 37 must supply and pin
the [specified lifecycle correction](design-decisions.md#6-lifecycle-correction-selected-for-native-support)
before native support: checked install/unwind, finite timer quiescence, retained
cleanup-only handles, retryable step-aware teardown and propagated MAC/HAL
cleanup errors. Unmodified v6.1 cannot satisfy these failure paths. Retain
resources until callbacks are quiescent and cleanup outcomes are known.

The core's local `bind` validates borrowed MDIO callbacks and settings before
replacing a binding, without touching the PHY. `end` is local teardown with
explicit pending-result and cleanup rules. Hardware initialization, reset,
configuration and recovery use separately named bounded operations. A successful
bind or controller transfer proves neither PHY identity nor a usable link.
Keep one retained token/result slot; terminal evidence survives until the owner
consumes or explicitly acknowledges it. Exact signatures and teardown behavior
belong to prompts 7-8.

Distinguish operation admission across polls from the short controller lock for
one indivisible transaction. Never keep that lock through hardware wait intervals
or console output. Preserve each consumed event sample on partial acquisition
failure, while marking incomplete aggregate observations stale/invalid.

SDK state also needs one owner: direct core/raw changes to AN enable can stale
IDF's cached setting, and raw ioctls bypass PHY methods. Use public stopped-driver
AN/speed/duplex controls for mirrored settings, shared procedures for other PHY
fields, and hardware readback plus `G_AUTONEGO` agreement before restart after
invalidation. Raw writes do not become desired settings. There is no SDK full-
advertisement ioctl. Avoid the pinned flow-control ioctl's argument-width defect;
the owner calls correctly typed MAC policy and PHY advertisement routes.
Full portable pause resolution remains richer than the pinned SDK's Boolean MAC
notification. Base helpers are reused only where their semantics fit; shared
portable procedures can implement overridden methods without a second driver.

Core Status will use the peer `Err`/`Status` value shape with static message
storage and full signed backend detail, but MDIO-specific error meanings. Four
passive health states describe managed transport evidence only. Desired settings,
observed stable bits, configuration trust, link resolution and application
readiness remain separate. A failed or late write leaves its possible effect
visible. Primary failure and finite cleanup outcomes stay separate; successful
cleanup cannot erase the primary failure. The source comparison and
[conventions](prompts/CONVENTIONS.md) record the selected names and limits.

Public declarations will carry Doxygen when implemented: units, validity,
legal state, borrowed lifetime, owner synchronization, I/O and destructive-read
effects, output validity, errors, total bounds and cleanup. Private procedures
will explain ordering and cite exact TI sections for non-obvious chip behavior.
Prompt 9 introduces checks with the first real public headers; this planning
decision creates no placeholder API.

## Planned consumer and evidence boundaries

One example-only CLI in `examples/common/` will serve a host fixture and the
native applications. Attaching it to an application performs local setup only;
startup, owner admission, driver polling and console transport remain with that
application. Strict whole-token parsing, bounded input/output and retained
correlated terminal records need executing dispatcher and parser tests. A
separate bounded Python runner will require a verified firmware/session identity
and preserve raw records, case outcomes and cleanup evidence in a finalized
artifact bundle. Neither CLI nor runner belongs in the portable core.

The plain C++ MCU consumer will link the core without SDK headers. An ordinary
ESP32-P4 Ethernet example will link the native PHY adapter with the supported
MAC framework. A separate application-owned EtherCAT boundary will demonstrate
maintenance admission and frame handoff without claiming an EtherCAT stack.
Each application retains handles on uncertain cleanup so it can report or retry
their release. These remain planned tests and examples, not completed builds.

Version metadata will derive from `library.json` when introduced. Source
documentation checks will inspect actual public declarations, warnings and
generated pages; Doxygen's `EXTRACT_ALL` alone does not establish comment
coverage. Exported consumers will build without Git, Python, Doxygen or sibling
paths. The [comparison](library-comparison.md) maps the future files and current
tool availability.

## Intended functionality

- Detect the supported device and revision, accounting for documented operating
  modes and identity behavior; reject unknown devices deliberately.
- Standard Clause 22 and indirect extended/MMD register access, with explicit
  errors and serialization across the whole multistep transaction.
- Autonegotiation, advertisement masks and resolved speed/duplex. Expose 10 and
  100 Mbit/s choices, and supported forced modes, without hard-coding the first
  board's speed policy.
- Hardware/software reset primitives, bounded completion checks, restore and
  readback of owned configuration, and clear treatment of reset-loaded straps.
- Link/status snapshots and register dumps suitable for board bring-up.
- Incremental, explicitly supported diagnostics such as loopback, BIST, cable
  diagnostics and interrupt handling. Feature presence must be verified per part.
- Optional low-power, wake and clock/LED controls only after documenting their
  interactions and exact variant support.

These are implementation stages, not permission to omit documented capabilities.
The endpoint is the complete publicly documented software-accessible feature set
per supported part. "Optional" means opt-in operation, not optional library
coverage. Include all legal controls/settings and documented programming
sequences, even when the starting board cannot exercise them. Account for every
non-reserved field and hardware-only property using the
[full-coverage contract](prompts/FULL_COVERAGE.md). Generic raw register writes
alone do not complete a stateful feature. Unresolved essential evidence remains
an open implementation gate, not an exclusion justified by the initial use case.

All waits and transport calls need aggregate bounds including acquisition,
frames, release and result publication. The pinned MAC's nominal 1000 us polling
allowance per frame is not a proved wall-clock maximum; it accepts no caller
deadline. No software preemption is claimed. A timed-out command may remain busy
and blocks every participant on that controller until owner recovery proves
safe reuse. Actual backend allowances and physical timing remain qualification
work. Application code owns scheduling, recovery and power cycles.

The DP83826I/E datasheet documents a 10BASE-Te amplitude configuration in
section 8.3.13: set `VOD_CFG3` (extended address `0x030E`) to `0x4A40`.
Its applicability, reset behavior and initialization sequence
must be checked when implementing 10 Mbit/s support; a generic PHY status/read
implementation alone does not prove correct 10 Mbit/s operation. See the
[primary datasheet](reference/source/TI-DP83826I-E-SNLS647I.pdf).

## Boundaries and qualification

The repository does not supply an EtherCAT master stack, servo-control protocol,
IP stack, power delivery/PoE, PHY PCB design or universal DP8382x compatibility. Reference material
for another variant is not implemented support.

The planned examples cover ordinary Ethernet/IP, an application-owned EtherCAT
integration boundary, and an ordinary portable owner. CLI and Python HIL tooling
follow sibling conventions with PHY-specific commands and evidence. Core health,
link state, configuration trust and application readiness remain separate.
SNLS647I section 8.4.1.1 limits Fast Link Drop to MII; do not promise it for the
initial RMII board or apply ESC-specific app-note recipes to a P4 master.

Implementation establishes native register-sequence tests and linked ESP-IDF
examples. Hardware qualification then covers the exact board at
100/full and a separate suitable test setup for generic 10 Mbit/s operation,
including reset, cable disconnect/reconnect and recovery. Do not infer physical
interoperability or signal compliance from mocked register tests.
