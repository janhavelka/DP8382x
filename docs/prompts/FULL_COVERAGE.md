# Full chip coverage and public-source research

The requested endpoint is a complete library for each supported exact PHY,
starting with DP83826I. The initial P4/RMII/100-full application determines
bring-up order and example defaults, **not the library's feature ceiling**.
Read this contract with COMMON before every stage.

## Meaning of complete

Implement every publicly documented software-accessible capability and legal
operation of the exact supported chip/mode, with usable configuration, readback,
status and diagnostics where provided by silicon. Account for every documented
register and non-reserved field, including fields described outside the summary
register table and manufacturer-documented feature procedures. A PHY uses MDIO
register operations and sequences, not a separate invented opcode command set.

Runtime-optional features remain required implementation scope when the chip
supports them: EEE, wake/WoL, PHY low-power states, interrupt service, LED/GPIO,
clock/interface controls, test modes and diagnostics cannot be dropped merely
because the first example leaves them disabled. Keep opt-in defaults and explicit
owner admission for disruptive behavior. An unconnected IRQ or another interface
on the first PCB limits that board's test coverage, not the generic API.

Use typed operations for multistep or stateful features. Named register/field
definitions and bounded register access complement them; a generic raw-write
escape hatch alone does not implement EEE, WoL, reset, BIST, TDR or a full-feature
driver. Simple independent controls can share typed configuration/field helpers;
there is no requirement for a separate getter/setter class for every bit.

Keep these distinctions in the coverage ledger:

- **Implemented:** public operation, documented legal settings/effects, error
  handling and executing tests exist; SDK and physical evidence remain separate.
- **Hardware-only:** the feature is a strap/electrical/physical property rather
  than programmable behavior. Document prerequisites, observable state and the
  application's responsibility; do not invent an API that can change a POR strap.
- **Not supported by this exact silicon/mode:** cite evidence and reject the
  unavailable operation before I/O. Do not classify supported-but-unimplemented
  features this way.
- **Blocked or not implemented:** required work remains. Record the source
  conflict, missing implementation/evidence and next step. Missing board wiring,
  a limited upstream driver or initial-use-case priority does not close this row.
- **Outside the PHY:** MAC/DMA, network or EtherCAT stack, motion control, board
  power delivery and PoE remain application/platform responsibilities. PHY
  low-power controls are still in scope.

No required software feature may be relabeled an optional future enhancement to
pass final review. If a source conflict prevents implementation, continue useful
independent work but retain the gap and withhold a full-feature completion claim.
Experimental/undocumented silicon controls are not guessed into public support.

## Research broadly, prefer primary sources

Begin with [public-source research](../reference/public-research.md), the
[pinned source review](../reference/source-code.md), the
[reference archive](../reference/README.md) and the
[DP83826I inventory](../reference/dp83826i-feature-inventory.md).

For each feature and each later variant, consult the current available sources:

1. Exact TI datasheet/revision, errata, application notes, EVM/tool guides and
   rollover documents, including feature text and programming examples outside
   register tables. These establish authoritative silicon behavior.
2. TI GitHub repositories: dedicated Ethernet RTOS drivers, MCU+ SDK repositories,
   and the combined Ethernet repository's Linux/Zephyr material. Inspect headers,
   source, callbacks, device bindings, examples, release notes and relevant fixes,
   not just a README or search excerpt. Pin inspected commits/files.
3. Espressif's supported PHY/base/MAC sources and component examples for adapter
   behavior. Their abstraction or unsupported feature is not a silicon limit.
4. Relevant TI E2E engineer clarification and supplied scripts, identified by
   author, exact part, revision/mode, thread date and applicability. Forum advice
   is supporting evidence, not automatically a datasheet amendment.
5. Maintained original projects such as Zephyr and Linux, and other manufacturers'
   exact-device SDK implementations if found. These are independent comparison
   sources, with their own version, license and target limitations.

