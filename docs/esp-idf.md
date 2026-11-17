# Native ESP-IDF PHY owner

The native adapter in [Phy.h](../include/DP8382x/esp_idf/Phy.h) uses the existing
C++17 core through the public `esp_eth_phy_t` interface. Its baseline is
**ESP-IDF v6.1, commit `fff9895c82d744c7237be8847347bdd1b07c6643`, ESP32-P4**.
The component metadata selects that release and target. This is a selected
integration baseline, not a tested range of SDK releases or MACs. Executing
results and remaining gates belong to the [prompt39 report](reports/implementation/39-esp-idf-link-bounds.md);
the [source audit](reports/implementation/39-native-source-audit.md) records
the inspected inputs. A linked image does not establish physical operation.

## Component and application boundary

The root CMake component explicitly includes portable source files and
`src/esp_idf/Phy.cpp` and `PhyControl.cpp`. Ordinary host CMake includes only portable files and has
no ESP-IDF, Git, Python, sibling or Doxygen dependency. SDK headers stay in
`include/DP8382x/esp_idf/`. The wrapper embeds only
`struct PhyHandle { esp_eth_phy_t phy; Phy* owner; };`, with standard-layout and
first-member checks. It does not use the generic IEEE 802.3 PHY base or private
Ethernet driver storage.

The application creates the actual MAC and a nonrecursive controller mutex,
supplies a `SharedBus`, chooses exact part/interface/address and timing bounds,
and calls `Phy::create`. The factory replaces the core transport callbacks with
direct actual-MAC MDIO batches. It does not reset pins, probe the PHY or install
the framework. Failed creation leaves the output unchanged. The MAC, bus, mutex,
board callback context and all RX/stack contexts must outlive every callback,
including retained resources after a terminal failure.

One application owner serializes lifecycle/control entry. The instance gate also
serializes callbacks and snapshots; snapshots copy data without MDIO. Lock order
is instance gate, controller mutex, controller internals. Every participant on
the MDIO controller shares the same mutex, silence admission and quarantine.
A complete C22/MMD/RMW batch owns that mutex; no selector state or controller
lock survives a terminal batch. Notifications, SDK lifecycle calls, callback
draining, waits and output run without locks their callbacks need.

## Local entry and framework callbacks

The table describes admitted calls, rather than permission to bypass the owner
using the published SDK handle. Unexpected phase/nesting rejects before PHY I/O.
Native errors are retained without truncating the underlying `esp_err_t` detail.
Core typed calls preserve their `Status`, operation token and result contracts.
The SDK return cannot carry core error-detail categories, per-frame/release
receipts or consumed event prefixes. Inspect the lifecycle journal, operation
result, transport/evidence snapshots and acquired link snapshot for that detail;
SDK INVALID_STATE alone does not distinguish owner admission from lost proof.
MAC/event errors in the native lifecycle journal do not count as core MDIO health
transactions.

