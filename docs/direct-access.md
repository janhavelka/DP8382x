# Bounded portable Clause 22 and indirect MMD access

Initialization uses these same terminal batches under [OP-14](api-contracts.md#one-cooperative-operation-and-retained-result).
All raw writes reject an occupied operation slot or cleanup barrier. Safe reads
remain admitted between polls and with retained results, except reset WAIT.
Each operation poll executes at most one unsplit batch and releases before waits.
See [initialization and the owner example](initialization.md).

Prompts 11–12 ship `DP8382x::DP8382x` in
[`DP8382x.h`](../include/DP8382x/DP8382x.h), backed by one compiled portable source.
Link `DP8382x::DP8382x` through CMake. The
[compiling direct-read example](../examples/portable/DirectRead.h) binds an
application transport, checks Status before publishing a PHYIDR1 word and detaches
locally. The [MMD example](../examples/portable/MmdAccess.h) reads raw straps and
modifies stable LEDCR fields on an already bound driver. The
[separate consumer](../test/consumer/main.cpp) executes both using an
explicit host simulation; it is not a physical MDIO backend.

The owner declares the exact DP83826I, MAC interface, protocol PHY address 0..31,
terminal batch callback, microsecond clock and conservative platform bounds.
Null context is legal; missing callbacks or positive fixed/frame/clock-lag
bounds are not. Protocol width is separate from the part's three address straps.
`bind`, `end`, construction/destruction and copied snapshots make no bus or clock
calls. Invalid replacement preserves the old binding and receipts. Drivers cannot
be copied or moved. The owner serializes every access, including snapshots, and
keeps the borrowed resources alive. Defensive reentry rejects live teardown with
BUSY; it does not make concurrent unsynchronized calls safe.

The read policy admits known nonconsuming words: BMCR, PHYIDR1/2, ANAR, ALNPAR,
ANNPTR, ANLNPTR, CR1/2/3, PHYSCR, BISCR, LEDCR, PHYCR, BICSR1/2 and PHYRCR.
See the [ledger](registers/dp83826i.json) for their numeric map and the original
SNLS647I tables 8-19..48 for access semantics. Raw observations establish neither
identity acceptance nor current diagnostic/link interpretation. PHYCR's address
field and BISCR status can change independently of the software owner.

The ordinary raw write/RMW policy is LEDCR's independent blink, polarity,
drive-enable and drive-value fields; every reserved bit must be zero. This is an
explicit pre-initialization maintenance exception, not managed configuration.
The application establishes the physical part/address and safe LED0 wiring/pin
use. No initialization, cached LED state, reset, link or diagnostic
operation is implied. The complete typed LED/mux API remains prompt 32.

Other ordinary writes, holes, public selectors, consuming/latch reads and CDCR reject with
INVALID_PARAM/ACCESS_POLICY before either callback. CDCR itself is not claimed
read-clear: observing enabled resistor calibration obliges follow-up recovery,
so a bare read is not an accepted standalone procedure. Selectors are private
steps of the complete MMD operation. Explicit destructive acquisition and mixed maintenance use the
[side-effect access guide](side-effect-access.md), preserving the same policy.

Identity acceptance is now available through [readIdentity](identity.md), separate
from raw word reads. Existing maintenance exceptions remain owner-attested; a
failed/rejected probe blocks all maintenance mutations before callbacks until
accepted reprobe or explicit rebind. Selectors used by ordinary MMD reads do not
count as semantic mutation or bypass this policy.

## Callback and budget contract

Each direct call passes one frame, MMD read/write four and MMD RMW five, always
in one batch. Address/data values are checked
at the public uint32 boundary before narrowing, including register 0x8000 and
data 0x10000. The backend owns the shared controller lock and stops on the first
failure. It acquires ownership once and executes the frames inside that scope;
calling a mediator that recursively reacquires ownership is forbidden.
It borrows descriptors only until synchronous terminal return and must
not retain buffers, reenter the driver or retry. All users, including another PHY
instance, must use the same controller admission/quarantine mechanism.

Execute descriptors in order; a dependent write uses only its earlier successful
read and computes `(read & preserveMask) | value`. Stop on the first failed frame
or deadline, release on every return and classify the terminal report before
reopening shared admission. Release/idle uncertainty or malformed evidence
quarantines every controller user until owner repair. A completed READ supplies
valueValid; a WRITE supplies no read value. OK has detail zero and every message
has static non-null storage. Preserve signed backend detail and possible effects;
absence of an MDIO acknowledgement does not establish absence of a hardware effect.

Admission reserves fixedUs + frameCount * frameUs, strictly
less than remaining time after clock-lag allowance. The original absolute
exclusive deadline and frame allowance reach the callback unchanged. A zero-frame
budget, expired deadline or insufficient bound makes no batch call. The backend
must bound execution itself, including scheduling and controller finalization;
detecting lateness after return cannot prove a finite backend. Only a raw post-return sample at or beyond the deadline denies successful
publication; there is no publication reserve. Admission equality is insufficient. A backward clock
or overflow of raw time plus lag latches CLOCK_CONTRACT until owner repair and a
fresh binding; no later sample silently repairs it. Frozen time cannot add frames.

Only OK, MDIO_ERROR, MDIO_TIMEOUT, the two defined owner/quarantine BUSY details
and defined OPERATION_TIMEOUT details are terminal primary callback outcomes.
BUSY has no frames. Release accepts only OK/MDIO_ERROR/MDIO_TIMEOUT. Null messages,
illegal statuses/details, short success, contradictory frame validity and extra
slots fail normalization. The exact raw report and signed details remain in the
snapshot. Attempted values without valueValid are not output, even if their
scratch word is nonzero. Unused/unattempted slots must be default. Optional valid
completion stamps must be nondecreasing and lie between the pre-call raw and
post-call upper samples. A failed or missing frame closes the valid prefix;
later reported attempts are retained as contradictory evidence, never completion
counts. Bad timestamp metadata alone does not erase independent physical counts.

## Complete indirect transactions

`readMmd` and `writeMmd` validate DEVAD and the sixteen-bit offset before any
callback. Vendor DEVAD31 uses the documented mapping. The source-implied EEE read tuples (3,0014), (7,003C), (7,003D) are also admitted
under the recorded D2 decision and physical case Q1. Other DEVAD3/7 offsets and
all raw EEE writes reject with INVALID_PARAM/ACCESS_POLICY. Opaque recipe-only
addresses also reject; their named recipes own exceptional access. Indirect REGCR/ADDAR accesses
are explicitly unavailable in SNLS647I 8.3.11.1 and cannot establish a public
selector session. Native Clause 45 is not implemented or implied.

The direct and indirect aliases share one source-based read/write policy. Beyond
the direct words listed above, nonconsuming extended reads cover detailed LED,
compliance-pattern, preamble, FLD, cable configuration and defined result words,
MSE, pin/clock configuration, amplitude, receiver gain, IPG, straps and WoL
configuration/MAC/password registers. The policy table in the
[implementation](../src/DP8382x.cpp) corresponds to 38 detailed extended records;
the [ledger](registers/dp83826i.json) owns their numeric map. All-reserved result
banks, destructive RXFS, holes and procedure-only addresses remain inaccessible.
RXFCFG1's W0C acknowledgement is harmless to read but excludes generic write/RMW.
Raw MSE is not SNR; raw TDR is not a fresh capture or calibrated fault distance;
raw straps do not prove reset resampling. Field/recipe interpretation gates remain.

Separate read/write traces are printed in SNLS647I 8.3.11.4/.5; the combined
five-frame RMW below is inferred from selector retention in 8.3.11.1, not printed:

| Frame | Read SOR1 | Write LEDCR | Modify LEDCR |
| --- | --- | --- | --- |
| 1 | W 0D = 001F | W 0D = 001F | W 0D = 001F |
| 2 | W 0E = 0467 | W 0E = 0018 | W 0E = 0018 |
| 3 | W 0D = 401F | W 0D = 401F | W 0D = 401F |
| 4 | R 0E, staged word | W 0E = 0692 | R 0E, earlier-read slot 3 |
| 5 | — | — | W 0E = (read & preserveMask) OR value |

All words in the table are hexadecimal; readIndex is zero-based. REGCR's reserved
bits are zero in complete literal control words. `modifyMmd` requires a nonzero
mask wholly inside the stable writable fields, and replacement value wholly inside
that mask. LEDCR preserves other documented stable bits and forces its reserved bits
zero. Other targets preserve read-back reserved bits unless an exact recipe overrides them. For a deliberately poisoned test read FFFF, mask 0012 and value 0002 yield
0682, not a replay of reserved ones. The read and dependent write occur within the
same nonincrementing selection, so five frames are sufficient. The earlier
eight-frame design was a conservative reselection policy, not a silicon requirement.
No operation exposes an incrementing mode or caller-supplied function opcode.

Both four- and five-frame calls admit their entire count and bound before the
callback. A later cooperative caller with fewer frames must yield before entry;
this direct API returns OPERATION_TIMEOUT/FRAME_BUDGET without launching a prefix.
The wrapper still stops and releases on an actual in-flight error/deadline; that
is a terminal failed transaction, never a suspended selector session. The original
remaining allowance is forwarded unchanged because these calls use one batch.
Release precedes any later scheduled wait; no poll holds a controller lock.

Snapshot frames 0..2 describe selector/address effects. Frame 3 is the target
read/write; only frame 4 mutates the target in RMW. A failed setup can change
selectors without changing the target. A failed final write can have changed the
target even if Status is an error. Copy the raw receipt with the initiating call's
address/phase attribution; the snapshot is not a standalone transaction trace.
Every next explicit call emits the full selection again, regardless of prior
success/failure. A later read is new evidence, not retroactive confirmation of
the previous ambiguous write. No retry or selector-cleanup write is hidden.

Every callback increments the saturating batch count; frame counts use only the
independently valid requested prefix. Successful frames remain counted on later
primary/release/time failure. No managed-health API is implemented here and raw
access will not feed its success/failure statistics. The normalized primary error
takes precedence over release/unsafe-controller error, then clock fault, then
deadline failure. `late` records positive raw-clock evidence only;
an upper-bound overlap alone does not deny publication. Clock-lag uncertainty
remains a declared limitation rather than a second publication reserve.

Release/idle uncertainty quarantines all controller users in the trusted backend.
Subsequent calls may enter that wrapper but cannot launch a frame until application
recovery proves safe reuse. If malformed evidence escapes the wrapper, this
instance exposes stopped=true and stops locally too; the owner must shut down all affected users and repair
the backend. Local `end`/rebind cannot release a controller or clear its shared
interlock. Copy needed receipts before another batch, rebind or end replaces them.

An attempted failed write may have applied; a completed write followed by an error
or late return retains that fact. No retry, cache rollback or readback is implied.
Success is terminal controller completion, not verified pin state. Physical MDIO
has no I2C ACK/byte-count proof of accepted register data.

## Backend evidence and verification

SNLS647I 7.6, 8.3.11 and table 8-20 specify MDC 2.5 MHz nominal and 24 MHz maximum,
MDIO output delay 0..13 ns and input setup/hold minima 10 ns. Suppressed-preamble
transfers need at least 500 ns between transactions followed by a positive MDC
edge with MDIO high. A 32-bit preamble follows reset or invalid opcode/turnaround.
The owner coordinates power/reset readiness and bus-wide silence, including the
documented 50 ms power-up and 2 ms external-reset-release intervals with a valid
clock. These are silicon requirements, not invented frame timeout defaults.

Host tests prove admission, normalization, release/effect retention and output
staging using synthetic time and fault injection. Actual lock/controller bounds,
waveforms, preamble, gap and readiness require the identified backend plus admitted
measured traces. SDK link, physical timing and hardware qualification are NOT RUN.
The [direct-access report](reports/implementation/11-clause22-transport.md),
[MMD report](reports/implementation/12-atomic-mmd.md) and
[test guide](../test/README.md) record commands, scope and evidence.

Prompt 16 applies the persistent HARD quiet floor to every frame, including raw
reads, the collector and later operations. A timed call can sample the clock to
reject or expire the floor before MDIO. Cached access remains clock-silent; the
shared owner must also exclude other PHY addresses. See [initialization](initialization.md).
