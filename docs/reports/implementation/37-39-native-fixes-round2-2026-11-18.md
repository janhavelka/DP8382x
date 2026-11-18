# Native adapter second-round review, 2026-11-18

This corrects completed prompts 37–39 and responds individually to the attached
review of `998cb6de85bf0b86900d5882a2e616b384572f05`. Prompt 40 remains unstarted.
Entry was clean `main`, equal to `origin/main` after fetch. Root/parent agent
rules, COMMON, all three original prompts, ownership decisions and current
implementation were reread. The [preceding report](37-39-native-fixes-2026-11-18.md)
is historical; this report and the [native guide](../../esp-idf.md) describe the
corrected behavior. No portable feature, register value or vendor file changes.

The lead owns production code, shared declarations, documentation and Git.
Independent gpt-6.2-astra high-reasoning agents reproduced lifetime/deadline and
observation failures separately, appropriate to the SDK ordering and retained
effects. One then owned regression tests; the lead reviewed/integrated those
changes. A gpt-6.2-sol high-reasoning agent reviewed the prose inventory. Its
whitespace corrections are a separate checked commit. All agents use distinct
ignored build directories. Reviewers have no Git or SDK/sibling write authority.

## Evidence checked again

The supplied `review998/a/rvprobe.cpp` and `review998/b/probe.cpp` reproductions
were present and run against production adapter/core plus real pinned driver
sources. Each applicable case becomes a registered regression in
[native.cpp](../../../test/esp_idf/host/native.cpp), including the related
late-admission paths found in independent review.

The SDK remains ESP-IDF v6.1 commit
`fff9895c82d744c7237be8847347bdd1b07c6643`, locally
`C:/esp/v6.1/esp-idf`. The harness verifies the complete selected dependency
closure of 45 SDK blobs at all three tick rates. Inspected source includes
`components/esp_eth/src/esp_eth.c`, `esp_eth_netif_glue.c`, public PHY/driver
headers, `src/phy/esp_eth_phy_802_3.c` and the P4 EMAC implementation under
`src/mac/esp_eth_mac_esp.c`. The immutable
[driver source](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/esp_eth.c)
confirms that running timer polls ignore get_link failures, stop changes FSM
before later fallible work, and uninstall deletes the timer before PHY deinit.
The P4 valid set_link(DOWN) path already attempts MAC stop and updates the SDK
link cache even when a later DISCONNECTED post fails; it discards the MAC stop
return. Therefore the review's claim that this necessarily
leaves the MAC UP is too broad; missing framework/fence/direct-stop observation
is the confirmed defect.

Rechecked [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf), section 7.6
and reset/clock requirements, against the current
[official TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf), still revision I.
The existing 50 ms POR, 2 ms release readiness and clock-qualified pulse rules
remain unchanged. A foreign quiet floor is controller admission, not this PHY's
reset-completion time. No timing value is inferred from host execution.

Fresh read-only sibling review found unchanged clean revisions: TMP1x2
`afd50889`, PCA9555 `0aec2212`, INA228 `8e439d6c`, TCA9548A `4a244ada` and
FieldCore-node `c40ea834`. Paths checked include their AGENTS, Status headers,
implementation/test directories, native examples and Python runners, with exact
path families in the [source audit](39-native-source-audit.md). In particular:

- TMP1x2 `src/TMP1x2Operations.cpp` retains admission, cancellation and results.
- PCA9555 `src/PCA9555.cpp` separates deadline failure from owed cleanup.
- INA228 `examples/esp_idf/basic/main/main.cpp` captures an operation ID before
  result consumption; `Ina228IdfI2cTransport.cpp` preserves backend details.
- TCA9548A `examples/espidf_basic/main/main.cpp` preserves SDK failures and
  explicitly classifies controller fault state.
- FieldCore `src/devices/i2c/Ina228Module.cpp` separates owner admission, operation
  identity and publication.

Adopt retained effects and precise error ownership; adapt to whole MDIO batches.
Reject I2C transfer granularity, bus-specific NACK categories and offline I/O
latches. Existing sibling workflows were read as command precedents only; this
repository still defers first-execution remote CI to prompt 71.

