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
borrowed exceptions, fabricated EEE wire offsets and reclassified software scope.
These 77 tests are research-tool evidence only.

`test_operation_timing.py` adds eight executable prompt-8 design probes for
strict aggregate admission, whole MMD frame allowances, post-callback wait
anchors, tick quantization, certain versus uncertain expiry, uint64 overflow,
finite frozen-clock poll allowances and reserved cleanup deadlines. They use
independent synthetic values and integer arithmetic, not a fake PHY or unused
operation engine. The [operation scenarios](../docs/api-contracts.md#operation-acceptance-scenarios-and-examples)
remain execution-ready call/status/effect recipes for the first real consumer.
These research/design tests total 85; the 13 version-tool tests below bring
current Python discovery to 98. No test claims driver execution.

## Portable C++17 foundation

The root `DP8382x` CMake target exports three standalone public headers.
`DP8382X_BUILD_TESTS` defaults to OFF. With tests ON, four CTest suites exercise
Status helpers, valid/invalid link settings, register-mask operations and passive
transport values. Three extra translation units compile each public header alone.
The tests use explicit checks that stay active in Release builds.

```sh
cmake -S . -B build/host -DDP8382X_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
cmake -S test/consumer -B build/consumer
cmake --build build/consumer
ctest --test-dir build/consumer --output-on-failure
python tools/check_doxygen.py
```

The separate consumer configures the root library with tests OFF and links only
portable values. The Doxygen command requires installed Doxygen; it generates
to a temporary directory, checks documented pages/members/links and verifies
that missing public comments and a bad `@param` fail. Missing Doxygen means the
documentation check is NOT RUN/BLOCKED, never PASS. These host results do not
execute a device, SDK or physical PHY.

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

No driver, firmware build, CI workflow or physical hardware test exists at this stage.
The [80-prompt promptset](../docs/prompts/README.md) now defines those future
deliverables. Its [verification contract](../docs/prompts/VERIFICATION.md)
separates host, portable consumer, SDK link, package, remote CI and physical
evidence. See [planning validation](../docs/reports/promptset-validation.md) for
the checks actually performed on this documentation-only change.

## Planned implementation evidence

- Host tests: exact register transactions, identity/variant checks, advertisement
  masks, error propagation, bounded reset completion and partial-write failures.
- SDK integration: compile and link the example against a pinned ESP-IDF release.
- Hardware: boot and repeated reset, straps and clock, MDIO, 100/full link,
  reconnect/recovery, diagnostics and a separate 10 Mbit/s configuration.

Use datasheet behavior as the test oracle. Keep the evidence for each layer
separate; register mocks do not prove a linked firmware image or physical link.
