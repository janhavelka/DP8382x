# Portable implementation

`DP8382x.cpp` owns the explicit register access policy, bounded terminal transport,
Clause 22/MMD and exact identity operations, destructive-evidence retention and source-specific mixed
updates. It uses fixed storage, no SDK or platform clock and no allocation.
Public contracts live in `include/DP8382x/`; the ledger owns source facts.
See [architecture](../docs/architecture.md), [direct access](../docs/direct-access.md)
and [side-effect access](../docs/side-effect-access.md). SDK-only code belongs in
`esp_idf/`; typed features arrive with their complete procedures.
