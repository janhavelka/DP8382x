# Prompt 14: exact identity and capability selection

Executed 2026-11-07 after the requested 45-minute wait: delay began at 21:16:22
UTC and implementation began after 22:01:22 UTC. Continued execution branch
`promptset/dp8382x-implementation-20261028`, clean and fetched/equal at
`4bc8e043890d9aaf601879a459b7db64cd7f6320`. Original base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`. Final commit and verified upstream
equality are recorded in the handoff to avoid a self-referencing commit.

## Rules, prerequisites and ownership

Read root/parent guidance, COMMON, prompt 14, CONVENTIONS, EVIDENCE,
FULL_COVERAGE, ENGINEERING, VERIFICATION, SOURCE_MAP and the relevant API
contracts. Checked README, architecture, variants and reference index. Namespace/
class DP8382x, PascalCase types and camelCase methods remain unchanged. One
application owner serializes the instance and snapshots; the existing terminal
batch owns the controller across both identity frames. No automatic discovery,
family factory, retry, SDK dependency or address selection was added.

Prompt 13 supplies the tested access policy/collector/transport; prompt 3 supplies
the exact identity ledger. The only ledger procedure assigned to stage 14 is
`identify`; it is now implemented. The lead owns public headers, source, shared
guides, integration and Git. Focused tests and source review were disjoint.
No workflow exists yet; first-execution remote CI remains prompt 71 only.

## Source evidence and selected conventions

The [fresh peer review](14-peer-source-review.md) records current clean revisions
and actual declarations, implementations, tests/registration, CLI rendering,
Python runners, agent rules and workflow files for MB85RC, TMP1x2 and PCA9555.
All siblings remained read-only. Adopt exact declared-model validation and
invalidated permissions after rejected reidentification. Adapt byte-completion
staging to two MDIO frames. Reject ACK/configuration plausibility and sensor or
memory identities as a PHY oracle. No sibling code was copied.

Independent source reviewer `identity14_sources` used gpt-6-astra/high, selected
under AGENTS for silicon/validity reasoning. It reopened original SNLS647I tables
8-21/22, pp56–57; table 4-3, p3; BASIC strap table 8-12, p51; and SOR2 table 8-88,
p90. The current official [TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
is byte-identical to the archived August-2025 revision I: 3,366,322 bytes,
SHA-256 `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Vendor bytes/provenance were not modified.

The current official TI repository still resolves to
`94bf4343004732983b0b8e1eec123d88ff70453b`; the
[dp83826.c source](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c)
has blob `e0b64710350d764fe16ffd089fbdc5246e600039`. Its BSD-3-Clause notice remains
applicable to that source; no implementation was imported. Lines 51–54/147–160
filter OUI 080028, models 11/13 and revision 1. Lines 163–183 accept MII/RMII;
the adjacent RGMII-support comment is incorrect against the exact datasheet.

Independent original-table literals are BASIC `2000/A111` and ENHANCED
`2000/A131`, with OUI assembled from both words. Both modes support MII and RMII;
BASIC+RMII is legal. Shared IDs cannot discover I/E suffix, temperature grade,
actual interface straps or A-generation support. Revision 1 is a bounded exact-
part acceptance policy, not a claim that other revisions do not exist.

## Implementation and failure behavior

The existing public driver header now ships `IdentityAcceptance`, `DeviceIdentity`,
`Capabilities`, `readIdentity`, `identitySnapshot` and `capabilities`. One existing
source file implements them. A private exact-part revision table supplies policy;
there is no caller-configurable revision bypass or premature relative enum.

`readIdentity` uses one batch of reads at Clause 22 registers 2 and 3. Ordinary
output publishes only on complete terminal success and accepted identity. The
snapshot retains independently valid raw prefix words from this attempt only.
Complete trustworthy words may be decoded on terminal failure, but cannot be
accepted. Partial/rejected values, timestamps, acceptance and staleness are
explicit. No identity word is synthesized from a prior call or reset default.

Zero in either required word takes precedence over FFFF; these produce
DEVICE_NOT_FOUND/ZERO_ID or ONES_ID without claiming physical absence. Other
complete candidates check OUI, model, revision and required mode in that order,
returning DEVICE_MISMATCH with its precise detail. Failed transport/release,
deadline and malformed reports retain their primary status/detail instead of
being reclassified as an identity mismatch. Counts, no-retry and exclusion remain
at the existing terminal boundary. A private callback-executed flag avoids using
an old receipt after local rejection, including when counters have saturated.

