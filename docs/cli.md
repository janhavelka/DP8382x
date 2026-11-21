# Shared example CLI

The portable C++17 processor in [examples/common](../examples/common/Dp8382xCli.h)
accepts commands through one application owner. Setup queues a prompt. It does
not bind, initialize, probe or reset a PHY. Cached commands call the owner's
snapshot callback once. That callback copies values under the existing owner
admission and performs no MDIO. Health, link resolution and configuration trust
are separate. The application still decides whether a link suits its session.

This processor provides local and cached queries, identity and discovery,
typed profile configuration, policy-checked register access, link observations,
event collection, cooperative lifecycle commands, diagnostics and opt-in
controls. The [network application](../examples/esp_idf/basic/README.md),
[raw-frame owner](../examples/esp_idf/owner/README.md) and compiled host fixture
all use this processor, grammar and record format.

## Input and commands

Use case-sensitive `<group> <verb> [args]` for grouped commands. The general
commands and listed aliases are single tokens. Spaces and tabs separate
tokens, including at the ends of a line. There is no quoting, escaping,
comment syntax, abbreviation or case folding.

The processor stores up to 192 edited bytes, 10 tokens including the command,
and 64 bytes per token. ASCII 0x20 through 0x7E and tab are accepted. Backspace
and DEL erase one stored byte. CR and LF terminate a line. A contiguous CRLF
terminates one line, including when it is split across calls. Every completed
line, including a blank, invalid or discarded line, consumes one request
number. A suffix at EOF without a delimiter is incomplete.

An overlong line, embedded NUL, high byte (including UTF-8), or other control
character poisons the line through its delimiter. Backspace cannot recover it.
After a transport input gap call `discardInput()` after delivering any older
retained bytes and before feeding newer ones. The gap breaks CRLF adjacency.
The next delimiter ends the discarded line, and its following line can run.
An argument or token failure rejects the entire line before dispatch.

| Command | Alias | Arguments and effect |
| --- | --- | --- |
| `help [topic]` | `?` | Show aligned help from the command and settings tables. Topics include `local`, `application`, `cached`, `connection`, `configuration`, `registers` and each general command. |
| `version` | `ver` | Report library and application identity. |
| `session <nonce>` | | Echo a 1–64 byte protocol token and report identity for synchronization. |
| `capabilities` | `caps` | Show passive capability evidence from accepted silicon identity. |
| `local board` | | Show declared board/backend, interface, clock, address and provenance. |
| `local color [off\|on]` | | Query or set human help styling. Default off. Each styled title resets ANSI before the line ends. |
| `local quiet [0..1]` | | Query or mute human help and prompts. Default 0. Protocol records remain enabled. |
| `local console` | | Show the processor's queue and backpressure counts, plus transport counters where available. |
| `application status` | | Copy the selected application's cached state. No MDIO. |
| `application stop` | | Request a stopped and fenced maintenance window. Raw-frame sessions must be released first. |
| `application start` | | Require verified settings, reconcile the stopped SDK state and start the framework. |
| `application admit`, `application release` | | Explicitly admit or release a raw-frame session, using the raw-frame owner's current policy. |
| `application send` | | Queue the raw-frame owner's fixed test frame when the build and session allow it. A later service turn decides submission. |
| `cached health` | `drv`, `health` | Copy managed MDIO health, counts and last error. |
| `cached link` | | Copy link validity, staleness, resolved speed/duplex and pause evidence. |
| `cached settings` | `cfg`, `settings` | Copy desired and observed link profile summaries and configuration trust. |
| `cached operation` | | Copy the owner slot and cleanup barrier. Does not poll or take a result. |
| `cached transport` | | Copy all-source MDIO batch/frame counts and controller flags. |
| `connection identity` | | Copy retained identity, including validity, admission and staleness. |
| `connection probe` | | Read both identity words at the bound address and check exact-part admission. No scan or selection. |
| `connection discover [first last]` | | Read raw ID words at each address in an inclusive range. Default DP83826I range is 0–7. Explicit ranges can use 0–31. |
| `configuration desired`, `configuration observed` | | Copy the named profile and trust, with observed staleness separate. |
| `configuration validate <profile>` | | Check a complete profile and optional page exchange without changing it or calling the owner. |
| `configuration apply <profile>` | | Verify a complete profile through the typed operation and reconcile the stopped SDK cache where applicable. |
| `configuration verify` | | Recheck identity and stable owned fields. No semantic writes. |
| `configuration invalidate` | | Attest external mutation under maintenance, without MDIO. Leaves history and cleanup obligations intact. |
| `registers read c22 <reg>` | | Read one policy-approved Clause 22 register, 0–31. |
| `registers read mmd <devad> <reg>` | | Read one admitted DEVAD/offset in a complete four-frame transaction. |
| `registers write c22 <reg> <value>` | | Write LEDCR at 0x18 under maintenance. |
| `registers write mmd <devad> <reg> <value>` | | Write vendor LEDCR alias 31:0x0018 under maintenance. |
| `registers modify <devad> <reg> <mask> <value>` | | Update selected LEDCR alias bits in one five-frame transaction under maintenance. |
| `registers bmcr <mask> <value>` | | Update only mask 0x3100 bits in one stopped two-frame operation under maintenance. |
| `registers dump c22 <first> <last>` | | Inspect up to 32 consecutive addresses through safe read policy. |
| `registers dump mmd <devad> <first> <last>` | | Inspect up to 32 offsets in one explicit MMD space through safe read policy. |
| `connection initialize [reset-kind]` | `init`, `begin` | Initialize and verify owned settings. Default `none`. |
| `connection end` | | Release through the application owner. See the teardown rule below. |
| `recovery reset <pcs\|hard>` | | Issue the selected reset and verify owned replay. |
| `recovery restart` | | Issue the circuitry restart, then verify owned state. |
| `recovery reinitialize [reset-kind]` | | Open a new restoration operation and budget. Default `none`. |
| `owner start initialize [reset-kind]`, `owner start verify` | | Admit the same initialization or read-only verification used by the other entry points. |
| `owner poll <token>` | | Service one explicit step of the active CLI token. The CLI also services admitted work on its ordinary ticks. |
| `owner status` | | Copy the slot and cleanup barrier without advancing. |
| `owner result [token]` | | Copy the latest delivered CLI operation's outcome, effects and original request number. |
| `owner cancel <token>` | | Freeze the active CLI token without transfers or a new maintenance grant. Effects already recorded remain. |
| `owner abandon` | | Clear local cleanup debt after the application confirms external disposition. |
| `link read [allowed-modes]`, `link partner [allowed-modes]` | | Make one coherent link and partner acquisition through the owner. Native use requires STOPPED. |
| `link cached [allowed-modes]` | | Describe the cached link, raw advertisement and optional policy match without MDIO. |
| `link collect` | | Consume six event/counter sources once through the collector. Native use requires STOPPED. |
| `cached events` | | Copy the aggregate and retained source words, including a consumed partial prefix. |
| `link watch <count> <duration-ms> [allowed-modes]` | | Sample the application's cached link periodically. |
| `test stress <count> <duration-ms>` | | Read safe Clause 22 PHYIDR1 once per round. |
| `test stop`, `test stats` | | Stop the active watch/stress without I/O or copy the last finished run's counts. |

