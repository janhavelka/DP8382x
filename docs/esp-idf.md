# Native ESP-IDF PHY owner

## P4 ordinary Ethernet application

The [basic example](../examples/esp_idf/basic/README.md) is a complete native
ESP32-P4 image with explicit board RMII pins, PHY address 0 as a declared strap
input, application-owned reset/readiness, one `Phy` owner, `esp_netif` DHCP,
bounded callback facts and a finite stop/start cycle. Build it with:

```bat
scripts\idf.cmd -C examples/esp_idf/basic -B C:\esp\build\dp8382x-p40 set-target esp32p4 build
```

The image is inert by default (`DP8382X_BOARD_READY=n`). The current saved
netlist confirms the P4 signal mapping but does not demonstrate a supply source
on the PHY `3V3` input. Flash/monitor instructions and the qualified fixture
gate are in the example README; no physical link, DHCP or peer result is claimed.
The example's Git/dirty identity is refreshed in its build directory; an
archive reports unknown. Its `1000 ms` poll and conditional controller/callback
bounds are application policy, not measured PHY or scheduling guarantees.

The basic example independently reads the EMAC address before attaching
the glue and compares it with the attached netif before starting. ESP-IDF
v6.1 discards both the address-read and netif-set return values inside
`esp_eth_post_attach`. A mismatch or unavailable readback stops startup.
The example keeps the delivered operation result in static storage and
configures a 16 KiB main task. It services a negotiating link through the
single framework poller.

The application acquires lwIP/default loop, netif, bus mutex, MAC and PHY,
prepares the board, installs the driver, creates glue/attaches netif/registers
handlers, initializes and verifies 100/full AN, reconciles SDK state, then
starts the existing framework poller. The application task, never an event
handler, performs stop/retire and prints cached management, trust, resolved
link/profile and IP evidence. It does not infer reachability from DHCP.
After install, the native RX lifetime gate retains glue, netif, driver, MAC,
PHY, handlers and static callback context until restart. A failed attach may
already have published the netif RX pointer. Before install, only locally
created resources are released; ESP-IDF v6.1 does not support `esp_netif_deinit`
after lwIP init. An event handler does no blocking work and cannot wait for its
own unregister. Glue deletion has no independent deadline, as unregister can
wait for the longest default-loop handler. The first primary and cleanup
errors stay separate from the native lifecycle journal.

The native adapter in [Phy.h](../include/DP8382x/esp_idf/Phy.h) uses the existing
C++17 core through the public `esp_eth_phy_t` interface. Its baseline is
**ESP-IDF v6.1, commit `fff9895c82d744c7237be8847347bdd1b07c6643`, ESP32-P4**.
The component metadata selects that release and target. This is a selected
integration baseline, not a tested range of SDK releases or MACs. Executing
results and remaining gates belong to the [prompt 39 report](reports/implementation/39-esp-idf-link-bounds.md)
and [2026-11-18 corrections](reports/implementation/37-39-native-fixes-2026-11-18.md),
followed by the [second-round review](reports/implementation/37-39-native-fixes-round2-2026-11-18.md)
and [third-round corrections](reports/implementation/37-39-native-fixes-round3-2026-11-18.md);
the [source audit](reports/implementation/39-native-source-audit.md) records
the inspected inputs. A linked image does not establish physical operation.

## Component and application boundary

The [CLI owner bridge](../examples/esp_idf/common/Dp8382xCliNative.h) borrows
an installed owner and an application maintenance check. It uses stopped
typed apply/verify, the existing token/result path, and public SDK
reconciliation. Raw probe, discovery and register access provide fresh
receipts captured under the native gate, including failed prefixes and
the audited release status. The bridge can cancel an admitted token without
I/O or a new maintenance grant. Explicit CLI link/event acquisition requires a
stopped native owner and shares the core collector with framework polling.
Watch copies cached link evidence. The application provides a release hook for
CLI end and independent external disposition for abandon. The release hook owns
SDK stop and fencing and preserves storage still reachable by callbacks. Console
attachment is stage 47.

Typed CLI feature acquisitions use the stopped instance gate. Reset-bearing
feature procedures reserve the same shared bus silence as initialization.
Poll, cancel and result delivery use the existing native slot. The application
supplies the feature attestation for wiring and fixture prerequisites.

