# 01 - Audit the current library ecosystem

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: None. Planning reports are starting evidence, not
completed implementation-stage output.

Read [CONVENTIONS](CONVENTIONS.md). Start with a short rule reminder: this stage's
scope, owned files, current sibling evidence, one execution branch, simple
language, documentation/local checks and commit/sync. Choose available subagent
models by task complexity under COMMON; the lead owns decisions and Git.

## Fresh audit and owned work

Inspect TMP1x2, INA228, PCA9555, TCA9548A, MB85RC, BME280 and SHT3x-main deeply;
inventory all other chip libraries under Projects. Read FieldCore-node's current
promptsets, owner contracts, representative module and tooling. Review TUSB32x's
promptset while distinguishing its partial implementation.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

For a new execution, create and record its new branch/base before this audit
stage. Resume an already recorded execution on that same branch. Follow root
AGENTS; do not start a new set on main or reuse a previous set's branch.

Produce `docs/library-comparison.md` and the initial execution record. Record exact
revisions/dirty states and concrete source paths for public naming, layout, Status,
configuration/health, lifecycle, callbacks, serialization, examples, CLI, Python
runners, tests, metadata, generated versions, CI and agent guidance. Separate existing
code from aspirational docs. Identify incompatible peer conventions rather than
inventing one universal API.

Inspect this repo's reference checker, parent CI rules and root AGENTS' explicit
first-set remote-CI exception before proposing new checks.
Record the actual toolchain availability, configured remote and repository ownership; do
not guess owner handles or inherit sibling push/merge/fixture permissions. Review actual
workflow commands and test registration, not only badges. Siblings stay read-only; don't
fetch/change/build all of them as an incidental audit.

Map each adopted convention to its source and PHY adaptation. MDIO is not I2C, a lost
Ethernet cable is not an absent management device, and multi-frame indirect operations
cannot share an I2C-style per-transfer poll budget without a transaction guarantee.
Decide which peer examples and runners are strong behavioral precedents.

Inventory likely consumers: ordinary C++ MCU owners, native P4 Ethernet application and
an application-owned EtherCAT master. Document what FieldCore currently supplies and
what a future integration must own. Do not add FieldCore dependency types or an
imaginary Ethernet module.

## Concrete comparison and decisions

Begin with these actual precedents, then verify they still exist and behave as
observed. Include conflicting conventions; do not label a preference universal.

| Topic | Source to inspect | Decision to record for this PHY |
| --- | --- | --- |
| Status | TMP1x2/PCA9555 `Status.h`: `Err`, `Status`, `errorName`, `Status::Error` | Preserve static message lifetime, full backend detail and helper spelling; use MDIO-specific errors |
| Health | TMP1x2 `HealthSnapshot`/`track`; PCA9555 three-state `DriverState`; INA228 `HealthPolicy` | Selected four-state passive telemetry, exact counting unit, no offline admission or cable-as-bus-fault rule |
| Lifecycle | TMP1x2 `bind`, `begin`, `end`, `startInitialize`, `takeResult`; PCA9555 `pollOperation` | Select one documented vocabulary; do not copy competing aliases or assume `begin` is bus-silent |
| Configuration evidence | PCA9555 `ObservedState`/`RegisterImage`; INA228 `HardwareState` | Separate desired, observed and trusted configuration; failed writes preserve uncertainty |
| Source structure | TMP1x2 `src/TMP1x2Configuration.cpp`, `TMP1x2Operations.cpp`, `CMakeLists.txt` | Split by real responsibility, explicit portable/SDK source lists, no file per register |
| CLI/runner | TMP1x2 `Cli::feed`/`processCommand`/`tick`, 24Cxx identity parser, INA228 framed HIL parser | One shared grammar, retained terminal results, exact correlation and actual flashed-image evidence |
| Source documentation | TMP1x2 `Doxyfile`, `tools/check_doxygen.py`, PCA9555 public headers | Doxygen with behavior and ownership from the first source-bearing stage; checked generated API pages |

For each row record source path/symbol, revision/dirty state, what the implementation
actually does, the adopted/adapted/rejected rule, and where the DP8382x contract
will live. Inspect a representative test and its registration. A claimed test that
is not registered is a gap, not proof. Identify stale sibling docs without editing
them. MDIO register semantics still come from the exact TI evidence.

Create a concise future file-responsibility map from CONVENTIONS. Name the core,
SDK adapter, shared CLI, host tests, runner, version tool and Doxygen owner. Map
consumer requirements to those layers rather than adding a FieldCore-specific
service. Record which proposed files are not needed yet.

Choose useful delegation: bounded path inventories can use a lightweight model;
API/state/ownership comparisons need stronger reasoning and lead review. Give
each task owned paths, read-only sources, exact questions and acceptance evidence.
Record available model selection and rationale without requiring a fixed model
name in future sessions. Agents may share the execution branch with disjoint
owned paths; the lead alone switches branches, stages, commits and syncs.

## Documentation in this stage

Update `docs/library-comparison.md`, the current execution report, relevant
architecture/design notes and the progress record. Review root README and
CHANGELOG for the audited scope and decisions, keeping all driver support claims
planned. Record the source-documentation standard and future Doxygen command in
the execution plan; no C++ header or fake API is needed for a planning-only audit.
Use plain statements such as "PCA9555 uses three health states; this PHY plan
selects four passive states and keeps link state separate."

## Acceptance and handoff

Check every cited path and sampled declaration/dispatcher/test registration. Review a
topic-by-topic adopted/adapted/rejected table and a gap list against the request. Run
existing reference/link and diff checks. No implementation, build or HIL claim results
from this audit.

Expected output: Current conventions, evidence inventory and consumer boundaries.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
