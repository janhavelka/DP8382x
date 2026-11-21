# ESP-IDF examples

Both P4 examples report their configured `uart0` baud in the session identity.
The [console runner](../../docs/hil-runner.md#live-serial-attachment) can check
an already-running example through a qualified serial fixture. Confirm the
active image identity, PHY address and reset-silent wiring before attachment.

## The first native Ethernet application

The [P4 basic example](basic/README.md) now links the actual ESP32-P4 MAC,
DP83826I owner and `esp_netif` DHCP. It reports cached PHY/link/profile and IP
evidence, performs a finite stop/start cycle, and retains installed resources
under the native RX lifetime gate. Its default image never starts hardware;
the saved board netlist leaves the PHY `3V3` supply unproved. Register diagnostics
and the shared CLI remain task 47 work.

The second owner/EtherCAT-boundary example remains planned. Prompt 37 supplies a buildable
[P4 link fixture](../../test/esp_idf/adapter_link/README.md) and
[native component guide](../../docs/esp-idf.md); its execution guard is disabled
and no hardware result is claimed. Use the [board context](../../docs/esp32p4-board.md)
and [official SDK references](../../docs/reference/source-code.md) when implementing
it. Keep EtherCAT master integration in the application that owns that stack.

The [promptset](../../docs/prompts/README.md) also plans an owner example demonstrating the 100/full EtherCAT
integration boundary, followed by a shared diagnostic CLI. Neither supplies an
EtherCAT master or servo-control implementation. Application power delivery is
outside the library and examples.

The basic example owns MAC/controller creation, board settings, PHY address,
clock, scheduling and DHCP. It admits one executing owner per PHY operation,
uses the shared complete-transaction MDIO lock and prints copied snapshots.
It retains installed resources under the native RX lifetime gate and reports
primary and cleanup failures separately. The future shared CLI will attach
without starting hardware work. A separate plain C++ MCU consumer proves core
portability; the basic example links the selected ESP-IDF SDK. Existing sibling
I2C examples do not establish P4 physical link operation.

The [prompt-6 decision](../../docs/design-decisions.md) now fixes these routes:

- Keep native lifecycle/SDK handles inside the application owner. Reserve control
  work, release the instance gate before synchronous SDK calls, and drain callbacks
  without holding locks they need. Timer polling is the only running link poller.
- Register the rejecting raw frame hooks. CLI register/feature commands submit
  typed owner requests to the shared core/batch path; destructive dumps use the
  collector. Forward batches directly to the actual MAC under one shared bus lock.
- Prompt 38 supplies stopped AN/forced configuration, shared reset silence and
  `reconcileSdkState` after successful core result delivery. Public SDK getters
  retain last-published link speed/duplex; expose desired settings separately.
  Use typed `setFlowControl` and retain both MAC/PHY effects, never the defective
  flow ioctl. The linked fixture exercises these routes behind its disabled guard.
- Prompt 39 exposes cached `linkSnapshot` for acquired link/full RX/TX pause,
  separate from `lifecycleSnapshot`'s accepted SDK fields. Keep callback receipts
  and late accepted event effects; retry only the current lost event. Declare
  measured workload/controller bounds before enabling real-time claims; a 100Hz
  tick delay is not proof of a 2ms minimum wait.
- Stop and drain before disruptive configuration. Apply AN/speed/duplex through
  public SDK controls and verify hardware/cache agreement before restart. Full
  advertisement and diagnostics use the shared chip procedures. MAC flow-control
  policy is separate from full portable RX/TX pause resolution.
- Pin unmodified v6.1 and use the implemented native owner lifecycle. A failed
  install publishes no handle; only MAC/PHY deletion remains. After successful
  install, reusable teardown is BLOCKED by native_rx_quiescence: retain the
  driver/glue/netif/MAC/PHY and contexts until restart. Failed stop never licenses
  freeing live contexts. See the native ownership design and prompt 37 report.
- Select GPIO54 reset, MDC40, MDIO39 and REF_CLK input GPIO44 from the current
  board evidence, with declared/verified PHY address 0. RMII uses TX_EN49,
  TXD0=41, TXD1=42, CRS_DV51, RXD0=52 and RXD1=53. Confirm against the final
  schematic/netlist before flashing; qualify the R21/C24 reset pulse and
  power/clock readiness before firmware qualification.
  Do not inherit GPIO5, auto-detection, no post-reset wait or 2000 ms polling as
  correct board defaults. Use elapsed-time checks for the 2 ms/50 ms SMI windows.
  Close shared-controller admission before external power-up/hardware reset and
  through readiness;
  other PHYs must remain silent too, although the bus mutex is released.

The portable owner example will bind two core instances to borrowed batch
contexts sharing one controller lock, explicitly initialize/configure/poll/take
results and publish copied snapshots. It needs no SDK headers, tasks, heap or
FieldCore dependency. This is a design walkthrough, not a completed consumer.

The [value/direct-call contract](../../docs/api-contracts.md) now supplies exact
declarations and a plain-owner example. Bind explicit Variant/interface/address,
borrowed batch and monotonic clock callbacks, and proved time allowances. A live
link query stages the result and assigns valid down on success; failed aggregate
queries leave caller output unchanged while retaining consumed samples. Publish
copied health/link/configuration views separately. `resetStatistics` clears only
managed cumulative totals, and a later transport success does not verify settings.
Native examples must preserve original signed errors and release/clock faults;
the complete stopped-driver SDK transition remains in the owner design. These
are application-example instructions; the separate link fixture proves only
the selected component's real SDK image link.

Owner examples return binding failures to the caller and use the core's staged
output contract directly. Health's last error identifies a counted transport,
release or timing failure; an earlier admission error can still be the public
primary result. Copy the retained transport/collector evidence before subsequent
requests replace the last batch receipt.

The [cooperative example](../../docs/api-contracts.md#operation-acceptance-scenarios-and-examples)
gives the plain-owner path; the [native guide](../../docs/esp-idf.md) owns the
executing SDK callback paths. Retain full primary
and cleanup evidence, keep a token when take fails, and budget native scheduling
inside the existing lifecycle reservation. A cancelled/failed operation may
still require later owner cleanup. A successfully installed owner cannot be deleted under native_rx_quiescence;
stop/retire retains its resources until restart. Never infer RX quiescence from
local end, result delivery or timer callback drain. Tick sleeps
are scheduling only; elapsed-time admission uses the qualified monotonic clock.

Owner contexts are non-copyable/non-movable and retain a reference to their exact
core instance. Native terminal status and the full delivered result must agree.
External invalidation changes current device snapshots without rewriting a
pending historical result or restarting failed/discharged cleanup.

The prompts 37-39 audit makes SharedBus::admission the single silence authority.
Do not overwrite INITIALIZING, BOARD_PREPARING or FAILED_CLOSED to admit another
PHY. Explicit discovery shares core transport counts but does not retarget or
accept identity; show its raw receipt separately from configured device evidence.
