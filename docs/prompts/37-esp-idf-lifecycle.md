# 37 - Implement the native ESP-IDF factory and lifecycle

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [6](6-framework-ownership.md), [8](8-operation-contracts.md)
and the implemented DP83826I procedures reconciled in
[36](36-primary-feature-closure.md), including transport, initialization,
reset, configuration and link. Record concrete inputs.

Use unmodified ESP-IDF v6.1 through public headers under the current
[ownership design](../design-decisions.md). Re-audit its source-derived
`mac->stop`/`set_mediator` sequence per SDK revision. No SDK fork or unpublished
patch is required. Public-route failures that cannot be resolved are terminal
until system restart; retained resources and limits remain explicit.

## Current evidence and ownership

Reopen current INA228/PCA9555/TCA9548A native component lifecycle, cleanup and
error mapping; compare `TMP1x2/examples/esp_idf/basic/main/main.cpp`
(`mapError`, `finish`, `initializeConsole`) and PCA9555's
`mapI2c`/`ensureDevice`. Inspect the pinned Espressif `esp_eth_phy_t`, IEEE
802.3 base, mediator, driver and MAC source, including actual controller
waits. Record revisions, paths, dirty state and adopted/adapted/rejected
conventions. I2C categories and S2/S3 examples do not establish Ethernet
behavior.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Apply [SDK lifecycle/readiness evidence](../reference/library-sdk.md) and
[EVIDENCE](EVIDENCE.md). Select board address/reset wiring explicitly: generic
SDK defaults are address -1, GPIO 5, reset timeout 100 ms, AN timeout 4000 ms,
assertion 100 us and no default post-reset wait. Enforce the exact chip's 2 ms
reset-release and 50 ms POR SMI readiness, with clock conditions and real
elapsed time. Treat SDK defaults as policies, and TI readiness as silicon
requirements. A declared address is the default; explicit discovery validates
each read status before using IDs and has a total bound (the reference search
can attempt 96 reads).

## Implement

Implement the native factory in `include/DP8382x/esp_idf/Phy.h` and
`src/esp_idf/Phy.cpp`. Use `struct PhyHandle { esp_eth_phy_t phy; Phy* owner; };`
with first-member recovery and a static_assert, not `phy_802_3_t` or its header.
Enumerate all PHY entries, their owner, access, errors and bounds. Bind only the
declared legal address; discovery is explicit, bounded and never selects.
`set_addr` accepts the same value only; retarget by delete/recreate.

Deliver binding, local init/deinit/delete, live link and framework start/stop
integration. The owner resets the board and waits clock-qualified readiness
before install, with no MDIO. Adapter `reset_hw` returns OK without action;
`set_mediator` only stores. `init` is local and `autonego_ctrl(G_STAT)` returns
desired AN without I/O. After install, run typed initialization/configuration
through direct MAC frames inside the core batch lock. Borrowed contexts outlive
all callbacks; no lock needed by a callback is held while draining it.

Basic notifications and controller bounds must already be correct. Do not
defer working callback behavior to task 39. Avoid duplicate link events and
invented speed/duplex on down or unresolved links. Audit actual finite
controller/lock limits; a core deadline cannot preempt a blocking SDK call.
Task 38 expands SDK-aware configuration/raw entry, while task 39 deepens
conformance evidence. Expose implemented owner routes and enforce rejection of
unadmitted raw MDIO at the mediator boundary. Keep control ioctls application-
owned and record their partial-effect limits as specified in task 38. Maintain
explicit availability for routes introduced by later stages.

Use two static raw-register ioctl hooks that unconditionally return
ESP_ERR_NOT_SUPPORTED, including calls from the current owner task. No handle
context lookup is needed for rejecting hooks. Typed owner raw access uses the
core policy and direct MAC frame methods inside the batch; no recursive ioctl
or mediator acquisition. Test real ioctl rejection with zero MAC frames.

Trace install/start/stop/uninstall partial effects in one record:
`{stage, firstError, cleanupError, publishedTuple, publicationUncertain, eventLost}`.
One phase enum (Stopped, Control, Starting, Running, Stopping, Maintenance)
controls entry. PHY reset/pwrctl return NOT_SUPPORTED and point at typed operations.
Any SDK-called AN/speed/duplex/pause/loopback callback completes as one terminal
batch without a wait, or rejects and points at the typed route; no callback
owns a token/poll loop.

