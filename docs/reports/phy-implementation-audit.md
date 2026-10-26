# PHY implementation audit

Historical numbering: DP8382x stage numbers in this report refer to the former
34-stage plan. Use the [current 80-prompt index](../prompts/README.md) and
[old-to-new mapping](prompt-workload-audit.md) for execution; its sole first-set
remote-CI checkpoint is now 71. The original findings below retain their dated
scope and are not current execution instructions.

Reviewed on 2026-10-26 against repository base
`35ced9bb9d0b05c1c0b41ecff7cee5e757bcb163`. This is a source and architecture
review for the implementation promptset. No driver was implemented, compiled or
tested on hardware by this review. Local archived TI documents were used;
ESP-IDF's pinned source and versioned documentation were also inspected online.

## Framework boundary

The user's requested deliverable is now a reusable microcontroller library,
including an ESP32-P4 integration. The earlier optional-portability wording in
[architecture](../architecture.md) has been reconciled with that request by
this planning change. The portable public API and chip behavior must build and
be usable without ESP-IDF, FreeRTOS, Arduino, FieldCore, a MAC driver or an IP
stack. A fake host backend is useful verification, but does not establish
hardware portability by itself.

An early design decision must assign one owner to every operation: identification,
register transport, standard control, vendor initialization, reset, polling,
status collection, MAC notification and lifetime. Reuse the supported ESP-IDF
Ethernet framework in its adapter. Avoid two independently active state machines
configuring or polling one PHY. Sharing algorithms or delegating standard
operations through a backend is acceptable; merely exposing ESP-IDF types through
a renamed API does not meet the portable-library objective.

