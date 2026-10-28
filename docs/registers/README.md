# DP83826I ledger schema (version 1)

[dp83826i.json](dp83826i.json) is the authoritative research ledger for exact
DP83826I / SNLS647I. It is deliberately a small part-specific JSON document,
not a register-code generator. The older reference catalogues are source inputs.
Prompts 3–5 complete all primary-part source dispositions, including unresolved gates.
No record claims an implemented driver merely because its research is complete.

Run `python tools/verify_ledger.py` and
`python -m unittest discover -s test -p "test_*.py" -v` from the repository root.
The checker uses only the Python standard library. Its structural and coverage
checks do not prove silicon numbers; independent literal oracles live in
[register vectors](../register-vectors.md) and the executing test suite.

## Common representation

All numeric bit values, masks and addresses are uppercase hexadecimal strings
with a `0x` prefix; widths, stage numbers, page numbers and schemaVersion are JSON
integers. Booleans are not integers. Semantic `id` values are globally unique,
including field IDs. References use these IDs rather than display names. Empty
arrays mean none; `null` means unknown, never zero. Text must explain unknowns.
Every register, field and procedure has nonempty `sources`: objects containing
`source` (a key in root sources) and `location` (section/table/page).

The root contains `schemaVersion` (1), `variant` (DP83826I), `specification`
(SNLS647I), `sources`, `identity`, `timings`, `registers`, `procedures`, `features`
and `gates`, plus `printedAddresses`, `pauseResolution` and `writeActions`. Source entries have `id`, `path`, `revision`, `authority` and
`license`; local paths are repository-relative and must exist. The archived
payload hashes remain in the original provenance manifest.

## Registers and fields

Each register has `id`, `name`, `modes` (BASIC and/or ENHANCED), `space` (C22 or
MMD), `devad` (null for C22), `address`, `width` (16), `aliases`, `resetValue`,
`resetKnownMask`, `resetNotes`, `reservedMask`, `access`, `readEffects`, `sources`,
`researchStage` (3, 4 or 5), `status` (COMPLETE, RECIPE or PENDING), `fields` and `gates`.
`aliases` contains complete `{space, devad, address}` alternative access tuples.
C22 addresses are 0..31; MMD offsets are 0..65535 and DEVAD is 0..31.
REGCR/ADDAR never have indirect aliases. Duplicate primary or alias tuples are
invalid, even when names differ. Standard registers other than these two have
explicit DEVAD-31 aliases; there is no inferred bank-wide write permission.
No aliases for an MMD-primary record are currently established. They must remain
empty until an explicit source-backed allowance is added with its tests; an alias
cannot bypass hole, direct-only selector or unresolved printed-address restrictions.

`resetValue` contains only known one bits, and must be a subset of
`resetKnownMask`. It is a partial value, not a value to write. A known zero bit
has its mask set and value clear. Defaults absent from the specification stay out
of the known mask. Documented dynamic reset values describe the reset instant,
not persistent readback; strap/mode dependencies and contradictory printed defaults
are stated in resetNotes and gates. PENDING is no longer accepted in the completed
primary inventory. An all-reserved completed register has
reservedMask 0xFFFF and no fields: it has no ordinary writable bits.

RECIPE means the exact procedure-only write is researched but field/reset meanings
are not documented. It keeps resetValue/resetKnownMask/reservedMask
null and fields empty. Seventeen loopback, EEE and shared power/compliance offsets
use this status. Known prose bits are exact actions, not invented field partitions.
Its source-backed action does not permit arbitrary reads,
writes, saved-word restoration or a claimed reset value.

Completed registers partition all 16 bits between reservedMask and disjoint
fields. Each field has `id`, `name`, `mask`, `access`, `legal`, `prohibited`,
`neutral`, `readEffects`, `validity`, `sources` and `owner`. Legal/prohibited
arrays hold **unshifted** encodings. `legal: "range"` means all encodings fitting
the contiguous field except prohibited values; reserved encodings are prohibited.
`neutral` is the unshifted write-command-neutral value or null for ordinary
state/read-only fields. Access vocabulary is R, RH, RC, RCH, RW, RHW, W1S,
W0C, W1C. W1S/W1C neutral is zero; W0C neutral is all field bits one. Mixed
register writes must compose field semantics, not echo an arbitrary read value.
The CDCR start field retains its published RW access while requiring neutral zero:
its description explicitly defines a self-clearing trigger, not persistent state.
Owner is a feature ID. Research for a non-core shared field may remain in its
feature's assigned stage even when the numeric field partition is complete.

## Procedures, features and gates

