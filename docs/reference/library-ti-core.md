# TI RTOS DP83826 library: concrete core evidence

This is a planning and review catalogue, not implemented driver support. Rechecked
2026-10-26 against the official TI repository and archived SNLS647I. Values marked
**library** describe that particular implementation; they are not automatically
silicon requirements. **Datasheet** facts refer to the exact archived revision.
**Conflict** requires an explicit disposition before the affected implementation.
Use this with the [full feature inventory](dp83826i-feature-inventory.md), not as a
replacement: the TI library implements only a small subset of the documented PHY.

The prompt-3 [ledger](../registers/dp83826i.json) now records current core
decisions, including separately gated software-hard-reset strap resampling,
PHYSTS forced-mode interpretation and CR3 defaults. The fresh prompt-3 audit
corrected VOD_CFG3's upper termination field to R/W as table 8-81 specifies;
the previously asserted read-only conflict was erroneous. This catalogue retains source-code defect details;
it is not a second implementation register map. See the
[fresh source audit](../reports/implementation/3-source-audit.md).

## Immutable sources and review bounds

The live official `main` resolved to commit
`94bf4343004732983b0b8e1eec123d88ff70453b` (2026-07-29T22:40:01Z).
The GitHub connector returned these exact files and Git blob identities:

| Key | Pinned file | Git blob SHA-1 |
| --- | --- | --- |
| C | [rtos_drivers/src/dp83826.c][c] | `e0b64710350d764fe16ffd089fbdc5246e600039` |
| P | [rtos_drivers/src/dp83826_priv.h][p] | `37ab9b94d67687c569ac2e1e36b315fb90eb6749` |
| H | [rtos_drivers/include/dp83826.h][h] | `3124a65530b9c549ab50eef9c47e8fc8ca852cd4` |
| G | [rtos_drivers/src/generic_phy.c][g] | `39414bd3b1004cdf6075bf0f4be30180babdc84d` |
| GH | [rtos_drivers/include/generic_phy.h][gh] | `156c51ba77aaede5b6e096bb8f5a6050620b2f4a` |

Line numbers below are one-based in those immutable files. All five carry the
TI BSD-3-Clause notice at lines 1-31; see also the [root license][license]. This
catalogue records facts and original analysis, not imported driver source. Any
future reuse must retain the applicable copyright, conditions and disclaimer.
The public header's empty configuration and LED enum do not establish working
LED APIs, C++ portability, tested feature completeness or native ESP-IDF support.

The [original DP83826I/E PDF](source/TI-DP83826I-E-SNLS647I.pdf) was reopened for
the tables cited below. Page numbers are printed PDF page numbers. Rendered pages
15, 16, 74, 87 and 88 were visually inspected to check timing columns, LED mask width
and LED numbering. Other cited fields were read from the original PDF text and
compared with the archived searchable extract. Preserve the original PDF and
[manifest](source/provenance.json); these source pins are not additions to the
vendor payload archive. No hardware measurement or SDK build was performed.

## Identity and capability facts

Prompt 7 re-opened the original C/P/G files on 2026-10-29; all three blob IDs
above matched and official main still resolved to the same commit. A bounded
five-commit path-history check found no newer main revision for C/G. Original
SNLS647I pages 56/57/73 were visually rechecked. The
[API contract](../api-contracts.md) now fixes staged identity/link outputs,
valid link-down success and explicit revision-1 source admission, separately
from raw revision defaults and physical qualification. A low first BMSR sample
is retained even if the second fails. The reset-address defect still requires
rejecting mask 0x8000 as a C22 address before callbacks. Vendor payloads and
numeric ledger definitions remain unchanged.

