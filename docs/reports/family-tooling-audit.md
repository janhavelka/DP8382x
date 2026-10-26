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
