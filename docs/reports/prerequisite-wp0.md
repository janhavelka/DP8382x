# Prompt-13 prerequisite WP0: evidence and ledger ownership

Applied 2026-11-07 on `promptset/dp8382x-implementation-20261028` from the
clean synchronized entry `81ab2d3`. The lead owns integration and Git. This
package changes research provenance, procedure ownership and planning text;
implementation-stage completion remains unchanged.

## Original-source checks

Downloaded the original TI bytes and verified both hashes against the audit:

| Source | Bytes / pages | SHA-256 |
| --- | --- | --- |
| [SNLA428](../reference/source/TI-DP83826-TDR-SNLA428.pdf), June 2023 | 343,135 / 11 | `b262302866cc3e8abb401f435c6935368192500b4d5fbc27faeca21c75799d33` |
| [SNLS603D](../reference/source/TI-DP83TG720R-Q1-SNLS603D.pdf), April 2025 | 3,302,917 / 186 | `a13ecacc0d2d28c404c4e719fc76d665a4472723fceafbe7b5e5a49baecfb627` |

The provenance manifest records requested/resolved URLs, retrieval UTC, original
notices, byte counts, revision dates, page counts, extraction tool and extract
hashes. Existing vendor payloads were not changed. The archive now contains
27 PDFs and two ZIPs. Both new source IDs are available in the ledger.

Reopened original SNLS647I pages 40, 48, 54 and 73: the 10BASE-Te initialization
word, strap sampling, reset commands and rescal-clear/hard-reset contingency.
Unconditional initialization is the audit's explicit library policy; it is not
presented as a datasheet requirement for an exclusively 100-Mbit application.
Original SNLA428 page 3 supplies segment 1A/1B and gain recipe evidence for the
later coordinated TDR corrections. SNLS603D page 47 explicitly describes its
own MMD address-prefix notation; its MMD nibble differs from DP83826 notation,
so this remains cross-part analogy rather than proof of DP83826 access.

## Corrections

Corrected owner stages and matching test/gate text for interrupt configuration
and test (27), cable autorun (26), TDR calibration (25), resistor calibration (35),
link recovery (34), raw RECR/RMII observations (20), RXFS SFD reference (31), and
FLD defaults (34). The feature matrix now agrees. Physical cases Q7/Q16/Q17/Q18
remain unexecuted requirements.

Updated 14 generic procedure budgets to independent per-target accounting:
snapshot/read-modify-write/verify costs 13 Clause-22 frames for an MMD target;
direct read/write/verify costs three. Retained analog configuration's separately
counted 68-frame primary and 56-frame restoration paths. Removed obsolete
duration member names and changed `recoveryRequired` to `cleanupOwed`; the
operation-contract package owns the broader restoration-state rewrite.

Corrected the calibration source citation to SNLS647I tables 8-47/48 and section
8.4.1. Initialization now repeats and verifies VOD_CFG3 for every reset kind.
BMCR/PHYRCR reset and restart command steps use literal whole words. Manual cable
admission rejects enabled autorun or rescal before cable writes, with the later
autorun/recovery owners explicit. Loopback vectors now say they are composed
from table 8-19, with full duplex chosen by the test, rather than printed examples.

The EEE tuple/source-gate and SNLA428 recipe adoption changes require their
coordinated WP3 ledger, vector and validator updates; archival alone does not
resolve those gates. The lead handles COMMON authority/read-map, shared rule
guides and prompt path corrections outside this package's writable paths.

## Fresh sibling review for prompt 13

Both references were clean when read; no sibling files were edited.

| Reference | Revision | Reviewed paths |
| --- | --- | --- |
| PCA9555 | `414e0f66b13082e974ec27a4a6e86985a65d83fd` | `src/PCA9555.cpp` transport/cleanup wrappers (149-268), shadow invalidation (267-310), input capture/park (459-489); `test/test_basic.cpp` ambiguous-write, observation-versus-shadow and primary/cleanup cases (1124, 1439, 2044, 2120, 2194) and registration; `examples/01_basic_bringup_cli/main.cpp` cached settings (485) and live dump (534). |
| INA228 | `73f954838d3fac8cce747d54d0ad738940d4c625` | `include/INA228/INA228.h` DiagAlert/snapshot (110-145) and destructive APIs (896-919); `src/INA228.cpp` capture (2886-2931), sanitized configuration (2932-2956), live API (2478-2500), cached snapshot (1624); `test/test_basic.cpp` destructive preservation (2034, 2067), consume-then-fail (3662), partial diagnosis (4485), registered cases; CLI cached (920), destructive (986) and raw diagnostic (2348) rendering. |

Adopt one capture path, immediate preservation of consumed evidence, cached
display without I/O, and observation separate from configuration trust. Adopt
primary failure retention: successful protocol cleanup must not erase failure
or make health successful. Adapt to MDIO frame/transaction evidence and explicit
per-source history loss; INA228's indeterminate job effect alone is insufficient
for the prompt-13 contract. Reject I2C byte/ACK counting, pointer-park writes and
sensor-specific status polarity as PHY evidence. INA228's runner includes live
`diagraw` in smoke coverage; do not treat that as permission for a safe PHY dump.

Read both sibling AGENTS files, `.github/workflows/ci.yml`, and
`tools/run_i2c_hil.py`. The runners use finite monotonic sessions. PCA pins
PlatformIO 6.1.19 / ESP-IDF 5.4; INA pins PlatformIO 6.1.19 / ESP-IDF 6.0.1.
Those are sibling observations, not this library's SDK baseline. No sibling
build, remote workflow, physical fixture or hardware qualification was run.

## Verification and delegation

The lead delegated this read/reconciliation package to the current capable
coding model; original source interpretation and existing records were inspected
before editing. Separate stronger silicon review remains with the lead's source
reviewer. No code was copied from vendor or sibling implementations.

Local checks PASS: ledger validator (96 registers, 52 procedures, 35 features),
all 77 ledger Python tests, reference integrity (29 artifacts, 46,886,437 bytes,
27 extracts), and diff whitespace. Link counts are recorded by the lead after
concurrent documentation integration. C++ host, consumer, version and Doxygen
checks are the lead's complete package verification. SDK/package, remote CI and
physical evidence remain separate; remote CI is reserved for prompt 71.

Lead integration on 2026-11-07 also passed the current Release host suites,
tests-OFF linked consumer, Doxygen negative fixtures and version check. The full
Python discovery passed 97 tests after the concurrent WP2 timing-probe update.
Stage implementation status remains unchanged by this evidence/plan checkpoint.