| Source | Concrete value or behavior | Interpretation for this project |
| --- | --- | --- |
| C 51-54, 147-160 | OUI `0x080028`; models `0x11` or `0x13`; revision exactly `0x01` accepted | Library filter, not proof that all other revisions are absent or unsupported |
| GH 135-144 | ID1 OUI shift 6; ID2 OUI mask `0xFC00`, shift 10; model mask `0x03F0`, shift 4; revision mask `0x000F`, shift 0 | Decode `OUI=(ID1<<6)\|((ID2&0xFC00)>>10)`; retain raw words |
| SNLS647I tables 8-21/22, pp 56-57 | ID1 `0x2000`; ID2 model `0x11` BASIC, `0x13` ENHANCED; revision field default 1 | Derived revision-1 combined IDs `0x2000A111` and `0x2000A131`; these are mode identifiers, not I/E ordering-suffix identifiers |
| C 163-183 | MII and RMII accepted; RGMII rejected | Rejection agrees with original-DP83826 interfaces; the adjacent comment claiming PHY RGMII support is wrong for this datasheet |
| G 439-513 | Read BMSR and OR four 10/100 capability flags into caller storage; optionally reads generic gigabit status | Caller output is not initialized here; do not copy this as a complete staged-result contract |
| GH 124-133; SNLS647I table 8-20 | BMSR capabilities: 100/full `0x4000`, 100/half `0x2000`, 10/full `0x1000`, 10/half `0x0800` | All four remain generic library capabilities independent of first-board 100/full policy |

`GenericPhy_getId` (G 316-360) reads `0x02`, assigns the upper half of `*pId`, then
reads `0x03` and ORs the lower half. A second-read failure leaves partial output.
The future driver must stage both words and publish identity only on complete
success, preserving the backend error and previous caller output on failure.

## Register addresses and useful common masks

Addresses here are Clause 22 register numbers unless explicitly called vendor
offsets. P 58-101 supplies every DP83826 address named by that private header.
GH 69-111 supplies the generic addresses. The datasheet permits indirect vendor
access to the ordinary registers except REGCR/ADDAR themselves (8.3.11.1 p 35).

| Address | Library name / purpose | Exact-part disposition |
| --- | --- | --- |
| `0x00` | BMCR | Control with W1S commands, not unrestricted RMW |
| `0x01` | BMSR | Capability/status read consumes latched evidence |
| `0x02`, `0x03` | PHYIDR1, PHYIDR2 | Raw identity words |
| `0x04`, `0x05` | ANAR, ANLPAR | Advertisement and partner ability; datasheet spells the latter ALNPAR |
| `0x06` | ANER | Negotiation expansion and consumed page evidence |
| `0x07`, `0x08` | ANNPTR, ANNPRR | Next-page TX/RX; exact datasheet RX name ANLNPTR |
| `0x09`, `0x0A`, `0x0B` | DP83826_CR1, CR2, CR3 | Do not substitute GH's gigabit names at `0x09/0x0A` |
| `0x0D`, `0x0E` | PHY_MMD_CR/DR = REGCR/ADDAR | Indirect selector/data registers; direct access only |
| `0x0F` | GH calls this GIGESR | DP83826 actually defines FLDS; gigabit interpretation is invalid and reading consumes FLD causes |
| `0x10` | PHYSTS | Link/mode plus status fields with source-dependent clearing |
| `0x17`, `0x18`, `0x19`, `0x1F` | RCSR, LEDCR, PHYCR, PHYRCR | Mixed status/control, LED, crossover, and reset/restart respectively |
| vendor `0x0025` | MLEDCR | LED0 routing; exceeds five-bit Clause 22 register range |
| vendor `0x0303`-`0x0306` | LED0/1/2/3_GPIO_CFG | Per-pin mux restrictions differ; a shared mask is not a shared writable contract |
| vendor `0x0460`, `0x0469` | LEDCFG, LEDCFG2 | Header has incorrect LED associations and reserved-field definitions; see below |

Common source values (GH 113-186), independently interpreted against SNLS647I
tables 8-19 through 8-27 and 8-31:

| Register/group | Source bit masks/encodings | Bounds and omissions |
| --- | --- | --- |
| BMCR | reset `0x8000`; loopback `0x4000`; 100 Mbit `0x2000`; AN enable `0x1000`; power-down `0x0800`; isolate `0x0400`; AN restart `0x0200`; full duplex `0x0100` | Reset and AN restart are W1S/self-clearing. Source's SPEED1000=`0x0040` is reserved on this PHY; reject it |
| BMSR | capabilities `0x7800`; AN complete `0x0020`; AN capable `0x0008`; link `0x0004`; extended capability `0x0001` | Source's GIGEXTSTS=`0x0100` is reserved on DP83826. Datasheet also supplies preamble suppression `0x0040`, remote fault `0x0010`, jabber `0x0002` |
| ANAR and ANLPAR | 100/full `0x0100`; 100/half `0x0080`; 10/full `0x0040`; 10/half `0x0020`; 100 group `0x0180`; 10 group `0x0060` | GH supplies ANAR selector bit `0x0001` only. Full selector field is `[4:0]`, mask `0x001F`; do not preserve an invalid selector by only setting bit 0 |
| ANER | partner AN capable `0x0001` | This lone source constant does not cover the whole register or its read effects |
| REGCR | address mode `0x0000`; data/no increment `0x4000`; read/write increment `0x8000`; write-only increment `0xC000`; fixed vendor DEVAD `0x001F` | Datasheet additionally identifies DEVAD 3 and DEVAD 7; EEE wire offsets remain separately unresolved in the full inventory |

