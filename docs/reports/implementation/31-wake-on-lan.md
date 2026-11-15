# Prompt31: complete Wake-on-LAN controls

Implemented ledger procedures `wol_configure` and `wol_acknowledge` for exact
DP83826I. [The WoL guide](../../wake-on-lan.md) specifies admitted settings,
collector ownership, bounded work, retained failure and physical qualification.
The source module extends the existing operation switch and shared restoration.

## Rules, scope and checkpoint

Started on clean fetched `main` at
`519c3aa51479c3c6c23f8c22d9a4c55154980986`, equal to `origin/main`.
Read the complete attachment and repository prompt31, COMMON, current API OP
contracts and engineering/evidence/coverage conventions, parent/root guidance,
architecture/variant/reference guides and relevant prerequisite implementations.
Consumed stage27 IRQ policy/source selection,28 power state/shared replay,
5 exact WoL tables/gates and13 source-specific W0C/single-collector access.

PascalCase types/camelCase methods, existing Status and operation-lifetime rules
remain authoritative. The lead owns shared header/source hooks, integration,
guides/progress and Git. The cohesive `DP8382xWake.cpp` owns current WoL behavior,
not a generic register manager, scheduler or second collector. Applications own
MAC/receive quiescence, pins/clocks, controller and instance serialization,
retry policy and actual host wake. Vendor archives and siblings remain unchanged.
No SDK implementation or fixture activity occurred. No workflow exists yet;
remote CI/automatic triggers remain prompt71, under the first-set exception.

## Current source and sibling audit

The independent source reviewer inspected original SNLS647I pp91-93 tables8-90
through8-97 plus8.3.5 and GPIO tables8-75/76. API octet0 maps to datasheet MAC
byte5; asymmetric literal words were derived independently, not codec round trips.
Both original downloaded official PDFs match the archived bytes:

| Source | Revision | SHA256 |
| --- | --- | --- |
| SNLS647I | Aug2025 | `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3` |
| SNLA261A | Mar2024 | `8e58f74fef0c285f33e42593538b0dde60da44f1d484546a4c3ff6fed6d4acb4` |

Current TI Linux `ti-linux-6.18.y` at
`fe51a7637cfb7c37e90778949da9338daac552f7`,
`drivers/net/phy/dp83822.c`212-270 (GPL-2.0), uses the same wire packing and
three-bit disable, but write-one family acknowledgement and a narrower event
harvest. It does not replace exact-part W0C or establish live quiescence. This is
a factual comparison; no GPL implementation was copied. Bounded exact TI/E2E
search found no inspected stronger W0C/bit0/live-reconfiguration clarification;
that negative search does not prove none exists.

Fresh sibling trees were clean/read-only; paths relative to their roots:

| Reference/revision | Actual reopened evidence |
| --- | --- |
| INA228 `4de84f2564eb6382835ca9a72d8688377657dcb4` | Public DiagAlert/Snapshot111-148; source2478 readDiagAlert,2886 _readDiagAlertTracked/_captureDiagAlert and1624 cached getter; tests2157 overflow retention/4835 reset-filtered history, registrations5671/5736; CLI printDiagSnapshot919 versus live readDiagAlert987; runner and AGENTS. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | Public operation/cleanup contracts; source261 _i2cWriteCleanupTracked/_invalidateShadowPair; test2120 primary timeout plus cleanup bus error/registration2729; CLI main307 signed status/cached settings, parser runner and AGENTS. |
| MB85RC `3ace168eb01d687ab60e9fe88cc72322e5cae260` | Public SleepState; source402 enterSleep/446 wake uncertainty; test5350 wake failure/registration6537; CLI main515 cached sleep uncertainty, `tools/test_hil_runner.py` and AGENTS. |

Adopt staged publication, immediately retained destructive prefixes, separate
primary/restoration evidence and passive CLI views. Reject I2C ACK/tREC/pointer
cleanup and INA conversion waits as PHY sequencing or timing evidence.

## Implementation and independent review

