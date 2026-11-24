# Implementation progress

The [independent review of prompts 47 and 48](reports/implementation/47-48-independent-review.md)
starts from synchronized `683270b` on `main`. It corrects P4 deadline and UART
loss handling, and hardens the offline verifier and fixture teardown. The
offline fixes were synchronized at `28dc14a`. Physical console and PHY
qualification remain open.

Prompt 48 adds [offline protocol verification](hil-runner.md) on `main`
from synchronized `37059a0`. The compiled host fixture and supplied raw
transcripts share one strict decoder, case plan and durable evidence pair.
The [stage report](reports/implementation/48-runner-protocol-artifacts.md)
records the offline checks. Serial sessions follow in prompt 49.

Prompt 47 attaches the shared CLI to both P4 applications on `main` from
synchronized `12085ea`. The owner task retains all PHY and application state.
UART0 and SDK log V1 share the bounded writer, and the P4 applications expose
their maintenance and frame policies through the same command table. The
[stage report](reports/implementation/47-cli-native-integration.md) records
the host and pinned SDK evidence. Physical UART and board qualification remain open.

The [independent review of prompts 45 and 46](reports/implementation/45-46-independent-review.md)
starts from synchronized `d0d018b` on `main`. It closes CLI deadline and
cancellation gaps, checks passive sample and identity evidence, constrains
native feature dispatch, and exercises the 64-bit BIST interval. Prompt 47
still owns native console attachment. Physical qualification follows separately.

