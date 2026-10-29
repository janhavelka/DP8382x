# Portable core and native SDK ownership decisions

Accepted design, 2026-10-29, prompt 6. This selects the architecture; it does
not claim an implemented driver, supported SDK build or hardware result.
The [architecture](architecture.md) gives the layer overview, the
[execution report](reports/implementation/6-framework-ownership.md) records
the audit, and the [silicon ledger](registers/dp83826i.json) owns chip facts.
Prompts 7–8 freeze public signatures and value layouts without reopening these
ownership choices unless new evidence requires a coordinated change.

## 1. One chip implementation and one batch boundary

Use the allocation-free C++17 `DP8382x::DP8382x` core in both modes. It owns
identity, register policy, chip procedures, persistent intent, observations,
trust, operation results and the single destructive-evidence collector. The
application serializes **all** access to an instance, including cached copies.
The native adapter translates `esp_eth_phy_t` calls to those same procedures;
it does not maintain another chip configuration or polling state machine.

Select **one synchronous terminal batch callback**, borrowed from the owner.
Do not also expose a begin/end transaction-scope API or independent public frame
callbacks. Each batch contains a bounded fixed array of C22 read/write steps.
A write can use a preceding read slot with an explicit preserve mask and literal
set bits, so an RMW does not return to the caller between its read and write.
The core constructs masks, command-neutral values and order from chip policy;
the backend only evaluates that fixed data dependency and executes frames.
Batch descriptors contain no branches, loops or user callback opcodes; backend
iteration is bounded by their fixed count. Chip-specific decisions stay in core.
Prompts 7/12 must size the array for an eight-frame, explicitly reselected MMD
read/modify/write. Simple C22 uses one frame, C22 RMW two, MMD read/write four.

The application callback acquires one controller-wide lock, executes the whole
batch and releases it on every exit. Every other PHY/backend user on that bus
must share that lock and the controller fault state. A per-PHY or per-frame lock
is insufficient. An uncoordinated external debugger or SDK base driver is outside
the supported concurrent configuration; stop and isolate it before use.

The board/controller coordinator also owns a **bus-silence admission interval**.
Before external power-up/hardware-reset transitions requiring SMI silence it
closes batch admission for every participant, drains any active batch, then
performs the transition. After actual reset release
or power-up, admission stays closed until the applicable readiness/clock-qualified
deadline. Release the controller mutex while waiting, but admit no other PHY's
frames: SNLS647I 8.3.11 requires the shared SMI bus to remain inactive. Unknown
release/power timing keeps the bus closed until the owner establishes readiness.
Check/update this admission flag under the controller lock; closing it drains
the prior batch without acquiring any other instance gate. It is separate from
the quarantine flag for unknown controller completion/release.

Stop at the first failed step. Record attempted/completed frames, completed read
slots, the failed index, possible write effects, possibly consumed unreturned
reads, and release status. Keep completed destructive samples in those slots
even on later failure; merge them into the core collector before releasing
instance serialization. Ordinary read outputs publish only on complete success.
An error never implies that the failed frame had no effect. Reestablish MMD
selectors at the next admitted transaction; no speculative selector cleanup is
needed. An uncertain lock release or still-running controller command quarantines
the shared controller for **all** participants until owner recovery proves it
idle and released. Passive health is not this admission interlock.

The callback must return terminally; `IN_PROGRESS` is invalid at this boundary.
No retained caller buffers, asynchronous completion, retries or reentry into the
driver are allowed. Allocation, mutexes, RTOS, logging and clocks stay outside
the core. Pure batch data/transform definitions belong with the core transport
contract; a backend must not duplicate silicon register logic.

## 2. SDK baseline and qualified base reuse

Select ESP-IDF v6.1, commit `fff9895c82d744c7237be8847347bdd1b07c6643`, as
the source baseline for ESP32-P4. The seven source identities in
[library-sdk](reference/library-sdk.md) were rechecked against downloaded bytes.
This unmodified revision has lifecycle defects described below; a verified
correction is required before claiming native support. Do not silently move to
another SDK or label the baseline a tested support range.

The adapter owns one `phy_802_3_t` wrapper, one core instance and native-only
owner context. Use a standard-layout first-member wrapper with compile-time
layout checks to recover the owned context from its `esp_eth_phy_t`; do not cast
the private Ethernet driver layout. Keep borrowed MAC, controller and owner
resources alive until callback quiescence and acknowledged results.

