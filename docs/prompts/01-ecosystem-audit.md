# 01 - Audit the current library ecosystem

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: None. Planning reports are starting evidence, not
completed implementation-stage output.

## Fresh audit and owned work

Inspect TMP1x2, INA228, PCA9555, TCA9548A, MB85RC, BME280 and SHT3x-main deeply;
inventory all other chip libraries under Projects. Read FieldCore-node's current
promptsets, owner contracts, representative module and tooling. Review TUSB32x's
promptset while distinguishing its partial implementation.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

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
