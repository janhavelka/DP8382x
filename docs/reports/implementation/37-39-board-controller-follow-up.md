# Native board reset and shared-controller recovery

This adapter correction follows the synchronized prompt 40 checkpoint
`abf2c0b16e226353ada1da89001f8043bca4234c` on `main`. It covers
`Phy::prepareBoard`, the public board callback allowance, shared-quarantine
disposition and their native regressions. The source inputs are SNLS647I
§7.6, the pinned ESP-IDF v6.1 P4 MAC at `fff9895c` and the
[native readiness and lifecycle contracts](../../esp-idf.md#readiness-and-lifecycle).
Three independent assignments covered reset timing, shared-controller
recovery and a final code review. The lead checked the behavior, public
contract and builds.

## Confirmed findings

- **Reset admission.** The earlier code could assert RESET_N with enough time
  for the assertion alone. The complete preflight includes both qualified pin
  callbacks, the assertion interval, the 2 ms post-release SMI silence and
  tick rounding for both waits. A rejected attempt preserves the existing
  board admission and POR quiet floor when timing and controller ownership are
  known. The callback default of 1,000 µs is an application allowance. The
  actual scheduler, GPIO timing and RC pulse still require measurement.
- **Earliest assertion.** The selected delayed-clock reset procedure waits
  100 µs after both clock stabilization and POR completion. Checking the two
  old limits independently could assert at the POR bound itself. The shared
  POR floor and the later earliest assertion now have separate meanings.
- **Failure after assertion.** Once the assertion callback is invoked, the
  owner attempts release once, even on callback error or budget exhaustion.
  It retains the release quiet floor and keeps the bus FAILED_CLOSED after a
  failed sequence. When the primary and cleanup both fail, the lifecycle
  journal retains both causes. An attempted release alone supplies no
  evidence of a qualified reset pulse.
- **Controller quarantine.** The original regression checked refusal only.
  The extended test overlaps a running observation with a known outside
  controller read. It checks that the peer is denied during quarantine, the
  original transport remains well-formed, and the outside call completes.
  The application stops the owner, accounts for the outside caller and
  establishes idle before clearing quarantine. Starting before reconciliation
  is rejected. Reconciliation performs zero MDIO transfers and explicit restart
  produces a new timestamped link observation. The pinned P4 MAC has no public
  idle fence. The [recovery procedure](../../esp-idf.md#recovering-a-quarantined-shared-controller)
  describes when the owner must retain resources and use system restart.
- **Expired entry and CONTROL recovery.** A later independent audit found that
  an already expired call returned before publishing the reported POR floor.
  See the [third auditor follow-up](40-p4-network-example.md#third-auditor-follow-up)
  for that correction, the recovery step for an owner in CONTROL and the
  deferred minor items.
- **Observation detail codes.** Several `NOT_INITIALIZED` cases share detail
  zero. [Prompt 76](../../prompts/76-docs-api-audit.md) now owns their stable
  caller-visible distinctions and rendering tests.

## Verification on Windows

| Layer | Result |
| --- | --- |
| Pinned ESP-IDF driver and adapter | PASS. 355/355 Release and 355/355 UBSan-trap CTests. `board_deadline_domains`, `board_reset_window` and `shared_quarantine` execute at 100, 333 and 1,000 Hz. Both builds also executed all 63 basic-example cases. |
| Portable and consumer | PASS. 242/242 Release host cases and 2/2 separate tests-off consumer cases. |
| Pinned P4 builds | PASS. Basic Ethernet, `adapter_link` and the raw owner all linked through `scripts/idf.cmd`. Board access remained disabled. |
| Source and documentation | PASS. 103 Python tests, reference hashes and local links, register ledger, version generator check, 291 Doxygen HTML pages and Git whitespace checks. |
| Physical and remote CI | NOT RUN. GPIO waveform, controller idle, actual callback and scheduling times, link, DHCP and traffic require an identified fixture. First-set remote CI belongs to prompt 71. |

Precommit P4 binaries identified `abf2c0b` with the adapter edits present.
The basic image SHA-256 was `1c64fca425ac6baaf68fe16ed6600f28e781348574b7cbf087dbd8ba0db0e18a`.
The adapter fixture SHA-256 was `7105252bbb300be0e1a45910c974a2ef506392dde857adbc00499b3415d34455`.
See the [basic build guide](../../../examples/esp_idf/basic/README.md) and
[adapter fixture instructions](../../../test/esp_idf/adapter_link/README.md) for reproduction.