| Pinned base surface | Selected use and reason |
| --- | --- |
| `esp_eth_phy_802_3_obj_config_init` | Local base object setup only; overwrite the entire callback table before publication. Validate declared address/configuration first. No inherited I/O callback remains reachable. |
| `set_mediator`, `get_addr`, `set_addr` | Reuse their local storage behavior inside owner guards. Address changes only while stopped, uninitialized and quiescent; validate 0–31, update the core binding coherently, invalidate old identity. No implicit address scan. |
| `reset`, `reset_hw`, `init`, `deinit`, `pwrctl` | Override. Generic waits, reset domains, power behavior and implicit scan/reset do not implement the chip contract. Deinit is local after explicit cleanup, never an implicit power-down. |
| `autonego_ctrl`, `set_speed`, `set_duplex`, `advertise_pause_ability`, `loopback` | Override through shared configuration/diagnostic procedures. Generic BMCR/ANAR RMW lacks the selected coupling, access policy and complete batch exclusion. |
| `get_link`, `set_link`, `updt_link_dup_spd` | Override. One chip collector and a separate notification journal handle latch semantics, same-link changes and partial publication. Base link cache is not independent evidence. |
| OUI/model/revision helpers, `basic_phy_init/deinit`, address detection | Do not use their MDIO procedures. Shared identity performs both checked reads and exact-part validation. Generic search ignores read status and can attempt 96 reads. |
| All `get/set_mmd_addr`, `read/write_mmd_data/register` helpers | Do not use. Partial bitfield initialization, 32-bit values, separate frames and weak function validation conflict with the contract. |
| `del`, `custom_ioctl` | Owned deletion checks quiescence/results; never use generic `free` on the composed object. Set optional `custom_ioctl` null; typed owner APIs provide chip extensions without invented SDK commands. |

MMD control is constructed as a complete numeric word, including reserved
`[13:5] = 0` and upper 16 bits zero. Validate external integer widths **before**
narrowing: PHY/register address, DEVAD, function, 16-bit offset and data. The
accepted DP83826I extended procedure uses DEVAD 31 and literal `0x001F`, then
`0x401F`, with a 16-bit ADDAR value. Other namespaces/functions require their own
ledger evidence; generic 5-bit DEVAD validity is not silicon authorization.
The [poison probe](../test/sdk_mmd_poison.c) tests the pinned SDK types after
nonzero initialization and independently checks complete-word construction.
It demonstrates source/encoding behavior, not an executing DP driver.

## 3. Admission, call chains and lock order

The application supplies an instance gate, a controller batch callback and a
monotonic clock. Native code additionally owns a control reservation recording
operation, generation, calling task and allowed synchronous SDK callback kinds.
Validate the expected phase and one active callback at a time; task identity alone
never authorizes nested mutation or reentry. Unexpected callback nesting rejects.
The reservation is logical admission across calls, not a held mutex. There is
one cooperative core operation/result slot. A pending result is never overwritten
by timer polling, reset or a later command.

Each native synchronous wrapper records the exact token it creates. It drives
that same cooperative procedure, releasing the instance gate and controller lock
between waits, then consumes only its own token into the enclosing native
operation journal before a subsequent SDK callback can use the slot. The
journal preserves substep results until application delivery/acknowledgement.
An application-owned token/result makes such native work busy; wrappers never
consume it. Direct short reads and timer collection do not allocate a second
operation slot.

The lock order is **instance gate → controller lock → controller internals**.
Never acquire another instance gate while holding the controller lock. Never
enter an SDK control API, emit a mediator notification, wait for callback drain,
sleep for hardware, or print while holding the controller lock. Instance access
may span a short terminal batch but cannot remain locked across an SDK call that
synchronously reenters a PHY wrapper.

For native lifecycle/control: acquire the instance gate, validate and reserve,
release the gate, call the SDK, let each synchronous wrapper acquire the gate
and validate the reservation/task, then reacquire to record the SDK result.
The outer caller does not access core memory while the gate is released.
Unsolicited direct PHY mutation callbacks fail admission before MDIO. Timer
`get_link` uses nonblocking admission and returns busy during incompatible work;
its SDK caller ignores errors, so the adapter retains the error for the owner.
No task waits for the owner task to process a request from a synchronous SDK
callback. This avoids an owner→SDK→owner queue deadlock.

