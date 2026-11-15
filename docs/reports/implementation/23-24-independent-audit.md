# Independent audit of prompts 23-24

Date: 2026-11-15. Entry was clean, fetched and synchronized on `main` at
`63b4121c9b38fe5421c50eea958a441b81e58ff8`. Re-read both original prompts in
full and reviewed the implementation from `9dee869` through prompt 23 `d58a7da`
and prompt 24 `63b4121`. This audit does not start prompt 25. Final commit and
upstream equality are recorded in the handoff.

## Scope and review

Owned procedures: `loopback_near`, `loopback_digital`, `loopback_reverse`,
`loopback_external`, `bist_run`, `bist_capture`. Checked shared contracts,
OP-1 through OP-17, current source, public declarations, registered tests and
portable examples. One application owner, one operation slot, one collector
and the existing initialization/profile restoration remain authoritative.
No new framework, allocation, retry, SDK path or fixture action was introduced.

Three inherited capable agents reviewed disjoint scopes in parallel:
`audit2324_lifecycle` examined state, deadlines, barriers and restoration;
`audit2324_silicon` checked original PDF arithmetic, access and ordering;
`audit2324_tests` inspected acceptance coverage, examples and adversarial tests.
This kept independent reasoning on silicon and failure boundaries. Reviewers
initially worked read-only in separate ignored build directories. After the lead
confirmed findings, the test reviewer owned only `test/bist.cpp`; the lead reviewed
that contribution and owned production changes, shared documentation and Git.
No sibling or vendor payload was changed.

## Findings and corrections

| Finding | Evidence against original implementation | Correction and regression |
| --- | --- | --- |
| MII line transmission violated its prerequisite during transitions | Table 8-40 permits BISCR6 only with BMCR14 set. Start set BISCR6 first; stop cleared BMCR14 first. | Enable BMCR14 before the final BISCR word; clear BISCR before BMCR on exit. Independent literals and a per-write prerequisite assertion cover both interfaces, fault/cancel boundaries and retries. No extra frames. |
| BIST stop mutated before validating current identity | A session followed by ID2 `A141` performed one BISCR write before DEVICE_MISMATCH. | Reuse the existing stop identity phase before PRBS disable. Six normal/analog/digital regressions cover mismatch and second-ID timeout, two reads and zero writes, retained barriers and a later explicit stop. |
| Dwell collector evidence was omitted from typed BIST observations | A permitted BISCR read saw loss `0400`; later clear-loss bookends returned `syncLossObserved=false` and ERRORS_OBSERVED. Only typed acquisition accumulated session flags. | Accumulate current-session activity/loss in the single collector. Six cases cover explicit collector, direct C22 and vendor alias reads, subsequent RH changes and bus-silent host-history clearing. |
| HARD recovery skipped a previously completed restore tail | Stop failed after restoring loopback fields but before completing BIST controls. Later HARD reloaded FIFO/isolation yet reused the completed-tail flag, reporting VERIFIED with FIFO `0000` and BMCR `1400`. | Every admitted HARD with a retained session rearms both tails. Twenty-seven late-frame-failure/cancellation cases across normal/analog/digital require saved FIFO, BMCR extras, IPG/packet and profile restoration. Independent reproduction now returns FIFO `0300`, BMCR `1000` and complete restoration. |

The independent lifecycle reproducers were recompiled against the corrected code;
both passed. Tests additionally cover six admitted 10-Mbit BIST sessions across
MII/RMII and normal/analog/external paths, with independent speed/selector literals,
acquisition and full restoration. The earlier BIST matrix covered these paths
at 100 Mbit/s only. The fake's vendor C22 aliases now use the same read semantics
as direct reads, instead of bypassing hardware-controlled status composition.

A proposed observation-spacing issue was checked separately: a failed acquisition
may already have captured/cleared the counter, and a later explicit acquisition
need not wait a new `observationUs`. The guide already anchored that spacing to
activation and complete acquisitions. Public comments now state this limit too.
No automatic retry occurs, and `intervalUncertain` remains true; the library
does not claim a full-duration count or PASS. This is not a proven silicon timing
defect or an exact-interval implementation.

## Fresh sources and boundaries

Reopened original SNLS647I PDF pages 41-42, 54, 68-69 and 72-73, including
tables 8-19/40/45/46. Direct original-PDF extraction confirmed the BISCR6
prerequisite. The current official [datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
remains SNLS647I, August 2025; the current [troubleshooting note](https://www.ti.com/lit/pdf/snla423)
remains SNLA423A, June 2025. Independent review confirmed mode fields, the exact
digital recipe, snapshot word `807D`, packet/IPG units, RH sync-loss semantics
and saturation/wrap distinction. Existing `loopback_recipe` and
`bist_result_semantics` gates remain justified. No analog `030B=3380` write or
invented counter-unlatch sequence was added.

Fresh clean read-only sibling reviews:

| Revision | Reopened paths and adaptation |
| --- | --- |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | `AGENTS.md`, operation result declarations, `src/PCA9555.cpp` cleanup paths, `test/test_basic.cpp::test_failed_cooperative_input_read_preserves_primary_and_cleanup_evidence` and registration. Retain primary and restoration evidence; automatic I2C pointer cleanup is not the PHY OP-9 model. |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `AGENTS.md`, `examples/common/Tmp1x2CliDiagnostics.cpp` finish/restore paths, `tools/test_hil_tmp1x2_runner.py` partial-write cancellation and retained-shutdown cases, CMake registration. Adopt exact-token and partial-result correlation; reject sensor timing and automatic recovery as PHY recipes. |

## Verification and maintained documentation

Windows, GNU C++ 15.1, Ninja, C++17 Release with strict warnings and active
CHECKs under NDEBUG. No second compiler was found on PATH.

| Layer / command | Result |
| --- | --- |
| `cmake -S . -B build/audit2324 -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`, build, `ctest --test-dir build/audit2324 --output-on-failure --no-tests=error` | PASS: 127/127, including 15 intentional Release failure probes |
| Same build in `build/audit2324-ubsan` with `-fsanitize=undefined -fsanitize-undefined-trap-on-error` | PASS: 127/127 |
| Independent reviewer `build/audit2324-tests` | PASS: 127/127 after integration; original baseline also passed, demonstrating missing edge cases |
| `cmake -S test/consumer -B build/audit2324-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF`, build, registered CTest | PASS: 1/1 |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS: 103 tests |
| `python tools/check_doxygen.py` | PASS: 175 pages and two rejected negative fixtures |
| `python tools/verify_ledger.py` | PASS: 97 registers, 52 procedures, 35 features; structural evidence |
| `python tools/verify_references.py` | PASS: archive hashes and local links |
| `python scripts/generate_version.py check`, `git diff --check` | PASS |
| SDK link, exported package, ASan/second compiler | NOT RUN in this portable correction |
| Remote CI | NOT RUN; first execution reserves it for prompt 71 |
| Physical mode/packet/counter/electrical qualification | NOT RUN; source gates and Q5/Q8/Q11 remain separate |

Updated public Doxygen, API/loopback/BIST guides, independent vectors, factual
ledger procedures, test guide, changelog, stage reports and progress. README,
architecture, variants and examples were reviewed: their interfaces, status and
ownership claims remain correct, so no unrelated rewrites were made. No workflow
exists yet and none was introduced. Final maintainability review kept fixes in
existing phases and removed the duplicate typed-only status accumulator.
