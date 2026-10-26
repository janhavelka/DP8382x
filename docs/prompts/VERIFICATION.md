# Verification contract and completion gates

## Evidence layers

| Evidence | What it establishes | What it cannot establish |
| --- | --- | --- |
| Reference/link check | Archived payload integrity, local documentation links | Current online revisions, correct silicon interpretation |
| Host behavior tests | Protocol ordering, validation, failure/state contracts | SDK integration or physical bus timing |
| Portable consumer | Public headers and real core link without frameworks | Another MCU's working MDIO adapter |
| SDK firmware build | Actual pinned SDK APIs and final image link | Flashed identity, straps, clocks, cable operation |
| Package consumer | Exported files sufficient in a clean environment | Registry publication or supported untested platforms |
| Remote CI | Named jobs for the recorded exact commit | A later dirty tree or another commit |
| Physical fixture | Recorded board/silicon/modes actually exercised | Universal variants, electrical compliance or servo safety |

Keep these separate in `docs/validation.md` when created. Each claim has a
command, result, artifact/revision, environment, date and limit. Record test
counts; zero tests and configured-but-unrun workflows are not success. Preserve
failures and exclusions instead of relabeling them as a pass.

## Host tests required with implementation

- Reconcile every authoritative register, non-reserved field and documented
  feature procedure with the source/coverage ledger, API/configuration route,
  CLI and independent tests. Source-proved nonprogrammable/inapplicable items
  need reasons; absent implementation stays an open gap. Include exact documented
  recipe-only addresses without treating arbitrary unlisted locations as safe.
- Scripted terminal MDIO backend with independently derived literal register
  vectors; exact address/value/order/call-count checks and errors at every step.
  Include writes applied before an error, unchanged read outputs, all-zero/all-one
  identity observations, unknown modes/revisions and unsupported variants.
- Complete indirect-transaction serialization, cross-owner contention, failed
  acquisition, release on all exits, lock order and no IDF-base bypass. Count
  logical operations and actual MDIO frames separately; zero cooperative budget
  causes no I/O. Test partial batches and no illegal interleaving.
- Register access attributes, reserved masks, W1C/read-clear/latch-low/counter
  behavior and independent test oracles. Pin unresolved datasheet ambiguities;
  do not choose a convenient fake behavior and call it silicon truth.
- Reset kinds/straps/restore, advertisement and forced-mode validation, 10BASE-Te
  setup, Auto-MDIX/robust/manual combinations, symmetric/asymmetric pause and
  full-duplex resolution, negotiation timing, link changes, partial config,
  stale observations,
  raw invalidation and wrong-policy links. Cover legal 10/100 and half/full
  combinations per device; board-specific restrictions do not limit core tests.
- Deadline exhaustion, clock wrap/quantization/frozen clock handling, bounded
  callback budget, cancellation at side-effect boundaries, retained results,
  independent instances, lifecycle reuse, no allocation in core hot paths.
- Health versus link/config evidence, no-I/O snapshots, saturating counters,
  timestamp validity and diagnostic traffic attribution. A cable-down observation
  and invalid argument must not turn into an MDIO transport fault.
- Native CLI parser/output/owner execution, all malformed input cases, bounded
  console under saturation, truthful errors/restoration, firmware provenance and
  Python fragmented/duplicate/reboot/truncation cases.

Use CMake/CTest with tests OFF by default for consumers. Register real suites,
strict compiler warnings, applicable sanitizers and a second host compiler where
available. Assertions must remain active in Release tests. Framework-boundary,
metadata, generated-file, source/document and package guards supplement behavior
tests. Do not build a large AST/checker framework for simple invariants.

## Framework and application evidence

Pin the ESP-IDF release and target after current source audit. Existing reference
baseline is v6.1, commit `fff9895c82d744c7237be8847347bdd1b07c6643`, not a promise
that it is the final tested range. A header shim or syntax-only build is not
native IDF evidence. Build/link the actual component and P4 examples, test callback
lifetime/start/stop/delete paths, dependency declarations and the IEEE 802.3
base boundary. State all SDK API/timing restrictions discovered.