Three inherited capable-model agents worked on disjoint responsibilities:
`wol31_sources` reviewed original PDFs/current sources/siblings and final silicon
semantics read-only; `wol31_review` reviewed integration then owned only the new
Wake source; `wol31_tests` owned only `test/wol.cpp`, with independent literals and
an isolated Release build. The lead reviewed actual source/diffs, implemented
shared ownership/restoration hooks, and performed Git. No agent edited siblings.

The complete request is validated before desired intent changes. All-zero/all-one
48-bit matches are representable. Disabled Secure-ON intent is dormant; explicit
disable clears enable/Secure-ON/magic bits. Unknown mask flag and reserved bits
remain preserved, with no custom-pattern claim. No-op comparison prevents an
unrelated identical settings request from writing RXFCFG1. Explicit acknowledgement
always writes after harvesting RXFS/MISR2 through the shared collector.

Configuration disables/verifies before credential or mux writes, verifies every
credential and changed pin field, then enables last. Stage31 owns only the exact
low-three-bit route function needed now; GPIO conflicts and broader controls stay
with stage32. Shared restoration covers the selected direct/IRQ route and a
separately owned IRQ LED mux after disabling global IRQ delivery. External clock
availability is checked, not invented. Selected route does not claim other
pre-existing pins are disconnected.

Status-only full change is111frames/28callbacks; unchanged settings35/10;
acknowledgement25/7; complete live view33/8. Pin checks/updates and an owned IRQ
tail add finite explicit work under the same aggregate budget. Each batch is at
most five frames. No lock spans polls or output; no retry, automatic ACK, reset,
drain or physical-wake deadline is added. Short slices yield before indivisible
transactions. Cancellation retains its original result; restoration is separate.

Confirmed review issues fixed and regression-tested:

- Preserve verified credential/control trust across a successful ACK, while
  retaining actual contradictions and failed/uncertain effects.
- Reconcile EEE/LED admission in both directions; direct LED1 conflicts with the
  clock recipe, LED2 retains its own admitted route constraints.
- Invalidate WoL trust on RXFS selector, pin, credential or control contradictions,
  including independent reads between polls and valid prefixes before failures.
- Restore muxes after actual reset loss instead of hiding the loss in a fake or
  claiming read-only preconfiguration fulfills routing restoration.
- Keep combined direct-WoL/IRQ routing acyclic and complete; each restore failure
  retains debt and every cancellation boundary is tested.
- Keep old raw pin values historical in the33-frame live pass by clearing their
  current validity; failed refreshes retain old time with explicit staleness.
- Preserve W0C pending evidence under normal composition and an alternate-W1C
  stress model without claiming that model settles exact silicon polarity.

Final maintainability review retained one shared RXFCFG1 composer, one collector,
one lifecycle slot and fixed storage. The new file has a current cohesive
responsibility. No alternate GPIO framework, borrowed-buffer lifetime or opaque
family capability was introduced.

## Documentation, checks and limitations

Updated public Doxygen, README/CHANGELOG, API/architecture/behavior/initialization,
WoL/health/side-effect guides, ledger/coverage, independent vectors, source
comparison, test guide and [owner example](../../../examples/portable/Wake.h).
The tests-OFF consumer compiles/links configuration, acknowledgement, observation
and cached use; the independent host suite executes normal and failed owner flows.
Version/tool commands and variant admission need no change. No other family is
claimed supported. Future Doxygen citations retain exact table/access qualifiers.

Windows GNU15.1/Ninja/C++17 Release, active assertions and strict warnings:

- PASS:187 registered host CTests, including eight WoL suites and the deliberate
  Release failure-path test. Independent route/IRQ/partial/failure tests also pass.
- PASS: separately configured tests-OFF `build/wol31-consumer`, two CTests.
- PASS:103 Python tests; ledger97registers/52procedures/35features; reference/local
  links; Doxygen220 pages and two negative fixtures; generated version and diff checks.
- NOT RUN: SDK/controller and installed-package qualification; portable source
  consumption is the verified layer here.
- NOT RUN: remote CI, reserved for prompt71; physical Q3 W0C/RH-bit0 stability,
  live-reconfiguration races, pulse/line routing, receive-power behavior and actual
  MAC/MCU/application wake. Existing unrelated gates stay open.

Stage31 software and local checks are complete. Final commit/upstream equality
is recorded in the handoff rather than a self-referencing report commit.
