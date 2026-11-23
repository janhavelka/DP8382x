# Prompt 56 · DP83826E software qualification

## Phase (a) · Declared part and shared identity

Prompt 56 is **IN PROGRESS**. On 2026-11-23 the wrapper verified the prompt 55
completion at `f752c373b0ad3616c2a5e97fded47957bee151c4`. The worktree and
index were clean on `main`, 0 ahead and 0 behind `origin/main`. This slice
establishes the evidence for selecting a declared E and the identity tests it
needs. Public selection still admits only I.

### Shared silicon record

The original I and E share the register map and procedures in SNLS647I
(August 2025, section 8). The ordering-grade differences are operating
temperature and leakage limits keyed to ambient temperature. Pages 1 and 10
specify −40 to 85 °C for I and −40 to 105 °C for E. Page 14 gives the input
and tri-state leakage limits for those ranges. A review of the part-qualified
text found no I/E selection in the programming procedures. Both declarations
can use the original DP83826 map and common procedures. Reopen this decision
if TI identifies a suffix-specific programming condition.
[Archived SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf) ·
[Family comparison](../../reference/library-family.md#identity-and-a-generation-comparison)

| Source in SNLS647I | Consequence for a declared E |
| --- | --- |
| Tables 8-21 and 8-22, pp. 56–57 | Read Clause 22 registers 2 then 3. Accept `2000 A111` as BASIC and `2000 A131` as ENHANCED, both revision 1. Preserve the supplied suffix. Neither temperature grade nor selected board interface is observable from this pair. |
| Section 8.3.13 and p. 14 note (2) | The 10BASE-Te procedure requires `0x030E = 0x4A40`. Check the existing initialization and restoration order on E. |
| Table 8-23, p. 57 | The application must keep local ANAR bit 13 (`0x2000`) clear. Received partner/status indications remain observable. |

The archived PDF SHA-256 is
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
The pinned TI RTOS implementation checks OUI `0x080028`, model `0x11` or
`0x13`, and revision 1. The Linux constants name C (`0x2000A130`) and NC
(`0x2000A110`), with model values corresponding to ENHANCED and BASIC.
The declaration supplies the ordering grade in both cases. Revision 4 and
Strap11 EMC guidance belong to A-generation. The lead reopened the two pinned
source files to check the comparison.
[TI RTOS dp83826.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) ·
[TI Linux dp83822.c](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c#L22)

### Bind and identity acceptance

- Preserve the numeric values in `include/DP8382x/Config.h`. In
  `src/DP8382x.cpp`, replace the I-only revision list and fixed capability
  return with the bound profile. Resolve the candidate before callbacks and
  install it only after every bind guard succeeds. A rejected rebind must
  preserve the previous binding and pending result.
- Use [test/identity.cpp](../../../test/identity.cpp) for both declarations.
  Keep independent literal words for both modes, wrong OUI/model, revision
  0/4, zero/ones and contradictory required mode. Check the retained
  declaration for identical words on two differently bound instances. Check
  bounded identity-only reads on mismatch, ordinary output preservation on
  failure, and zero callbacks for invalid input and cached capabilities.
- The pure forced-link validator currently requires robust mode. The shared
  I/E preflight will own that requirement. Before applying the prompt's
  `(flags & 30U) == 0U` test expression, reconcile the encoding in
  [test/foundation.cpp](../../../test/foundation.cpp). The current forced
  vector uses bits 1, 2 and 3 for advertisement and bits 4 and 5 for the
  matched mirror controls. Decimal 30 also masks bit 4. Preserve the legal
  paired-mirror case and exercise robust both ways in the pure test.

### Sibling conventions checked at entry

Two read-only agents used the session model for independent source work, one
on the original TI document and one on live sibling conventions. Both siblings
were clean on `main` tracking `origin/main`. Their root AGENTS and CI workflows
were read. DP8382x has no tracked workflow at this checkpoint.

| Read-only sibling and revision | Source paths and decision |
| --- | --- |
| TMP1x2 `0791429f8fce1170e2082f1b011456fc461318b8` | `include/TMP1x2/Config.h`, `src/TMP1x2Configuration.cpp`, `test/variants/test_variants.cpp`, `examples/common/Tmp1x2CliOutput.cpp`, `tools/hil_tmp1x2_runner.py`, `CMakeLists.txt`. Preserve enum values, validate before I/O and test literal capabilities. The CLI separates the selected model from board pin availability. |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `include/MB85RC/CommandTable.h`, `src/MB85RC.cpp`, `test/test_basic.cpp`, `examples/01_basic_bringup_cli/main.cpp`, `tools/hil_runner.py`, `platformio.ini`. Adapt the single variant table and mismatch/cache tests. MB85RC selects by product ID. The PHY needs a declared suffix plus observed model and revision. |

### Prerequisites and stopping point

The [primary closure report](36-primary-feature-closure.md) returns deep-power
exit to prompt 28, pin-field interpretation to 32/33, and electrical controls
to 35. Those shared software gaps also limit complete E support. The
[ownership](53-regression-transaction-ownership.md) and
[lifecycle](54-regression-lifecycle-deadlines.md) suites and the
[primary matrix](55-regression-coverage-portability.md) supply regressions for
the implemented subset. The primary ledger assigns zero procedures to prompt
56. E physical qualification needs its own identified fixture.

Phase (a) remains open. Next, trace map-based access, robust preflight and VOD
through initialization, configure, reset/restore, diagnostics and SDK callers.
Map the CLI status and default scan bounds, native factory, example declaration,
runner identity and fixture evidence to named tests. Record the implementation
file ownership, Doxygen/guide changes, focused failure/deadline/cleanup cases
and required real SDK links before entering phase (b).

**Checks for this slice.** The existing Windows GNU 15.1.0 Release build passed
all ten selected DP83826I foundation and identity tests. Prompt 55's completion
commit changed only documentation since that build. The command below exercises
the baseline. E execution, portable/package consumers, SDK image link, remote CI
and physical qualification were **NOT RUN** in this slice. The ledger check
passed for 97 registers, 52 procedures and 35 features.

```powershell
ctest --test-dir build/p53-baseline -R '^(identity_(decode|policy|failures|lifecycle|ownership|scan)|foundation_(status|link|masks|passive))$' --output-on-failure --no-tests=error
python tools/verify_ledger.py
```
