# Prompt 37: native ESP-IDF factory and lifecycle

Execution began on clean, fetched, synchronized `main` at
`cb5ffe5b5d21e700c3e9fbef434e192a50b1aeff`. Prompt37 is complete for the selected
native implementation, with reusable installed teardown **BLOCKED** by the
required `native_rx_quiescence` gate. The terminal retained-resource route is
implemented and tested. Final commit/upstream equality belongs to the handoff,
avoiding a self-referencing commit in this report.

## Inputs, ownership and scope

The prerequisites are the current ownership design, OP-1 through OP-17, the
implemented portable transport/collector/identity/initialization/reset/configuration/
link procedures and prompt36's bounded closure. Prompt36's source-gated branches
remain blocked; they do not block the implemented native subset. The full
[source audit](37-native-source-audit.md) records fresh peer paths, revisions,
dirty state, SDK functions, actual tool identities and adopted/rejected behavior.
No sibling, SDK or vendor payload was changed.

The lead owns shared/public contracts, the native adapter, integration,
architecture, progress and Git. `p37_host_tests` and `p37_peer_fixture` used
gpt-6.1-sol/high for the disjoint real-driver harness and fixture/source audit.
`p37_sdk_review` used gpt-6-astra/high for independent lifetime/deadline reasoning;
it also implemented the specifically assigned core restart procedure and its
tests before returning to read-only review. All contributions were reviewed
against actual code and SDK source. Separate build folders avoided interference.

## Delivered behavior and reviewed failure paths

- Native `Phy::create`, first-member `PhyHandle`, local installer callbacks and
  immutable explicit address. Factory validation rejects missing MAC callbacks
  before allocation; outputs remain unchanged on failure. Gate/fence allocation
  unwinds locally. The adapter owns no borrowed MAC or controller mutex.
- Clock-qualified, bus-wide board silence through POR/reset readiness before
  installation, with finite elapsed-time/poll checks and no MDIO. The fixture
  explicitly selects the maintained P4 pins/address and disabled execution guard.
- Direct actual-MAC terminal batches under the shared controller mutex, including
  complete MMD/RMW exclusion, 16-bit read validation, deadline admission, retained
  native errors and controller quarantine. Raw SDK hooks always reject, even for
  owner calls, without touching outputs or issuing frames.
- Stopped typed NONE/PCS/RESTART initialization and AN configuration reuse the
  portable core and retained operation results. `restartAutoNegotiation` shares
  BMCR composition with configuration and performs one three-frame batch without
  a token/wait. Uncertain mutation withdraws trust and retains restoration debt.
  Native HARD and mirrored mode transitions remain explicitly unavailable until
  stage38; no success stub or second chip implementation was introduced.
- Initial/running live link uses the same collector. DOWN/unresolved observations
  never invent speed/duplex. Qualified UP publishes speed, duplex, narrow peer
  pause and link; changed UP first projects DOWN. Identical tuples emit nothing.
  Each accepted field is retained if a later notification fails. Known P4 LINK
  event failure preserves accepted MAC/cache state; event-only retry never replays
  UP. Generic uncertain publication closes further publication/start admission.
- One phase enum and lifecycle record preserve primary and cleanup failures.
  Start failure calls SDK stop; stop handles the pinned unarmed-timer INVALID_STATE,
  fences once, repairs missing DOWN/STOP and observes actual MAC-stop failure.
  Fence retry polls the same timer, with no needed lock held while draining.
  Retire closes admission and retains all installed resources until restart.
- Explicit discovery reserves the whole selected scan (at most 64 frames),
  validates each independent ID read, never selects an address, and retains
  partial evidence while leaving ordinary output unchanged on failure.

Independent review and regression work corrected install-result publication
under snapshot contention (a terminal mailbox preserves the SDK outcome),
STOP-resume event accounting, lost-event supersession, partial notification
evidence and aggregate notification allowance. It also closed quarantine receipt
classification, active-operation discovery bypass, discovery arithmetic overflow,
clock-latch bypass, terminal event retry, raw-uninstall admission and expired-stop
preflight. The final review also found a discarded expired-stop result during
retirement; retirement now records that cleanup failure before retaining resources.
Review found no remaining blocking defect in the admitted routes.