The pinned ESP-IDF base supplies standard reset, power, negotiation, link and MMD
helpers. Its complete MMD read/write helpers combine address selection and data
access without an explicit transaction lock in those functions. Its automatic
address detector checks only ID1 and discards the read result. Its reset helper
writes BMCR, which is not equivalent to every DP83826 reset kind. These are
specific reasons to review and wrap selected base behavior, while preserving
framework integration. [Pinned IEEE 802.3 source](https://raw.githubusercontent.com/espressif/esp-idf/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/phy/esp_eth_phy_802_3.c).

The versioned ESP32-P4 guide separates MAC/PHY installation from attachment to
`esp_netif`, provides an input callback for frames, and describes driver-owned
periodic link checks. A raw-frame application can therefore be a separate example
from ordinary IP networking. Actual frame lifetime, callback scheduling and
teardown need review in the chosen SDK implementation before coding.
[ESP-IDF v6.1 Ethernet guide](https://docs.espressif.com/projects/esp-idf/en/v6.1/esp32p4/api-reference/network/esp_eth.html).

## Ownership requirements for prompts

- Application owns the MDIO controller, callback contexts, bus arbitration,
  scheduler, deadlines, reset GPIO, pins/clocks, board policy and recovery choices.
  The library owns documented chip state and transaction logic only.
- Serialize complete indirect transactions and read-modify-write operations.
  Include SDK base callbacks, raw access, diagnostics and CLI in the ownership
  analysis. A mutex per individual MDIO operation cannot protect REGCR/ADDAR.
  If multiple PHYs share a controller, document controller arbitration separately
  from each PHY's indirect address state. Avoid recursive callback deadlocks.
- Define object/context lifetime, callback reentrancy, ISR restrictions, stop and
  cancellation behavior, partial initialization rollback, and callback draining
  before destruction. Public framework-free status must retain actionable
  transport failure information when adapted to an SDK error.
- Long operations need finite budgets including transport, arbitration and all
  polling. Prefer explicit progress/deadline handling for reset and diagnostics;
  document any bounded synchronous API. Do not add hidden threads, retries,
  time sources, global bus objects or automatic power cycling to the core.
- Operation result, device communication health, link state, configuration
  validity, diagnostic state and data freshness are separate facts. A disconnected
  cable is not necessarily a broken driver. A successful register read does not
  prove IP connectivity or EtherCAT readiness. A failed read must not publish a
  fresh zero-valued or stale link-up snapshot.
- One collector owns read-clear/latching diagnostics. Normal CLI/status rendering
  consumes snapshots rather than stealing events or clearing counters. A raw dump
  must identify and explicitly handle destructive reads. Preserve a sequence or
  timestamp and validity for partial snapshots and counter saturation.

## Exact DP83826I behavior to carry into implementation

The authoritative source for this table is
[SNLS647I](../reference/source/TI-DP83826I-E-SNLS647I.pdf).
The searchable extraction is a navigation aid; inspect PDF tables before encoding
masks. PDF pages 49, 65 and 68 were visually checked during this audit.

| Area | Evidence | Required implementation treatment |
| --- | --- | --- |
| Identity | Sections 8.5.1.3-4, tables 8-21/22 | ID1 is `0x2000`; model `0x11` means BASIC and `0x13` means ENHANCED. Preserve revision separately. These models are not orderable suffixes. Reject missing, foreign or unsupported identity with no vendor writes. Explain the accepted revision policy. |
| Clause 22 and extended access | Sections 8.3.11.1-6, 8.5.1.13-14 | Use direct addresses 0-31; REGCR/ADDAR are `0x0D`/`0x0E`. Vendor DEVAD is 31; EEE uses DEVAD 3 and 7. Reinitialize address selection for each complete operation. Use supported no-increment access and stop at the first failed step. Do not confuse this sequence with native Clause 45 transport. |
| Generic 10/100 operation | Sections 8.3.1, 8.3.13, 8.5.1.1/5 | Support documented speed/duplex capabilities and constrained advertisement. In the 10BASE-Te path, review programming `VOD_CFG3`, extended address `0x030E`, to `0x4A40`, including ordering and replay after relevant reset. Test both advertisement and forced-mode paths. |
| Reset distinctions | Section 8.5.1.1 BMCR[15]; section 8.5.1.30 PHYRCR[15:14] | BMCR reset resets PCS registers and retains vendor registers; PHYRCR hard reset is reset-pin-like; PHYRCR restart retains registers. Give each operation an accurate contract and reapply only owned configuration. Do not poll a reset bitmask as a register address. |
| Physical reset and straps | Section 7.6, pin descriptions and section 8.4.1.1/table 8-7 | Validate reset/clock timing from the chosen device. Strap1 and Strap6 latch on power-on only; pin reset is not a substitute for power-on. Keep GPIO levels and clock sequencing outside the portable core. |
| Current link and events | Sections 8.5.1.2/16 | BMSR has latched/read-clear behavior; PHYSTS link status is not cleared by reading PHYSTS. Define current-vs-latched semantics and a coherent speed/duplex/link sample, including failure or transition during collection. |
| Counters and interrupts | Sections 8.5.1.18-21 and PHYSTS cross-references | MISR and event/counter reads have side effects. Preserve events centrally; define saturation, reset and accumulation semantics. Keep interrupts optional because the first board does not connect PHY interrupt. |
| Loopback and BIST | Sections 8.3.14-15, BMCR and BISCR tables | Sequence and permitted speed/interface vary by mode. Near-end loopbacks generally require disabling negotiation; BMCR's MII-loopback entry specifies different BISCR selection for 10 vs 100. BIST counter acquisition itself has side effects. Restore prior configuration and report failed restoration. |
| Cable diagnostics | Sections 8.3.16, 8.5.1.29 and TDR registers | Respect disconnected/quiet-link-partner preconditions. Bound completion, distinguish failed/incomplete measurements, preserve raw evidence and require cable propagation assumptions for distance. Do not transplant the DP83822 TDR procedure into DP83826. |
| Fast Link Drop | Section 8.4.1.1, page 49 | The enhanced-mode section explicitly restricts FLD to MII. The first board is RMII: do not promise ESC-oriented fast link-drop behavior or copy MII strap recipes into its configuration. |
| Optional features | Sections 8.3.3-8 and 8.3.18 | EEE, low power, wake, LEDs/GPIO and clock outputs need capability and interaction review. Keep them opt-in; changes may affect the clock feeding the MAC or repurpose pins. PHY low-power controls are distinct from excluded board power/PoE management. |

Two visible datasheet ambiguities should become explicit review items, not guessed
implementation details. PHYSTS speed/duplex descriptions annotate ENHANCED forced
mode in a way that needs reconciliation with BMCR control and observed behavior.
RECR table 8-39 describes a 16-bit field but saturation at `0xFF` and a `0x7F`
half-full event. Resolve against current TI clarification and controlled evidence,
or expose raw values with conservative documented limits and withhold unsupported
derived claims. The documentation is authoritative, but contradictory text cannot
be made consistent by tests that simply mirror a chosen interpretation.

## Applications and feature scope

The first application's policy remains autonegotiation advertising 100/full only,
with resolved-mode validation and no silent 10 Mbit/s fallback. The general
library must retain 10/100 operation. The second application is ordinary
microcontroller Ethernet networking; IP/DHCP/DNS/TLS and internet reachability
belong above the MAC/PHY. Board power and PoE are outside this library.

An ESP32-P4 EtherCAT master uses a MAC frame path. It does not become an EtherCAT
Slave Controller because an application note contains an ESC register sequence.
Keep master stack, cycle scheduling, servo semantics and motion operation in the
application. A library example should demonstrate the configuration and raw-frame
boundary without operating actuators or claiming deterministic system timing.

[SNLA344C](../reference/source/TI-DP83826-EtherCAT-SNLA344C.pdf) describes EtherCAT
PHY requirements and specific strap/register configurations; its RMII latency
footnote must be interpreted in its ESC context. The F28P65 examples in
[SNLA494](../reference/source/TI-DP83826-EtherCAT-Frame-Errors-SNLA494.pdf) and
[SNLA495](../reference/source/TI-DP83826-EtherCAT-Link-Down-SNLA495.pdf) use an
EtherCAT slave's ESC counters and management ownership. Their diagnostic method
can inspire fault isolation, but their ESC addresses, MII loops and forced-mode
test writes are not ESP32-P4 initialization defaults.

## Family expansion and acceptance evidence

Implement exact DP83826I first. Track implementation, host verification, SDK build
and physical qualification independently for each exact device/revision/interface.
DP83826E shares documentation but still needs qualification. DP83826AI/AE require
[SDAA067A](../reference/source/TI-DP83826-to-DP83826A-SDAA067A.pdf) and their own
datasheet review: FLD and EMC-related straps/configuration differ. DP83825I and
DP83822 variants need distinct capabilities; archive presence is not support.
Unsupported or ambiguous variants/features should return an explicit failure.

The promptset should require host fault injection at every transaction step,
wrong/missing identities, timeout and clock rollover boundaries, atomicity,
partial configuration, reset replay, read-clear semantics, link flaps, absent
partner, forced/negotiated modes, feature rejection and diagnostic restoration.
Add SDK integration tests for mediator error propagation, event ordering,
single ownership and teardown. Separately build a real framework-free consumer
and a linked ESP32-P4 example using the packaged public API.

Physical evidence must identify firmware commit, dirty state, SDK/toolchain,
board revision, chip/revision/mode, PHY address, clocks, peer and cable. Cover
100/full application policy and separate generic 10 Mbit/s traffic, cold/pin/
software resets, unplug/replug, wrong or absent peer, sustained traffic and
recovery. HIL runners need finite timeouts, interruption-safe cleanup, capability
and firmware identity checks, and structured pass/fail/skipped/blocked outcomes.
Host mocks, SDK builds, ping, PHY BIST and EtherCAT cycle qualification answer
different questions; never collapse them into one hardware-support claim.

Original code remains MIT. The repository's
[source review](../reference/source-code.md) already identifies a suspected TI
reset address/mask error and license boundaries. Keep GPL reference code out of
MIT implementation, preserve applicable notices for any permissible reuse, and
exclude archived vendor payloads from firmware compilation and release packages.
