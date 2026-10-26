# 27 - Prove independent application and framework consumption

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-26 software outputs; unsupported variants remain
unavailable, not silently simulated.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

## Fresh audit and owned work

Inspect TCA9548A standalone consumer, TMP1x2/24Cxx ordinary C++ examples and FieldCore
module adapters. Recheck real platform support before selecting any additional firmware
example.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Build an ordinary C++ owner consumer using only installed/exported public headers and
the actual core. Demonstrate borrowed transport, caller time, complete transactions,
initialize/configure/observe, passive projection, cancellation/recovery admission and
two independent PHY instances. A scripted host backend is explicitly simulation, not
hardware support.

Provide a compact firmware integration guide showing how a FieldCore-like module can
translate Status/health/link snapshots without core dependency on its contracts or task
topology. Explain MDIO owner identity, MAC framework cooperation and shared read-clear
collection. Do not edit FieldCore-node or claim it already has Ethernet support.

Rebuild minimal and CLI native-IDF consumers. Test lifecycle and include/source
selection. Verify consumer code does not reach into private headers or sibling trees.
Confirm examples cover ordinary networking and the 100/full EtherCAT application
boundary separately.

Evaluate Arduino/another MCU adapter only against an actual requested or supported
MAC/MDIO backend. If feasible and in scope, implement/test a thin adapter using the same
core/CLI; otherwise document precise unimplemented platform scope. Framework neutrality
is required; advertising fictional S2/S3 EMAC examples is not. Avoid a generic backend
catalogue created solely for appearance.

## Concrete standalone owner and application mapping

Apply [CONVENTIONS](CONVENTIONS.md). Inspect
`TMP1x2/test/core_no_arduino/compile_main.cpp`, `TMP1x2/CMakeLists.txt` and
`24Cxx/CMakeLists.txt` for exported header/source selection and tests-OFF behavior.
Reopen `FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`, its borrowed
`I2cOwnerTransport` and current `DeviceModuleStatus`/result contracts. Use
`mapLibraryStatus`, `mapJobEffect`, `snapshot` and `copyStatus` to explain explicit
translation and cached projection; do not copy their I2C or sensor policy into
DP8382x or edit FieldCore-node.

Create one useful ordinary application under `examples/portable/basic/` with a
small `CMakeLists.txt`, `main.cpp` and README, adapting an existing consumer if
already present. Link target `DP8382x` through its public usage requirements;
do not include `src/` files, private headers, sibling directories or test fixture
headers. Keep host script/fake transport clearly named and local to the simulated
example. A real MCU backend requires separate actual hardware evidence.

The running example must demonstrate the stage 04 API in a direct sequence:

1. Construct two independent driver instances with distinct borrowed contexts and
   caller-owned monotonic time. Explain callback lifetime and complete transaction
   ownership, including a shared-controller lock when both instances share MDIO.
2. Explicitly initialize/configure one instance, advance the admitted operation
   with finite owner budgets, retain its terminal result and obtain passive
   `HealthSnapshot`/`LinkSnapshot` and configuration evidence. Do not hide a
   blocking convenience loop behind a method named poll.
3. Show legal generic 10/100 policy versus a separate 100/full application check.
   Demonstrate cancellation/recovery admission and primary/cleanup uncertainty
   without silently replaying writes or resetting either instance.
4. Inject a backend failure on one instance; verify its counters, status detail
   and trust change while the other instance remains independent. Demonstrate
   cached rendering with zero frame delta and failed reads preserving outputs.
5. Release results and resources in the documented order. Reject rebinding or
   context destruction while owned work/cleanup still references the context.

Add `docs/firmware-integration.md` with an example translation table covering
`Status::{code,detail,msg}`, progress, transport health, configuration trust,
link validity/resolution and application readiness. Preserve backend detail;
static message lifetime does not make it a stable machine error code. Explain
how a FieldCore-like module can own `start`/`poll`/retained results and copy cached
snapshots without firmware contracts in public DP8382x headers. Do not prescribe
new tasks/registries, sensor timestamps or existing Ethernet support for FieldCore.

Configure/link/run with tests OFF in a clean consumer build using only public
artifacts. Verify no configure-time Python/Git/Doxygen/SDK dependency. Run a
second available compiler with the same source and report availability honestly.
Stage 28 owns release-package extraction policy; this stage proves independent
application consumption and must not claim that unbuilt package is already
validated. Build both actual P4 applications separately and inspect source lists
for accidental portable/SDK mixing.

Document example helper ownership and bounds in Doxygen, maintain the portable
README and integration guide, and update root README consumer claims and
CHANGELOG. Run source documentation checks and verify documented includes,
commands and example output. A disjoint agent can implement/review the ordinary
consumer after public contracts are frozen; use independent reasoning review for
mapping/state validity. The lead owns any shared API correction and integration.

## Acceptance and handoff

Compile/link/run the ordinary tests-OFF consumer with a second host compiler when
available. Build real native-IDF images and inspect dependencies. Check zero sibling/SDK
requirements for core consumption. Report host portability, SDK integration and physical
MCU support independently.

Expected output: Independent portable consumer and useful application integration
guidance. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
