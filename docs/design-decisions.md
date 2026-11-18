# Portable core and native SDK ownership decisions

This maintained design applies the delegated remaining-prompts audit decisions.
The [API contract](api-contracts.md) owns portable values and OP-1 through OP-17;
headers own shipped declarations. The [architecture](architecture.md) is an
overview, and [feature coverage](feature-coverage.md) is the single coverage map.
Prompts37-39 implement the subset listed in the [native guide](esp-idf.md), with
real-driver host tests and a P4 SDK-linked fixture. Broader typed controls remain
with their owning stages. Source-derived sequences are not physical qualification.

## 1. One chip implementation and one batch boundary

Use the allocation-free C++17 core for identity, register policy, chip procedures,
persistent intent, observations, operations and one destructive-evidence collector.
The application serializes every instance call, including cached snapshots, and
publishes copies to other tasks. Native code translates the supported PHY interface
to those same procedures; it does not create another chip configuration engine.

One borrowed synchronous terminal batch callback owns each complete C22/MMD/RMW
transaction. Fixed frames use preceding-read preserve masks and literal bits.
The core owns masks, neutral command values and access policy; the backend owns
controller execution and locking. Eight-frame capacity covers compound accesses.
C22 costs one frame, C22 RMW two, MMD read/write four and stable MMD RMW five.
The five-frame sequence is inferred from SNLS647I 8.3.11.1 selector-state text,
not printed as a combined recipe; Q4 remains a physical case. Each call reselects.

All bus participants share the controller lock, quarantine and silence admission.
One atomic BusAdmission claims board readiness or initialization across owners;
no PHY can overwrite another reservation. Failed board readiness remains
FAILED_CLOSED even after never-installed adapter destruction.
Close shared admission and drain active traffic before power/reset transitions;
release the mutex during readiness waits without admitting another PHY's frames.
No selector state, controller lock, or caller buffer survives a terminal batch.
Stop on first error and retain valid read prefixes, uncertain effects, time and
release results. Merge destructive data into the collector before another instance
call. Failed/unknown controller completion quarantines every participant until
application repair. Core local end/bind cannot clear shared quarantine.

## 2. SDK baseline and owned PHY wrapper

Use unmodified ESP-IDF v6.1 at
`fff9895c82d744c7237be8847347bdd1b07c6643`. Re-audit the public route below whenever
the pin changes. No SDK support range is implied by this source selection.

Use `struct PhyHandle { esp_eth_phy_t phy; Phy* owner; };`, recover by a checked
first-member cast and one layout static_assert. Do not include the generic PHY
base header or cast private Ethernet driver storage. The application owns one
wrapper, core and borrowed actual MAC. Address is immutable: set_addr accepts
the identical address only; retarget by deletion and recreation.

| Callback or entry | Executing behavior |
| --- | --- |
| set_mediator, get_addr | Local guarded storage; no MDIO |
| reset_hw, init, deinit | Local no-action success under lifecycle guards; the owner does board readiness before install and typed initialization afterward |
| autonego_ctrl(G_STAT) during install | Return declared desired AN without MDIO |
| reset, pwrctl | ESP_ERR_NOT_SUPPORTED; use the explicit typed owner route |
| autonego_ctrl, speed, duplex, pause, loopback controls | One terminal batch with no wait, or reject and direct to the typed route; no callback token/poll loop |
| get_link | Same core collector and retained projection; framework timer is the only running-mode poller |
| set_link(DOWN) | Administrative stop projection, no PHY status read; unsolicited UP rejects |
| raw mediator register hooks | Unconditional ESP_ERR_NOT_SUPPORTED with zero MAC frames |
| del | Owned deletion only after completed resource/callback disposition |
| custom_ioctl | Null; typed owner requests expose extensions |

