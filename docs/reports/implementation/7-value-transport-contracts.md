# Prompt 7: values, Status and direct transport contracts

Executed 2026-10-29 on `promptset/dp8382x-implementation-20261028` from clean,
fetched, synchronized checkpoint `49aadf784aa17b996fc07ee967183596ea7bead7`.
Original base remains `5e148fffde074b85fadc15d8ff66fe2c77111036`;
origin remains `https://github.com/janhavelka/DP8382x.git`. The final handoff
records this stage's commit and verified upstream equality. No sibling or vendor
payload was edited; no new execution branch was created.

The [fresh independent audit](7-contract-audit.md) starts from this stage's
synced commit `61e2057590951c740a713786b471228f43d6fbe1` and corrects four
contract/example gaps. Verification below records the original stage; the
audit report carries the corrective evidence.

## Outcome and scope

The [API contract](../../api-contracts.md) freezes exact foundational values and
direct declarations with Doxygen-style/prose behavior. It defines Status and
detail namespaces, declared part versus raw/accepted identity, legal link intent,
borrowed terminal batch data/results, aggregate budgets, local lifecycle,
non-copyable/non-movable ownership, passive health, snapshot validity and direct
read/write/link publication. Every successful live query assigns output, including
down; partial destructive samples survive a failed aggregate.

The typed responsibility map contains exactly 35 feature IDs and all 52 procedure
IDs. Its normative field/legal-value rule includes all 283 ledger fields and
gated recipe actions. This does not invent unfinished feature signatures or
claim raw writes complete multistep behavior. No production header, future facade
stub or cooperative engine was shipped. Prompt 8 owns the remaining operation
state machine, token/acknowledgement and cleanup signatures.

Prerequisites were prompt 6's complete owner/batch architecture and prompts 3–5's
research/feature records. Parent/repository AGENTS, COMMON, CONVENTIONS,
ENGINEERING, FULL_COVERAGE and EVIDENCE governed the stage. Current workflow
inspection again found no `.github/workflows`; existing local research commands
remain applicable. Remote CI dispatch/status/waiting remains prompt 71 only.

The lead owns all changes: API contract, architecture/design links, feature map,
API/example notes, source evidence, prompt-8 dependency, convention note,
README/CHANGELOG, reports/progress and this report. Variants, board guidance,
numeric ledger, test tools and source archive needed no change. Existing source
gates and the prompt-37 SDK correction are preserved.

## Fresh sibling evidence

All worktrees below were clean. Paths are relative to
`C:/Users/Honza/Documents/Projects`; source/tests were inspected, not executed.
Each repository's AGENTS was re-read; its CI/Git rules do not replace this
repository's first-execution policy.

| Repository / revision | Inspected source and deliberate choice |
| --- | --- |
| TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `include/TMP1x2/{Status.h,Config.h,TMP1x2.h}`, `src/{TMP1x2.cpp,TMP1x2Configuration.cpp}`, `test/{variants/test_variants.cpp,helpers/test_helpers.cpp}`. Adopt Status shape/helpers, explicit enum validation, saturating totals, timestamp validity and totals-only resetStatistics. Adapt managed counting to whole MDIO batches. Reject unbind-before-validation, weak IN_PROGRESS-only normalization and zero-threshold normalization. |
| PCA9555 `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | `include/PCA9555/{Status.h,Config.h,PCA9555.h}`, `src/PCA9555.cpp`, `test/{test_basic.cpp,test_public_contracts.cpp}`. Adopt validated terminal completion/effects, failure-preserving bind and non-copy/move traits. Adapt its three health states and physical-I2C counting to the selected four passive MDIO states. |
| INA228 `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b` | `include/INA228/{Config.h,INA228.h}`, `src/INA228.cpp`, `test/test_basic.cpp`. Keep HardwareState/trust independent of health. Reject LATCH_OFFLINE admission policy and the peer's PASSIVE meaning that never reaches OFFLINE. |
| MB85RC `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | `include/MB85RC/{CommandTable.h,MB85RC.h}`, `src/MB85RC.cpp`, `test/{test_basic.cpp,test_public_contracts.cpp}` and `tools/test_hil_health_snapshot.py`. Adopt explicit model/capability membership and distinction from supportedByDriver. Reject AUTO/suffix inference, wrapped totals and zero-as-disabled OFFLINE threshold. |
| SHT3x-main `f65f15573e3c9b1ef04cce72ad08f307d021e563` | `include/SHT3x/SHT3x.h`, `src/SHT3x.cpp`, `test/test_basic.cpp`. Adopt copied cache/restore intent and explicit validity. Reject UNSUPPORTED as an unobserved sentinel and readSettings mutating ordinary output before a later live-read failure. |

Additional consumer/tool audit paths:

- TMP1x2 `examples/common/Tmp1x2CliOutput.cpp`,
  `tools/{hil_tmp1x2_runner.py,test_hil_tmp1x2_runner.py}`.
- PCA9555 `examples/espidf_basic/main/main.cpp`,
  `tools/{run_i2c_hil.py,test_run_i2c_hil_parser.py}`.
- INA228 `examples/esp_idf/basic/main/main.cpp` and
  `tools/{run_i2c_hil.py,test_run_i2c_hil_parser.py}`.
- MB85RC `examples/common/DiagnosticCore.h`, `examples/espidf_basic/main/main.cpp`,
  `tools/{hil_runner.py,test_hil_health_snapshot.py}`.
- SHT3x-main `examples/common/Sht3xCli.cpp`, `examples/idf/basic/main/main.cpp`,
  `tools/{run_sht3x_hil.py,test_run_sht3x_hil_parser.py}`.