Notifications use a reserved immutable projection: finish acquisition and record
it under the gate, release the gate and controller lock, call the mediator, then
reacquire the gate to journal success/failure. Competing mutations remain denied
by the logical reservation; snapshots can copy the last published state. Mediator
and event handlers must not synchronously submit a mutation and wait for it.
Events request later work. Callback counters cover executing adapter work, but
SDK timer drain must also fence callbacks that have not yet entered the adapter.

```text
Portable application owner (serialized, supplies time)
  -> DP8382x public method -> shared chip procedure -> terminal batch
  -> application controller lock -> MCU MDIO frames -> release
  -> retain samples/effects and publish result -> owner schedules next poll

Native owner control -> reservation -> esp_eth_* (no instance gate held)
  -> owned esp_eth_phy_t callback -> instance gate -> same core procedure
  -> terminal batch -> shared controller lock -> actual MAC read/write_phy_reg
  -> release controller -> retain evidence -> release instance gate
  -> mediator on_state_changed (if needed) -> journal result

SDK link timer -> owned get_link -> same admission/collector/projection
CLI / IRQ request -> application owner -> typed operation / same collector
```

Only the framework timer polls link in RUNNING native mode. The application
copies cached link state; it does not run a second link poller. While STOPPED or
in maintenance, the application may explicitly collect link through the same
procedure. IRQ requests and link polling share one collector, not separate
destructive readers. The initial board has no PHY IRQ connection.

### Frame hooks and handle context

Install both public `esp_eth_config_t::read_phy_reg`/`write_phy_reg` hooks.
Reject **all external raw mediator/ioctl frames**, even apparently safe ones;
the supported register entry is the typed owner API below. There is no hidden
thread-local authorization flag that turns several raw calls into an MMD batch.
Raw `ETH_CMD_READ_PHY_REG`/`WRITE_PHY_REG` get the address then call the mediator,
bypassing PHY register methods; the hooks are the final rejection point.

Resolve each incoming handle with `esp_eth_get_phy_instance` and
`esp_eth_get_mac_instance`, verify the owned callback identity before recovering
the adapter wrapper, and compare the actual MAC with its borrowed transport.
The pinned install assigns both pointers before set-mediator/reset/init and
before publishing `*out_hdl`. Therefore context comes from the **incoming**
handle, never a global singleton or the not-yet-returned application handle.
Foreign/mismatched instances reject before transport. The approved core batch
calls this actual MAC's `read_phy_reg`/`write_phy_reg` directly; it never returns
through the mediator, raw ioctl or another lock-acquiring hook.

The actual P4 MAC's init/control code does not require mediator PHY-register
frames. All selected base I/O is overridden. Thus rejecting those frame hooks
does not remove a required internal transport route. A newly selected MAC/SDK
must re-audit this assumption. Applications keep SDK handles and MAC pointers
inside their native owner; other clients receive request interfaces and copied
state. Direct SDK controls and direct MAC MDIO are unsupported bypasses.

## 4. Complete responsibility matrix

`P` is the serialized portable application. `N` is the native reservation and
instance gate above. `B` is the single controller-wide terminal batch. `T` is
the application monotonic time, sampled by native code with `esp_timer_get_time`
and supplied to the same core procedure. Local cached calls do not invoke T.
All I/O rows use B, terminal bounds and retained failed-step evidence. All
native rows use N, including getters. These are planned public entry families,
not fabricated implemented signatures or SDK ioctls.

