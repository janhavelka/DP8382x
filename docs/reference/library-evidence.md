# Extracted public-library evidence

Reviewed on **2026-10-26**. These catalogues extract concrete register addresses,
field masks, encodings, procedures, timings and implementation defects from
public source, with TI's own repositories first. They are research inputs for
the [80-prompt implementation plan](../prompts/README.md); no driver or physical
qualification is implemented by these documents.

| Catalogue | Concrete content | Main prompts |
| --- | --- | --- |
| [TI core register and procedure evidence](library-ti-core.md) | DP83826 RTOS/public/private/generic definitions, identity, MDIO, reset, link, exact masks and rejected upstream behavior | 3, 11-22 |
| [TI feature evidence](library-ti-features.md) | MCU+ ICSS and TI Zephyr controls, interrupts, LED/GPIO, clocks, diagnostics, power and timing comparisons | 4-5, 23-36 |
| [Exact-family evidence](library-family.md) | TI RTOS/Linux and upstream Zephyr differences for E/A/DP83825I/DP83822, numeric identity/interface/wake/analog facts and conflicts | 56-67 |
| [Native SDK evidence](library-sdk.md) | ESP-IDF base/MAC register access, actual polling/default values, initialization hazards and ownership limits | 6, 37-42, 53-54 |

The numbered prompts include the relevant numeric starting points and failure
cases. The full tables live here so each prompt stays a manageable task rather
than repeating the entire catalogue. Follow [EVIDENCE](../prompts/EVIDENCE.md)
every time: reopen the actual sources and exact datasheets, correct errors and
keep unverified facts separate from measured behavior.

## Source scope and limits

TI dedicated Ethernet RTOS, MCU+ ICSS-EMAC, TI's combined Linux/Zephyr package,
the TI Linux mirror and upstream Zephyr are primary code comparisons. ESP-IDF
is authoritative for the pinned adapter implementation, not TI silicon behavior.
The [source index](source-code.md) and [public research catalog](public-research.md)
retain additional source and license context. Original TI datasheets and app
notes remain in the [reference archive](README.md), with unchanged hashes.

An additional GitHub search for DP83826 under TexasInstruments returned the
same RTOS material in released SDK trees. Two inspected examples are:

- [Processor-SDK-TDA private header](https://github.com/TexasInstruments/Processor-SDK-TDA/blob/77d805197ae37bcb5a0d86b707fa0a335076070a/mcu_plus_sdk/source/board/ethphy/enet/rtos_drivers/src/dp83826_priv.h),
  Git blob `37ab9b94d67687c569ac2e1e36b315fb90eb6749`: the same three-bit
  MLEDCR mask and LED1/LED3 field assignments appear at lines 153-196.
- [AM263Px SDK DP83826 implementation](https://github.com/TexasInstruments/mcu_plus_sdk_am263px/blob/dee0b767c4ebaa9b19d12b145af14046e3eee0a3/source/board/ethphy/enet/rtos_drivers/src/dp83826.c),
  Git blob `e0b64710350d764fe16ffd089fbdc5246e600039`: line 349 still passes
  the reset bit mask as a register address. Both files carry BSD-3-Clause notices.

These repeated defects are evidence of shared ancestry, not additional support
for the values. Exact datasheet comparisons and corrected vectors are in the
core/feature catalogues. Search also found generated AM263x/AM263Px API pages;
generated descriptions are not an independent silicon specification.

Targeted GitHub searches for DP83825 under nxp-mcuxpresso and DP83826 under
STMicroelectronics returned no matches in this pass. This is a bounded search
result, not a universal absence claim. No installer, undocumented binary tool,
entire vendor repository or GPL implementation was imported into the library.
The catalogues record factual observations and source pointers; actual reuse
still requires per-file license review and applicable notices.

## Reconcile into one implementation ledger

Prompts 3-5 and the exact-family source prompts must reconcile these dated
observations with the full datasheet inventory into the single implementation
ledger. Do not build a second competing production register map from a copied
header. Preserve code-observed values even when rejected, alongside the actual
datasheet value, disposition, independent literal test and source conflict.

Some complete-chip procedures are absent from all inspected drivers. Absence
from public code does not make a documented chip feature optional or unsupported.
Use exact TI programming evidence, and keep unresolved meanings or address
translations as explicit gates. No claim here proves electrical compliance,
EtherCAT timing, packet behavior or an untested part/revision.
