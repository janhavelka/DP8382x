# Additional family and tooling audit

Historical numbering: DP8382x stage numbers in this report refer to the former
34-stage plan. Use the [current 80-prompt index](../prompts/README.md) and
[old-to-new mapping](prompt-workload-audit.md) for execution; its sole first-set
remote-CI checkpoint is now 71. The original findings below retain their dated
scope and are not current execution instructions.

Source inspection on 2026-10-26, supplementing the
[sibling audit](sibling-driver-audit.md). Paths below are relative to Projects.
No sibling files, Git state, build output or hardware were changed.

## Observed snapshots

| Repository | Observed HEAD | State at this observation |
| --- | --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | Clean main |
| 24Cxx | `936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e` | Clean main |
| TUSB32x | `93f2d31374dc16bcc95800d0e0c516c4372faa4a` | Clean execution branch, locally ahead of tracking ref by one |

TUSB32x changed concurrently during this planning session; the sibling auditor
observed an earlier dirty state/revision. Neither observation proves remote
synchronization or a frozen whole-session source snapshot. Future stages re-audit.

## Concrete evidence and decisions

| Inspected source | Actual finding | DP8382x use |
| --- | --- | --- |
| `TMP1x2/include/TMP1x2/Status.h` | Static-lifetime Status with Err/detail/msg and helpers; owner timeout/progress/cancellation differ from bus errors | Adopt value shape and familiar naming with MDIO errors, no I2C aliases |
| `TMP1x2/include/TMP1x2/Config.h` | Borrowed terminal callbacks, explicit model/address validation and owner time | Choose analogous PHY transport/identity rules, not its repeated START or sensor clock semantics |
| `TMP1x2/include/TMP1x2/TMP1x2.h` | Separate configuration, operation and passive health snapshots; explicit start/poll/result | Reuse separation; atomic indirect MDIO is indivisible even if a peer budgets individual callbacks |
| `TMP1x2/docs/code-structure.md` | Core split by actual responsibilities, shared CLI parsing/diagnostic/output sources | Small responsibility-based files; no requirement for a file per register |
| `TMP1x2/examples/common/` | A single CLI used across framework entry points | Same portable processor for host and IDF examples; no two drifting command grammars |
| `TMP1x2/tools/hil_tmp1x2_runner.py` | Finite bounds, exact record parsing, firmware identity, distinct fail/inconclusive, optional serial dependency | Strong runner precedent; source grammar and tests must still be verified during implementation |
| `TMP1x2/tools/check_contracts.py` | Framework include/timing/allocation guards and metadata generation check | Useful supplement to actual portable compilation and behavior tests, not sole proof |
| `TMP1x2/.github/workflows/ci.yml` | Host sanitizers/tests, docs, package and actual Arduino/IDF builds | Reuse check categories; its S2/S3 matrix does not establish an EMAC backend |
| `24Cxx/CMakeLists.txt` | Real C++17 target, tests OFF by default, registered core/CLI/transport/Python/transcript tests | Portable consumption and executable CLI/runner parity |
| `24Cxx/include/EEPROM24Cxx/Status.h` | Logical/preflight vs physical outcomes and distinct busy detail | Preserve evidence semantics while excluding EEPROM/I2C-specific error guesses |
| `24Cxx/tools/` current inventory | Runner, identity/guard/artifact helpers and tests exist | An older TMP comparison saying 24Cxx had no standalone runner is stale; live source outranks historical reports |
| `TUSB32x/docs/prompts/README.md`, `COMMON.md`, `9-transport-identity.md` | 38 staged instructions with shared contracts, current-source audit, detailed failure cases and completion evidence | Use phased tasks/shared contracts and explicit source evidence; choose PHY-specific stages |
| `TUSB32x/AGENTS.md`, `docs/implementation-progress.md`, `test/` | Only passive values/validators and host foundation at observation; driver still planned | Do not treat its future device/owner API as implemented precedent |
| TUSB32x Git/CI sections | Local branch/commit/push permissions and one-CI-stage exception | Initially excluded as sibling-specific permissions; the later explicit user instruction is adapted in the [workflow follow-up](agent-workflow-audit.md) |

## Resulting design requirements

Use permanent DP8382x naming and the common portable header layout, typed Status
and passive snapshots. Keep actual timing, side effects and errors appropriate
to MDIO and Ethernet. Own one complete transaction across core/base/CLI, and
preserve one configuration collector/owner. Ship a real framework-free consumer
and a native IDF component using the supported MAC/PHY framework.

Examples and tools should feel like the sibling libraries: grouped diagnostic
help, strict input, bounded work, accurate firmware identity, explicit disruptive
operations, finite stress, structured artifacts and independently tested verdicts.
Library support and physical board/variant qualification stay distinct.

Do not clone an application task framework, sensor method catalogue, I2C scan,
EEPROM busy polling, sensor-specific transfer counts or a sibling's permissions.
Additional Arduino/MCU integrations require actual supported backends; general
portability is demonstrated first by the C++ boundary and native IDF adapter.

This review ran no sibling tests, SDK builds or hardware campaigns.

## 2026-11-17 Current-code response

Rechecked against `77c544f415edb26a0e23de76591a0163b9b73147`, current
public headers/core/adapter, CMake and all owning prompts. Current sibling HEADs
and clean states are in the [sibling response](sibling-driver-audit.md).
In particular TMP1x2 is `afd50889312f6e218a6a34eca687e3e8547d8b40`,
24Cxx is `1bd9f969d1df55ca5819b0d290ff331869e7d5bc`, and TUSB32x is
`ff8882bff1c3a360efd6bacb43a33d6f13f33dc4`; all were clean when read.
The earlier concurrent TUSB32x observations remain historical, rather than
conflicting current identities.