| Operation / public entry | Portable chain and owner | Native chain / base or mediator method | Time, state effects and result collector |
| --- | --- | --- | --- |
| Construction, `bind`, `end`, factory, `del` | P validates replacement before changing borrowed binding. No bus I/O. Refuse active work, owed cleanup or unacknowledged result. | Owner constructs MAC/adapter before install; guarded base object configuration, then owned `del` after successful uninstall/drain. | No clock for local core calls. Binding generation and retained results stay core-owned; allocation/resource outcomes stay native-owner-owned. |
| Install/init and identity, `startInitialize` | P → shared identity, initialization and replay; exact declared part/address and checked ID1/ID2. Explicit discovery is separate bounded read-only work. | `esp_eth_driver_install` → guarded mediator/reset hooks → MAC init → owned PHY `init` → shared procedure; `autonego_ctrl(G_STAT)` reads verified state. No base implicit scan/power/reset. | T; board readiness prerequisite, finite frames/polls. Core identity/config/result collector; native resource journal for earlier SDK effects. |
| PCS reset / hard reset / circuitry restart, `startReset` | P → exact ledger reset procedure and finite replay; invalidate affected observations without discarding consumed samples. | Stopped owner → owned `reset` (PCS) or typed reset route → same procedure. Reconcile SDK settings before next start. | T; reset completion, command issue/effect and cleanup recorded separately. No borrowed base 10 ms polling loop. |
| External reset/POR, `invalidateDeviceState`, `reset_hw` | P owns GPIO/power/clock and records actual release/readiness; core invalidates then initializes. | Owned `reset_hw` invokes application board action under install/maintenance reservation; no inherited GPIO 5. Failure precedes MAC init in corrected SDK. | Board high-resolution time plus T. Core trust/link invalidation; native board receipt/partial reset outcome. No MDIO before readiness. |
| Power, `startConfigure` / typed power operation | P → shared IEEE power/down/deep-power/active-sleep procedure with exact feature gates and cleanup. | Owned `pwrctl` maps Boolean only to documented IEEE power; other modes use typed owner entry while stopped and clocks coordinated. | T, separate requested policy/observed mode/trust. No implicit power action in local end/deinit; result includes uncertain wake/clock effects. |
| Persistent settings / advertisement / next pages / crossover | P → shared validated configuration and finite replay; temporary profiles excluded. | Stopped owner composes transaction: PHY-owned fields through shared procedure; SDK-mirrored fields through public stopped controls below. No full-advertisement ioctl exists. | T; desired vs observed stable bits vs trust; core operation result plus native SDK reconciliation journal. Source-gated branches reject before mutation. |
| AN enable/disable/restart/status | P → shared AN procedures; restart acceptance is distinct from completed negotiation. | Owner → `ETH_CMD_S_AUTONEGO` → owned `autonego_ctrl(EN/DIS)`; `G_STAT` for install; start's `RESTART` uses shared bounded initiation. `ETH_CMD_G_AUTONEGO` checks SDK cache agreement. | T; publish callback Boolean only on verified success, leave output unchanged on failure. Cache disagreement/uncertain write blocks restart; core keeps effects even on SDK error. |
| Forced speed/duplex | P → coupled forced-mode procedure, including crossover and 10BASE-Te prerequisites. | Stopped owner first disables AN publicly, then `ETH_CMD_S_SPEED` and `ETH_CMD_S_DUPLEX_MODE` → owned callbacks using same coupled procedure. Entire setting tuple validated before first mutation; partial steps retained. | T; configured mode is not resolved PHYSTS evidence. Getter cache may reflect last notification; identify it as such, never as hardware verification. |
| Pause advertisement and MAC policy | P → complete four-combination advertisement and full RX/TX resolution. | Owned `advertise_pause_ability` accepts only 0/1 for SDK Boolean compatibility (clear both/set both); typed owner API handles exact bits. MAC policy calls actual `enable_flow_ctrl(bool)` with its own result. Do not call defective `ETH_CMD_S_FLOW_CTRL`. | T for PHY I/O; retain both MAC and PHY partial effects. Portable resolved pause and SDK peer Boolean projection are distinct values. |
| Live link / cached link / notifications | P → `readLinkStatus` → single latch/event collector; `linkSnapshot` copies only. | Timer → owned `get_link` → same collector → `on_state_changed(SPEED, DUPLEX, PAUSE, LINK)` journal. Owned `set_link(DOWN)` performs SDK stop projection only, not a PHY status read; unsolicited UP rejected. | T for live read. Timestamp/validity, raw sample history, full resolution in core; projection steps and failures in adapter. No notification while B held. |
| Interrupt enables/service and destructive counters/events | P → shared typed configure/service → one collector; ISR only signals P. | N admits owner request, same collector as `get_link`; no SDK interrupt reader or raw read-clear route. | T, each consumed partial sample retained; failed read may lose unreturned history. IRQ enable mutation and cleanup use operation result. Cached consumers never clear hardware. |
| Loopback, BIST, cable, electrical stimulus | P admits fixture/prerequisites → shared temporary profile/test/stop/restore. | Stopped N → same operation; owned `loopback` maps to exact supported procedure, not bare BMCR bit change. No second generic loopback implementation. | T, primary and separate finite cleanup budget; core retains samples, temporary effects, pending restore and persistent intent. Native owner coordinates MAC stop. |
| EEE/LPI/WoL, LED/GPIO, interface/clock/repeater, quality/compatibility | P → typed shared feature procedures and ledger gates. | Stopped N for disruptive settings, same procedures; board/network coordination external. No imaginary SDK controls. | T where I/O/waits occur; feature collector and configuration trust. Reset-bearing EEE replay is finite and nonrecursive; unresolved gates remain explicit. |
| Raw C22/MMD and CLI register commands | P → shared access-policy checks → B. Selectors only internal; destructive addresses only via collector. Authorized raw mutations require maintenance and invalidate affected trust. | CLI submits typed N request → same core access path → actual MAC. Raw ioctls always rejected by hooks. `custom_ioctl` absent. | T for access; width/namespace errors cause zero I/O. Raw result, all-bus statistics and mutation uncertainty retained separately from managed health. |
| Health/config/operation snapshots, `takeResult`, `cancel` | P copies/consumes exact token; cancellation is local and schedules bounded cleanup through later owner polls. | N copies core evidence for application publication; no SDK or timer read on snapshot. SDK callbacks do not steal application tokens/results. | No clock or I/O for snapshots/cancel/result retrieval. Core owns terminal evidence until explicit take/acknowledgement. |
| Start/stop/uninstall and callback drain | Portable owner stops its own scheduling, finishes/acknowledges cleanup, then local end. | Native owner reserves, calls corrected SDK without gates held; timer drain, application/stack references and callbacks complete before deinit/uninstall/del. | T and finite SDK cleanup deadline. Native resource journal owns FSM/timer/notifications/handles; core result survives stop/failure. Failed cleanup retains resources. |
| MAC/DMA frames, IP/EtherCAT, board supply/clock and recovery policy | Application/backend owns these; no core entry. | Existing MAC/framework and application stack own frame transmission/reception. Owner coordinates MAC settings, netif/event references and frame drain. | Application deadlines/readiness and native resource journal; neither PHY health nor link proves application timing or safety. |

