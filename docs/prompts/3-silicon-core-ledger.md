# 3 - Establish the primary register ledger and core silicon behavior

Read [COMMON](COMMON.md), [FULL_COVERAGE](FULL_COVERAGE.md),
[ENGINEERING](ENGINEERING.md) and [SOURCE_MAP](SOURCE_MAP.md).
Prerequisites: the source and convention decisions from
[1](1-ecosystem-core-audit.md) and [2](2-ecosystem-tooling-audit.md).
State the current rules, evidence scope, owned paths and checkpoint briefly.

## Complete outcome

Create one authoritative machine-readable register/field/procedure ledger for
DP83826I/SNLS647I, its documented schema, and complete identity, access, reset
and link records. Seed the full feature inventory with explicit owners for
diagnostic research in prompt 4 and remaining controls in prompt 5. This task
closes core research; it does not claim full-ledger or driver completion yet.

## Concrete public-source inputs

Use [TI core data](../reference/library-ti-core.md) and
[EVIDENCE](EVIDENCE.md). Seed the comparison with OUI `0x080028`, models
`0x11/0x13`, ID fields `0xFC00/0x03F0/0x000F`, C22 REGCR/ADDAR `0x0D/0x0E`
and vendor DEVAD `31`; derive and verify each exact-part meaning independently.
Record PHYRCR register `0x1F` separately from its hard-reset mask `0x8000`.
Derive an independent regression vector for correct completion polling and
rejection of out-of-range Clause 22 addresses. Compare actual reset domains
and read effects in the PDF; source-code defect details remain in the catalogue.

## Work and sources

Recheck TMP1x2 `include/TMP1x2/Config.h` model/validators and MB85RC
`include/MB85RC/CommandTable.h` variant/capability structures, implementation
and rejection tests. Record paths/revisions and the exact-model principle
adapted to readable PHY identity. TUSB32x capability planning is a reference,
not evidence that its entire implementation exists.

Start from the [research catalog](../reference/public-research.md),
[inventory](../reference/dp83826i-feature-inventory.md) and original TI PDFs.
Search current manufacturer GitHub/SDK sources, product errata/support, EVM
material and primary upstream TI RTOS/ICSS-EMAC, Linux/Zephyr and Espressif
implementations. Pin commits/document revisions and per-file licenses. Explain
findings and limitations; vendor macros are not a complete specification and
GPL source cannot be pasted into this MIT project. Preserve vendor bytes/hashes.

Use a small explicit ledger such as `docs/registers/dp83826i.json`, not a generic
register-description framework. Define records for:

- exact variant/mode, address space/DEVAD/wire address, width, reset value and
  known mask, access attributes, reserved fields and authoritative source;
- field masks, legal/prohibited encodings, W0C/W1C/W1S/RC semantics, command
  neutral values, read side effects and validity conditions;
- ordered procedures, owned masks, prerequisites/waits, timeout, partial
  effects, cleanup/reset restoration and applicability;
- typed API/CLI/test ownership and separate documented, implemented,
  host-tested, SDK-built and hardware-qualified evidence;
- competing interpretations and selected evidence or explicit unresolved gates.

Add a small offline ledger validator and registered tests for its schema and
coverage invariants: unique semantic IDs, explicit address aliases, field bounds,
overlapping masks, unknown reset bits, source references and procedure ownership.
Use malformed temporary records to prove rejection. This checker validates the
record structure; independent PDF-derived vectors establish numeric correctness.
Document its command and run it as prompts 4-5 extend the ledger.

Fully audit PHY IDs, BASIC/ENHANCED model IDs, revision and declared part
distinctions; C22 versus indirect addressing; reset/restart/POR/strap effects;
10BASE-Te `VOD_CFG3`; advertisement, forced modes, MDIX and pause. ANAR bit 13
is application-prohibited despite its R/W marking; partner fault observation
remains legitimate. Record which reset resamples which strap and which owned
settings require replay. Do not infer I/E suffix from shared identity.

Reconcile summary tables, detailed definitions and feature-only addresses.
Unknown reset bits remain unknown; an all-reserved entry is not writable.
Create `docs/device-behavior.md`, `docs/register-map.md`, independent literal
`docs/register-vectors.md` and a coverage matrix summarizing the ledger without
duplicating its numeric map. Write complete accepted procedures, including
failure uncertainty. Flag PHYSTS forced-mode conflicts for coordinated review.

## Review and completion

Delegate independent original-PDF arithmetic/access review to a stronger
reasoning model if available; inventories can use a lighter model. The lead
owns schema and shared guides. Reviewers must not generate expected values
from the ledger they are checking. Record paths/model rationale and findings.

Verify every core row's source, legal encodings, sequence and independent
vectors; unresolved facts have named gates and next evidence needed. Review
README/CHANGELOG, variants and future Doxygen citation/access requirements;
update report/progress and run reference/link/diff checks. No placeholder C++
is needed for researched facts. Commit and sync checked work and coherent
units on the recorded branch. Remote CI remains prompt 71 only.