If a core clock fault during initialization prevents establishing the
shared reset silence floor, the adapter quarantines the bus before releasing
its initialization reservation. A peer may resume only after application
controller recovery establishes safe readiness.

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
Every participant checks admission, quarantine and the quiet floor before locking
and again **after acquiring the mutex**, then holds it for its whole transaction.
The second check closes the race with INSTALLING/BOARD_PREPARING reservations;
checking only before locking cannot make a logical silence window safe.
A complete C22/MMD/RMW batch owns that mutex; no selector state or controller
lock survives a terminal batch. Notifications, SDK lifecycle calls, callback
draining, waits and output run without locks their callbacks need.

## Local entry and framework callbacks

The table describes admitted calls, rather than permission to bypass the owner
using the published SDK handle. Unexpected phase/nesting rejects before PHY I/O.
`NativeStatus` keeps `coreStatus` and the actual `sdkError` separate; at most one
contains failure. SDK event, timer and MAC-policy errors are never labelled MDIO.
Local admission returns BUSY, lost verification NOT_INITIALIZED, and adapter
deadline expiry OPERATION_TIMEOUT. Actual SDK TIMEOUT stays in `sdkError`.
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
bounded local processing; gate waits are described below. `B(n)`, `N` and `T` are the
conditional bounds defined in [Timing declarations](#timing-declarations).
The timing/error tables describe observed return classification; real-time
qualification remains conditional. Timer polls and snapshots try the instance gate
without waiting. Lifecycle tasks and their synchronous start/control callbacks can
retry admission within their absolute deadline and finite `maxFencePolls` cap;
they release the gate before every delay. A busy timer entry is a missed observation,
not an attempted MDIO transaction. Ordinary failure preserves outputs. Unsupported
entries do not acquire a bus lock. No row delegates to the IEEE base.

The table's `L` covers local processing only. Add owner gate acquisition within
the existing absolute deadline wherever a lifecycle callback waits: STARTING
get_link/RESTART, CONTROL EN/DIS/SPEED/DUPLEX/PAUSE and STOPPING set_link. Each
such acquisition has at most `maxFencePolls` attempts with one-tick delays only
when remaining allowance permits; synchronous callbacks do not receive a new
deadline. RUNNING timer callbacks keep their zero-wait instance entry.

| PHY entry | Executing owner / complete transaction | Side effects and notifications | Error/output contract | Supported blocking bound |
| --- | --- | --- | --- | --- |
| `set_mediator` | Synchronous installer only; local borrowed pointer | Stores installer mediator before handle publication | OK during installation; otherwise INVALID_STATE; SDK ignores return | L |
| `reset_hw` | Installer; no transaction | No pin action: owner already completed board readiness | OK during installation, otherwise INVALID_STATE; SDK ignores return | L |
| `init` | Installer; no transaction | Local acknowledgement, no probe or reset | OK during installation, otherwise INVALID_STATE | L |
| `deinit` | Forbidden installed uninstall; no transaction | Atomic uninstall latch closes admission even if the gate is busy; later receipts cannot reopen it | Installed: INVALID_STATE, timer may already be deleted. Never installed: local OK. Installer failure never invokes PHY deinit | L |
| `autonego_ctrl(G_STAT)` | Installer or gated local query | Installer assigns initial true; later assigns desired mode only when valid | Null output INVALID_ARG; unknown desired/gate INVALID_STATE; ordinary failure preserves output | L |
| `autonego_ctrl(EN/DIS)` | Exact task/CONTROL reservation for public reconciliation | Acknowledges verified matching core mode into SDK cache; no write | Mismatch/unadmitted INVALID_STATE; output assigned only on OK | L |
| `autonego_ctrl(RESTART)` | Framework STARTING; shared core; one 3-frame BMCR batch | Command/readback; accepted/uncertain write retained; no AN wait | Core status retained internally, mapped native error; enabled assigned only on OK | L+B(3)+clock checks, within callback/start allowance under qualified bounds |
| `get_link` | Initial STARTING or RUNNING timer; core collector; batches of 8 and 2 frames | Acquired link evidence, then unlocked ordered projection; transient misses use the configured limit, lost trust requires owner stop | Physical observation stays separate from administrative state; partial notifications retain receipts | L+B(8)+B(2)+up to 5N+clock/local overhead |
| `set_link(DOWN)` | STOPPING owner/framework; no MDIO | Administrative DOWN only if last publication was UP | Unadmitted INVALID_STATE; accepted event loss separate from generic uncertainty | L+up toN, enclosed by stop deadline |
| `set_link(UP)` | No admitted unsolicited route | None | NOT_SUPPORTED | L |
| `reset` | Rejected; typed initialization owns resets | None | NOT_SUPPORTED | L |
| `pwrctl` | Rejected; no implicit power procedure | None | NOT_SUPPORTED | L |
| `set_addr` | Immutable declared address; no transaction | None | Identical address OK; any other NOT_SUPPORTED | L |
| `get_addr` | Immutable declared address; no transaction | Copies address | Null output INVALID_ARG, otherwise OK | L |
| `advertise_pause_ability` | Exact task/CONTROL PAUSE reservation; no transaction | Attests already verified AN bits 0/1; rejection retains possible preceding MAC policy mutation | Matching 0..3 OK, otherwise INVALID_STATE (including interference on busy gate) | L |
| `loopback` | Rejected; typed session owns stimulus | None | NOT_SUPPORTED | L |
| `set_speed` | Exact task/CONTROL SPEED reservation; no transaction | Acknowledges verified forced 10/100; SDK link cache changes only on notification | Illegal enum INVALID_ARG; mismatch/unadmitted INVALID_STATE | L |
| `set_duplex` | Exact task/CONTROL DUPLEX reservation; no transaction | Acknowledges verified forced half/full; no link-cache publication | Illegal enum INVALID_ARG; mismatch/unadmitted INVALID_STATE | L |
| `custom_ioctl` | Null optional PHY entry | No PHY callback | Commands >=0x1FFF fall through to the P4 MAC custom handler, which returns INVALID_ARG for that range | MAC custom dispatch only |
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
with a `bool`, then the PHY callback with a separate `uint32_t`. Ability bit 0 is
symmetric pause and bit 1 asymmetric; 0..3 must match already verified AN settings.
Change advertisement through `startConfigure` first. MAC/application policy stays
explicit. The owner never uses defective `ETH_CMD_S_FLOW_CTRL` or passes a small
Boolean as uint32 storage. `FlowControlRecord` separates MAC/PHY attempts, accepted
and possible effects, and their results. Invalid input has no MAC/MDIO effect;
MAC failure/late success or later PHY rejection may leave partial policy. Explicit
successful retry reconciles it; there is no automatic rollback.

P4's `set_peer_pause_ability` Boolean enables RX and TX together. The adapter
therefore projects true only when the portable observation has valid resolved
`pauseRx && pauseTx`; none, one-way or invalid resolution projects false. All
four local pause abilities remain legal policy inputs. Full directional evidence
remains in `linkSnapshot`; this Boolean cannot implement one-way MAC pause.

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
Each address uses core `readIdentityAt`: the same terminal audit, cumulative
transport counters, last batch and clock-fault handling as other traffic. This
RAW probe never changes accepted identity/trust or managed health. `invoked`
distinguishes a fresh callback receipt from an earlier batch after local denial.
Discovery never selects or retargets the bound address. Normal installation uses
the explicitly declared address; there is no default search or fallback.

## Readiness and lifecycle

`SharedBus::admission` is the sole atomic silence reservation. It starts
BOOT_CLOSED. Board preparation claims BOOT_CLOSED or AVAILABLE as BOARD_PREPARING;
initialization claims only AVAILABLE as INITIALIZING. Installation claims AVAILABLE
as INSTALLING. Another participant cannot replace these reservations, even between
owner polls. During INITIALIZING only its admitting core can issue batches;
BOARD_PREPARING and INSTALLING admit no MDIO. Successful
board preparation/result delivery returns AVAILABLE. A board failure before any
GPIO invocation restores its previous BOOT_CLOSED/AVAILABLE state when timing
and controller release are known. The shared POR floor is merged at entry,
before the deadline, quarantine and reservation checks, and the owner's earlier
board readiness is withdrawn there. Reopening AVAILABLE or rejecting an expired
call therefore never waives a newly powered peer's 50 ms silence.
Any invoked pin callback can have applied an effect even on error. Such a
failure, unknown timing, or failed mutex release leaves FAILED_CLOSED independently
of that adapter's lifetime; uncertain controller release also quarantines the bus. Other PHY reset
success cannot clear it. Only explicit application disposition after all users
are quiescent, or system restart, may re-establish readiness.

`prepareBoard` closes controller-wide admission and drains complete batches,
then releases the mutex before GPIO callbacks or hardware waits. The application
supplies real power-on and continuous-clock timestamps plus a finite reset pin
callback. Under the selected additional-reset path for a delayed clock, wait
at least 100 µs after both the POR completion bound and clock stabilization
before asserting RESET_N. POR readiness is at least 50 ms after the supplies reach their final levels,
and SMI remains inactive at least 2 ms after reset release. Assertion is at least
25 us without debounce capacitors; the board's RC circuit requires its own
qualified pulse. These requirements come from SNLS647I section 7.6 and 8.3.11,
as reconciled in [initialization](initialization.md).

Immediately before the first pin call, the remaining exclusive deadline must
cover two `BoardControl::pinCallbackUs` allowances, the assertion hold, the
2 ms post-release quiet interval, clock lag for both waits and worst-case tick
rounding for each wait. The default per-call allowance is 1,000 µs. Qualify
it with the actual GPIO implementation and scheduler. If an attempted
assertion fails or its hold cannot finish, the owner makes one best-effort
release call. The shared bus stays FAILED_CLOSED. An unsuccessful release
does not establish pin level, pulse width or readiness. Inspect both the
primary and cleanup receipts before external disposition.

Elapsed-time checks and finite poll caps apply. A floor-rounded RTOS delay
alone is insufficient. Quiet
floors merge by atomic maximum. Board completion releases shared admission once,
after waiting for its own reset-release due time. A later peer floor remains
enforced by install and complete batches; it cannot turn this board's completed
readiness into a controller-wide failure. Before requesting a one-tick delay,
the remaining deadline must contain that tick at the configured rational rate.
Scheduler delay can still exceed the requested wait; elapsed checks detect it.
Board completion is independent of snapshot contention; its receipt publishes local readiness
at the next gated entry without overwriting a later participant's reservation.

Applications quiesce the bus before external power changes; preparation cannot
undo earlier SMI activity. The bus has no reset-pin topology or peer registry.
If RESET_N is shared, stop the affected peers, resolve their active operations
and call each installed peer's `invalidateDeviceState(AMBIGUOUS_MUTATION)` under
its owner before invoking the pin callback. After a proven completed reset,
`EXTERNAL_RESET` may attest that reset's effects; an uncertain pin error cannot.
Reinitialize and reconcile each affected peer before restarting it. Independent
RESET_N nets still share SMI silence, but do not by themselves invalidate the
other PHY's settings. This responsibility also applies to external resets.

Only after readiness does `install` call the public SDK installer. The fence
timer already exists. Installation claims silence, drains the shared controller
mutex, and rechecks quarantine/readiness before releasing that mutex for SDK waits.
Contended drain rejects before installation is attempted and can be retried.
The logical reservation excludes MDIO/reset/other installation throughout MAC
reset and CSR-clock configuration. Success releases it; failure leaves FAILED_CLOSED.
MAC initialization needs an existing PHY REF_CLK and polls
its reset in 10 ms steps, using `sw_reset_timeout_ms` (SDK default 100 ms).
The adapter's install callbacks do no PHY I/O. Installation failure publishes no
handle; only MAC and never-installed PHY deletion remain. After successful
installation, the owner runs typed initialization/configuration through direct
MAC batches, publicly reconciles SDK state, then admits `start` for verified normal settings.

Initialization claims shared logical silence before a zero-wait drain of the
batch mutex and retains it through result delivery. A failed drain releases its
reservation without changing hardware. Only that owner executes batches during silence;
no mutex spans polls or waits. Successful `takeResult` reopens admission while
preserving any reset quiet floor. Cancellation does not erase the floor. Drain
release failure quarantines the bus and issues no frame: it returns BUSY with
CONTROLLER_QUARANTINED and records the exact SDK failure if firstError is empty,
without incrementing MDIO accounting. The application must keep polling and deliver
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
callback cannot acquire its gate. Owner stop/fence repairs them; completed repair
returns OK and keeps the original unarmed-timer error in cleanupError. Queued STOPPING
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

`LifecycleRecord` copies current admission phase and retains a saturating start-cycle
sequence, stage, first error, first/latest cleanup errors and last accepted tuple,
publication uncertainty, mandatory `stopRequired` and pending event-loss evidence.
Only an admitted new start resets the error fields and increments the sequence;
stop retries and retirement preserve the cycle's evidence. Per-field acceptance
flags preserve successful SPEED/DUPLEX/PAUSE updates if a later notification
fails. UP publishes speed,
duplex, peer pause, then link. A changed UP tuple first projects DOWN. The P4
factory identity attests that valid MAC `set_link` succeeds; a subsequent LINK
notification error therefore retains accepted MAC/cache publication and a lost
event. `eventLost` refers to the current accepted LINK event; a later successful
LINK supersedes that pending event, while historical errors remain retained.
An adapter deadline forwarded by an SDK callback keeps its original portable
deadline status; the enclosing SDK return does not relabel it as an SDK timeout.
`retryLinkEvent` retries the event alone with a finite deadline. It never
replays MAC UP or rebuilds DMA descriptors. During stop, a pending DISCONNECTED
is attempted before STOP, including one lost by an earlier timer poll. If the
bounded repost fails, framework stop, the fence, STOP and actual MAC stop still
proceed; the event error and DOWN repair remain owed in STOPPING. In this failure
case STOP can precede repaired DISCONNECTED. Cleanup event retries and standalone
STOP repair request zero queue wait, so a full queue cannot consume each new
cleanup allowance and indefinitely withhold hardware stop. Successfully
repaired stop returns OK while retaining the earlier cleanup error. An error on another MAC can leave
publication uncertain and closes further ordinary publication/start admission.
Owner stop still attempts the actual MAC stop. If current DOWN/cache/event state
cannot be established, stop/retire return the failure and remain STOPPING;
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
| Coherent link up with AN in progress and `waitForNegotiatedLink` enabled | Publish MAC DOWN, keep the coherent PHY observation, clear the miss count and continue framework polling. |
| Running observation changes, temporary admission/mutex contention, own publisher is in flight, or other unresolved UP with trust intact | Preserve the tuple for fewer than maxObservationMisses; at the limit request administrative DOWN without stopRequired. Next coherent poll may publish UP normally |
| Bus is FAILED_CLOSED or quarantined (including unexplained P4 busy while holding the mutex), clock/transport contract fails, trust/SDK agreement is lost, or publication has a terminal unknown effect | Preserve acquired/partial/stale evidence and actual cause; request administrative DOWN and latch stopRequired. FAILED_CLOSED preflight records NOT_INITIALIZED with shared-board-readiness context. Application must stop and resolve the shared fault before restart. This does not claim physical cable-down |
| First resolved UP | SPEED, DUPLEX, PAUSE, UP; each field accepted separately |
| Speed, duplex or Boolean peer pause changes while up | DOWN, SPEED, DUPLEX, PAUSE, UP; latest acquired and accepted fields stay separate |
| Same fully published tuple | No notifications/events; a still-lost event needs explicit event retry |
| Notification returns OK late | Retain accepted field/transition; return TIMEOUT; known prefix remains safe. A later poll may reapply idempotent fields while down, but never replay an accepted unchanged UP |
| P4 LINK returns error | Record accepted link/cache and eventLost; stop on start failure, otherwise owner may retry only current event |
| Generic MAC/field notification error | Preserve accepted prefix and uncertain failing effect; stop and terminal retention |
| Event retry accepted after deadline | Clear eventLost, return TIMEOUT and retain error; never send it again as though unaccepted |
| Stop then restart | Administrative DOWN and fenced timer stop; next resolved UP republishes all fields into SDK caches |

`maxObservationMisses` is positive (default 3), checked by the factory. It counts
consecutive admitted RUNNING queries, saturates, and clears after a coherent
publishable observation. A denied instance gate cannot update that count; this
is a poll-count policy, not an unconditional wall-time guarantee. The latest
`lastObservationStatus` remains visible even when `firstError` already contains
an older event failure. If the limit is reached during our own publication,
`observationDownOwed` survives until that receipt completes: the next available
poll withdraws UP before collecting another sample, and the following good poll
can republish. No concurrent publisher is introduced. Initial STARTING failures
still fail start and use its separate cleanup allowance.
Persistent gate contention can therefore retain stale UP indefinitely. An
arbitrary upper cap on the positive counter would not solve that starvation;
the application must enforce its own observation-age/owner scheduling policy.
A successful physical read does not override lost settings trust: reset detected
in that read requires stop immediately, for both unresolved UP and coherent DOWN.
Observation validation and publication admission retain the same instance gate;
it is released only after staging the in-flight publisher. Owner invalidation
cannot slip between them and then allow stale UP. During notification callbacks
invalidation returns BUSY; the application must retry before any external mutation.

The application must use owner lifecycle methods. Direct `esp_eth_start`,
`esp_eth_stop`, `esp_eth_driver_uninstall`, PHY deletion and mediator replacement
can cause SDK effects before a PHY callback rejects. In particular, uninstall
can delete its timer before reaching `deinit`; a rejected attempt does not
restore that timer or authorize a later start. After that uninstall latch,
owner stop uses only its independent
fence, DOWN/STOP repair and actual MAC stop; it never calls the SDK's deleted timer.
Native cleanup remains STOPPING until resolved, then MAINTENANCE. Bus-silent
cancel/takeResult remain admitted even there: the application must cancel any
unfinished core operation and consume its result to release INITIALIZING silence.
Native stop completion does not claim core result delivery. Arbitrary control ioctls
remain application-owned and can have partial MAC effects before PHY rejection.
If a never-started core operation is still in CONTROL, this forbidden uninstall
route conservatively uses the independent fence and direct MAC stop, even though
the current cycle never started the MAC. SDK FSM STOP and timer deletion do not
attest hardware idle; no START evidence means no synthetic STOP event. Accepted
MAC-stop receipts are still retained, and no deleted SDK timer is used.
If forbidden direct stop runs while the link is already DOWN, the SDK emits no
PHY callback to reveal whether STOP was delivered. Cleanup after raw uninstall
therefore conservatively posts STOP; a previously delivered raw STOP can be
duplicated. The call returns INVALID_STATE and closes normal admission; firstError
preserves the earliest failure if it already contains another cause. Exactly-once accepted
event bookkeeping applies to owner-routed transitions, not this invisible bypass.
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

`retire` enters terminal MAINTENANCE and returns NOT_SUPPORTED only after successful
stop/fence/DOWN/STOP/MAC-stop disposition (or an idle, never-started owner). Failure
returns its actual error and retains the same resources. Already admitted cleanup
remains retryable in STOPPING; denied initial admission preserves the prior phase.
Retire's final gate can time out after stop has completed, leaving STOPPED with
no new cleanup error. CONTROL rejection also preserves its existing obligations.
An already stopped owner with no work owed performs no framework stop, fence,
event or MAC call. Terminal retention keeps driver, glue, MAC, PHY, netif, controller
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
RMW sequence. A lock failure issues no frame: zero-tick contention returns BUSY; a positive
requested wait that expires returns MDIO_TIMEOUT with native detail and counts
as a managed transport failure when issued by managed traffic; uncertain controller completion or
release quarantines the shared domain. The pinned P4's busy-at-entry INVALID_STATE
proves only that this read/write issued no command. While holding the shared
mutex, unexplained controller activity means idle is unproved; a bypassing MDIO
user is one possible cause. Return MDIO_ERROR with exact INVALID_STATE, preserve
the completed prefix and mark the rejected frame unattempted. The independent
releaseStatus records missing idle assurance and controllerReusable is false;
quarantine blocks every participant until external disposition. A mutex-release
failure similarly uses releaseStatus MDIO_ERROR/ESP_FAIL, with its own message,
while preserving the primary frame error. This is a terminal transport fault,
not a fictional MDIO frame; init's pre-transaction drain instead uses BUSY and
the native lifecycle journal. Transaction reuse evidence is captured before
unlock, so a later owner's failure cannot corrupt this completed receipt.
Generic MAC failures and actual
P4 MDIO timeouts retain uncertain effects and quarantine. Passive health cannot reopen it.

### Recovering a quarantined shared controller

1. Close application traffic and maintenance admission for every bus participant.
   Stop the affected framework owners and confirm each stop/fence receipt.
   An owner in CONTROL cannot stop: cancel its operation, deliver the result
   with `takeResult` and keep the reported possible writes first. Until that
   delivery succeeds the owner keeps CONTROL, and the bus stays closed.
   Keep installed resources and late RX contexts alive.
2. Identify and stop any MDIO access outside the shared owner. Establish that
   every outstanding controller command has completed, the controller is idle,
   the mutex ownership is known and all future participants use that mutex.
   The pinned MAC has no public controller-idle query. A cancelled timer,
   successful mutex take or elapsed delay alone supplies insufficient evidence.
3. With exclusive application ownership and that evidence, clear
   `bus.quarantined`. Preserve `bus.admission` and `quietUntilUs`. A
   FAILED_CLOSED board, ambiguous configuration write, lost controller mutex
   or stopped core transport needs its own disposition. If the application
   cannot establish safe reuse, keep admission closed until system restart.
4. While each owner is stopped, verify or explicitly restore its owned
   configuration as required, check `reconcileSdkState`, and explicitly
   `start` each eligible owner. Acquire fresh link evidence before admitting a
   new application session. Preserve the original error and any uncertain
   effects alongside the recovery results.

`L` covers fixed local/gate/clock processing under the qualified workload;
timer/snapshot instance acquisition requests zero ticks. Owner gate retries use
the original deadline and at most `maxFencePolls` attempts per acquisition.
`N=notificationUs` covers one entire
mediator call including actual MAC, logging, event posting and scheduling.
`T` covers the selected public timer-service call under that same workload.
Factory validation requires `callbackBudgetUs > B(8)+B(2)+clockLagUs+5*N`
and `linkPollMs*1000 >= callbackBudgetUs`, with checked 64-bit arithmetic and
positive cleanup allowance. The remaining callback allowance must cover local
work; a period alone does not guarantee prompt dispatch.
These are conditions supplied/qualified by the application, not constants
proved by the SDK. Five N allowances are reserved before live acquisition;
callback/start deadlines remain absolute. A notification, public reconciliation control or MAC/PHY policy callback
exceeding N or its aggregate deadline returns TIMEOUT after retaining accepted effects. A late
failed callback keeps its original error. Nothing preempts a synchronous call.

| Actual source path | Finite count / requested wait | What the bound must also cover |
| --- | --- | --- |
| MAC read/write PHY | 100us busy-delay steps, at most ten, nominal 1000us/frame; initial busy can reject immediately | Register access, delay accuracy, interrupts/preemption, logging; no internal MDIO mutex and no caller deadline |
| Complete backend batch | At most n MAC calls; controller mutex waits floor(lockWaitUs*configTICK_RATE_HZ/1000000) ticks, capped below portMAX_DELAY | Rational conversion also applies at rates such as 333Hz. fixedUs covers acquisition/release/overhead; frameUs covers each actual call. Subtick wait is a zero-wait try |
| MAC initialization | floor(sw_reset_timeout_ms/10) reset checks/delays in 10ms steps (default 100ms) | Clock setup, efuse/PM and init/unwind work, quantization and dispatch; `install` has no aggregate timeout and is not a real-time API |
| MAC stop | At most 100 HAL stop attempts with 25us busy delays, nominal 2500us | Each HAL call and possible 1000-read FIFO flush, scheduler/logging; hidden flush error remains an observation limit |
| MAC multi-register mutex | 100ms requested wait for destination-filter add/remove | Separate MAC filter API only. Not used by MDIO or SPEED/DUPLEX/PAUSE/LINK notification paths |
| SDK START/STOP/LINK event posts | Zero requested queue wait on framework paths | Event allocation/internal critical sections, dispatch/logging; a full queue can fail after prior MAC/cache effects |
| Owner event retry/STOP repair | Running explicit retry uses floor(remainingUs*Hz/1000000) ticks; cleanup retry and STOP repair request zero queue wait | Scheduling and event internals, then elapsed check. Subtick allowance requests zero ticks |
| Timer fence | One precreated timer arm; at most maxFencePolls checks, delay(1) between | Prior timer callbacks, timer-task dispatch and owner wake latency; timer active includes executing callback |
| Running link polls | check_link_period_ms, accepted 1..4294967ms (32-bit SDK ms*1000 limit) | Serial timer-task backlog, callback execution, owner admission. Period is not response time or EtherCAT timing |

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
checks between calls prevent renewed budgets. Failed or late start has a separate
`cleanupBudgetUs` allowance anchored after the SDK start returns; its primary
result is never replaced by cleanup. Accepted fence arms, STOP delivery and MAC
stop completions survive gate contention and are not replayed on a stop retry.
A pre-call check cannot bound a
call whose backend/scheduler itself is unbounded. Deadline expiry retains
resources and effects; it does not prove a callback was interrupted or RX drained.

There is no source-qualified numeric minimum that guarantees cleanup completion:
fence dispatch, prior callbacks, scheduler latency and generic MAC stop lack a
universal finite bound. `cleanupBudgetUs` is therefore a positive **attempt**
allowance, not a completion promise. A small value can expire with effects and
obligations retained; the owner then calls stop with a separately chosen finite
deadline. Fence/MAC attempt counts do not turn that allowance into a minimum.
There is a necessary lower bound for an owed DOWN: the allowance remaining at
publication must be strictly greater than `notificationUs`. Thus a total
`cleanupBudgetUs <= notificationUs` cannot admit that DOWN. The factory permits
this explicitly limited attempt; it may close framework admission or retain a
fence receipt, then requires a later owner stop to deliver DOWN. No successful
cleanup or vanished obligation is reported. A larger total is also insufficient
unless prior callbacks, gate waits, fence and MAC work leave enough time.
Stop closes RUNNING admission only after acquiring the gate; an expired or
exhausted initial admission leaves polling active. After a completed stop,
retire admission failure cannot manufacture a cleanup error or undo completion.
The journal distinguishes an observed exclusive deadline (`DEADLINE`), exhausted
finite gate/fence/board polls (`POLL_BUDGET`), and insufficient remaining allowance
for a whole tick or notification (`BOUND_ADMISSION`). These remain core
OPERATION_TIMEOUT through SDK forwarding. An actual SDK TIMEOUT retains its
SDK domain even if the clock has also crossed the owner's deadline.

At 100Hz one tick is 10ms. `vTaskDelay(1)` can wake at the next boundary much
sooner than 10ms; it is only a scheduling hint. Board readiness rechecks elapsed
esp_timer microseconds after every wake and anchors minimum waits after pin
callback completion plus clockLagUs (P4 converts counter ticks by floor division).
Power-on/clock event samples must have that same bounded uncertainty. A 2ms reset-release floor therefore needs another tick
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

**Timer-task stack:** the fixture configures 3584 bytes before the SDK's conditional
extra stack. The second-round P4 GCC `-fstack-usage` check measured 1392 bytes for the nested
`getLink`/`readLinkStatus`/core `_execute`/native `execute` frames alone, before
deeper SDK/logging/error paths. These static frames do not establish a safe total.
Qualify the timer task's actual high-water mark under release logging, failure and
contention workload, with measured margin, before enabling board execution.

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

An application may call `withAdmissionEvidence` to decide session admission
under the same gate that protects framework observations. The synchronous
callback receives cached link, settings and health, the running MAC's agreement
with the observed link, and the current esp_timer time. Gate contention returns
BUSY before the callback. Rendering can use the application's passive snapshot.
The P4 raw owner can submit one admitted frame inside this gate using its
single-task, TX-mutex-off configuration. It keeps the SDK buffer borrow,
session check and MAC publication synchronized. The
[frame lifetime table](ethercat-integration.md#frame-ownership-on-the-pinned-p4-driver)
describes late RX, buffer release and the installed lifetime.
Set `PhyConfig::waitForNegotiatedLink` to keep the framework timer running
when a coherent sample shows link up with autonegotiation in progress. The MAC
stays down until the PHY resolves the link. This applies at start and on later
polls. The option defaults to false.
See [application ownership](ethercat-integration.md).