## 5. SDK cache, notifications and raw reconciliation

`esp_eth.c` stores `auto_nego_en`; its getter is cached. `ETH_CMD_S_AUTONEGO`
requires STOP and passes the cache's address to the PHY callback. The callback
must stage its output and write it only after the shared operation succeeds.
Forced speed/duplex controls require cached AN disabled. No direct core write
may quietly bypass those controls in native managed configuration.

For AN→forced: stop/drain, validate the complete tuple, apply prerequisites and
PHY-owned fields, use public AN-disable, then public speed/duplex controls, read
back stable owned bits, check `ETH_CMD_G_AUTONEGO`, and only then allow restart.
For forced→AN: prepare advertisement/crossover/10BASE-Te dependencies, use public
AN-enable, verify stable state/cache agreement, then start. The driver start
path also works when cached AN is false; its owned `get_link` must enforce
readiness even though no `autonego_ctrl(RESTART)` callback occurs.

An accepted-then-failed BMCR write, external reset, or authorized raw mutation
marks configuration and SDK agreement dirty. While stopped, re-identify if
needed, replay desired settings through their respective public owners and
verify hardware plus `G_AUTONEGO` before restart. If a public reconciliation
step fails, remain stopped with stale/unknown evidence; do not patch private
caches. Raw writes never silently become persistent desired settings.

Link acquisition uses the ledger's latch/clear owners and resolves speed/duplex
only from sufficient evidence. `forced_physts` remains an implementation gate
for unresolved forced-mode observation: configured mode may be reported as
configured, but cannot generate a fabricated resolved UP notification. AN
observation can proceed independently when its evidence is sufficient.

Retain the complete portable pause result. The native `ETH_STATE_PAUSE` value
is the pinned framework's narrower full-duplex/partner-symmetric-pause Boolean;
it cannot encode all resolved RX/TX combinations. Publish that projection with
its meaning, and keep MAC/application flow-control policy external. Do not
promise that the SDK automatically implements every portable asymmetric result.

On an UP projection, send speed, duplex, peer-pause, then link, recording each
accepted step. If a later callback fails, retain earlier effects and leave the
projection incomplete; never report atomic rollback. On a changed tuple while
link remains up, project DOWN first, update the tuple, then UP so MAC settings
are changed while stopped. An unchanged verified projection emits nothing.
Unknown observation does not invent down/up; mark stale and let application
policy choose stop/recovery. Stop's explicit DOWN is an administrative event,
distinct from physically observed cable-down.

