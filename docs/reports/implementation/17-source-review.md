# Prompt 17 source and procedure review

Reviewed on 2026-11-08 for `configure_an` and `next_page_exchange`. The source
review used gpt-6-astra at high reasoning for silicon sequencing and retained
evidence, selected under the repository delegation guidance. It preceded implementation of the configuration
translation unit; it is therefore not an independent review of that implementation.
The separate peer review supplies that layer. This report changes no vendor payload,
sibling repository, fixture, SDK baseline, workflow or Git history.

## Primary documents and fresh provenance

| Source | Reopened evidence and identity |
| --- | --- |
| [TI original DP83826I/E PDF](../../reference/source/TI-DP83826I-E-SNLS647I.pdf), [current official PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) | SNLS647I, August 2025. Archived and freshly downloaded bytes both total 3,366,322; SHA-256 `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`. Reopened original pages 25, 54–60 and the PHYSTS clearing description in table 8-34. |
| [IEEE original document mirror](https://www.my-wire.de/schnittstellenbeschreibung/ethernet.pdf) | IEEE 802.3-2012, Section Two; 20,249,231 bytes; SHA-256 `29774aba6d5b82524f4323214b1035603b0aa901c5f971d770049776b0a6c2af`, identical to the prompt-3 pin. Read printed pages 294–298, 303–305, 312, 320, 732–736; rendered and visually inspected Figure 28-18 on page 320. |

The IEEE URL is a nonpublisher mirror of an identified original document, not a
claim about the latest IEEE edition. Its copyrighted payload and page rendering
remain in system temporary storage. The report records factual conclusions and
locations without copying document passages. TI archive notices and hashes remain
unchanged; see [third-party notices](../../reference/THIRD_PARTY_NOTICES.md).

## Exact-part literals

These values were derived from original TI tables before inspecting the new
production implementation. Addresses below are Clause 22 unless explicitly stated.

| Source | Independent expected fact/vector |
| --- | --- |
| Table 8-23 | ANAR `04`; technologies `0100/0080/0040/0020`; selector mask `001F`, value `0001`; pause `0400/0800`; NP `8000`; prohibited local RF `2000`. |
| Table 8-23 | Ordinary advertisement: `(old & 5200) \| requested`; requested includes selector 1. Complete technology set `01E1`; 100/full `0101`; its pause variants `0501/0901/0D01`. Compare `ADFF`, including required NP/RF zero. |
| Table 8-19 | BMCR `00`; AN/restart `1000/0200`; reset `8000`; stable owned comparison `3100`. Configuration RMW preserves `4CFF`, writes `1200`, and compares expected `1000`; commands are not stable settings. |
| Tables 8-25/34 | ANER `06`; page `0002`, local NP `0004`, partner NP `0008`, partner AN `0001`. Table 8-34 establishes page clearing on ANER read despite table 8-25's RH notation. |
| Tables 8-26/27 | TX/RX `07/08`; writable TX `B7FF`; read-only preservation `4800`; payload `07FF`; message `2000`; ACK2 `1000`; toggle `0800`. Null composition `2001`, plus retained hardware toggle. |
| Table 8-20 | BMSR `01`, AN completion `0020`; restart-clear is not negotiation-complete. |

The four pause advertisements remain representable with half-only technology;
MAC pause resolution is separate. VOD_CFG3 setup belongs to initialization, so
configuration does not repeat the vendor-MMD `030E=4A40` write. Optional EEE/WoL
activation is outside this procedure.

## IEEE procedure findings

The following summarizes the pinned IEEE comparison contract, not measurements:

1. Restart clears the loaded-page indication. Preserve stale event history, restart,
   then distinguish the initial BASE event from later NEXT events. The first event
   authorizes ALNPAR, not ANLNPTR.
2. Bilateral next-page willingness permits the first post-BASE register-7 load.
   Register-7 writing supplies the next page; subsequent loads do not restart AN.
3. Retain each received page before another load. First peer toggle inverts base
   bit 11; subsequent toggles alternate. Thus asymmetric-pause bit 11 deliberately
   supplies the initial comparison baseline. Hardware owns transmitted toggle.
4. ACK2 expresses semantic compliance, separately from receipt. Exhausted local
   data requires null pages while the peer continues. Bilateral NP zero ends the
   exchange; negotiation completion remains separately observed.
5. Unformatted series follow their defining message immediately. Null identifies
   the end of partner information. Rejecting local null before further data is an
   admission inference from that meaning, not a separately printed prohibition.

Locations: 28.2.3.4, particularly .6/.7/.13; 28.2.4.1.6; Table 28-8;
Figure 28-18; Annexes 28B.4 and 28C.

Selected finite bounds, duplicate retention, error statuses, admission barriers,
NP restoration and fixed storage are project contracts. They do not assert an IEEE
maximum page count or a universal silicon completion deadline.

| Message code | Stage-17 admission and original Annex 28C scope |
| --- | --- |
| 1 | Null; no following unformatted payload. |
| 4 | One unformatted remote-fault-number page, values 0–3. This does not enable prohibited ANAR RF. |
| 5 / 6 | Four immediately following unformatted pages for OUI / PHY ID. User-defined residual bits are not rejected by invented constraints. |
| 2 / 3 / 7 / 8 / 10 | Defined/reserved-expansion or technology-specific messages outside the admitted stage-17 semantics; not relabeled as universally reserved. |
| 9 / 11 | Extended Next Page formats outside this 16-bit exchange API. |
| 0 / 12–2047 | Reserved in the pinned 2012 allocation. |

Clause 28C.9 specifies two following unformatted pages for code 7; do not infer
a one-page grammar from the abbreviated allocation-table description.

## Current public implementation comparison

GitHub API default-branch HEAD checks on the review date returned these unchanged
pins. A repository HEAD check alone is not a review of every file at that commit.

| Repository and pin | Files actually reopened and disposition |
| --- | --- |
| [TI RTOS](https://github.com/TexasInstruments/ethernet-rtos-drivers/tree/94bf4343004732983b0b8e1eec123d88ff70453b), `94bf4343004732983b0b8e1eec123d88ff70453b` | `rtos_drivers/src/generic_phy.c`, `dp83826.c`, `dp83826_priv.h`, `phy_common_priv.h`, `rtos_drivers/include/generic_phy.h`; file headers are BSD-3-Clause. `GenericPhy_setAdvertisement` lines 515–641 accepts empty input and replaces selector bit 0 alone. `GenericPhy_ctrlAutoNegotiation` lines 885–946 uses generic RMW; completion helpers separately inspect restart and BMSR. No complete host next-page service was established. |
| [TI combined software](https://github.com/TexasInstruments/ti-ethernet-software/tree/450af1144b8caeb58724bcbae081b409f6bf5c3a), `450af1144b8caeb58724bcbae081b409f6bf5c3a` | Reopened `zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c`, Apache-2.0. `phy_ti_dp83826x_cfg_link` lines 584–651 resets/configures/renegotiates under an unbounded mutex and rejects AN-disabled requests. These platform policies do not define the portable procedure. |
| [TI MCU+](https://github.com/TexasInstruments/mcupsdk-core/tree/4b9a66f4e18e02a834961f5f834155c846cab041), `4b9a66f4e18e02a834961f5f834155c846cab041` | HEAD reconfirmed only. Prior file-level findings remain attributed to their original reports; no new implementation or licensing claim is inferred from this check. |

No vendor implementation was copied. GPL comparisons remain factual references;
this task imports no GPL source. Existing ESP-IDF baseline selection is unchanged.

## Search limits, gates and validation

Reopened the [current TI product listing](https://www.ti.com/product/DP83826I).
Targeted searches included `site.e2e.ti.com DP83826 next page ANNPTR` and
`site.ti.com DP83826 next page errata`. They yielded no applicable exact-part
next-page clarification or separate erratum resolving physical event/load timing.
This bounded search does not establish absence of all errata or support material.

`next_page_handshake` / Q10 remains physical qualification. Source-supported
software can implement the designated Clause 28 protocol now. Remove stale ledger
language that makes Q10 a software gate or replays persistent NP=1: transient
exchange intent must not survive reset/restoration. Failure retains consumed data,
possible NP state and a separately admitted restoration obligation.

The author subsequently implemented the configuration translation unit, then
released it to the lead and independent peer reviewer. Author checks used Windows,
GCC 15.1.0, Ninja and CMake Release in `build/an17-sources`: compilation with the
repository's strict warnings passed; the then-current 57 registered CTests passed.
Those results establish host behavior only and precede final integration checks.
SDK link, remote CI and physical evidence were NOT RUN by this reviewer. No
fixture action or remote CI query occurred.