GH 166-179 and 188-191 also defines gigabit control/status masks. They are generic
cross-device material, not DP83826 feature evidence: GIGCR master configuration
`0x1000`, master enable `0x0800`, 1000/full `0x0200`, 1000/half `0x0100`;
GIGSR master result `0x4000`, local/partner receiver `0x2000/0x1000`,
1000/full/half `0x0800/0x0400`; GIGESR full/half `0x2000/0x1000`.
Do not import these definitions into this part's writable register policy.

The source's basic advertisement helpers omit documented pause (`0x0400`),
asymmetric pause (`0x0800`), next-page (`0x8000`) and complete next-page behavior.
ANAR remote-fault `0x2000` is explicitly forbidden to applications by table 8-23,
despite an R/W marking. Absence from TI code cannot remove a documented feature;
an R/W marking cannot override that explicit restriction.

## Complete DP83826 private-header field inventory

These grouped rows account for P's named masks/offsets/encodings, including the
incorrect ones. Values are factual source extractions; corrections are identified
separately. This is deliberately not a generated implementation header.

| P lines | Register | Library fields and literal values | Datasheet comparison |
| --- | --- | --- | --- |
| 103-108 | RCSR `0x17`; CR1 `0x09` | RMII enable bit 5=`0x0020`; Fast RXDV bit 1=`0x0002`; robust Auto-MDIX bit 5=`0x0020` | Tables 8-41/28 agree; full register coverage needs more fields |
| 111-114 | CR2 `0x0A` | extended full-duplex bit 5=`0x0020`; RX_ER during idle bit 2=`0x0004`; odd-nibble detection disable bit 1=`0x0002` | Table 8-29 agrees; bit 1 has BASIC/ENHANCED strap conditions |
| 116-118 | CR3 `0x0B` | FLD field offset 0, mask `0x000F` | Table 8-30 also defines independent descrambler FLD bit 10=`0x0400`, pair swap bit 5=`0x0020`, polarity swap bit 6=`0x0040`; source mask is incomplete coverage |
| 120-127 | PHYSTS `0x10` | speed mask `0x0002`, 100=`0`, 10=`0x0002`; full duplex=`0x0004`; link bit 0=`0x0001` | Table 8-34 agrees on encodings but ENHANCED forced-mode text needs resolution before treating status as universal actual speed |
| 129-136 | PHYCR `0x19` | auto-MDIX `0x8000`; forced pair mask/MDIX=`0x4000`, MDI=`0`; LED stretching bypass offset 7/mask `0x0080` | Table 8-43 agrees; pause RX/TX status `0x2000/0x1000` and other fields are missing from this header |
| 138-146 | LEDCR `0x18` | blink offset 9/mask `0x0600`; polarity offset 7/mask `0x0080`; drive-enable offset 4/mask `0x0010`; drive-value offset 1/mask `0x0002` | Table 8-42 agrees; blink codes 0/1/2/3 mean 20/10/5/2 Hz (50/100/200/500 ms) |
| 148-150 | PHYRCR `0x1F` | SWRESET bit 15=`0x8000`; SWRESTART bit 14=`0x4000` | Table 8-48: hard reset versus circuitry restart retaining registers; both W1S/self-clearing |
| 152-172 | vendor MLEDCR `0x0025` | LED0 offset 3/mask `0x0038`; enable bit 0=`0x0001`; modes 0..14 listed below | **Conflict:** table 8-49 field is bits 6:3, correct mask `0x0078`. Bit 0 is R/W with reset 1, but encoding 0 is reserved; polarity bit 9=`0x0200` is absent from source |
| 174-188 | vendor GPIO `0x0303`..`0x0306` | all four CTRL offset 0/mask `0x0007` | Tables 8-74..77 mark LED0/LED3 control R, LED1/LED2 R/W. Do not infer writable pins or valid modes from masks alone |
| 190-196 | vendor LEDCFG `0x0460` | source LED3 offset 4/mask `0x00F0`; LED2 offset 8/mask `0x0F00`; LED1 offset 12/mask `0xF000` | **Conflict:** table 8-85 has LED3 offset 12/mask `0xF000`, LED2 offset 8/mask `0x0F00`, LED1 offset 4/mask `0x00F0`; low nibble reserved |
| 214-230 | vendor LEDCFG2 `0x0469` | LED1 drive enable/value/polarity offsets 0/1/2, masks `0x0001/0x0002/0x0004`; reserved offset 3=`0x0008`; LED2 enable/value/polarity offsets 4/5/6, masks `0x0010/0x0020/0x0040`; reserved offset 7=`0x0080` | Table 8-89 agrees on these positions; preserve reserved fields |
| 231-238 | vendor LEDCFG2 `0x0469` | source LED3 enable/value/polarity offsets 8/9/10, masks `0x0100/0x0200/0x0400`; reserved offset 11=`0x0800` | **Conflict:** table 8-89 marks bits 8..10 reserved too. Do not expose these as programmable LED3 controls without stronger exact-part evidence |

