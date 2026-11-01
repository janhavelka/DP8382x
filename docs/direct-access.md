# Bounded portable Clause 22 access

Prompt 11 ships `DP8382x::DP8382x` in
[`DP8382x.h`](../include/DP8382x/DP8382x.h), backed by one compiled portable source.
Link `DP8382x::DP8382x` through CMake. The
[compiling direct-read example](../examples/portable/DirectRead.h) binds an
application transport, checks Status before publishing a PHYIDR1 word and detaches
locally. The [separate consumer](../test/consumer/main.cpp) executes it using an
explicit host simulation; it is not a physical MDIO backend.

The owner declares the exact DP83826I, MAC interface, protocol PHY address 0..31,
terminal batch callback, microsecond clock and conservative platform bounds.
Null context is legal; missing callbacks or positive frame/overhead/clock-lag
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

The only write policy in this stage is LEDCR's independent blink, polarity,
drive-enable and drive-value fields; every reserved bit must be zero. This is an
explicit pre-initialization maintenance exception, not managed configuration.
The application establishes the physical part/address and safe LED0 wiring/pin
use. No initialization, cached LED state, reset, selector, link or diagnostic
operation is implied. The complete typed LED/mux API remains prompt 32.

All other writes, holes, REGCR/ADDAR, consuming/latch reads and CDCR reject with
INVALID_PARAM/ACCESS_POLICY before either callback. CDCR itself is not claimed
read-clear: observing enabled resistor calibration obliges follow-up recovery,
so a bare read is not an accepted standalone procedure. Selectors will be private
steps of prompt 12's complete MMD operation. Prompt 13 owns side-effect-aware
access; no placeholder methods expose those operations now.

## Callback and budget contract

Each accepted call passes one frame in one batch. Address/data values are checked
at the public uint32 boundary before narrowing, including register 0x8000 and
data 0x10000. The backend owns the shared controller lock and stops on the first
failure. It borrows descriptors only until synchronous terminal return and must
not retain buffers, reenter the driver or retry. All users, including another PHY
instance, must use the same controller admission/quarantine mechanism.

Admission reserves acquisition + frame execution + release + overhead, strictly
less than remaining time after clock-lag allowance. The original absolute
exclusive deadline and frame allowance reach the callback unchanged. A zero-frame
budget, expired deadline or insufficient bound makes no batch call. The backend
must bound execution itself, including scheduling and controller finalization;
detecting lateness after return cannot prove a finite backend. Output publication
also reserves clock-lag and overhead. Equality is insufficient. A backward clock
or overflow of raw time plus lag latches CLOCK_CONTRACT until owner repair and a
fresh binding; no later sample silently repairs it. Frozen time cannot add frames.

Only OK, MDIO_ERROR, MDIO_TIMEOUT, the two defined owner/quarantine BUSY details
and defined OPERATION_TIMEOUT details are terminal primary callback outcomes.
BUSY has no frames. Release accepts only OK/MDIO_ERROR/MDIO_TIMEOUT. Null messages,
illegal statuses/details, short success, contradictory frame validity and extra
slots fail normalization. The exact raw report and signed details remain in the
snapshot. Attempted values without valueValid are not output, even if their
scratch word is nonzero. Unused/unattempted slots must be default. Optional valid
completion stamps must lie between the pre-call raw and post-call upper samples.

Every callback increments the saturating batch count; frame counts use only the
independently valid requested prefix. Successful frames remain counted on later
primary/release/time failure. No managed-health API is implemented here and raw
access will not feed its success/failure statistics. The normalized primary error
takes precedence over release/unsafe-controller error, then clock fault, then
deadline/publication failure. `late` records positive raw-clock evidence only;
an upper-bound overlap alone denies publication without proving lateness.

Release/idle uncertainty quarantines all controller users in the trusted backend.
Subsequent calls may enter that wrapper but cannot launch a frame until application
recovery proves safe reuse. If malformed evidence escapes the wrapper, this
instance stops locally too; the owner must shut down all affected users and repair
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
The [stage report](reports/implementation/11-clause22-transport.md) and
[test guide](../test/README.md) record commands, scope and evidence.