## Finding dispositions

| Item | Disposition | Correction or source-backed limitation |
| --- | --- | --- |
| 1 | Fixed | Separate transient misses from lost trust. Positive `maxObservationMisses`, default 3, counts admitted RUNNING failures. Below the limit retain UP; at the limit withdraw administratively without latching stopRequired. Later coherent evidence republishes normally. Own publication in flight defers the DOWN debt; no second publisher runs. `lastObservationStatus` exposes the current cause even behind an old firstError. Contention, changing samples, P4 busy before issue and unresolved UP mode are transient; quarantine, stopped/invalid-clock transport, lost SDK agreement and terminal publication uncertainty require stop. |
| 2 | Fixed | Board preparation waits for its own reset-release due time and retains the shared atomic maximum. Install/batches still enforce a later peer floor; that floor cannot fail this completed board procedure or close the controller. |
| 3 | Fixed | Stop changes phase only after successful in-budget admission. Expired, exhausted or late-acquired first entry leaves RUNNING polling intact. Already admitted cleanup still retains timeout evidence. Gate retry refreshes time after a failed acquisition before requesting another tick. |
| 4 | Fixed; claim narrowed | Failed DISCONNECTED repost no longer withholds SDK stop/fence/STOP/direct MAC stop. Cleanup posts request zero queue wait and preserve the repair debt/error; STOP can precede the eventually repaired DOWN event. Actual P4 MAC DOWN may already be accepted, so the old behavior did not universally leave MAC UP. |
| 5 | Fixed | Raw-uninstall latch closes start/control but preserves native STOPPING cleanup using the independent fence and actual MAC, never the deleted SDK timer. Cancel/takeResult remain bus-silent and admitted even in retained MAINTENANCE. Successful result delivery releases owned INITIALIZING silence independently of phase. Native stop completion does not consume the core result. |
| 6 | Fixed | Retire's final gate timeout creates no cleanup error after stop has completed. Stop in completed MAINTENANCE is idempotent OK, matching the public contract. Resources remain retained. |
| 7 | Not a numeric-validation defect; contract clarified | `cleanupBudgetUs` is a positive attempt allowance. No source-qualified universal minimum covers scheduler/fence dispatch, previous callbacks and generic MAC stop. Inventing a minimum would falsely promise completion. A too-short allowance retains obligations; an explicit later stop supplies its own finite deadline. Finite poll/attempt caps remain enforced. |
| 8 | Fixed | Board delays require a full tick within remaining allowance using the rational conversion, tested at 1000/100/333 Hz. Recheck elapsed time after wake; scheduling itself remains conditional. |
| 9 | Fixed | Board aggregate expiry and stopping-publication gate expiry retain core OPERATION_TIMEOUT even when forwarded through esp_eth_stop. Actual SDK callback TIMEOUT remains sdkError. Init mutex-release failure is BUSY/CONTROLLER_QUARANTINED; exact ESP_FAIL becomes firstError if empty, preserving an earlier cause otherwise. No MDIO sample or token is published. |
| 10 | Fixed documentation | SharedBus Doxygen and guide explicitly require all participants to recheck admission/quarantine/quiet after taking the controller mutex and then hold it for the whole batch. A pre-lock check alone cannot protect INSTALLING. |
| 11 | Fixed tests | Failed initialization covers actual ESP_FAIL mapped to MDIO_ERROR, as well as the existing failure classes. Tests assert lifecycle phase CONTROL before taking a terminal result and STOPPED afterward, plus result/transport details. |
| 12 | Fixed in separate prose commit | Hand-reviewed insertion-only word/number spacing in maintained Markdown and comments. Code, identifiers, literals, paths, hashes, quoted source labels and vendor payloads are preserved. The separate commit and final checks are recorded in the handoff. |