Prompt 46 adds the [typed diagnostic and opt-in control routes](cli.md#typed-diagnostics)
on `main` from synchronized `76d900b`. It connects feature settings, validity,
stimulus approval and separate restoration to the shared owner and record
schema. The [stage report](reports/implementation/46-cli-diagnostics-controls.md)
and [feature matrix](feature-coverage.md) track host evidence and open source
and physical gates. Firmware console attachment follows in prompt 47.

Prompt 45 adds owner-routed [link, events and lifecycle CLI commands](cli.md)
on `main` from synchronized `110b6b0`. It preserves one correlated terminal
result per operation, keeps replay separate from cancellation, and bounds
watch and stress. The [stage report](reports/implementation/45-cli-link-lifecycle.md)
records portable, pinned SDK host and P4 link checks. Firmware console attachment
and physical qualification follow in later stages.

The [independent review of prompts 43 and 44](reports/implementation/43-44-independent-review.md)
starts from synchronized `0f493d4` on `main`. It corrects CLI clock-fault
handling, frame-count and release evidence, and expands the compiled CLI and
native owner regressions. The prompt 47 console and physical gates remain open.

Prompt 44 adds owner-routed [CLI identity, profile and register commands](cli.md)
on `main` from synchronized `d1cdb0c`. It preserves partial scan and dump
evidence, enforces maintenance for disruptive work and reconciles stopped SDK
settings. The [stage report](reports/implementation/44-cli-configuration-registers.md)
and [feature matrix](feature-coverage.md) record host coverage and open physical cases.

Prompt 43 implements the [shared CLI grammar and record schema](cli.md) on
`main` from synchronized `468e269`. The processor has fixed input/output
bounds, passive owner queries, session correlation and terminal backpressure.
The [stage report](reports/implementation/43-cli-foundation.md) records host
fixture evidence. Prompts 44–46 add live handlers and prompt 47 attaches the
native console.

The [third auditor follow-up](reports/implementation/40-p4-network-example.md#third-auditor-follow-up)
closes the expired-entry POR floor, the STOP check after journal retries and
pre-install journal retention. It defers the remaining minor items to prompt 54.

The [stages 37–39 board and controller follow-up](reports/implementation/37-39-board-controller-follow-up.md)
checks the complete reset allowance before GPIO and tests explicit recovery
from shared controller quarantine at all three host tick rates.

The [prompt 40 follow-up](reports/implementation/40-p4-network-example.md#second-auditor-follow-up)
rechecks the historical gate-closed P4 ELF and the application’s reporting
of undelivered results, lifecycle errors and unresolved link profiles.

The [independent audit of prompts 40–42](reports/implementation/40-42-independent-review.md)
corrected P4 startup, stop and event handling, verified the attached network
address, and closed a raw-session readmission gap after SDK cached link-down.

Prompt 42 extends the [P4 owner](../examples/esp_idf/owner/README.md) with a
fixed-capacity raw frame boundary. Host tests execute the SDK handoff and
exercise RX delayed across release, readmission and retirement. The pinned
P4 image links with board access and test traffic disabled. See the
[stage report](reports/implementation/42-owner-frame-boundary.md).

Prompt 41 adds the [ESP32-P4 application owner](../examples/esp_idf/owner/README.md)
on `main`. Admission checks current link, MAC agreement, health and configuration
under one gate. Explicit release and frame quiescence precede maintenance.
The [stage report](reports/implementation/41-owner-admission-example.md) records
host recovery tests, the P4 link and the remaining board and timing evidence.

Prompt 40 software stage is complete: it adds the native ESP32-P4 ordinary Ethernet application on `main` from
clean synchronized `022cccb7ffcffc211a81398764eb64380346dccb`. It uses the
real pinned MAC/PHY, explicit current-netlist pins, DHCP, a default-closed
board gate, separate cached health/link/trust/IP output and finite stop/start.
Verification includes 35 application host cases within 318/318 pinned native
Release and UBSan CTests, 236/236 portable CTests, two portable consumer cases
and the actual pinned P4 image link.
The [stage report](reports/implementation/40-p4-network-example.md) records
tests, linked image and ownership. The latest saved board netlist does not
demonstrate a supply source for PHY `3V3`, so flashing, physical link, DHCP and
peer reachability remain NOT RUN. Reusable installed teardown remains BLOCKED
by `native_rx_quiescence`; installed resources are retained until restart.

The [80-prompt implementation plan](prompts/README.md) is in its first
execution. Prompts 1–35 are complete for supported procedures (DPD remains source-blocked): source research, shared contracts, portable
values/version tooling, direct/indirect and side-effect access, exact identity and complete software reset/initialization.
Current local checks include 236 registered portable Release CTests, 103 Python tests,
two linked tests-OFF consumers, Doxygen negative checks, ledger/reference integrity
and deterministic version validation. Prompts 37-39 and their audits add 283 pinned-driver/native host
tests in Release and UBSan and an actual ESP32-P4 image link. Physical qualification
remains later work; first-execution remote CI belongs to prompt 71.
The driver now provides the restricted Clause 22/MMD subset, stable-field RMW
and compiling portable examples. Prompt 13 adds explicit destructive acquisition,
retained partial evidence and safe MISR/WoL updates. Further SDK controls, CLI, runner and hardware
qualification remain future work. Prompt 5 closes primary-part source dispositions, including
explicit unresolved gates. Research completion does not claim driver completion.

Prompt 15 adds [caller-driven initialization](initialization.md), cancellation,
retained results and separately admitted restoration. Prompt 17 adds negotiated
profile replay; prompt 18 adds forced modes and coupled routing/extension.
Prompt 19 adds [live/cached link observations](link-observation.md), qualified
mode/pause and retained change evidence. Prompt 20 adds [typed event/counter acquisition](event-counters.md)
and explicit retained-history semantics. Prompt 21 adds [passive health](health.md),
exact managed batch accounting and cached state without an offline admission latch.
Prompt 22 adds [explicit reconciliation](reconciliation.md): bounded verification,
retained mismatch evidence and recovery through the existing initialization path.
Prompt 23 adds [loopback sessions](loopback.md), with separate start/stop
operations, retained temporary state and verified restoration.

## Execution record

The [third-round native correction review](reports/implementation/37-39-native-fixes-round3-2026-11-18.md)
starts from synchronized `5bee640` on main. It corrects same-poll trust loss,
failed shared readiness, no-effect board admission, unexplained P4 busy state and
timeout/release receipts. All seven comments have individual dispositions,
including cleanup-attempt and forbidden-uninstall limitations. Prompt 40 remains
unstarted; installed resources remain retained under native_rx_quiescence.

The [second-round native correction review](reports/implementation/37-39-native-fixes-round2-2026-11-18.md)
starts from synchronized `998cb6d` on main. It corrects transient observation
recovery, peer quiet-floor readiness, stop admission and continued cleanup after
event failure or forbidden uninstall, with precise deadline/error evidence.
All 12 comments have individual dispositions; prose spacing is a separate unit.
Prompt 40 remains unstarted and native_rx_quiescence remains BLOCKED. Final
verification totals and synchronized checkpoints are recorded in the handoff.

The [2026-11-18 native correction review](reports/implementation/37-39-native-fixes-2026-11-18.md)
rechecks all 17 findings at 1a8bc8e and fixes lifecycle retry/completion, error domains,
running-state withdrawal, pause/tick/budget validation and shared installation
admission. Regression coverage includes 68 native suites at both 1000/100Hz and a
333Hz timing case. Prompt 40 remains unstarted; RX lifetime and physical timing/
stack qualification remain separate gates. Final checked commit is in the handoff.

The [17-item follow-up](reports/audit-followup-2026-11-17.md) starts from 955ecf3
and changes no stage status. Checked/synchronized units are 3fe72be (BISCR/private
helper),681a10e (contracts/source guides) and 380cd83 (prompt acceptance).
Larger result/admission/harness/timing work stays with 76/53/55/54; existing
source, physical and native RX lifetime gates remain open.

The [2026-11-17 audit-response review](reports/current-review-responses.md) starts
from synchronized `main` at `77c544f415edb26a0e23de76591a0163b9b73147`.
All 30 audit records now contain individual current-code responses, and all 197
G/M/N/R dispositions were checked against current source, prompts and primary
evidence. Corrections align SDK reference guidance, IRQ mux ownership,
initialization replay bounds and future CLI discovery with existing behavior.
No new production defect was confirmed. This maintenance advances no numbered
stage and preserves unresolved silicon, physical and installed-RX lifetime gates.

The [independent prompts 37-39 audit](reports/implementation/37-39-independent-audit.md)
began on clean synchronized `main` at `ec792d2ea5bee28d20092999c1ec6facad57e088`.
It corrects shared board/initialization reservations, routes discovery through
core transport accounting, distinguishes controller wait expiry from contention,
and enforces individual SDK control/policy callback bounds with retained effects.
Malformed timeout details and the modeled timer-active interval are also corrected.
All 236 portable, 63 native Release/UBSan, two consumer and 103 Python tests pass;
actual P4 linking and source/document checks pass. Physical timing, source gates
and reusable installed RX teardown remain separately unqualified. Final commit
and upstream equality are recorded in the handoff.

Prompt 39 began on clean synchronized `main` at
`456b9710095b4e1394cfb8d4acdeb90b722d981f`. The
[report](reports/implementation/39-esp-idf-link-bounds.md) and
[fresh audit](reports/implementation/39-native-source-audit.md) close the native
entry, notification and conditional timing tables. Independent review corrected
publication/start/stop/fence completion contention, late event classification,
poll-period overflow and floor-clock readiness anchors. All 55 native Release and
UBSan cases,235 portable tests, two consumers,103 Python tests, source checks and
actual P4 link pass. Physical wall-time qualification and reusable RX teardown
remain explicit gates. Final commit/upstream equality is recorded in the handoff.

Prompt 38 began on clean synchronized `main` at
`a744e77e3ac000be5ea12b1c4b5fdef55b50efde`. The
[report](reports/implementation/38-esp-idf-state-arbitration.md) records complete
AN/forced SDK state reconciliation, native raw/collector routes, shared reset
silence and correctly typed MAC/PHY flow policy with retained partial effects.
Independent review corrected callback contention, stale forced pause, foreign
flow interference and direct-start/stop event evidence. Local host, pinned-driver,
consumer, source-document checks and actual P4 link pass. The installed RX lifetime
gate, physical qualification and source-gated chip branches remain unchanged;
final commit/upstream equality belongs to the handoff.

Prompt 37 began on clean synchronized `main` at
`cb5ffe5b5d21e700c3e9fbef434e192a50b1aeff`. The
[report](reports/implementation/37-esp-idf-lifecycle.md) records the native factory,
board readiness, shared terminal transport, implemented owner/callback routes,
partial lifecycle evidence and actual pinned SDK link. Independent ownership
review and modeled raw/glue RX capture tests preserve the `native_rx_quiescence`
gate: reusable installed teardown remains BLOCKED, with terminal resources
retained until system restart. Prompts 38/39 expand configuration/conformance;
source-gated chip branches and physical Q13 remain open. Final commit/upstream
equality is recorded in the handoff.

The [independent prompt 36 audit](reports/implementation/36-independent-audit.md)
starts from clean synchronized `main` at `67e5e97cee016f57177d0496f1c47a51bd77821b`.
It corrects quarantined link freshness and reset handling inside configuration
and collision batches. Required source gates and BLOCKED primary closure remain unchanged;
the audit report records fresh focused checks separately from historical totals.

Prompt 36 began on clean synchronized `main` at
`2f64d3018179e86a11e9e747eecb1d13e75989aa`. Its bounded review and focused fixes
are delivered; full primary closure remains BLOCKED by mandatory source-gated
branches. The [report](reports/implementation/36-primary-feature-closure.md) records
independent source/integration review, returned acceptance and fresh local checks.
Native work may continue on the implemented subset; no SDK or physical completion
is claimed. Final commit/upstream equality is recorded in the handoff.

The [independent prompts 34-35 audit](reports/implementation/34-35-independent-audit.md)
starts from synchronized `main` at `c292dc75658b01d224617548d896f88fb8c6ef71`.
It corrects quality evidence handling, combined-owner analog replay, electrical
reset/session reconciliation and interleaved reset invalidation. Existing source
and physical gates remain open; its report records fresh local verification and
the final handoff records commit/upstream equality.

Prompt 35 began on clean synchronized `main` at
`a4ee2d4f18b12dc577ed52e75387ab936b8bfc3d`. It implements source-admitted
analog profiles, compliance/collision sessions and conditional calibration
recovery through the shared operation/reset/collector paths. The
[report](reports/implementation/35-electrical-compliance-calibration.md) records
independent PDF/code review, failure-prefix corrections and local verification.
SDK, remote CI and physical qualification remain separate; final commit and
upstream equality are recorded in the handoff.

Prompt 34 began on clean synchronized `main` at
`e3c25db71c8520d83b997a04c5d107d64b69c707`. It completes the five source-admitted
quality/TX/link-loss procedures through one cooperative profile, the existing
collector and shared restore path, plus raw MSE snapshots. The
[report](reports/implementation/34-quality-link-loss.md) records independent
source/code review, tests and physical/source limits. SDK, remote CI and
hardware results remain separate; the final commit and upstream equality are
recorded in the handoff.

The [independent prompts 32-33 audit](reports/implementation/32-33-independent-audit.md)
starts from synchronized `main` at `2938ed718725b834fc1da105d93a3370d77f2e28`.
It corrects premature restoration readiness, pin-owned clock dependency proof
and live interface admission for shared LED2/3 routes. Existing source and
physical gates remain open; final check and synchronization evidence is in
the audit report and handoff.

Prompt 33 began on clean fetched `main` at
`3e6e0bfb86d6ed681ba3dde191ff9973078ba31e`, synchronized with upstream.
[The report](reports/implementation/33-clock-interface-repeater.md) records the
complete source-admitted interface/clock/compatibility and isolation profile,
per-PHY repeater configuration, collector-safe RCSR access and shared replay.
Physical Q12/Q17 and board clock/repeater qualification remain NOT RUN.
Final verified commit and sync equality are recorded in the handoff.

Prompt 32 began on clean fetched `main` at
`328b5edbfe3b33c45d680aa34327bd96ef79b4a0`. The
[stage report](reports/implementation/32-led-gpio.md) records bounded typed pin
profiles, exact source/mode restrictions, dependency ownership and shared replay.
Local verification is recorded below; Q12, SDK and remote CI remain NOT RUN.
Final commit and synchronized upstream equality are recorded in the handoff.

Fresh prompts 30-31 audit began on clean synchronized `main` at
`12a17284831dc1ae7a6b2e3584f551eed73f5f25`. The
[audit report](reports/implementation/30-31-independent-audit.md) records three
confirmed WoL corrections, independent LPI review and final local verification.
Physical Q2/Q3 qualification remains NOT RUN.

Prompt 31 began on clean synchronized `main` at
`519c3aa51479c3c6c23f8c22d9a4c55154980986` (completed prompt 30).
[The report](reports/implementation/31-wake-on-lan.md) records complete WoL
matching/indication, bounded status and acknowledgement, pending-event preservation,
narrow route ownership and shared restoration. All local checks pass; physical
Q3 and SDK qualification remain NOT RUN. Remote CI stays prompt 71. Final commit
and upstream equality are recorded in the handoff.


Prompt 30 began on clean synchronized `main` at
`356320ac645f7fad405ff35b8ca3e7c853423ea4` (completed prompt 29).
The [stage 30 report](reports/implementation/30-legacy-mac-lpi.md) records bounded
forced-TX-LPI start/stop, slot-free dwell with OP-9 protection, cached prerequisites,
retained uncertain effects and separate stop/HARD restoration. Local Release,
portable consumer and documentation checks pass. Q1/Q2 physical evidence and SDK
qualification remain NOT RUN; remote CI stays prompt 71. Final commit and verified
upstream equality are recorded in the handoff.


Prompt 29 began on clean, fetched and synchronized `main` at
`ce3df832c334c68e23a824333c9e5c86908b5ab8`. The
[stage report](reports/implementation/29-eee-negotiation.md) records complete
explicit EEE recipe/disable, fail-closed mapping checks, one effective HARD,
shared replay and bounded collector-based observations. All local checks pass;
Q1 wire/control/interoperability qualification remains NOT RUN. EEE ownership
explicitly upgrades initialization reset requests to HARD. Source/SDK/physical
boundaries stay separate; final commit and upstream equality are in the handoff.

The [independent prompts 27-28 audit](reports/implementation/27-28-independent-audit.md)
began on clean, fetched and synchronized `main` at
`cd0420e2846a218b0b2a934f7f2074d33a4568a5`. It fixes inter-poll identity/reset/
selector invalidation, retained control contradictions, complete owned-policy
proof and OP-2 admission order. Prompts 1-28 retain their existing scope; DPD,
Q6/Q9, SDK and physical gates remain separate. Final verified audit commit and
upstream equality are recorded in the handoff.

Prompt 28 began on clean, fetched and synchronized `main` at
`ced8b1ce5418aec044eb156b507ea78573302576`. The
[stage report](reports/implementation/28-power-states.md) records active-sleep,
IEEE software intent, separate PLL control, shared profile/reset integration and
live/cached power evidence. Independent source, sibling, test and lifecycle
reviews are integrated. `deep_power_exit` remains an essential source gate;
DPD rejects before I/O. Q6 and SDK/physical qualification remain NOT RUN.
Final commit and upstream equality are recorded in the handoff.


Prompt 27 began on clean, fetched and synchronized `main` at
`87aafc01c959d5f811fb7342a037e26c105acac4`. The
[stage report](reports/implementation/27-interrupt-service.md) records complete
[interrupt policy and deferred service](interrupts.md), separate test start/stop,
single-collector attribution/pending counter requests and verified shared replay.
Independent source, sibling/test and lifecycle reviews are integrated; local
checks pass. Physical Q9 remains NOT RUN. Final SHA and upstream equality are
recorded in the handoff.

The [independent prompts 25-26 audit](reports/implementation/25-26-independent-audit.md)
began on clean, fetched and synchronized `main` at
`e1e14a84f80fd2826c245c231bfda8cbabfc6a8c`. Original-PDF, lifecycle and
test reviews found no production defect. The audit adds multi-run interruption
and partial-reacquisition regressions and repeats local verification. Existing
source/physical gates remain open; this does not start prompt 27. Final audit
commit and upstream equality are recorded in the handoff.

Prompt 26 began on clean, fetched and synchronized `main` at
`1aedc22238027120da7eb9ea389fb11074a6d27f`. The
[stage report](reports/implementation/26-cable-autorun.md) records explicit
autorun policy, bounded shared raw acquisition, manual handoff and persistent
replay/verification. Independent source/lifecycle/tests and local checks pass;
physical trigger timing and Q7 accuracy remain NOT RUN. Final SHA/upstream
equality are recorded in the handoff.

Prompt 25 began on clean, fetched and synchronized `main` at
`87b08d2ccb407d4fe33b1dc03920600855570c7a`. The
[stage report](reports/implementation/25-cable-manual.md) records independent
source/peer/test review, complete [manual cable capture](cable.md), raw controls,
partial evidence and separately admitted HARD restoration. Autorun remains
prompt 26; calibration recovery remains prompt 35. Physical Q7 is NOT RUN.
Final commit and upstream equality are recorded in the handoff.

The [independent prompts 23-24 audit](reports/implementation/23-24-independent-audit.md)
began on clean synchronized `main` at `63b4121c9b38fe5421c50eea958a441b81e58ff8`.
It corrects line-transmit ordering, BIST stop identity admission, dwell evidence
retention and complete HARD replay after a partial stop. Prompts 1-24 remain
complete; source/physical gates and prompt 25 remain unchanged. The final audit
checkpoint and upstream equality are recorded in the handoff.

Prompt 24 began on clean, synchronized `main` at
`d58a7da95b0f3e96c0a908b70eb2ecaa439b1377`. The
[stage report](reports/implementation/24-bist-generator-checker.md) records the
source/peer audit, independent reviews, all BIST controls and bounded raw
acquisition/stop. PASS/exact intervals remain gated; BIST physical qualification NOT RUN.
Final commit/upstream equality are recorded in the handoff.

Prompt 23 began on clean, synchronized `main` at
`9dee869f7e43beeea9f3c1837bf111029e29a74b`. The
[stage report](reports/implementation/23-loopback-modes.md) records fresh TI/sibling
reviews, all loopback paths and bounded separate stop/restoration, independent
literal/failure tests and local verification. Session and physical evidence are
separate; Q8 remains NOT RUN. Final SHA/upstream equality are in the handoff.

Fresh prompts 21-22 audit began on clean, synchronized `main` at
`04985e91f892160d1767405647a211667dec7438`. The
[independent report](reports/implementation/21-22-independent-audit.md) records
one reset-evidence correction and a stale initialization-guide correction.
Release and UBSan each pass 109 CTests; 103 Python tests, portable consumer,
Doxygen, ledger/reference/version checks also pass. Stage completion remains
1-22; final commit and upstream equality are recorded in the handoff.

Prompt 22 completed on `main`, starting from synchronized prompt-21 checkpoint
`fe539237f3a2e71078491c81c9ac2678ffad2f2a`. The
[stage report](reports/implementation/22-reconcile-recover.md) records fresh sibling/TI
sources, independent review, verification and interrupted-restoration tests:
108 Release CTests, 103 Python tests, portable consumer, Doxygen, ledger,
reference and version checks pass. SDK/remote CI/physical execution remain later
work. Final commit/upstream equality is recorded in the handoff.

Prompt 21 completed on `main`, starting from synchronized audit checkpoint
`0a2d2c45f5ee43b5458949acaf7f21ebb99bb67c`. The
[stage report](reports/implementation/21-passive-health.md) records fresh sibling/TI
sources, independent review, explicit traffic classification and local results:
97 Release CTests, 103 Python tests, portable consumer, Doxygen, ledger,
reference and version checks pass. SDK/remote CI/physical execution remain later
work. Final commit/upstream equality is recorded in the handoff.

2026-11-15 promptset-audit maintenance began on clean, synchronized `main` at
`f576dee3b067e16f51e3a8e51d9ffe17b35924ab`. The
[finding-by-finding report](reports/promptset-audit-dispositions.md) records
independent adjudication, the BMCR power-pin correction, future prompt changes
and current verification. Stage completion remains 1-20; larger native/power/CLI
work stays with its named owner, and physical evidence remains NOT RUN. Final
verified commit/upstream equality is recorded in the handoff.

Prompt 20 entry / synchronized prompt 19 checkpoint:
`0d176b555c7fb35c1836041d9b7499ac5e9d5304`. The same execution branch/base is
retained. [Stage 20 report](reports/implementation/20-event-counter-acquisition.md)
records source/peer audits, collector completion and local verification. Final
commit/upstream equality is recorded in the handoff rather than self-reference.

Prompt 19 entry / completed prompts 13-18 audit checkpoint:
`8ee2efd77c27eb03dce069e1a0c98bd7fcb1440c`, fetched and equal to upstream.
The existing execution branch/base is retained. [Prompt 19 report](reports/implementation/19-link-observation.md)
records the bounded observation, independent reviews, tests and explicit physical
limits. Final commit and verified upstream equality are recorded in the handoff.

Fresh prompts 13-18 audit entry: `507dff3424668f7404760dca905070b4a4ad1e6f`,
clean and equal to upstream. The [independent audit](reports/implementation/13-18-independent-audit.md)
fixes interrupted-verification status, reset-result flags, missing-receipt
uncertainty and completed page-load retention. Release and UBSan trap builds
each pass all 68 CTests. The same execution branch/base is retained; final
commit and verified upstream equality are recorded in the handoff.

Prompt 17 final synchronized checkpoint / prompt 18 entry:
`57a06cb12140b8f2666e1b1a46c64ffe80f8035c`.
Prompt 18 implements complete forced/coupled profiles and shared reset replay;
see [report](reports/implementation/18-forced-crossover-settings.md). Final SHA
and upstream equality are recorded in the handoff, avoiding self-reference.

Prompt 16 final synchronized checkpoint / prompt 17 entry:
`1e0cec9067c498ac5bc7f2c03607395af73e8c2a`.
Prompt 17 implements verified negotiated profiles and finite typed next-page
exchange; see [report](reports/implementation/17-negotiated-link-settings.md).
The final commit/upstream equality is in the handoff, avoiding self-reference.

Prompt 15 final synchronized checkpoint / prompt 16 entry:
`5f05f97a24b929ad5ded85eb08112a4514208bdf`.
Prompt 16 implements HARD/RESTART and external-reset/POR reconciliation with
persistent silence across failures and teardown. See its report; final synced
SHA is in the handoff, avoiding a self-referencing commit.

Prompt 14 final synchronized checkpoint / prompt 15 entry:
`f7165130b8f584aaf43fee997e4887aa4c3bc064`.
Prompt 15 report records source and independent review, local checks and retained
scope gates. Its final commit/sync SHA is in the handoff to avoid a self-reference.

Execution branch: `main`, from 2026-11-11. The owner asked that no new branch be
created until version 0 is finished. Prompts 1-20 ran on
`promptset/dp8382x-implementation-20261028`, which was fast-forwarded into `main`
at `7f80fd807942b13ca979b26fd61e2edf538416a4` on 2026-11-11 after the local checks
passed there.
Original branch base: `main` / `origin/main` at `5e148fffde074b85fadc15d8ff66fe2c77111036`
(fetched, clean and equal before branch creation on 2026-10-28).
Prompt 1 verified commit/sync checkpoint: `77e839810fe8338223a2c1fef89f0af5f48a6f3e`.
Prompt 2 comparison checkpoint: `05d69f585e8d8984b90645a8ec931802b071ecb1`
(verified equal to its same-name upstream before the final report commit).
Prompt 3 entry checkpoint: `e89c024aa4a149bcf56e7ba5dd7bcf23d4966d40`
(prompt-2 final maintenance, fetched and equal to its same-name upstream).
Prompt 3 initial commit/sync checkpoint:
`01ce198a2ea2bf1284793b3876b90ea8ca5a1d86` (also the fresh independent audit's
clean, synchronized entry). The [follow-up audit](reports/implementation/3-core-ledger-audit.md)
corrected silicon facts and validator gaps; its final commit/sync SHA is recorded
in the handoff to avoid a self-referencing commit. No branch or base change occurred.
Prompt 3 audited checkpoint / prompt 4 clean synchronized entry:
`42a908b28d9ad471a60b5ca220127eb9e7dbf9ec`.
Prompt 4 final commit/sync checkpoint / prompt 5 clean synchronized entry:
`2805cfdea3aacd1f870d27998cd99d0270cccb2a`.
Prompt 5 final commit/sync checkpoint / fresh prompts 4–5 audit entry:
`604096c4355765d45a6686a16fabd479be2d2e5e`.
The [fresh audit](reports/implementation/4-5-silicon-ledger-audit.md) corrects
split-field transition/cleanup ordering and closes independent-oracle/schema
gaps; 77 tests and local checks pass. Its final commit/sync identity is recorded
in the handoff, avoiding a self-referencing commit. Branch and base are unchanged.

Prompts 4–5 audit checkpoint / prompt 6 clean synchronized entry:
`e42343b0baf26367fcb22d70d51f4a47744a81b8`.
Prompt 6 completes the [portable/native ownership design](design-decisions.md)
and [source/review report](reports/implementation/6-framework-ownership.md).
It selects one shared core and terminal batch mechanism; native support requires
the specified SDK lifecycle correction at prompt 37. Local research checks and
the 272-vector poison probe pass; production/SDK/physical evidence remains
separate and NOT RUN. Final commit/sync SHA is recorded in the handoff.

Prompt 6 final commit/sync checkpoint / prompt 7 clean synchronized entry:
`49aadf784aa17b996fc07ee967183596ea7bead7`.
Prompt 7 freezes [values and direct-call contracts](api-contracts.md), with
[source/review evidence](reports/implementation/7-value-transport-contracts.md).
All 35 feature families and 52 procedures have typed responsibility mappings;
cooperative lifetime/cleanup signatures remain prompt 8. The 77 research tests,
reference/ledger/link checks and strict C++17 documentation syntax check pass.
No production declarations, driver behavior or SDK support are claimed. Final
commit/sync SHA is recorded in the handoff.

Prompt 7 final checkpoint / fresh audit synchronized entry:
`61e2057590951c740a713786b471228f43d6fbe1`.
The [fresh contract audit](reports/implementation/7-contract-audit.md) corrects
BMSR examples and health/time/release classification, preserving the selected
architecture and prompt 8's cooperative scope. Corrective checks and final
commit/sync identity are recorded in its report and handoff.

Prompt 7 audit checkpoint / prompt 8 clean synchronized entry:
`382462cdec356ae901c0a501f96435830518430f`.
Prompt 8 completes the [cooperative API contract](api-contracts.md#one-cooperative-operation-and-retained-result)
and [source/review report](reports/implementation/8-operation-contracts.md).
One slot, lifetime tokens, finite primary/cleanup allowances and explicit owner
disposition resolve the clock-fault cleanup dependency. All 85 research/design
tests, eight portable documentation blocks and local checks pass; driver/SDK/
physical evidence remains separate and NOT RUN. Final sync SHA is in the handoff.

Prompt 8 final checkpoint / fresh audit clean synchronized entry:
`2b570bb5dcf52b2eacbf97055babb8d17f944fbd`.
The [fresh contract audit](reports/implementation/8-contract-audit.md) resolves
terminal invalidation ambiguity, binds noncopyable/nonmovable example owners to
their exact PHY and fixes native terminal journaling. All 85 research/design
tests and strict C++17 owner checks pass. The report and handoff record final
local verification and the corrective commit/sync identity.

Prompt 8 audit final commit/sync checkpoint / prompt 9 clean synchronized entry:
`7311a81962f45d5c3a0baf7a709b89603875acfe`.
Prompt 9 ships passive C++17 values, pure validators and source-proved constants
with four registered host suites, standalone header compilation, a separate
tests-OFF consumer and a Doxygen check with negative fixtures. The
[source/review report](reports/implementation/9-portable-source-foundation.md)
records the exact local results. The branch/base above remain unchanged;
verified code/test/tool checkpoint: `079bf0f2f6451069de5296aee1b4e2dc811cea72`;
the final prompt-9 commit/sync SHA belongs in the handoff to avoid a
self-referencing commit.

Prompt 9 final commit/sync checkpoint / prompt 10 clean synchronized entry:
`8ca1d6032c591596c358266e083eeedc8505a87f`.
Prompt 10 adds `library.json` as release-version truth, a generated public
`Version.h`, strict sync/check and temporary-fixture tests. The
[source/review report](reports/implementation/10-version-tooling.md) records
exact peer revisions, preservation cases and local evidence. The execution
branch/base above remain unchanged. Verified code/test/tool checkpoint:
`37c787203c7aa7de86e8871b200846d0f0b4ed5c`. The final prompt-10
commit/sync SHA is in the handoff to avoid a self-referencing commit.

The [fresh prompts 9–10 audit](reports/implementation/9-10-foundation-audit.md)
verified the shipped code and tests independently, corrected current header
counts and CTest zero-test/version guidance, and replaced a stale planning-only
verification link. Implementation status and version remain unchanged. Its
final corrective commit/sync SHA is in the handoff.

Prompts 9–10 audit final commit/sync checkpoint / prompt 11 synchronized entry:
`19bf456d472752865ec28bbc01f361d0cf25b229`.
Prompt 11 implements [bounded direct access](direct-access.md), with
[source/review evidence](reports/implementation/11-clause22-transport.md).
Nine Release CTest suites, five standalone header compilations, the linked
tests-OFF portable read consumer, Doxygen negative checks, 98 Python tests and
ledger/reference checks pass. Access and raw LEDCR coverage are PARTIAL at the
feature level; the complete stage-11 subset is implemented. SDK and hardware
remain NOT RUN. Same execution branch/base; final commit/sync SHA is in the handoff.

Prompt 11 final commit/sync checkpoint / prompt 12 synchronized entry:
`c678f59b3e74bac359acee1d247b43a1b84c2dea`.
Prompt 12 implements atomic MMD access and stable LEDCR RMW through the same batch
boundary and canonical access policy. Independent original-PDF review replaces
the earlier eight-frame RMW policy with the minimum five-frame nonincrementing
sequence; every new call reselects completely. See the
[stage report](reports/implementation/12-atomic-mmd.md). The complete access
feature remains PARTIAL pending task 13 and EEE gates; SDK/physical evidence is
NOT RUN. Final verified commit/upstream equality is in the handoff.

Prompt 13 entry was synchronized `81ab2d3`; prerequisite source checkpoint
`88af952` was committed, pushed and verified equal to upstream. The
[stage report](reports/implementation/13-side-effect-register-access.md) records
COMMON's prerequisite WP0–WP7 corrections and the completed access implementation.
Latest explicit user instructions retain the collector at 13, superseding the
audit's proposed deferral. Local Release, portable consumer, Doxygen, ledger,
Python and reference checks pass; SDK/package/physical evidence is NOT RUN.
Final commit/upstream equality is recorded in the handoff.

Prompt 14 began after the requested 45-minute delay on 2026-11-07, at 22:01 UTC.
Its clean synchronized entry was `4bc8e043890d9aaf601879a459b7db64cd7f6320`.
The [stage report](reports/implementation/14-identity-capabilities.md) records
source/peer review, accepted identity policy, failure/invalidation behavior and
actual local verification. Final commit/upstream equality is in the handoff.

Until version 0 is finished, every prompt and resumed session works on `main`;
create no new branch (root AGENTS). Record stage outcomes, actual checks,
remaining dependencies and completed checkpoints under [COMMON](prompts/COMMON.md). Planning maintenance
leaves implementation status unchanged.

Commit and sync each completed prompt and coherent verified intermediate unit.
For the first execution, prompt 71 owns remote CI; all stages retain focused
local checks. Record the actual CI-tested SHA separately from later results.

| Prompt | Implementation status | Evidence |
| --- | --- | --- |
| [1 Audit current driver APIs, state and ownership](prompts/1-ecosystem-core-audit.md) | COMPLETE | [Live comparison](library-comparison.md), [execution report](reports/implementation/1-core-api-state-ownership.md); local reference/link and diff checks PASS. Commit/sync SHA in final handoff. |
| [2 Audit examples, tools, repository structure and prompt workflows](prompts/2-ecosystem-tooling-audit.md) | COMPLETE | [Live comparison](library-comparison.md), [execution report](reports/implementation/2-ecosystem-tooling-audit.md); local reference/link and diff checks PASS. Commit/sync SHA in final handoff. |
| [3 Establish the primary register ledger and core silicon behavior](prompts/3-silicon-core-ledger.md) | COMPLETE | [Ledger/schema](registers/README.md), [behavior](device-behavior.md), [vectors](register-vectors.md), [coverage](feature-coverage.md), [execution report](reports/implementation/3-silicon-core-ledger.md); fresh independent audit integrated, 96 register records, 20 complete core rows, 41 tests and local reference/link/diff checks PASS. Source conflicts remain named implementation gates. |
| [4 Research diagnostic, event and counter procedures](prompts/4-silicon-diagnostic-ledger.md) | COMPLETE | [Report](reports/implementation/4-silicon-diagnostic-ledger.md), [ledger](registers/dp83826i.json), [vectors](register-vectors.md); 34 detailed diagnostic records plus three exact recipe records, 16 procedures and eight researched features. Integrated 57 research tests and local ledger/reference/link/diff checks PASS. Source/physical gates remain explicit; no driver/SDK/hardware claim. |
| [5 Complete power, wake, interface and remaining source coverage](prompts/5-silicon-power-interface-ledger.md) | COMPLETE | [Report](reports/implementation/5-silicon-power-interface-ledger.md), [ledger](registers/dp83826i.json), [coverage](feature-coverage.md); 79 complete detailed records, 17 recipe records, five blocked EEE associations, 283 fields, 52 procedures and all 35 researched families. 74 research-tool tests and local ledger/reference/link/diff checks PASS. 24 source gates remain explicit; no driver/SDK/hardware claim. |
| [6 Resolve portable core and native SDK ownership](prompts/6-framework-ownership.md) | COMPLETE | [Decisions/matrix](design-decisions.md), [architecture](architecture.md), [report](reports/implementation/6-framework-ownership.md); independent ownership review resolved, 77 research tests, 272 poisoned SDK-type vectors and local reference/ledger/link/diff checks PASS. Native lifecycle correction remains an explicit prompt-37 support gate; no driver/SDK-link/physical claim. |
| [7 Freeze value, Status and direct transport contracts](prompts/7-value-transport-contracts.md) | COMPLETE | [API contract](api-contracts.md), [report](reports/implementation/7-value-transport-contracts.md); independent state/error review resolved, six C++17 documentation blocks type-checked, all 35 features/52 procedures mapped, 77 research tests and local reference/ledger/link/diff checks PASS. Cooperative lifetimes remain prompt 8; no production/SDK-link/physical claim. |
| [8 Freeze cooperative operation, time and cleanup contracts](prompts/8-operation-contracts.md) | COMPLETE | [API contract](api-contracts.md), [report](reports/implementation/8-operation-contracts.md); independent lifetime/source review resolved, 85 research/design tests, eight portable C++17 documentation blocks and local reference/ledger/link/diff checks PASS. Native callback example is source-reviewed; driver, SDK link and physical execution remain later-stage work. |
| [9 Build the portable values, tests and source-documentation foundation](prompts/9-portable-source-foundation.md) | COMPLETE | [Public headers](../include/DP8382x/README.md), [report](reports/implementation/9-portable-source-foundation.md); four C++17 host suites, tests-OFF consumer, Doxygen negative checks and local research/reference/diff checks PASS. Device, SDK and physical evidence NOT RUN. |
| [10 Add deterministic version metadata and generation](prompts/10-version-tooling.md) | COMPLETE | [Version tool guide](../scripts/README.md), [report](reports/implementation/10-version-tooling.md); strict generated-header check, temporary version fixtures, C++17 host/consumer/Doxygen and research/reference/diff checks PASS. Device, SDK and physical evidence NOT RUN. |
| [11 Implement bounded Clause 22 transport](prompts/11-clause22-transport.md) | COMPLETE | [Direct-access guide](direct-access.md), [report](reports/implementation/11-clause22-transport.md); bounded binding/read/write subset, retained transport evidence, five new registered host suites and linked portable example PASS. Doxygen, version, 98 Python tests, ledger/reference/diff PASS. MMD/collector/init/SDK/physical evidence remain separate later scope. |
| [12 Implement complete atomic MMD transactions](prompts/12-atomic-mmd.md) | COMPLETE | [Transport guide](direct-access.md), [report](reports/implementation/12-atomic-mmd.md); atomic four-frame read/write and five-frame stable RMW, canonical aliases, phase-failure/exclusion tests and linked portable examples PASS. Local Release, Doxygen, ledger, reference, Python and diff checks PASS; EEE, side-effect-aware access and SDK/physical gates remain explicit. |
| [13 Implement side-effect-aware register access](prompts/13-side-effect-register-access.md) | COMPLETE | [Report](reports/implementation/13-side-effect-register-access.md), [access guide](side-effect-access.md); complete named catalog, collector, MISR/WoL updates, partial-evidence/invalidation tests and linked portable example. Local Release/consumer/Doxygen/Python/ledger/reference/diff checks PASS; physical/SDK qualification NOT RUN. |
| [14 Implement exact identity and capability selection](prompts/14-identity-capabilities.md) | COMPLETE | [Report](reports/implementation/14-identity-capabilities.md), [identity guide](identity.md); exact-address two-frame acquisition, partial/rejected evidence, passive capabilities, negative-evidence admission, independent Release suites and linked probe. Local host/consumer/Doxygen/Python/ledger/reference checks PASS; SDK/physical NOT RUN. |
| [15 Implement initialization and complete PCS reset operations](prompts/15-initialize-pcs-reset.md) | COMPLETE | [Report](reports/implementation/15-initialize-pcs-reset.md); 40 Release CTests, linked consumer, 102 Python tests, Doxygen/reference/ledger checks; SDK/physical NOT RUN |
| [16 Implement hard reset, restart and external-reset reconciliation](prompts/16-hard-reset-restart.md) | COMPLETE | [Report](reports/implementation/16-hard-reset-restart.md); 47 Release CTests, linked consumer, 102 Python tests and documentation/reference checks; Q5/SDK/physical NOT RUN |
| [17 Implement negotiated link settings and verified application](prompts/17-negotiated-link-settings.md) | COMPLETE | [Report](reports/implementation/17-negotiated-link-settings.md); host/consumer PASS, physical Q10 NOT RUN |
| [18 Implement forced modes and coupled crossover settings](prompts/18-forced-crossover-settings.md) | COMPLETE | [Report](reports/implementation/18-forced-crossover-settings.md); host/consumer PASS, Q11 NOT RUN |
| [19 Implement live and cached link observations](prompts/19-link-observation.md) | COMPLETE | [Report](reports/implementation/19-link-observation.md); Release/UBSan host, tests-OFF consumer and documentation checks PASS; SDK/physical NOT RUN |
| [20 Implement complete event and counter acquisition](prompts/20-event-counter-acquisition.md) | COMPLETE | [Report](reports/implementation/20-event-counter-acquisition.md); host/UBSan, consumer, docs and reference checks PASS; RECR/RMII derived semantics and physical Q16/Q17 remain gated |
| [21 Implement passive driver health and exact accounting](prompts/21-passive-health.md) | COMPLETE | [Report](reports/implementation/21-passive-health.md); managed callback counts, cached state/error evidence, 11 focused health suites and portable example; physical NOT RUN |
| [22 Implement explicit reconciliation and bounded recovery](prompts/22-reconcile-recover.md) | COMPLETE | [Report](reports/implementation/22-reconcile-recover.md); bounded stable-field verification, retained mismatch/cleanup evidence, shared initialization recovery, ten focused suites and portable example; physical NOT RUN |
| [23 Implement all supported loopback modes](prompts/23-loopback-modes.md) | COMPLETE | [Report](reports/implementation/23-loopback-modes.md); all documented mode/speed/interface paths, separate start/stop, retained temporary state, shared restore and independent host/consumer tests; Q8 physical NOT RUN |
| [24 Implement complete BIST generator and checker sessions](prompts/24-bist-generator-checker.md) | COMPLETE | [Report](reports/implementation/24-bist-generator-checker.md); all controls, bounded raw capture, partial evidence and verified stop; host/consumer PASS, exact-interval/PASS and physical BIST remain gated |
| [25 Implement complete manual cable capture](prompts/25-cable-manual.md) | COMPLETE | [Report](reports/implementation/25-cable-manual.md); six-run/raw capture, partial evidence, bounded HARD restoration and pure interpretation; host/consumer PASS; physical Q7 NOT RUN |
| [26 Implement cable autorun and shared result acquisition](prompts/26-cable-autorun.md) | COMPLETE | [Report](reports/implementation/26-cable-autorun.md); one owned enable flag, fixed raw acquisition, manual handoff, reset replay and verification; host/consumer PASS, physical timing/accuracy NOT RUN |
| [27 Implement bounded interrupt configuration and service](prompts/27-interrupt-service.md) | COMPLETE | [Report](reports/implementation/27-interrupt-service.md), [IRQ guide](interrupts.md); all sources/policy/test/service/replay, independent literals and failure regressions. 154 Release tests, two tests-OFF consumers and source/document checks PASS. Q9 remains NOT RUN. |
| [28 Implement active-sleep policy and explicit power states](prompts/28-power-states.md) | PARTIAL: DPD source-blocked | [Report](reports/implementation/28-power-states.md), [power guide](power.md), six Release suites and portable owner example. Active-sleep/IEEE/PLL implemented; deep_power_exit and Q6 remain open. |
| [29 Implement exact EEE negotiation and status](prompts/29-eee-negotiation.md) | COMPLETE | [Report](reports/implementation/29-eee-negotiation.md), [EEE guide](eee.md); committed/synced 356320ac. Q1 physical pending. |
| [30 Implement bounded legacy-MAC LPI sessions](prompts/30-legacy-mac-lpi.md) | COMPLETE | [Report](reports/implementation/30-legacy-mac-lpi.md), [LPI guide](lpi.md); bounded start/stop, retained OP-9 state, independent Release/consumer tests. Q1/Q2 physical pending. |
| [31 Implement complete Wake-on-LAN controls](prompts/31-wake-on-lan.md) | COMPLETE | [Report](reports/implementation/31-wake-on-lan.md), [WoL guide](wake-on-lan.md); eight Release suites, bounded config/ACK/status, routing/replay and portable owner example. Q3 physical pending. |
| [32 Implement all LED and GPIO controls](prompts/32-led-gpio.md) | COMPLETE for source-admitted routes | [Report](reports/implementation/32-led-gpio.md), [audit](reports/implementation/32-33-independent-audit.md); typed pin profiles and shared replay with literal/failure tests. pin_fields branches and physical Q12 remain gated. |
| [33 Implement clock, interface and repeater controls](prompts/33-clock-interface-repeater.md) | COMPLETE for source-admitted routes | [Report](reports/implementation/33-clock-interface-repeater.md), [audit](reports/implementation/32-33-independent-audit.md); interface/clock/isolation/repeater profiles and collector-safe replay. Unresolved clock routes and physical Q12/Q17 remain gated. |
| [34 Implement quality and hardware link-loss controls](prompts/34-quality-link-loss.md) | COMPLETE | [Report](reports/implementation/34-quality-link-loss.md), [guide](quality.md), five source-admitted procedures, single collector and shared replay; 214 Release/UBSan tests, 103 Python tests, two portable consumers and local source/document checks PASS. Physical timing and derived MSE units remain gated. |
| [35 Implement electrical controls and finite compliance procedures](prompts/35-electrical-compliance-calibration.md) | COMPLETE | [Report](reports/implementation/35-electrical-compliance-calibration.md), [guide](electrical.md); four source-admitted procedures, shared reset/replay, 289 split-field transition vectors and retained failure evidence. 229 Release/UBSan tests, 103 Python tests, two consumers and local checks PASS. Source gates and physical Q19 remain open. |
| [36 Audit complete primary-chip feature coverage](prompts/36-primary-feature-closure.md) | BLOCKED: primary closure; bounded review delivered | [Report](reports/implementation/36-primary-feature-closure.md); common reset-observation and EEE freshness fixes, 230 Release/UBSan CTests, two consumers and local checks PASS. Required source-gated branches remain with owners 28/32/33/35; prompt 55 owns row closure. |
| [37 Implement the native ESP-IDF factory and lifecycle](prompts/37-esp-idf-lifecycle.md) | COMPLETE; reusable teardown BLOCKED | [Report](reports/implementation/37-esp-idf-lifecycle.md), [native guide](esp-idf.md), [source audit](reports/implementation/37-native-source-audit.md); 234 portable Release, 24 pinned-driver/native Release and UBSan, two tests-OFF consumers and actual pinned P4 image link PASS. Installed resources remain retained under native_rx_quiescence; physical Q13 and remote CI NOT RUN. |
| [38 Complete SDK-aware configuration and raw MDIO arbitration](prompts/38-esp-idf-state-arbitration.md) | COMPLETE | [Report](reports/implementation/38-esp-idf-state-arbitration.md), [native guide](esp-idf.md), [source audit](reports/implementation/38-native-source-audit.md); 235 portable Release, 43 pinned-driver/native Release and UBSan, two tests-OFF consumers, actual P4 link and local source checks PASS. Reusable installed teardown remains BLOCKED; physical and remote CI NOT RUN. |
| [39 Prove native link notifications and adapter timing contracts](prompts/39-esp-idf-link-bounds.md) | COMPLETE | [Report](reports/implementation/39-esp-idf-link-bounds.md), [conformance/timing guide](esp-idf.md), [fresh source audit](reports/implementation/39-native-source-audit.md);55 native Release/UBSan,235 portable, two consumer tests and actual pinned P4 link PASS. Physical native_wall_time qualification and native_rx_quiescence remain separate gates; remote CI NOT RUN. |
| [40 Build the complete ESP32-P4 ordinary Ethernet example](prompts/40-p4-network-example.md) | COMPLETE for software/image | [Report](reports/implementation/40-p4-network-example.md), [board audit](reports/implementation/40-board-source-audit.md), [example](../examples/esp_idf/basic/README.md); 35 application tests, pinned P4 image. Physical PHY power/link/DHCP NOT RUN; reusable installed teardown BLOCKED. |
| [41 Implement explicit application admission and maintenance ownership](prompts/41-owner-admission-example.md) | Software complete | [Host and P4 link evidence](reports/implementation/41-owner-admission-example.md). Physical session qualification remains open. |
| [42 Demonstrate bounded native MAC frame handoff](prompts/42-owner-frame-boundary.md) | Software complete | [Host handoff and P4 image link](reports/implementation/42-owner-frame-boundary.md). Reusable teardown remains blocked by the SDK RX lifetime. |
| [43 Implement the portable CLI, bounded console and record schema](prompts/43-cli-foundation.md) | COMPLETE | [Grammar and schema](cli.md), [source audit and verification](reports/implementation/43-cli-foundation.md). Real-core host fixture and eight CLI CTests. Native console and physical evidence follow. |
| [44 Add complete identity, configuration and register CLI routes](prompts/44-cli-configuration-registers.md) | COMPLETE | [Grammar](cli.md), [matrix](feature-coverage.md), [verification](reports/implementation/44-cli-configuration-registers.md). SDK console attachment and physical qualification follow. |
| [45 Add link, lifecycle and retained-operation CLI commands](prompts/45-cli-link-lifecycle.md) | COMPLETE | [CLI guide](cli.md), [matrix](feature-coverage.md), [report](reports/implementation/45-cli-link-lifecycle.md). Portable, pinned SDK host, consumers, P4 links and source checks passed. |
| [46 Expose every diagnostic and opt-in feature through typed commands](prompts/46-cli-diagnostics-controls.md) | COMPLETE | [CLI guide](cli.md#typed-diagnostics), [matrix](feature-coverage.md), [report](reports/implementation/46-cli-diagnostics-controls.md). Portable, pinned SDK host, consumers, P4 links and source checks passed. Source and physical gates remain explicit. |
| [47 Integrate the shared CLI in both P4 applications and close coverage](prompts/47-cli-native-integration.md) | COMPLETE for software/image | [Report](reports/implementation/47-cli-native-integration.md) and [coverage matrix](feature-coverage.md). Shared console, pinned native host and both P4 links verified. Physical UART and PHY qualification remain open. |
| [48 Implement the offline protocol verifier and durable evidence bundles](prompts/48-runner-protocol-artifacts.md) | COMPLETE | [Report](reports/implementation/48-runner-protocol-artifacts.md) and [runner guide](hil-runner.md). Compiled host fixture and offline evidence verified. Serial follows in prompt 49. |
| [49 Implement bounded serial sessions without implicit hardware actions](prompts/49-runner-serial-session.md) | COMPLETE | [Report](reports/implementation/49-runner-serial-session.md) and [runner guide](hil-runner.md). Passive attach, bounded serial teardown and artifact replay verified with injected ports and the compiled CLI fixture. 263 Release CTests and two tests-OFF consumers PASS. Physical reset-silent attachment and PHY evidence NOT RUN. |
| [50 Build finite observation, configuration and link-mode campaigns](prompts/50-hil-observation-modes.md) | COMPLETE | [Report](reports/implementation/50-hil-observation-modes.md) and [case table](hardware-validation.md). 264 Release CTests, 8 focused UBSan CTests, two tests-OFF consumers and offline artifact replay PASS. Physical evidence NOT RUN; first-board INT BLOCKED. |
| [51 Build diagnostic and opt-in control fixture campaigns](prompts/51-hil-diagnostic-fixtures.md) | COMPLETE | [Report](reports/implementation/51-hil-diagnostic-fixtures.md) and [fixture cases](hardware-validation.md#feature-fixtures). 265 Release CTests, nine focused UBSan CTests, two portable consumer CTests and offline artifact replay PASS. Deep-power exit, native feature grants and first-board INT BLOCKED. Physical evidence NOT RUN. |
| [52 Prove finite soak and interrupted-campaign failure behavior](prompts/52-hil-soak-interruption.md) | COMPLETE | [Implementation, independent review and verification](reports/implementation/52-hil-soak-interruption.md). 267 Release CTests, 11 focused UBSan CTests, two portable consumer CTests and offline artifact replay PASS. The interruption suite replayed 2,927 cut prefixes and rejected 436,290 field mutations. Physical evidence NOT RUN. Deep-power exit, native feature grants and first-board INT remain gated. |
| [53 Close cross-entry transaction and SDK ownership regressions](prompts/53-regression-transaction-ownership.md) | COMPLETE | [Final verification](reports/implementation/53-regression-transaction-ownership.md#phase-d-final-verification). Cross-entry ownership, retained write effects and reset/EEE admission passed in 647 Release CTests, 36 portable and 79 native UBSan tests. Both standalone consumers passed. Three P4 images linked at clean `65ec309`. Physical evidence NOT RUN. |
| [54 Close lifecycle, deadline and retained-evidence regressions](prompts/54-regression-lifecycle-deadlines.md) | COMPLETE | [Final verification](reports/implementation/54-regression-lifecycle-deadlines.md#phase-d-final-verification). Reset deadlines, retained results and application teardown passed in 662 Release CTests, 107 native and 10 portable UBSan tests. Both tests-OFF consumers passed. Three P4 images linked at clean `7fbdab2`. Physical evidence NOT RUN. |
| [55 Audit independent coverage, portability and executing test registration](prompts/55-regression-coverage-portability.md) | COMPLETE | [Final verification](reports/implementation/55-regression-coverage-portability.md#phase-d-final-verification). The 87 DP83826I coverage rows name 96 executing tests. Debug, Release and UBSan each passed all 667 CTests at `dd1dad5`. Both tests-OFF consumers passed. Source-gated chip procedures and physical qualification remain open. |
| [56 Qualify the complete DP83826E software path](prompts/56-e-support.md) | BLOCKED on full I/E feature closure | [Final verification](reports/implementation/56-e-support.md#phase-d-final-verification). The declared E passed 30 E-specific Release and UBSan registrations and the adapter linked for ESP32-P4 at clean `39713f1`. Full E qualification requires the shared deep-power, pin and electrical procedures tracked by 28, 32/33, 35 and 36. Physical E testing NOT RUN. |
| [57 Establish exact AI/AE source and identity policy](prompts/57-a-source-policy.md) | COMPLETE | [Final verification](reports/implementation/57-a-source-policy.md#phase-d-final-verification) covers 79 detailed register tables, 285 fields and 52 procedures for both A grades at `0e2850f`. All 100 ledger tests, 697 I/E host CTests and two portable consumer tests passed. Prompts 58 and 59 own A implementation. Physical qualification remains open for each grade. |
| [58 Implement A-generation identity, reset and link differences](prompts/58-a-core-link.md) | COMPLETE | [Final verification](reports/implementation/58-a-core-link.md#phase-d-final-verification) covers the 21 assigned procedures for both revision-4 A declarations. All 791 Release CTests, 94 AI/AE UBSan tests and two portable consumers passed. All 22 compiled campaign results matched replay. Four ESP32-P4 images linked at clean `38ec9f1`. Physical qualification remains open for each grade. Prompt 59 owns diagnostic and optional-control closure. |
| [59 Close A-generation diagnostic and optional-control coverage](prompts/59-a-feature-closure.md) | NOT STARTED | None |
| [60 Establish DP83825I sources and implement its core differences](prompts/60-p25-source-core.md) | NOT STARTED | None |
| [61 Close complete DP83825I feature and integration coverage](prompts/61-p25-feature-closure.md) | NOT STARTED | None |
| [62 Establish exact DP83822 subtype and feature evidence](prompts/62-p22-source-subtypes.md) | NOT STARTED | None |
| [63 Implement DP83822 copper identity, reset and link behavior](prompts/63-p22-copper-core.md) | NOT STARTED | None |
| [64 Implement DP83822 interface, clock and fixed RGMII shifts](prompts/64-p22-interface-clock.md) | NOT STARTED | None |
| [65 Implement exact DP83822IF/HF fiber behavior](prompts/65-p22-fiber.md) | NOT STARTED | None |
| [66 Implement DP83822 diagnostics and documented test procedures](prompts/66-p22-diagnostics.md) | NOT STARTED | None |
| [67 Close DP83822 power, wake, LED and complete subtype coverage](prompts/67-p22-optional-closure.md) | NOT STARTED | None |
| [68 Prove independent two-instance application consumption](prompts/68-portable-owner-consumer.md) | NOT STARTED | None |
| [69 Finalize deterministic version and supported-package metadata](prompts/69-package-metadata.md) | NOT STARTED | None |
| [70 Validate bounded archives and clean extracted consumers](prompts/70-package-export-consumers.md) | NOT STARTED | None |
| [71 Execute the single dedicated remote CI checkpoint](prompts/71-ci-checkpoint.md) | NOT STARTED | None |
| [72 Execute admitted physical boot, link and recovery campaigns](prompts/72-hw-link-campaign.md) | NOT STARTED | None |
| [73 Execute admitted feature, medium and timing qualification](prompts/73-hw-feature-campaign.md) | NOT STARTED | None |
| [74 Verify user quick starts and consumer journeys](prompts/74-docs-user-consumers.md) | NOT STARTED | None |
| [75 Reconcile complete feature claims and evidence guides](prompts/75-docs-feature-evidence.md) | NOT STARTED | None |
| [76 Audit exported source contracts and executable Doxygen checks](prompts/76-docs-api-audit.md) | NOT STARTED | None |
| [77 Verify maintenance guidance, tools and ownership boundaries](prompts/77-docs-maintainer-tools.md) | NOT STARTED | None |
| [78 Review silicon completeness and configuration independently](prompts/78-review-silicon.md) | NOT STARTED | None |
| [79 Review ownership, bounds and evidence independently](prompts/79-review-ownership-evidence.md) | NOT STARTED | None |
| [80 Close final implementation and qualification claims](prompts/80-final-integration.md) | NOT STARTED | None |

A stage can be complete as software while its separately tracked physical
qualification remains NOT RUN, if its acceptance explicitly permits that.
Status legend: at 72/73 use BLOCKED when software/readiness is complete but
physical measurements are NOT RUN. Prompt 80 can be COMPLETE for its report and
checks while the execution set and final handoff remain PARTIAL.
Prompts 72-73 cannot complete required physical measurements without actual evidence;
the final handoff must disclose outstanding required gates and retain incomplete
overall status until those gates close.
Do not change statuses merely because a prompt or future command exists.

Record each stage under COMMON's report template in
`docs/reports/implementation/N-topic.md`, linking actual checks and unresolved
requirements from its row above. Include execution branch/base, completed commit
and verified sync result. Keep CI and physical evidence tied to their actual
commit/image and conditions.