A procedure has `id`, `feature`, `modes`, `sources`, `ownedMasks` (objects with
`register` and `mask`), `prerequisites`, `steps`, `waits`, `timeout`,
`partialEffects`, `cleanup`, `restoration`, `gates`, and `owner`, with optional
`writeActions` IDs for the exact exceptional writes below. These text/list
contracts are required and nonempty except waits/gates/ownedMasks may be empty
when no such action exists. `steps` is an ordered nonempty list of instructions.
Owned masks must fit documented non-reserved writable fields and the owning
feature's register coverage. There is no read-only ownership exception. Read-only observation
procedures use no owned masks. Exact recipe exceptions must be documented rather
than legalized by a broad mask. Owner is an object with `api`, `cli`, `test` and
`implementationStage`; these are planned responsibilities, not frozen signatures.

A feature has `id`, `name`, `researchStage`, `implementationStages`, `registers`,
`procedures`, `owner`, `scope` (SOFTWARE, HARDWARE_ONLY, OUTSIDE_PHY or ABSENT),
`evidence`, `gates` and `notes`. `evidence` has separate `documented`,
`implemented`, `hostTested`, `sdkBuilt`, `hardwareQualified` values, each one of
COMPLETE, PARTIAL, PENDING, NOT_RUN or NOT_APPLICABLE. Host-tested means driver
behavior, not successful ledger validation. Feature register/procedure links and
field owners must resolve. Every register has a feature owner and every feature
has API/CLI/test ownership (hardware-only/outside features explain applicability).
Every software feature has a procedure and a primary implementation owner in
prompts 11–36. Straps, outside-PHY work and absent capabilities have fixed explicit
scopes; board wiring cannot reclassify programmable controls as hardware-only.
Later SDK/CLI/fixture stages are integration/test owners, not replacements for
the primary software owner.

`writeActions` entries have `id`, `register`, `mask`, `value`, `neutral`, `effect`,
`prerequisites`, `sources`, `gates` and `feature`. Values are positioned register
bits, unlike unshifted field encodings. The checker accepts only 28 audited
action tuples: BICSR1 capture, digital loopback, EEE/LPI, DPD entry and compliance.
BICSR1's read counter remains read-only; its write command
overlaps that read view without creating overlapping field partitions. Its zero
neutral protects ordinary IPG writes. Whole-word recipe actions have null neutral
because there is no documented general command field. Every action must be used
by its exact owning procedure, which must retain the action's evidence gates.
Even another procedure of the same feature cannot borrow an exceptional action.
Unknown actions, changed values/masks/owners and invented writable aliases fail.

A gate has `id`, `conflict`, `decision`, `nextEvidence`, `ownerStage` and `sources`.
Gates name unresolved behavior; they are not claims of unsupported hardware.
Identity records and timing records carry independent source citations and
explicit policy/qualifier text. Timing values separate silicon MIN/NOM/MAX from
application deadlines; no nominal value becomes a completion bound.

`identity` contains `oui`, `id1`, `ouiLowMask`, `modelMask`, `revisionMask`,
`models` and `combinedIds` keyed by BASIC/ENHANCED, `documentedRevision`,
`policy` and `sources`. Timing entries contain `id`, numeric `value`, `unit`,
`qualifier`, `start`, `end`, `conditions` and `sources`. The number describes the
qualified source interval, not a driver timeout. Timing numbers must be
nonnegative and representable within the finite binary64 range; this is a JSON
tool representation bound, not a silicon timing limit. `pauseResolution` contains
`fields` (ordered local/partner encodings), `phycrRxTx` (row-local, column-partner
matrix), `eligibility` and `sources`.

`printedAddresses` contains `{id, devad, printedAddress, sources, gates, feature,
status, controls}` records, with optional explanatory `disposition` text.
`status` remains BLOCKED; `controls` is a nonempty list of source descriptions
distinguishing exact-part facts from comparison-only status fields.
These are **not wire tuples** and cannot be passed to an MMD helper.
They retain unresolved EEE notation without silently stripping address digits.
DEVAD uses hex strings, like other numeric address fields. A disputed MSE address
is retained in its gate, not added as an established register alias.

## Coverage and evolution

The validator requires the exact core register set and the complete 79-entry
summary inventory, the feature-only address entries and all 12 core procedures
with their assigned features. Removing a row cannot
make coverage pass. It checks the seeded feature families and their stage owners.
Prompt 4 additionally requires all 37 diagnostic rows (34 detailed records and
three recipe-only records), eight documented feature families and 16 procedures.
Prompt 5 requires the final 39 rows (25 detailed and 14 opaque recipe records),
24 further procedures and complete research for every feature. The final
inventory is 96 records, 283 fields, 52 procedures and five blocked printed
associations. All 79 detailed entries have complete bit partitions.
Continue adding independently sourced vectors and negative tests with changes.
Do not fill unknown reset bits with zeros or
turn a printed EEE address into a wire offset while its gate remains open.
