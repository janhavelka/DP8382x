# Pinned ESP-IDF register, timing and ownership evidence

Reviewed on **2026-10-26**, ESP-IDF **v6.1**, commit
`fff9895c82d744c7237be8847347bdd1b07c6643`. These are observed SDK behaviors,
not TI silicon limits or this library's implemented support. Reopen the exact
source and apply [EVIDENCE](../prompts/EVIDENCE.md) before reuse. All inspected
files below declare Apache-2.0. Git blob IDs identify repository objects; they
are not SHA-256 hashes of archived payloads. No SDK code was imported here.

## Source identities

| Key | Pinned source | Git blob |
| --- | --- | --- |
| PHY-H | [esp_eth_phy.h](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/esp_eth_phy.h) | `a5a0d14e85dca35be5cb629054db9e4167d43c27` |
| BASE | [esp_eth_phy_802_3.c](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/phy/esp_eth_phy_802_3.c) | `7d5d407d029c63494dffaaa3d82f8701b6b73790` |
| BASE-H | [esp_eth_phy_802_3.h](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/esp_eth_phy_802_3.h) | `57a2ff901eebbe4d19b53cb675ad8f48c5906b93` |
| REGS | [eth_phy_802_3_regs.h](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/eth_phy_802_3_regs.h) | `e9091f63a39ac1861bb043b5f25373cbb0d749f0` |
| MAC | [esp_eth_mac_esp.c](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/mac/esp_eth_mac_esp.c) | `6448fcd6de4669ddf1e836c992ca8a1accc9dd96` |
| DRIVER | [esp_eth.c](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/esp_eth.c) | `76dab829d2099a574a07aeceeaa82491950fe07c` |
| DRIVER-H | [esp_eth_driver.h](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/esp_eth_driver.h) | `405138e4e63dd93a21896bb970b8ad19d68127f4` |

Line references below use these pinned files. The findings are static source
review, not a build, executing test or physical measurement. TI requirements
come from [SNLS647I](source/TI-DP83826I-E-SNLS647I.pdf), especially 7.6,
8.3.11 and the detailed register tables. Exact-family differences remain in the
[family catalogue](library-family.md).

## Actual defaults and waits

| Source | Observed value or behavior | Required interpretation |
| --- | --- | --- |
| PHY-H 276-296 | Default address `-1` enables detection; reset timeout `100 ms`; AN timeout `4000 ms`; reset GPIO `5`; assertion/post-reset fields `0` select defaults | Application/SDK choices, not PHY requirements. Select the board's actual address/GPIO explicitly; `-1` for reset GPIO means no physical reset |
| BASE 20-21, 714-722 | Default hardware reset assertion `100 us`; a nonpositive post-reset setting becomes `ESP_ETH_NO_POST_HW_RESET_DELAY=-1` | Generic base supplies no default post-reset wait. Its default assertion exceeds DP83826's `25 us` minimum without debounce caps, but does not establish post-reset readiness |
| BASE 485-506 | Assertion below `10000 us` uses busy delay; larger values use integer `us/1000` and RTOS ticks. Positive post-reset milliseconds use RTOS delay | Rounding/scheduling must satisfy the chosen minimum and aggregate bound; configure actual board timing rather than trusting a zero/default field |
| SNLS647I 7.6 p15; 8.3.11 | Hardware reset pulse minimum `25 us`; SMI-ready maximum `2 ms` after reset release and `50 ms` after power-up; prose requires bus inactivity at least those readiness intervals | Required wire/readiness constraints. Source software-reset completion separately; do not apply every GPIO/POR timing to BMCR by analogy |
| SNLS647I 7.6 p17 footnote 2 | If external clock is delayed, reset follows POR completion and at least `100 us` of clock stability | Board supplies clock/reset readiness; elapsed time since `bind()` alone cannot prove it |
| BASE 133-149 | PCS reset writes BMCR with reset bit set; polls in `10 ms` sleeps for `reset_timeout_ms/10` iterations | SDK policy. Below `10 ms`, the command is still written but the poll loop never executes; this cannot implement zero-budget/no-I/O admission |
| BASE 155-212 | AN restart uses `100 ms` sleeps for `autonego_timeout_ms/100` iterations | Expiry is not returned as a timeout error; code can return success with AN incomplete. Success means the control call completed, not resolved link evidence |
| BASE 307-334 | Power-up polls BMCR power-down clear using the same `10 ms`/reset-timeout loop | Bit readback is not proof of PHY analog or clock settling; exact chip power modes need their own procedure |
| DRIVER-H 190-200 | Default link-check period `2000 ms` | Framework scheduling policy, not an EtherCAT link-loss budget or silicon Fast Link Drop time |
| MAC 41, 211-253 | Each PHY frame busy-polls in `100 us` steps, up to nominal accumulated `1000 us`; busy before launch returns invalid-state, still busy after loop returns timeout | Finite iteration count but not a measured wall-clock deadline. Include scheduling/instruction overhead; the callback accepts no caller deadline |
| MAC 91, 185-193 versus 211-253 | A `100 ms` multi-register MAC mutex exists, but the shown PHY read/write methods do not acquire it | Do not count this mutex as MDIO/MMD serialization. Provide one real owner across all participants |

