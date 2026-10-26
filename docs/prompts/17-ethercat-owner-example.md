# 17 - Demonstrate EtherCAT application ownership boundaries

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-16.

## Fresh audit and owned work

Revisit FieldCore owner scheduling/status/CLI ingress and comparable peer owner-service
examples. Read SNLA344C/494/495 for applicability while preserving master-versus-ESC
distinction.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Create a separate ESP32-P4 owner/integration example and guide for a future
EtherCAT-master application. Configure the requested autonegotiation advertisement of
100/full, verify resolved policy before admitting an application session, and expose
link loss/recovery evidence to the owner. Do not automatically force speed, reset the
PHY or resume servo operation.

Demonstrate the actual supported raw MAC frame boundary only where the pinned SDK
permits it. Frame buffers, scheduling, protocol demultiplexing, EtherCAT master stack,
process data and servo control remain application responsibilities. Avoid competing
esp_netif/raw-frame ownership on the same interface; document the selected arrangement
and buffer lifetime.

Implement a small owner example with bounded link service, maintenance admission and
cached health/status projection. A diagnostic/reset request is rejected while an
application session owns the link. The example may exercise lifecycle and frame handoff
with harmless explicit test traffic; it must not masquerade as a working EtherCAT master
or create a success stub for master start.

Document startup/stop/recovery decisions for a loading machine, required integration
work and the separate timing/servo qualification boundary. Do not transplant C2000 ESC
resets or MII Fast Link Drop promises into the RMII P4 design. Adding a real third-party
master stack is a separately scoped application project, not completion padding for this
library.

## Acceptance and handoff

Build/link the example. Test policy mismatch, link loss, admitted/rejected maintenance,
bounded owner fairness and cleanup/buffer ownership. Report whether raw-frame handling
was actually exercised or only built. No claims of EtherCAT cyclic timing, servo motion
or safety qualification without real application evidence.

Expected output: Buildable owner example and honest EtherCAT-master integration
contract. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.