The **failing notification itself** may already have changed MAC/SDK state.
Pinned LINK handling calls MAC `set_link`, changes the cached link and then
posts the event; a failed event post can follow a successful MAC start. Repeating
UP can reinitialize DMA descriptor chains. On any notification failure, mark
publication uncertain and freeze ordinary notification replay, including timer
UP retries. The owner must stop/drain and reconcile through the corrected SDK
before publishing another UP. Preserve event-delivery uncertainty separately
from physical observation and accepted/uncertain MAC effects. Acceptance includes
event-post failure after MAC start, not only failures before a callback acts.

## 6. Lifecycle correction selected for native support

Unmodified v6.1 cannot meet the complete failure contract. Install ignores
mediator/reset errors and frees the driver on failure without MAC/PHY unwind;
uninstall consumes its reference count before fallible teardown. Nonblocking
timer stop/delete cannot prove callback quiescence. The MAC additionally drops
some stop/deinit errors. An adapter counter cannot fence a timer callback that
already captured the driver but has not entered the adapter.

**Selected route:** prompt 37 must carry or select a narrowly scoped, reviewed
SDK lifecycle correction against this baseline, pin its actual source identity
and execute failure regressions before enabling native support. This is a
required dependency, not an alternative wrapper-only implementation. No such
patched revision is claimed to exist yet. Corrected behavior must provide:

1. Checked mediator/reset returns before MAC init, with a per-step installation
   journal. Treat failed init as potentially partial, not merely uninitialized.
2. Finite unwind while the driver/mediator remains alive. Extend the supported
   install failure contract to return a non-null **cleanup-only handle** when
   unwind is incomplete; null means all internal resources were released. The
   owner preserves the primary install error separately from cleanup status.
   This is a proposed SDK correction, not an API in unmodified v6.1.
3. A stopped/closing barrier that disables new timer callbacks and uses finite
   blocking timer quiescence outside callback/instance/controller locks. It must
   work after start fails before the timer was armed and from a non-timer task.
   A timeout retains the handle and resources, with admission closed.
4. Start/stop result records for FSM changes, event publication, timer arming
   and MAC link effects. Stop continues required cleanup after an earlier error;
   the first failure survives. No success merely because FSM already says STOP.
5. Retryable step-aware uninstall: reject live external references before
   teardown, drain callbacks, and retain resources/journal on failure. Do not
   retry an already-deleted timer or deinitialize freed resources. After all
   callbacks/references are gone and cleanup completed, detach borrowed mediator
   pointers while the driver is alive; then free the driver and permit PHY/MAC
   deletion. The correction must provide supported local MAC
   detach semantics after quiescence; the current MAC `set_mediator` rejects
   null. The owned PHY wrapper can clear its own local base storage without
   calling the base setter, which also rejects null. No existing null-setter
   contract is assumed.
6. Propagated MAC start/stop/deinit and HAL FIFO-flush errors so successful
   teardown actually establishes its promised state. Hardware effects can still
   be partial; failure does not license freeing live DMA resources.

Applications use the owner wrapper for lifecycle and controls; they do not call
SDK lifecycle directly while an owner operation is active. Incomplete corrected
SDK cleanup is quarantined and retried only through its supported cleanup path.
If this correction cannot be supplied, native implementation/support remains
BLOCKED; portable work remains eligible. Do not bypass the gate with successful
stubs, private field edits, an arbitrary delay, or unverified repeated uninstall.

### Failure and two-instance walkthrough

| Boundary | Required disposition |
| --- | --- |
| A and B construct, no public handle yet | Each owns distinct PHY wrapper/core/MAC. Their buses may share one controller lock; no shared singleton identity or result slot. |
| Install A or B invokes a frame hook | The incoming handle resolves its assigned PHY/MAC via public getters before output publication. Hook denies raw I/O; any approved core operation uses that instance's stored actual MAC directly. |
| Mediator/reset setup fails | Corrected SDK stops before MAC init; retain reset partial effects and finite unwind results. Borrowed context survives until cleanup completes. |
| MAC init or later PHY init fails | Corrected SDK unwinds partial steps while mediator remains valid; failed cleanup publishes cleanup-only handle. Other instance stays independent unless the shared controller is quarantined. |
| Stop races an active/queued link callback | Close admission, drain SDK timer including pre-wrapper callback window without held gates, then drain adapter/stack work. A counter alone or STOP flag is insufficient. |
| Delete with active work/result or failed SDK cleanup | Reject local deletion; retain core result and native handles. Only explicit result delivery/acknowledgement and proved resource release permit delete. |