P 156-170 and 198-212, plus H 78-124, share LED source codes:

| Code | Meaning in library and tables 8-49/85 |
| --- | --- |
| `0x0` | Link established |
| `0x1` | RX or TX activity |
| `0x2` | TX activity |
| `0x3` | RX activity |
| `0x4` | Collision |
| `0x5` | 100BASE-TX speed |
| `0x6` | 10BASE-T speed |
| `0x7` | Full duplex |
| `0x8` | Link with activity blinking |
| `0x9` | Activity stretch |
| `0xA` | MII link at 100 Mbit/full duplex |
| `0xB` | LPI |
| `0xC` | TX/RX MII error |
| `0xD` | Link-loss indication retained until BMSR read |
| `0xE` | PRBS-error indication retained until counter cleared |

Code `0xF` is reserved in the datasheet. Independent regression vectors:
MLED code 8 contributes `0x0040`, code E contributes `0x0070` under mask `0x0078`;
the source mask `0x0038` destroys their high field bit. For LEDCFG, LED1 code 2,
LED2 code 3 and LED3 code 4 contribute `0x0020`, `0x0300`, `0x4000` respectively;
preserving reset low nibble 5 yields `0x4325`. These vectors test field placement,
not permission to overwrite unrelated live settings or proof of LED pin operation.

## Transport and MMD procedure audit

G 205-235 forwards direct read/write callbacks and returns status. It has no
local five-bit address validation, aggregate deadline, transaction acquisition,
write-effect certainty or staged read output. These are missing guarantees in
this wrapper, not proof that every possible external TI backend is unbounded.
The future portable API must supply its own explicit enforceable contract.

G 237-300 implements the following vendor DEVAD 31 transaction, stopping after
the first failing callback:

| Step | Read vendor offset A | Write vendor offset A, value V |
| --- | --- | --- |
| 1 | write Clause 22 `0x0D = 0x001F` | same |
| 2 | write Clause 22 `0x0E = A` | same |
| 3 | write Clause 22 `0x0D = 0x401F` | same |
| 4 | read Clause 22 `0x0E` | write Clause 22 `0x0E = V` |

SNLS647I 8.3.11.1-.5 and tables 8-31/32 corroborate the protocol. Example
vendor-address vector: `A=0x030E`; a required 10BASE-Te initialization write has
`V=0x4A40` (8.3.13, note on page 40), giving literal frames `(0D,001F)`, `(0E,030E)`,
`(0D,401F)`, `(0E,4A40)`. Successful selector writes are not successful target
configuration. Any ambiguous error must retain possible effects and must not
trigger an implicit retry. The next explicit operation re-establishes all selectors.

C 290-316 uses a different six-frame vendor RMW: the four read frames above,
then a repeated `0x0D=0x401F`, then write `0x0E` with the merged data. Only the
read status is tested; all five write results are ignored and the function returns
void. A failed selector/address write can therefore leave a stale target selected;
a later successful read does not make the target safe. Reject that error contract.
The source contains no lock spanning this sequence. A project RMW must hold
complete transaction ownership, check every phase and preserve consumed evidence
when the target is mixed/read-clear. Repeating the mode write is library behavior,
not an independently established silicon-required extra frame.

