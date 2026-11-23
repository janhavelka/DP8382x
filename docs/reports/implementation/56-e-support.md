# Prompt 56 · DP83826E software qualification

**Full E software qualification is BLOCKED on primary feature closure.** The
declared revision-1 E passes the implemented core, adapter, CLI and simulated
fixture paths. Deep-power exit and specific pin and electrical procedures still
need exact-part evidence and implementation for both grades. Physical
qualification is a separate open gate. [Final verification](#phase-d-final-verification)

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

## Phase (c) independent review

The wrapper verified clean `main` at
`d1f7706a15428692c01355c7d1fd932e48ac75e1`, equal to `origin/main`.
Two read-only reviewers worked in parallel on the implementation since
`5f57254`. The silicon reviewer traced the bound part through identity, raw
access, forced operation, VOD, ANAR and recovery. The integration reviewer
traced the native adapter, board declarations, shared CLI, fixture, artifact
replay and registrations. Both used the current high-reasoning model to
check behavior across entry points. The lead integrated the fixes and each
reviewer re-read the final change for their finding.

The source check covered SNLS647I pp. 14, 40, 57, 61 and 85. The phase (a)
sibling paths and revisions were rechecked. TMP1x2 and MB85RC were clean.
The declared grade, shared original map and literal revision-1 identities
agree with the shared silicon record. The review also confirmed mandatory
VOD ordering, local ANAR bit 13 clearing and retention of partner evidence.
The CLI timestamps its output using the application clock. Local profile
validation issues no driver-clock or MDIO callback.
[Source and sibling audit](#shared-silicon-record)

| Finding reproduced on both I and E | Fix and regression |
| --- | --- |
| After applying AN with robust crossover off, the compiled CLI accepted `registers bmcr 0x1000 0`. Readback showed both BMCR AN and CR1 robust clear. SNLS647I table 8-28 requires CR1 bit 5 in forced mode. | The core's existing raw BMCR owner checks the part rule before submitting its two-frame transaction. An explicit AN enable is sufficient. A preserved AN bit or robust setting requires current verified readback. The I/E `native_restart_basic_control` cases check rejection without frames or clock calls, preserved trust, stale readback after cancellation and legal writes. On the corrected CLI, CR1 read zero, the write returned INVALID_CONFIG with no batch, BMCR stayed 0x1000 and configuration stayed VERIFIED. |
| Native `startConfigure` checked the link fields before taking its gate, then checked negotiation options inside the core. With the next semaphore take set to fail, forced mode plus a negotiation wait returned BUSY. Entering the gate can also apply deferred lifecycle outcomes. | The adapter checks the complete settings and options with the shared part-specific validator before gate entry. The I/E `native_configuration_admission` cases assert INVALID_CONFIG/LINK_COMBINATION, an untouched semaphore sentinel, token and frame count, and preserved phase and SDK agreement. |

All four new regression cases failed against the phase (b) implementation
before the fixes. The lead updated the portable and native Doxygen, public
API, raw-access, CLI and SDK guides and CHANGELOG. The README and support
matrix already describe the scope of I/E support.

### Local review receipts

Windows x86-64, GNU 15.1.0, Ninja 1.12.1, CMake 4.0.1 and Python 3.12.10.
The following commands use the phase (b) process-local SDK Git setting.

```powershell
cmake --build build/p53-baseline --parallel 7
ctest --test-dir build/p53-baseline -E '^cli_hil_' --no-tests=error --output-on-failure -j 4
ctest --test-dir build/p53-baseline -R '^cli_hil_(runner|campaign|features)$' --no-tests=error --output-on-failure -j 1
cmake --build build/p56-vectors --parallel 5 --target dp8382x_native_restart_tests dp8382x_native_host_tests
ctest --test-dir build/p56-vectors -R '^(e_)?native_(restart_(admission|basic_control|failures|vectors)|configuration_admission)$' --no-tests=error --output-on-failure
```

- **Host PASS.** All 691 non-HIL Release CTests passed, including all 30 E
  registrations. The three compiled-fixture runner, campaign and feature
  CTests passed. The reviewer independently replayed the saved I and E
  artifacts named in phase (b). Serial simulation, soak and interruption
  CTests last ran in phase (b).
- **UBSan PASS.** All seven selected portable and native registrations passed
  with C/C++ undefined-behavior traps enabled. The first regeneration omitted
  the process-local SDK Git setting and stopped at the ownership check.
  Repeating with the setting from phase (b) passed the pinned source checks.
- **Python and documentation PASS.** Standard unittest discovery ran 246
  tests with 52 skipped. Doxygen checked 294 generated pages and rejected
  both negative fixtures. Reference verification covered 29 vendor artifacts,
  27 extracts and 2,295 local links. The ledger passed for 97 registers,
  52 procedures and 35 features. Generated version and whitespace checks passed.
- **Portable consumer PASS.** The phase (b) tests-OFF consumer configure,
  build and CTest commands passed 2/2 on the review worktree.
  Exported package consumer **NOT RUN**.
- **SDK link PASS.** The phase (b) three-project wrapper commands linked all
  three P4 applications against ESP-IDF v6.1 `fff9895c`. Each identity
  header, ELF and firmware image contained the full phase (b) SHA and dirty
  marker. The table gives the binaries after the final source changes.

| P4 link target | ELF bytes / SHA-256 | Firmware bytes / SHA-256 |
| --- | --- | --- |
| 56l adapter, E | 7,356,924 / `50f3bde9d0694eaa1bf872ae139d5143b685b83a0235b06cab6037ac61c42a23` | 388,288 / `ba84f228234c5b241c933f24c4d564df88b95fdb370f70ec01baadd280eb2123` |
| 56b basic, I | 10,027,320 / `b856b9b475f341da9a463fe3fe60ffb6b410903f9b4a9b8fcb2c7f01bddee323` | 684,448 / `65360da00164a2663bd93de552e512a3143e12d55d891d2321b40dac06ccee9c` |
| 56o owner, I | 8,720,680 / `fbef68902cb9fd718a51762bc436ef7f3da303fb1a4f3f956a5438f3fb0b33db` | 562,528 / `341690c55872d4513ba0d1fdcef05c932c27ac5141615f8361de3348b3e7e0c4` |

Remote CI and physical qualification **NOT RUN**. Prompt 71 owns remote CI.
Deep-power exit, unresolved pin fields and electrical controls retain the
source gates recorded by prompts 28, 32/33 and 35. Prompt 56 remains
**IN PROGRESS** for phase (d).

## Phase (d) final verification

On 2026-11-23 the wrapper verified the review commit
`39713f1f2bdf0b38c2f38d7410c8ea0f3de491ab` equal to `origin/main`.
The entry index and worktree were clean on `main`, 0 ahead and 0 behind.
This is the tested source commit. Two agents used the session model for
independent builds, one for the real SDK links and one for UBSan and the
standalone consumers. The lead ran the Release, offline fixture and
documentation checks and reconciled the evidence against prompt 56.

### What the E tests establish

The 30 explicit E registrations comprise 23 core and seven pinned-native
cases. They cover preservation of the declared grade with identical I/E ID
words, both operating modes at revision 1, bounded reads on mismatch,
callback-free rejection, reset defaults, VOD write/readback order, local
ANAR bit 13 clearing, forced-mode crossover, retained partial effects and
separate restoration. The native tests also cover configuration admission,
operation failures and the shared CLI. The compiled fixture exercises 44
source-admitted E feature cases and retains the blocked deep-power case.
The phase (b) table maps these checks to the implementation and source.
[Qualification vectors](#shipped-behavior-and-qualification-vectors)

The source check confirmed the archived SNLS647I hash and its VOD and
advertisement requirements. The clean TMP1x2 and MB85RC checkouts remain at
the phase (a) revisions. Their variant validation, product-ID dispatch,
CLI, runner and registration conventions still support the recorded choice
to bind the grade from the caller. The README, CHANGELOG, API, variants,
feature and fixture guides describe the implemented paths and their limits.
[Source and sibling audit](#sibling-conventions-checked-at-entry)

### Local checks on the review commit

Windows x86-64, GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1 and Python 3.12.10.
Use the phase (b) process-local SDK Git setting and Release configure command.
The pinned-native verifier checked 45 real SDK inputs and the basic
application verifier checked its 37-header manifest at ESP-IDF v6.1
`fff9895c82d744c7237be8847347bdd1b07c6643`.

```powershell
cmake --build build/p53-baseline --parallel 5
ctest --test-dir build/p53-baseline -E '^cli_hil_' --no-tests=error --output-on-failure -j 4
ctest --test-dir build/p53-baseline -R '^cli_hil_' --no-tests=error --output-on-failure -j 1
ctest --test-dir build/p53-baseline -R '^cli_hil_serial$' --no-tests=error --output-on-failure --repeat until-fail:3 -j 1
```

- **Release host PASS after serial rerun.** All 691 non-HIL registrations
  passed in 4.48 s. Five of six offline HIL registrations passed on the
  first attempt, including the feature, soak and interruption campaigns.
  The injected serial test exited after 15 test reports in 8.39 s with no
  traceback. Its isolated reruns passed three times in 24.12 s with the
  same binaries and deadlines. The cause of the first exit is unconfirmed.
  Across these runs all 697 registered tests passed.
- **UBSan PASS.** All 691 non-HIL registrations passed, including all 30 E
  registrations and the native I/E preflight and raw BMCR review fixes.
  Use the same Release configure command with `-B build/p56-vectors` and
  set both `CMAKE_C_FLAGS` and `CMAKE_CXX_FLAGS` to
  `-fsanitize=undefined -fsanitize-undefined-trap-on-error`. Build with
  `--parallel 5` and run the non-HIL CTest command above in that directory.
- **Portable consumer PASS.** A fresh tests-OFF configure of
  `test/consumer` in `build/p56d-consumer` with Ninja and Release, build
  with `--parallel 3` and CTest passed both registered consumers.
  Exported package consumer **NOT RUN**. Prompt 70 owns that check.
- **Python and documentation PASS.** Standard unittest discovery ran 246
  tests, 194 passed and 52 skipped. The compiled-fixture cases also run
  through CTest. `python tools/check_doxygen.py` verified 294 HTML pages
  and rejected both negative fixtures. `python tools/verify_references.py`
  validated 29 payloads, 27 extracts and 2,296 local links.
  `python tools/verify_ledger.py` checked 97 registers, 52 procedures and
  35 features. `python scripts/generate_version.py check` and
  `git diff --check` passed.
- **Offline replay PASS.** The runner guide's 10BASE-Te campaign executed
  and restored on the compiled ENHANCED E fixture at RMII address 1.
  Both the expected and producer grade were DP83826E and the simulated
  firmware prefix was `39713f1`. The new
  `build/p56d-evidence/campaign-e-10base-te.json` pair passed
  `verify-artifacts`. Its transcript was 14,231 bytes, SHA-256
  `f75a3088afc81b82f64bce88aec56d40159c88b33c809cd052995f09786a429d`.
  The saved I pair from phase (b)'s replay command also passed.
- **SDK link PASS.** The phase (b) three-project `scripts/idf.cmd` commands
  linked the E-declared adapter fixture and both fitted-I applications at
  the pinned v6.1 commit above. The identity header, ELF and binary for
  each ESP32-P4 target contained the full clean review SHA.

| P4 target | ELF bytes / SHA-256 | Firmware bytes / SHA-256 |
| --- | --- | --- |
| 56l adapter, E | 7,356,924 / `40ebe8e4cfda5c63d485b074661f5985ba0f346b1c8f7c0a799936f5174f3def` | 388,288 / `3b01f6ee635ecc7da6872403ba9fc5bb409c8e45fef06b79a2677561518d5cad` |
| 56b basic, I | 10,027,320 / `abda5cc1272374a2121374c665dbc4d7f3c37b7bdc17e3cf9b5a4759e43a0124` | 684,448 / `e138ed9dcbc9d2aef976ca4ec9c516d34dd019fda28b5fbebf843e920b9e167a` |
| 56o owner, I | 8,720,680 / `54b6e366a7898781cd20ebca912c234f96faebf6d413d55a0f5079667b2cf628` | 562,528 / `3ca4b772180bb7ba255263cf59e25f647346724e7b8033afab671fcb56f387c7` |

### Remaining qualification gates

Prompt 36 records primary feature closure as **BLOCKED**. I and E use the
same procedures, so full E software qualification inherits the following
dependencies. Each owner needs authoritative clarification, implementation
of the admitted sequence, and failure and restoration tests. Re-run the I/E
cases on those changes before marking prompt 56 complete.

| Owner | Evidence needed to close the E gap |
| --- | --- |
| 28, deep-power exit | Safe composition and preservation at 0x0428, exit order and when SMI and clocks become usable again. |
| 32/33, pin and clock controls | Correct per-pin and per-mode selector encodings and activation conditions for the gated routes. |
| 35, electrical controls | Resolution of IOCTRL examples, legal receiver-gain encoding, and 100 Mbit/s test access, composition and stop at 0x0428. |

The [primary closure report](36-primary-feature-closure.md#required-remaining-acceptance-and-downstream-use)
tracks the owning procedures. Physical E testing is **NOT RUN** and needs an
authorized, identified fixture. Remote CI is **NOT RUN** and belongs to
prompt 71. Prompt 57 can use the verified shared identity and profile as
inputs to its independent source research.