Unsigned input uses decimal or explicit `0x`/`0X` hex. Leading zeros do not
select octal. Require at least one digit, reject signs, partial conversions and
values above 4294967295 except for the 64-bit operation tokens and BIST
observation interval. Each command then checks its argument ranges.
The wire's signed details use decimal with an optional minus and retain the
whole int32 range.
Enums match their complete tokens. Missing or extra arguments reject.

### Application commands

The P4 network application supports `status`, `stop` and `start`. The
raw-frame owner also supports `admit`, `release` and `send`. Unavailable
application routes, including all of them in the host fixture, return
`UNSUPPORTED`. These commands share the PHY owner's service loop. An
application status query copies existing evidence. Background SDK polls can
still contribute frames to all-source totals. The other commands use
the CLI work slot and a 5 s deadline.

Active traffic denies disruptive CLI work. Release a raw session and service
any pending operation result before requesting `application stop`. The
application opens maintenance only after a successful framework stop and any
required raw-frame quiescence. Use `configuration apply` for a verified AN or
forced profile. Raw `registers bmcr` effects can require `recovery reinitialize`
before the owner can verify and start. A failed operation retains its own
terminal result and any cleanup owed. The replay has a new token and budget.
`application start` reconciles the stopped SDK state before starting traffic.

The raw application requires the verified 100/full-only AN profile for start
and a fresh resolved 100/full link for admission.
`application send` accepts no bytes or destination arguments. It queues the
build's opt-in fixed test frame for later release to the SDK through ordinary
owner service. Queue acceptance carries no transmit result. The network
application leaves local-peer probing to the fixture operator.

### Typed profile syntax

```text
configuration apply an <modes> <pause> <pair> <flags>
configuration apply an <modes> <pause> <pair> <flags> wait
configuration apply an <modes> <pause> <pair> <flags> <partner-limit> <packed-pages>
configuration apply forced <10|100> <half|full> <pair> <flags>
```

`configuration validate` accepts the same profiles. AN modes are a nonzero
mask with bits 1 = 10/half, 2 = 10/full, 4 = 100/half and 8 = 100/full. Pause
is `none`, `symmetric`, `asymmetric` or `both`. Pair selection is `auto`,
`mdi` or `mdix`. Flags are a 0–7 mask. Bit 1 enables robust routing, bit 2
enables coupled pair/polarity mirroring and bit 4 requests extended full
duplex. Forced profiles require robust routing. Forced 10 cannot enable
extended duplex. The native stopped route also rejects extended forced
half duplex because it cannot publish a qualified SDK link mode.

With `wait`, stable setting verification and AN completion are separately
observed. For next pages supply a partner limit of 1–16 and one token of 1–16
four-digit hex words. A word uses message bit 0x2000, ACK2 bit 0x1000 and an
11-bit payload. NP and toggle are owner-controlled. Examples include
`2001` (final null) and `20040003` (remote fault, code 3). The existing
typed validator enforces messages 1/4/5/6, ordered unformatted page counts,
and reserved values. Core checks EEE compatibility and live prerequisites
under the original budget.

### Direct access limits

C22 addresses have five bits. Register 0x1F is distinct from its reset
command mask 0x8000. Direct reads reject selectors 0x0D/0x0E, consuming or
latched fields, reserved holes and procedure-only addresses. Vendor MMD
DEVAD 31 inherits the same rules for aliases. The additional safe EEE
tuples are 3:0x0014, 7:0x003C and 7:0x003D. No other DEVAD/offset is inferred
from a printed EEE address. Dumps report blocked rows as skipped and issue
no frame for them. Read-clear and follow-up observations stay with the
typed collector and its later CLI route.

Raw LEDCR writes may set only 0x0692. A write replaces the entire owned
subset. The vendor mask form preserves other admitted stable bits. BMCR
maintenance accepts a nonzero subset of 0x3100 and a value contained in
that mask. It cannot reset the PHY, set loopback or write power controls.
Both raw forms require explicit admission and the application must confirm
LED0 electrical use before granting LED maintenance. Accepted or uncertain
semantic writes withdraw current proof. After BMCR maintenance restore the
owned profile and reconcile SDK agreement before any native restart.

Discovery samples only PHYIDR1/2 in one batch per address. Address 0 is
ordinary. A zero or all-ones ID word means no plausible response evidence.
A transport failure keeps its address, signed detail and any valid first
word. Results neither select nor initialize a PHY, and cannot infer an
isolated or powered-down state.

### Lifecycle and ownership

`reset-kind` is `none`, `pcs`, `hard` or `restart`. NONE still programs and
verifies mandatory VOD and replays owned settings. PCS uses BMCR `0x00` bit
`0x8000`; HARD uses PHYRCR `0x1F` bit `0x8000`; restart uses PHYRCR `0x1F` bit
`0x4000`. The restore plan and quiet floor stay with the existing owner. An
owned EEE replay can select HARD even with a different request, so check both
reset fields. A successful initialization is followed by a separate link
observation for current link evidence. Native restart also requires the
application's stopped SDK reconciliation and start path.

Initialization and end require maintenance. Each initialization, reset or replay
gets one 5 s deadline and a 512-frame allowance. End on a portable binding is
local and bus-silent once results and cleanup obligations are settled. The
native bridge requires the application's quiesce/teardown hook. That hook owns
stop/fencing and retained resources. It receives an absolute deadline or zero
when the CLI cannot provide one. A missing hook returns `INVALID_CONFIG`.
`owner abandon` requires both maintenance and independent application
confirmation of an external reset, power removal, isolation or ownership
transfer. The command cannot itself attest disposition. Abandon preserves
historical evidence and records no proof of restoration or controller recovery.

