# 80 - Close final implementation and qualification claims

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Inputs are actual implementation, packages,
guides, [78](78-review-silicon.md) and [79](79-review-ownership-evidence.md)
findings, prompt 71's CI evidence and 72–73's physical results or explicit gates.
A partial result can be reviewed; overall COMPLETE requires every required
software/evidence gate actually closed.

## Current evidence and ownership

Reopen `../TCA9548A/tools/check_package.py` and `tools/test_hil_artifacts.py`,
TMP1x2 `tools/check_doxygen.py` and registered host/variant suites, and SCD41
`tools/check_target_package_consumer.py`. Inspect final current source,
generated files, CLI/help, Python runner, workflows as source, CONTRIBUTING/AGENTS
and maintained guides. Record revisions, dirty state, paths and adopted/adapted/
rejected conventions; historical handoffs alone do not prove the final result.

The lead owns final contracts/claims/progress, integration and Git. Delegate
read-only claim-to-artifact or clean-consumer journeys with explicit paths,
acceptance and available models suited to complexity; review findings. No agent
mutates shared contracts/Git/siblings or operates fixtures incidentally. Do not
add layers/API aliases for superficial checklist uniformity.

## Concrete public-source inputs

Reconcile [public-library evidence](../reference/library-evidence.md) using
[EVIDENCE](EVIDENCE.md): record which source observations were adopted, corrected,
rejected or remain unresolved, and link the resulting independent tests and
actual measurements. A complete list of addresses is not complete implemented
features. Do not erase source conflicts, vendor defects or missing hardware
qualifications to obtain a final passing status.

## Work and acceptance

Review the finished ordinary C++ consumer, native P4 adapter, network and EtherCAT
ownership examples, shared CLI, Python runner, packages, documentation and
complete exact-part source/register/field/procedure coverage. The source ledger
defines the denominator, not exported functions. Every documented programmable
feature/legal setting requires implemented typed behavior and independent tests;
opt-in defaults and missing first-board fixtures do not excuse software gaps.
Hardware-only/inapplicable/outside-PHY dispositions need citations. Essential
unresolved silicon, implementation, adapter, CI or requested physical evidence
prevents overall COMPLETE.

Create this prompt's report with scope, command, input revision/hash, environment,
result/artifact and limit for each distinct evidence layer:

| Layer | Final accounting |
| --- | --- |
| Software | Exact part/revision/interface/medium, full ledger and remaining rows |
| Host | Executing counts, independent vectors, strict/Release-active checks, CLI/Python negative and restore cases |
| Portable/package | Ordinary consumer run, artifact identity/content, extraction-only link/run, tests OFF and no Git/Python/Doxygen/sibling requirement |
| SDK | Actual SDK/target/example/configuration and linked image identity |
| Remote CI | Prompt 71's exact SHA/jobs/URLs/results and later substantive changes outside that result |
| Physical | Exact 72–73 fixture/part/mode evidence, failures and untested features |
| Application | Actual network/frame ownership scope, separate master/servo timing and power delivery |

Select final local verification by changed inputs and stale evidence. Rebuild
extraction consumers after export/source fixes, real SDK links after adapter/
public/build changes, behavior suites after logic changes and strict docs/links
after declaration/prose changes. Reuse artifacts only with matching inputs and
dependencies; a package before a fix or old green CI does not cover final code.
Do not repeat unchanged full matrices or dispatch/poll/wait for remote CI here.

Reconcile README, CHANGELOG, Doxygen, API/architecture/variants/feature/testing/
package/hardware guides, CONTRIBUTING, validation and progress. Preserve vendor
bytes/notices and failed evidence. Keep a finite pending list with dependency,
owner and exact next command/fixture; continue eligible work and report partial
status when required gates remain. Record no-change reasons for unaffected guides.
Check final links/diff and checkpoint coherent verified fixes with tests/docs.
Commit/sync on the same execution branch: fetch, safely integrate, push, final
fetch, local/upstream equality and zero ahead/behind, plus remaining dirty task
or unrelated work. Failed push stays explicitly unsynced with local commit/error/
next step. No empty review commit or self-referencing hash loop. Hand off precise
supported behavior, examples/tools, commands/results, CI versus final local
identity and remaining limits. No tags, publication, sibling edits, flashing or
fixture authority is implied.
