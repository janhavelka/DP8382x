# 31 - Reconcile user-facing guides and feature coverage

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-29 implementation outputs; consume stage 30's actual
evidence or explicitly recorded block.

## Fresh audit and owned work

Re-audit peer README/integration/testing/feature coverage organization and FieldCore
source-map distinction between requirements and historical reports.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Document the complete register/field/procedure ledger and public research basis.
State full software feature coverage only when every required programmable row
is implemented; keep unsupported-silicon, hardware-only, unavailable-fixture and
unfinished-software rows visibly distinct. Explain runtime opt-in defaults without
describing unused first-board features as optional implementation scope.

Reconcile README, architecture, variants, API/integration, CLI, examples, testing,
hardware-validation, changelog and reference notices with actual implementation. Remove
outdated planning claims only when the behavior exists. Keep unqualified/unimplemented
features explicit and link durable documentation rather than making users read prompts.

Give concise ordinary C++ and native-IDF quick starts using actual buildable code.
Explain application ownership of pins/address/clocks/locks/time/recovery, generic 10/100
options and separate 100/full profile. Document live vs cached status, no-link vs
transport error, configuration uncertainty and how a FieldCore-like owner maps them.

Document package/component consumption, supported SDK range, backend bounds, firmware
identity and runner commands. Provide ordinary Ethernet networking and EtherCAT master
integration boundaries without pretending the library contains either stack. Explain
optional/disruptive diagnostics and variant restrictions.

Maintain a requirement-to-source/API/test/CLI/runner/evidence matrix with software and
physical columns. List unresolved silicon interpretations, adapter limits and remaining
hardware fixtures with concrete next steps. Avoid blanket production-ready or
whole-family support language.

Keep prompt execution history in progress/reports and permanent behavior in maintained
guides. Do not erase failure evidence or vendor source provenance while cleaning stale
planning text.

## Acceptance and handoff

Build documented snippets/consumers, exercise CLI/runner help and check all local
links/version names. Trace every advertised feature and supported variant to
implementation and tests. A stage 30 block remains visible; documentation completion
does not close it.

Expected output: Accurate usable guides and complete claim-to-evidence mapping. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.