Search exact part names, register mnemonics/addresses and feature-specific terms.
Follow useful source links, compare implementations, and look for changes/issues
that explain discrepancies. Finish a broad initial sweep and targeted sweeps
for incomplete rows; repeatedly collecting identical links is not progress.
Capture unsuccessful/blocked searches without asserting that no public source
exists. Access-restricted downloads stay recorded as unavailable; do not claim
their contents were reviewed. Do not execute downloaded tools to read docs.

For each used source record owner, URL, exact revision/commit/file or section,
retrieval date, license, relevant facts, actual inspection depth and conflicts.
Preserve byte hashes/provenance for any payload actually archived. Mutable
`latest` SDK pages are navigation aids; record the version they returned and use
immutable evidence for implementation. Verify manufacturer code against the
datasheet: incomplete implementations, incorrect masks and unsafe waits exist.

Review per-file licensing, including mixed-license manufacturer repositories.
Do not copy GPL implementation into MIT sources. Permissively licensed code reuse
retains the applicable notices; factual register evidence is not permission to
relicense someone else's implementation. Reference code stays outside firmware
source lists and release packages. Do not create an unreviewed vendor-code dump.

## Exhaustive register and procedure inventory

Stage 02 creates a machine-readable register/field/feature ledger and readable
guides from authoritative documents, extending the planning inventory. Use stable
semantic IDs, not prompt numbers. Choose a small JSON/CSV format plus a checker
where it makes coverage reliable; avoid a general code generator framework.

For every documented register/field, record:

- exact part/revision/mode and address space: Clause 22 or MMD DEVAD/address;
- canonical name, address, width, mask, reset value and reset/mode dependencies;
- R/RW/W1C/read-clear/latch/self-clearing/reserved/write-only semantics;
- legal values, units/encoding, prerequisites, side effects and ownership;
- documented sequence requirements, waits, ordering, stop/recovery behavior;
- primary source section/table and any alternate-source conflict;
- implemented symbol/codec, typed operation, CLI and independent tests;
- host/SDK/physical status, fixture requirements and remaining gate.

Inventory all tables and explanatory sections. Do not treat every unlisted offset
as an accessible register. Conversely, reconcile official feature recipes that
write offsets absent from the summary table: maintain a separate allowlist of
exact source-backed procedures/values/prerequisites. Do not offer arbitrary raw
writes into those locations or invent field meanings/reset values. Unsupported
holes and reserved bits stay protected.

The source ledger, production metadata and test oracle are distinct: a checker
may prove no inventory row was silently dropped, but cannot establish that an
incorrectly extracted mask is correct. Independently derive representative and
boundary vectors from original PDF tables and verify all field partitions,
access attributes, aliases and procedure-only exceptions.

## API, CLI, tests and closure

Map each programmable feature to an actual typed API/configuration path, readable
diagnostic state, source documentation, CLI commands and meaningful tests. Cover
all legal modes/settings and failure boundaries, not only enabled/disabled
happy paths. CLI may group related fields, but must make every applicable public
feature usable with discoverable help; unavailable board operations report why.
Provide runner suites/fixture instructions for physical properties and destructive
tests. Keep live reads with side effects under the existing single collector.

Required feature families include identification/straps, complete MDIO access,
reset/restart, 10/100 and negotiation/next-page/pause, MDI/MDIX, link/status/error
events and counters, interrupt masks, all supported loopback/BIST/PRBS options,
cable diagnostics, LED/GPIO mux and polarity, clock/interface controls, EEE/LPI,
WoL/frame filtering, all PHY low-power states, and documented repeater/compliance/
analog controls. The inventory determines exact membership and limits; a name
on this list does not assert every family member supports that feature.

At stages 14, 19, 22, 23-26 and 31-34 reconcile the full ledger, not just the
current driver's exported functions. Tests and raw access cannot mask omitted
features. Each family expansion repeats the complete source/register/feature
audit rather than only checking its PHY ID and common networking path.

Final completeness requires no unexplained register/field/procedure omissions
and no required software rows left deferred. Every exception has a valid silicon
or application-boundary reason, or remains an open completion gate. Physical
qualification remains independently honest when fixtures are unavailable.