Operation tokens accept positive decimal or hexadecimal `uint64_t` values.
Poll and cancel apply to the CLI's active token. A poll or cancel reply uses
its own request ID. The admitted operation keeps its own originating request
and emits exactly one terminal result. A cancelled or failed operation may
leave cleanup debt. Reinitialize uses a new token, deadline and result. The
retained `owner result` copy stays available across other local commands and
is replaced when the next terminal operation result is delivered.

### Link and bounded runs

An `allowed-modes` value is a nonzero 1–15 bit mask with the same mode bits as
the AN profile. It checks a resolved current link against caller policy and
does not change settings or application readiness. A live link command uses
the core's complete 8-frame plus 2-frame consistency check with a shared 2 s
deadline. The first BMSR word is historical latch-low evidence. The second
and final BMSR/PHYSTS reads establish current consistency. Partner evidence
comes from the same acquisition, and is usable only for a qualified AN base
page. Forced and incomplete negotiation withhold unproved mode or pause data.

The six-word collector reads PHYSTS, FLDS, MISR1, MISR2, FCSCR and RECR in one
batch. The first failed word ends the batch. Valid returned words, including a
consumed MISR1 before a failed MISR2, remain in retained history. Cached views
never consume the hardware again. FCSCR contributes to the host sum. RECR
remains a raw word under its documented interpretation gate.

Watch and stress allow 1–32 rounds and durations from `count` to 5000 ms. They
start on a service tick and space subsequent rounds by the duration divided
by count in microseconds, with no catch-up burst. Deadlines are exclusive and
the host must keep servicing ticks and output. Watch samples cached evidence,
including staleness, so changes depend on the application's collector service.
Stress attempts one raw PHYIDR1 batch per round and contributes zero managed
health transactions. The runs share the CLI work slot and the console record
budget. Outstanding owner work or cleanup debt blocks a new run. A stop or
fault ends the run and preserves every queued sample.

## Typed diagnostics

Use `help diagnostics`, `help controls`, `help irq`, `help power`, `help eee`,
`help lpi` and `help wol` for command forms. The owner validates each complete
request and uses its own typed settings and procedure. Supplying a flag asks
for a particular path. The application independently verifies wiring, clock,
traffic and fixture prerequisites before admitting it. The native bridge uses
the stopped owner for live feature calls and holds shared silence for procedures
that can reset. Physical operation requires the identified fixture.

Live typed reads require a current accepted PHY identity. Before a successful
probe or initialization they return `NOT_INITIALIZED` without a transfer.
A current rejected identity reports the identity admission error. Cached
queries and the explicit LPI stop keep their existing admission rules.

`diagnostics selftest` reads the bound identity in a two-frame batch and PHYRCR
in a one-frame batch on a later tick. It stops on the first failure or active
reset command. The two batches share a 2 s deadline. It acquires no destructive
counters and enables no stimulus. `check=identity` and `check=reset_status`
identify samples. A successful sample requires complete frame evidence. The
terminal uses the run fields and counts actual attempts.

| Command family | Input and effect |
| --- | --- |
| `diagnostics loopback-start mode speed [flags [fifo]]` | `mii`, `pcs-in`, `pcs-out`, `digital`, `analog`, `reverse`, `external`. Use `10` or `100` Mbit/s. Select the single flag for the path. Flags 1 enables 100M MII line transmission, 2 analog termination, 4 live peer, 8 external path and 16 digital with HARD exit. Ordinary MII/PCS use zero. PCS input requires 10M and digital requires 100M. Reverse FIFO codes 0–3 select 4/5/6/8 nibbles. |
| `diagnostics loopback-stop`, `diagnostics loopback-status` | Stop with the existing restoration procedure, or copy retained activity and effects. Digital exit can require HARD. |
| `diagnostics bist-start mode counter bytes ipg observe-us speed [path [flags]]` | `off`, `single`, `checker`, `both`. Counter mode `saturate` or `wrap`. Packet length 0–2047 bytes and IPG 0–1020 bytes in multiples of four. Give the minimum observation interval as a positive uint64 in µs. The core checks that the interval fits when added to the activation time. Speed `10` or `100`. Path `line` has flag 0. Use the corresponding loopback flag for `digital`, `analog` or `external`. |
| `diagnostics bist-read [none\|single\|stimulus\|both]`, `diagnostics bist-stop`, `diagnostics bist-status` | Explicit capture consumes a BIST error sample. Optional evidence requests require separate application confirmation. Stop restores the saved settings. Status only copies the last evidence. |
| `diagnostics cable-start mask flags`, `diagnostics cable-raw channel dc avg segment duration-skip shadow thresholds flags` | SNLA428 run mask 1–63 selects 1A/1B/2/3/4/5. Raw tuning uses `ab`, `ba`, `aa` or `bb`, DC 0/1, averaging code 0–6 and segment 1–5. The remaining words pack the documented fields below. Both commands require flags 3 for a quiet peer and admitted HARD exit. They retain raw captures with per-word qualification. |
| `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret delay-ps-per-m` | Reread the current manual capture, stop/restore with HARD, copy history, or interpret a qualified SNLA428 capture. Interpretation needs positive caller-supplied propagation delay in picoseconds per metre. Raw tuning and incomplete capture withhold a distance. |
| `diagnostics cable-autorun off\|on`, `diagnostics cable-autorun-read`, `diagnostics cable-autorun-status` | Configure one hardware link-loss TDR policy. An explicit acquisition returns raw hardware results without waiting for a known trigger. Status copies policy. |
| `diagnostics compliance-start mode mdi\|mdix pattern flags`, `diagnostics compliance-stop` | Modes `tx100`, `nlp10`, `prbs10`, `pattern10` and source-gated `count100`. Pattern is zero except `pattern10` codes 0–13 and gated `count100` codes 0–31. Flags 3 requests the identified stimulus fixture and admitted HARD exit. Stop and persistent reapply use separate receipts. |
| `diagnostics collision-start 10\|100 3`, `diagnostics collision-stop`, `diagnostics calibration-recover 1`, `diagnostics electrical-status` | Collision requires MII and proven COL/TX_EN control. Recovery requires distinct attestation for the CDCR contingency and HARD. Status copies retained effects and validity. |

Exact raw field ranges, measurement limits and stop semantics are maintained
with the corresponding procedures. [Loopback](loopback.md), [BIST](bist.md),
[cable](cable.md), [electrical](electrical.md)

