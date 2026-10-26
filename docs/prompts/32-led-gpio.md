# 32 - Implement all LED and GPIO controls

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [27](27-interrupt-service.md), [31](31-wake-on-lan.md),
[5](5-silicon-power-interface-ledger.md). Verify the concrete inputs below; retain
unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver documented LED/GPIO sources, mux, polarity, drive, blink and stretch
controls with exact applicability, conflict validation and verified restoration.
Extend coherent typed configuration and shared pin ownership rules; no class
per pin or field, generic pin registry or alternate raw-setter aliases.

Recheck the [LED source comparison](../reference/library-ti-features.md#led-interface-and-link-quality-controls)
against visually inspectable SNLS647I tables 8-49/85/89. LED0 uses
`0025[6:3]`, mask `0x0078`; LED1/2/3 use `0460[7:4]/[11:8]/[15:12]`.
Test every LED independently with literal vectors, including LED0 modes 8..E
to exercise the full four-bit selector, reject source 0xF, and retain MLEDCR bit 0=1 (zero is reserved).
LEDCFG2 bits 8/9/10 are reserved, not usable LED3 drive controls.
Blink codes 0-3 at `0018[10:9]` mean 50/100/200/500 ms. Resolve per-pin mux
R-versus-R/W and reserved clock selections for each pin before enabling its programmable path.

Reopen PCA9555 desired/observed and raw shadow-invalidation code/tests,
LDC1614 deferred interrupt/pin source and relevant SCD41/RV3032-C7 public IRQ
controls. Inspect actual declarations, dispatcher/tests and current registration.
Use peer validation/ownership patterns only; SNLS647I defines PHY pin functions,
electrical encodings and reset behavior.

Make a complete legal-settings table for every documented pin/source/mux/drive/
polarity/blink/stretch field. Distinguish reset-loaded/strap behavior, runtime
programmability and observable hardware-only properties. Unknown or reserved
encodings are rejected, not treated as tuning choices. Unconnected initial-board
pins still belong to the generic software API. Include the effect of collector
reads on latched LED indications: BMSR consumption clears link-loss indication,
and BIST error-counter clearing affects PRBS indication. Document and test these
shared effects without adding another LED-owned status reader.

Validate the entire pin profile before I/O, including conflicts with interrupt,
WoL and source-defined clock/interface uses. Preserve each existing user's owned
settings; a later setter cannot silently steal a pin or bypass the IRQ/WoL
collector. For documented runtime transitions, specify admission and ordering.
Source-proved unsafe/unavailable live transitions reject and document the
application's required stopped/reconfigured state.

Apply command-neutral writes once, verify stable owned bits and publish desired,
observed and trusted settings separately. Extend every applicable reset and
power kind's restoration through the common ledger; do not restore status or
resample a POR-only strap by assertion. Preserve unknown effects after accepted
writes fail and invalidate relevant pin evidence after raw/external mutation.

Short settings use bounded direct calls where appropriate; scheduled waits use
the existing operation engine only when required by a real procedure. Keep
aggregate deadlines, complete RMW/MMD ownership and no lock across waits.
Cancellation/end cannot undo a pin mutation; restore failure leaves untrusted
effects visible with independent primary/cleanup status.

Test independent literal masks/encodings for each source and legal extrema,
invalid/reserved values, simultaneous user conflicts and mode restrictions.
Inject failures before/after every write/readback/restore, including ambiguous
pin reassignment. Verify reserved-bit and pending-event preservation, exact
transaction limits, zero-I/O rejection and reset/power replay.

Compile examples for a normal LED profile and a conflict rejection. Document
units, physical pin dependencies and the difference between configured drive/
blink behavior and measured electrical/timing performance. Do not claim actual
visible LEDs, IRQ delivery or external clocks from host register tests.

## Documentation, verification and checkpoint

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