Keep health, hardware trust and historical errors separate in presentation;
peer numeric ordinals, parser grammars and "online" vocabulary are not a shared
ABI. The lead independently reopened Status, callback/result definitions,
health tracking, INA health/trust declarations, MB capability metadata and the
SHT cached/live output paths when integrating the inventory.

## Original TI and SDK comparison

TI official main remained `94bf4343004732983b0b8e1eec123d88ff70453b` on
2026-10-29. A bounded latest-five path history check for `dp83826.c` and
`generic_phy.c` found no newer main revision. Original files and byte-verified
blob identities were rechecked under [the core source catalogue](../../reference/library-ti-core.md):

- `rtos_drivers/src/dp83826.c`: `e0b64710350d764fe16ffd089fbdc5246e600039`.
- `rtos_drivers/src/dp83826_priv.h`: `37ab9b94d67687c569ac2e1e36b315fb90eb6749`.
- `rtos_drivers/src/generic_phy.c`: `39414bd3b1004cdf6075bf0f4be30180babdc84d`.

Confirmed actual code: GenericPhy_isLinkUp leaves output untouched when the first
BMSR link bit is low; GenericPhy_getId publishes the first half before a possible
second-read failure; Dp83826_isResetComplete reads the command mask as an address.
The lead read those exact bodies. Original SNLS647I pages 56/57/73 were visually
reopened by the source auditor: zero link is valid, BMSR fault/link consumption
must be retained, model distinguishes BASIC/ENHANCED, revision default 1 is not
universal future acceptance, and PHYRCR address/masks are distinct. No vendor
implementation was copied into MIT code.

SDK examples retain prompt 6's pinned v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643` ownership decisions: actual MAC
transport, rejecting raw hooks, public stopped-driver cache reconciliation,
qualified Boolean pause projection, exact native-token ownership and mandatory
lifecycle correction. No changed SDK baseline or native-support claim occurred.

## Independent review

Read-only agents retained separate outcomes and no writable paths:

- `sibling_audit`: gpt-6.1-sol/high for live vocabulary, validation and
  state/example/test inventory.
- `sdk_audit`: gpt-6-astra/high for exact TI identity/output evidence and the
  exhaustive feature/procedure mapping.
- `ownership_review`: gpt-6-astra/xhigh for independent state/error, output,
  deadline, release and snapshot consistency review.

The lead alone wrote shared declarations and integrated every finding. Review
closed exact successful-frame counts, conservative failed effects, controller-
unsafe status, trusted wrapper quarantine before shared admission, retained raw
invalid status evidence, clock-error precedence/latching/accounting, strict
exclusive deadline admission and publication allowance, inactive setting fields,
fresh mismatch versus stale partial verification, and failed link refresh state.
The reviewer found no remaining architectural blocker; final consistency notes
on publication-reserve health counting and preservation of original callback
reports were then applied. Unused INVALID_BUDGET detail was removed.

Clock-fault cleanup is explicitly carried to prompt 8: timed I/O cannot proceed
on a broken clock, and end/rebind cannot erase owed cleanup/results. The next
stage must define owner-assisted quiescence and acknowledgement. This is the
requested cooperative-lifetime boundary, not a hidden success path here.

## Local verification and remaining evidence

Host: Windows NT 10.0.26300.0, PowerShell 5.1.26100.9711, Python 3.12.10,
MinGW-w64 UCRT g++ 15.1.0.

| Evidence | Result and scope |
| --- | --- |
| Host research tools: `python tools/verify_references.py` | PASS: 27 unchanged vendor artifacts (43,240,385 bytes), 25 extracts, 1,288 local links in 148 Markdown files. |
| Host research tools: `python tools/verify_ledger.py` | PASS: 96 registers, 52 procedures, 35 features; structure/coverage only. |
| Host research tools: `python -m unittest discover -s test -p "test_*.py" -v` | PASS: all 77 tests in 15.564 s. |
| C++17 documentation declaration/example syntax | PASS: six blocks with strict diagnostics and ownership/Status static assertions. No link or execution claim. |
| Typed feature/procedure mapping against JSON | PASS: exactly 35 unique feature IDs, all 52 procedure IDs, 283 fields included by the normative legal-value rule. |
| `git diff --check` | PASS. |
| Host driver / portable-package consumer / SDK link | NOT RUN: working implementations belong to later stages. |
| Remote CI | NOT RUN: reserved for prompt 71. |
| Physical evidence | NOT RUN: no fixture operation or physical qualification. |

The six `cpp contract`/`cpp example` blocks were extracted into one temporary
translation unit and checked with `g++ -std=c++17 -Wall -Wextra -Werror
-pedantic -fsyntax-only`, including static assertions for deleted copy/move and
Status OK/IN_PROGRESS semantics. No code was installed into public headers;
syntax/type checking does not establish driver behavior or a linked consumer.
The feature table was checked against JSON for exactly 35 unique IDs and all
52 procedure IDs, rather than relying on a visual count.

README/CHANGELOG, architecture, design, API/example plan, feature map and source
catalogue were reconciled together. Doxygen-style contracts are in the design;
actual public-header Doxygen checks begin with real code in prompt 9. Existing
test/README commands remain accurate; no new production suite is claimed.

Host driver execution, portable/package consumers, SDK link/lifecycle execution
and physical qualification remain NOT RUN. SDK lifecycle corrections and
existing silicon source gates remain explicit. Remote CI was not dispatched,
queried or awaited; prompt 71 owns that checkpoint. No release or fixture action
was performed.
