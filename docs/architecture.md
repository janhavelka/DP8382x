# Scope and architecture

These are implementation intentions, recorded on 2026-10-24. No driver API has
been implemented or frozen.

## Initial component

Start with a DP83826I PHY component integrated through ESP-IDF's Ethernet PHY
interface. The ESP32-P4 MAC driver owns frame transmission/reception and MDIO
transport; the PHY component configures and observes the external transceiver.
Use Espressif's IEEE 802.3 PHY base for common behavior where appropriate.
See the [pinned official source references](reference/source-code.md).

| Layer | Responsibility |
| --- | --- |
| DP8382x component | Identity checks, supported chip configuration, PHY status, register access, reset/reinitialization primitives and diagnostics |
| ESP-IDF PHY adapter/base | PHY lifecycle integration, mediator calls, standard IEEE 802.3 behavior and MAC link notifications |
| ESP-IDF MAC driver | ESP32-P4 MAC, DMA, frame I/O and management bus transport |
| Board firmware | Pins, PHY address, clock wiring, power sequencing, link policy, retries and recovery decisions |
| Application/network stack | IP networking or EtherCAT master operation, scheduling and application protocol behavior |

A portable register layer can be added if it serves another host or makes
meaningful testing easier. Decide that boundary before coding; do not recreate
the existing Ethernet stack or force a FieldCore dependency.

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
IP stack, PHY PCB design or universal DP8382x compatibility. Reference material
for another variant is not implemented support.

First implementation should establish native register-sequence tests and one
linked ESP-IDF example. Hardware qualification then covers the exact board at
100/full and a separate suitable test setup for generic 10 Mbit/s operation,
including reset, cable disconnect/reconnect and recovery. Do not infer physical
interoperability or signal compliance from mocked register tests.
