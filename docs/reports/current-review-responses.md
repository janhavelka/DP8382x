# Current responses to repository audits

Reviewed 2026-11-17, starting from clean synchronized `main` at
`77c544f415edb26a0e23de76591a0163b9b73147`. The user requested every audit file's
findings and proposed fixes be checked against current code/prompts, datasheets
and sound engineering, with necessary corrections and a response in each file.

## Scope and decisions

The filename inventory found 30 audit records: 29 under this reports tree plus
REMAINING_PROMPTS_AUDIT. Every record was read in full and now contains a dated
current-code response covering its findings/recommendations and rejected fixes.
The rolling promptset disposition table covers all 197 unique IDs:172 G findings,
M-1, six N additions and 18 R contract-review findings. Fifty-two rows needed
current-status/evidence updates. The original remaining-prompts audit retains
its historical text, with a prominent pointer to the current dispositions.

Three other matching filenames are task definitions, not reports of findings:
[prompt 1](../prompts/1-ecosystem-core-audit.md),
[prompt 2](../prompts/2-ecosystem-tooling-audit.md) and
[prompt 76](../prompts/76-docs-api-audit.md). Their relevant requirements were
checked against COMMON and current scope; this review does not execute future
prompt 76 or add artificial findings to those instructions. Other stage/source
reviews were consulted as supporting evidence. No numbered stage is advanced.

Confirmed corrections:

- The [public source catalogue](../reference/source-code.md) still recommended
  generic IEEE-base inheritance/helper reuse. It now points to the selected
  owned PHY wrapper, shared core batches and unconditional raw-ioctl rejection.
- [Prompt 27](../prompts/27-interrupt-service.md) still assigned IRQ mux writes
  to the pin-profile stage. It now matches the actual single interrupt owner;
  pin profiles validate dependencies instead of creating a second writer. The
  ledger, coverage/direct-access guides and historical stage note agree.
- The [initialization guide](../initialization.md) overstated its baseline frame
  counts and said no RCSR mutation occurs. It now excludes all owned optional
  settings from those counts and describes actual collector-routed RCSR replay.
- [CLI_CONTRACT](../prompts/CLI_CONTRACT.md) and prompts 44-46 prescribed a second
  probe binding. They now use shipped `readIdentityAt`/native `discover`, retaining
  common accounting, clock and shared-bus admission. Ordinary output and cached
  partial receipts stay distinct; local denial cannot claim a fresh acquisition.

No additional production defect was confirmed. Implemented fixes were retained;
obsolete CLEANUP-engine/base-wrapper/alias advice was explicitly superseded.
Historical review counts and discarded prototype claims are not current evidence.
Exact-part authority, failure-output validity, one collector/transaction owner,
separate restoration and explicit source/physical gates govern the responses.
No generic practice was used to override a chip field table. Future CLI/runners,
variants, packaging and CI remain assigned requirements, not shipped features.

## Review ownership and evidence

The lead owns shared documentation/prompts, the rolling dispositions, integration
and Git. Three parallel reviewers had disjoint audit-response paths; production,
shared contracts, SDK and sibling files were read-only to them. gpt-6-astra/high
reviewed silicon/state/restore claims; gpt-6.1-sol/high reviewed foundation/native
contracts and historical research/tooling respectively. These choices match the
reasoning and source-inventory work. The lead checked reported gaps against
actual current methods/tests and reviewed all contributed response sections.

Primary evidence includes original archived SNLS647I access/reset/timing/WoL
tables, SNLA428 recipes, SNLS783A generation deltas, SNLS638C role/access/timing
conflicts, SNLS505H FX/RGMII/REGCR/cable tables and applicable TI notes. Original
PDF layout was inspected where columns/access encodings matter. Selected TI
driver sources and the actual unmodified SDK pin
`fff9895c82d744c7237be8847347bdd1b07c6643` were reopened. Exact sources, sibling
revisions and applicability limits are recorded in each relevant response.
No vendor payload, sibling repository or SDK source was modified.

## Current verification

