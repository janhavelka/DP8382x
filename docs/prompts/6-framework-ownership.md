# 6 - Resolve portable core and native SDK ownership

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [2](2-ecosystem-tooling-audit.md)'s consumer/tool findings and
the concrete register/procedure facts in prompts 3-5 needed for ownership.
Unresolved unrelated feature evidence stays open. Restate the current rules,
owned design files, checks and same-branch checkpoint briefly.

## Complete outcome

Choose one implementable portable C++17 plus native ESP-IDF architecture in
`docs/design-decisions.md` and `docs/architecture.md`. Complete the entire
ownership/call-chain matrix now: splitting SDK state, bus ownership and lifetime
into incompatible decisions would create implementation defects. This is a
design deliverable, not an adapter support claim or a second generic PHY stack.

## Source audit and responsibility matrix

Reopen TMP1x2 borrowed transport/`bind`/`startConfigure`/`poll`/`takeResult`,
PCA9555 `pollOperation`/`ObservedState`, TCA9548A cleanup, native IDF examples
and FieldCore `Ina228Module` owner/cached projection methods. Record current
paths/revisions and adopted/adapted/rejected behavior; I2C transfer granularity
does not transfer to indirect MDIO.

Pin actual supported ESP-IDF sources, including `esp_eth_phy.h`,
`esp_eth_phy_802_3.h/.c`, `esp_eth.c` and `esp_eth_driver.h`. Starting evidence
is v6.1 commit `fff9895c82d744c7237be8847347bdd1b07c6643`; audit any changed
baseline explicitly. For construction/delete, identity, reset/power, settings,
pause, link notifications, interrupts, diagnostics and raw ioctl/CLI, name:
public entry, shared chip procedure, base/mediator method, owner/lock, time
source, state effects and result collector in portable and SDK modes.

- Qualify base reuse by behavior. The pinned base maps nonzero pause ability
  to both advertisement bits, and its MAC pause notification is Boolean peer
  capability, not full resolved RX/TX pause. Keep complete portable evidence,
  document narrower projection and override incompatible methods through
  shared procedures. MAC/application flow-control policy remains external.
- Trace cached SDK state: `esp_eth.c` stores `auto_nego_en`. SDK-owned settings
  must use supported stopped-driver controls such as `ETH_CMD_S_AUTONEGO`
  and agree with `ETH_CMD_G_AUTONEGO`. Raw BMCR changes invalidate state and
  require public reconciliation before restart. Never patch private caches or
  invent ioctls for full advertisement/other PHY-owned fields.
- Trace actual `ETH_CMD_READ_PHY_REG`/`WRITE_PHY_REG` mediator bypasses.
  Public `esp_eth_config_t::read_phy_reg`/`write_phy_reg` hooks intercept
  frames, not whole MMD transactions. Select an owner-aware route or explicitly
  prohibit bypass and provide a supported owner entry. Forward to underlying
  MAC methods without recursively entering the mediator/ioctl/lock path.
- Choose one terminal batch callback or explicit transaction-scope mechanism.
  Show complete MMD and RMW exclusion across every participant, another PHY
  on the same bus, error exits and nested lock order. Do not ship both options
  for speculative flexibility. Release controller locks before waits/polls;
  logical operation admission is separate.
- Audit implicit base address scanning/reset, synchronous callbacks, actual
  lock/controller waits and quantization. Derive aggregate bounds including
  acquisition, frames and release; a timeout parameter alone proves nothing.
  State unbounded backend limits honestly; no software preemption claim.

Walk competing live-status/MMD callers, reset versus retained evidence,
stop/delete versus callbacks, accepted-then-failed writes, raw mutation versus
base polling, and IRQ versus destructive reads. Name admission, invalidation
and cleanup owners. One collector preserves consumed events. Keep board pins,
GPIO reset timing, clocks, IP/EtherCAT/frame handling and recovery policy outside
core. Show portable and native call chains and source/include selection without
SDK headers, heap, tasks or platform timing in the portable layer.

## Review, documentation and acceptance

Use an independent stronger-reasoning ownership reviewer when available;
delegate read-only inventories separately. The lead owns shared decisions and
Git, checks every call chain and resolves findings. Record model rationale.
No implementation success stubs are needed to finish the design.

Update API-plan/example notes, architecture and design decisions together.
Specify future Doxygen requirements for borrowed lifetime, non-reentrancy,
output publication, aggregate bounds and teardown. Review README/CHANGELOG.
Accept only a matrix with no unowned entry, duplicate poll/config owner or
unexplained deadlock; verify the plain C++ use case and actual SDK paths.
Run local reference/link/diff checks, record limitations/report/progress, then
commit and sync checked work and coherent units on the recorded branch.
Remote CI remains prompt 71 only.
