# 30 - Execute the available physical qualification matrix

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-29 software/fixture outputs for the tested part; exact
hardware and authorized operations must be available.

## Fresh audit and owned work

Recheck peer HIL evidence classification, runner/firmware identity and cleanup handling.
Re-open the exact board/netlist, part/revision documents and current SDK build identity.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Run stage 21's campaigns on explicitly identified hardware when it is available and
within the task scope. Establish exact PHY part/revision/interface/address,
board/netlist, reset/straps/clock, link peer/cable, firmware SHA/dirty state and SDK.
Verify identity from firmware rather than the host checkout.

Prioritize DP83826IRHBR with P4: boot/reset/restart, 100/full-only autonegotiation
policy, resolved-policy rejection, disconnect/reconnect, peer reset, sustained ordinary
traffic, bounded recovery and diagnostic restoration. Test generic 10 Mbit behavior
using a suitable configuration/partner, not an implicit drive-port fallback. Cover legal
duplex/mode differences with real peer configuration evidence.

Use instrumentation for timing/clock/strap claims and exact fixtures for
BIST/TDR/IRQ/LED/wake/power cases. Ordinary PHY low-power operations do not include
board PoE/power delivery. No incidental servo motion or loading-machine control.
EtherCAT cycle/servo qualification belongs to the later application, not this stage's
PHY pass.

Run only supported variant suites on actual parts and keep every family's physical row
separate. Preserve failures, interrupted runs, missing fixtures and restored/untrusted
state. Fix confirmed library/runner defects with regression tests and rerun affected
cases.

If no hardware is attached, complete readiness documentation and case inventory, record
this stage BLOCKED/NOT RUN and continue independent documentation/review work. Do not
invent a physical result or mark the entire promptset complete.

## Acceptance and handoff

Retain bounded raw/structured artifacts, identity, per-case verdict and cleanup
evidence. Distinguish serial protocol, real packets, electrical measurements and
application timing. Review failures and remaining unqualified variants explicitly; a
skipped-only campaign cannot pass.

Expected output: Actual available hardware evidence or a precise unclosed fixture gate.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.