The source hardcodes DEVAD 31, so it does not implement the complete DEVAD 3/7 EEE
access contract. Nor does the presence of postincrement constants prove tested
incrementing operations. Keep the existing EEE wire-offset gate visible rather
than inventing a translation from printed addresses `0x1001/0x203C`.

## Initialization, reset and recovery procedure audit

| Source | Actual procedure | Required adaptation / failure case |
| --- | --- | --- |
| C 131-134; H 129-132 | InitCfg does nothing; configuration has no members | No reusable settings defaults or complete silicon initialization follow from this |
| C 186-227 | Check config pointer/size; set Auto-MDIX according to inverse loopback flag; select MII/RMII through indirect RCSR bit 5 | Interface write still occurs after invalid config pointer/size. Unsupported interface is not rejected here. Prove zero transfers on invalid input |
| C 229-253 | Write PHYCR auto-MDIX; enabling also sets CR1 robust Auto-MDIX; disabling forces MDI via PHYCR bit 14=0 | Both writes' errors ignored. Disable branch does not clear CR1 robust setting. Preserve caller policy and restoration rather than equating loopback exit with unconditional defaults |
| G 136-203 | Generic PCS reset writes BMCR reset command, later reads BMCR and tests bit 15 clear | Distinct from DP83826 hard reset. Source helper does not supply a timer/deadline or reset-completion delay |
| C 318-331 | Global software hard reset uses PHYRCR address `0x1F`, mask/value `0x8000` | Command accepted does not mean reset finished; reinitialize affected state and 10BASE-Te setup according to exact reset scope |
| C 333-359 | Completion reads register argument `PHYRCR_SWRESET`, i.e. `0x8000`, then tests bit 15 | **Confirmed address/mask defect.** Correct register is `0x1F`; a five-bit truncating backend might read BMCR instead. Never copy the literal call |
| C 361-393 | Restart writes PHYRCR `0x1F` bit 14=`0x4000`; completion reads `0x1F`, tests bit 14 clear | Correct register/mask distinction. Failed poll leaves output untouched, and logging can read that output; stage outputs explicitly |
| G 404-437 | Power-down helper changes both BMCR isolate `0x0400` and power-down `0x0800` | Library policy, not proof both bits are mandatory. Clearing both on wake discards prior isolation. RMII leader clock effect of isolate must be represented |

SNLS647I table 8-19 specifies PCS register reset while vendor registers are
retained. Table 8-48 specifies hard reset versus restart of circuitry with
register retention. Straps have separate POR-only restrictions in 8.4.1;
do not reduce these to one generic reset. The code does not perform the datasheet
10BASE-Te `VOD_CFG3=0x4A40` setup, identity revalidation, settings verification,
event retention, cancellation cleanup or bounded recovery. Those remain necessary
project work; the absence of code is not absent silicon functionality.

## Negotiation, forced modes and observations

G 515-642 sets advertised speed/duplex bits in `0x04`, includes selector bit 0,
and has an early success/no-I/O case for advertisement=0. Enable/disable helpers
(G 644-883) change only requested technology bits. These helpers neither perform
a complete replacement of all documented AN fields nor restart negotiation.
Do not copy their advertisement=0 behavior as successful application of an empty
requested configuration. Verify the full resulting fields, reserved bits and
separate requested/applied state.

G 885-946 changes BMCR AN enable `0x1000`, restart `0x0200`, or both `0x1200`.
G 948-1115 reads ANER partner capability, BMCR enable, BMSR completion and BMCR
restart-clear. Restart-clear means negotiation has started, not link completion:
SNLS647I table 8-19 explicitly distinguishes those states. Single BMSR reads can
consume remote-fault/link evidence and must use the shared collector.

G 1117-1215 forced-speed procedure uses mask `0x2140` (including reserved
gigabit bit 6) and values 10/half=`0`, 10/full=`0x0100`, 100/half=`0x2000`,
100/full=`0x2100`. It does not itself disable AN. For this PHY, use the proved
speed/duplex fields, explicitly disable AN within the owned procedure, neutralize
commands and handle crossover policy. CR1 robust Auto-MDIX `0x0020` is required
when using Auto-MDIX in forced mode (table 8-28); manual MDI/MDIX and pair/polarity
swap remain distinct controls. Test a failure after each accepted write.