The raw and netif/glue tests separately pause RX after capturing its mediator
but before entering the application callback, with application RX count zero.
Retire retains driver/glue/MAC/PHY/context storage; resumed RX still references
live objects. This models the source race and owner decision, not actual RX
task scheduling. No safe public-API reusable teardown route was established.

## Verification

Environment: Windows/PowerShell, GNU host compiler 15.1.0, Ninja/CMake 4.0.1; SDK
`C:\esp\v6.1\esp-idf`, unmodified ESP-IDF v6.1 commit
`fff9895c82d744c7237be8847347bdd1b07c6643`, Python 3.12.10 and P4 GCC 15.2.0
(`esp-15.2.0_20251204`). The repository wrapper alone activated SDK tools.

| Layer | Result | Executed evidence |
| --- | --- | --- |
| Portable host | PASS | Release configure/build and 234/234 CTests, including three new terminal-restart suites with 160 assertions; Release assertions enabled |
| Portable consumer | PASS | Separate `test/consumer` configure/build with tests and pinned SDK tests OFF; 2/2 linked CTests |
| Pinned driver/native host | PASS | Actual `esp_eth.c`/glue plus adapter; 24/24 Release and 24/24 UBSan trap CTests; all 45 selected SDK blobs verified |
| Actual SDK image link | PASS | ESP32-P4 fixture builds/links through `scripts\idf.cmd`; binary size `0x58b60`, no flash or execution |
| Research/tool tests | PASS | 103 Python unittest tests; ledger validates 97 registers, 52 procedures and 35 feature families |
| Documentation/version | PASS | Doxygen public-contract/negative checks, deterministic version check, local reference/link and diff checks |
| Installed/exported package | NOT RUN | The tested portable consumer uses add_subdirectory; no package-publication claim |
| Remote CI | NOT RUN | First-execution prompt71 policy; no dispatch/status/waiting |
| Physical/Q13 | NOT RUN | No authorized fixture execution, timing measurement, real RX scheduling or cable/link result |
| Reusable installed teardown | BLOCKED | Pinned source RX lifetime gate; resources remain retained until restart |

Commands used from the repository root (SDK tests are off for ordinary consumers):

```sh
cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure --no-tests=error
cmake -S test/consumer -B build/p37-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF -DDP8382X_BUILD_PINNED_IDF_TESTS=OFF
cmake --build build/p37-consumer
ctest --test-dir build/p37-consumer --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p37-agent-host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf
cmake --build build/p37-agent-host
ctest --test-dir build/p37-agent-host --output-on-failure --no-tests=error
cmake -S test/esp_idf -B build/p37-agent-host-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_PINNED_IDF_PATH=C:/esp/v6.1/esp-idf -DCMAKE_C_FLAGS="-fsanitize=undefined -fsanitize-undefined-trap-on-error" -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/p37-agent-host-ubsan
ctest --test-dir build/p37-agent-host-ubsan --output-on-failure --no-tests=error
python tools/verify_references.py
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
python tools/check_doxygen.py
python scripts/generate_version.py check
git diff --check
```

```bat
scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-adapter set-target esp32p4 build
```

The final source rebuild used `build` on that configured P4 folder. Fixture Git
identity refreshes each build independently of the library version; the final
post-commit identity is recorded in the handoff.

## Documentation and remaining limits

Reviewed/updated public Doxygen, API contracts, architecture, responsibility
matrix/design decisions, native/component guide, example plans, test/tool guides,
README, CHANGELOG, feature coverage and progress together. Component metadata
pins IDF6.1/P4 without duplicating the library version. Exact silicon ledger,
variants, source gates, portable operation lifecycle and SDK pin are unchanged.

Conditional fixture transport/notification bounds are explicitly unmeasured.
SDK nominal 1000 us MDIO polling, 100 ms MAC mutex waits and 2500 us stop polling
are contributions, not scheduling-inclusive guarantees. Synchronous SDK calls
cannot be preempted; late effects remain retained. HAL FIFO-flush timeout
visibility and physical timer/RX behavior remain separate limits. Stage38 owns
SDK-cache mode reconciliation and expanded raw/control routes; stage39 adds
conformance depth. Neither future stage is claimed complete here.