One indirect read or write takes four C22 frames in this base. Four times the
nominal `1000 us` polling allowance is only the loop contribution, not a claimed
`4 ms` wall-clock bound. Derive the actual allowance from owner acquisition,
all frame callbacks, required bus gaps, CPU/RTOS delays and release. The
DP83826 suppressed-preamble gap is at least `500 ns` plus the stated MDC/MDIO
idle condition; a polling interval alone does not prove every wire gap.
Do not hold the transaction lock during PHY settling or later cooperative polls.
Exercise coarse RTOS ticks explicitly: if `pdMS_TO_TICKS` rounds down, `2 ms`
at `100 Hz` becomes zero ticks. Setting that numeric field alone cannot prove
the required bus-silent interval; check actual elapsed time from reset release.

## Indirect management and validation defects

REGS 192-212 places DEVAD in bits `[4:0]`, reserved bits in `[13:5]` and function
in `[15:14]`; C22 control/data addresses are `0x0D`/`0x0E`. BASE-H 38-41 names
functions `0` address, `1` data without increment, `2` read/write increment and
`3` write increment. Those SDK enum names do not prove every function is legal
on a particular TI register bank.

BASE 621-698 performs address selection, address write, data-mode selection,
then data read/write through separate mediator calls. It rejects DEVAD above
31 but supplies no whole-sequence owner or deadline. The register data API is
32-bit while the wire register is 16-bit; validate width before mutation.

**Confirmed source initialization hazard:** BASE 609, 628, 648 and 669 declare
`mmdctrl_reg_t` without initializing its whole value, then assign only DEVAD
and function. REGS defines nine reserved low-word bits that those assignments
do not initialize. The later whole-value write can therefore include
indeterminate reserved bits. The data-address union also has a 32-bit value
with only a 16-bit member assigned on write. Do not assume a compiler makes
these bits zero or that a physical PHY ignores them. Merely adding a mutex
does not fix this problem.

Reusing these helpers requires a verified correction in the selected SDK or
an adapter path through the already-correct shared register procedures. Do not
create a second independent chip implementation. Source-proved DEVAD 31 access
must put exactly `0x001F` into address mode and `0x401F` into nonincrementing
data mode, with reserved bits zero. Test exact whole words and invalid function
casts; BASE's data helper rejects function 0, not every out-of-enum value.
The unresolved EEE printed-address/wire-offset question is not solved by these
generic helpers.

## Identity and link evidence

| Source | Observed behavior | Application to this library |
| --- | --- | --- |
| BASE 509-531 | Auto-detection makes up to three passes over addresses `0..31`, accepts ID1 other than `0x0000/0xFFFF`, and ignores the read callback's return status | Up to 96 attempted reads is a search policy, not exact device qualification. Error/stale data must not select a PHY. Prefer declared address; any explicit discovery remains bounded and read-only |
| BASE 535-545 | Generic init detects address if requested, powers up, then resets | This changes hardware; neither init nor SDK install can be described as bus-silent binding |
| BASE 563-599 | OUI `(ID1 << 6) | ID2.oui_lsb`; model and revision come from ID2 | Preserve raw IDs and apply exact DP83826 model/revision/mode policy; generic OUI decoding alone is insufficient |
| BASE 217-305 | One BMSR read checks link; speed/duplex/partner reads and notifications occur when cached link changes | A first latch-low sample and same-link mode changes need the selected chip collector/observation contract. Do not invent current link or fresh resolved settings from a single generic event |
| BASE 289-300 | Up transition notifies speed, duplex, peer-pause, then link; cached link updates only after notification succeeds | Earlier callbacks may have effects before a later callback fails. Preserve that partial publication and avoid duplicate or fictitious resolution |
| BASE 357-371 | Any nonzero pause argument sets both ANAR bits 10 and 11 (`0x0C00`); zero clears both | This is not the complete four-combination advertisement API. Use shared exact procedures for full portable settings |
| BASE 292-297 | Pause notification is true for full duplex with partner symmetric-pause bit | It is a Boolean projection, not full IEEE RX/TX pause resolution; MAC flow-control enable policy remains external |

