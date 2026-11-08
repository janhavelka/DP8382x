# Side-effect-aware register access

Stage 15 applies OP-14 at the same transaction boundary: raw writes reject
running work, retained results and cleanup barriers; raw reads/collector reject
reset WAIT. The historical `configurationDirty` latch is independent of the
current `isInitialized` result and is not cleared by verifying mandatory VOD.
See [initialization](initialization.md) for invalidation and restoration.

Stage 13 provides explicit destructive acquisition and narrow mixed-register
maintenance on the bounded transport. The [ledger](registers/dp83826i.json) remains
the complete field/access/encoding/reset catalogue. Named addresses and masks in
[CommandTable.h](../include/DP8382x/CommandTable.h) are checked against it; neither
a name nor a readable word authorizes a raw write. Unknown reset bits stay unknown.
The private policy table classifies every ledger address, including reserved,
selector, recipe-only and mandatory-follow-up entries. No register-manager layer
or caller-supplied transaction is exposed.

The application's serialized owner establishes the exact part/address, readiness,
maintenance admission and safe board wiring. Initialization, IRQ pin setup,
complete WoL configuration and diagnostic sessions remain their later typed
procedures. All participants share controller ownership and quarantine. Actual
MDIO timing and silicon behavior remain physically unqualified.

## Safe dumps and explicit acquisition

Ordinary `readClause22`/`readMmd` admit only safe-read policy entries. A safe dump
uses those routes and displays copied `evidenceSnapshot()` for destructive
sources. It never refreshes destructive state implicitly. Selectors, CDCR,
all-reserved entries and procedure-only addresses remain rejected; CDCR's rescal
follow-up makes it unsuitable for an ordinary dump despite having no RC fields.
Source-implied EEE reads are limited to the three D2 tuples in the transport guide.

`acquireEvidence(mask, output, budget)` is an explicit consuming operation. Eleven
sources are selectable. Their exact numeric masks and addresses remain in the
header/ledger; the acquisition order is:

| Source | Read effect and interpretation boundary |
| --- | --- |
| PHYSTS | Capture first when requested. Several mirrors clear only when another source is read. Its jabber bit is not cleared by reading PHYSTS. |
| BMSR | Retain latched-low and latched-high evidence; one sample is not a resolved link result. |
| ANER | Page-received clearing follows PHYSTS's description even though ANER is typed RH. |
| FLDS | Destructive fast-link-drop causes; raw masks do not prove current link state. |
| MISR1/2 | Reading consumes event bytes; low bytes are stable enables. MISR2 polarity/WoL stays raw without qualified routing state. |
| FCSCR | Consumed low-byte counter sample; saturation can hide additional events. |
| RECR | Consumed raw word; effective width, wrap and saturation remain source-gated. |
| RCSR | Raw RC status; reset polarity and rearm meaning remain gated. |
| 10BTSCR | Polarity history clears per description despite its R access label. |
| RXFS | Retains raw filter/WoL history and routing; reserved returned bits are not controls. |

PHYSTS mirrors depend on RECR, 10BTSCR, FCSCR, ANER, MISR and BMSR reads
(SNLS647I table 8-34). Collection orders requested sources accordingly, but does
not add unrequested destructive reads. It is not a simultaneous hardware snapshot.
The complete requested frame count is admitted before I/O, then batches of at
most eight frames share the original deadline and remaining count. Every RXFS
read includes its complete four-frame MMD selection. Any failure stops the call;
there are no retries or selector cleanup writes.

## Partial evidence and cached views

Every valid consumed read prefix is retained when its terminal batch returns,
before the next batch or public return. A later frame, release or deadline failure
does not erase it. The ordinary aggregate output is assigned only on complete
success. On error use the cached snapshot and its per-source validity/staleness;
`lastRequested` identifies the acquisition, not which sources succeeded.

Each source retains the latest exact word, sample count and unions of observed
ones and zeros. The zero union preserves latched-low history. FCSCR's low byte and
RECR's full raw words additionally contribute to `sampledSum`, with explicit
arithmetic saturation. These are sums of returned samples, not guaranteed physical
event totals: silicon saturation, missed history and RECR's unresolved effective
width remain visible limitations. In particular, repeated RECR reads do not create
a qualified counter width or modulo-delta algorithm.