The selected transient bound counts queries admitted to the instance gate;
starvation or a stopped scheduler has no unconditional wall-clock bound. A
failed stop/start retains its original error and cleanup history. No cleanup
budget preempts a synchronous SDK call. Raw direct stop while already DOWN can
post STOP without any PHY callback; after forbidden uninstall its delivery is
unknowable through the public API. Conservative owner repair can duplicate that
raw STOP. This is documented and tested as a prohibited-bypass limitation, not
an exactly-once guarantee for unseen external actions.

## Verification and documentation

Windows GNU 15.1.0/Ninja Release retains active assertions. Native tests compile
the production adapter, core and actual pinned SDK at 1000, 100 and 333 Hz.
UBSan uses both C/C++ `-fsanitize=undefined -fsanitize-undefined-trap-on-error`.
P4 linking uses the repository wrapper and SDK GCC 15.2.0. Final run results
and synchronized commit IDs are recorded below and in the handoff.

| Layer / command | Result | Evidence |
| --- | --- | --- |
| Native Release, `build/r2-native`, CTest `--output-on-failure --no-tests=error` | PASS | 262 cases: 85 native suites at each of 1000/100/333 Hz plus seven pinned-driver cases; independent reviewer also passed 262 in `build/r2-tests` |
| Native UBSan trap, `build/r2-ubsan`, same CTest command | PASS | All 262 cases with C and C++ undefined-behavior traps |
| Portable Release, prescribed configure/build and CTest in `build/host` | PASS | 236 executing tests, assertions active |
| Tests-OFF package consumer, `build/r2-consumer` | PASS | Both registered consumers compile, link and execute |
| `scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-fix2 build` | PASS | Real P4 SDK image linked, execution guard remains false |
| P4 static stack inspection | PASS, partial scope | Actual fixture compile commands plus `-fstack-usage`: getLink 304, readLinkStatus 512, core batch 416, native batch 160 bytes; nested subtotal 1392 before deeper SDK paths. Separate verifiedSettings 448 bytes. No dynamic high-water measurement |
| `python tools/check_doxygen.py` | PASS | 291 HTML pages, links/assets and two rejected negative fixtures |
| `python tools/verify_ledger.py` | PASS | 97 registers, 52 procedures, 35 features; structural research validation |
| `python tools/verify_references.py` | PASS | 29 artifacts, 27 extracts and repository links/hashes |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS | 103 Python tests |
| `python scripts/generate_version.py check`; `git diff --check` | PASS | Version current, no whitespace errors |
| Remote CI; physical execution | NOT RUN | Prompt 71 policy; no fixture operation or physical timing/stack qualification |

All earlier regressions remain enabled. Running the full suite at 333 Hz required
the board tick-boundary test to compute a rational boundary rather than truncate
the tick period. Two stale expectations were corrected: fully repaired stop
returns OK with the prior error retained, and the added ESP_FAIL initialization
case follows the same controller-quarantine admission as a timeout.

README, changelog, native Doxygen/guide, architecture, ownership decisions, test
notes and progress are reconciled. Portable API/feature ledger and example API
signatures need no change; no new feature or stage is claimed. Existing native
examples continue using owner lifecycle and explicit scheduling/budget inputs.

Reusable installed teardown remains **BLOCKED by native_rx_quiescence**. Driver,
MAC, glue, PHY, netif and borrowed resources remain alive until system restart.
Physical PHY execution, traffic, wall-time and stack high-water evidence are
NOT RUN; no fixture was operated. Remote CI is NOT RUN under prompt 71 policy.

Checked and synchronized units: `990b30d` contains the behavior, regressions and
guide; `1deab61` contains only the reviewed spacing corrections. Each matched
origin/main with zero ahead/behind after push and fetch. The spacing unit inserts
2,892 spaces across 162 files and preserves every other byte. Additional checks
confirm unchanged executable tokens, string literals, Markdown code/link targets,
known hashes and referenced anchors. Its Doxygen, reference/ledger/version checks
and all 103 Python tests pass. Final documentation review narrows the shutdown
proof, first-error retention and timeout-phase wording without changing behavior.
The final handoff records the documentation checkpoint and clean image identity.
