# 03 - Resolve the portable core and ESP-IDF ownership boundary

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-02.

Read [CONVENTIONS](CONVENTIONS.md). Record a short reminder of the portable
boundary, complete-transaction ownership, current sibling audit, documentation,
local checks and single-branch commit/sync. Use a stronger reasoning model for
the ownership decision or independent review when available; delegate inventories
separately and keep shared architecture decisions with the lead.

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

## Concrete ownership review

Trace `TMP1x2::bind`, `startConfigure`, `poll`, `takeResult` and the private
transport wrappers; contrast PCA9555's `pollOperation` and `ObservedState`.
Reopen FieldCore `Ina228Module::{bind,start,poll,takeResult,refreshStatus,
snapshot,copyStatus,mapLibraryStatus}`. Its useful pattern is one active owner
and cached publication. Copy neither its contracts into the library nor its
I2C transaction granularity into an MMD sequence.

For each operation in the responsibility table name the concrete public entry,
portable procedure, IDF callback/base helper, lock holder, time source, side
effects and result collector. Write the call chain for both a plain MCU and the
native IDF path. Include normal link polling while a CLI command or diagnostic
is pending; do not solve races only for the application calls you control.

Select one implementable transport mechanism and explain the rejected alternative:

- A terminal batch callback may keep all frames in an indirect transaction under
  one owner, but must report bounded completion, frame failures and uncertain
  writes without partial outputs pretending to be complete.
- An explicit owner-held transaction scope must cover base/mediator access too,
  release on every exit, have a defined lock order and avoid nested acquisition
  when a helper calls back through the mediator.

Do not ship both mechanisms just for flexibility. Derive the choice from the
pinned SDK's real entry points and a standalone consumer. Explain how another
PHY on the same bus participates. Protect controller-wide state only as needed,
while never releasing an indirect address/data sequence midway.

Review these interleavings on paper before coding: live status vs MMD diagnostic;
reset vs retained link evidence; stop/delete vs an in-flight callback; callback
failure after an accepted write; CLI raw mutation vs base polling; and interrupt
notification vs destructive status read. Identify the admission result and who
invalidates cached state. Testability must follow from real boundaries, not a
second architecture present only in fakes.

State what cannot be bounded by the chosen SDK and how that limitation affects
the adapter claim. A millisecond timeout passed to a callback does not prove
the controller or lock wait honors it. Keep GPIO reset timing and board clocks
in the application adapter; no bus initialization or background recovery in core.

## Documentation in this stage

Update architecture, `docs/design-decisions.md`, the ownership table and API-plan
notes together. Review README/CHANGELOG and the example plan for the chosen
boundary. Define the Doxygen text that future callbacks and adapter lifecycle
methods need: borrowed lifetime, non-reentrancy, complete transaction, output
publication, deadline and delete preconditions. Write clear call chains and
state consequences; do not turn design sketches into implemented support claims.

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

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
