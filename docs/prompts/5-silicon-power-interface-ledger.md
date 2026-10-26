# 5 - Complete power, wake, interface and remaining source coverage

Read [COMMON](COMMON.md), [FULL_COVERAGE](FULL_COVERAGE.md),
[ENGINEERING](ENGINEERING.md) and [SOURCE_MAP](SOURCE_MAP.md).
Prerequisites: [3](3-silicon-core-ledger.md) and
[4](4-silicon-diagnostic-ledger.md)'s topic records and shared schema.
Restate owned source work, rules, documentation/checks and checkpoint briefly.

## Complete outcome

Close the DP83826I source inventory: every detailed-register entry, field and
documented feature procedure has an explicit researched disposition, including
source-blocked entries. Assign all software work to prompts 11-36 and define
the later exact-family research boundaries. Research may finish with documented
conflicts; affected implementation/support gates remain open until resolved.

## Work and exact evidence

Revisit current TMP1x2/MB85RC variant and capability declarations, corresponding
rejection tests and TUSB32x source planning. Record actual paths/revisions and
PHY adaptations. Use the original SNLS647I tables, TI product/support sources,
manufacturer SDK/GitHub, relevant EVM material and the public research catalog;
pin new evidence and respect per-file licenses/vendor provenance.

- Complete IEEE/deep power-down and active-sleep policy/state distinctions.
  PHYSCR[14:12]=110 enables automatic partner-dependent behavior; BISCR[8]
  observes state. Enabling policy may succeed while linked/awake. Do not
  invent a forced-entry or partner wake deadline.
- Cover EEE capability/advertisement/partner/status/bypass and legacy-MAC LPI
  controls, with reset/power interactions. SNLS647I prints MMD3
  1001/1014/1016 and MMD7 203C/203D; other TI material also mentions lower
  offsets. A printed address/selector association does not prove its wire
  offset or a digit-stripping convention. Keep exact access traces/clarification
  as an explicit gate; do not import DP83825I's SNLA328 recipe by analogy.
- Cover WoL MAC, SecureON, CRC/filtering, pulse/level indication and routing.
  RXFCFG1 bit 11 is W0C: zero acknowledges level indication. Record neutral
  mixed-register writes and DP83826 MAC octet vectors from its own tables;
  SNLA261A's DP83822 example uses different acknowledgement/ordering. A named
  mask flag does not establish unsupported custom pattern matching.
- Audit interrupt output, LED/GPIO mux/polarity/source/drive/blink/stretch,
  pin conflicts and clock/interface/RMII repeater/compatibility controls.
  Separate board wiring and POR-only straps from programmable overrides.
- Audit legal electrical/timing controls, analog/compliance stimuli, resistor
  calibration, quality/link-loss/recovery controls and any remaining next-page,
  polarity/jabber or status fields. Preserve IOCTRL/COMPT reserved-field and
  GPIO/clock table conflicts. A source-backed exceptional recipe does not
  authorize generic reserved-bit writes.

Reconcile the existing 79-entry inventory with summary tables, full field
definitions and addresses used only by recipes. Every remaining row needs
exact applicability, legal values, effects, source, implementation owner and
test/fixture plan; neither runtime opt-in nor absent board wiring removes it.
Keep one factual ledger, source-proved shared records plus explicit differences,
and independent original-source vectors. Avoid duplicated numeric guide tables.

Enumerate later DP83826E, AI/AE, DP83825I and DP83822I/H/IF/HF research using
SNLS647I, SNLS783A/SDAA067A, SNLS638C and SNLS505H respectively. Shared model
IDs/revision defaults do not detect ordering suffix. Unsupported undocumented
Linux suffixes remain reference-only. This task does not redo every family's
implementation audit or claim their support.

## Review and completion

Delegate independent power/wake or pin/electrical PDF review with suitable
reasoning models; the lead owns schema and final coverage reconciliation.
Record chosen scopes/model rationale. Validate literals and neutral write
values independently. Update device behavior, vectors, feature/variant guides,
source-conflict gates and future Doxygen citations; review README/CHANGELOG.
Run reference/link/diff checks and record report/progress with full inventory
coverage and explicit unresolved facts. Commit and sync checked work and
coherent units on the same branch. Remote CI remains prompt 71 only.
