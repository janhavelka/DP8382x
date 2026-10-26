# 1 - Audit current driver APIs, state and ownership

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[SOURCE_MAP](SOURCE_MAP.md). Prerequisites: none. This is an evidence task;
the existing planning reports do not complete it. Briefly restate the applicable
rules, owned paths, source audit, checks and checkpoint before starting.

## Complete outcome

Produce the core comparison in `docs/library-comparison.md`: observed source
behavior, exact revision/dirty state and path/symbol, adopted/adapted/rejected
convention, and its DP8382x destination. Keep implementation claims planned.
Prompt 2 owns tooling and the broader inventory; this task closes the API,
state and ownership comparison without waiting for that independent work.

Before a new execution starts, create a new descriptive branch from the
synchronized intended base and record branch/base in implementation progress.
Resume an existing execution on its recorded branch. Never execute a new set
on main or create a different branch for each prompt or subagent.

## Work and source evidence

Inspect current public headers, actual implementations and representative
registered tests in TMP1x2, INA228, PCA9555, TCA9548A, MB85RC, BME280 and
SHT3x-main. Read applicable agent rules. Use `rg --files` to locate current
paths; unavailable sources are explicit limitations, not invented comparisons.

- Compare TMP1x2/PCA9555 `Status.h`: `Err`, `Status`, `errorName`,
  `Status::Error`, static message lifetime and backend detail. Preserve the
  selected shape in CONVENTIONS while replacing sensor/I2C meanings with MDIO.
- Trace TMP1x2 `HealthSnapshot`/`track`, PCA9555 three-state `DriverState`,
  INA228 `HealthPolicy` and current SHT3x/TCA9548A telemetry. Record conflicting
  conventions. The PHY selects four passive states; cable-down, configuration
  trust and application readiness remain separate from transport health.
- Trace `bind`, `begin`, `end`, `startInitialize`, `pollOperation` and
  `takeResult` to real bus/lifetime behavior. A sibling name does not prove a
  bus-silent operation. Compare borrowed callbacks, cleanup, result retention
  and actual lock granularity with complete indirect MDIO transactions.
- Compare PCA9555 `ObservedState`/`RegisterImage` with INA228 `HardwareState`.
  Preserve uncertain write effects and desired/observed/trusted distinctions.
- Inspect TMP1x2 responsibility-based `src/` and explicit CMake source lists.
  Record useful file boundaries without imposing one class/file per register.
- Read FieldCore-node `src/devices/i2c/Ina228Module.h`/`.cpp`, its actual
  contracts and owner adapters. Trace `bind`, `start`, `poll`, `takeResult`,
  `refreshStatus`, `snapshot`, `copyStatus` and `mapLibraryStatus`. Separate
  library responsibilities from application scheduling and cached publication.

For each substantive convention inspect an exercising test and its registration;
README claims or unregistered test source are insufficient evidence. Do not
fetch, build or edit all siblings incidentally. No FieldCore dependency or
imaginary Ethernet module belongs in this library.

## Delegation, documentation and acceptance

The lead owns the comparison and architecture decisions. Assign read-only
reviews of disjoint libraries to available subagents; use stronger reasoning
for state/ownership and a lighter model for path inventories. Record choices
and rationale, and verify contributions against source.

Update the architecture's core responsibility decisions, report and progress;
review README/CHANGELOG and explain relevant no-change decisions. Record the
source-documentation standard for future Doxygen without creating fake headers.
Use simple engineering language. Confirm every cited path/symbol and sampled
test registration, then run reference/link and diff checks. This task produces
reviewed evidence, not driver, build or hardware support.

Commit and sync the checked prompt output on the recorded execution branch;
also checkpoint coherent verified units during work. The lead alone mutates
Git. Follow COMMON for verified upstream equality and honest sync failures.
Remote CI belongs only to prompt 71 during this first execution.