## SDK state and raw-entry ownership

DRIVER 49-65 stores `auto_nego_en` and custom register callbacks. Lines 77-98
route mediator reads/writes through the custom callbacks when present, otherwise
directly to MAC methods. DRIVER-H exposes those callbacks at lines 125/146.
They intercept frames, not atomic MMD transactions; forward to underlying MAC
methods rather than recursively calling the same mediator.

DRIVER 457-505 requires STOP for AN/speed/duplex changes; forced speed/duplex
also require cached AN disabled. `ETH_CMD_G_AUTONEGO` returns the cache, not
a fresh BMCR read. `ETH_CMD_S_AUTONEGO` calls the PHY and updates that cache.
DRIVER 511-528 raw ioctls get the PHY address and call the mediator directly,
bypassing PHY-specific register methods. Raw BMCR mutation must invalidate
managed knowledge and reconcile public SDK state before restart. Do not patch
private fields or invent nonexistent advertisement ioctls.

## Installation context and partial framework effects

| Source | Observed behavior | Integration requirement |
| --- | --- | --- |
| DRIVER 226-256; DRIVER-H 125/146, 403/414; DRIVER 557-572 | MAC/PHY pointers are assigned before initialization; custom register hooks receive the internal Ethernet handle before `*out_hdl` is published. Public `esp_eth_get_phy_instance` and `esp_eth_get_mac_instance` return those instances | Resolve the owned adapter context and MAC through these public APIs, with verified instance/lifetime rules; no private-layout cast or global singleton is needed |
| DRIVER 244-269 | Installation ignores `set_mediator` and `reset_hw` returns and initializes MAC before PHY. Failure attempts timer deletion without checking its return, then frees the driver without MAC/PHY deinit | A retained PHY failure cannot prevent earlier MAC effects. Require verified error propagation or a proved safe owner preflight, using a verified SDK correction where necessary. Unwind resources/stale mediators and retain failed cleanup evidence |
| DRIVER 300-320 | Start sets FSM START before negotiation, START event, link observation and timer start | A returned failure may retain START state and a published event. Test cached AN enabled and disabled, admission rejection and supported cleanup |
| DRIVER 323-344 | Stop sets FSM STOP before timer-stop, link-down and STOP event operations | Preserve partial stop effects and remaining callback/resource ownership on error |
| DRIVER 274-297 | Uninstall changes reference count from 1 to 0 and deletes the timer before PHY/MAC deinit | A later failure leaves partially dismantled state; retry/free needs the selected SDK's proved lifecycle path |
| DRIVER 450-451, 491-509 | Address/loopback setters can mutate PHY; flow control invokes MAC first, then PHY advertisement | Route controls through the owner; raw MDIO-hook rejection cannot undo a preceding MAC mutation |
| DRIVER-H 381 versus DRIVER 493-494 | Flow-control ioctl documents `bool*`, reads a Boolean for MAC, then dereferences the same pointer as `uint32_t*` | This is an argument-width/alignment defect for an ordinary Boolean object. Use a verified SDK correction or supported owner-routed calls with correctly typed MAC/PHY arguments and explicit partial-effect handling |

These observations add executing regression requirements to prompts 6 and
37-39. They remain static source evidence, not build or hardware results.

Required regression cases include stale-ID read failures during discovery,
exact zero-reserved MMD words, contention through the actual raw-ioctl path,
reset budgets below one SDK polling quantum, AN expiry without a link,
AN-to-forced stop/start/getter agreement, partial notification failure and
post-hardware-reset bus silence. Compare authorized physical traces to the
documented readiness and ownership expectations; a successful SDK link or mock
does not measure MDIO timing. Keep unavailable evidence explicitly open.
