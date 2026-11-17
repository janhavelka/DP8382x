# Prompt 38: native configuration source audit

Read-only source audit performed on 2026-11-17. Entry was clean `main` at
`a744e77e3ac000be5ea12b1c4b5fdef55b50efde`. The lead owns shared headers,
native integration, architecture, progress, final evidence and Git. The
gpt-6.1-sol/high agents own bounded real-driver tests and this peer inventory /
P4 fixture extension; gpt-6-astra/high provides independent ownership and failure
review. The capable coding model fits the bounded harness/fixture work; stronger
independent reasoning is used for SDK cache, admission and partial effects.

Repository and parent AGENTS, prompt 38, COMMON, relevant shared contracts,
README, architecture, variants, reference index, design sections 2-7 and the
operation contract were reopened. Prompt 37 provides the working native owner,
hash-checked real-driver host target and disabled P4 link fixture. No silicon
variant, board authorization or reusable teardown qualification is added here.

## Fresh peer inventory

All paths below are relative to `Projects/`. Fresh HEAD/status checks found
these repositories clean. Siblings and SDK remain read-only.

| Repository | Actual HEAD | Native production/example seam |
| --- | --- | --- |
| INA228 | `8e439d6c99ece1353cd9f555deed0314794412d9` | `examples/esp_idf/basic/main/Ina228IdfI2cTransport.h`, `.cpp`, `main.cpp` |
| PCA9555 | `0aec2212894910e0d872c3e511fbcd740837c5f7` | `examples/espidf_basic/main/main.cpp` |
| TCA9548A | `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | `examples/espidf_basic/main/main.cpp` |

For all three, re-audited `AGENTS.md`, public `include/<name>/Config.h`,
`Status.h`, facade, `src/<name>.cpp` and `.github/workflows/ci.yml`. Current
rules require framework-neutral core, native ESP-IDF examples, application bus
ownership, precise Status, finite work and evidence separated by layer. Their
Git and CI policies do not override this repository's main-only / prompt-71
first-execution exception. DP8382x still has no `.github/workflows` definitions.

The concrete behavior and failure inputs were:

- INA228: context/address validation; `ina228IdfInitI2c`,
  `ina228IdfDeinitI2c`, temporary-device removal retention and native timeout
  conversion. `src/INA228.cpp` keeps dirty-register evidence after successful or
  uncertain writes and cancellation. `test/test_basic.cpp` cases
  `test_cancel_and_timeout_are_bus_silent_with_precise_effects`,
  `test_recover_replay_failures_mark_dirty_for_each_write_position` and
  `test_raw_software_reset_marks_every_reset_register_dirty` exercise those
  distinctions. `tools/check_owner_contract.py` and
  `tools/check_idf_example_contract.py` check bounded owner/framework boundaries.
  Native CLI `runOwnerJob` retains the operation ID and consumes terminal
  timeout results. Runner inputs: `tools/run_i2c_hil.py`,
  `tools/test_run_i2c_hil_parser.py` (correlated finite sessions).
- PCA9555: `mapI2c` preserves NOT_ATTEMPTED versus MAY_HAVE_COMMITTED;
  `probeTargetAddress` keeps address-only evidence separate from ambiguous
  transfer faults. `pollOperation` in `src/PCA9555.cpp` separates deadline,
  transaction budget and pointer cleanup. `test/test_basic.cpp` cases
  `test_failed_direction_after_preload_retains_partial_and_uncertain_evidence`,
  `test_raw_configuration_writes_are_rejected_without_io`,
  `test_partial_raw_write_invalidates_whole_pair_until_full_pair_write` and
  deadline/cancellation cases retain partial state. `test/test_public_contracts.cpp`
  checks instance ownership. Native CLI and `examples/common/CliShell.h` use
  bounded input; the latter is Arduino glue, not a native core dependency.
  Runner/check inputs: `tools/run_i2c_hil.py`,
  `tools/test_run_i2c_hil_parser.py`, `tools/check_idf_example_contract.py` and
  `tools/check_hil_contract.py`.
- TCA9548A: `prepareAddress` creates replacement registration first, retains the
  old registration on failure, and retains a failed rollback handle with
  `registrationFaulted`. Failed native receive sets `receiveFaulted`, closing
  controller admission until reboot. Core writes invalidate stale mask
  observations on error. `test/test_driver/test_driver.cpp` cases
  `test_expired_owner_budget_performs_no_physical_io_or_cached_success` and
  `test_late_write_requires_a_new_cleanup_budget_and_failed_cleanup_stays_unknown`
  distinguish actual mutation, late completion and separate cleanup budgets.
  `examples/common/Tca9548aCli.cpp`, `CliArguments.h`, native `main.cpp`,
  `tools/check_cli_contract.py`, `tools/check_idf_example_contract.py`,
  `tools/tca9548a_hil.py`, `tools/test_tca9548a_hil.py` and
  `tools/test_hil_artifacts.py` supply strict parsing, command boundaries,
  bounded artifact/verdict and firmware-identity precedents.

Adopted: validate before mutation, one application owner, stable borrowed
contexts, retained uncertainty, separate primary/cleanup results, no automatic
retry, precise backend detail and explicit firmware identity. Adapted: late
I2C completion evidence to MDIO terminal receipts; TCA faulted-controller
retention to the existing Ethernet resource lifetime gate. Every MMD/RMW batch
needs controller-wide serialization across all PHY participants; peer I2C
single-owner examples do not prove that transaction property.

Rejected: PCA9555 native `ensureDevice` ignores removal failure and clears the
old handle; INA228 initialization can replace the original add-device failure
with cleanup failure. Neither is a model for losing primary failure or resource
ownership. Peer address retargeting is not a reason to permit native PHY address
changes: current-value `set_addr` is idempotent; other values require
delete/recreate, subject to the existing teardown gate. Sensor health admission,
I2C NACK meanings, Arduino types, S2/S3 matrices and peer SDK support ranges are
not DP8382x native PHY contracts.

## Reopened pinned SDK

Actual checkout: `C:\esp\v6.1\esp-idf`, clean tracked state at
`fff9895c82d744c7237be8847347bdd1b07c6643`. `git hash-object` matched the
maintained [SDK catalogue](../../reference/library-sdk.md):

| SDK-relative file | Actual blob |
| --- | --- |
| `components/esp_eth/include/esp_eth_driver.h` | `405138e4e63dd93a21896bb970b8ad19d68127f4` |
| `components/esp_eth/src/esp_eth.c` | `76dab829d2099a574a07aeceeaa82491950fe07c` |
| `components/esp_eth/src/phy/esp_eth_phy_802_3.c` | `7d5d407d029c63494dffaaa3d82f8701b6b73790` |
| `components/esp_eth/src/mac/esp_eth_mac_esp.c` | `6448fcd6de4669ddf1e836c992ca8a1accc9dd96` |

Source-derived routing requirements, using those original line numbers:

- Driver 457-476 and 497-505 require STOP for AN/speed/duplex mutation and
  cached AN disabled for forced setters. `ETH_CMD_S_AUTONEGO` passes the SDK's
  cache address to the PHY callback; `ETH_CMD_G_AUTONEGO` returns that cache
  without BMCR I/O. Use the supported stopped-driver control to reconcile AN.
- Driver 164-172 updates cached speed/duplex after successful MAC notification.
  Getters at 478-480 / 503-505 return those last published values. Forced
  setters do not immediately update these caches. They are resolved/published
  cache observations, not desired-setting or fresh PHY readback.
- Driver 77-98 / 511-528 routes raw ioctls through mediator custom hooks,
  outside PHY-specific register methods. Both hooks must reject even for the
  owner task. The typed owner route must call shared core validation/procedures
  and direct MAC frame primitives within one terminal batch, avoiding mediator
  recursion and duplicate locking.
- Header 381 documents `bool*` for flow control; driver 491-495 reads the same
  pointer as both `bool*` and `uint32_t*`, and MAC mutation precedes PHY
  advertisement. Reject that argument-width convention. The supported owner
  path uses separately typed MAC Boolean and PHY uint32_t calls and retains
  both results/effects; a later PHY failure cannot erase an accepted MAC effect.
- Base 609-675 uses partly initialized whole-word MMD values and rejects only
  function ADDRESS rather than all invalid enum casts. Keep the shared core's
  initialized values, exact DEVAD31 words `0x001F` / `0x401F`, 16-bit width
  checks, selector/alias protections and one four-frame batch.
- MAC 211-253 frame methods have no MDIO transaction mutex or caller deadline.
  The nominal 1000 us busy-loop allowance is not a wall-clock measurement.
  Keep instance admission before controller acquisition; release the controller
  before waits, later polls or output, including all failure paths.
- Driver start 300-321 changes FSM before AN, START event, link callback and
  timer arming. Raw BMCR/reset invalidation must block both owner start and
  direct framework start, with cached AN true and false. Preserve any earlier
  FSM/event effect and use the existing supported stop/fence cleanup path;
  rejection cannot retroactively make start effect-free.

The `native_rx_quiescence` gate is unchanged. Installed driver, netif/glue,
MAC/PHY and callback storage remain alive until system restart. A controller
lock, STOP or zero callback count cannot authorize reusable deletion while RX
can retain the old mediator/context before application handoff.

## Evidence boundary

Peer/SDK revision and source checks: **PASS** (read-only identity and source
inspection). This audit does not claim executing host tests, consumer tests or
an SDK image link; the final prompt-38 report records their actual commands and
results after integration. Remote CI: **NOT RUN**, first-execution prompt 71.
Physical operation: **NOT RUN**; the P4 execution guard remains disabled.
Reusable installed teardown: **BLOCKED**, `native_rx_quiescence`.
No delegated Git commit, synchronization or stage completion is claimed here.