Data validity and time validity are separate. A valid supplied frame timestamp is
used; otherwise the conservative post-callback upper bound is used. Invalid timing
metadata or a broken clock prevents a new valid timestamp without discarding a
separately valid word. A failed read with possible consumption, or an untrusted
completed read, sets sticky `historyLost`. A proven no-effect failed read only
marks the previous sample stale. Missing data never becomes zero events.

Cached readers perform no MDIO or clock callbacks. Raw semantic writes mark
`configurationDirty` and observations stale, even if the write later fails;
selector setup alone does not. `invalidateDeviceState` records external/raw
uncertainty without I/O and retains samples. No transport success clears dirty
configuration evidence. Future typed verification owns that transition. Binding
or local end resets local history; the owner copies needed evidence first.

A failed/rejected identity probe now blocks mixed maintenance before any I/O,
including WoL preharvest. Reprobe or explicit rebind is required to clear that
negative evidence; invalidation alone cannot. [Identity admission](identity.md)
describes the retained owner-attested exception before the first probe.

## Mixed-register maintenance

`modifyInterruptEnables` accepts only MISR1/2 low-byte masks. One two-frame owned
read/dependent-write retains the consumed high event byte even if the write fails.
It never echoes status bits. From sampled `A55A`, mask `0024`, replacement `0004`,
the literal write is `005E`; the captured event byte remains `A500`. This updates
enables only; it does not configure interrupt routing or implement IRQ service.

`modifyWakeControl` admits stable RXFCFG1 fields, excluding the undefined bit-mask
flag, reserved bits and direct access to the acknowledgement bit. A pulse-width
change owns the entire coded field. The owner excludes external configuration
writers for the call and quiesces traffic. Four frames compare stable state; an
unchanged unrelated request stops there. Otherwise five frames harvest MISR2 and
RXFS, then a five-frame atomic RMW updates RXFCFG1. Admission requires the worst-case
14-frame allowance. Each batch releases ownership before the next; the actual
read/modify/write remains indivisible. Failed preharvest prevents mutation.

SNLS647I table 8-90 says bit 11 is W0C. Unrelated writes put one there; explicit
acknowledgement puts zero there, including when stable fields are unchanged.
Other stable, undefined and reserved bits are preserved from the RMW read.
For sampled `1181`, neutral output is `1981` and acknowledgement is `1181`.
Changing pulse width with mask `0600`, value `0400` gives neutral `1D81` or
acknowledging `1581`. An event arriving after preharvest can still race an explicit
acknowledgement; this is not lossless hardware event storage. Q3 qualifies polarity
on the exact part; DP83822 write-one examples do not override it.

Pure `cmd::` compositions cover these stable/neutral rules and BMCR's command
suppression. BMCR power-down is supplied from owner intent, never echoed from the
power pin observation. Reset/restart commands remain distinct literal words for
their owning procedures. CDCR's self-clearing R/W trigger, BICSR1's lock-and-clear
action and description-only read effects remain named in the catalogue; they are
not generic W1C behavior. No W1C field is asserted for DP83826I.

Reserved handling is per register: LEDCR forces zero, whereas LEDCFG and CR2
preserve their reserved readback outside owned fields. Their independent poisoned
vectors are `FFFF/00F0/00A0 -> FFAF` and `FFFF/0020/0000 -> FFDF`.
LEDCFG LED1/LED3 masks and LEDCFG2 reserved fields remain protected by the catalogue
and literal tests. Recipe-only writes remain limited to their exact named sequence,
prerequisites and gates; no generic access is granted by their presence in the map.

`lastRequested` describes the latest admitted acquisition. `lastStatus` is the
latest acquisition or admitted mixed-update outcome, not per-sample validity;
a comparison-only WoL exit leaves it unchanged. Consult each sample explicitly.

Prompt 16 applies the persistent HARD quiet floor to every frame, including raw
reads, the collector and later operations. A timed call can sample the clock to
reject or expire the floor before MDIO. Cached access remains clock-silent; the
shared owner must also exclude other PHY addresses. See [initialization](initialization.md).