The cable `duration-skip` word packs duration at bits 15:8 and initial skip
at 7:0. `shadow` packs averaging at 6:4 and forward shadow at 3:0. `thresholds`
packs location at 12:8, short threshold at 7:3 and pulse width at 2:0.
Reserved high bits must be zero.

A denied application grant returns `BUSY` with owner-admission detail.
The core also validates individual board and mode prerequisites. An absent
bridge returns `INVALID_CONFIG` with `reason=backend`. Read the status and
detail together. `SOURCE_GATE` identifies an unmet procedure prerequisite
such as missing exact-part evidence or unconfirmed board routing. It may
accompany `INVALID_CONFIG` or `UNSUPPORTED`. A confirmed silicon exclusion
such as BASIC mode without LED2 uses `UNSUPPORTED` without that detail.
Cooperative commands have a 5 s original deadline and up to 512 frames.
An untimed cache query does no I/O. Terminal acquisitions have a 2 s deadline
and up to 64 frames, with narrower collector limits where specified.

## Typed controls

Use `keep` only for an unowned setting. A complete profile includes every
previously owned field required by the typed procedure. Status commands copy
retained observations with independent validity and staleness.

| Command family | Input and effect |
| --- | --- |
| `controls pins p0 p1 p2 p3 blink stretch swap0 clock1` | Pin tokens `keep`, `led,source,polarity,drive`, `wake`, `irq`, `col3`, `col5`, `col6`, `high`, `tx-error`, `clock`. LED source 0–14, polarity `keep`, `low` or `high`, drive `normal`, `low` or `high`. The core checks pin-specific values and rejects reserved LED3 drive. Blink `keep` or 0–3 for 50/100/200/500 ms, stretch `keep`, `enabled` or `bypass`, swap `keep`, `off` or `on`, clock1 `keep` or `override`. |
| `controls interface mii\|rmii25\|rmii50 rmii-flags elasticity bytes ppm compat-flags clock isolate` | RMII flags 1 follower-only TX shift, 2 RMII 1.0, 4 RX_DV, 8 repeater. Elasticity codes 0–3 denote 14/2/6/10 bits. Bytes and ppm are signed-independent nonnegative board bounds. Compatibility flags 1 fast RX_DV, 2 **disable** odd-nibble detection, 4 MII COL/full-duplex, 8 120 ns IPG. Clock `keep`, `clkout25`, `led1`, `pll` or `recovered`, with the last two subject to source gates. Isolate `keep`, `off` or `on`. Repeater configures one side and requires application-confirmed topology for both PHYs. |
| `controls quality fld tx100 tx10 recovery` | Each field group can use `keep`. FLD cause mask is a subset of `0x040f`, MII-only when active. Optional FLD raw codes are `mask,window,errors,descrambler` in ranges 1–63/1–63/0–63. TX100 is `scrambler-bypass,fifo-code,rx-error-idle` with FIFO 0–3. TX10 is `lower-threshold,squelch,nlp-disable,jabber-disable,short-preamble` with squelch 0–8. Boolean tuple values 0/1. Recovery `off` or `on`. |
| `controls mse`, `controls mse-status` | Acquire one raw DEVAD 31 word at offset `0x0218` with existing link context, or copy it. Preserve the source interpretation gate. |
| `controls analog amp term mac-slew io-slew gain` | Amplitude tuple is negative MDI/MDIX `0x28..0x38`, positive MDI/MDIX `8..0x18`, and zero code `0..63`. Termination tuple is MDI/MDIX 0–15 and independent 0/1 enables. MAC slew `keep`, `fast` or `slow`; I/O slew `keep`, `slow` or `fast`. Numeric receiver gain 0–31 reaches the source gate. |
| `controls pins-status`, `controls interface-status`, `controls quality-status`, `controls analog-status` | Copy the matching retained settings and raw readback. |
| `irq configure misr1 misr2 polarity\|wake intr\|led1\|led2 flags` | Separate 0–255 event enables. Flags 1 active-high, 2 global delivery, 4 enhanced output disable. Pin role and excluded clock/COL users require application evidence. |
| `irq service [transactions samples frames]`, `irq cached-service`, `irq status`, `irq test-start`, `irq test-stop` | Defaults 2/5/5. Explicit service consumes each MISR once and conditionally reads requested counters. Use cached event history for partial evidence. Test start requires an explicit stimulus grant. Test stop and replay preserve independent outcomes. |
| `power configure normal\|down\|deep sleep pll-off`, `power read`, `power status` | Active sleep and PLL request are 0/1. PLL-off requires IEEE down. Entry/exit requires management and clock-loss evidence as applicable. Deep-power controls remain source-gated. |
| `eee configure off\|on advertisement bypass-mask`, `eee read`, `eee status` | Advertisement 0 or 2 for 100BASE-TX. Bypass-mask bits 0 and 1 request bypass of control bits 0 and 3. Configuration uses shared HARD reset and replay, with a compatible owned AN profile for enable. Read acquires local, partner and status evidence, including consuming EEE event words. |
| `lpi start`, `lpi stop`, `lpi status` | Legacy MAC LPI entry needs application-owned TX drain and current qualified 100/full EEE evidence. Readback of request/stop preserves observed and uncertain effects and carries no packet-readiness proof. |
| `wol configure off\|magic\|secure status\|irq\|led0\|led1\|led2\|led3 crc level\|pulse width mac [password]`, `wol read`, `wol status`, `wol acknowledge` | CRC 0/1. Width codes 0–3 denote 8/16/32/64 cycles of 125 MHz. MAC and secure password use six colon-separated hex octets. Secure mode requires the password. RX quiescence and admitted pin routes are application responsibilities. LED0/LED3 wake routing remains source-gated. Acknowledge consumes RXFS/MISR2 once before the qualified W0C command and keeps the W1S bit neutral. Records omit address and password bytes. Raw CLI read/dump rejects the vendor password words prior to any I/O. |

Additional exact-part field limits and semantics are documented with the
procedures. [Interrupts](interrupts.md), [interface and clock](interface-clock.md),
[quality](quality.md), [power](power.md), [EEE](eee.md), [LPI](lpi.md),
[WoL](wake-on-lan.md)

