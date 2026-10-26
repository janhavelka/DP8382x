# Implementation progress

## Planning baseline

Date: 2026-10-26. Starting revision:
`35ced9bb9d0b05c1c0b41ecff7cee5e757bcb163`, branch `main`.
The initial DP8382x working tree was clean. A configured origin was observed;
the original setup instruction saying no remote was selected was historical.
No publication or Git synchronization is claimed by this planning work.

The [audit reports](reports/README.md) and [34-stage promptset](prompts/README.md)
are prepared. **No driver/API implementation, firmware example, library test
suite, CI, CLI or HIL runner has been created or executed here.**
Current executable verification remains the reference/documentation checker.

The follow-up scope requires complete documented chip functionality beyond the
initial board, with mandatory [full-coverage research](prompts/FULL_COVERAGE.md),
an exhaustive [DP83826I inventory](reference/dp83826i-feature-inventory.md) and
[additional public manufacturer sources](reference/public-research.md).
These are planning artifacts; all implementation stages remain NOT STARTED.

## Execution record

Execution branch/base: not started. Record the actual values when implementation
begins. Keep changes and reports scoped to the authorized task; another
repository's branch or commit/push rules are not inherited.

| Stage | Implementation status | Evidence |
| --- | --- | --- |
| [01 Audit the current library ecosystem](prompts/01-ecosystem-audit.md) | NOT STARTED | None |
| [02 Establish exact silicon and family capabilities](prompts/02-silicon-capabilities.md) | NOT STARTED | None |
| [03 Resolve the portable core and ESP-IDF ownership boundary](prompts/03-framework-ownership.md) | NOT STARTED | None |
| [04 Specify API, Status, state and timing contracts](prompts/04-public-contracts.md) | NOT STARTED | None |
| [05 Build the portable foundation and version tooling](prompts/05-host-foundation.md) | NOT STARTED | None |
| [06 Implement bounded MDIO and register transactions](prompts/06-mdio-register-access.md) | NOT STARTED | None |
| [07 Implement identity and supported-device selection](prompts/07-identity-capabilities.md) | NOT STARTED | None |
| [08 Implement lifecycle, reset and bounded restoration](prompts/08-reset-lifecycle.md) | NOT STARTED | None |
| [09 Implement generic 10/100 link configuration](prompts/09-link-configuration.md) | NOT STARTED | None |
| [10 Implement link snapshots and change evidence](prompts/10-link-observation.md) | NOT STARTED | None |
| [11 Complete health, invalidation and recovery primitives](prompts/11-health-recovery.md) | NOT STARTED | None |
| [12 Implement supported loopback and BIST](prompts/12-loopback-bist.md) | NOT STARTED | None |
| [13 Implement supported cable diagnostics](prompts/13-cable-diagnostics.md) | NOT STARTED | None |
| [14 Implement remaining documented PHY controls](prompts/14-interrupt-power-led.md) | NOT STARTED | None |
| [15 Implement the native ESP-IDF PHY adapter](prompts/15-esp-idf-adapter.md) | NOT STARTED | None |
| [16 Create the ESP32-P4 ordinary Ethernet example](prompts/16-p4-network-example.md) | NOT STARTED | None |
| [17 Demonstrate EtherCAT application ownership boundaries](prompts/17-ethercat-owner-example.md) | NOT STARTED | None |
| [18 Build the shared portable CLI and console](prompts/18-cli-foundation.md) | NOT STARTED | None |
| [19 Expose implemented PHY behavior through the examples](prompts/19-cli-feature-coverage.md) | NOT STARTED | None |
| [20 Implement the bounded Python session and artifact runner](prompts/20-python-runner.md) | NOT STARTED | None |
| [21 Build finite observation, fault and diagnostic suites](prompts/21-hil-fixture-suites.md) | NOT STARTED | None |
| [22 Close protocol, ownership and portability test gaps](prompts/22-core-regression.md) | NOT STARTED | None |
| [23 Qualify the DP83826E software path](prompts/23-dp83826e.md) | NOT STARTED | None |
| [24 Implement separately qualified DP83826AI and DP83826AE](prompts/24-dp83826a.md) | NOT STARTED | None |
| [25 Implement the documented DP83825I](prompts/25-dp83825i.md) | NOT STARTED | None |
| [26 Implement exact documented DP83822 variants](prompts/26-dp83822-family.md) | NOT STARTED | None |
| [27 Prove independent application and framework consumption](prompts/27-portable-consumers.md) | NOT STARTED | None |
| [28 Validate packages, metadata and ownership](prompts/28-packages-metadata.md) | NOT STARTED | None |
| [29 Establish the actual CI and local verification matrix](prompts/29-ci-matrix.md) | NOT STARTED | None |
| [30 Execute the available physical qualification matrix](prompts/30-hardware-qualification.md) | NOT STARTED | None |
| [31 Reconcile user-facing guides and feature coverage](prompts/31-user-documentation.md) | NOT STARTED | None |
| [32 Audit source documentation and maintenance ownership](prompts/32-maintainer-documentation.md) | NOT STARTED | None |
| [33 Review silicon correctness and ownership independently](prompts/33-independent-core-review.md) | NOT STARTED | None |
| [34 Close implementation, tooling and qualification claims](prompts/34-final-integration-review.md) | NOT STARTED | None |

A stage can be complete as software while its separately tracked physical
qualification remains NOT RUN, if its acceptance explicitly permits that.
Stage 30 cannot be complete without the required available-hardware evidence;
final overall completion must disclose and retain any outstanding required gates.
Do not change statuses merely because a prompt or future command exists.

## Per-stage report template

Create `docs/reports/implementation/NN-topic.md` during execution, with:

- baseline branch/revision and dirty state; prerequisites and owned files;
- current sibling audit with exact paths and decisions adopted/adapted/rejected;
- exact TI/SDK references, decisions, implementation/API changes and consumers;
- commands and results for host, CLI/Python, SDK, package and remote CI separately;
- physical fixture/image identity and evidence, or explicit NOT RUN/BLOCKED;
- unresolved gates, next eligible stage and any actual authorized Git result.

Record tested commit identity without a self-referential commit-hash update loop.
Historical evidence does not verify a later changed tree.
