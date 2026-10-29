# Prompt 6: portable core and native SDK ownership

Executed 2026-10-29 on `promptset/dp8382x-implementation-20261028`, continuing
the original base `5e148fffde074b85fadc15d8ff66fe2c77111036`. Clean entry
checkpoint `e42343b0baf26367fcb22d70d51f4a47744a81b8` was fetched and equal
to the same-name upstream. Remote remains `https://github.com/janhavelka/DP8382x.git`.
The final handoff records this stage's verified commit and sync, avoiding a
self-referencing commit. No branch, sibling or vendor-payload changes occurred.

## Outcome and prerequisites

The [design decisions](../../design-decisions.md) and
[architecture](../../architecture.md) select one implementable C++17/core/native
boundary with a complete public-entry/call-chain matrix. One terminal batch
callback serializes complete MDIO/MMD/RMW across the bus; one collector retains
destructive evidence. SDK callbacks, cached AN, pause projection, notifications,
raw denial, installation context, shared-bus silence, lifecycle and finite cleanup
have named owners. There is no second generic PHY stack or placeholder driver.

Inputs were prompt 2's actual consumer/tool findings and prompts 3–5's ledger,
procedure/coverage records, independent vectors and unresolved gates, including
the fresh e42343b audit. Parent/repository AGENTS, COMMON, ENGINEERING,
CONVENTIONS, VERIFICATION, EVIDENCE, FULL_COVERAGE and relevant source mapping
were re-read. Research completion does not close `forced_physts`, EEE or other
existing silicon gates. The stage is a design deliverable, not native support.

Owned files: design decisions, architecture, API-location plan, native-example
notes, board source context, SDK source catalogue, feature-coverage note,
device-behavior/ENGINEERING clarification, prompt-37 dependency, poison
probe/runner, test guide, README/CHANGELOG,
reports index, this report and progress. The lead alone edited and owns Git.

## Current sibling audit

Paths below are relative to `C:/Users/Honza/Documents/Projects`. All five
reference worktrees were clean at inspection. These are inspected sources/tests,
not claims that their test suites or firmware were executed here.

