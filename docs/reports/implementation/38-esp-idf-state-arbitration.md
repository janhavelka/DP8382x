# Prompt 38: SDK-aware configuration and raw arbitration

Prompt 38 began on clean, fetched, synchronized `main` at
`a744e77e3ac000be5ea12b1c4b5fdef55b50efde`. It completes the requested SDK
configuration and raw-entry scope while preserving prompt 37's lifecycle and
terminal installed-resource retention. Final commit/upstream equality is recorded
in the handoff, avoiding a self-referencing commit in this report.

## Inputs and ownership

The inputs are prompt 37's actual factory/driver harness/P4 fixture, the shared
core configuration/reset/transport/collector implementation, the frozen single
owner matrix and OP-1 through OP-17. Fresh reads include root/parent AGENTS,
COMMON, CONVENTIONS, ENGINEERING, VERIFICATION, EVIDENCE, current architecture,
variant/reference scope and relevant CLI/source responsibilities. No ledger
procedure has implementationStage38: this stage integrates existing procedures
without changing silicon masks, source gates or supported variants.

The [source audit](38-native-source-audit.md) records current INA228/PCA9555/
TCA9548A production, native, failure-test, CLI/runner and workflow inputs with
exact revisions and clean states. Actual ESP-IDF v6.1 remains unmodified at
`fff9895c82d744c7237be8847347bdd1b07c6643`; driver/header/base/MAC source was
reopened for cache ownership, control order, raw bypasses and flow argument width.
No sibling, SDK or archived vendor payload changed. No workflow exists yet;
first-execution remote CI remains prompt 71.

The lead owns public/shared headers, core/native integration, architecture,
progress, final report and Git. gpt-6.1-sol/high agents owned the disjoint native
harness and peer/source audit plus fixture. gpt-6-astra/high independently reviewed
ownership, partial effects and deadlines without editing production. The model
choices match bounded implementation versus stronger concurrency reasoning.
Build folders were separate; the lead checked each contribution and final diff.

## Implemented routes and decisions

- Native `startConfigure` now accepts complete legal AN and forced 10/100 half/full
  settings, including the core's ConfigureOptions. Pure invalid arguments preserve
  tokens and SDK agreement and perform no MAC/MDIO mutation. Extended full duplex
  with forced half duplex rejects because native UP cannot be qualified.
- Shared typed configuration is the sole chip engine. After taking a successful
  core result, `reconcileSdkState` calls public stopped S_AUTONEGO/G_AUTONEGO and
  forced S_SPEED/S_DUPLEX_MODE. Expected callback/task/phase/deadline guards
  acknowledge already verified settings without another write, wait or operation.
  The SDK owns its cache output. `SdkStateSnapshot` retains separate control
  acceptance and readback; core OperationResult remains immutable.
- SDK speed/duplex getters retain last published link values, including initial
  defaults or old values while stopped. Tests check this before and after forced
  start. Forced UP cannot project stale ANLPAR pause; only qualified completed
  negotiation can publish the narrower partner-capability Boolean.
- All initialization/reset choices reserve shared logical silence through result
  delivery. A zero-wait controller acquisition drains prior batches, then releases
  immediately. Owner polling never holds the controller through waits. Result
  delivery reopens admission while preserving any quiet floor/quarantine.
- Native raw C22/MMD writes and MMD RMW reuse core policy, complete batches and
  collector invalidation. Ordinary BMCR/selector/destructive aliases stay denied.
  Explicit `modifyBasicControl` permits only raw stable BMCR mask 0x3100 in one
  two-frame RMW, preserving desired intent and neutralizing commands/power-pin OR.
  Accepted/uncertain changes withdraw readiness/trust and SDK agreement; restore
  via initialization then reconcile. `acquireEvidence` retains consumed prefixes;
  cached evidence and transport receipts perform no I/O.
- Raw SDK hooks remain unconditional zero-frame rejection, including owner calls.
  There is no private SDK cache patch, generic registry, selector permit or second
  configuration state machine. MMD 31 uses initialized 0x001F/0x401F selectors with
  all four frames under one shared bus lock; RMW uses five.
- `setFlowControl` calls MAC(bool) and PHY(uint32_t) separately after matching the
  verified AN pause profile. The defective S_FLOW_CTRL ioctl is never used by the
  supported route. A fixed receipt distinguishes attempts, accepted/uncertain MAC
  effects, PHY acceptance and both errors. Local validation has no effects; partly
  applied SDK controls make no such promise. Explicit retry is application policy.
- Invalidated direct framework starts are rejected for both cached AN values while
  retaining earlier FSM/START effects. Owner stop/fence repairs the pinned unarmed
  timer path, preserving its original INVALID_STATE and separate cleanup evidence.
  `set_addr` remains idempotent for the declared address; installed deletion stays gated.