Do not copy I2C peers' S2/S3 matrices into an EMAC example. Additional frameworks
or targets require a working MAC/MDIO integration and their own build evidence.
An Arduino wrapper is conditional on a real supported target/backend; it is not
required merely because the I2C libraries use Arduino. An ordinary non-IDF C++
owner consumer is required, even when only P4 has linked firmware evidence.

EtherCAT evidence separates PHY link operation, raw MAC frame handoff and actual
master/servo cycle timing. Network examples may test DHCP/static IP and a local
peer; internet/DNS reachability failures are application evidence, not automatic
PHY failure. Power delivery and PoE tests are outside this library.

## Physical qualification matrix

For each tested exact part/revision, record board/netlist revision, interface,
clock source/direction, reset/strap values, PHY address, SDK/firmware identity,
link partner/cable, mode and test equipment. Recheck first-board wiring before
flashing; the board snapshot is not final netlist authority.

Exercise cold boot, each applicable reset/restart, configuration restoration,
100/full-only advertised policy, general 10/100 behavior on a suitable fixture,
unplug/replug, peer restart, wrong partner capability, repeated link flaps,
transport faults where controllable, bounded recovery, power-down/wake when
supported and requested diagnostics with restoration. Test both successful and
inconclusive/no-link outcomes. Keep manual fixtures and electrical tests explicit.

Observe RMII clock/straps/reset timing with appropriate instruments when claiming
those properties. Packets passing or link LEDs do not prove signal compliance,
deterministic latency or EtherCAT timing. A-frame or sibling variant tests do not
qualify another silicon revision. Electrical compliance requires its own setup.

HIL runner creation and dry-run tests are deliverables even without attached
hardware. Actual qualification remains NOT RUN/BLOCKED until the fixture exists
and is in authorized scope. Do not operate a loading machine or move servos as
an incidental library test. Continue independent docs/build/package work and
keep incomplete hardware gates visible at final handoff.

## CI, packaging and local planning checks

Source documentation is checked from stage 05 onward, not deferred to CI or stage
32. Run the established Doxygen check when public declarations, source comments
or documentation inputs change. Verify warnings and required public coverage,
inspect representative generated pages, and test that missing documentation and
invalid parameter references fail the configured checker. Generated page presence
alone is not documentation coverage. Review README/CHANGELOG and relevant guides
with the behavior; a no-change reason is sufficient for an unaffected file.

Reuse actual workflow commands and pinned dependencies; retain required hooks.
Under root AGENTS' user-requested first-set exception, remote CI execution and
run-status checking belong only to prompt 29. Prompts 01-28 and 30-34 use relevant
local checks, including SDK/package builds, without dispatching/polling/waiting
for remote runs even after major fixes or commit/sync. At prompt 29 record actual
jobs/results and tested SHA; later work keeps that evidence tied to its original
commit. Never claim an older green SHA covers the final implementation.

Automatic runs can still occur on push. Preserve triggers, hooks and required
merge/release checks; do not disable or bypass them. After the first execution,
the parent CI policy resumes unless the user instructs otherwise. Keep fixture
jobs opt-in to declared hardware; don't fake hardware in hosted CI.

Test packages by extracting into a clean temporary directory without source-tree
or sibling fallbacks. Validate archive paths, required notices and excluded
vendor/reference/build/HIL material. Build a minimal consumer and linked SDK
example from the exported contents. Generate metadata deterministically and
check versions match. Publication is a separate action from local package tests.

For this documentation-only promptset, current executable checks are:

```sh
python tools/verify_references.py
git diff --check
```

Review all numbered prompt links, dependency ordering, shared names, requirement
coverage and truthful progress entries. Future commands are introduced with
their implementation; don't claim nonexistent tests or firmware builds ran.