These are source-reviewed call chains and implementation acceptance cases.
Prompt 37 must execute them against the actual corrected SDK; a host dispatch
model cannot substitute for that firmware/lifecycle evidence.

The P4 on-chip MAC factory resets EMAC module 0 and its deletion disables that
module's clock/GPIO. Use **one** physical P4 EMAC owner; two native P4 factory
objects are not independent hardware instances. Two-handle dispatch/lifecycle
tests use two identified instrumented MAC instances in the actual SDK fixture
(software lifecycle evidence only). Test another PHY sharing the physical MDIO
controller as a separate batch-participant case, not by attaching the same MAC
object to two Ethernet drivers. A real second MAC backend needs its own source,
link and physical qualification.

## 7. Bounds, board readiness and competing operations

For n frames, admit an entire batch only when remaining time/work can cover

`B(n) = A_instance + A_controller + sum(F_i + gap_i) + R_controller + P_result`.

`F_i` includes controller launch, waiting, validation and scheduler/instruction
overhead; release and evidence publication have allowances too. `A_instance`
is charged by the owner before entering core. A failed lock acquisition performs
no frames. Do not multiply a per-frame timeout after each step to extend the
original deadline. Reserve finite cleanup separately. A full operation also
includes admitted hardware waits, poll intervals, scheduling, notification work
and cleanup; synchronous native wrappers use the same procedure and budget,
never another reset/configuration loop. They release B between polls and waits.

The P4 MAC has finite 100 us busy-loop steps with a nominal 1000 us frame polling
allowance, no passed caller deadline and no MDIO mutex in these methods. Its
100 ms MAC multi-register mutex is not an MDIO transaction lock. Four frames
therefore have a nominal 4000 us polling contribution, **not a proven 4 ms
wall-clock bound**. The unmodified backend supplies no unconditional scheduler
latency maximum or software preemption. A late callback is reported late with
its effects; a controller timeout may leave hardware busy and quarantines B.
Implementation must establish platform allowances or explicitly report the
backend's unproved wall-clock bound. Do not claim hard real-time EtherCAT timing.

Use a finite frame/poll count even when the clock freezes. Anchor post-action
minimum waits after observed completion, or conservatively at the next owner
timestamp. Native elapsed-time rechecks use `esp_timer_get_time`; RTOS sleep
rounding alone is insufficient. No controller lock is held during settling or
output. Nested notification/MAC internal waits are outside B and included in
the operation's native bound; missing SDK/backend bounds remain visible.

The current [board snapshot](esp32p4-board.md) was re-read at hardware-repository
commit `6f8333ef04caa785be374478d0cd413b8e055ed1`: reset GPIO54, MDC49,
MDIO43, PHY-supplied 50 MHz on GPIO44, expected PHY address 0 and no connected
PHY IRQ. GPIO5/auto-detect are not valid inherited choices. Existing R21/C24
reset circuitry means the datasheet's 25 us minimum **without debounce caps**
does not by itself qualify the board pulse. Board firmware establishes actual
pin release and clock stability; this task did not validate the netlist or pulse.

SNLS647I 7.6/8.3.11 require SMI bus silence for 2 ms after reset release and
50 ms after power-up, under their clock conditions; table values are readiness
maxima, enforced as minimum waiting windows. If external clock arrives late,
reset follows POR and at least 100 us stable clock. Base default 100 us assertion,
no post-reset delay, 100 ms reset timeout, 4000 ms AN timeout and 2000 ms link
poll are SDK policies. At a 100 Hz RTOS tick, truncating 2 ms to zero ticks
violates readiness; round conservatively and recheck elapsed time. The first
board's poll period is explicitly configured application policy, not EtherCAT
timing. Validate resulting MDC frequency and actual management-frame gap/preamble
behavior; the MAC's Ethernet frame preamble setting proves neither.