The application receives the selected feature and the requested proof mask
through `FeatureAttestation`. The diagnostic routes pass the applicable flags
above. A zero mask still asks for a feature grant, for example when arming the
IRQ stimulus or link-loss cable autorun. Pin bits 0–3 release selected LED pins,
bit 4 releases LED1/CLKOUT, bit 5 releases MII COL and bit 6 requests quiesced
outputs. Interface bits 0–3 cover pins, reference, SMI and clock changes, bit 4
covers repeater wiring and bits 15:8 carry the requested reference MHz. The
10M quality gate uses bit 0. Analog bits 0 and 1 cover the identified fixture
and termination override. IRQ, power, LPI and WoL use the `ControlProof` values
documented in the owner header. A valid grant depends on current application
state and must survive the admitted procedure's terminal delivery.

## Console service

Serialize `setup`, `feed`, `discardInput` and `tick` in the application task.
The borrowed context and immutable identity strings live through its last call.
Setup succeeds once per processor. The snapshot, clock and write callbacks
return promptly, do not reenter the processor and do not retain buffer pointers.
Use `feed` to accept at most 128 bytes per call. Keep the unaccepted suffix for
another call. After a real transport overrun use the input-gap rule above.

The FIFO holds 4096 bytes and copies each accepted chunk into its storage.
Human chunks can be dropped whole when full. `tick` offers at most one
contiguous span of at most 256 bytes to the nonblocking writer. The writer
returns the length of its accepted prefix, including zero for backpressure.
The queue retains all remaining bytes. No controller lock spans output work.

Both P4 builds use the board's UART0 serial bridge at 115200 baud, 8 data
bits, no parity and one stop bit. The application pins ESP-IDF log V1 and
disables the secondary USB console. Its UART task alone calls the driver's
TX path. The driver takes a TX mutex before accepting bytes into the FIFO.
Keeping TX with one task prevents contention on that path. A zero-byte return
leaves the suffix for a later pump turn. The owner continues servicing the
processor, application events and due operations without waiting for UART.

The driver has a 1024-byte RX buffer and a 16-event queue. The UART pump reads
up to 128 bytes and offers up to 256 output bytes per turn. The owner shares a
512-byte input ring and 2048-byte processor-output ring with the pump. UART
overflow, frame, parity or break events and event queue saturation mark an
ordered input gap. The pinned driver's ISR notification latches FIFO overflow,
frame and parity errors even when the event queue drops the report. Startup
enables framing-error detection explicitly. Draining 16 events during a
check marks possible queue loss. Checks before and after a read keep a
damaged fragment away from dispatch. The pump discards the affected driver
input and reports the gap to the owner. The processor finishes older retained
bytes, discards any damaged line through its delimiter and resumes on the
following line. The reported input-byte loss is a lower bound when its exact
size is unknown.

The application status lines and ESP log V1 hook use a separate queue of
eight ordinary lines of up to 384 rendered bytes, plus one line being drained.
Formatting uses a local buffer. Queue insertion uses a short try-lock and
drops a whole line on contention, capacity exhaustion, invalid framing or
truncation. The pump prefixes ordinary lines with `[log] ` and keeps them
outside in-progress machine records. Their storage is independent of required
terminal records. SDK log V2 can fragment a line across hook calls. ROM,
panic and direct `printf` output can bypass the hook and corrupt a record.

After queuing output, `tick` advances at most one owner call or one
discovery/dump row. These ranges use a single 2 s exclusive deadline and
a frame allowance for the whole range. A row can consume one C22 frame,
a two-ID batch or one four-frame MMD batch. Global admission, clock and
deadline errors stop the range. Independent transport errors are retained
per row and do not authorize a retry. Monotonic time and shared quiet floor
checks use the owner's actual clock. A copy of an expired floor or a failed
prior receipt cannot deny or replace fresh evidence.
A successful row requires a fresh batch receipt. A missing or invalid
receipt stops the range and leaves only a known prefix of the frame count.

The typed operation uses a 5 s exclusive deadline, 512 frames, 1 ms
scheduled poll interval and 4096 stalled core polls. Follow
`workPending()` even when the console has nothing to send. Application
scheduling must continue serving an admitted token and permit delivery of a
terminal result after transient contention. It must keep maintenance and
stopped SDK conditions through apply and reconciliation. Result delivery
waits for a record slot. Polling an already admitted operation can continue
while output is stalled.

At the exclusive deadline the CLI stops advancing the operation. If the result
is still running, the CLI requests owner cancellation without more MDIO and
takes the frozen result once a record slot is available. The terminal record
reports `OPERATION_TIMEOUT` with the separate core cancellation and effect
evidence. A result already frozen can still be taken after the deadline.

A detected clock regression disables new live CLI commands until the processor
is replaced. The CLI cancels any operation that still needs polling through
its owner without advancing hardware work, then takes the retained result.
The terminal record keeps the core status, possible effects and any cleanup
owed. A clock fault also prevents SDK reconciliation and prevents a successful
watch, stress or selftest terminal if detected while recording its final sample.
Local and cached queries remain available.

The owner status/result views can run during an active operation when the
console can accept another line. An explicit poll/cancel uses the first available
service tick and retains its own response. The processor stops accepting another
input line until that response can be stored. Console writes run outside the
owner call and controller lock.

A complete record occupies at most 1024 bytes including CRLF. The queue admits
the entire record and any needed leading CRLF together, using its logical tail
to detect an open prompt or partial human line even after short writes. When
the record cannot fit, the processor holds its serialized bytes in one slot
and stops accepting new input until the slot clears. A writer stall does not
repeat a command or overwrite its outcome. Separately admitted asynchronous
features must retain their own result until this delivery boundary. Queue
acceptance means local ownership of the bytes. Host receipt needs a matching,
complete record on the other end.

The prompt is `> ` and the title is `=== DP8382x CLI Help ===`. Human output
and machine output use the same cursor tracking. ROM, panic or other direct
output that bypasses the queue can corrupt a record. The host must reject a
corrupted candidate and keep its evidence as received.

## Schema 1

A record starts at column zero with the exact ASCII prefix `@DP8382X ` and
ends with CR LF. Its body is space-separated `key=value` fields, with
one ASCII space between adjacent fields and no leading/trailing space in the
body. Field names are lowercase ASCII letters, digits and underscore, starting
with a letter. Values use ASCII letters, digits, underscore, period, colon or
hyphen. There are no escapes. Application identity tokens and session nonces
must be 1–64 bytes in this alphabet. Report unavailable archive identity and
its tree as `unknown`. Known firmware may have `clean`, `dirty` or `unknown`
tree evidence. The release value in `library` is independent of the firmware
build identity.

