# Prompt 56 · DP83826E software qualification

## Phase (a) · Declared part and shared identity

Prompt 56 is **IN PROGRESS**. On 2026-11-23 the wrapper verified the prompt 55
completion at `f752c373b0ad3616c2a5e97fded47957bee151c4`. The worktree and
index were clean on `main`, 0 ahead and 0 behind `origin/main`. This slice
establishes the evidence for selecting a declared E and the identity tests it
needs. Public selection still admits only I.

### Shared silicon record

The original I and E share the register map and procedures in SNLS647I
(August 2025, section 8). The ordering-grade differences are operating
temperature and leakage limits keyed to ambient temperature. Pages 1 and 10
specify −40 to 85 °C for I and −40 to 105 °C for E. Page 14 gives the input
and tri-state leakage limits for those ranges. A review of the part-qualified
text found no I/E selection in the programming procedures. Both declarations
can use the original DP83826 map and common procedures. Reopen this decision
if TI identifies a suffix-specific programming condition.
[Archived SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf) ·
[Family comparison](../../reference/library-family.md#identity-and-a-generation-comparison)

| Source in SNLS647I | Consequence for a declared E |
| --- | --- |
| Tables 8-21 and 8-22, pp. 56–57 | Read Clause 22 registers 2 then 3. Accept `2000 A111` as BASIC and `2000 A131` as ENHANCED, both revision 1. Preserve the supplied suffix. Neither temperature grade nor selected board interface is observable from this pair. |
| Section 8.3.13 and p. 14 note (2) | The 10BASE-Te procedure requires `0x030E = 0x4A40`. Check the existing initialization and restoration order on E. |
| Table 8-23, p. 57 | The application must keep local ANAR bit 13 (`0x2000`) clear. Received partner/status indications remain observable. |

The archived PDF SHA-256 is
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
The pinned TI RTOS implementation checks OUI `0x080028`, model `0x11` or
`0x13`, and revision 1. The Linux constants name C (`0x2000A130`) and NC
(`0x2000A110`), with model values corresponding to ENHANCED and BASIC.
The declaration supplies the ordering grade in both cases. Revision 4 and
Strap11 EMC guidance belong to A-generation. The lead reopened the two pinned
source files to check the comparison.
[TI RTOS dp83826.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) ·
[TI Linux dp83822.c](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c#L22)

### Bind and identity acceptance

- Preserve the numeric values in `include/DP8382x/Config.h`. In
  `src/DP8382x.cpp`, replace the I-only revision list and fixed capability
  return with the bound profile. Resolve the candidate before callbacks and
  install it only after every bind guard succeeds. A rejected rebind must
  preserve the previous binding and pending result.
- Use [test/identity.cpp](../../../test/identity.cpp) for both declarations.
  Keep independent literal words for both modes, wrong OUI/model, revision
  0/4, zero/ones and contradictory required mode. Check the retained
  declaration for identical words on two differently bound instances. Check
  bounded identity-only reads on mismatch, ordinary output preservation on
  failure, and zero callbacks for invalid input and cached capabilities.
- The pure forced-link validator currently requires robust mode. The shared
  I/E preflight will own that requirement. Before applying the prompt's
  `(flags & 30U) == 0U` test expression, reconcile the encoding in
  [test/foundation.cpp](../../../test/foundation.cpp). The current forced
  vector uses bits 1, 2 and 3 for advertisement and bits 4 and 5 for the
  matched mirror controls. Decimal 30 also masks bit 4. Preserve the legal
  paired-mirror case and exercise robust both ways in the pure test.

### Sibling conventions checked at entry

Two read-only agents used the session model for independent source work, one
on the original TI document and one on live sibling conventions. Both siblings
were clean on `main` tracking `origin/main`. Their root AGENTS and CI workflows
were read. DP8382x has no tracked workflow at this checkpoint.

| Read-only sibling and revision | Source paths and decision |
| --- | --- |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h`, `src/TMP1x2Configuration.cpp`, `test/variants/test_variants.cpp`, `examples/common/Tmp1x2CliOutput.cpp`, `tools/hil_tmp1x2_runner.py`, `CMakeLists.txt`. Preserve enum values, validate before I/O and test literal capabilities. The CLI separates the selected model from board pin availability. |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h`, `src/MB85RC.cpp`, `test/test_basic.cpp`, `examples/01_basic_bringup_cli/main.cpp`, `tools/hil_runner.py`, `platformio.ini`. Adapt the single variant table and mismatch/cache tests. MB85RC selects by product ID. The PHY needs a declared suffix plus observed model and revision. |

### Prerequisites and stopping point

The [primary closure report](36-primary-feature-closure.md) returns deep-power
exit to prompt 28, pin-field interpretation to 32/33, and electrical controls
to 35. Those shared software gaps also limit complete E support. The
[ownership](53-regression-transaction-ownership.md) and
[lifecycle](54-regression-lifecycle-deadlines.md) suites and the
[primary matrix](55-regression-coverage-portability.md) supply regressions for
the implemented subset. The primary ledger assigns zero procedures to prompt
56. E physical qualification needs its own identified fixture.

The first slice covered identity and bind. The implementation brief below
completes the remaining core procedure, SDK, CLI and fixture assessment.

**Checks for this slice.** The existing Windows GNU 15.1.0 Release build passed
all ten selected DP83826I foundation and identity tests. Prompt 55's completion
commit changed only documentation since that build. The command below exercises
the baseline. E execution, portable/package consumers, SDK image link, remote CI
and physical qualification were **NOT RUN** in this slice. The ledger check
passed for 97 registers, 52 procedures and 35 features.

```powershell
ctest --test-dir build/p53-baseline -R '^(identity_(decode|policy|failures|lifecycle|ownership|scan)|foundation_(status|link|masks|passive))$' --output-on-failure --no-tests=error
python tools/verify_ledger.py
```

## Phase (a) implementation brief

The wrapper verified the first assessment at
`7c4c8f9124e7b8a3c9c81c582d2f4a782b21e936`. The index and worktree were
clean on `main`, 0 ahead and 0 behind `origin/main`. The lead rechecked the
two sibling revisions above. Two read-only agents used the session's
high-reasoning model to trace register effects and fixture identity across
callers. The lead checked their findings against the source and registered
tests. No ledger procedure has implementation owner 56. The existing I/E
source record resolves G8-1. The design below implements G8-2.
[Audit dispositions](../promptset-audit-dispositions.md)

### Core selection and procedure ownership

- The lead owns `include/DP8382x/{Config,DP8382x}.h`, one private
  `src/PartTable.h`, core integration and test registration. Keep the
  specified `PartProfile` fields and four `RegisterMap` values. Resolve the
  immutable profile by declaration at bind. Clear the instance pointer on
  end. A failed bind preserves the prior profile, identity and pending work.
  Append E to the public enum. Use one original-map definition for I and E.
  Replace the cast-2 invalid case in `test/identity.cpp` and the single-value
  exhaustive predicate in `test/foundation.cpp`.

  | Map | Model(s) | Revision mask | Interfaces | Max strapped address | Operating mode | Forced needs robust |
  | --- | --- | --- | --- | ---: | --- | --- |
  | DP83826 | 11h, 13h | 0002h | MII, RMII | 7 | yes | yes |
  | DP83826A | 11h, 13h | 0010h | MII, RMII | 7 | yes | yes |
  | DP83825 | 14h | 0001h | RMII | 3 | no | no |
  | DP83822 | 24h | 0001h | MII, RMII, RGMII | 31 | no | no |

  Only the original I/E declarations can resolve a bindable profile in 56.
  Keep future map data private and reject invalid casts, including casts
  adjacent to E. Model membership and revision membership have distinct
  rejection details. Decode BASIC/ENHANCED only on a map with operating modes.
  Reject a specific required mode for a modeless profile before callbacks.
  Derive cached capability fields from reviewed profile facts. The private
  feature mask may represent only capabilities backed by an implemented route.
  Unknown or stale identity returns invalid capabilities. Board pin availability
  and application grants have separate checks.
- In `src/DP8382x.cpp`, pass the map through `registerPolicy`, readable and
  writable masks, direct/indirect access, and the separate EEE DEVAD/offset
  lookup. Preserve unbound and invalid-argument results without dereferencing
  an absent profile. Keep selector, read-clear, recipe-only and write-preserve
  rules, alias serialization and the existing collector. Maps awaiting
  qualification must fail closed. Future A masks need their own evidence even
  where addresses agree. Retain the 97-row inventory check in
  `test/test_access_policy.py` alongside independent wire tests.
- In `Config.h`, keep enum, mask, paired mirror and extended-duplex checks
  pure. Move the forced/robust condition into one part-aware preflight.
  In `DP8382xConfiguration.cpp::startConfigure`, reject before token
  assignment, desired-state mutation, clock or MDIO. Use the same rule for
  declared-part CLI validation and native admission. Check retained desired
  settings when used by verify, EEE and diagnostics. Check temporary forced
  profiles in loopback and electrical compliance and the subsequent replay.
  Preserve the native extended-half restriction. Make the I-specific
  interface message in `DP8382xInterface.cpp` reflect actual admission.
- The forced foundation loop can retain all 128 flag combinations. Use
  `0x10` for a mirror mismatch and `0x20` for the pair bit. Set the polarity
  bit to pair XOR mismatch. Bits 1–3 still encode advertisement, bit 0
  robust and bit 6 extended duplex. Now `(flags & 30U) == 0U` excludes
  exactly forced advertisement or mismatched mirror. Check both paired
  mirror values for each robust value and all four explicit mirror tuples.
  Both robust values pass otherwise legal pure validation. Bound I and E
  reject forced robust-off locally. AN permits robust-off.
- Keep the VOD decision in the existing initialize/replay state machine in
  `DP8382x.cpp`. Only the original and A maps require its write/readback.
  Trace entry from NONE/PCS/HARD/RESTART, `_setInitStep`, power preparation,
  EEE recipe return and verify-only entry. Retain reset silence, the analog
  owner's later override, and the common completion path. Verification reads
  the effective owned value. Ordinary link configuration has no VOD write.

### Acceptance on the two declarations

Run the same test bodies with actual I and E bindings. Record registrations
and assert the returned declaration. Shared source justifies procedure reuse.
Prompt 55's matrix records which procedures have executed on I.

| Route | Required independent observations |
| --- | --- |
| Identity and bind | Run the bind/identity cases above with literal IDs, invalid enums, mode and revision mismatches, partial second read, release error and exhausted budget. On an initial identity mismatch, initialize stops after one batch reading 2 then 3. A rejected probe blocks dependent writes. Rebind, invalidate, end and two simultaneous instances must preserve their documented snapshot and token semantics. |
| Reset, VOD and analog | Use `operation_initialization`, `reset_traces`, `reset_failures` and `analog_replay`. SNLS647I §8.5.1.63 gives VOD_CFG3 reset `0x8400`. Check DEVAD 31, offset `0x030E`, mandatory write `0x4A40` then a fresh addressed read with mask `0xFFC0`. Check ordering before link replay and before a separately owned analog override. Retain actual post-reset ID/strap words for each simulated outcome. |
| Advertisement and forced routing | Extend `negotiation_vectors`, `negotiation_pages`, `forced_admission` and `forced_replay`. Seed ANAR with `0xF21F`. Both advertisement application and next-page clearing must force bit 13 to zero. Check the selector, owned fields and write/restart order, preserve partner remote-fault evidence, and assert zero ANAR writes on forced paths. |
| Failure and cleanup | Inject partial reads, target-write effects before terminal error, failed release, late completion and cancellation at the changed boundaries. Check ordinary output and retained partial evidence separately. Run `verify_interrupted_replay` and `verify_barrier` on a declared E. Check stale/trust flags, retained primary status, exactly one result delivery and fresh-budget restoration when cleanup is owed. Repeated cached queries leave clock and bus counters unchanged. |

### Adapter, examples and fixtures

- `src/esp_idf/Phy.cpp::create` already forwards `config.core` to bind.
  Preserve that factory and its output on failure. Rejected configuration
  must preserve the token, native phase and SDK reconciliation state.
  Exercise both suffixes in the pinned-driver `native_factory`,
  `native_configuration_admission`, `native_forced_lifecycle` and failure
  cases. The fitted P4 applications keep their I declaration. Source
  each application's core configuration and printed board identity from the
  same board setting in `examples/esp_idf/{basic,owner}/main/main.cpp`.
- In `examples/common/Dp8382xCli.cpp`, update `Processor::identity`,
  `profileHelp` and `route`. Session, version and local board must print the
  declaration. Default discovery reads addresses 0–7 for I/E, including
  before a probe. An unknown declaration requires an explicit range.
  Explicit 0–31 discovery preserves the binding. Keep one grammar.
  `configuration validate` can check the declared profile without an owner
  call. Fresh I/E capabilities report link mask 15 and interface mask 3.
  Test `cli_processor_{grammar,cached,routes}`, `native_cli_discovery`,
  `native_cli_feature_admission` and compiled transcripts. Preserve stopped
  maintenance, feature grants and first-board INT restrictions.
- Give the compiled fixture's `Options` and `Fixture::setup` in
  `test/cli/dp8382x_cli_fixture.cpp` an explicit simulated suffix. Use the
  same value at bind and in the CLI session record. Its simulated ID words
  still depend on the selected mode. Keep the runner's expected suffix
  independently checkable against actual producer output. Update
  `tools/dp8382x_hil_{protocol,runner,campaign}.py` for exact expected and
  observed I/E declarations. Exercise E BASIC and ENHANCED, a deliberately
  wrong suffix with matching words, saved I replay, E artifact replay and
  a changed declaration in the transcript/manifest. Use
  `cli_hil_{runner,campaign,features}` and the Python protocol/artifact
  tests. Retain missing fixture/grant evidence in the verdict.

### Implementation handoff and evidence

The lead owns public declarations, the part table, maintained API/architecture
guides, registration, progress and integration. After the core contract is
set, disjoint work can cover native/CLI/runner integration and portable test
vectors. Update public Doxygen, README, CHANGELOG, the identity and variants
tables, forced/initialization guidance, CLI, native and fixture guides.
Link the shared I/E source record from the feature matrix and record E's own
executed tests and physical status. Keep the primary ledger and its open
procedure gates traceable. Complete E software qualification still requires
the source evidence identified by owners 28, 32/33 and 35 above.

For phase (b), build the Release host with pinned native tests. Run the new
I/E registrations, affected host/CLI/Python suites, tests-OFF portable
consumer, Doxygen, ledger, references, version and whitespace checks. Link
an E-declared adapter fixture and both fitted-I examples with ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`. Use the prompt 53 three-project
loop with build names `56l`, `56b` and `56o` under
`C:/esp/build/dp8382x-loop`. Each invocation is
`scripts/idf.cmd -C <project> -B <dir> -D IDF_TARGET=esp32p4 -D SDKCONFIG=<dir>/sdkconfig build`.
Record ELF/bin and firmware Git/tree identity. Keep board execution separately
gated. [Build commands](53-regression-transaction-ownership.md#phase-b-implementation)

**Phase (a) finished.** On Windows x86-64 with GNU 15.1.0, the existing
Release/Ninja build with pinned SDK tests enabled passed **84/84** baseline
registrations in 1.31 s. These exercised existing I paths. The reference
check passed for 29 payloads, 27 extracts and 2,286 links. Ledger structure
passed for 97 registers, 52 procedures and 35 features. Whitespace passed.
The phase (a)
portable consumer, exported package, SDK image links, remote CI and physical
evidence are **NOT RUN**. The next stage is prompt 56 phase (b).

```powershell
ctest --test-dir build/p53-baseline -R '^(foundation_|identity_|transport_|mmd_|access_|operation_|reset_|negotiation_|forced_|verify_|native_(factory|configuration_admission|cli_discovery)$|cli_processor_(cached|routes)$|example_board_config$|cli_fixture_transcript$)' --output-on-failure --no-tests=error
```

## Phase (b) implementation

The wrapper verified clean `main` at
`5f57254cc802bc3372fdbd344df2e36425081238`, equal to `origin/main`.
The lead owned the part table, public API, core and native admission, test
registration and integration. Two implementation agents used the current
high-reasoning model for the two independent cross-layer tasks. One owned
CLI, examples and fixture evidence, the other owned the portable and pinned
native test vectors. Both finished. No ledger procedure has owner 56.

The lead reopened the TMP1x2 and MB85RC paths listed in the phase (a) sibling
audit, including CLI, runner, tests, registration and agent rules. Both
checkouts were clean at the same revisions. The implementation adopts local
model validation and a private part table. A product-ID lookup alone cannot
select a suffix here. The phase (a) SNLS647I record, pinned TI RTOS identity
filter and TI Linux model names still agree on the programming boundary.
[Shared silicon record](#shared-silicon-record)

### Shipped behavior and qualification vectors

| Boundary | Implementation and executing evidence |
| --- | --- |
| Binding, identity and capabilities | `src/PartTable.h` supplies the original map to I and E, with revision mask 0x0002. Each binding installs its profile after validation and busy guards. A failed rebind retains the prior declaration and pending result. A-generation, DP83825 and DP83822 rows have no selectable declarations. The I/E identity suites cover both modes, invalid casts, all model/revision values, partial acquisition, late/release failures and simultaneous instances reading identical words. |
| Register access and initialization | Direct and indirect raw admission use the selected map, including DEVAD-specific EEE reads. Unbound calls retain address and access syntax checks. The 97-register inventory and executing E raw-access suites cover selector, evidence and recipe protections. Only original and A maps select the mandatory VOD step. Reset and analog suites check the original 0x8400 reset value, addressed 0x030E write of 0x4A40 and masked readback before subsequent owned settings. Initialization mismatch tests allow only the two ID reads. |
| Link settings | Pure validation covers enums, active fields, coupled mirrors and extended duplex. The overload with a declaration enforces robust crossover for forced I/E. Core, native and CLI preflight reject invalid intent before clock or bus work. Forced temporary loopback and compliance profiles set robust explicitly. Replay uses retained, admitted persistent intent. The native extended-half restriction remains. The foundation test covers both robust values, both mirror pair states and mismatches. AN tests inject local 0xF21F independently at advertisement and next-page clear and retain the partner's remote-fault evidence. |
| CLI and native applications | Session and board identity report the declaration. Default discovery uses the part's 0..7 strap range before probing. Explicit scanning still spans 0..31. Both fitted apps use the same board declaration for bind and display. In the first integrated run two host-only example console fixtures omitted their declaration. Configuration admission returned INVALID_CONFIG/ENUM_VALUE. The fixtures now use the fitted board constant like their firmware. |
| Runner and artifacts | `--fixture-variant` selects the producer independently of the runner's `--variant` expectation. BASIC and ENHANCED E, a mismatched grade with the same ID, altered transcript/manifest grades and historical I replay all execute. Declared E passed 44 executable feature cases. The 45th case retained the source block on deep-power exit. The HIL tests use simulated endpoints. |
| Ownership and recovery | E reuses the existing transport owner, collector, timeout/cancellation and cleanup barriers. The explicit E registrations exercise failed release, interrupted mutation, cached state, result delivery, separate restoration and SDK reconciliation. No board pin or application grant is inferred from the declared part. |

The current API, architecture, identity, variants, forced settings, initialization,
CLI, fixture, hardware, feature and native guides document the shipped subset.
The layer-specific receipts below apply to the implementation worktree.

### Local implementation receipts

Windows x86-64, GNU 15.1.0, Ninja 1.12.1, CMake 4.0.1 and Python 3.12.10.
The pinned-native configuration verified 45 SDK inputs at ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643` and the basic host application
checked its 37-header manifest. Use the process-local SDK safe directory
during both configuration and build.

```powershell
$env:GIT_CONFIG_COUNT = '1'
$env:GIT_CONFIG_KEY_0 = 'safe.directory'
$env:GIT_CONFIG_VALUE_0 = 'C:/esp/v6.1/esp-idf'
cmake -S . -B build/p53-baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON -DDP8382X_BUILD_PINNED_IDF_TESTS=ON -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p53-baseline --parallel 6
$focus = '^(e_|identity_|foundation_|transport_|access_|mmd_|operation_|reset_|negotiation_|forced_|analog_|verify_|eee_|electrical_|loopback_|native_|pinned_driver_|cli_processor_|example_|owner_|cli_fixture_transcript$)'
ctest --test-dir build/p53-baseline -R $focus --no-tests=error --output-on-failure -j 4
ctest --test-dir build/p53-baseline -E "($focus)|^cli_hil_" --no-tests=error --output-on-failure -j 3
ctest --test-dir build/p53-baseline -R '^cli_hil_(runner|campaign|features)$' --no-tests=error --output-on-failure -j 1
ctest --test-dir build/p53-baseline -R '^cli_hil_(serial|soak|interruptions)$' --no-tests=error --output-on-failure -j 1
cmake -S test/consumer -B build/p56b-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/p56b-consumer --parallel 4
ctest --test-dir build/p56b-consumer --no-tests=error --output-on-failure
```

- **Host.** The final 546-test selection passed in 4.98 s, including all 29
  separately registered E cases. The remaining 144 non-HIL cases passed in
  1.62 s. Runner, campaign and feature CTests passed 3/3 in 47.58 s.
  Serial simulation, soak and interruption replay passed 3/3 in 150.20 s.
  Together these four selections passed **696/696** Release CTests.
  A separate C/C++ UBSan trap Release build under `build/p56-vectors` passed
  119/119 selected registrations. Its agent also ran E access compositions,
  admission, EEE raw/recipe, MMD traces and forced matrix/transitions directly.
  Both sanitizer compiler flags were
  `-fsanitize=undefined -fsanitize-undefined-trap-on-error`.
- **Offline replay.** The documented E 10BASE-Te example in
  `build/p56b-cli/evidence/campaign-e-10base-te.json` passed execution,
  restoration and replay. The simulated producer identified as E, RMII,
  address 1, firmware prefix `abcdef0`. Its transcript contains 36 confirmed
  commands and 14,231 bytes, SHA-256
  `e9e02b94fa977a749d46087fcccd970985b6fdac2399d27a2840686fb2fa3394`.
  The historical I pair in
  `build/p52-release/evidence/phase-b-10base-te.json` also replayed.
- **Portable consumer PASS**, 2/2 with library tests OFF. Exported package
  consumer **NOT RUN**. Prompt 70 owns packaging.
- **SDK link PASS**, three ESP32-P4 ELF/bin pairs. The E-declared adapter
  fixture and fitted-I basic and owner apps all carry the full phase (a)
  base SHA and `dirty` in the generated identity header and built images.
  Use the three-project loop and local clangd, ccache and Git settings
  specified in the phase (a) brief, with build directories 56l, 56b and
  56o and the existing `53cache`. The wrapper builds used only local SDK
  inputs. Each pair below was hashed after the final link.

| Project | ELF bytes and SHA-256 | Firmware bytes and SHA-256 |
| --- | --- | --- |
| 56l adapter link, E | 7,356,428 / `cb60cc786b3a8c90ec7779be32714e6aad3d3ad9ec385a095ff1fb719a1a4598` | 388,288 / `77ad9be114063fb0aa8d0de78b5cd2b38b8a511d4080320175454b1a89591f12` |
| 56b basic, I | 10,026,824 / `9133d098eaff629d74de846710bf54e31848b7795aa8cc26a9b6ca0f273b1dbb` | 684,256 / `abe787896c314b01bf606924e0990bda687043b9afb9a94231c6be73e973c64e` |
| 56o owner, I | 8,720,184 / `3a3791cdf00cd73a3e3ebdbb1558627716db23a9dea7f4d33593d899b5096689` | 562,336 / `25af12ff5c5309629259ef6c1b05cf5f1b9e1bd6600d5acfcc2948b2ed2887db` |

Python discovery passed with 246 tests run and 52 skipped when compiled
fixtures or inventory inputs were absent. Doxygen initially treated the
private profile's forward-declared namespace as public. Its internal
declaration is now marked accordingly. Doxygen passed 294 pages and both
negative fixtures. Ledger validation passed for 97 registers, 52 procedures
and 35 features. References, version and whitespace checks passed.

Remote CI and physical qualification are **NOT RUN**. Prompt 71 owns CI.
The open deep-power, pin and electrical gates in the primary closure report
apply equally to E. Phase (c) reviews this implementation on its wrapper-
verified commit. Prompt 56 remains **IN PROGRESS**.
