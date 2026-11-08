# Prompt 18 - forced modes and coupled crossover settings

Date: 2026-11-08. Branch: `promptset/dp8382x-implementation-20261028`.
Base: `5e148fffde074b85fadc15d8ff66fe2c77111036`.
Fetched, clean, synchronized entry/prerequisite checkpoint:
`57a06cb12140b8f2666e1b1a46c64ffe80f8035c` (prompt 17).
Final checked SHA and upstream equality are recorded in the handoff rather than
requiring a self-referencing commit.

## Rules, ownership and outcome

Read repository/parent AGENTS, COMMON, selected prompt, CONVENTIONS, ENGINEERING,
EVIDENCE, FULL_COVERAGE, SOURCE_MAP, VERIFICATION, CLI contract and remaining-prompts
audit. Reopened README, architecture, variants/reference index, frozen OP contracts
and actual stage-16/17 lifecycle/configuration code. Owned ledger procedures:
`configure_forced` and `extended_link_configure`. Owned paths are existing public
config/driver headers, configuration and transport/lifecycle source, focused
tests/registration, portable example/consumer and affected source/user guides.
No new production source file, SDK code, sibling change or fixture action.

`LinkSettings` and `startConfigure` now apply complete forced 10/100 half/full
profiles and all documented automatic/robust/manual routing combinations. Both
negotiated and forced profiles own CR1 robust, PHYCR route, coupled CR3 mirror
and CR2 extension fields. Pure validation rejects contradictory inactive intent,
independent mirror-bit requests, forced10 plus extension, forced robust=false,
and forced AN advertisement/pause/page/completion-wait requests before I/O or
desired-state change. Forced manual plus robust is legal. Extension is explicit
owner intent with no invented peer-evidence preflight.

The same operation slot, token, aggregate budget and terminal transaction owner
remain in use. A complete profile saves touched words, applies coupled controls,
activates BMCR and verifies every owned stable field. Normally controls precede
activation; when entering AN with robust disabled, the clear follows AN enable
so the previous forced interval remains robust. Forced never accesses ANAR and
ignores AN-restart readback, which has no meaning with AN disabled. Automatic
PHYCR routing preserves bit14; CR2 changes only its extension bit. No additional
VOD write occurs in configuration. Every initialization reset kind uses the same
profile replay after mandatory VOD setup, including 10 Mbit/s profiles.

Accepted desired intent, observed words, decoded complete observations and trust
remain independent. Complete in-budget mismatch publishes actual hardware fields
with DIRTY trust; a later successful field cannot erase the first mismatch.
Partial or failed verification preserves older tuple/time as stale history. A
forced request observing AN unexpectedly enabled cannot borrow historical ANAR
to form a fresh tuple. A non-IEEE selector cannot establish IEEE technology bits.
Possible writes, late completion and cancellation preserve effects and OP-9's
barrier. Restoration has its own admission/token/result; no automatic rollback,
retry, speed fallback or unrequested forced profile is introduced.

## Fresh sources and independent review

[Silicon review](18-source-review.md) reopens original SNLS647I sections 8.3.2,
8.5.1.10 and exact tables, pins freshly fetched TI bytes and current source
revisions/licenses, and derives literal vectors independently of production
constants. Vendor artifacts remain unchanged. Q11 covers robust manual routing,
mirror interaction and physical forced-status interpretation; it is NOT RUN.

[Peer audit](18-peer-source-review.md) reopens clean current TMP1x2/PCA9555
headers, complete-profile implementations, ambiguity/readback tests, Doxygen,
CLI rendering, runners and registrations. The report records exact paths/pins.
Their shadow/evidence patterns are adapted; sensor timing, I2C ACK rules and
first-write desired adoption do not replace this project's PHY contracts.

Delegation used gpt-6-astra/high for original-PDF silicon and independent source
review, and the current capable model for disjoint tests and sibling/docs review.
The lead wrote shared production changes and integrated all contributions/Git.
Reviewers did not edit production or siblings. Independent build directories
were used. Actual code/diff review found and resolved robust-disable transition
ordering and ignored forced AN-command handling. Lead/peer review also corrected
the inherited observed-tuple publication gap, retained timestamp validity, and
selector-dependent decoding; each has executing regressions.

Final maintainability review retained explicit profile phases, small source-specific
composition helpers, one collector and one configuration/replay procedure. The
public values are reused; no new reset alias, profile factory, register manager
or alternate transport path was added.

## Verification

Environment: Windows/PowerShell, GNU C++ 15.1.0 WinLibs, Ninja, C++17 Release,
strict warnings including conversions treated as errors. Test checks remain
active under NDEBUG. All bus/partner behavior below is an identified host fake,
not measured DP83826I hardware.

| Layer | Result |
| --- | --- |
| Host implementation | PASS: 67 registered CTests, including seven forced suites, all existing negotiated/page suites, and nine executing negative Release probes. |
| Portable consumer | PASS: tests-OFF linked consumer runs generic AN, application100/full AN and manual-MDI forced100/full owner examples plus previous transport/identity/reset examples. No SDK dependency. |
| Research/tools | PASS: 102 Python tests; ledger97 registers/52 procedures/35 features; reference/vendor-hash/local-link and diff checks. |
| Public API/version | PASS: Doxygen153 HTML pages with two rejected negative fixtures; deterministic version read-only check. Version remains0.1.0. |
| SDK/package release | NOT RUN: SDK linking and release-package campaigns retain their later owners; portable linkage does not imply them. |
| Remote CI | NOT RUN: first execution reserves dispatch/status checks for prompt71; no workflow changes. |
| Physical | NOT RUN: Q11 routing/mirror, actual partner agreement, electrical timing and physical link/duplex qualification require admitted identified fixtures. |

Commands:

```sh
cmake -S . -B build/forced18-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/forced18-release
ctest --test-dir build/forced18-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/forced18-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/forced18-consumer
ctest --test-dir build/forced18-consumer --output-on-failure --no-tests=error
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
python tools/check_doxygen.py
python scripts/generate_version.py check
python tools/verify_references.py
git diff --check
```

Independent tests cover 36 forced and24 negotiated routing profiles, automatic
manual-bit preservation, extension/mirror clearing, literal20/24-frame sequences,
AN/forced and10/100 transitions, live robust state after each transition write,
every-frame failure/cancellation, accepted/late writes, per-field mismatch and
later failure, actual observed tuple/history, every reset replay, two PHYs and
short indivisible poll slices. Existing page/pause regressions retain their
semantics and account for the new owned controls. No production mask generates
the literal test oracle.

## Documentation and remaining boundaries

Updated public Doxygen, API/architecture, source catalogue, ledger and feature
coverage, behavior/replay guides, independent vectors, README/CHANGELOG, examples,
test guide and progress. Variants, version metadata and tool interfaces were
reviewed without change: no new part, version, SDK or runner is claimed. Future
Doxygen/CLI/SDK routes must use the same owned profile procedure. The initial
board's100/full AN advertisement remains application data, separate from forced
operation. Configuration proves local stable controls, not partner duplex,
negotiated pause, link-up or electrical compatibility. Live link resolution and
managed health remain later stages; loopback restoration tests belong to prompt23.
Stage18 software is complete with Q11 physical evidence explicitly NOT RUN.