Capabilities are bus/clock silent and invalid until fresh identity acceptance.
Accepted DP83826I reports four 10/100 modes, MII/RMII and copper; fiber/gigabit
are absent. These foundational source facts do not claim complete typed feature
software, initialization, link-up, managed health, SDK or physical qualification.
Rebind/end clear identity; semantic writes and external invalidation retain old
bytes but make them stale and withdraw capability validity.

Review identified two admission/invalidation issues and they were fixed:

- Changed or failed identity acquisition must invalidate older device observations
  and configuration evidence. Accepted mode change installs a fresh identity while
  the older samples remain stale and configuration dirty.
- Existing owner-attested maintenance must not override a known failed/rejected
  probe. All five maintenance entry points now return that retained error before
  any clock/MDIO, including WoL preharvest. Invalidation does not erase it; accepted
  reprobe or explicit rebind clears it. Never-probed and stale-success maintenance
  exceptions remain unchanged, separately from fresh typed capabilities.

Identity shares `_execute` with raw reads. No reset operation/state exists at
stage 14, so the application still establishes external reset readiness and bus
silence. Stage 15 introduces OP-14 reset-wait/hard-floor admission at that shared
boundary with real reset-consumer tests. No unused operation state or current
claim of enforced future reset waits was added.

## Tests, independent review and documentation

`identity14_tests` used the current capable model for only `test/identity.cpp`.
Five suites reuse the shared protocol controller with an identity-specific fixture;
expected silicon literals are independent of production constants. They cover
both interfaces/modes, every model/revision encoding, both OUI portions, invalid
declarations, future/alias identities, all protocol addresses, second-read failure,
signed backend details, release/clock/deadline/report failures, exact counts,
unchanged output, cached silence, mode changes, mutation rejection, invalidation,
reprobe/rebind and two-instance contention/reentry. The five suites execute
34,608 explicit checks in Release. A registered negative probe proves they remain
active under NDEBUG.

`identity14_peers` used the current capable model for peer research and read-only
production review. The stronger source reviewer then independently reviewed the
final actual implementation, tests, example, registration and guide; no confirmed
actionable gaps remained. Reviewers used separate build directories where needed
and did not mutate Git. The lead reviewed actual contributions and final diff.

Updated Doxygen, [identity/decision guide](../../identity.md), variants, API,
architecture, direct/side-effect guides, source comparison, feature ledger/matrix,
test guide, progress, README and CHANGELOG. The portable probe is compiled and
executed against the real driver with an explicit simulated backend. Version
metadata is unchanged; generator/version commands need no change. SDK, CLI,
runner and package implementations remain later work and acquire no new support
claim. Doxygen now explicitly requires the identity/capability type pages.

## Verification and checkpoint

Environment: Windows PowerShell, GNU MinGW-w64 15.1.0 x86-64, CMake 4.0.1,
Python 3.12.10, Doxygen 1.13.2. Strict C++17 Release builds use conversion/sign
warnings as errors and active CHECKs. Reproduction commands:

```sh
cmake -S . -B build/identity14-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/identity14-release
ctest --test-dir build/identity14-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/identity14-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/identity14-consumer
ctest --test-dir build/identity14-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/verify_references.py
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

| Evidence layer | Result |
| --- | --- |
| Compiled portable host | PASS: 33 registered CTests, including five identity suites and five active-check negative probes. |
| Portable build-tree consumer | PASS: tests OFF; direct, MMD, evidence and exact probe examples link and execute without SDK/tool generation dependencies. |
| Python research/tools | PASS: 102 tests, including unchanged independent identity ledger literals. |
| Ledger | PASS: 97 registers, 283 fields, 52 procedures and 35 families; identity implementation/host evidence updated separately. |
| Reference/link integrity | PASS: archived bytes/hashes and final local documentation links verified. |
| Public documentation | PASS: 86 generated HTML pages, local assets/links and two rejected negative fixtures. |
| Version / diff | PASS: generated header current, whitespace clean. |
| Installed/exported package | NOT RUN: later packaging stages. |
| SDK link | NOT RUN: native implementation remains prompts 37–39. |
| Remote CI | NOT RUN: reserved for prompt 71. |
| Physical | NOT RUN: actual part/revision/address/strap measurements and MDIO timing require an admitted fixture. |

Final maintainability review retained one chip procedure and transport path,
fixed values, a small private revision table and one mutation-admission helper.
Cached data and ordinary output remain distinct; no hidden I/O, independent lock,
discovery registry or copied sibling silicon rules were introduced. Source and
physical gates outside identity remain unchanged. The lead commits this complete
checked unit and verifies same-branch upstream equality in the handoff.
