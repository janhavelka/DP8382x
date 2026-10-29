# Public interface location

Reserved for the future DP8382x public headers. No API is implemented yet.
See [architecture](../../docs/architecture.md) and the accepted
[ownership/call-chain decisions](../../docs/design-decisions.md).

Prompt 6 selects one framework-independent C++17 core, one borrowed terminal
MDIO batch callback, and one retained operation/result slot. Prompts 7–8 freeze
the exact types/signatures; this note does not publish placeholder declarations.
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
teardown/quiescence. Add source documentation checks with real code in prompt 9.
