# Prompt 20 independent source review

Reviewed on 2026-11-10 for `collect_event_counters`. Entry checkpoint
`0d176b555c7fb35c1836041d9b7499ac5e9d5304`, execution branch
`promptset/dp8382x-implementation-20261028`. The lead owns implementation,
shared contracts, current sibling audit, integration and Git. This reviewer owns
only this report. The lead selected gpt-6-astra/high for independent silicon and
destructive-evidence reasoning. No fixture, SDK or remote CI was operated.

Read parent/root AGENTS, README, architecture, variants, reference index,
COMMON, prompt 20, EVIDENCE, relevant ENGINEERING/CONVENTIONS, API contracts,
device behavior, the collector ledger and stage-19 source review. Reopened
the entry collector declarations and retention implementation. This report
states source facts and recommended acceptance cases; it is not an assertion
that an implementing test suite or physical case has passed.

## Fresh original sources

Downloaded the [current official TI datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
and compared its bytes with the [preserved original](../../reference/source/TI-DP83826I-E-SNLS647I.pdf).
It remains SNLS647I, August 2025, 3,366,322 bytes, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`;
the bytes are identical. Rendered and visually inspected original printed
pages 59, 63, 64, 66-70, 72 and 92, including tables 8-25, 8-33, 8-34,
8-36 through 8-39, 8-41, 8-44 and 8-91. Renderings are temporary files
outside the repository; vendor payloads and provenance are unchanged.

Rechecked the [TI product documentation](https://www.ti.com/product/DP83826I)
and bounded TI/E2E searches for RECR saturation, RCSR overflow/underflow and
DP83826I errata. No applicable clarification resolving those two conflicts
was identified. Search absence does not establish absence of unpublished
errata. No later-family table or macro was substituted for exact-part evidence.

The GitHub API reports current TI RTOS main unchanged at
`94bf4343004732983b0b8e1eec123d88ff70453b`. Re-fetched these original files:

| File at that revision | Reviewed scope | SHA-256 |
| --- | --- | --- |
| [dp83826.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c) | `Dp83826_printRegs`, lines 255-288 | `600732cd0e3e197cb9e01f26761e2574b9527142ecee34e95361511e153df5ad` |
| [dp83826_priv.h](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826_priv.h) | Register/mask definitions, lines 58-127 | `be7802418e285b50de8420953d3c9a9923acb0c489a505a799e61da9fe22764f` |
| [generic_phy.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c) | `GenericPhy_printRegs`, lines 302-314 | `61fc91d936a008ec40237c06e1f9681cfda8e463de1e53df2698bfde2e0be447` |

The source retains BSD-3-Clause file notices. No implementation was copied.
The chip printer ignores read errors and reads generic `GIGESR`, which occupies
the exact-part FLDS address. The generic printer scans through that address
and other destructive words without retaining their effects. Neither printer
is a safe dump or collector design, and neither resolves RECR/RCSR semantics.

## Independent literal facts

All short register addresses below are Clause 22. RXFS is vendor MMD DEVAD
`0x1F`, wire offset `0x04A1`, using the established indirect transaction.
Masks are derived from the original bit columns, not imported macros.

| Literal event bit | MISR1 `0x12`, table 8-36 | MISR2 `0x13`, table 8-37 |
| --- | --- | --- |
| `0x8000` | Link-quality change while link is on | EEE error |
| `0x4000` | Energy-detection change | Autonegotiation error |
| `0x2000` | Link-status change | Page received |
| `0x1000` | Speed change | Loopback FIFO overflow/underflow event |
| `0x0800` | Duplex change | MDI crossover change |
| `0x0400` | Autonegotiation completion event | Sleep-mode event |
| `0x0200` | False-carrier counter exceeds half-full | Polarity or WoL event, selector-dependent |
| `0x0100` | Receive-error counter exceeds half-full | Jabber event |

Both event bytes clear on read. Both low bytes, mask `0x00FF`, are enables.
An event is history, not the current link, speed, duplex, sleep or polarity
state. Repeated physical occurrences can coalesce in a single bit before any
software read; even a single returned one does not establish multiplicity.
MISR2 FIFO history does not resolve RCSR fault polarity.

FLDS `0x0F`, table 8-33, has read-clear cause mask `0x01F0`: energy loss
`0x0010`, SNR `0x0020`, MLT3 errors `0x0040`, RX errors `0x0080`, descrambler
sync loss `0x0100`. These are enabled detector causes that produced link loss,
not present link quality. Reserved mask is `0xFE0F`. Physical Fast Link Drop
qualification remains mode-dependent; the existing MII-only boundary is retained.

FCSCR `0x14`, table 8-38, is unambiguously an eight-bit read-clear counter.
The counter stops at `0xFF`; the interrupt threshold is exceeded after `0x7F`.
Thus `0x007F` is below the first crossing value `0x0080`, `0x00FE` is an
unsaturated sample and `0x00FF` is a lower bound of 255. Reserved upper bits
do not contribute to arithmetic. No modulo-wrap behavior is documented here.
For example, successfully consumed samples `0x00FF`, then `0x0002`, establish
at least 257 false-carrier events, not an exact lifetime total of 257.

RECR `0x15`, table 8-39, prints field 15:0 and describes a 16-bit counter,
but the same original table says counting stops at `0xFF` and interrupts above
`0x7F`. The count applies to invalid data symbols with valid carrier/RXDV and
excludes MII loopback. Preserve all 16 returned bits and the conflict. Do not
truncate to eight bits, infer 65535 saturation, claim wrap, or publish a
qualified accumulated receive-error total. The entry implementation's raw-word
`sampledSum` is arithmetic on raw observations, not source-qualified counter
semantics; stage 20 should keep RECR arithmetic explicitly unavailable.

RCSR `0x17`, table 8-41, gives bits 3/2 (`0x0008`/`0x0004`) RC access and
reset zero, but describes zero as overflow/underflow and one as normal.
Retain raw `0x0000/0x0004/0x0008/0x000C`; neither positive nor inverted Boolean
fault decoding is sufficiently established. Do not reinterpret a read-cleared
zero as a newly detected fault or assume the next read rearms a status latch.

## Mirror order and mixed access

When mirrored history is requested, collect PHYSTS `0x10` before its clearers.
Original table 8-34 gives the dependencies explicitly:

| PHYSTS mask | Clearing access |
| --- | --- |
| `0x2000`, receive-error history | RECR `0x15` read |
| `0x1000`, polarity history | 10BTSCR `0x1A` read |
| `0x0800`, false-carrier history | FCSCR `0x14` read |
| `0x0100`, page history | ANER `0x06` read |
| `0x0080`, pending interrupt | MISR read; prose names `0x12` |
| `0x0040`, remote fault | BMSR `0x01` read, or reset |

Do not generalize the PHYSTS RC column to all mirrors clearing on its own read.
Its signal/descrambler bits `0x0600` remain raw RC evidence, with no universal
quality verdict. Jabber bit `0x0020` explicitly does not clear on PHYSTS read.

ANER table 8-25 labels page bit `0x0002` RH, while PHYSTS explicitly says its
page mirror clears on ANER access. 10BTSCR table 8-44 labels polarity bit
`0x0010` R, while its prose explicitly clears both copies on a 10BTSCR read.
Therefore access prose takes precedence over an assumed harmless R/RH read.
Every 10BTSCR RMW/readback must retain the consumed word. Next-page exchange
keeps its ANER ownership; a public collection cannot steal that history.

RXFS table 8-91 prints selector `0x1000`, reset one: zero selects polarity,
one WoL; enabling WoL automatically selects WoL. Named RCH status bits are
SFD error `0x0080`, bad CRC `0x0040`, invalid Secure-On password `0x0020`,
and magic-packet status `0x0001`. Status bits 4:1 remain reserved even though
their access cells also say RCH. The SFD description points to RXFCFG1 bit13,
which that register calls reserved: retain the existing `rxfs_sfd_reference`
gate and avoid inventing an enabling/configuration meaning.

MISR2 `0x0200` can be given a specific label only when owned selector state
was known stable during the pending event interval. A later RXFS sample alone
does not prove what generated older coalesced events. Keep the shared raw label
by default; no extra destructive RXFS read solely for decoding. Reset defaults
describe reset silicon, not arbitrary bind-time ownership. Reset or uncertain
WoL mutation invalidates prior selection trust. Prompt 27 owns source changes
and the documented whole-word selector writes; stage 20 must not add a generic
RXFS RMW or transplant DP83822 W1S semantics.

## Minimal typed publication and acceptance advice

Extend the existing retention path, with fixed-size values and no new reader
of destructive addresses. Per-source values need raw/latest validity, stale
state, completion timestamp validity, epoch/sequence, retained occurrence
masks and sticky possible-loss state. Keep complete aggregate publication
separate from retained prefixes. An ordinary aggregate output remains unchanged
on any acquisition failure, while a separately cached partial view can expose
the successfully consumed words. Timestamp validity is independent of raw
data validity; a clock problem must not erase a valid returned word.

FCSCR needs distinct hardware-saturation and software-overflow evidence.
The former becomes sticky at a sample of `0x00FF`; the latter saturates the
bounded host accumulator. Either makes the result a lower bound. Unknown
consumption creates a history gap even if later samples are zero. A sum may
be exact for collected intervals only when all their boundaries and samples
are known; it is never automatically the chip's lifetime count.

Software acknowledgement clears only retained host history and performs no
MDIO; it cannot clear pending silicon events/counters. Reset/restart starts a
new observation epoch or leaves old evidence tagged with its old epoch and
stale validity. Do not silently merge old samples into a new exact interval.
ORed history is intentionally bounded/coalesced; document that neither the
source latches nor host masks preserve event multiplicity. Sequence/sum
overflow must be visible rather than wrapping silently.

Use the literal rows above in separate bit tests and add: simultaneous events;
a new MISR1 event after its read but before MISR2; repeated zero/nonzero reads;
MISR1 success followed by MISR2 failure; failure at every batch/frame boundary;
target-read timeout after effect; selector failure before target access;
late-success retained words; invalid timestamps; reset epoch separation;
two cached readers and acknowledgement with zero transport calls; active ANER
exchange rejection; and MMD transaction exclusion. No finite set of reads is
an atomic silicon snapshot, and no extra drain loop is needed to claim one.

## Gates and evidence layers

The source audit is PASS for identifying exact documented masks, dependencies
and conflicts. Original current PDF comparison is PASS. No source contradiction
was resolved by assertion or a mock.
`python tools/verify_references.py` passed after this report was added: 29 vendor
artifacts, 27 text extracts and 1,500 local links in 188 Markdown files.

- `recr_width`: BLOCKED for derived width/saturation/arithmetic. Q16 needs an
  exact-part correction or identified controlled invalid-symbol/RXDV injection
  around `0x007F/0x0080`, `0x00FF/0x0100`, `0x7FFF/0x8000/0xFFFF`, including
  read-clear and IRQ traces. Raw acquisition is independently justified.
- `rmii_rc`: BLOCKED for derived fault polarity/rearm. Q17 needs identified
  controlled RMII clock-offset/fault traces before/after repeated reads and
  reset. Raw acquisition is independently justified.
- `rxfs_sfd_reference`: remains a source conflict for SFD configuration meaning;
  the raw returned bit is available without resolving it.
- Host implementation, portable/package consumer and Doxygen checks: lead's
  integration responsibility; no pass claimed by this source-only review.
- SDK link, remote CI and physical cases Q16/Q17: NOT RUN here. First-execution
  remote CI remains prompt 71; physical stimulus requires separately authorized
  fixtures and is not replaced by this audit.

No production/shared/sibling file was edited by this reviewer. Commit/sync and
the final implementation status belong to the lead's stage handoff.

## Subsequent implementation review

Read the implemented `CounterSnapshot`, `collectEvents`, `clearEvidence`,
`_markEvidenceDiscontinuous`, source event-mask projection and FCS-only sum,
along with `test/events.cpp` and the shared receipt/clock handling. No confirmed
silicon-mask, destructive-order, output-on-error or timestamp arithmetic defect
was identified in that review. The six-read order is PHYSTS, FLDS, MISR1, MISR2,
FCSCR, RECR in one existing callback. It does not introduce an RXFS read or
another collector. RXFS history mask `0x00E1` excludes source selection and
reserved fields. RECR arithmetic now remains zero; FCSCR hardware saturation
and host overflow have distinct sticky flags.

The new test source includes each MISR/FLDS literal bit, first threshold and
hardware saturation boundaries, each failed read phase, possible versus absent
read effects, partial prefixes, primary versus release failure, late completion,
invalid/backward timestamps, new events between reads, cached readers and
controller exclusion. Review of those test cases is not an independent run.
The reviewed version did not directly exercise the UINT64 host-sum overflow or
UINT32 sample-count saturation branches; this coverage gap was sent to the lead.
The lead subsequently factored the existing bounded arithmetic into private
`src/DP8382xEvidence.h` and added literal limit tests against those production
helpers. This resolves the gap without a driver-state injection or public test API;
the executing stage report records the resulting checks.

Reset history uses sticky `discontinuous` plus FCS lower-bound state, not an
allocated per-epoch archive. New latest samples can become fresh while the
retained OR and sums still span that discontinuity. Documentation/ledger must
describe that distinction and must not promise attribution of retained event
bits to individual reset epochs. `clearEvidence` is a bus-silent explicit host
history discard, preserves other domains, and correctly documents that the
next hardware sample may contain events from before the discard.
