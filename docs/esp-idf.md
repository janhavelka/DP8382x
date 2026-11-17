# Native ESP-IDF PHY owner

The native adapter in [Phy.h](../include/DP8382x/esp_idf/Phy.h) uses the existing
C++17 core through the public `esp_eth_phy_t` interface. Its baseline is
**ESP-IDF v6.1, commit `fff9895c82d744c7237be8847347bdd1b07c6643`, ESP32-P4**.
The component metadata selects that release and target. This is a selected
integration baseline, not a tested range of SDK releases or MACs. Executing
results and remaining gates belong to the prompt-37 implementation report;
the [source audit](reports/implementation/37-native-source-audit.md) records
the inspected inputs. A linked image does not establish physical operation.

## Component and application boundary

The root CMake component explicitly includes portable source files and
`src/esp_idf/Phy.cpp`. Ordinary host CMake includes only portable files and has
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

| Entry | Executing owner and access | Result and bounds |
| --- | --- | --- |
| `set_mediator` | Installer; stores the borrowed mediator locally | Infallible for the admitted installer call; no MDIO or wait. The SDK ignores its result. |
| `reset_hw` | Installer; local no-action callback | OK; board reset/readiness already completed. The SDK ignores its result. |
| `init` | Installer; local initialization only | OK for admitted install; no PHY I/O. Typed initialization follows install. |
| `deinit` | Failed-install local unwind; installed destructive teardown is denied | Installed teardown closes admission and returns INVALID_STATE; retain resources because the SDK may already have changed its timer/refcount. |
| `get_addr` | Reads immutable declared address locally | INVALID_ARG for null output; no MDIO. |
| `set_addr` | Compares locally with the immutable address | Same value returns OK; another value returns NOT_SUPPORTED. Retarget by recreation before installation or system restart after retained installation. |
| `autonego_ctrl(G_STAT)` | Installer/local query; returns admitted desired AN | No MDIO; invalid output rejects. Prompt 37 admits AN configuration only. |
| `autonego_ctrl(RESTART)` | Framework during STARTING; same core restart procedure | One terminal three-frame batch, no wait/token/poll loop; propagates controller/configuration failure. Success means submission/readback, not AN completion. |
| Other `autonego_ctrl` commands | SDK configuration request | NOT_SUPPORTED in prompt 37; prompt 38 owns public SDK-cache reconciliation and mode transitions. |
| `get_link` | Framework initial STARTING collection or RUNNING timer; same core collector | Bounded ten-frame live observation and qualified notifications. No invented mode for down/unresolved links; unchanged tuples emit nothing. |
| `set_link(DOWN)` | Admitted framework/owner stop projection | Administrative DOWN without PHY status reads; notification errors and accepted effects are retained. Other phases reject. |
| `set_link(UP)` | No unsolicited administrative UP route | NOT_SUPPORTED; only qualified live evidence can publish UP. |
| `reset`, `pwrctl` | SDK control request | NOT_SUPPORTED; use an admitted typed owner operation. These callbacks never reset or power hardware. |
| `set_speed`, `set_duplex` | SDK configuration request | NOT_SUPPORTED at this stage; use the SDK-aware owner routes delivered by prompt 38. |
| `advertise_pause_ability`, `loopback` | SDK control request | NOT_SUPPORTED at this stage; no partial PHY procedure or hidden wait. Later typed owner admission must coordinate these effects. |
| Raw register read/write hooks | Static mediator-boundary rejection, including owner-task calls | Unconditional NOT_SUPPORTED; no context lookup, MAC frames or output mutation. |
| `del` | Local never-installed destruction only | Rejects installed/active objects or a live fence. The borrowed MAC/bus are never deleted by the PHY. |
| `custom_ioctl` | No callback installed | Null; chip extensions belong to typed owner entry points. |

The owner exposes stopped `startInitialize` for `NONE`, `PCS` and `RESTART`,
AN-only `startConfigure`, `pollOperation`, `takeResult`, `cancel`, local operation
and lifecycle snapshots, and policy-checked stopped `readClause22`/`readMmd`.
These routes reuse existing procedures and retained results. Initialization
always applies/verifies mandatory VOD settings; it can succeed with cable down.
Core operation cleanup remains a barrier and a separately budgeted restoration,
not implicit recovery. Native HARD initialization and forced-mode configuration
reject until the coordinated shared-bus/SDK-cache routes are delivered.

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
MAC batches, then admits `start` only for verified normal AN settings.

The single admission phase is STOPPED, CONTROL, STARTING, RUNNING, STOPPING or
MAINTENANCE. Framework START changes its own state before all later steps can
succeed. Every start failure therefore attempts SDK stop and preserves the first
error separately from stop/fence/publication cleanup. Initial `get_link` proves
the SDK posted START before entering that callback. The unarmed-timer stop path
changes the framework state and returns INVALID_STATE without publishing STOP.

After a stop, arm the precreated ESP_TIMER_TASK one-shot fence exactly once,
then task-delay/poll until inactive under a finite deadline, outside the timer
task. A retry polls the same fence; it does not rearm or claim an unobserved STOP
event. After the fence, publish missing administrative DOWN, post missing STOP
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

`TransportBounds` states caller-qualified acquisition/release overhead, per-frame
execution and clock lag. `lockWaitUs` caps requested controller-mutex waiting;
the declared fixed overhead must cover that configured acquisition allowance.
`callbackBudgetUs` is an aggregate allowance; publication reserves the positive
caller-qualified `notificationUs` for each mediator call, checks elapsed time,
and retains accepted/uncertain effects when a later failure or deadline occurs.
Declarations must cover the actual selected backend and scheduling assumptions.
The adapter cannot preempt a blocking SDK call or turn post-return detection
into proof of a wall-clock bound.

Live-link admission reserves five notifications before acquisition, covering a
changed UP tuple's DOWN/SPEED/DUPLEX/PAUSE/UP sequence. Callback and lifecycle
deadlines remain absolute through the admitted work.

The pinned MAC MDIO loop uses 100 us steps with a nominal 1000 us accumulated
poll allowance and no MDIO mutex. Its separate multi-register MAC mutex has a
100 ms wait, including notification control paths. Actual stop uses nominal
2500 us busy polling; instruction/scheduler overhead is additional. Controller
failure or uncertain release quarantines the shared domain. A later instance
must receive a bus-silent quarantine denial rather than silently repairing it.
Application repair is separate from passive transport health and link state.

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
