# 26 - Implement exact documented DP83822 variants

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-25 software outputs; independent unresolved variant
gates remain explicit.

## Fresh audit and owned work

Re-audit peer per-model capability and nonuniform hardware validation patterns. Read
SNLS505H, relevant DP83822 TDR/app notes and exact orderable-part tables.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Repeat FULL_COVERAGE's complete public-source and register/field/procedure audit
for every exact part in this stage. Implement all documented programmable
features, including those absent from the first board or upstream driver's API.
Track inherited, different and newly available controls explicitly. Identity and
common link operation alone do not complete variant support; software gaps stay
open while hardware qualification remains a separate evidence layer.

Qualify DP83822I, H, IF and HF individually. Build a matrix of observed identity,
board-declared subtype, copper/fiber and RGMII/RMII/MII capabilities,
clock/reset/straps, optional features and documented temperature/package distinctions.
Include exact RGMII timing/delay controls where supported; a particular MAC adapter may
reject an interface without removing it from the generic device model. Do not infer
suffix from shared IDs or expose fiber on copper-only parts.

Review all standard and extended register differences, startup/10 Mbit requirements,
interrupt/latch/counter behavior, link decoding and diagnostic procedures. DP83822 TDR
instructions apply only to supported exact conditions. Avoid flattening all variants
into a shared feature bitset without source evidence.

Implement shared behavior plus explicit differences with maintained bounded ownership
and reset/restoration. Include copper/fiber configuration validation and supported
forced/negotiated mode semantics. Any feature requiring an absent medium/fixture remains
honestly unqualified, even if software paths compile.

Update portable API capability queries, native adapter, CLI feature coverage and Python
suite selection. Keep undocumented variants unsupported and preserve all previous part
regressions. A large family namespace is not evidence of family-wide hardware support.

## Acceptance and handoff

Run a per-exact-part host matrix including interface/medium rejection, shared/ambiguous
IDs, register differences, link and optional diagnostics, plus regression for original
DP83826I. Build supported SDK configurations. Hardware fixture rows name exact
subtype/medium and cannot inherit another member's pass.

Expected output: Documented DP83822 subtype support and explicit medium/interface
limits. Include source documentation, relevant guides, feature/evidence matrix and
progress updates. Apply COMMON's completion rules; report actual commands, results and
remaining gates. Continue only when the concrete required inputs exist; COMMON explains
how unrelated gates stay open.
