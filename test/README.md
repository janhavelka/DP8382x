# Verification

## Current workspace

Run from the repository root using Python 3.10 or newer:

```sh
python tools/verify_references.py
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
git diff --check
```

The checker verifies every archived file's size and SHA-256, PDF signatures,
ZIP members and CRCs, derived-text hashes, archive inventory, and local Markdown
file links. It is offline and does not claim that TI has not published an update.
PDF parsing was also checked while collecting the initial archive.

The ledger validator checks the [part-specific schema](../docs/registers/README.md),
complete source inventory, masks, aliases, reset knowledge, citations and ownership.
`test_ledger.py` is registered through standard-library unittest discovery. It
tests valid/malformed temporary files and independently transcribed original-PDF
vectors, including every core field partition, access type and the pause matrix.
It also rejects missing core procedures, unproved MMD aliases and masks outside
their procedure feature's register coverage. Assertions
are unittest methods and remain active under Python optimization. Run both
commands as prompts 4–5 extend the ledger. These are research-tool tests, not
executing driver, package or hardware tests.

`test_diagnostic_ledger.py` extends registered discovery with original-PDF field,
access and reset literals for every prompt-4 detailed record, recipe actions and
legal encodings. Temporary malformed files prove rejection of dropped diagnostic
rows/procedures, invented/retargeted actions, changed recipe addresses and lost
required action ownership.

`test_remaining_ledger.py` covers every remaining detailed reset/access partition,
LED placement, odd-nibble polarity, pin legality, analog split fields, W0C neutral
writes, MAC/password ordering, nominal EEE timing and exact recipe actions.
Its independent oracles match every semantic field's mask/access type and all
24 remaining procedure ownership maps. Split-field arithmetic covers all 289
valid endpoint pairs, legal intermediate codes and restoration prefixes.
Malformed temporary files reject incomplete source coverage, erased gates,
borrowed exceptions, altered selected EEE wire mappings and reclassified software scope.
The 80 ledger tests above are research-tool evidence only.