C 395-450 reads PHYSTS `0x10` once, returns invalid link if bit 0 clear, otherwise
decodes speed mask `0x0002` and duplex `0x0004`. Its debug path can use uninitialized
`speed` on link-down or failed reads and `val` on failed reads; no project logging
may format invalid outputs. Literal low-bit vectors are `0x0001`=100/half,
`0x0005`=100/full, `0x0003`=10/half, `0x0007`=10/full **within a mode where those
status bits are established valid**. The ENHANCED forced-mode wording in table
8-34 remains a source/hardware gate; library decoding alone does not close it.

G 1217-1268's `isLinkUp` only performs the second BMSR read when the first link
bit is **set**. If the first bit is clear, it returns successful read status
without assigning `*pLinkUp`. This is a concrete output-validity defect and an
unsafe basis for a latch-low current-link procedure. Independent tests must cover
first-low/second-high recovery, persistent low, first-high/second-low, second-read
failure and caller output initially true. Keep first-sample history even when a
second sample fails. Cable absence remains a successful MDIO observation, not a
transport fault or an implicit recovery request.

## Destructive reads, counters and safe diagnostic output

The TI library provides no complete event/counter acquisition path. C 255-288
prints values despite ignored read failures; it reads BMSR and also uses generic
GIGESR at `0x0F`, actually the DP83826 FLDS clear-on-read register. G 302-314
similarly dumps every address 0..15 without checking failures or register holes.
Neither is a safe-dump design. Cached formatting must perform no bus accesses;
explicit live collection must preserve every consumed sample and partial errors.

Concrete datasheet vectors absent from that library:

| Register / table | Masks and effect | Required collector consequence |
| --- | --- | --- |
| FLDS `0x0F`, table 8-33 | bits 8:4=`0x01F0`, RC; energy/SNR/MLT3/RX/descrambler causes map to `0x0010/0x0020/0x0040/0x0080/0x0100` | Capture causes before another observer consumes them |
| MISR1 `0x12`, table 8-36 | high-byte RC events, low-byte enables; bit 15..8=quality, energy, link, speed, duplex, AN complete, false-carrier half-full, receive-error half-full | Literal event mask `0xFF00`, enables `0x00FF`; preserve high-byte sample before stable-control writes |
| MISR2 `0x13`, table 8-37 | high-byte RC events, low-byte enables; bit 15..8=EEE error, AN error, page, loopback FIFO, crossover, sleep, polarity/WoL, jabber | Same masks, different meanings; bit 9 polarity/WoL selection also depends on RXFS |
| FCSCR `0x14`, table 8-38 | low-byte RC count `0x00FF`; saturates `0xFF`; interrupt on exceeding `0x7F` | Saturation means a lower bound, not an exact lifetime count |
| RECR `0x15`, table 8-39 | field 15:0 RC, described as 16-bit but prose says maximum `0xFF` and half-full `0x7F`; no count in MII loopback | Preserve raw sample and unresolved width/saturation discrepancy; do not invent 16-bit wrap behavior |
| RCSR `0x17`, table 8-41 | RC overflow bit 3=`0x0008`, underflow bit 2=`0x0004`; printed descriptions say 0=fault, 1=normal | Resolve inverted-looking status semantics before derived fault reporting |
| RXFCFG1 vendor `0x04A0`, table 8-90 | bit 11=`0x0800` is W0C | Neutral composition retains 1; deliberate 0 acknowledges. Never use a generic zero-command policy |

PHYSTS table 8-34 additionally ties error bit 13 to RECR read, polarity bit 12 to
10BTSCR read, false-carrier bit 11 to FCSCR read, page bit 8 to ANER read, interrupt
bit 7 to MISR read and remote-fault bit 6 to BMSR read. A generic RC annotation does
not mean every PHYSTS read clears each mirrored flag. Preserve these dependencies
in the access ledger and test acquisition ordering and partial failures.

## Timing: silicon limits versus library policy

No delay constant, retry interval, reset timeout, lock deadline or link timeout
is defined by the reviewed DP83826/generic procedures. Their reset/completion
functions are single calls. Never invent a TI-library timeout from them or treat
an external scheduler's interval as a silicon minimum.

