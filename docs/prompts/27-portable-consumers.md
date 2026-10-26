# 27 - Prove independent application and framework consumption

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-26 software outputs; unsupported variants remain
unavailable, not silently simulated.

## Fresh audit and owned work

Inspect TCA9548A standalone consumer, TMP1x2/24Cxx ordinary C++ examples and FieldCore
module adapters. Recheck real platform support before selecting any additional firmware
example.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Build an ordinary C++ owner consumer using only installed/exported public headers and
the actual core. Demonstrate borrowed transport, caller time, complete transactions,
initialize/configure/observe, passive projection, cancellation/recovery admission and
two independent PHY instances. A scripted host backend is explicitly simulation, not
hardware support.

Provide a compact firmware integration guide showing how a FieldCore-like module can
translate Status/health/link snapshots without core dependency on its contracts or task
topology. Explain MDIO owner identity, MAC framework cooperation and shared read-clear
collection. Do not edit FieldCore-node or claim it already has Ethernet support.

Rebuild minimal and CLI native-IDF consumers. Test lifecycle and include/source
selection. Verify consumer code does not reach into private headers or sibling trees.
Confirm examples cover ordinary networking and the 100/full EtherCAT application
boundary separately.

Evaluate Arduino/another MCU adapter only against an actual requested or supported
MAC/MDIO backend. If feasible and in scope, implement/test a thin adapter using the same
core/CLI; otherwise document precise unimplemented platform scope. Framework neutrality
is required; advertising fictional S2/S3 EMAC examples is not. Avoid a generic backend
catalogue created solely for appearance.

## Acceptance and handoff

Compile/link/run the ordinary tests-OFF consumer with a second host compiler when
available. Build real native-IDF images and inspect dependencies. Check zero sibling/SDK
requirements for core consumption. Report host portability, SDK integration and physical
MCU support independently.

Expected output: Independent portable consumer and useful application integration
guidance. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.