`test_operation_timing.py` adds seven executable operation-contract design probes for
strict aggregate admission, whole MMD frame allowances, post-callback wait
anchors, tick quantization, certain versus uncertain expiry, uint64 overflow,
finite stalled-clock polls and the four-lag hard-reset floor. They use
independent synthetic values and integer arithmetic, not a fake PHY or unused
operation engine. The [operation scenarios](../docs/api-contracts.md#operation-acceptance-scenarios-and-examples)
remain execution-ready call/status/effect recipes for the first real consumer.
Together with two structural catalog/policy checks and 13 version-tool tests,
current Python discovery contains 102 tests. These Python tests do not claim driver execution;
the C++ transport suites below exercise the compiled driver separately.

## Portable C++17 foundation

The root `DP8382x` CMake target provides five standalone public headers and links
the portable direct/indirect implementation. `DP8382X_BUILD_TESTS` defaults to OFF.
With tests ON, four foundation CTest suites exercise
Status helpers, valid/invalid link settings, register-mask operations and passive
transport values. Five extra translation units compile each public header alone.
Five transport suites (`binding`, `direct`, `reports`, `timing`, `ownership`)
execute the production facade with independent literal access vectors and a
controller fault-injection model. They exhaust all Clause 22 addresses, all
16-bit LEDCR values and all primary status byte encodings, plus report/release,
deadline, output-sentinel, no-retry, lifetime and two-instance ownership cases.
Five MMD suites (`traces`, `admission`, `failures`, `reports`, `ownership`) bring
the original subtotal to 14 behavioral CTest suites. They execute literal four-frame access
and five-frame stable RMW, every selector/address/data failure, complete next-call
reselection, competing users between frames, alias policy, reserved/recipe/EEE
rejection, aggregate budgets and retained effects. The shared controller fake
models selection per PHY while one lock excludes all PHY addresses. Deliberate
corrupt reports test the driver's second check, not a conforming backend example.
The tests use explicit checks that stay active in Release builds.
The commands below use CTest 3.26 or newer so an empty test set fails;
the portable target itself supports CMake 3.16 or newer.

```sh
cmake -S . -B build/host -DDP8382X_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
cmake -S test/consumer -B build/consumer
cmake --build build/consumer
ctest --test-dir build/consumer --output-on-failure --no-tests=error
python tools/check_doxygen.py
```

The separate consumer configures the root library with tests OFF and links the
real portable driver plus compiling direct-read and atomic-MMD examples, using an explicitly
simulated backend. The Doxygen command requires installed Doxygen; it generates
to a temporary directory, checks documented pages/members/links and verifies
that missing public comments and a bad `@param` fail. Missing Doxygen means the
documentation check is NOT RUN/BLOCKED, never PASS. These host results do not
execute a physical PHY or SDK. The controller model cannot qualify lock/controller
timing or physical MDC/MDIO waveforms; see the [backend requirements](../docs/direct-access.md).

## Side-effect access and shared harness

Nine `access` suites exercise literal command-neutral composition, all-source
acquisition, each destructive failure phase, later-batch failure, MISR updates,
WoL W0C behavior, admission, cached invalidation and cross-owner exclusion.
They retain partial samples and raw counter arithmetic without inventing a
qualified event total. Their literals come from original tables, independently
of production constants. Four registered WILL_FAIL probes prove Release checks
remain active, bringing CTest to 27 tests. `support/Check.h` is shared by all
executables; `support/FakeController.h` supplies the transport/MMD controller.
The independent access fake models clear effects and pending wake state.
`test_access_policy.py` checks catalog/ledger and raw-access coverage structurally;
it is not the numeric oracle. The tests-OFF consumer also links
`examples/portable/EvidenceAccess.h` and exercises the actual collector/MISR API.

## Exact identity

Five `identity` suites (`decode`, `policy`, `failures`, `lifecycle`, `ownership`)
test literal BASIC/ENHANCED IDs, every model/revision code, both OUI portions,
all protocol addresses, zero/FFFF precedence, partial and malformed reports,
release/clock/deadline failures, cached query silence, negative-evidence write
admission, mode changes, invalidation, rebind and two-owner exclusion. A fifth
registered negative probe proves active Release checks; CTest now has 33 tests.
The tests reuse the shared controller's ownership/descriptor engine with an
identity-specific fixture. The tests-OFF consumer also executes
`examples/portable/IdentityProbe.h` against the real linked driver.

## Release version tooling

`test_version_tool.py` is registered by the Python discovery command at the top
of this guide. Its temporary repositories check valid boundary versions, strict
JSON/schema/version failures, unchanged committed output on invalid input or
injected write failures, no-write `check`, import silence and idempotent `sync`.
The committed generated `Version.h` is also compiled alone with strict warnings
and included by the tests-OFF CMake consumer. Run these from the repository root:

```sh
python scripts/generate_version.py sync
python scripts/generate_version.py check
python -m unittest discover -s test -p "test_*.py" -v
```

The second `sync` must leave the generated header unchanged. The
[tool guide](../scripts/README.md) defines the exact version syntax and the
separate future firmware Git-identity rule. Neither CMake configure nor the
consumer invokes the generator.

Prompt 6 also supplies an isolated C11 SDK encoding probe. Obtain the original
[pinned register header](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/include/eth_phy_802_3_regs.h)
outside the repository (an existing exact SDK checkout is sufficient), then run:

```sh
python tools/probe_sdk_mmd.py /path/to/eth_phy_802_3_regs.h --compiler gcc
```

The tool rejects an unexpected Git blob hash before compiling, copies only that
header into a temporary build directory, and runs
[`sdk_mmd_poison.c`](sdk_mmd_poison.c) with C11, warnings-as-errors, optimization
and `NDEBUG`. Explicit checks remain enabled. It tests 256 poisoned control and
16 data-word cases against the SDK's actual C types, reproducing 192 partial-
control initialization hazards and checking zeroed reserved/upper bits for
complete construction. Compilation/execution have 30/10-second limits. It is a
separate source-evidence check, not one of the Python ledger tests, a production
MMD implementation or a linked SDK image. Production width/admission/failure
and two-instance SDK tests remain assigned in the
[ownership acceptance matrix](../docs/design-decisions.md#8-source-selection-api-documentation-and-acceptance).

No firmware build, CI workflow or physical hardware test exists at this stage.
The [80-prompt promptset](../docs/prompts/README.md) now defines those future
deliverables. Its [verification contract](../docs/prompts/VERIFICATION.md)
separates host, portable consumer, SDK link, package, remote CI and physical
evidence. The [prompt 9](../docs/reports/implementation/9-portable-source-foundation.md)
and [prompt 10](../docs/reports/implementation/10-version-tooling.md) reports
record current source/tool checks. The older
[planning validation](../docs/reports/promptset-validation.md) records only
the planning-stage documentation checks.

## Planned implementation evidence

- Host tests: exact register transactions, identity/variant checks, advertisement
  masks, error propagation, bounded reset completion and partial-write failures.
- SDK integration: compile and link the example against a pinned ESP-IDF release.
- Hardware: boot and repeated reset, straps and clock, MDIO, 100/full link,
  reconnect/recovery, diagnostics and a separate 10 Mbit/s configuration.

Use datasheet behavior as the test oracle. Keep the evidence for each layer
separate; register mocks do not prove a linked firmware image or physical link.

## Initialization and PCS reset (prompt 15)

`operations.cpp` executes six registered suites: initialization, admission,
timing, failures, lifecycle and ownership. Its independent controller model
checks literal NONE/PCS traces (20/32 frames), all 52 successful-trace frame
failure positions, every release phase, retained strap words and malformed
release reports. It checks exact frame counts, initialized preservation on a
proved effect-free failure, cancellation at each poll/effect boundary, separate
restoration tokens/results, stale tokens and bus-silent cleanup/teardown.

Timing cases include strict OP-6 equality at 100 Hz, re-admission at the actual
pre-callback sample, post-return wait anchors, uint64 overflow, regression latch,
raw expiry, frozen-clock stalls, zero/short slices and unsplit MMD. Contention
covers every batch frame and two PHY addresses. The six suites execute 14,338
active checks; their negative probe demonstrates Release checks remain enabled.

```sh
cmake -S . -B build/init15-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/init15-release
ctest --test-dir build/init15-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/init15-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/init15-consumer
ctest --test-dir build/init15-consumer --output-on-failure --no-tests=error
python tools/check_doxygen.py
```

The stage15 checkpoint registered 40 tests; stage16 extends this to 47,
including seven intentional failure probes. [Initialize.h](../examples/portable/Initialize.h) is linked and executed
by the tests-OFF consumer for NONE/PCS and separate failed restoration/abandon.
Doxygen requires the public operation/evidence pages and rejects undocumented
negative fixtures. These results prove portable software behavior; physical
bus/reset timing, SDK link and package export remain separate evidence layers.

## HARD/RESTART and external reset (prompt 16)

`reset.cpp` adds registered traces, floor, failures, timing, external and ownership
suites, plus its Release failure probe. Current total is 47 CTests. The independent
PHY model checks all 64 HARD/RESTART successful-trace frame failure positions,
all 24 batch-release positions, malformed semantic receipts, raw sentinel outputs,
actual attempt/completion counts and no retries. Reset, restore and readback are
separate observations. A successful callback with a missing command receipt
cannot erase possible mutation or the shared-bus floor.

The universal PHYRCR precheck updates earlier operation traces to NONE20 and
PCS32 frames; HARD/RESTART use32 each. Existing operation coverage remains active.
Reset tests cover 1000us polling with a 2000us floor, strict100Hz OP-6 arithmetic,
clock regression, unrepresentable floor, late accepted command, sticky/overlapping
commands, zero budgets, every cancellation boundary, mode/strap changes, stale
results, two PHYs and full transaction exclusion. External reset/POR notifications
have zero callbacks. Host models do not resolve the physical hard-reset strap gate.

```sh
cmake -S . -B build/reset16-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/reset16-release
ctest --test-dir build/reset16-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/reset16-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/reset16-consumer
ctest --test-dir build/reset16-consumer --output-on-failure --no-tests=error
```

The linked owner consumer executes all four kinds and carries SharedMdioQuiet
outside driver lifetime. It holds other PHYs through failed/cancelled HARD,
take/abandon/end and exact floor expiry. SDK link, exported package and Q5 physical
qualification remain NOT RUN; no fixture action or remote CI was invoked.

## Negotiated configuration and Clause 28 exchange

`negotiation.cpp` adds registered Release suites for all60 technology/pause
vectors, admission, replay, faults, ownership, pages, page faults/grammar,
completion, every-page-frame/cancel boundaries and timing. Independent literals
exercise selector replacement, RF prohibition, reserved preservation, no extra
VOD, every reset replay, first-error retention, partial read-clear evidence,
BASE/NEXT distinction, toggle, null continuation, refusal and finite budgets.
No production masks generate expected wire values. Failure probes remain active
in Release. The tests-OFF linked consumer runs both generic and100/full examples.

```sh
cmake -S . -B build/an17-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/an17-release
ctest --test-dir build/an17-release --output-on-failure
cmake -S test/consumer -B build/an17-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/an17-consumer
ctest --test-dir build/an17-consumer --output-on-failure
python tools/check_doxygen.py
```

Host traces do not qualify physical Q10, MDC timing or SDK integration. See the
[stage17 report](../docs/reports/implementation/17-negotiated-link-settings.md).

## Forced and coupled profile verification

`forced.cpp` registers matrix, admission, transitions, failures, replay, ownership
and timing suites plus an active Release failure probe. Stage 18 checkpoint:67 CTests.
Independent literals cover all36 forced and24 negotiated routing combinations,
20/24-frame traces, every-frame failure/cancel boundaries, all reset kinds,
late accepted writes, small indivisible poll slices and two-PHY exclusion.
Observed tuples are decoded from actual readbacks on complete mismatch; partial
failure preserves older tuple/time as stale. The negotiated tests retain all
next-page/pause assertions with the additional coupled controls.

```sh
cmake -S . -B build/forced18-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/forced18-release
ctest --test-dir build/forced18-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/forced18-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/forced18-consumer
ctest --test-dir build/forced18-consumer --output-on-failure --no-tests=error
```

The consumer executes [Forced.h](../examples/portable/Forced.h) against the real
core. Host success does not qualify hardware partner compatibility or Q11.

## Independent prompts 13-18 audit

Current total: 68 CTests. `forced_verification_outcomes` interrupts an already
mismatched verification pass with transport error, cancellation, clock fault,
deadline, invalidation or frame exhaustion, both in CONFIGURE and initialization
replay. The original mismatch survives separately from the terminal status.
Existing access suites now reject missing whole/suffix receipts while preserving
valid prefix samples and possible history loss; sound zero-frame denials remain
effect-free. Negotiation tests cover NONE reset flags, message5/6 grammar,
16-page limits, inverted initial toggle, and completed loads before late/release
failures. The [audit report](../docs/reports/implementation/13-18-independent-audit.md)
records source review and final checks.

The Release command above also runs in `build/audit13-18-release`. For Windows
UBSan trap coverage, configure a separate directory with:

```sh
cmake -S . -B build/audit13-18-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON "-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/audit13-18-ubsan
ctest --test-dir build/audit13-18-ubsan --output-on-failure --no-tests=error
```
