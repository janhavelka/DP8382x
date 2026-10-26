# 15 - Implement the native ESP-IDF PHY adapter

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-14 and the approved stage 03 ownership matrix.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

## Fresh audit and owned work

Re-audit native-IDF component lifecycle/cleanup and error mapping in
INA228/PCA9555/TCA9548A; inspect actual pinned Espressif PHY/base/mediator sources and
compare to the design.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Implement the supported esp_eth_phy_t factory/adapter in platform-only paths. Reuse the
selected IEEE 802.3 base methods for standard behavior and the common chip-specific
implementation for DP83826I handling. Follow the ownership matrix exactly: no second
independent reset, autonegotiation, link polling or MAC notification path. Audit the
SDK's pause negotiation/peer ability and flow-control notification contract; map
resolved RX/TX evidence correctly without enabling application/MAC flow-control policy
implicitly.

Wire borrowed mediator/MDIO transport and complete-transaction serialization across base
MMD, raw ioctls, application operations and CLI. Audit address discovery and reset
semantics rather than inheriting them blindly. Establish lock order and callback
reentrancy, init/deinit/delete lifetime, start/stop handoff and callback error
translation. Document any framework allocation and cleanup outside the allocation-free
core.

Prove underlying timeout behavior for the pinned SDK/MAC. If callbacks lack configurable
aggregate limits, constrain supported operations and document actual finite bounds;
don't pretend the core timeout can preempt an SDK block. Avoid private/version-fragile
APIs unless justified and tested with a narrow version contract.

Create component metadata/source selection without exposing SDK headers to portable
consumers. Use accurate minimum/tested SDK ranges, not a guessed broad semver range. Add
a real linked P4 integration fixture now; it may be small but must instantiate the
actual MAC/PHY framework. No successful hardware shims.

## Concrete implementation route

Apply [CONVENTIONS](CONVENTIONS.md), especially structured `Status`, passive
health, public-header layout and simple engineering language. Reopen
`TMP1x2/examples/esp_idf/basic/main/main.cpp` (`mapError`, `finish`,
`initializeConsole`) and `PCA9555/examples/espidf_basic/main/main.cpp`
(`mapI2c`, `ensureDevice`) as error/lifetime comparisons. Confirm
their current names before citing them. Their I2C error categories, discovery,
controller APIs and S2/S3 matrix do not establish Ethernet behavior.

Use the stage 04 public signatures. A suitable platform layout is
`include/DP8382x/esp_idf/Phy.h` and `src/esp_idf/Phy.cpp`, with one adapter
implementation and a narrow SDK-facing factory. These are proposed paths, not
existing files or a reason to add a second public core facade. Freeze the actual
factory spelling from the chosen SDK contract and expose only the lifecycle and
configuration needed by a real caller. Keep reusable chip procedures in their
existing core modules; do not paste them into each virtual method.

1. Enumerate every method in the pinned `esp_eth_phy_t` and selected base. Map
   each to its executing owner, transaction boundary, backend calls, time bound,
   error translation, side effects and notification. Resolve new SDK methods
   deliberately rather than leaving null or successful no-op entries.
2. Establish mediator binding, owner context and lifetime before any operation.
   Route base reads, chip operations and raw access through the same complete
   transaction entry. Trace nested base-to-mediator calls to prevent recursive
   locking. Record which ordinary base operations need a grouped transaction.
3. Implement factory failure, mediator rejection, initialization failure and
   stop/deinit/delete unwinding. Separate bus-silent object release from explicit
   fallible PHY power/reset actions. SDK allocation stays in the adapter and must
   be paired with release; no callback may access a released context.
4. Test link notification sequencing: unchanged state gives no duplicate event;
   down/incomplete negotiation never supplies invented resolved speed/duplex;
   pause evidence has the correct direction. Preserve `Status::detail` at the
   SDK boundary and explain any information that an SDK return type cannot carry.
5. Trace actual controller wait loops and lock acquisition in the pinned SDK.
   Compute the worst complete indirect transaction and aggregate operation bound,
   including quantization. A late-return test detects a contract violation but
   cannot prove preemption. Leave unsupported bounds visible instead of replacing
   the controller with a successful mock.

Keep the real link fixture small, for example
`test/esp_idf/adapter_link/main/main.cpp`, unless the existing buildable consumer
already exercises the same factory and all required dependencies. Register exact
platform sources in component CMake; an explicit portable source list must omit
them. Inject failure at each acquired resource and each callback/notification
boundary. Check output arguments and retained primary/cleanup errors on failures.

Update public adapter Doxygen for factory ownership, `del`/deinit behavior,
borrowed context lifetime, supported SDK versions, serialization and actual
blocking limits. Update `docs/esp-idf.md`, the adapter ownership table, root
README support/build claims and CHANGELOG in this stage; show one real factory
consumer and the exact local build command. Run the established Doxygen check
for changed public declarations without suppressing warnings. Use an integration
agent for SDK analysis and a separate read-only reviewer for lock/lifetime paths
when useful; keep shared headers and Git with the lead. Select available agents
by reasoning needed, with no fixed model names or agent-specific API variants.

## Acceptance and handoff

Run host adapter contract/fault tests where meaningful and build/link real native IDF P4
firmware. Verify all esp_eth_phy_t methods, delete/error unwinding, no duplicate
notifications and transaction arbitration. Rebuild the plain host consumer to detect
leakage. Report missing SDK as BLOCKED, not simulated success.

Expected output: Native adapter and real SDK integration evidence. Include source
documentation, relevant guides, feature/evidence matrix and progress updates. Apply
COMMON's completion rules; report actual commands, results and remaining gates. Continue
only when the concrete required inputs exist; COMMON explains how unrelated gates stay
open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
