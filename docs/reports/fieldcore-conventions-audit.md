# FieldCore-node conventions audit

Audited on 2026-10-26 for the DP8382x implementation promptset. This is source
inspection, not a FieldCore build, test run or hardware qualification. Paths in
this report are relative to the sibling `../FieldCore-node/` unless stated
otherwise. That repository was read-only throughout this audit.

## Observed baseline

- Branch: `rodtester`, tracking `origin/rodtester`.
- HEAD: `d1683149d47fbe7d685ffd600ddaf4eb36281585`.
- The working tree already had changes in RodTester coordinator/capture/runtime,
  production campaign Python tools and tests, plus `.codex-timing-test.log`.
  The checkout is therefore not an immutable clean-source snapshot. Recheck the
  relevant paths before executing a future prompt; do not overwrite or commit
  those changes from DP8382x.
- `AGENTS.md` identifies TunnelMonitor and CO2Control as current WiFi products;
  RodTester is in progress. No present DP8382x/Ethernet integration is established
  by this audit. `include/TunnelMonitor/contracts/Health.h` currently declares
  I2C, RS485, SPI storage, WiFi and E2 bus identities, without an Ethernet entry.

The useful model is the boundary between reusable chip libraries and firmware
owners. DP8382x should be usable behind a similar owner without depending on
FieldCore types, task topology, device IDs, build profiles or product policy.

## Promptset organization that actually exists

| Evidence | Observed convention | DP8382x adaptation |
| --- | --- | --- |
| `docs/prompts/README.md` | Catalog distinguishes active promptsets, deferred work, unresolved decisions and historical reports. Opening a prompt does not execute it. | Provide one clear implementation entry point and an ordered manifest; this preparation does not implement the library. |
| `docs/prompts/rodtester/README.md` | Indexed dependency order, per-step deliverables, prerequisite gates, software versus target versus physical completion, and execution instructions. Lexical filename order is not assumed to be execution order. | Use a simple dependency order, with explicit prerequisites and completion levels. Avoid lettered step complexity unless a real split becomes necessary. |
| `docs/prompts/rodtester/execution_contract.md` | Shared instructions are mandatory with every numbered prompt. They cover authority, source re-audit, bounded ownership, verification, coordination and evidence. | Put repeating driver-specific rules and mandatory sibling-library audits in a shared execution contract, linked from every stage. |
| `docs/prompts/rodtester/source_map.md` | Maps existing owners, intended homes, real consumers, source registration and executing tests. It explicitly warns against generating the whole tree as scaffolding. | Identify public headers, core, adapters, examples, tooling and tests, naming planned homes as planned. Create a file only when its implementation or durable contract has a real consumer/test. |
| `docs/prompts/rodtester/01_contracts.md` | Starts from actual source seams and requires a consumer table before new contract types. Units, time domain, unknown states, capacity and ownership accompany each contract. | Freeze PHY transport/lifecycle/diagnostic contracts before implementation; identify SDK and portable consumers. Do not invent broad interfaces from naming similarity alone. |
| `docs/prompts/rodtester/20a_software_audit.md`, `25a_board_free_handoff.md` | Finite independent audits and a concrete readiness handoff; software completion does not close electrical or physical gates. | End with a requirement-to-evidence audit and a finite hardware qualification matrix; unavailable hardware does not block unrelated software work. |
| `docs/prompts/external_trigger/README.md` | A deferred set explicitly says it is unimplemented, records missing prerequisites, and rechecks dated code assumptions. | Additional silicon variants and optional features must retain explicit unsupported/unqualified status until their actual gates close. |
| `docs/repository_map.md` | Navigation connects an owner to contracts, implementations and focused tests. | Maintain a compact map appropriate to this smaller library; no need to clone FieldCore's entire documentation hierarchy. |

Each implementing prompt should contain: goal and scope; shared required
reading; fresh source/library audit; prerequisites; actual paths and ownership;
concrete deliverables; failure/edge cases; focused and integration verification;
acceptance criteria; evidence and next-step handoff. It should identify which
facts are established and which APIs/files it proposes to add.

FieldCore's policy authority is `docs/guidelines/`; its implementation plan owns
order/status, decisions own accepted choices, and open questions own gaps.
Reports are evidence, not a competing specification. DP8382x can retain its
existing `docs/architecture.md` and `docs/variants.md` as the authority and add a
small implementation plan and decision/gap record rather than moving everything
to match directory names mechanically.

At closure, FieldCore removes obsolete completed prompts only after enduring
requirements and evidence have proper homes, relying on Git for historical
copies. For DP8382x, adopt the principle that prompts are execution instructions,
not the only API documentation or a permanently stale second backlog. Whether
to retain a completed promptset as a reference is a repository decision.

