# 03 - Resolve the portable core and ESP-IDF ownership boundary

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-02.

## Fresh audit and owned work

Re-audit borrowed transport and cleanup in PCA9555, TCA9548A, TMP1x2 and native IDF
examples; inspect FieldCore's owner-to-library adapters and passive status projection.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Rewrite the maintained architecture around a real portable C++17 chip interface plus
native ESP-IDF adapter. Compare viable boundaries against actual pinned esp_eth_phy_t
and IEEE 802.3 base source. Choose one in `docs/design-decisions.md`; do not implement
competing generic drivers. A future portable option is insufficient for this user's
requirement.

Produce a responsibility table covering construction/destruction, identity, reset,
power, advertisement/autonegotiation, pause advertisement and resolved RX/TX pause
evidence, link sampling, speed/duplex/flow-control notifications, MMD helpers,
interrupts, raw ioctl/CLI and diagnostics. Show which implementation executes in
portable mode and IDF mode, what is shared, and how duplicate
polling/reset/configuration is prevented. The MAC/application owns flow-control enable
policy and frame handling.

Specify application-controlled arbitration for every MDIO participant, complete indirect
and RMW transactions, nested callback lock order, exception/error exits and lifetime.
Reuse IDF MMD helpers only with whole-operation serialization. Inspect implicit base
address scan and reset behavior; override unsafe/wrong semantics deliberately. Identify
SDK-internal waits and whether the proposed aggregate bound is implementable.

Choose transaction callback vs explicit transaction scope, logical-operation/frame
accounting and cooperative boundary. Bound long waits while preserving atomic MMD
access. If IDF offers no safe lock hook for a path, route all entry points through a
proved adapter owner or document and reject the unsupported configuration; do not claim
mutex safety by assertion.

Specify adapters' source/include selection, resource lifecycle and any permitted
framework allocation outside core. Define simple non-IDF owner usage and P4 integration
sketches. Do not import network stacks, product buses, recovery tasks or clocks into
core.

## Acceptance and handoff

Walk two competing callers through an MMD access and reset/link transition; show no
interleaving or deadlock. Trace every selected IDF callback and base helper to an owner.
Confirm a plain C++ consumer can use the design without any SDK. Record constraints
before coding.

Expected output: One implementable ownership design, transport boundary and IDF method
matrix. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.