| Competing operations | Admission, invalidation and cleanup owner |
| --- | --- |
| Live status against MMD/RMW, including another PHY | Instance gate serializes same-instance work; shared B excludes all frames of each batch across instances. No controller→instance inversion. Each acquired batch releases on failure. |
| Reset against retained diagnostic/event evidence | P/N admits only after prior result delivery/acknowledgement and needed cleanup. Invalidation marks old samples stale but preserves consumed history and possible loss epoch. Reset is not an evidence clear command. |
| External power-up/hardware-reset readiness against another PHY's frames | Board/controller coordinator closes all batch admission before transition and drains active B; holds logical bus-silence admission through the actual-release/clock-qualified deadline, without holding the controller mutex during the wait. MDIO-issued reset/power commands retain their distinct ledger procedures. |
| Accepted write then timeout/cancel | Core retains attempted effects and loses affected trust; owner admits separate finite cleanup. Cancellation cannot undo a frame. Shared controller busy/unknown prevents any participant's next batch. |
| Raw mutation against SDK polling/cache | Maintenance stop/drain precedes admitted raw mutation. Core invalidates; owner publicly reconciles SDK fields/readback before restart. External raw ioctls reject at hooks even while a reservation exists. |
| IRQ against link/event/counter acquisition | ISR only queues bounded indication; the same collector combines requests. Consumers copy retained samples; no destructive raw dump. |
| Stop/delete against callbacks/notifications | Native closing barrier blocks new work; corrected SDK drains before freeing driver; owner drains stack/adapter work without their locks held and preserves incomplete cleanup handles/results. |

## 8. Source selection, API documentation and acceptance

Portable public headers stay in `include/DP8382x/`, implementation in explicit
`src/` lists. Native files are selected separately from
`include/DP8382x/esp_idf/` and `src/esp_idf/`; only those and applications may
include SDK headers or use heap/tasks/platform clocks. Tests, examples, CLI,
reference archives and SDK patches are not portable production sources.
No sibling paths are runtime/build dependencies.

The plain C++ walkthrough is: construct two core instances, bind borrowed batch
contexts sharing a bus lock, explicitly initialize/configure each using owner
time, poll only active operations, take retained results, publish copied
snapshots, then quiesce/acknowledge/end. An ordinary polling wait on A holds no
bus lock, so B can run when no shared bus-silence interval is active. Power/reset
readiness waits block all bus participants through logical admission. No IDF
type, task, heap, global clock or FieldCore owner is needed.
This proves design reachability; actual standalone build/link belongs to the
source foundation and later consumer stages.

Public Doxygen must specify borrowed context/buffer/lock lifetime, serialized
cached access, no callback reentrancy, complete batch exclusion, terminal return
and release obligations, aggregate bounds and unproved backend limits. Document
output staging versus retained destructive samples, accepted/uncertain effects,
primary versus cleanup status, result acknowledgement, teardown rejection and
native callback quiescence. Include exact register namespace/units/access effects,
source gates and the narrower SDK pause/cache meaning. Source documentation is
introduced with real declarations in prompt 9, not placeholder success code.

| Acceptance work | Owning stage / evidence required |
| --- | --- |
| Poisoned MMD storage, exact 0x001F/0x401F and 16-bit values | Prompt 6 research probe; prompts 7/12 execute equivalent cases through production transport, invalid inputs and every failing frame. |
| Cross-entry/same-bus MMD and RMW, selector denial, raw ioctl contention | 11–13, 37–38 and 53; actual complete batches, second PHY, errors/release quarantine and actual SDK raw ioctl rejection. |
| Lifetime and two-instance hook dispatch, install/start/stop/uninstall failures | 37; corrected pinned source, real SDK link, callbacks before output publication, pre-wrapper timer race, partial MAC/HAL failures and cleanup retries. |
| Cache reconciliation and link/pause projection | 17–22, 38–39; cached AN true/false start, four pause advertisements, full portable resolution, narrower native projection, same-link mode changes and partial mediator failure. |
| Zero budgets, late writes, reset silence, coarse/frozen clocks | 8, 15–16, 37–39, 54; aggregate admission and conservative anchors; physical timing separately in 72–73. |
| Plain C++ and native source/include separation | 9/37/68–70; actual core and pinned SDK links, later clean installed/exported consumers. |

Source-gated PHY features remain attached to their existing ledger gates. Native
lifecycle corrections, physical reset/MDIO timing and the unresolved forced-mode
observation are explicit implementation/qualification limits; none is relabeled
as absent silicon or covered by a design review.
