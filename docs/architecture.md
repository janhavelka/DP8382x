# Scope and architecture

These are implementation intentions, updated on 2026-10-28 after the live
[API, state, ownership and tooling comparison](library-comparison.md). No driver API has
been implemented or frozen.
The [core silicon behavior](device-behavior.md) and [machine-readable ledger](registers/dp83826i.json)
now establish the prompt-3 research boundary. In particular, configured forced
mode is distinct from resolved link evidence while the PHYSTS conflict remains
open; prompts 6, 18, 19 and 39 must preserve this in portable/SDK ownership.
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
| ESP-IDF PHY adapter/base | PHY lifecycle integration, mediator calls, standard IEEE 802.3 behavior and MAC link notifications |
| ESP-IDF MAC driver | ESP32-P4 MAC, DMA, frame I/O and management bus transport |
| Board firmware | Pins, PHY address, clock wiring, power sequencing, link policy, retries and recovery decisions |
| Application/network stack | IP networking or EtherCAT master operation, scheduling and application protocol behavior |

Portability is a requirement of the requested general library. Decide the exact
boundary before coding using an operation-by-operation ownership table for the
portable path and IDF adapter. Standard behavior can delegate to the IDF base
in IDF mode; the portable core and base must not independently configure, reset
or poll the same PHY. Share chip-specific semantics, and do not recreate the
MAC/stack or force a FieldCore dependency. Prove the portable boundary with a
standalone consumer and the adapter with actual linked SDK firmware.

Application code owns controller setup, callback lifetime, transaction locks,
monotonic time, scheduling and recovery decisions. Complete indirect MDIO and
read/modify/write transactions must be serialized across the core, base driver,
ioctl, polling and CLI. Choose the transaction mechanism before implementation;
a lock on each MDIO frame is not sufficient. Give read-clear diagnostics a single
collector and expose passive cached observations to other consumers.

The application serializes access to each driver instance, including cached
snapshots, and publishes copies to other tasks. SDK installation/start/stop and
deletion have explicit partial-failure and callback-lifetime contracts. Resolve
hook context through supported instance APIs, and retain resource ownership
until callbacks are quiescent and cleanup outcomes are known.

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
IDF's cached setting, and raw ioctls bypass PHY methods. Use supported control
paths for SDK-mirrored state and the documented owner/register hooks for access.
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

All waits and transport calls must have a defined bound. Application code owns
synchronization, scheduling, recovery policy and any hardware power cycle.
An indirect register operation must not interleave with another MDIO owner.

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
