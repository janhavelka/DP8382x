# Public interface location

This directory now ships portable C++17 `Status.h`, `Config.h`,
`CommandTable.h` and generated `Version.h`: passive values, error helpers,
pure link validators, selected source-proved register/mask constants and release
version values. They perform no MDIO I/O and need no SDK. Edit `library.json`
and run `python scripts/generate_version.py sync` for version changes; never
hand-edit `Version.h`.
There is no device facade or working operation API yet.
See [architecture](../../docs/architecture.md) and the accepted
[ownership/call-chain decisions](../../docs/design-decisions.md).

Prompt 6 selects one framework-independent C++17 core, one borrowed terminal
MDIO batch callback, and one retained operation/result slot. Prompt 7's
[API contract](../../docs/api-contracts.md) now freezes exact foundational values
and direct signatures; prompt 8 completes cooperative lifetimes. The remaining
method signatures are design contracts until implemented at their owning stage.
`bind`/`end` and cached snapshots are local, but still require owner serialization.
Invalid replacement binding preserves the existing valid binding. `end` cannot
discard active work, owed cleanup or unacknowledged evidence.

Short C22/MMD access uses complete bounded batches; RMW expresses a masked write
from a preceding read inside the same batch. The core owns register access policy
and chip procedure order; transport owns controller-wide exclusion and terminal
frame execution. Every destructive sample has one collector. Snapshots copy values
without I/O or clock reads. Cooperative procedures use `start...`,
`pollOperation`, `takeResult` and `cancel` under the shared conventions.

Native public headers belong only in `esp_idf/`; core headers cannot include SDK
types, allocate, use a platform clock or depend on an application framework.
Native owner entry points reconcile SDK-cached settings through supported stopped
controls and expose typed raw/feature requests. Arbitrary SDK raw frame ioctls are
rejected. Native support depends on the selected SDK lifecycle correction.

Doxygen must describe borrowed context/lock/buffer lifetime, non-reentrancy,
legal states, complete transaction exclusion, aggregate bounds and backend limits,
ordinary output publication versus retained partial destructive samples,
accepted/uncertain effects, separate primary/cleanup status, acknowledgement and
teardown/quiescence. Prompt 9 checks the shipped header comments with Doxygen;
later methods must add their own documentation when they work.

Implement methods only at their owning stage: transport accounting begins at 11,
identity at 14, settings/link at 17–19 and managed health at 21. Do not publish
working-looking health methods that return fixed UNINIT/READY before then.
Drivers and operation owners are neither copyable nor movable. Direct reads stage
ordinary output, while the collector retains consumed partial evidence. A valid
link-down is successful output, not missing data or an MDIO error.

The first operation consumer at prompt 15 implements the frozen slot, lifetime
tokens, separate cleanup budget and exact-result take. Introduce hard reset and
later configuration/recovery options only with their behavior. Failed cleanup
requires owner-assisted quiescence/disposition before result delivery and end;
acknowledgement does not claim a stopped PHY or restored configuration. Include
clock-return lag, tick rounding, final-poll exhaustion and partial effects in
public Doxygen and executing tests at that stage.
