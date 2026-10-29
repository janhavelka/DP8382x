# Prompt 9: portable values, tests and source documentation

Executed 2026-10-29 on `promptset/dp8382x-implementation-20261028` from
clean, fetched, equal checkpoint `7311a81962f45d5c3a0baf7a709b89603875acfe`.
The execution base remains `main` / `origin/main` at
`5e148fffde074b85fadc15d8ff66fe2c77111036`. The lead alone changed this
repository and owns Git; siblings and vendor material remained read-only. The
final synchronized commit is in the handoff to avoid a self-reference.

## Scope and decisions

Prompts 7–8 supply the accepted values, transport and operation contracts.
This stage implements only the passive subset in `include/DP8382x/Status.h`,
`include/DP8382x/Config.h` and `include/DP8382x/CommandTable.h`. The root
`CMakeLists.txt` exports an INTERFACE C++17
target with an explicit header list and tests OFF by default. No `src/*.cpp` is
needed for pure values. `test/foundation.cpp`, three isolated-header translation
units and `test/consumer/` execute host and separate consumer checks. `Doxyfile`
and `tools/check_doxygen.py` check actual public source documentation. Driver,
operation engine, SDK adapter, version generator, examples, hardware and package
support remain later-stage work.

The public functions `errorName`, `isValidVariant`, `isValidPhyAddress` and
`validateLinkSettings` perform no I/O and do not mutate inputs. Status keeps
static-lifetime messages and full signed backend detail. Link validation checks
all enum storage, nonempty legal AN technology masks, complete forced settings,
robust crossover and inactive advertisement. It does not claim source-gated
partner exchanges or silicon admission. Selected `cmd::REG_*`/`MASK_*` constants
describe wire values and access effects; they are not a raw-access permission.
The batch and binding structs are passive frozen contract values, with no
transport execution or successful hardware stub.

## Current source review

Paths below are relative to `C:/Users/Honza/Documents/Projects`. All three
working trees were clean at review. `rg --files` located the named headers,
source checks and tests before reading them; actual CMake registration was
checked separately.

| Source at exact revision | Observed path/symbol and exercising registration | Decision and destination |
| --- | --- | --- |
| TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `include/TMP1x2/{Status.h,Config.h,CommandTable.h}` provide error/value/table shape. `CMakeLists.txt` registers `test/variants/test_variants.cpp`, `test/helpers/test_helpers.cpp`, `test/core_no_arduino/compile_main.cpp` and `tools/test_check_doxygen.py`. `Doxyfile` and `tools/check_doxygen.py` generate pages; EXTRACT_ALL/page presence alone does not prove source comments. | **Adapt** status/pure validation and explicit source/test selection into the three DP8382x public headers and root CMake. **Reject** sensor/I2C meanings, unfiltered IDF sources and a page-only Doxygen gate. |
| TCA9548A `ce425e1984ac269e7934dd811a06ec08b91e307c` | `include/TCA9548A/{Status.h,Config.h,CommandTable.h}` are standalone. `test/core_no_arduino/CMakeLists.txt` registers `compile_main.cpp` as `core_consumer`; root CMake propagates C++17 to a linked target. | **Adopt/adapt** isolated portable compilation and propagated C++17 into `test/consumer/` and the `DP8382x` target. **Reject** mux-specific APIs and I2C semantics. |
| 24Cxx `936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e` | `include/EEPROM24Cxx/{Status.h,Config.h,CommandTable.h}` and `test/core_no_arduino/compile_main.cpp` exist. Root `CMakeLists.txt` registers helper/CLI/transport tests, but does not register that separate consumer; `.github/workflows/ci.yml` builds its package extraction. | **Adapt** the consumer intent into an actually registered tests-OFF consumer. **Reject** treating an unregistered source or package extraction alone as linked consumer evidence. |

The lead verified every cited path and registration. An independent semantic
review also checked the shipped register values against original SNLS647I
printed pages 54–58, 63 and 73 (tables 8-19–23, 8-31/32, 8-48). No copied
sensor behavior or unproved register value was accepted.

## Delegation and review

After the lead froze shared headers, a capable coding agent (`gpt-6.1-sol`,
medium effort) owned only root CMake and host/consumer tests, and a second
(`gpt-6.1-sol`, high effort) owned only Doxyfile/checker. A stronger read-only
reviewer (`gpt-6-astra`, high effort) checked Status, validators, comments and
original-PDF constants. These model choices matched coding versus silicon and
semantic risk; no agent changed Git, shared headers, progress or siblings.
The lead reviewed the code and corrected public comment precision before
verification. Each build used its own directory.
The verified code/test/tool checkpoint is
`079bf0f2f6451069de5296aee1b4e2dc811cea72`.

## Local evidence

Windows, MinGW-W64 GCC 15.1.0, CMake 4.0.1 with Ninja, Python 3.12.10 and
Doxygen 1.13.2 were available. Debug and Release native configure/build/CTest
each passed **4/4** registered suites and compiled all three headers alone.
The separate Release consumer configured the library with tests OFF, propagated
C++17 from a C++11 request and passed **1/1** CTest. Release ran 127,573 active
checks; a temporary `--verify-failure-path` invocation exited 1 as required.
`python tools/check_doxygen.py` passed 47 generated HTML pages, public XML
comment/member checks, local links and two expected Doxygen failures (missing
public documentation and invalid `@param`); generated output was removed.

Required research/reference checks passed: 27 archived artifacts, 25 derived
extracts and 1,312 local links in 152 Markdown files; 96 register records,
52 procedures and 35 features; Python unittest discovery **85/85**;
`git diff --check` passed.
No repository workflow exists to dispatch in this stage, and first-execution
remote CI belongs to prompt 71.

| Evidence layer | Result | Limit |
| --- | --- | --- |
| Host portable values/tests | PASS | Four CTest suites in Debug and Release; no PHY bus behavior. |
| Portable build-tree consumer | PASS | Separate tests-OFF application; no installed/exported package yet. |
| Source documentation | PASS | Doxygen source/pages/links and negative fixtures; no semantic hardware proof. |
| SDK link, remote CI, physical PHY | NOT RUN | Later prompts own these layers. |

README, CHANGELOG, API/architecture, public-location and test guides now name
the shipped subset and exact commands. `src/README.md` records why no empty
source file exists. The examples and native guides, variants, silicon ledger,
source map and feature coverage need no change because this stage adds no
device, variant, chip fact, example or SDK behavior. Version metadata remains
prompt 10. The final diff was reviewed for clear control flow and no unused
implementation abstractions.
