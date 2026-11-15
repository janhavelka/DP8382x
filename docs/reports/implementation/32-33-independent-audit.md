# Independent audit of prompts32 and33

Re-read both complete original prompts and audited their current implementation
and actual `328b5ed..2938ed7` changes, independently of the previous handoff.
Started on clean fetched `main` at
`2938ed718725b834fc1da105d93a3370d77f2e28`, equal to upstream. The baseline
205 Release CTests passed; the audit nevertheless reproduced three gaps.

## Findings and corrections

1. **Initialization readiness published before owned restoration completed.**
   Mandatory VOD/link proof set initialized before the new pin/interface tails.
   A failed HARD replay could leave initialized true with unverified settings
   and cleanup debt. Mutating tails now withhold readiness until full successful
   restoration. Tests cover72 combinations of pin/interface/both ownership,
   link/no-link profiles, NONE/PCS/HARD/RESTART, success/failure/cancellation,
   zero poll slices and separate repair. The RESTART command itself still
   preserves readiness; mandatory setup and replay are separate obligations.
2. **Pin-owned LED1 clock uncertainty did not reach dependent views.**
   The helper considered interface-owned routing but omitted
   `PinSettings::led1ClockOverride`. Accepted route-write errors or clock-status
   read errors could leave IRQ/WoL views verified. Passive dependency folding
   now checks either owner without modifying private source proof or creating
   circular restoration dependencies. Tests cover IRQ, direct WoL and WoL via
   IRQ, failed writes/reads, retained timestamps, bus-silent cached readers,
   live detector acquisition and explicit initialization repair.
3. **Declared interface substituted for current pin applicability.**
   Actual MII with declared RMII could admit LED3 or waive LED2 COL admission.
   Original SNLS647I table5-1 and table8-77 show why read-only mux reset0 is
   insufficient. Pin LED2/3 profiles now check RCSR[5] through the collector
   before mutation. Direct LED2 IRQ/WoL routes enforce the same rule in the fifth
   frame of their existing mux-read batch. Later contradictory mode observations
   withdraw trust and freeze an active pass before another mutation. Tests model
   actual strap/reset mode independently from board declaration, both mismatch
   directions, exact bounds, consumed RC samples, read-after-effect failure and
   later contradictions. Full pin bounds become85frames/24callbacks; GPIO
   source acquisition gains one frame without another callback.

No field mask, source encoding, polarity or reserved-bit correction was needed.
Existing tests independently cover every LED source0..E per admitted pin,
drive/polarity/blink extrema, RCSR elasticity/shift/revision, clock selection,
compatibility and isolation/power composition. Exact source conflicts remain
explicit gates: BASIC polarity, LED3 MII, pin31 codes5/6, PHYCR[5], PLL/recovered
clock activation and RMII RC interpretation. No unsupported permission was added.

A reviewer also questioned whether a failed raw RCSR prerequisite should erase
an earlier interface observation. It supplies no new mode word or timestamp;
the failed pin/source result is stale and the collector records possible loss.
The previous interface observation retains its existing time under the raw-read
contract. It is not published as a fresh successful interface acquisition.

## Evidence and review scope

Read COMMON, convention/evidence/coverage/engineering/verification contracts,
API OP1-17, architecture, variants and reference index. Lead owns public headers,
integration, documentation/progress and Git. One application/controller owner,
bounded terminal batches, the sole collector and existing restoration remain.
Siblings and vendor payloads are read-only. No workflow exists; first-set remote
CI remains prompt71. No new framework, registry or alternative procedure was added.

`audit32_pins` reviewed pin implementation/tests and sibling uncertainty patterns;
`audit33_lifecycle` reviewed deadlines, shared restoration, integration and tests;
`audit32_33_sources` independently reopened original PDF tables and checked
coverage, literal vectors and the final production diff. All used the inherited
capable model for source/ownership reasoning. Reviewers initially had read-only
scope; the first two subsequently received disjoint test-only assignments.
Independent build directories isolated verification. The lead checked the actual
findings, code, tests and diff and implemented the small production corrections.

Original SNLS647I August2025, archived SHA256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
pages6/69/83 and tables5-1,8-19/41/42/43/49/73-78/84/85/89 were rechecked.
The current [TI datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
still identifies revisionI. Reopened the Apache-2.0
[TI driver at450af114](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c).
No vendor implementation was copied.

Fresh read-only sibling checks, all clean:

- TMP1x2 `afd5088`: Config.h, src/TMP1x2Configuration.cpp validation/apply/verify,
  test/test_core.cpp registrations and examples/common/BoardConfig.h.
- PCA9555 `8b461b2`: src/PCA9555.cpp shadow invalidation/establishment,
  test/test_basic.cpp ambiguous-write/read-proof cases and registration,
  tools/test_run_i2c_hil_parser.py failure/strict-output cases.
- LDC1614 `da104689`: public readDeviceStatus/readDataReady,
  src/LDC1614.cpp deferred INTB versus destructive STATUS and test_basic cases.
- SCD41 `0b9d3d02`: src/SCD41.cpp runtimeSnapshot/data-ready implementation;
  no public IRQ surface was invented from unrelated sensor behavior.
- RV3032-C7 `15d7a278`: src/RV3032.cpp clock IRQ/masks and
  test/test_native/test_datetime.cpp registered native cases.
- FieldCore-node `c40ea834`: I2cOwnerTransport.h, Rv3032Module.cpp cached
  copyStatus/transfer and agent rules. Application ownership is adopted;
  I2C ACK/framing and sensor timing are not PHY evidence.

Updated public Doxygen, pin/interface/API/initialization/IRQ/WoL guides, independent
vectors, ledger, tests, CHANGELOG, progress and stale audit dispositions. README,
architecture, variants, version and portable examples were reviewed: no API,
family, build or example invocation changes require edits there. The existing
examples remain linked and tested; SDK and CLI implementation stay with later stages.

## Final verification

Windows GNU15.1/Ninja, C++17 Release with strict warnings and active assertions:

- PASS: `cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DDP8382X_BUILD_TESTS=ON`, build and CTest:205 registered tests.
- PASS: same full205 tests in `build/audit32-33-ubsan` with
  `-fsanitize=undefined -fsanitize-undefined-trap-on-error`. Strict compilation
  exposed signed-promotion warnings in two pre-existing IRQ/WoL test literals;
  explicit unsigned arithmetic fixes them without changing expected values.
- PASS: tests-OFF `test/consumer` configured/built in
  `build/audit32-33-consumer`, both registered CTests.
- PASS: `python -m unittest discover -s test -p "test_*.py" -v`:103 tests.
- PASS: `python tools/verify_references.py` (29 preserved vendor artifacts,
  27 extracts and local links), `python tools/verify_ledger.py`
  (97 registers/52 procedures/35 features), `python tools/check_doxygen.py`
  (234 pages and two rejected negative fixtures),
  `python scripts/generate_version.py check`, and `git diff --check`.

Physical clocks, LEDs,
IRQ/WoL delivery, repeater traffic and Q12/Q17 remain NOT RUN. SDK/controller and
installed-package qualification remain NOT RUN; source consumption is tested
separately. Remote CI remains NOT RUN by the prompt71 policy.

The final commit and verified upstream equality are recorded in the handoff;
this report does not embed its own commit hash.
