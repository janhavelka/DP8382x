# 46 - Expose every diagnostic and opt-in feature through typed commands

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites: [43](43-cli-foundation.md),
[45](45-cli-link-lifecycle.md), the relevant complete primary feature
procedures and [41](41-owner-admission-example.md).

## Current evidence and ownership

Reopen TMP1x2 shared diagnostic/output jobs, current INA228/TCA9548A command
result tests and MB85RC restoration classification. Inspect their actual
Python counterparts and source registration, not only command lists. Reopen
exact-part TI sections for each exposed control and diagnostic. Record current
revisions, paths, adopted/adapted/rejected conventions and evidence gaps.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Apply [TI feature evidence](../reference/library-ti-features.md) and
[EVIDENCE](EVIDENCE.md). Test LED0 mode 8 through the field including bit 6;
`0x0038` cannot encode that mode. Reject reserved LED3 drive controls. Derive
odd-nibble enable polarity from the exact field table. For WoL, distinguish
status mask `0x0800` from the W0C/W1S acknowledgement word and verify byte
order. Use independent expected words rather than round-tripping production
metadata.

## Implement

Add discoverable grouped typed routes for read-only selftest, loopback, BIST,
manual/automatic cable diagnostics and the complete programmable IRQ,
LED/GPIO, clock/interface, power, wake and EEE/LPI controls. Include repeater,
quality/link-loss and electrical compliance/calibration routes where the
exact-part ledger requires them, with explicit stimulus admission. Reconcile
against the full register/field/procedure denominator, including feature
recipes and legal settings unused by the first board. A raw write command is
not a feature implementation, and runtime opt-in does not mean optional scope.

Every handler validates tokens, cross-field legality and exact-part capability
before transfers. Report absent silicon support, missing software and absent
board/backend/fixture as different facts. Unconnected IRQ/clock pins do not
remove generic command paths. Reuse typed public operations and the single
owner; do not create CLI-only setters, direct uncoordinated MDIO or another
diagnostic state machine.

Disruptive stimulus, power/clock changes, raw writes and resets require
explicit maintenance admission and reject an active application session, busy
operation or retained result before I/O. Clearly distinguish read-only checks
from link-disrupting tests. Define selftest as bounded observations of
existing state; it does not enable BIST, loopback, calibration or other
stimulus implicitly. Report diagnostic validity, operation completion,
uncertain effects and primary versus restoration failure independently;
cleanup success cannot turn a failed command into success. Preserve consumed
event samples and exact mixed command/status semantics through existing
procedures.

Update ledger rows with exact variant/mode, legal values, public method/field,
CLI/help, owner route, record, independent host case and physical requirement.
Required omissions stay open until filled; neither the initial P4
configuration nor the vendor reference driver defines the maximum feature set.

## Acceptance

Run compiled transcript success and boundary cases for every handler family,
including unsupported capability, invalid combinations, timeout, cancellation,
cleanup failure and active-session zero-frame rejection. Use independent
literal expected register values/order for representative multistep features.
Test a stalled console while owner deadlines/cleanup continue. Keep host
execution separate from fixture/electrical evidence.

Document handler inputs/effects, legal settings and destructive-read semantics
in Doxygen and `docs/cli.md`. Update feature guides and exhaustive CLI
coverage.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