The receiver removes exactly the CRLF delimiter. It discards lines without a
column-zero prefix and does not strip whitespace, ANSI or prompt bytes from
a candidate. A prefixed record with invalid bytes, malformed/unknown/duplicate
fields, missing mandatory fields, bad types or an unsupported schema is
invalid evidence. Match the field set for the command and state. Field order
is stable on output and immaterial to validation.

| Common field | Meaning |
| --- | --- |
| `schema` | Decimal 1. |
| `boot`, `req` | uint32 application boot identifier and uint64 firmware request counter starting at 1. A fresh processor resets the counter. The application supplies a varying boot value. |
| `type` | `session` for a successful handshake, `sample` for a range row, otherwise `result`. |
| `cmd` | Canonical command. Groups use a dot as separator, such as `cached.health`. `unknown` means dispatch could not select one. |
| `op` | uint64 operation token. Zero when the request has no operation. The cached slot token is a separate payload value. |
| `state` | `terminal` for a final outcome. A cooperative start emits `admitted` with `primary=IN_PROGRESS` and a nonzero token when the core accepts work. `progress` marks one range or run sample. |
| `primary`, `detail` | Exact printable `Err` and signed int32 detail. Only terminal `OK` is command success. Details never narrow a backend error. |
| `cleanup`, `cleanup_detail` | Separate printable cleanup outcome, or `none` and 0 when no cleanup was executed. A later cleanup cannot replace a primary error. |
| `time_valid`, `time_us` | 0/1 and uint64 firmware clock in microseconds, sampled when the record is created. Both are zero when the clock is absent or a fault is detected. On a valid clock, zero is usable. |
| `deadline_valid`, `deadline_us` | 0/1 and uint64 exclusive command deadline on that clock. Local commands and early validation errors use zero. |
| `reason` | `none`, or the fixed local reason listed below. |

Decimal wire values have no sign except a leading minus for signed negative
details. Bounds are 0–4294967295 for uint32, 0–18446744073709551615 for uint64,
and −2147483648–2147483647 for int32. Flags are exactly 0 or 1. Firmware time
cannot be compared directly to the host's elapsed or UTC clock. Request numbers
belong to a boot, with no separate counter-exhaustion rule.

Local syntax errors use `INVALID_PARAM` with detail 0. Reasons are `empty`,
`command`, `topic`, `arity`, `enum`, `number`, `nonce`, `token_count`,
`token_length`, `line_length`, `byte`, `input_loss`, `profile`,
`range`, `space` and `access`. Missing live owner support uses
`INVALID_CONFIG` with `reason=backend`. A nonterminal or
invalid cached callback status becomes `INVALID_CONFIG` with reason
`callback` and retained signed detail. An internal formatting failure uses
`INVALID_CONFIG` and reason `format`. Other callback errors are returned with
their actual status. Error records may carry a valid acquired prefix or
write/operation receipt. They never invent successful ordinary read output.

### Success payloads

| Command | Additional fields in a successful record |
| --- | --- |
| `help` | None. The human help remains optional output. |
| `session`, `version`, `local board` | `nonce library firmware tree sdk target backend clock variant interface address console baud`. Non-session commands use `nonce=none`. The address is 0–31, and the interface is MII or RMII. `baud` is the UART rate where applicable. Zero denotes an unavailable or inapplicable rate. |
| `local color`, `local quiet` | The setting's name with value 0 or 1. |
| `local console` | `queued dropped_human stalled_writes`. The first is processor queue bytes, the other two are saturating uint32 counts. The P4 builds add `input_queued output_queued lines_queued input_dropped input_gaps lines_dropped lines_truncated uart_stalls output_backpressure`. The first three are queue depths in bytes, bytes and lines. The other values are uint32 totals modulo 2³². `input_dropped` is a lower bound when the driver reports possible loss. `output_backpressure` counts short or refused writes from processor to pump. |
| `application status` | The P4 network view uses `installed traffic maintenance retired shutdown carrier ip events`. The raw owner view reports its phase, generation, readiness, restart and maintenance state, frame accounting and retained SDK errors. |
| `application send` | `queued=1 generation` on raw-frame queue acceptance. Later SDK and frame outcomes remain in `application status`. |
| `capabilities` | `valid modes interfaces copper fiber gigabit`. Masks are the portable `LINK_*` and `INTERFACE_*` values and zero if invalid. |
| `cached health` | `health initialized successes failures consecutive last_error last_detail last_ok_valid last_ok_us last_error_valid last_error_us`. Health names are UNINIT, READY, DEGRADED and OFFLINE. Counts are uint32. |
| `cached link` | `valid stale up resolved speed_mbps duplex negotiation pause_valid pause_rx pause_tx observed_valid observed_us`. A current result requires valid=1 and stale=0. Unresolved fields use 0 or `unknown`; duplex is `half` or `full` when resolved. Negotiation is `disabled`, `in_progress`, `complete` or `unknown`. Observed time may be historical. |
| `cached settings` | `trust desired_valid desired_mode desired_modes desired_speed_mbps desired_duplex observed_valid observed_stale observed_mode observed_modes observed_speed_mbps observed_duplex`. Mode is `an`, `forced` or `unknown`. Only the active mode's corresponding data applies. Observed staleness is independent of desired intent. |
| `cached operation` | `slot token slot_status slot_detail cleanup_owed temporary slot_deadline_us`. Slot is `idle`, `running` or `result_pending`. An occupied slot carries its own token and deadline. The query's terminal outcome does not advance that slot. |
| `cached transport` | `batches attempted completed stopped clock_invalid controller_reusable`. Counts are uint32 and can include application startup or other owner traffic. |
| `owner status` | The same payload as `cached operation`. |

### Application payloads

In the network application, `installed` indicates an installed owner.
`traffic`, `maintenance`, `retired`, `shutdown` and `carrier` are 0/1
indications. `ip` is the last application IPv4 address in dotted decimal
and `0.0.0.0` means no address. `events` counts received Ethernet and IP callbacks.

