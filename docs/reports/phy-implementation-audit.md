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

## 2026-11-17 Current-code response

Reviewed every original requirement against DP8382x
`77c544f415edb26a0e23de76591a0163b9b73147`, shipped headers, core/native
sources, CMake/tests, current ledger and owning prompts. The archive originals
were reopened directly with PDF text/field layout, rather than relying only on
the old catalogue. SNLS647I pages 15, 27-29, 35, 40-41, 44, 49, 54, 57-58,
65-69, 73-74, 87-88 and 91 were checked; page 68 (RECR) and 91 (W0C) were also
rendered and visually inspected. The official TI DP83826I download was reopened
on 2026-11-17 and still identifies SNLS647I revision I. Original family tables
were reopened as described in the [correctness response](prompt-correctness-audit.md).

| Original finding / requirement | Current response and concrete evidence | Disposition |
| --- | --- | --- |
| Framework-free usable C++17 API and one chip owner | Portable root CMake builds only core sources; `test/consumer` links the same target with tests OFF. `Config.h`, `DP8382x.h` and `design-decisions.md` assign transport/collector/chip behavior once, and SDK translation is isolated in `src/esp_idf`. A host fake remains software evidence. | IMPLEMENTED |
| Do not blindly reuse base reset/MMD/discovery/polling | Local pinned v6.1 base and `esp_eth.c` were reopened. Current wrapper uses the public PHY interface and shared core, rejects raw mediator hooks, and performs explicit identity discovery through the owner. Base delays/cache/selector defects are not executing DP8382x procedures. The earlier generic-base starting suggestion in the reference guide is superseded by `design-decisions.md` section 2. | IMPLEMENTED current architecture |
| Raw-frame versus IP examples and real frame lifetime | Adapter and raw-RX retention tests exist under `test/esp_idf`; complete ordinary IP/owner/raw-frame applications remain prompts 40-42. Installed reusable teardown is explicitly blocked by `native_rx_quiescence`; current stop/retire does not claim a drained RX lifetime. | IMPLEMENTED adapter subset; examples/teardown OPEN |
| Application controller, board, synchronization, scheduler and recovery ownership | `Config.h`, native owner/configuration and design-decision tables preserve application-owned pins/clocks/reset and one serialization domain. `test/mmd.cpp`, `reset.cpp` and native shared-admission cases cover whole batches and silence. No mutex spans silicon waits/output. | IMPLEMENTED |
| Lifetime, no re-entry/ISR callbacks, cancellation and uncertain effects | Shipped callback/operation contracts, OP-9 debt and native terminal receipts retain accepted/uncertain effects and independent primary/release/restoration results. `test/operations.cpp`, `loopback.cpp`, native lifecycle/notification tests exercise these boundaries. Physical wall-time remains conditional on declared workload/controller bounds. | IMPLEMENTED admitted contract; timing qualification OPEN |
| Identity ID1/model/revision is not suffix detection | Original table 8-22 confirms BASIC 11h/ENHANCED 13h/revision default 1. `src/DP8382x.cpp` identity acquisition and `Config.h` validators admit exact declared DP83826I under the selected revision policy only. `test/identity.cpp` covers partial/wrong/zero/FFFF identity and no vendor writes. | IMPLEMENTED |
| C22 versus indirect extended namespace, selectors and first-failure stop | Original 8.3.11.1 names DEVAD31/3/7 and selector reinitialization. Core constructs reserved-zero REGCR address/data words and complete atomic MMD/RMW batches; `test/mmd.cpp` checks literal frames and every failure boundary. No native Clause45 claim or raw-selector bypass is made. | IMPLEMENTED |
| Generic 10/100/duplex/advertisement and 10BASE-Te VOD | Original 8.3.13 page 40 explicitly requires vendor 030E=4A40. Initialization/replay in `src/DP8382x.cpp` and settings in `DP8382xConfiguration.cpp` preserve it before activation/owned override. `test/operations.cpp`, `reset.cpp`, `negotiation.cpp`, `forced.cpp` cover replay and both modes; local ANAR fault is rejected. | IMPLEMENTED |
| PCS/HARD/RESTART are different commands/domains | Original BMCR/PHYRCR field descriptions and current ledger retain distinctions. Current core polls the proper register, preserves accepted reset effects/shared silence and restores owned persistent state. `test/reset.cpp` and `verify.cpp` include interleaved/direct/alias reset observations and historical result retention. | IMPLEMENTED; hard-reset strap Q5 remains OPEN |
| POR-only straps and physical readiness | Original page 49 explicitly marks Strap1/6 POR-only. Portable code does not own GPIOs; native board-readiness phases use declared clock/time bounds and shared admission. `test/esp_idf/host/native.cpp` has tick-boundary readiness tests. No cold/pin/software reset hardware timing or strap measurement is claimed. | IMPLEMENTED software policy; physical OPEN |
| Current link versus destructive history; stale failures | `DP8382xLink.cpp` uses the one collector/coherent bounded read plan. `test/link.cpp` covers latch-low/current transitions, failure sentinels, pause and forced interpretation. Forced mode uses the selected BMCR-based D4 interpretation; PHYSTS raw evidence does not resolve the enhanced-forced ambiguity by itself. | IMPLEMENTED D4 software selection; Q11 OPEN |
| MISR/counter side effects, partial evidence, optional interrupts | `src/DP8382x.cpp::_retainEvidence`, `DP8382xInterrupts.cpp` and event APIs retain each consumed prefix before aggregate failure. `test/access.cpp`, `events.cpp`, `interrupts.cpp` cover selector attribution/history loss/half-full demand. First-board pin delivery remains unwired/unqualified. | IMPLEMENTED; electrical delivery OPEN |
| RECR contradictory width/saturation | Original table 8-39 still specifies bits15:0 while using FF/7F prose. Current collector exposes raw RECR without inventing calibrated totals/width; `docs/event-counters.md` and ledger retain the gate. A fake cannot settle the physical saturation behavior. | OPEN source/physical semantics, safely bounded |
| Loopback/BIST speed/interface/stop semantics | Original BMCR detail supplies different 10/100 companion selections, despite general 8.3.14.2 prose. `DP8382xDiagnostics.cpp`, `test/loopback.cpp` and `bist.cpp` implement sourced controls with separate start/acquire/stop and persistent restoration debt. BIST reports raw/INCONCLUSIVE where interval/PASS meaning is unsupported. | IMPLEMENTED supported controls; physical outcomes OPEN |
| Cable diagnostics require exact recipe, preconditions and raw/derived separation | `DP8382xCable.cpp`, `test/cable.cpp`, `autorun.cpp` and `docs/cable.md` use the explicitly selected SNLA428 D3 transmit-A recipe, finite preflight/acquire/stop and matching-row interpretation. It is the recorded exception to generic datasheet precedence, not a DP83822 transplant. Q7 accuracy/coherence remains unmeasured. | IMPLEMENTED source-selected software; Q7 OPEN |
| FLD is restricted to MII | Reopened original page 49. `DP8382xQuality.cpp` and `test/quality.cpp` reject nonzero RMII FLD causes while permitting an explicit zero-cause disable. RMII application policy does not promise ESC FLD timing. | IMPLEMENTED; physical response OPEN |
| EEE/power/wake/pins/clock opt-in and interactions | Dedicated current modules share the same slot/collector and replay. `test/eee.cpp`, `power.cpp`, `wol.cpp`, `pins.cpp`, `interface.cpp` exercise ownership and failure paths. D2 EEE tuples are source-implied with Q1 pending; deep-power exit and contradictory pin routes remain gated. Enabling a policy does not prove actual sleep/wake or MAC-clock readiness. | IMPLEMENTED source-admitted subset; named gates OPEN |
| Health/configuration/link/application readiness separate | `DP8382xHealth.h`, snapshots and `test/health.cpp` keep managed transport evidence separate from configuration trust/link. Last failure survives later success; OFFLINE does not gate explicit attempts. No IP/EtherCAT readiness is inferred. | IMPLEMENTED |
| 100/full application policy, generic 10M, EtherCAT master and board power scope | Current README/board/contracts keep 100/full as application policy and IP/EtherCAT scheduling/servo/PoE outside core. SNLA344C/SNLA494/495 remain application-specific ESC references; no ESC address recipe becomes P4 initialization. Prompts 40-42 own executable application examples. | IMPLEMENTED boundary; examples OPEN |
| Exact-family support only after individual qualification | Current `docs/variants.md` and prompts 56-67 retain E/A/825/822 software/physical evidence separately. Shared identity or PDF presence is not support; no additional part is admitted by this audit. | OPEN future variants |
| Host fault matrix, SDK integration and consumers | Real CMake-registered source tests cover address/step failure, atomicity, deadlines, output validity, reset replay, collector loss, diagnostics and native cache/notification lifetimes. Prompt 53-55 still owns the full final regression/coverage matrix. Build-tree consumer exists; installed/exported consumers remain 69-70. | IMPLEMENTED existing suites; final/package gates OPEN |
| Physical identity/fixture/traffic/timing and bounded runners | No fixture was operated. Prompts 48-52 and 72-73 retain exact firmware/SDK/board/chip/mode/address/clocks/peer/cable, sustained traffic, reset/recovery and diagnostic evidence. Host mocks, linked images, ping/BIST and EtherCAT qualification remain separate. | OPEN future physical/runner work |
| MIT, GPL reference boundaries and vendor package exclusion | Current source/license/provenance policies preserve original vendor bytes and forbid GPL copying. No vendor source import is introduced here. Actual emitted package content/license/clean consumption remains 69-70. | IMPLEMENTED source boundary; release packaging OPEN |

This is a complete source/current-plan disposition of the old audit, not a fresh
hardware-support certification. Existing stage reports identify their actual
host/P4 build results; this response claims no new host, SDK, package, CI or
physical execution. Required source gates remain visible in the single feature
matrix and are not closed by successful source inspection.
