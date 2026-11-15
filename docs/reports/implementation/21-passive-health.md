# Prompt 21 - passive driver health and exact accounting

Date: 2026-11-15. Execution branch: `main`. Entry checkpoint:
`0a2d2c45f5ee43b5458949acaf7f21ebb99bb67c`, clean, fetched and equal to upstream.
The final commit and verified synchronization are recorded in the handoff,
without a self-referencing commit loop.

## Scope, rules and prerequisites

Read root/parent AGENTS, COMMON, prompt 21, CONVENTIONS, ENGINEERING, EVIDENCE,
FULL_COVERAGE, SOURCE_MAP, VERIFICATION, API contracts and OP-16, current audit
G11-2/G11-7 dispositions, README, architecture, variants and the reference index.
Prerequisites exist: terminal transport accounting (11), initialization and
retained restoration (15), single collector (13/20) and live link batches (19).
No ledger procedure has implementationStage 21: health is software policy,
not a new silicon control or a second event/counter collector.

The lead owns the public header, existing driver/link integration, private
arithmetic helper, examples, CMake registration, shared guides, progress and Git.
Names remain DriverState/HealthSnapshot, driverState/healthSnapshot/resetStatistics.
One application owner serializes every instance access; one controller owner
serializes every complete batch. Health adds no callbacks, clocks, allocation,
scheduler, retry, recovery manager or admission policy. Future SDK/CLI paths
remain with their owners. No remote CI exists yet; prompt 71 owns first triggers
and exact-commit remote verification.

## Implementation and review decisions

Health is accumulated at the existing _execute callback boundary after report
normalization, before local clock/deadline overlays. Each eligible callback is
one transaction, not one frame or one public operation. Traffic classification
is private and explicit, independent of operation-slot admission: current
initialization/configuration and live link are managed; raw/probe/collector and
low-level mixed-register maintenance are transport-only. Future temporary
sessions can use operation admission without inheriting managed classification.

A private helper performs saturating uint32 accumulation and timestamp/error
retention; the driver derives state from current initialization, streak and the
existing offlineThreshold. It is not a generic health manager. OFFLINE never
blocks a legal explicit attempt. resetStatistics clears only cumulative health
totals. Successful replacement binding/end resets local health; rejected
lifecycle calls retain it. Invalidation preserves history and changes projection.
RESTART/NONE follow existing OP-13 initialization rules; PCS/HARD effects withdraw
initialization. Managed restoration uses a separate token/result/budget and can
clear the streak without erasing the original error or failed operation result.

Current transport permits OPERATION_TIMEOUT after an attempted prefix. The
frozen predicate already counted this as failure but did not name the health
cause when no MDIO/release error exists. The contract now explicitly normalizes
that incomplete managed outcome to MDIO_ERROR with original detail while keeping
the public timeout/raw receipt. This is not proof of a physical wire defect.
Clean late or clock-fault transfers remain health successes; malformed reports
and release/controller faults count failures under primary-error precedence.
Post-callback raw timestamps allow valid zero and retain clockLagUs uncertainty.
New invalid time replaces that outcome's timestamp with zero/false; the opposite
historical timestamp is retained, but a broken clock timeline forbids age use.

Three inherited capable agents were used for disjoint peer/source audit,
contract/normalization review and test implementation. The exact source and
normalization work needed careful reasoning, but introduced no unresolved new
silicon rule requiring a different model. Reviewers were read-only; the test
agent owned only test/health.cpp and used build/health21-tests. The lead reviewed
all contributions and retained one explicit callback path and derived state.
Independent review requested separating traffic from operation admission and
clarifying reset projection; both recommendations were incorporated.

## Fresh sibling and manufacturer evidence

All four inspected sibling worktrees were clean. Siblings, FieldCore, SDK and
vendor payloads remained read-only.

| Repository / revision | Actual paths and decisions |
| --- | --- |
| TMP1x2 afd50889312f6e218a6a34eca687e3e8547d8b40 | include/TMP1x2/TMP1x2.h HealthSnapshot; src/TMP1x2.cpp track, healthSnapshot, read/write; test/test_core.cpp health; test/helpers/test_helpers.cpp healthAndInvalidation; CMake core/helpers/jobs/cli registration. Adopt passive projection, raw neutrality, retained error and totals-only reset. Adapt uint8/milliseconds to frozen uint32/microseconds and one MDIO batch. |
| PCA9555 8b461b2afe9910b5b15fe32cc049d52e3b498970 | include/PCA9555/PCA9555.h; src/PCA9555.cpp _updateHealth/_i2cWriteCleanupTracked; test/test_basic.cpp test_input_read_failures_survive_successful_cleanup_in_health_and_validity and registration; platformio.ini native Unity. Preserve primary failure despite successful cleanup, but DP separately admitted managed restoration follows normal accounting. Reject three-state/bind-implies-READY policy. |
| INA228 4de84f2564eb6382835ca9a72d8688377657dcb4 | include/INA228/Config.h HealthPolicy; src/INA228.cpp _updateHealth/_ensureNormalI2cAllowed; test/test_basic.cpp test_latched_offline_policy_remains_explicit_legacy_opt_in and test_passive_health_never_suppresses_owner_requested_transport, registrations. Its PASSIVE remains DEGRADED and LATCH_OFFLINE blocks traffic: neither is DP's exact four-state passive policy. Reject initial-bringup suppression and admission latch. |
| 24Cxx 1bd9f969d1df55ca5819b0d290ff331869e7d5bc | include/EEPROM24Cxx/EEPROM24Cxx.h; src/EEPROM24Cxx.cpp track and ACK-poll branch; test/test_core/test_core.cpp healthAndReadFailures/timingAndAckPolling and CMake core registration. Adopt passive state/effect separation; reject expected EEPROM address-NACK/tWR exceptions, which have no MDIO analogue. |