| Exact SNLS647I source | Value and qualifier | Engineering consequence |
| --- | --- | --- |
| 7.6 p 15 power-up T4; 8.3.11 p 34 | SMI ready maximum 50 ms; prose requires bus inactive at least 50 ms after power-up | Application must provide the power-up history or delay before first access; do not probe early |
| 7.6 p 15 reset T1 | RESET_N low minimum 25 us, without debouncing capacitors | Hardware pin/board timing, not a software-reset polling interval |
| 7.6 p 15 reset T2; 8.3.11 p 34 | SMI ready maximum 2 ms; prose requires inactivity at least 2 ms after reset deassertion | External-reset recovery must honor readiness before MDIO; applicability to software commands must be sourced explicitly |
| 7.6 p 17 footnote 2 | Delayed clock requires reset after POR completion and 100 us clock stability | Do not infer that time since bind proves oscillator/POR readiness |
| 7.6 p 15 | Power-up/reset to FLP nominal 1500 ms; strapped 100M signaling nominal 0.5 ms; RMII leader clock nominal 0.2 ms | These are nominal values, not maximum operation deadlines or universal link-up guarantees |
| 7.6 p 16 and 8.3.11 p 34 | MDC nominal 2.5 MHz, maximum 24 MHz; Clause 22 five-bit PHY/register fields, 16-bit data | Backend must prove electrical/controller compliance; permitted field width does not establish all strap addresses |
| 7.6 p 16 SMI T1-T3 | MDC-to-MDIO output delay 0-13 ns; MDIO input setup minimum 10 ns and hold minimum 10 ns | Qualify controller/pin timing and loading; frequency alone does not prove compliant transfers |
| table 8-20 p 56 | After reset/invalid opcode/turnaround: 32-bit preamble; suppressed-preamble transactions require minimum 500 ns gap plus positive MDC edge with MDIO=1 | Carry timing/framing requirements into adapter validation; callback duration alone does not prove wire compliance |
| 7.6 p 15 EEE rows | Sleep 210 us, quiet 20 ms, wake 36 us, refresh 200 us, all nominal | Useful comparison values for later EEE qualification, never hard upper bounds inferred from typical data |

The PHY's physical address straps are described as three bits/up to 8 devices in
8.3.11, although Clause 22 encodes five-bit addresses and PHYCR displays bits 4:0.
Keep protocol-range validation separate from source-qualified part/board address
selection. Do not silently claim 32 independently strappable DP83826 addresses.

## Implementation audit and closure rules

Follow the shared [concrete-evidence contract](../prompts/EVIDENCE.md) and
[source catalogue index](library-evidence.md).

At each consuming prompt reopen the current official source, record commit/file/
line and compare with these pins; reopen the exact current applicable datasheet
and errata, preserving archived revisions separately. Recheck every mask, field,
namespace, access type, sequence and min/nom/max qualifier actually used. Correct
the project code, tests, comments and this evidence when a finding is confirmed;
never reproduce vendor mistakes merely to match a reference implementation.

Use independent literal datasheet vectors and failures after each accepted frame.
Host tests prove software contracts only. Record actual part marking, revision,
BASIC/ENHANCED mode, straps, interface, clock and fixture for admitted hardware
checks. Compare real MDIO traces/readbacks and measured reset/link/pin behavior;
if unavailable, label that physical gate NOT RUN/BLOCKED and retain it. No guessed
timing, fabricated capture, presumed support or passing mock closes a silicon
contradiction. Continue independent eligible work while exact-part clarification
is pending; do not remove the affected documented feature from full coverage.

Named open gates include ENHANCED forced PHYSTS interpretation, RECR saturation,
RCSR RC polarity, LED/GPIO read-only/reserved contradictions and EEE wire offsets.
Confirmed source defects include reset address/mask confusion, MLED width,
LED1/LED3 field swap, fabricated LED3 writable controls, invalid-input I/O,
ignored RMW/configuration errors and successful link queries with untouched output.
These are review results, not hardware observations or implemented fixes.

[c]: https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826.c
[p]: https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83826_priv.h
[h]: https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/include/dp83826.h
[g]: https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c
[gh]: https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/include/generic_phy.h
[license]: https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/LICENSE