| Repository / revision | Reopened paths and selected conventions |
| --- | --- |
| TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `include/TMP1x2/{Config.h,TMP1x2.h}`, `src/{TMP1x2.cpp,TMP1x2Operations.cpp}`, `docs/owner-operations.md`, `test/{jobs/test_jobs.cpp,bus/test_bus.cpp}`. Adopt borrowed terminal transport, zero-I/O admission, retained token/results, deadline checks and conservative completion anchoring. Adapt I2C callback granularity to full MDIO batches. Reject `bind` unbinding before replacement validation and unconditional result discard. |
| PCA9555 `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | `include/PCA9555/{PCA9555.h,Config.h}`, `src/PCA9555.cpp`, `test/{test_basic.cpp,test_public_contracts.cpp}`. Adopt validate-before-rebind, `pollOperation`, `ObservedState` vs intent, retained partial observations and primary/cleanup distinction. Tests at 1019/2120/2194/2580 cover partial observations, cleanup and owed release. Reject automatically granting a fresh full I2C timeout after the primary deadline. |
| TCA9548A `ce425e1984ac269e7934dd811a06ec08b91e307c` | `include/TCA9548A/{TCA9548A.h,Config.h}`, `src/TCA9548A.cpp`, `test/test_driver/test_driver.cpp`. Adopt late-write uncertainty and explicit separate cleanup grant (test line 465); reject hardware-touching begin/no-op tick and single-register assumptions as core lifecycle defaults. |
| INA228 `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b` | `examples/esp_idf/basic/main/{main.cpp,Ina228IdfI2cTransport.h,Ina228IdfI2cTransport.cpp}`. Adopt retained bus/device/temporary-removal handles until successful release and primary error surviving cleanup. Reject a global diagnostic transport as native multi-instance context. |
| FieldCore-node `5ad9292dc8481cf08be9a61f662495a6474d1da0` | `src/devices/i2c/{Ina228Module.h,Ina228Module.cpp}`, `include/TunnelMonitor/i2c/I2cOwnerTransport.h`, `docs/guidelines/{ownership.md,runtime_connections.md}`, `src/cli/components/Ina228Cli.cpp`, `test/native/test_component_ina228/{test_ina228_module.cpp,test_ina228_cli.cpp}`. Adopt owner-private instance/context, immutable deadline, post-transport time check, cached `snapshot`/`copyStatus`, request-vs-status CLI and independent instances. Reject application-specific absent/fault/staleness policy as PHY health. |

Each repository's `AGENTS.md` was re-read with the parent guidance; its Git/CI
rules do not replace this execution's first-set exception. Additional current
example/CLI/runner evidence was inspected:

- TMP1x2 `examples/common/{Tmp1x2Cli.cpp,Tmp1x2Cli.h}`,
  `examples/esp_idf/basic/main/main.cpp`,
  `tools/{hil_tmp1x2_runner.py,test_hil_tmp1x2_runner.py}`: queued console input
  and owner execution are useful; unchecked IDF remove/delete errors are not.
- PCA9555 `examples/{common/CliShell.h,espidf_basic/main/main.cpp}`,
  `tools/{run_i2c_hil.py,test_run_i2c_hil_parser.py}`: preserve shared CLI owner
  routing; reject discarding handles after unchecked removal in `ensureDevice`.
- TCA9548A `examples/common/Tca9548aCli.cpp`,
  `examples/espidf_basic/main/main.cpp`,
  `tools/{tca9548a_hil.py,test_tca9548a_hil.py}`: retain old/replacement handles
  and block admission after uncertain cleanup; preserve lost-framing and
  unverified-restoration outcomes.
- INA228 `tools/{run_i2c_hil.py,test_run_i2c_hil_parser.py}` and FieldCore
  `scripts/{hil_i2c_startup.py,hil_cli_commands.py}`: finite sessions and actual
  firmware identity precede qualification; no copied I2C result proves MDIO.

Repository workflow inspection found no `.github/workflows`, CMake project,
package manifest or production public headers. Prompt 2's unchanged workflow/
registration findings remain historical evidence at their recorded revisions;
current runnable DP checks were inspected directly. No remote CI was dispatched,
queried or awaited.

## Original SDK and silicon evidence

All seven original SDK sources required by the prompt were reopened and their
blob identities checked: `esp_eth_phy.h`, `esp_eth_phy_802_3.h/.c`,
`eth_phy_802_3_regs.h`, `esp_eth.c`, `esp_eth_driver.h` and `esp_eth_mac_esp.c`
at v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`. Timer, HAL and P4
low-level sources were additionally traced. Paths, symbols, line anchors, blobs
and bounded official-fix comparison are in the maintained
[SDK evidence](../../reference/library-sdk.md#prompt-6-source-re-audit-and-selected-disposition).
Actual original sources, not catalogues alone, drove the design.

The comparison kept the baseline unchanged. Official master fixes for blocking
timer stop and reset delay rounding were found, but do not close the finite
lifecycle contract; observed master retains the other listed defects. The
selected path therefore requires a reviewed pinned SDK correction in prompt 37.
It is explicitly a future dependency, not an invented existing SDK API.

The lead rendered original SNLS647I pages 15, 34 and 36: timing columns and
no-debounce qualifier, entire-bus reset/POR silence, 16-bit SMI and literal
`0x001F`/`0x401F` nonincrementing MMD procedures. Existing detailed register
tables/vectors and gate dispositions remain unchanged. Source archive hashes
are preserved. Board `docs/ethernet-mcu.md` was re-read in
`EasyEDA/ESP32P4X-EasyEDA-PCB` at clean revision
`6f8333ef04caa785be374478d0cd413b8e055ed1`: GPIO54 reset, GPIO49/43 MDIO
controls, GPIO44 reference clock, no PHY IRQ and R21/C24 reset network. This
document comparison neither operated the design application nor qualified wiring.

The new C11 poison experiment uses the external original SDK register header,
checks blob `e9091f63a39ac1861bb043b5f25373cbb0d749f0`, and reads only
initialized objects. It reproduces the SDK's partial assignments over poison,
then tests complete numeric control/data construction. This proves the encoding
hazard on the identified host/compiler; it does not execute a PHY driver, fake
success, claim a native link or replace later production invalid-width tests.

## Independent review and disposition

Three read-only subagents had separate bounded outcomes and no writable paths:

- `sibling_audit`: gpt-6.1-sol/high for cross-source API/example/test inventory.
- `sdk_audit`: gpt-6-astra/high for pinned SDK state, controller bounds and
  install/timer failure paths; it also checked bounded official fix history.
- `ownership_review`: gpt-6-astra/xhigh for an independent challenge of the
  complete draft, stronger effort for concurrency, retained effects and lifetime.

The lead integrated and reviewed their findings; no agent changed Git or shared
sources. Review corrected five issues before acceptance:

1. Shared SMI silence now has controller-domain admission before external
   power-up/hardware reset, without a held mutex during readiness waits.
2. A failed link notification may itself have started MAC/DMA; projection freezes
   until owner stop/drain/reconciliation rather than retrying UP blindly.
3. Synchronous SDK work transfers only its own exact core token/result into the
   enclosing native journal; application results cannot be stolen or overwritten.
4. Mediator detach precedes driver free and needs supported MAC correction;
   current null setters are not assumed valid.
5. One physical P4 EMAC owner is distinct from two instrumented MACs in SDK
   dispatch/lifecycle tests and from another PHY on a shared controller.

The reviewer rechecked all five and found no architectural blocker, with final
wording corrections for detach order and external versus MDIO-issued reset
scope. Those wording corrections were applied. The lead checked the full matrix
against the prompt: no unowned entry or second chip poll/configuration owner;
no controller lock through waits/notifications; no unresolved lock-order choice.

## Verification and limitations

Local environment: Windows NT 10.0.26300.0, PowerShell 5.1.26100.9711, Python
3.12.10 and MinGW-w64 UCRT GCC 15.1.0. Commands ran from this repository on the
recorded execution branch.

| Evidence layer | Actual result and scope |
| --- | --- |
| Reference/link integrity | PASS: `python tools/verify_references.py`; 27 vendor artifacts, 43,240,385 bytes, 25 text extracts, 1,266 local links in 146 Markdown files. |
| Ledger | PASS: `python tools/verify_ledger.py`; 96 registers, 52 procedures, 35 features; structure/coverage only. |
| Host research tests | PASS: `python -m unittest discover -s test -p "test_*.py" -v`; 77 tests, 14.782 seconds. |
| SDK encoding experiment | PASS: `python tools/probe_sdk_mmd.py "$env:TEMP\dp8382x-sdk-audit-fff9895\eth_phy_802_3_regs.h"`; original header hash verified, C11 `-Wall -Wextra -Werror -O2 -DNDEBUG`; 256 control + 16 data cases, 192 poisoned partial-control hazards reproduced. |
| Probe negative fixture | PASS: a modified temporary header returns 1 before invoking a deliberately nonexistent compiler; the test checks the expected hash-mismatch diagnostic. |
| Diff | PASS: `git diff --check`; no whitespace errors. |
| Host driver behavior | NOT RUN: no production driver exists; source/encoding research is its own layer. |
| Portable/package consumer | NOT RUN: no implemented core/package to build. Plain C++ use and source selection were reviewed as design call chains. |
| SDK firmware link/lifecycle execution | NOT RUN: no adapter/corrected SDK fixture exists. Installation/failure/two-instance paths were source-reviewed only. |
| Remote CI | NOT RUN: reserved for prompt 71; no workflow exists here and no run was dispatched/queried. |
| Physical PHY/board/timing | NOT RUN: no fixture operation, flash or measurement performed. |

The SDK correction is a BLOCKED native-support dependency until implemented and
verified in prompt 37. This does not block completion of the requested design.

Documentation review updated README/CHANGELOG, architecture/design, API/example
notes, board provenance, feature/source coverage, test guide and prompt-37
dependency. Variants and numeric ledger remain correct and unchanged. No new
public declarations require Doxygen yet; future documentation duties and actual
build/fixture acceptance are explicit. No release or hardware action was taken.