Read sibling AGENTS and actual cached CLI/runner behavior: TMP
examples/common/Tmp1x2CliOutput.cpp, test/cli/test_cli.cpp and
tools/hil_tmp1x2_runner.py; PCA examples/common/HealthDiag.h and
tools/run_i2c_hil.py; INA examples/01_basic_bringup_cli/main.cpp,
examples/esp_idf/basic/main/main.cpp, tools/run_i2c_hil.py and
tools/test_run_i2c_hil_parser.py; 24Cxx examples/common/Eeprom24CxxCli.cpp,
tools/hil_runner.py and tools/test_hil_runner.py. Adopt cached rendering and
retained failures; reject timestamp-zero-as-never and exact deltas from
saturated counts. No inherited I2C frame/ACK semantics or foreign runner grammar.

Current TI ethernet-rtos-drivers main remains
`94bf4343004732983b0b8e1eec123d88ff70453b`, rechecked through the GitHub connector:
[generic_phy.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c)
lines 205-300, blob 39414bd3b1004cdf6075bf0f4be30180babdc84d;
[dp83826.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c)
lines 290-316/395-450, blob e0b64710350d764fe16ffd089fbdc5246e600039.
Both files retain BSD-3-Clause notices; no code copied. The generic MMD helper
checks all four phases; the chip RMW's ignored write errors remain rejected.

The current [official TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
is still SNLS647I August 2025, 3,366,322 bytes, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
byte-identical to the archive. Original PDF text on printed pages 36 and 63-65
was reopened: 8.3.11.4/.5 and tables 8-31/32 supply the literal four-frame MMD
sequence; PHYSTS table 8-34 bit 0 clear is link-down, not MDIO failure.
Neither source defines our health threshold, retry timer or automatic recovery.
No register fact, reserved mask or silicon capability changed.

## Documentation and acceptance

[Health guide](../../health.md) provides callback and lifecycle tables, time
validity and application mapping. Public Doxygen, API contracts, architecture,
README/CHANGELOG, direct/link/event/initialization/variant guides and feature
coverage now distinguish health from transport totals, chip counters, link and
configuration trust. Ledger procedure counts/owners are unchanged; the counter
feature note identifies prompt 21 as implemented software policy. Version remains
0.1.0; no package or supported-part change is warranted. No SDK/tool runner exists
in this stage; the current Doxygen checker now requires HealthSnapshot.

Health.h compiles/executes in the tests-OFF portable consumer and renders cached
health alongside a valid cable-down link without bus activity. test/health.cpp
uses independent literal frame counts and failures, not production constants as
its oracle. Private arithmetic boundary tests exercise UINT32_MAX saturation
without a public test hook. The new suites retain active CHECKs under NDEBUG.

## Verification

Environment: Windows PowerShell, GNU C++ 15.1.0/WinLibs, C++17 Release with
-Wall/-Wextra/-Wpedantic/-Werror/-Wconversion/-Wsign-conversion and active
CHECK assertions under NDEBUG. Independent final review found no blocking
requirement gap and independently passed the full 97-test suite in
build/health21-review. The test agent's 11 focused suites passed 2,985 CHECKs
in build/health21-tests; its assertion-failure probe also behaved as required.

| Command/layer | Result |
| --- | --- |
| cmake -S . -B build/health21 -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON; build; ctest --test-dir build/health21 --output-on-failure --no-tests=error | PASS 97/97 including 12 intentional failure probes and standalone public-header compilation. |
| cmake -S test/consumer -B build/health21-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release; build; CTest | PASS 1/1, tests OFF, real portable core and cached-health example. |
| python -m unittest discover -s test -p "test_*.py" -q | PASS 103 tests. |
| python tools/check_doxygen.py | PASS 159 generated pages and 2 rejected negative fixtures. |
| python tools/verify_ledger.py | PASS 97 registers, 52 procedures, 35 features; source structure, not physical behavior. |
| python tools/verify_references.py | PASS vendor hashes/extracts and local links, including this report and new guide. |
| python scripts/generate_version.py check | PASS current 0.1.0 generated header. |
| git diff --check | PASS. |

Final maintainability review kept one small callback classification, one derived
state query and one arithmetic helper. It found no duplicate state machine,
health admission latch, public test seam, retry or extra MDIO/clock call. Existing
frame accounting and silicon procedures remain unchanged.
SDK-link, installed/package consumer, remote CI and physical execution are NOT RUN
in this portable stage. Existing silicon gates and native_rx_quiescence remain
with their named later owners; none blocks passive accounting. Host results do
not qualify actual MDIO timing, wire faults or electrical reliability.
