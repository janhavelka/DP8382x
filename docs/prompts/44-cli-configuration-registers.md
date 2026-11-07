# 44 - Add complete identity, configuration and register CLI routes

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites: [43](43-cli-foundation.md),
[38](38-esp-idf-state-arbitration.md), and implemented identity, typed
settings and safe register procedures.

## Current evidence and ownership

Reopen INA228/TCA9548A command/result implementations and Python expectations,
TMP1x2 shared dispatch/output and MB85RC restoration classification with its
tests. Inspect current headers, source, native front ends, registration and
agent rules; record revisions, exact paths and adopted/adapted/rejected
behavior. Follow the frozen grammar rather than choosing new aliases or core
setters.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Use [core register facts](../reference/library-ti-core.md) under
[EVIDENCE](EVIDENCE.md). Distinguish C22 register `0x1F` from command mask
`0x8000`; direct register addresses outside `0..31` must not be truncated into
valid frames. MMD requests require explicit DEVAD and exact wire offset; a
printed EEE address is not proof of that tuple. Verify names, legal values and
help against the exact part, and keep raw-error diagnostics from masquerading
as corrected typed feature behavior.

## Implement

Extend the existing processor with identity/probe, bounded explicit discovery,
desired/observed settings, validation, apply/verify/invalidate, Clause 22/MMD
read, safe dump and guarded raw writes. Bound dump row count and total
acquisition budget; produce explicit partial validity when a later read fails.
Every relevant programmable setting needs a usable typed configuration path,
legal-value help and effect/result reporting; raw access cannot substitute for
a multistep feature procedure.

Validate arity, units, ranges, enums and cross-fields before admission. Discovery
reads only PHYIDR1/2 over an inclusive requested range with one deadline/frame
budget. Default upper strap address: DP83826/A7, DP83825=3, DP83822=31; other
0..31 ranges need explicit argument. Address0 is ordinary, with no invented
broadcast behavior. FFFF/0000 means no response evidence; transport errors are
per-address. Candidates never select/initialize a profile. Isolation/power-down
still answer MDIO and cannot be inferred from discovery.

Safe dumps skip side-effect/read-clear registers and indirect selectors/data
ports unless using their typed owner path; explicitly requested event
acquisition uses the one collector. Raw MMD selection is never assembled from
separate CLI writes to C22 `0x0D/0x0E`. Rejected input uses declared/cached
capability evidence; a command requiring fresh qualification performs only the
documented bounded reads before mutation, with that work separately reported.
Raw writes use the admitted complete transaction path and invalidate affected
evidence, preserving uncertain effects and backend detail. Cached
configuration remains distinct from observed stable bits and verified
ownership. All disruptive writes require maintenance admission and reject an
active application session before I/O.

IDF-attached configuration uses the SDK-aware owner. Mirror AN enable through
supported stopped-driver public controls while full advertisement/chip fields
use shared procedures. Raw BMCR mutations require reconciliation before
restart. Test AN-to-forced, getters and stop/start together; a mutex alone
cannot repair stale SDK cache. Keep generic legal 10/100 behavior separate
from the 100/full application policy, and distinguish absent silicon support,
incomplete software and unavailable backend/fixture.

Use `docs/feature-coverage.md` as the one matrix: one row per ledger procedure
and feature ID, exact part/mode, command or cfg path, owner route, record,
host test name and physical case or reasoned N/A. Require every ID and field
owner to resolve to a complete row. Prompts44/46/47 append CLI cells,47 closes
those cells,55 closes primary coverage;53 adds only gaps. `docs/cli.md` owns
grammar/records, not a duplicate matrix.

Discovery, dump, selftest, watch and stress are CLI-local sequencers over
public calls: a small fixed index/count/deadline struct advances from tick,
one batch per tick. They never perform uncoordinated MDIO or duplicate a chip
procedure. Discovery uses a separate probe driver on the shared transport:
bind each requested address, readClause22(02/03), end. Before each batch check
the bound driver's operationSnapshot and reject while RUNNING or busQuietValid;
this preserves hard-reset silence and normal report/quarantine checks.

## Acceptance

Execute compiled CLI transcripts for each handler and boundary, using
independent literal vectors for representative procedures. Prove zero
frames/state changes for rejected/passive input, finite discovery, safe
side-effect skipping, busy results and SDK-aware admission. Preserve the
single actual CLI fixture/schema.

Document handler inputs/effects and maintain `docs/cli.md` with legal values,
side-effect-read rules and success/failure examples. Update feature/CLI
coverage.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