The native owner implements AN/forced configuration, NONE/PCS/HARD/RESTART,
core polling/result delivery, stopped policy-checked raw access and explicit discovery (up to 64
identity frames, never selecting an address). SDK RESTART shares the core's
three-frame terminal AN restart/readback; it has no wait or operation token.
EN/DIS and speed/duplex/pause callbacks acknowledge verified shared-core settings
only within their exact public-control owner/task reservation; loopback rejects.
The [callback table](esp-idf.md#local-entry-and-framework-callbacks)
is the current implementation availability map for the responsibility matrix below.

No generic base reset, power, discovery, MMD or AN polling procedure executes.
All public widths are validated before narrowing. Vendor MMD REGCR values are
whole numeric 0x001F/0x401F words with reserved bits zero; that rule is not a
global target-register reserved policy.

## 3. Admission, call chains and lock order

Native admission uses one Phase enum: Stopped, Control, Starting, Running,
Stopping, Maintenance. A phase-by-method switch admits expected callbacks;
unexpected nesting rejects. Track one record containing stage, firstError,
cleanupError, publishedTuple, publicationUncertain, eventLost and stopRequired.
The two errors separate portable Status from native SDK failure and reset only
on an admitted new start, with a saturating cycle sequence. A reservation
is logical admission, not a held mutex. No callback steals an application token.

Lock order is instance gate, controller lock, controller internals. Never acquire
another instance gate with the controller lock held. Release locks before SDK
lifecycle/control calls that may re-enter the adapter, mediator notifications,
callback draining, waits and output. The outer owner reserves under its gate,
releases it for the SDK call, and reacquires to record the result; callbacks
reacquire and check the phase. Never synchronously queue a request to the owner
and wait from a callback invoked by that same owner.

Notifications use an immutable acquired projection. Reserve under the instance
gate, release locks, call the mediator, reacquire and record accepted or uncertain
effects. Event handlers only request later work and do not wait for synchronous
mutations. Terminal publication/start receipts survive post-callback gate contention;
the next gated entry adopts accepted effects before admission. Cached
linkSnapshot retains full acquired evidence separately from SDK fields.
Callback counters cover adapter work; the timer fence also addresses
callbacks that captured the driver before entering the adapter.

Portable owner -> shared core procedure -> terminal batch -> controller lock
-> MDIO -> release -> retained evidence -> owner scheduling.
Native typed owner -> same core -> batch -> borrowed actual MAC frame methods.
SDK timer -> owned get_link -> same collector -> unlocked mediator projection.
CLI and IRQ requests -> same owner; ISR never performs MDIO.

### Frame hooks and handle context

Both public raw register hooks are static unconditional rejection functions.
They need no incoming-handle lookup, global mapping or thread-local permit.
Raw SDK ioctls reach the mediator outside PHY methods, including when called by
the owner task, and always reject. Owner-routed raw/typed maintenance uses the
core policy and actual MAC frame methods inside the terminal batch. No loop
through mediator or another locking hook is permitted. The selected MAC's local
init/control needs no raw PHY hook route; a new MAC/SDK must re-audit that fact.

## 4. Complete responsibility matrix

| Responsibility | Portable owner | Native owner |
| --- | --- | --- |
| Board readiness | Supplies, pins, address, clocks, power/reset timing and shared silence | `prepareBoard` completes before `install`; confirm REF_CLK for MAC initialization |
| Bind/end/invalidation/result | Serialized core local calls under OP rules | Same core, plus retained native resources/callback lifetime |
| Initialization/reset/replay | startInitialize selects NONE/PCS/HARD/RESTART | Same core after install under shared logical silence, separate public SDK reconciliation before restart |
| Verify/configure | startVerify and startConfigure | AN/forced configure reuses core; stopped reconcileSdkState acknowledges verified mode through public controls without another chip procedure |
| Live link and events | Explicit core collector; passive copied snapshots | Implemented initial STARTING and timer RUNNING `get_link`; core collector and unlocked mediator publication, event-only retry |
| Interrupt configuration/service | Same collector owns consumed samples | No independent SDK or raw interrupt reader |
| Diagnostics and temporary stimulus | Source-admitted sessions, later stop/restoration and barrier | Stopped/maintenance owner also coordinates MAC and fixture |
| Power/EEE/WoL/LED/interface/clock | Exact typed procedures, configuration trust and source gates | Same procedures; no imaginary SDK commands |
| Raw C22/MMD/CLI | Core policy, complete transactions and invalidation | Stopped reads/LEDCR writes/RMW, explicit BMCR maintenance and collector acquisition use core/direct MAC batches; raw SDK hooks reject; CLI remains later stages |
| SDK lifecycle/notification | Not applicable | Implemented install/start/stop/fence/retire and one retained lifecycle record; never-installed destroy only, reusable installed teardown BLOCKED |
| DMA, IP/EtherCAT, retry policy | Application/backend owns them | Existing MAC/framework and application stack own them |

No public entry is implemented merely by appearing in this matrix. Exact source
gates and acceptance cases remain with each owning feature stage.

## 5. SDK cache, notifications and raw reconciliation

SDK auto_nego_en is a cache. Public stopped S_AUTONEGO passes its storage to the
callback: stage output and assign only after verified success. Speed/duplex
controls require cached AN disabled. Native managed changes cannot bypass these
public controls, and no full-advertisement ioctl exists.

For either mode transition: stop/drain, validate the tuple, execute shared core
configuration and deliver its verified result. Then call public S_AUTONEGO/
G_AUTONEGO and, for forced mode, S_SPEED/S_DUPLEX_MODE. Scoped callbacks acknowledge
the verified matching profile without writes or waits. SDK agreement remains false
until every public call completes in budget. G_SPEED/G_DUPLEX_MODE remain last
published link caches, not immediate desired readback.
An accepted/uncertain write, external reset or raw mutation invalidates agreement;
remain stopped until re-identification/replay/readback/public cache reconciliation
succeeds. Never patch SDK private caches or turn raw writes into desired intent.

BMCR readback supplies forced speed/duplex when AN is disabled and link is up;
PHYSTS low bits stay raw. The forced_physts question remains a physical-value
question rather than a blanket software block. With extended full-duplex enabled
or unverified, forced 100/half cannot be universally resolved; native admission
rejects that tuple when it cannot publish UP. Portable pause retains complete
RX/TX resolution. The P4 Boolean enables both directions, so the adapter projects
true only for valid resolved RX AND TX pause; one-way/invalid resolution projects
false. MAC flow-control policy is separate; avoid the pinned flow-control ioctl
argument-width defect and call the correctly typed MAC method.

For UP send speed, duplex, peer pause, then link and record each accepted step.
For a changed tuple while up, project DOWN before updates and UP. Emit nothing for
an unchanged verified tuple. Unknown observation does not invent cable-down;
administrative DOWN is explicit and distinct. Failed running acquisition or lost
trust/agreement requests that DOWN and latches stopRequired until complete stop;
the prior physical observation remains historical. OP-14 limits live resolution during
semantic writes or temporary profiles.

The P4 factory knows its pinned set_link cannot fail for a valid link argument:
after on_state_changed(LINK,x) returns, record published=x and never replay it.
If event posting failed, set eventLost and let the owner re-post the event only.
For another MAC, including instrumented ones, notification error may leave MAC
effects uncertain: stop and reconcile. Factory identity selects this behavior,
not the error code. Never retry UP blindly; it can rebuild DMA descriptors.

<a id="6-lifecycle-correction-selected-for-native-support"></a>
## 6. Public lifecycle route for the pinned SDK

1. Before esp_eth_driver_install, drive board reset and finish readiness waits
   without MDIO. MAC init configures SMI and needs PHY REF_CLK already present.
   INSTALLING reserves the shared controller domain after draining its mutex;
   release the mutex before MAC reset waits, then release logical silence once.
   Its reset polls in 10 ms steps up to sw_reset_timeout_ms, default 100 ms.
   Adapter reset_hw returns local OK and set_mediator only stores context.
2. Adapter init and install G_STAT are local/nonfallible under validated input.
   Install errors leave owner mac->del and phy->del; the API returns no retained
   partially installed handle. No reset error is deferred until after MAC init.
3. After install run typed startInitialize/configure through direct MAC batches.
   Their failure follows the same gated stopped teardown below, retaining primary
   evidence and referenced resources.
4. On esp_eth_start failure or late completion call owner stop with a separate
   finite cleanup allowance anchored after start returns. If no timer was armed SDK stop changes
   FSM to STOP then returns ESP_ERR_INVALID_STATE before DOWN or STOP publication.
   Other stop errors may also omit them. After the fence below, send missing
   administrative DOWN and repair a lost DISCONNECTED before posting STOP. If
   START was posted but STOP was not, post STOP using
   &hdl and sizeof(hdl), with finite ticks. get_link in Starting records START.
5. An already stopped owner with no obligations returns OK without callbacks.
   Otherwise fence after framework stop: an owner-created ESP_TIMER_TASK one-shot exists
   before install. Arm once with esp_timer_start_once(fence, 0), then poll
   esp_timer_is_active with task delay until false under a finite deadline.
   Never wait from the timer task or re-arm while polling. Timeout retains all
   resources and retryable STOPPING. After the fence, publish DOWN if the journal still says UP; then
   call mac->stop(mac) directly to observe the HAL result set_link discards.
   Each attempt can busy-wait 2500 us; finite retry holds no instance/bus lock.
6. Stop all transmit callers and end the application RX session. Keep its
   callback/context alive and make it drop/free arrivals without blocking. For
   attached netif, bound all default-loop handlers and STOP delivery; glue
   deletion has no independent timeout. Neither STOP delivery, a timer fence,
   a mediator swap nor application RX count zero proves that the MAC RX task
   has stopped using the SDK driver or netif glue.
7. Apply the `native_rx_quiescence` gate before any destructive teardown. The
   pinned RX task can retain the old mediator before its application callback;
   `set_mediator` is an unsynchronized assignment and uninstall frees that
   mediator's driver before `mac->del` terminates the RX task. The previously
   proposed swap/drain/uninstall/delete order is therefore not an accepted
   teardown procedure. Do not free the driver, glue, MAC, PHY, netif or borrowed
   callback context while that task may still reference them. Close admission,
   retain resources and primary/cleanup evidence, and require system restart.
   Enter terminal MAINTENANCE only after stop succeeds; failed retirement returns
   the actual error and keeps stop retries admitted. A forbidden raw uninstall
   atomically closes admission because the SDK may already have deleted its timer.
8. Prompts 37 and 42 must independently derive and test a public-API quiescence
   route before enabling reusable destruction. A changed SDK baseline requires
   explicit source re-audit, not a private cast or fork. Until such a route is
   established, report reusable teardown BLOCKED and the retained-resource
   terminal route separately. No delayed callback may access released storage.

The blocking evidence is in pinned `esp_eth_mac_esp.c`: set_mediator at 200,
RX task at 600 (mediator call at 630), and task deletion in free_driver_obj at
804; `esp_eth.c` uninstall at 274 frees the driver at 295. A scheduled test must
pause RX after it selects the old mediator but before callback entry, run stop,
and resume RX; zero application callback count must never authorize a free.
This is a source/lifetime gate, not a physical-only Q13 limitation. Q13 measures
stop/traffic behavior only after a safe software lifetime route is established.
The HAL FIFO-flush timeout remains a separate unobservable SDK limitation.
If a future baseline includes upstream d2ed96d2, re-audit the timer fence and
unarmed-stop behavior independently; that timer change alone does not close RX
lifetime safety.

### Failure and two-instance walkthrough

Keep distinct core/owner/wrapper records for two instrumented MAC instances;
one physical P4 EMAC module supports only one owner. A second PHY sharing MDIO
is a separate batch participant, never another native object sharing that MAC.
Test partial install/start/stop/deletion, callback fence ordering, zero-frame raw
ioctl rejection, result backpressure, event-post failure and shared quarantine.
An SDK-linked image proves symbols and configuration, not these executing cases.

## 7. Bounds, board readiness and competing operations

API OP rules define core timing, direct-call admission, cleanup barriers and
hard-reset floors. Native bounds also include owner admission, SDK/event/notification
work, frame methods and scheduling. The MAC uses 100 us MDIO steps with a nominal
1000 us frame polling allowance, no caller deadline and no MDIO transaction mutex.
Its 100 ms multi-register MAC mutex is not the bus lock. These are not wall-clock
or EtherCAT real-time guarantees; adapters must prove or report missing bounds.
Use high-resolution elapsed checks and completion upper bounds, not floor-rounded
RTOS delays. The [native conformance table](esp-idf.md#local-entry-and-framework-callbacks)
accounts for all17 vtable entries; the [timing table](esp-idf.md#timing-declarations)
keeps the native_wall_time workload-qualification gate explicit.

Current board record after the 2026-10-27 re-pin: MDC40, MDIO39, RESET54,
TX_EN49, TXD0=41, TXD1=42, CRS_DV51, RXD0=52, RXD1=53, REF_CLK44; expected address0,
PHY-supplied 50 MHz, no wired PHY IRQ. Explicitly set every P4 RMII pin because
SDK defaults differ, verify unique pin assignments and confirm final netlist before
flashing. P4 IO_MUX constrains the pads. R21/C24 reset circuitry requires measured
pulse qualification; the 25 us no-debounce-cap figure alone is insufficient.

External power/reset and attempted PHYRCR hard reset close shared admission for
all PHYs while releasing the controller mutex. OP-14 publishes busQuietUntilUs;
owner carries the outstanding floor across end/rebind. CLI discovery must inspect
the bound owner's operation/silence state instead of bypassing core transport.

esp_eth_transmit borrows its buffer for the call only. ESP_OK means queued to DMA,
NO_MEM is backpressure and INVALID_STATE means cached link down. If the default-off
transmit mutex is disabled, use one sending task. RX callbacks own each buffer on
every return, including error, and free it. Context has static lifetime through
MAC deletion and drops/frees after session end without blocking.

## 8. Source selection, API documentation and acceptance

Separate portable host, pinned-driver-on-host, SDK link, package, remote CI and
physical evidence. Stages 37-42 need executing pinned-driver host tests plus a
real ESP32-P4 idf.py build. The opt-in host target, off for consumers, takes
supplied esp_eth.c and its public header set, validates git blob hashes, and
compiles actual driver source with the adapter against small SDK/platform fakes
and an instrumented MAC. Model pinned timer return codes, including stop on an
unarmed timer. It proves install/start/stop/uninstall/ioctl routing, not real
timer concurrency, MAC hardware or timing. Missing source is NOT RUN, no success stub.

Retain source references in [library-sdk](reference/library-sdk.md), public Doxygen,
literal core tests, adverse lifecycle cases and exact result identities. The
2026-11-07 draft re-opened pinned source: esp_eth.c blob
76dab829d2099a574a07aeceeaa82491950fe07c, esp_timer.h blob
e48a213634d3e2cd6ba1dec657687a756d48c291, esp_eth_mac_esp.c blob
6448fcd6de4669ddf1e836c992ca8a1accc9dd96. Install lines200-271, stop323-344,
uninstall274-298, MAC set_link401-419/stop727-742 and timer activity430-444 were
re-read. The rest of this route retains the audit's source-reviewed decision;
no native build, execution or board measurement was performed for these drafts.
