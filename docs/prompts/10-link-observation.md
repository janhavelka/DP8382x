# 10 - Implement link snapshots and change evidence

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-09.

## Fresh audit and owned work

Re-audit passive cached snapshots/freshness in INA228, SHT3x and TMP1x2; inspect
FieldCore's copyStatus without I/O. Do not substitute sensor validity for link
negotiation semantics.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Implement explicit live link observations and passive cached LinkSnapshot. Handle
latch-low/current status using the exact required read sequence. Distinguish link down,
negotiation incomplete, resolved link, unknown/stale data and management failure.
Preserve old values with invalidity/age rather than publish partially read fields as
current.

Determine speed/duplex and partner/advertisement evidence according to mode and
documented source. Include resolved RX/TX pause evidence from the applicable
ANAR/ANLPAR/PHYCR semantics, valid only under documented full-duplex negotiation
conditions; don't infer it in half-duplex, forced or incomplete modes. Resolve or gate
the PHYSTS forced-mode ambiguity; don't choose a convenient bit interpretation. Define
what makes a snapshot coherent enough and expose uncertainty during an unstable
multi-register read rather than invent atomic silicon sampling.

Deliver change evidence for the adapter without duplicate notifications or polling. In
portable mode the application drives observation. In IDF mode stage 15 uses the approved
base/adapter path; there must be one collector and one MAC-notification owner. A cable
reconnection is not permission for core recovery policy.

Keep application policy evaluation separate: a valid link can fail a 100/full
requirement. IP address, DHCP, internet, EtherCAT state and servo availability are not
LinkSnapshot fields owned by the PHY. Document freshness and timestamps without implicit
platform clocks.

## Acceptance and handoff

Test link up/down/flapping, latched history vs current status, negotiation transitions,
forced modes, symmetric/asymmetric/no-pause partner combinations and half-duplex
invalidity, failures between reads, stale snapshots and changed/repeated notifications.
Assert cached views do no I/O and link-down does not increase MDIO failure counters.
Test a valid but application-incompatible link.

Expected output: Truthful passive/live link observations and adapter-ready change
evidence. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.