All 17 members of the pinned public vtable are accounted for below. Calls are
synchronous; input/output pointers are borrowed only until return. The installed
PHY, mediator and MAC have the retained lifetime described below. `L` means
bounded local work with zero requested gate wait; `B(n)`, `N` and `T` are the
conditional bounds defined in [Timing declarations](#timing-declarations).
The timing/error tables describe observed return classification; real-time
qualification remains conditional. An unsuccessful zero-wait instance acquisition returns INVALID_STATE, never
waits for another callback, and leaves ordinary outputs unchanged. Unsupported
entries do not acquire a bus lock. No row delegates to the IEEE base.

| PHY entry | Executing owner / complete transaction | Side effects and notifications | Error/output contract | Supported blocking bound |
| --- | --- | --- | --- | --- |
| `set_mediator` | Synchronous installer only; local borrowed pointer | Stores installer mediator before handle publication | OK during installation; otherwise INVALID_STATE; SDK ignores return | L |
| `reset_hw` | Installer; no transaction | No pin action: owner already completed board readiness | OK during installation, otherwise INVALID_STATE; SDK ignores return | L |
| `init` | Installer; no transaction | Local acknowledgement, no probe or reset | OK during installation, otherwise INVALID_STATE | L |
| `deinit` | Failed-install unwind; no transaction | Installed misuse closes admission permanently (uninstall may already have deleted SDK timer) | OK for failed-install local unwind; installed returns INVALID_STATE and retains objects | L |
| `autonego_ctrl(G_STAT)` | Installer or gated local query | Installer assigns initial true; later assigns desired mode only when valid | Null output INVALID_ARG; unknown desired/gate INVALID_STATE; ordinary failure preserves output | L |
| `autonego_ctrl(EN/DIS)` | Exact task/CONTROL reservation for public reconciliation | Acknowledges verified matching core mode into SDK cache; no write | Mismatch/unadmitted INVALID_STATE; output assigned only on OK | L |
| `autonego_ctrl(RESTART)` | Framework STARTING; shared core; one 3-frame BMCR batch | Command/readback; accepted/uncertain write retained; no AN wait | Core status retained internally, mapped native error; enabled assigned only on OK | L+B(3)+clock checks, within callback/start allowance under qualified bounds |
| `get_link` | Initial STARTING or RUNNING timer; core collector; batches of 8 and 2 frames | Acquired link evidence, then unlocked ordered projection; no invented mode or DOWN | Live/core error retained; downstream error/timeout preserves accepted fields; SDK receives narrower native error | L+B(8)+B(2)+up to5N+clock/local overhead |
| `set_link(DOWN)` | STOPPING owner/framework; no MDIO | Administrative DOWN only if last publication was UP | Unadmitted INVALID_STATE; accepted event loss separate from generic uncertainty | L+up toN, enclosed by stop deadline |
| `set_link(UP)` | No admitted unsolicited route | None | NOT_SUPPORTED | L |
| `reset` | Rejected; typed initialization owns resets | None | NOT_SUPPORTED | L |
| `pwrctl` | Rejected; no implicit power procedure | None | NOT_SUPPORTED | L |
| `set_addr` | Immutable declared address; no transaction | None | Identical address OK; any other NOT_SUPPORTED | L |
| `get_addr` | Immutable declared address; no transaction | Copies address | Null output INVALID_ARG, otherwise OK | L |
| `advertise_pause_ability` | Exact task/CONTROL PAUSE reservation; no transaction | Attests already verified AN bits0/1; rejection retains possible preceding MAC policy mutation | Matching0..3 OK, otherwise INVALID_STATE (including interference on busy gate) | L |
| `loopback` | Rejected; typed session owns stimulus | None | NOT_SUPPORTED | L |
| `set_speed` | Exact task/CONTROL SPEED reservation; no transaction | Acknowledges verified forced10/100; SDK link cache changes only on notification | Illegal enum INVALID_ARG; mismatch/unadmitted INVALID_STATE | L |
| `set_duplex` | Exact task/CONTROL DUPLEX reservation; no transaction | Acknowledges verified forced half/full; no link-cache publication | Illegal enum INVALID_ARG; mismatch/unadmitted INVALID_STATE | L |
| `custom_ioctl` | Null optional entry | None | SDK custom-command dispatch returns NOT_SUPPORTED | No PHY callback |
| `del` | Never-installed idle owner only; local core end and timer delete | Deletes adapter/gate/fence, never borrowed MAC/bus | Installed/active/fence-live INVALID_STATE; actual timer error retained by return; OK destroys pointer | L+T; no installed destruction bound is claimed |

Mandatory `read_phy_reg`/`write_phy_reg` config hooks are outside this vtable.
Both return NOT_SUPPORTED for every handle, including unpublished/null handles,
with zero frames and unchanged output. Owner typed access invokes actual MAC
primitives directly in complete C22/MMD/RMW batches. It cannot recurse through
mediator/ioctl hooks or acquire a second instance gate while holding the bus.

The owner exposes stopped `startInitialize` for `NONE`, `PCS`, `HARD` and `RESTART`,
legal AN/forced `startConfigure` (including ConfigureOptions), `pollOperation`,
`takeResult`, `cancel`, local snapshots and policy-checked raw/collector routes.
These routes reuse existing procedures and retained results. Initialization
always applies/verifies mandatory VOD settings; it can succeed with cable down.
Core operation cleanup remains a barrier and a separately budgeted restoration,
not implicit recovery. Extended full-duplex plus forced half-duplex rejects before
mutation because it cannot supply qualified native UP evidence. General 10/100
half/full modes remain available; the board's 100/full policy is external.

## Configuration, public reconciliation and raw entry

After successful core result delivery, call `reconcileSdkState(deadlineUs)` while
stopped. The shared core already applied/read back the complete profile, including
advertisement, routing and stable BMCR fields. Reconciliation calls public
`ETH_CMD_S_AUTONEGO`, reads `ETH_CMD_G_AUTONEGO` and, for forced mode, calls
`ETH_CMD_S_SPEED` and `ETH_CMD_S_DUPLEX_MODE`. Its scoped callbacks acknowledge
verified configuration without another PHY write or token. They reject another
caller/task or mismatched value. `SdkStateSnapshot` retains each accepted control,
AN-cache readback and aggregate status separately from the immutable core result.

Initialization/configuration admission and actual raw/external invalidation clear
SDK agreement. Effect-free invalid input preserves it. Start requires hardware
verification and public reconciliation; locking a raw write cannot certify the
SDK cache. Failed configuration requires the existing explicit restoration route.
SDK `G_SPEED`/`G_DUPLEX_MODE` remain last-published link caches, possibly defaults
or old data while stopped. Use `settingsSnapshot` for desired/observed settings.
Forced-mode UP publishes zero peer pause despite stale ANLPAR partner bits.

Stopped `writeClause22`, `writeMmd` and `modifyMmd` delegate core LEDCR-only
maintenance. Selectors, destructive reads, unsupported aliases and oversized
values reject before MAC frames. MMD control words are fully initialized
0x001F/0x401F in one four-frame batch (five for RMW).
`modifyBasicControl(mask,value,budget)` explicitly permits raw BMCR AN/speed/duplex
mask 0x3100 only, in one two-frame RMW. It neutralizes commands and never echoes
the power-pin OR. Accepted/uncertain mutation preserves desired intent but
withdraws identity/readiness/trust. Ordinary raw BMCR/MMD alias writes still reject.
Run `startInitialize({NONE})`, deliver its result and reconcile before restart.
Raw reset commands remain unavailable; typed reset uses initialization's lifecycle.

`acquireEvidence` is the sole native destructive-register route. Aggregate failure
leaves ordinary output unchanged; `evidenceSnapshot` preserves consumed prefixes,
and `transportSnapshot` retains frame/release uncertainty. Cached readers also
use the instance gate and never consume hardware evidence.

`setFlowControl(enabled,pauseAbility,deadlineUs)` calls actual MAC flow control
with a `bool`, then the PHY callback with a separate `uint32_t`. Ability bit0 is
symmetric pause and bit1 asymmetric; 0..3 must match already verified AN settings.
Change advertisement through `startConfigure` first. MAC/application policy stays
explicit. The owner never uses defective `ETH_CMD_S_FLOW_CTRL` or passes a small
Boolean as uint32 storage. `FlowControlRecord` separates MAC/PHY attempts, accepted
and possible effects, and their results. Invalid input has no MAC/MDIO effect;
MAC failure/late success or later PHY rejection may leave partial policy. Explicit
successful retry reconciles it; there is no automatic rollback.

An arbitrary flow ioctl can change MAC state before PHY rejection. An atomic
interference latch retains that uncertainty during another owner's callback
window or gate contention; enclosing success cannot overwrite it. Stop traffic
and explicitly reconcile. Unrelated MAC-only ioctls remain application-owned.
One terminal control receipt mailbox prevents post-call snapshot contention from
losing accepted effects or stranding CONTROL. The next gated entry adopts it;
this introduces no second operation slot or asynchronous SDK callback.

`discover(addressMask, output, budget)` is an explicit read-only owner diagnostic
while installed, stopped and free of pending operation/cleanup/temporary state.
Bit N selects address N; an empty mask rejects. The owner reserves two identity
frames per selected address before starting, for at most 64 frame calls across
all 32 addresses. Each read status qualifies its own word. Failure stops the
scan and leaves the ordinary output unchanged; `discoverySnapshot` copies the
retained per-address partial samples, attempted-frame count, terminal status and
last batch without MDIO. Raw ID words do not establish accepted part identity.
Discovery never selects or retargets the bound address. Normal installation uses
the explicitly declared address; there is no default search or fallback.

## Readiness and lifecycle

`prepareBoard` closes controller-wide admission and drains complete batches,
then releases the mutex before GPIO callbacks or hardware waits. The application
supplies real power-on and continuous-clock timestamps plus a finite reset pin
callback. Under the selected delayed-clock path, reset follows POR completion
and at least 100 us of stable clock. POR readiness is at least 50 ms after power
and SMI remains inactive at least 2 ms after reset release. Assertion is at least
25 us without debounce capacitors; the board's RC circuit requires its own
qualified pulse. These requirements come from SNLS647I section 7.6 and 8.3.11,
as reconciled in [initialization](initialization.md). Elapsed-time checks and
finite poll caps apply; a floor-rounded RTOS delay alone is insufficient.

Only after readiness does `install` call the public SDK installer. The fence
timer already exists. MAC initialization needs an existing PHY REF_CLK and polls
its reset in 10 ms steps, using `sw_reset_timeout_ms` (SDK default 100 ms).
The adapter's install callbacks do no PHY I/O. Installation failure publishes no
handle; only MAC and never-installed PHY deletion remain. After successful
installation, the owner runs typed initialization/configuration through direct
MAC batches, publicly reconciles SDK state, then admits `start` for verified normal settings.

Initialization reserves shared logical silence through result delivery, after a
zero-wait drain of the batch mutex. Only that owner executes batches during silence;
no mutex spans polls or waits. Successful `takeResult` reopens admission while
preserving any reset quiet floor. Cancellation does not erase the floor. Drain
release failure quarantines the bus. The application must keep polling and deliver
results; stopping scheduling does not provide bounded wall-clock completion.

Start publishes its terminal SDK outcome through a receipt adopted by the next
gated entry. The SDK arms its timer before returning, so a timer callback can
already own the gate; that contention must not strand STARTING or lose the
original start result. Failure still calls the existing stop/fence path.

The single admission phase is STOPPED, CONTROL, STARTING, RUNNING, STOPPING or
MAINTENANCE. Framework START changes its own state before all later steps can
succeed. Every start failure therefore attempts SDK stop and preserves the first
error separately from stop/fence/publication cleanup. Initial `get_link` proves
the SDK posted START before entering that callback. The unarmed-timer stop path
changes the framework state and returns INVALID_STATE without publishing STOP.

Direct framework start outside owner admission also rejects. Cached AN=true
changes SDK FSM before RESTART rejects; cached AN=false additionally posts START
before get_link rejects. Deferred evidence preserves these effects even when the
callback cannot acquire its gate. Owner stop/fence repairs them; it can return the
original unarmed-timer INVALID_STATE despite completed cleanup. Queued STOPPING
polls reject without inventing another start/STOP. Direct vtable get_link calls
are not an application route: STOPPED/CONTROL calls conservatively require repair.

After a stop, arm the precreated ESP_TIMER_TASK one-shot fence exactly once,
then task-delay/poll until inactive under a finite deadline, outside the timer
task. A retry polls the same fence; it does not rearm or claim an unobserved STOP
event. The serialized lifecycle owner records accepted STOP/arm receipts before
a fallible gate acquisition; cleanup errors are adopted through a terminal
receipt, so timer contention cannot duplicate those accepted actions. After the fence, publish missing administrative DOWN, post missing STOP
with the actual handle bytes and finite ticks, and call actual `mac->stop` to
observe the error hidden by pinned MAC `set_link`. MAC-stop attempts are finite
and run without the instance/controller lock.

`LifecycleRecord` retains stage, first error, cleanup error, last accepted tuple,
publication uncertainty and pending event-loss evidence. Per-field acceptance
flags preserve successful SPEED/DUPLEX/PAUSE updates if a later notification
fails. UP publishes speed,
duplex, peer pause, then link. A changed UP tuple first projects DOWN. The P4
factory identity attests that valid MAC `set_link` succeeds; a subsequent LINK
notification error therefore retains accepted MAC/cache publication and a lost
event. `eventLost` refers to the current accepted LINK event; a later successful
LINK supersedes that pending event, while historical errors remain retained.
`retryLinkEvent` retries the event alone with a finite deadline. It never
replays MAC UP or rebuilds DMA descriptors. An error on another MAC can leave
publication uncertain and closes further ordinary publication/start admission.
Owner stop still attempts the actual MAC stop, then `retire` retains resources;
reusable SDK/cache reconciliation after an unknown MAC effect is not supported.
A successful direct MAC stop alone does not prove the SDK cache or event state.

`linkSnapshot` copies the acquired core observation without I/O, including full
portable RX/TX pause evidence and its validity, staleness and refresh status.
It is independent of `lifecycleSnapshot`'s accepted SDK fields. One terminal
publication receipt preserves each accepted field even if snapshots contend
for the instance gate after the external callback. The next admitted entry
adopts it before reading state; no post-callback try-lock can strand admission.
An in-flight snapshot may still show the previous publication with uncertainty.

| Observation/publication outcome | Retained evidence and next action |
| --- | --- |
| Down, including initial cable absent | Valid acquired down; no speed/duplex invented; publish DOWN only after prior UP |
| Link up with incomplete negotiation/mode | Retain acquired unresolved evidence; return INVALID_STATE, publish no fabricated tuple |
| First resolved UP | SPEED, DUPLEX, PAUSE, UP; each field accepted separately |
| Speed, duplex or Boolean peer pause changes while up | DOWN, SPEED, DUPLEX, PAUSE, UP; latest acquired and accepted fields stay separate |
| Same fully published tuple | No notifications/events; a still-lost event needs explicit event retry |
| Notification returns OK late | Retain accepted field/transition; return TIMEOUT; known prefix remains safe. A later poll may reapply idempotent fields while down, but never replay an accepted unchanged UP |
| P4 LINK returns error | Record accepted link/cache and eventLost; stop on start failure, otherwise owner may retry only current event |
| Generic MAC/field notification error | Preserve accepted prefix and uncertain failing effect; stop and terminal retention |
| Event retry accepted after deadline | Clear eventLost, return TIMEOUT and retain error; never send it again as though unaccepted |
| Stop then restart | Administrative DOWN and fenced timer stop; next resolved UP republishes all fields into SDK caches |

The application must use owner lifecycle methods. Direct `esp_eth_start`,
`esp_eth_stop`, `esp_eth_driver_uninstall`, PHY deletion and mediator replacement
can cause SDK effects before a PHY callback rejects. In particular, uninstall
can delete its timer before reaching `deinit`; a rejected attempt does not
restore that timer or authorize a later start. Arbitrary control ioctls also
remain application-owned and can have partial MAC effects before PHY rejection.
The pinned flow-control ioctl has a bool/uint32 argument-width defect; do not
pass a small Boolean through it. Prompt 38 owns the correctly typed owner route.

## RX lifetime gate

Reusable installed teardown remains **BLOCKED by `native_rx_quiescence`**.
The pinned MAC RX task can load its mediator before entering the application
callback. Application RX count zero, STOP delivery and the timer fence do not
prove that task released the driver, glue or callback context. Mediator swapping
is unsynchronized. SDK uninstall frees driver storage before MAC deletion joins
the RX task; reversing those deletions leaves uninstall using a freed MAC.
No safe reusable public-API route has been established for this baseline.

`retire` closes admission and retains driver, glue, MAC, PHY, netif, controller
mutex and borrowed contexts until system restart. Stop TX callers, end the RX
session, and keep a nonblocking callback that consumes/frees late arrivals.
Initialization/start/stop failures after installation follow the same retained
route. Do not replace this gate with an arbitrary delay or a zero callback count.
The HAL FIFO-flush timeout hidden by the SDK remains a separate observation limit.

## Timing declarations

The exact supported SDK is v6.1 `fff9895c`, P4 with the actual RMII MAC in the
linked fixture. No other SDK range, RGMII timing or generic MAC wall bound is
qualified. `pinnedP4Mac` is the factory caller's attestation of that actual MAC,
not a result inferred from an error code. The host instrumented MAC tests the
other policy separately. Sources and line anchors are in the [fresh audit](reports/implementation/39-native-source-audit.md).

Define `B(n)=fixedUs+n*frameUs` for one complete n-frame batch, including its
controller acquisition, execution and release. `clockLagUs` bounds timestamp
uncertainty at sampling and is added by admission; it is not a scheduler budget.
The backend checks the whole batch before locking, checks again before each
frame, and releases on every return. C22 is B(1), MMD read/write B(4), MMD RMW
B(5), BMCR maintenance B(2), and AN restart B(3). Link acquisition has two batches
B(8)+B(2), so its bound includes two acquisitions/releases, not one ten-frame
lock. The shared mutex excludes every other PHY from each complete indirect or
RMW sequence. A lock failure issues no frame; uncertain controller completion or
release quarantines the shared domain. Passive health cannot reopen it.

`L` covers fixed local/gate/clock processing under the qualified workload;
instance acquisition requests zero ticks. `N=notificationUs` covers one entire
mediator call including actual MAC, logging, event posting and scheduling.
`T` covers the selected public timer-service call under that same workload.
These are conditions supplied/qualified by the application, not constants
proved by the SDK. Five N allowances are reserved before live acquisition;
callback/start deadlines remain absolute. A notification exceeding N or its
aggregate deadline returns TIMEOUT after retaining accepted effects. A late
failed callback keeps its original error. Nothing preempts a synchronous call.

| Actual source path | Finite count / requested wait | What the bound must also cover |
| --- | --- | --- |
| MAC read/write PHY | 100us busy-delay steps, at most ten, nominal1000us/frame; initial busy can reject immediately | Register access, delay accuracy, interrupts/preemption, logging; no internal MDIO mutex and no caller deadline |
| Complete backend batch | At most n MAC calls; controller mutex waits floor(lockWaitUs/tick) ticks, capped below portMAX_DELAY | fixedUs must cover acquisition/release and overhead; frameUs covers each actual call. Subtick wait is a zero-wait try, not rounded-up blocking |
| MAC initialization | floor(sw_reset_timeout_ms/10) reset checks/delays in10ms steps (default100ms) | Clock setup, efuse/PM and init/unwind work, quantization and dispatch; `install` has no aggregate timeout and is not a real-time API |
| MAC stop | At most100 HAL stop attempts with25us busy delays, nominal2500us | Each HAL call and possible1000-read FIFO flush, scheduler/logging; hidden flush error remains an observation limit |
| MAC multi-register mutex |100ms requested wait for destination-filter add/remove | Separate MAC filter API only. Not used by MDIO or SPEED/DUPLEX/PAUSE/LINK notification paths |
| SDK START/STOP/LINK event posts | Zero requested queue wait on framework paths | Event allocation/internal critical sections, dispatch/logging; a full queue can fail after prior MAC/cache effects |
| Owner event retry/STOP repair | Floor(remainingUs/tick) finite ticks | Scheduling and event internals, then elapsed check. Subtick allowance requests zero ticks |
| Timer fence | One precreated timer arm; at most maxFencePolls checks, delay(1) between | Prior timer callbacks, timer-task dispatch and owner wake latency; timer active includes executing callback |
| Running link polls | check_link_period_ms, accepted1..4294967ms (32-bit SDK ms*1000 limit) | Serial timer-task backlog, callback execution, owner admission. Period is not response time or EtherCAT timing |

An operation spanning owner polls has the sum of admitted B(n), actual minimum
waits, local/clock work and application dispatch gaps, all within its original
absolute deadline when the qualified schedule supplies those gaps. With at most
M remaining frames, callback work is bounded conservatively by
M*(fixedUs+frameUs); each nonempty batch consumes at least one frame. The existing
core poll/frame/stall limits bound performed work even for a frozen clock. If
polling stops, neither completion nor cleanup has an unconditional wall bound.
Restoration is separately admitted under OP-9; no controller lock spans a wait.

For stop, include framework stop/projection, timer-service arm, preceding timer
callback work, up to maxFencePolls owner wake/checks, missing DOWN/STOP delivery,
and at most maxStopAttempts actual MAC stops. All use one absolute deadline;
checks between calls prevent renewed budgets. A pre-call check cannot bound a
call whose backend/scheduler itself is unbounded. Deadline expiry retains
resources and effects; it does not prove a callback was interrupted or RX drained.

At100Hz one tick is10ms. `vTaskDelay(1)` can wake at the next boundary much
sooner than10ms; it is only a scheduling hint. Board readiness rechecks elapsed
esp_timer microseconds after every wake and anchors minimum waits after pin
callback completion plus clockLagUs (P4 converts counter ticks by floor division).
Power-on/clock event samples must have that same bounded uncertainty. A2ms reset-release floor therefore needs another tick
when the first wake is early. Budgets shorter than the next wake may time out
with admission still closed; there is no early MDIO to make them fit. MAC-init's
10ms tick delays are SDK polling behavior, not DP83826 readiness evidence.

**`native_wall_time` qualification gate:** no finite numerical hardware wall
bound is established here. The application must bound preemption/interrupts,
timer/event service, logging, clock lag and controller execution, and measure
those assumptions for the identified board/workload before making a real-time
claim. Arbitrary preemption has no finite bound. The fixture's declarations are
unmeasured conditional inputs; its default execution guard stays false. Host
failure injection and the real SDK image link remain useful evidence of their
own layers. This timing gate is separate from `native_rx_quiescence` and the
hidden MAC/HAL error limitations; it does not invent a successful backend.

## Reproducible build and verification layers

Use only the repository wrapper for `idf.py`; it checks the full pinned commit
and invokes the side-by-side environment without changing Windows settings:

```bat
scripts\idf.cmd --version
scripts\idf.cmd -C test\esp_idf\adapter_link -B C:\esp\build\dp8382x-adapter set-target esp32p4 build
```

The [fixture](../test/esp_idf/adapter_link/README.md) defaults to a volatile false
board-execution guard. Its final image retains actual MAC/factory/framework and
typed lifecycle symbols, while default boot only reports identity/status. The
application refreshes Git identity every build; release version still comes
from `library.json`. Conditional timing values in the fixture are unmeasured.
No flashing, board operation or physical qualification is implied.

The opt-in host target compiles actual supplied `esp_eth.c` and
`esp_eth_netif_glue.c`, the adapter, small OS/event/timer fakes and instrumented
MACs. Configure it with GNU or Clang and the pinned SDK path:

```sh
cmake -S . -B build/idf-host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/idf-host
ctest --test-dir build/idf-host --output-on-failure --no-tests=error
```

The compiler-derived selected dependency closure is recorded in
[sdk-files.json](../test/esp_idf/host/sdk-files.json). Configuration and builds
run [verify_sdk.py](../test/esp_idf/host/verify_sdk.py), checking the SDK revision,
canonical Git blob contents and the exact set of selected source/header files.
Only Windows CRLF checkout normalization is accepted. Fakes model the pinned
unarmed timer-stop INVALID_STATE and scheduled RX pause before application entry.
Raw and netif/glue handoffs are separate cases. These tests prove modeled
driver order, routing and lifetime; they do not execute real timer concurrency,
MAC hardware or physical timing.

Keep portable host, portable consumer, pinned-driver host, real SDK image link,
remote CI and physical results separate. First-execution remote CI belongs to
prompt 71. Q13 physical stop/RX qualification remains a separate gate after
software lifetime safety.