The pinned installer ignores set_mediator/reset_hw results, so make those local
and infallible as above. MAC init needs an existing PHY REF_CLK and performs
10 ms-step reset polling up to sw_reset_timeout_ms (default 100 ms). Board reset
failure is handled before install. An install error leaves only mac->del and
phy->del; there is no partially installed handle route. After successful install,
typed initialization failure uses ordinary uninstall with retained evidence.

Add exact component metadata/source selection and a small real P4 firmware
fixture, such as `test/esp_idf/adapter_link/main/main.cpp`, instantiating
actual MAC/PHY framework dependencies. The portable source list excludes SDK
files.

Apply the pinned lifecycle order and qualify it in Q13:

1. Create an adapter-owned ESP_TIMER_TASK one-shot fence before installation.
   On esp_eth_start failure call esp_eth_stop; an unarmed timer returns
   INVALID_STATE after FSM STOP and may omit link-down/STOP events.
2. After every stop, arm the fence once with esp_timer_start_once(fence,0), then
   task-delay/poll esp_timer_is_active to false under a finite deadline, outside
   the timer task. Retry by polling, not rearming. Timeout retains resources.
3. After the fence, if UP was published call adapter set_link(DOWN). If START
   was posted but STOP was not, post ETHERNET_EVENT_STOP using &hdl,sizeof(hdl)
   and finite ticks. Then call mac->stop to observe its HAL error, with finite
   retries and no held lock; each call may busy-wait 2500 us.
4. Stop all transmit callers. If netif attached, wait for the owner's STOP
   handler under a finite deadline, then delete netif glue. Swap the MAC mediator
   to a static stub: both stack-input callbacks free buffers and return OK,
   on_state_changed returns OK, register hooks return NOT_SUPPORTED. Do not use
   on_lowlevel_deinit_done. Wait for application RX in-flight count zero.
5. Uninstall driver, then mac->del, phy->del, delete fence, destroy netif last.
   Track FSM/ref-count admission since SDK exposes no public getter. A failure
   this public route cannot resolve leaves admission closed and resources alive
   until reboot. The HAL FIFO-flush timeout is unobservable, a documented limit.

The stub swap is unsynchronized with the RX task: a task paused after loading
emac->eth can still touch the freed driver. No public API closes this window;
the RX counter proves application callback completion only. A pre-handoff delete
can leak at most one frame. These are SDK limits, not claims a host fake proves
safe concurrency. When the selected baseline includes d2ed96d2, drop the timer
fence and re-audit the blocking-stop behavior.

ESP-IDF tag v6.1 / commit fff9895c is installed side by side in C:\esp\v6.1. Run
idf.py only through `scripts\idf.cmd`, which checks the pinned commit; if it is
absent, run `C:\esp\install-idf.ps1 6.1`. Never install ESP-IDF globally or set
IDF_PATH/IDF_TOOLS_PATH: PlatformIO in C:\pio uses its own ESP-IDF 5.5. Record
path, commit and tool versions; later stages reuse it. Installation failure
makes SDK evidence BLOCKED, never replaced by a header shim.

Implement opt-in pinned-driver host tests (off for consumers) per VERIFICATION:
hash-check supplied esp_eth.c and its seven public headers, compile real driver
source plus adapter with small OS/event/timer fakes and an instrumented MAC.
The timer fake preserves stop-on-unarmed INVALID_STATE. This proves driver order
and ioctl routing, not timer concurrency/MAC timing. Acceptance needs these
failure tests plus the real P4 idf.py image link.

## Acceptance

Inject factory/resource, mediator, initialization, callback and notification
failures; verify unwinding, unchanged outputs, retained backend detail and no
released-context access. Build/link the real pinned SDK fixture and rebuild
the plain host consumer. A shim cannot replace missing SDK evidence.

Document factory callbacks, lifetime, errors and bounds in Doxygen and
`docs/esp-idf.md`; update the operation-ownership table and component build
guide.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