In the raw-frame application, `phase` is `unconfigured`, `waiting`, `ready`,
`active`, `withdrawn` or `maintenance`. `gen` is the raw-frame generation.
`ready`, `restart`, `maintenance` and `retired` are 0/1 indications. `rx`,
`rxd`, `rxexp` and `rxother` count accepted, dropped, experimental and other RX
frames. `tx` and `txd` count queued and dropped submissions. `dma`, `txdenied`,
`txbp`, `txdown` and `txfailed` count DMA queue acceptance, admission denials,
backpressure, cached link-down and other SDK failures. `txattempted` qualifies
the SDK transmit history. `txfirst` and `txsdk` carry the signed first and
latest SDK error evidence. Use `owner status` for the PHY's operation slot.

### Lifecycle, link and run payloads

- Admitted initialize/reset/replay terminal results include `core_status core_detail`,
  `requested_reset effective_reset`, `reset_required reset_accepted reset_complete`,
  `persistent_restored mutation_completed mutation_possible cleanup_owed temporary`,
  `clock_invalid link_valid` and `sor1_valid sor1 sor2_valid sor2 straps_stale`.
  `link_valid=0` indicates that the operation did not acquire a fresh link.
  `reset_accepted` means the reset
  frame completed, while `reset_complete` requires later readback. SOR words can
  be historical when `straps_stale=1`. Admission carries the active token.
- `owner poll` includes `origin_req poll_status poll_detail` for the one poll.
  The query's `primary=OK` acknowledges service. The originating operation's
  status, cancellation and cleanup debt belong to its own terminal record.
  `owner cancel` acknowledges the request with `origin_req`. `owner result`
  carries `origin_req outcome outcome_detail core_status core_detail sdk_error
  mutation_completed mutation_possible cleanup_owed temporary` for a retained
  operation. It uses the query's request number and the referenced operation token.
- Detailed link records carry `observation valid stale up resolved speed_mbps
  duplex negotiation pause_valid pause_rx pause_tx observed_valid observed_us`,
  `policy_mask advertised_valid advertised_modes partner_valid partner_modes`,
  and raw `bmsr_first bmsr_current anar anlpar aner`, `last_status last_detail`
  and `round`. Observation is `unobserved`, `stale`, `down`, `pending`,
  `unresolved`, `wrong_policy`, `up`, `unavailable` or `management_failure`.
  `pending` denotes AN in progress while link is up. A down sample can still
  show `negotiation=in_progress`. An ordinary observation uses `round=0`.
- Event records carry `valid stale event_mask physts flds misr1 misr2 fcscr recr
  false_carrier_total lower_bound saturated observed_valid observed_us last_status
  last_detail`. `valid` and `stale` describe the aggregate. Mask bits 0–5
  identify the six word positions in collector order. In `link collect` this
  is the current call's audited prefix. In `cached events` it is retained
  history, and the timestamp applies only to the last complete aggregate.
- Watch and stress emit `type=sample state=progress` under the original request.
  Stress samples carry `round address valid value attempted`. A terminal run
  carries `rounds requested attempted attempted_known managed`. `rounds` counts
  successful rounds, `attempted` counts attributable physical frames, and
  `managed=0` identifies the use of cached or raw access. `test stats` uses a
  separate request and copies the last finished run with `running origin_req
  rounds attempted attempted_known managed outcome outcome_detail`.

For example, cancel token 2 after `init hard` admission with `owner cancel 2`.
Wait for `connection.initialize` on the admission's request number with
`state=terminal primary=CANCELLED`. If it carries `cleanup_owed=1`, an admitted
`recovery reinitialize hard` has a new request, token and result. The earlier
`owner.cancel` acknowledgement does not report replay success.

### Typed feature payloads

An admitted feature emits the standard `admitted` record with its token. Its
terminal record carries `core_status core_detail mutation_completed
mutation_possible cleanup_owed temporary persistent_restored effective_reset
reset_accepted reset_complete` plus its typed evidence. Stops and persistent
replay are separate, so a successful later operation cannot replace the failed
earlier outcome. A direct control reports its terminal result on the same
request without `admitted`. Optional fields flagged invalid carry no evidence.

- Loopback emits `mode speed_mbps controls_verified stopped hard_exit
  physts_valid physts`. Its cache view uses `session_op` and `stimulus_possible`
  to distinguish retained state from an admitted command token.
- BIST emits `session_op`, `outcome`, and independently guarded `before`,
  `counter` and `after` words. `measurement_valid`, `activity`, `lock`,
  `sync_loss`, `errors`, `saturated`, `wrap_possible`, `interval_uncertain`
  and `history_lost` describe the acquisition. `active_valid` guards the
  cached active-state claim. A zero counter can remain `inconclusive`.
- Cable receipts use `session_op recipe requested_mask complete stale stopped
  autorun status_changed prerequisite_valid prerequisite`. The six rows
  `r1a r1b r2 r3 r4 r5` carry uppercase hexadecimal
  `flags:cdcr:index:amplitude:peak`. Bits 0–3 guard the four words, bits 4–7
  mark staleness and bits 8/9 denote accepted/possible trigger. Bit 10 marks
  observed freshness, bit 11 marks CDCR failure and bits 13:12 give row state
  0 (not run), 1 (started) or 2 (complete). The interpretation command
  produces a `conclusion` and `distance_um` with `distance_valid`. Autorun's
  policy receipt preserves `desired` and `observed` with separate validity.
- Electrical receipts distinguish `compliance`, `collision`, verified controls,
  `stimulus_possible`, `stopped`, mode/pair/pattern, `collision_mbps`,
  `cdcr_valid cdcr recovery_needed hard_owed` and `electrical_reset_complete`.
  These fields describe retained procedure evidence.
- Pin, interface, quality and analog reports emit `desired trust stale
  valid_mask` and raw fields in each core snapshot's register order. The mask
  identifies valid words in output order. Direct pin/interface reports also
  include `attempted completed transactions batch_primary batch_detail
  release_status release_detail mutation_attempted mutation_possible verified`.
  MSE reports `raw_mse` with `valid stale observed_valid observed_us` and
  separately qualified previous `context_valid context_stale context_up`.
- IRQ policy and service emit qualified PHYSCR/MISR/RXFS or PHYSTS/MISR data.
  Service includes `more_pending`, selector attribution and independent
  `fcscr_requested fcscr_valid fcscr fcscr_lower_bound` and `recr_requested
  recr_valid recr`. The cache preserves consumed material across later errors.
- Power reports `power_state requested_down indicated_down requested_sleep
  requested_pll_off` and independent validity for BMCR, PHYSCR and BISCR.
  EEE reports requested enable, capability, advertisement, partner, EEE status,
  wake errors, control and negotiated evidence with independent validity.
  LPI reports the retained action and its control readback, with `batch_primary`,
  `batch_release`, their signed details, and completed or uncertain effects.