Environment: Windows/PowerShell, GNU 15.1.0, CMake 4.0.1/Ninja, Python 3.12.10.
SDK checks use only `scripts/idf.cmd` with C:/esp/v6.1/esp-idf and the existing
P4 fixture; no global environment/toolchain change or fixture operation.

| Layer | Result | Current evidence |
| --- | --- | --- |
| Portable host | PASS | Fresh independent Release configure/build and 236/236 CTests, active assertions, build/audit-current-response-silicon |
| Native host | PASS | Fresh independent Release configure/build and 63/63 CTests, both tick rates and 45-file SDK closure, build/response-foundation-native |
| Build-tree portable consumer | PASS | Separate tests-OFF configure/build and 2/2 CTests, build/audit-response-consumer |
| Python/source ledger | PASS | 103 Python tests; 97 registers / 52 procedures / 35 features; no structural/source-record drift |
| Documentation/version/references | PASS | Doxygen with negative fixtures, generated-version check, archived hashes/local links, diff check |
| Response inventory | PASS | 30 dated responses and exactly 197 distinct G/M/N/R rows, no missing or duplicate IDs |
| Actual SDK image | PASS | P4 adapter fixture linked with the unmodified v6.1 pin through scripts/idf.cmd; post-commit build and clean identity belong to the final handoff |
| Installed/exported consumer | NOT RUN | Still future packaging work; build-tree checks do not qualify an archive |
| Remote CI | NOT RUN | First-execution prompt 71 rule; no status query or dispatch |
| Physical qualification | NOT RUN | No flash, traffic fixture or Q1-Q19 measurements |
| Reusable installed teardown | BLOCKED | native_rx_quiescence; installed resources retained until restart |

No production inputs changed, so sanitizer reruns were not required for these
documentation corrections; the prior audit's UBSan result retains its own SHA.
Source-gated DPD, pin/electrical branches and unqualified native wall time remain
open. The final handoff records the verified commit/upstream equality without
embedding this file's own commit hash.

## Individual audit responses

Each linked file contains its own dated response and concrete issue dispositions.

| Audit record | Response |
| --- | --- |
| [agent-workflow-audit.md](agent-workflow-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [family-tooling-audit.md](family-tooling-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [fieldcore-conventions-audit.md](fieldcore-conventions-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/13-18-independent-audit.md](implementation/13-18-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/2-ecosystem-tooling-audit.md](implementation/2-ecosystem-tooling-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/21-22-independent-audit.md](implementation/21-22-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/23-24-independent-audit.md](implementation/23-24-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/25-26-independent-audit.md](implementation/25-26-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/27-28-independent-audit.md](implementation/27-28-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/3-core-ledger-audit.md](implementation/3-core-ledger-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/3-source-audit.md](implementation/3-source-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/30-31-independent-audit.md](implementation/30-31-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/32-33-independent-audit.md](implementation/32-33-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/34-35-independent-audit.md](implementation/34-35-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/36-independent-audit.md](implementation/36-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/37-39-independent-audit.md](implementation/37-39-independent-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/37-native-source-audit.md](implementation/37-native-source-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/38-native-source-audit.md](implementation/38-native-source-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/39-native-source-audit.md](implementation/39-native-source-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/4-5-silicon-ledger-audit.md](implementation/4-5-silicon-ledger-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/7-contract-audit.md](implementation/7-contract-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/8-contract-audit.md](implementation/8-contract-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [implementation/9-10-foundation-audit.md](implementation/9-10-foundation-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [phy-implementation-audit.md](phy-implementation-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [prompt-correctness-audit.md](prompt-correctness-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [prompt-workload-audit.md](prompt-workload-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [promptset-audit-dispositions.md](promptset-audit-dispositions.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [public-library-data-audit.md](public-library-data-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [sibling-driver-audit.md](sibling-driver-audit.md) | Revalidated individually; current-code response appended or current per-ID table updated |
| [Remaining prompts audit](../prompts/REMAINING_PROMPTS_AUDIT.md) | Section 12 plus all 197 per-ID dispositions |