Review found and fixed a BUSY detail that the core correctly rejected as an invalid
transport status, post-control gate contention that could strand CONTROL or lose
accepted effects, stale forced pause, foreign flow-control effects during a scoped
callback, queued STOPPING polls mistaken for new starts, and lost direct-start
evidence on callback gate failure. Terminal outcome/deferred-event mailboxes retain
these facts without blocking retries or holding locks around SDK calls. Atomic
phase inspection serves only failed-entry evidence; compound transitions remain
guarded. All findings have executing regressions and independent review closure.

## Local verification

Environment: Windows/PowerShell, CMake 4.0.1/Ninja, GNU host 15.1.0, Python 3.12.10;
SDK path `C:\esp\v6.1\esp-idf`, P4 GCC 15.2.0 (`esp-15.2.0_20251204`).
The actual pinned checkout and all 45 compiler-selected SDK source/header blobs
were verified. No SDK public Ethernet headers were replaced by shims.

| Layer | Result | Scope |
| --- | --- | --- |
| Portable host | PASS | 235/235 Release CTests; new literal BMCR maintenance vectors, rejection, failure/late/release outcomes; assertions active |
| Native pinned-driver host | PASS | 43/43 Release and 43/43 GNU UBSan trap CTests; original 24 plus 19 new; real esp_eth.c/glue and adapter |
| Portable consumer | PASS | Separate tests-OFF build, 2/2 linked CTests, no native dependency |
| Actual SDK image | PASS | Final P4 fixture image links all current native sources and actual SDK MAC/driver; `0x5c080` bytes before final identity refresh |
| Research/tooling | PASS | 103 Python tests; ledger 97 registers/52 procedures/35 features; version generation check |
| Source documentation | PASS | Doxygen 289 pages plus 2 negative fixtures; reference/local-link and diff checks |
| Installed/exported package | NOT RUN | Build-tree consumer only; package stages remain 68-71 |
| Remote CI | NOT RUN | First-execution prompt 71 policy |
| Physical | NOT RUN | Guard remains false; no flashing, board operation or timing measurement |
| Reusable installed teardown | BLOCKED | Unchanged native_rx_quiescence source gate; retain resources until restart |

The native regressions cover all four forced tuples and cache publication,
configuration validation, raw read/write/RMW boundaries, complete exclusion across
another PHY and reentry, failed acquisition/release, zero-effect rejection,
destructive prefix retention, raw BMCR replay/public reconciliation, shared reset
silence, flow widths/partial effects/interference, post-callback contention, and
direct-start failures with AN true/false including failed callback admission.
Correctly typed MAC/PHY calls model the known defective flow ioctl's mutation
ordering without deliberately invoking its unsafe bool-pointer width behavior.
CLI grammar belongs to 43; these tests prove the pure validation/owner boundary
that a CLI must call, not a currently nonexistent CLI implementation.

Commands run from the repository root:

```sh
cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p38-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p38-native -j 2
ctest --test-dir build/p38-native --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p38-native-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf -DCMAKE_C_FLAGS="-fsanitize=undefined -fsanitize-undefined-trap-on-error" -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/p38-native-ubsan -j 2
ctest --test-dir build/p38-native-ubsan --output-on-failure --no-tests=error
cmake -S test/consumer -B build/p38-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF -DDP8382X_BUILD_PINNED_IDF_TESTS=OFF
cmake --build build/p38-consumer -j 2
ctest --test-dir build/p38-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_references.py
python tools/verify_ledger.py
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

```bat
scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-p38-peer set-target esp32p4
scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-p38-peer build
```

Final source image SHA256 before commit identity refresh:
`82677CB7912EE21671FB5FA214C02C6329ABE6DC3F0E896CDEEF3486DEF62385`.
Its identity is entry commit `a744e77` plus dirty tree; final clean identity is
verified after commit and recorded in the handoff. An image link proves its layer,
not timer concurrency, RX scheduling, MAC/MDIO or physical link behavior.

## Documentation and remaining gates

Updated public Doxygen, API/native/architecture/ownership guides, configuration/
restoration/raw guides, feature coverage, example plans, test guide, README,
CHANGELOG and progress. Component/source selection adds only PhyControl.cpp;
portable consumers still exclude SDK code and optional tools. The version,
scripts/README build activation, variants, SDK pin and register ledger source
facts are unchanged; no release-version increment or invented silicon support.

Timing declarations remain conditional on the application-qualified backend;
the fixture's values are unmeasured. Synchronous SDK calls cannot be preempted.
Late accepted effects survive timeout. Physical qualifications, source-gated chip
branches and the installed RX lifetime gate remain open. No unsupported teardown
or unrelated native feature facade is introduced to conceal those limits.