| Original evidence / recommendation | Current response and concrete location | Disposition |
| --- | --- | --- |
| TMP1x2 Status, terminal callbacks, model/address/time validation | Reopened `include/TMP1x2/Status.h` and `Config.h`. DP8382x `Status.h` and `Config.h` use static messages and borrowed bounded MDIO batches, explicit PHY address/variant, backend detail and owner clock. I2C aliases/repeated START are excluded. `test/foundation.cpp`, `transport.cpp`, `identity.cpp` exercise these differences. | IMPLEMENTED |
| Separate configuration, operation and passive health snapshots | DP8382x shipped facade and `src/DP8382xHealth.h` maintain desired/observed/trust, retained token/result and passive health separately. `test/health.cpp`, `operations.cpp`, `forced.cpp` cover passive getters and invalid/partial evidence. | IMPLEMENTED |
| Small responsibility files, no file per register | Current source list in root CMake uses one core plus configuration/link and actual feature modules. SDK sources are selected separately under `src/esp_idf/`. The historical proposed split names are not mandatory filenames. | IMPLEMENTED |
| TMP1x2 shared CLI / one command grammar | Current peer CMake links common CLI sources and runner tests. DP8382x has portable owner examples but no `examples/common` CLI implementation yet. Prompts 43-47 plus CLI_CONTRACT own the shared processor, grammar and native routing. | OPEN future implementation |
| TMP1x2 runner, identity, finite bounds and parser/verdict tests | Reopened current `tools/hil_tmp1x2_runner.py`: finite serial deadlines, firmware matching and explicit unrun outcomes remain useful precedents. DP8382x prompts 48-52 own producer-correlated sessions, artifacts and verdict failures; peer grammar is not a PHY protocol. | OPEN future implementation |
| Text guards supplement actual compilation | `tools/check_doxygen.py`, version checks and ledger checks exist; root CMake and `test/consumer` compile/link the real portable sources. `test/esp_idf` compiles pinned SDK driver source with bounded host substitutes and separately documents that scope. A source guard cannot establish physical callback bounds. | IMPLEMENTED foundation; later tool checks OPEN |
| TMP1x2 CI categories, no inferred EMAC board support | Reopened its workflow and registrations. DP8382x uses its own P4 SDK-linked fixture, not the peer S2/S3 matrix. No workflow exists yet; prompt 71 owns CI creation/execution. | IMPLEMENTED boundary; CI OPEN |
| 24Cxx C++17, tests OFF and registered driver/CLI/Python/transcript tests | Reopened current `24Cxx/CMakeLists.txt`. DP8382x root CMake has tests OFF by default, explicit sources, real CTest cases and separate tests-OFF `test/consumer`. CLI/runner parity remains prompts 43-52; installed/exported consumption remains 69-70. | IMPLEMENTED build-tree consumption; later scopes OPEN |
| 24Cxx logical versus physical outcomes / busy detail | DP8382x `BatchResult`, effects, release status, operation status and OP-9 restoration debt retain these distinctions with MDIO meanings. `test/transport.cpp`, `operations.cpp`, `reset.cpp` cover late/ambiguous/partial outcomes. No EEPROM busy timing is imported. | IMPLEMENTED |
| 24Cxx standalone runner existed, earlier absence claim stale | Live `tools/hil_runner.py`, `hil_identity.py`, `hil_guards.py`, `hil_artifacts.py` and corresponding tests still exist. This corrects the old inventory only; no peer runner test or HIL was run here. | CONFIRMED inventory correction |
| TUSB32x staged prompts and current-source audits | Current TMP/FieldCore/TUSB contracts remain structural references, not authoritative PHY behavior. DP8382x has 80 stages, mandatory COMMON, EVIDENCE and SOURCE_MAP, and per-stage reports. | IMPLEMENTED plan |
| TUSB32x only passive foundation implemented at original observation | Its current README/core now show implemented driver behavior. The dated claim describes 2026-10-26 and must not describe current TUSB32x support or determine DP8382x APIs. | HISTORICAL; SUPERSEDED as current inventory |
| TUSB32x Git/CI permissions initially excluded | DP8382x root AGENTS now provides its own explicit main-only/checkpoint/first-set-71 rules. Peer permissions still do not transfer. | SUPERSEDED by repository policy |
| Framework-free consumer, native framework reuse and one transaction/configuration owner | `test/consumer`, `src/esp_idf/Phy.cpp`, `PhyControl.cpp` and `docs/design-decisions.md` provide the current paths. Native raw SDK hooks reject bypass; typed owner routes call the same core. Destruction after installation remains source-gated. | IMPLEMENTED subset; teardown OPEN |
| Permanent names, typed Status, truthful diagnostics and physical/family boundaries | CONVENTIONS and shipped headers preserve permanent names and independent health/link/configuration facts. Feature matrix/progress keep future variants, runner, packet and hardware evidence separate. Optional Arduino/other-MCU backends require actual supported implementations. | IMPLEMENTED contract; future CLI/variants/qualification OPEN |
| Do not clone task framework, sensor APIs, I2C scan, EEPROM polling or permissions | Current portable source/CMake has no FieldCore/sibling dependency, allocation, SDK scheduler or board policy. An MDIO discovery operation reads identity at explicit addresses and does not choose a board address. | IMPLEMENTED |

All source/tooling observations above are read-only. Existing stage reports own
past test results; this response does not turn source inspection into new host,
SDK-link, package, CI or physical PASS evidence.
