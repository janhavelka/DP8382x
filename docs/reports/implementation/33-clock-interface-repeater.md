# Prompt 33: clock, interface and repeater controls

Implemented the six source-admitted interface, clock, compatibility, repeater
and isolation ledger procedures through `configureInterface`, with passive
`interfaceSnapshot` and the existing initialization/verification restoration path.
The [guide](../../interface-clock.md) defines legal settings and board admission.

## Rules, ownership and checkpoint

Started on clean fetched `main` at
`3e6e0bfb86d6ed681ba3dde191ff9973078ba31e`, equal to `origin/main`.
Read the full pasted/repository prompt 33, COMMON and shared evidence, convention,
coverage and verification contracts, parent/root instructions, architecture,
variants and reference index. The lead owns public values, integration, shared
documentation, ledger and Git. Siblings are read-only; vendor bytes are unchanged.
One application owner serializes each instance and complete MDIO transactions.
Board clocks, MAC stop, wiring, scheduling and physical qualification remain
application responsibilities. No workflow exists; remote CI remains prompt 71.

## Fresh source and sibling audit

Original SNLS647I August 2025 SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
sections 8.3.7-10 and tables 8-19/28/29/35/41/73/84, establish the independent
literal vectors. RCSR RC bits remain collector-owned, reserved bit 6 is preserved,
elasticity encodings are non-monotonic and GENCFG IPG is distinct from CR2's
RX_ER-idle control. BMCR isolation preserves software power intent, rather than
copying a pin-OR power observation. ENHANCED-only clock override does not imply
BASIC lacks the runtime repeater route. CLKOUT's name does not guarantee a
25MHz output for every reference. Electrical +/-50ppm requirements remain
separate from the documented +/-100ppm elasticity arithmetic.

Current TI Ethernet software main remains
`450af1144b8caeb58724bcbae081b409f6bf5c3a`.
`zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c`
(Apache-2.0), lines 381-426, provides a comparison for MII/leader/follower masks.
Its mode switch does not establish collector retention or verified application;
no vendor implementation was copied. Existing SDK defects remain catalogued.

Read-only sibling sources inspected:

- TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40`: Config.h,
  src/TMP1x2Configuration.cpp validators/beginApply/stepApply/verify,
  test/test_core.cpp zero-I/O, mismatch and partial-apply registrations,
  examples/common/BoardConfig.h and tools/test_hil_tmp1x2_runner.py.
- PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970`:
  src/PCA9555.cpp shadow invalidation/establishment, test/test_basic.cpp
  ambiguous writes, partial raw writes and read-versus-write proof; native
  examples/espidf_basic/main/main.cpp and tools/test_run_i2c_hil_parser.py.
- FieldCore-node `c40ea8341edf8d665460b902cafefbe857a7f73f`:
  include/TunnelMonitor/i2c/I2cOwnerTransport.h and I2cDeviceBinding.h,
  src/devices/i2c/Rv3032Module.cpp snapshot/copyStatus/transport, and its
  test/native/test_component_rv3032/test_rv3032_module.cpp registration.

Adopted complete validation, separate desired/observed/trust and application
ownership. Rejected I2C ACK rules, sensor delays and cached views as physical
clock evidence. Naming follows existing PascalCase values/camelCase operations.

## Implementation and independent review

`interface33_sources` independently reviewed original PDF tables and current TI
and sibling sources, then owned only the new guide. `interface33_review` owned
only the cohesive interface source and subsequently reviewed integration.
`interface33_tests` owned only independent literal tests and the portable example.
The inherited capable model was used for silicon, deadline and ownership reasoning;
separate build directories prevented verification collisions. The lead reviewed
their actual code and diffs and integrated shared state, tests registration and Git.

Whole-profile and complete-budget admission precede intent changes. Default RMII
uses 32 frames/13 callbacks; full clock/isolation uses 39/16; full read-only verification
uses 19/9. Each C22 or five-frame MMD RMW remains indivisible. There are no silicon
waits, retries, hidden rollback, clock switching or two-PHY registries.

The shared tail restores interface before dependent pins/IRQ/WoL and final power.
Reset, power and diagnostic restoration reuse it. VERIFY reads only owned masks.
RCSR consumes raw RC evidence through the single collector, retaining partial
samples and possible history loss separately from ordinary snapshot validity.
Failed/uncertain writes retain OP-9 debt and require separately admitted recovery.
Two-PHY repeater setup retains independent results if the second PHY fails.

Review corrections cover mode validity after failed reads, earlier mismatch
retention between polls, owned PLL/power preparation and source-order restoration.
Passive LED1 pin/IRQ/WoL trust now depends on verified owned clock routing; failure
does not leave a falsely verified dependent view or create cyclic repair rules.
Retained operation results fold those dependencies after failure invalidation.

## Documentation and verification

Updated public Doxygen, README/CHANGELOG, API/architecture/behavior/initialization,
access and interface guides, ledger/coverage/vectors/source notes, test guide,
progress and [portable owner example](../../../examples/portable/Interface.h).
Variants and version truth need no change: no additional family is supported.
Future CLI uses the same typed owners and passive evidence.

Windows GNU 15.1/Ninja, C++17 Release with strict warnings and active checks:

- PASS: `cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DDP8382X_BUILD_TESTS=ON`, build and registered CTest:205 tests including eight
  interface suites and their deliberately failing Release assertion probe.
- PASS: separately configured tests-OFF `test/consumer` in
  `build/interface33-consumer`, build and both registered consumer CTests.
- PASS: `python -m unittest discover -s test -p "test_*.py" -v`:103 tests.
- PASS: `python tools/verify_references.py`:29 preserved vendor artifacts,
  27 text extracts and all local links; `python tools/verify_ledger.py`:
  97 registers,52 procedures,35 features (structure/coverage only).
- PASS: `python tools/check_doxygen.py`:234 generated pages and two rejected
  negative fixtures; `python scripts/generate_version.py check` and
  `git diff --check`.

SDK/controller and installed-package qualification are NOT RUN; this stage checks
portable source consumption. Remote CI is NOT RUN, reserved for 71. Physical
Q12 clock/mux and Q17 RMII RC polarity/clear semantics remain NOT RUN, as do board
clock edges, repeater traffic and electrical timing. PLL/recovered-clock mux
activation remains source-gated; no guessed permission closes that conflict.

Final synchronized commit/equality is recorded in the handoff, avoiding a
self-referencing report commit. Unrelated source and physical gates remain open.