## Runtime ownership and portability evidence

| Concrete source | What is implemented there | Consequence for a PHY library |
| --- | --- | --- |
| `include/TunnelMonitor/i2c/I2cOwnerTransport.h` | Library-free value types and caller-supplied callbacks. `transferOnce`/`probeOnce` receive explicit device, transaction and deadline; observation contains transport outcome, backend error, byte counts, effect and completion time. | Supply a narrow MDIO transport and explicit bounded-error contract. Borrowing context and callback lifetimes must be documented. Do not copy I2C byte-count/NACK/address semantics into Clause 22. |
| `src/devices/i2c/Ina228Module.h`, `.cpp` | One private `INA228::INA228` object; `bind`, `start`, `poll`, `takeResult`, cancellation/invalidation and cached status. Callback translation lives in the module, behind the owner. | A future firmware wrapper should translate DP8382x results to application types. The driver must not depend on firmware contracts or own another bus task. |
| `src/devices/i2c/Ina228Module.cpp`, `prepareLibraryConfig`, `i2cWrite`, `i2cWriteRead` | Library callbacks are wired to the existing owner. Each callback checks the original deadline and delegates one transfer; completion time is checked too. | Preserve original operation bounds through the adapter. For indirect MDIO, serialize the entire address/data sequence even if it includes multiple physical transfers. Do not yield an unprotected multi-register transaction just to imitate the I2C scheduling cadence. |
| `src/devices/i2c/Ina228Module.cpp`, `bind` | Product-specific calibration, thresholds, library timing and health policy are validated by the firmware wrapper. | Board PHY address, reset/clock wiring, chosen 100/full advertisement and recovery policy belong in application configuration, not reusable defaults. These INA228 values are not library-wide conventions. |
| `src/devices/i2c/Ina228Module.cpp`, `refreshStatus`, `snapshot`, `copyStatus` | Status access uses copied cached state; stale projection is separate from actual measurement work. | Make cached observation distinguishable from a live MDIO refresh. Reading CLI health/status must not silently consume interrupt latches or perform diagnostics. |
| `include/TunnelMonitor/device/Ina228Status.h` | Fixed-layout, trivially copyable application status with explicit flags, physical units and last-read uptime. | Use compact bounded snapshots with explicit validity, units, source and timestamp. Never represent unknown speed/duplex/link as a fabricated valid default. |
| `docs/guidelines/runtime_connections.md`, `src/i2c/I2cTask.cpp` | Owners execute hardware work; policy uses commands/results and consumers use published snapshots. Admission, completion and uncertain effects are distinct. | Keep chip operations separate from application retry, outage and recovery decisions. SDK link polling, CLI diagnostics and other MDIO users must participate in the same serialization scheme. |
| `docs/guidelines/i2c_peripherals.md` | Driver modules own chip semantics; generic owner handles bus, queues, deadlines and recovery. Routed transport has explicitly bounded callbacks and retention. | Borrow the ownership principle, not a copied I2C task, mux router, command queue or generic workflow framework. The ESP-IDF Ethernet framework remains the first adapter's owner boundary. |

FieldCore deliberately wraps hardened chip libraries instead of copying their
register protocols into firmware. That supports a reusable DP8382x library
with a thin application wrapper. It does not justify reimplementing ESP-IDF's
MAC, DMA, Ethernet stack or common PHY machinery. The implementation promptset
must settle that reuse boundary explicitly while delivering a genuinely
framework-neutral public/core surface where required by the user.

## Naming, statuses, health and CLI

`AGENTS.md` requires permanent names, namespace `TunnelMonitor`, PascalCase
types/enums, camelCase functions/variables and `kPascalCase` constants. Prompt
numbers and temporary milestone names stay out of runtime files, APIs and CLI
output. Adopt the permanent naming principle with `DP8382x`; do not import the
FieldCore namespace or assume its enum capitalization overrides the actual
sibling-library conventions.

`include/TunnelMonitor/contracts/Health.h` defines health projection as
`Initializing`, `Ok`, `Degraded`, `Fault`, `Unknown`, `Disabled`, with stable
lowercase text. It separately defines `PresenceState` and required/optional
`HealthRole`. `docs/guidelines/time_health_watchdog.md` owns firmware aggregation
and watchdog policy. These are application semantics, not a mandate to put
required-device aggregation or watchdog feeding in every chip library.

For DP8382x, preserve distinct facts: management-transport success, recognized
PHY identity, configuration trust, link state, negotiation progress, speed/
duplex validity, and diagnostic failure. A disconnected cable can be a valid
link-down observation while MDIO is healthy. A valid link does not prove DHCP,
Internet reachability, EtherCAT operation, servo state or application readiness.
Expose enough facts for the caller to make that decision and choose passive
health projection unless an explicit operation requests hardware work.