- WoL reports `desired_valid trust stale route control_valid control enabled
  secure level pulse crc magic match_word_mask rxfs_valid rxfs misr2_valid
  misr2`. The match mask reports word validity and carries no credentials.

### Identity, registers and profile payloads

Reads and samples encode raw words in decimal. `id1_valid`, `id2_valid`,
`decoded_valid` and `stale` govern meaning independently. Cached identity and
probe carry `id1 id2 oui model revision mode identity_state identity_status
identity_detail` alongside those flags. Modes are `basic`, `enhanced` or
`unknown`. Identity state is `accepted`, `rejected` or `unassessed`.
A zero/default payload on a failed probe does not replace historical identity.

Successful direct reads carry `address devad value`. C22 uses `devad=0`.
Raw write results after the owner call carry `effect batch_valid
batch_violation release release_detail`. Effect values are `applied` for a
completed target frame, `possible` for uncertainty, `none` when a fresh receipt
excludes a target effect, and `unknown` when no receipt exists. A completed
write still needs verified configuration. Release is an independent batch
result, distinct from operation cleanup. `release` uses the core's audited
status, with the signed detail retained even for a malformed report.
`batch_valid=1` means this call
produced the receipt, and `batch_violation` is the core's audit enum ordinal
with 0 for none. A malformed receipt does not authorize publishing ordinary
values or counting its attempts as exact.

Discovery samples carry `address id1_valid id1 id2_valid id2 candidate
attempted batch_valid batch_violation`. Dump samples carry `address devad
valid value skipped attempted batch_valid batch_violation`. A skipped row
reports the original access-policy error and is excluded from the terminal
error count. A terminal range result carries `rows errors skipped attempted
complete attempted_known end_status end_detail`. `complete=1` means the
whole range was visited. `attempted_known=0` means the count is a known
prefix only. Primary status retains the first error. `end_status` separately
reports a global stop cause. A complete range can still contain failed rows,
and a complete dump can contain only skipped rows.

Desired and observed profile views carry `valid stale trust mode modes
speed_mbps duplex pause_sym pause_asym pair flags`. Stale observed fields
remain historical. Validate and invalidate have no extra success payload.
Apply/verify admission has no extra payload. The corresponding terminal
receipt carries `core_status core_detail sdk_error mutation_completed
mutation_possible cleanup_owed temporary an_complete`. The core result stays
visible if SDK reconciliation fails. A nonzero SDK error produces terminal
`INVALID_CONFIG` unless the owner supplies a more specific core error. The
portable owner has no SDK error. `cleanup_owed` identifies later restoration
debt. This command does not perform restoration.

Operation receipts also carry `np_requested np_complete np_owed np_refused
np_base_event np_base_valid np_base np_missing np_tx np_rx np_duplicates
np_words`. `np_words` is `none` or a packed sequence of four-character
uppercase hex words. `----` preserves each missing payload slot. The TX/RX
counts include completed local loads and consumed partner slots, respectively.

For example, `registers read c22 0x02` can yield a valid terminal record:

```text
@DP8382X schema=1 boot=42 req=4 type=result cmd=registers.read op=0 state=terminal primary=OK detail=0 cleanup=none cleanup_detail=0 time_valid=1 time_us=9500 deadline_valid=1 deadline_us=2009400 reason=none address=2 devad=0 value=8192
```

A failed target write can still carry possible effect:

```text
@DP8382X schema=1 boot=42 req=5 type=result cmd=registers.write op=0 state=terminal primary=MDIO_ERROR detail=-43 cleanup=none cleanup_detail=0 time_valid=1 time_us=9600 deadline_valid=1 deadline_us=2009500 reason=none effect=possible batch_valid=1 batch_violation=0 release=OK release_detail=0
```

For example, a successful session after boot 42 has these bytes followed by
CRLF:

```text
@DP8382X schema=1 boot=42 req=1 type=session cmd=session op=0 state=terminal primary=OK detail=0 cleanup=none cleanup_detail=0 time_valid=1 time_us=7600 deadline_valid=0 deadline_us=0 reason=none nonce=host-87 library=0.1.0 firmware=unknown tree=unknown sdk=unknown target=host backend=sim-mdio clock=simulated variant=DP83826I interface=RMII address=1 console=host-stdio baud=0
```

An invalid line at request 2 produces:

```text
@DP8382X schema=1 boot=42 req=2 type=result cmd=unknown op=0 state=terminal primary=INVALID_PARAM detail=0 cleanup=none cleanup_detail=0 time_valid=1 time_us=8700 deadline_valid=0 deadline_us=0 reason=byte
```

## Session handshake

Send `session <new-nonce>` and require its exact echoed nonce, a valid session
record, the expected identity and schema. The reply's boot and request number
seed the host counter. The nonce is echoed verbatim and is not stored as
processor state. Every other record correlates by boot/request and, for future
operations, by token. A record with another boot or request belongs to older
or unrelated work. A new boot requires a new synchronization before sending an
ordinary command.

Use one outstanding command. Increment the expected request for each complete
line sent, including lines the processor rejects. After a timeout send only
`session <new-nonce>` to resynchronize on an intact stream. Without a valid
matching reply, stop sending commands. A partial command write can leave the
input framing uncertain and needs a newly verified connection. Session queries
do not reconfigure the processor or discard pending work.

The [host fixture](../test/cli/README.md) produces software transcripts from the
real processor and core. Its fault controls are confined to the executable.
The [console runner](hil-runner.md) verifies those transcripts and uses this
handshake for serial attachment to an already-running console. The
[campaigns](hardware-validation.md) add stopped-owner preflight, full settings
snapshots, timed live observations and independently correlated cleanup.
Both paths preserve paired raw and JSON evidence.

Feature campaign correlation follows the command tables. Typed cached
queries are unscheduled. Scheduled READ uses up to 2 s and 64 frames.
Scheduled WRITE, START, OBSERVE and RESET use up to 5 s and 512 frames.
Cooperative success requires an admitted token and a matching terminal;
direct reads and writes terminate with `op=0`. A rejected command may
terminate before admission. A terminal failure can retain valid partial
words, possibly changed settings or cleanup debt. Keep those fields with
the original request and correlate an owning stop or replay separately.
The [fixture cases](hardware-validation.md#feature-fixtures) define the
independent readback, stimulus and measurement needed per family.