`docs/guidelines/cli.md` and `src/cli/CliI2c.cpp` demonstrate:

- Compact `help` plus topic help; only implemented commands are active.
- Owner-backed operations submit once and return promptly; copied request/result
  state is printed separately from acceptance.
- Status formatting consumes cached owner snapshots and exposes transport
  failures and queue/operation facts without bypassing ownership.
- HIL-only commands have a distinct availability boundary.

A chip bring-up CLI belongs in examples/common support and platform examples,
not the portable driver core. Reuse sibling-library framing, command naming,
status tokens and parser contracts where observed; a small driver example does
not need FieldCore's topic registry, persistence service or full application
command router. Live register reads, reset, loopback and tests that disrupt the
link must be explicit commands with truthful completion/error output.

## Tests, Python tooling and evidence

`scripts/verify.py` is FieldCore's shared verification gate source and
`.github/workflows/verify.yml` calls it. The workflow pins PlatformIO Core and
tool versions, while `docs/guidelines/focused_verification.md` maps changes to
specific native, Python, application and firmware checks. The Windows
`scripts/pio.cmd` wrapper selects the user's managed installation; its existence
does not authorize installing another Core or copying a personal absolute path.

The stronger reusable practices are:

1. Compile actual library/owner code with scripted transports. Tests must verify
   behavior, fault sequences, lifetime and deadlines, not duplicate the same
   implementation expressions or silently replace real sources with stubs.
2. Register tests and substantive source files in their actual build targets.
   A successful unused test file is not evidence; source selection and final
   consumer linkage need their own proof.
3. Keep Python runners finite: bounded subprocesses/serial I/O, explicit device
   and image identity, machine-readable artifacts and tested parser/verdict
   logic. A fixture transcript tests the runner, not physical hardware.
4. `scripts/hil_run_verdict.py` separates PASS, FAIL, BLOCKED, NOT_RUN and
   COMPLETED. Panic/reset/non-allowlisted errors remain failures; missing scope
   cannot silently count as success. DP8382x should adopt equivalent truthful
   semantics sized for its own runner.
5. `scripts/rodtester_production_evidence.py` and the prompt source map illustrate
   independent decoding/verification of captured output. Include malformed,
   truncated, stale and missing-result cases in PHY CLI runner tests.
6. Reports bind exact source/image/SDK/configuration/fixture and list what was
   actually run. Native, SDK-link and physical results remain separate. Keep
   failures and limits when condensing evidence.

Do not copy FieldCore's full firmware matrix, private dependency credentials,
product profiles, build generator framework or GPIO restrictions into this
library. Build and CI complexity must follow actual supported targets. The
current DP8382x reference checker remains the available check until real build
and test infrastructure is implemented.

## Agent and Git rules to reuse selectively

The RodTester execution contract assigns shared headers, composition and
integration to the lead. Each agent receives a goal, writable paths, read-only
paths, a frozen shared contract, forbidden scope, acceptance cases and a
handoff. Reviewers inspect actual source and executing test evidence, including
the final diff. Disjoint implementation starts after shared contracts settle;
if no safe disjoint edit exists, review is read-only. Agents do not race tests
that mutate the same build directory. These are useful promptset rules.

FieldCore's `rodtester`/`develop` branch names, commit/push/merge permissions,
completed prompt dispatches, fixture approvals and branch-retirement workflow
belong to that repository and session. They do not grant those actions in
DP8382x. Follow DP8382x's own instructions, actual Git state and parent CI
policy. Siblings remain read-only unless the user explicitly requests edits.

## Recommended promptset acceptance checklist

- Every stage re-audits relevant sibling I2C libraries and changed FieldCore
  owner seams, recording why each borrowed convention fits MDIO/PHY behavior.
- Public portable contracts, SDK adapter and application policy have a concrete
  ownership table and real consumer tests before feature breadth grows.
- Core code contains no board pins, network stack, EtherCAT master, servo
  policy, power delivery or FieldCore dependency.
- Health/status/CLI use consistent permanent vocabulary with explicit invalid,
  unsupported, busy, timeout and not-run meanings.
- Examples and Python runners exercise the same real library and expose
  bounded failure behavior, not successful hardware substitutes.
- Native verification, real ESP32-P4 SDK build/link and exact-fixture PHY
  qualification have independent completion rows.
- Family coverage records exact part/revision evidence. Additional suffixes
  and optional features cannot inherit DP83826I support automatically.
- Final audit closes software requirements with actual tests and leaves a
  finite, honest list of unresolved electrical/physical evidence.

No FieldCore source, configuration, Git state or hardware was modified, and no
FieldCore test result is claimed by this report.
