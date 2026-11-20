# Changelog

## Unreleased

- Reject live CLI work on clock regression, freeze admitted configuration
  through its owner and retain its result. Stop a scan or dump when the owner
  cannot account for a successful row's frames.

- Route CLI identity, discovery, typed profile configuration and safe register
  access through the existing owners. Preserve partial samples and mutation
  effects, verify stopped SDK cache reconciliation and keep peers quarantined
  when an attempted reset lacks a usable silence anchor.

- Add the portable example CLI, strict byte parsing, cached owner queries and
  schema 1 session records. Keep terminal output across stalled writes and test
  the byte stream with the real core and simulated MDIO.

- Publish a reported power event's POR silence, and withdraw the native owner's
  earlier board readiness, before `prepareBoard` can reject the call. The basic
  P4 example now checks for an unexpected STOP after journal confirmation and
  keeps a pre-install PHY whose final journal copy is unconfirmed.

- Check the full reset and SMI quiet-time allowance before the native board
  owner asserts RESET_N. An attempted assertion now gets a release attempt
  even on failure, with independent primary and cleanup receipts.

- Preserve the basic P4 example's terminal result across delivery contention
  and distinguish historical lifecycle errors and unavailable link evidence.

- Fixed P4 example startup and event handling, retained operation effects on
  failure, and verified the netif MAC before DHCP. Raw TX now rejects invalid
  frame descriptors and requires verified framework recovery after cached DOWN.

- Added raw frame handoff to the P4 owner example with fixed RX/TX storage,
  generation checks at submission and an explicitly enabled single test frame.

- Added the ESP32-P4 application owner example with explicit 100/full admission,
  revocable session generations, maintenance after release and retained results.
  Native applications can evaluate current link and MAC publication together.

- Add the native ESP32-P4 ordinary Ethernet example: explicit netlist pin checks,
  default-closed board admission, DHCP, cached PHY/link/IP reporting, one finite
  stop/start cycle and retained installed resources. Expose passive native
  health; keep the unpowered PHY rail and physical networking as open gates.

- Correct the third native review: stop on same-poll reset or failed shared board
  readiness, quarantine unexplained P4 controller activity, and restore pre-GPIO
  admission without waiving POR silence. Preserve deadline/poll/admission causes
  and transaction release evidence; document cleanup attempts and shared reset ownership.

- Correct the second native review: recover transient link observations, preserve
  peer quiet floors without failing completed board readiness, and continue native
  cleanup after event failure or forbidden uninstall. Expired stop admission has
  no new effects; owner deadlines remain separate from actual SDK errors.

- Correct all 17 native adapter audit findings: retry incomplete stops/retirement,
  retain contended completion and cancellation evidence, use independent start
  cleanup, close admission after forbidden uninstall, and repair DOWN before STOP.
  Separate SDK/portable failures, withdraw stale running UP, project resolved pause,
  validate callback/period bounds and exclude controller peers during installation.

- Recheck the 17-item audit follow-up against current sources; remove drifted
  API declaration/guide copies, correct timing/source/native/variant guidance,
  and assign compact results, admission and harness/timing work to their owners.

- Preserve read-only BISCR Power Mode bit 8 during 10M compliance setup, with
  an independent wire-value regression; consolidate the repeated private feature
  phase predicate without changing admission behavior.

- Revalidated all 30 repository audit records and 197 promptset findings against
  current code, prompts and sources, with dated responses in each audit.
  Corrected stale SDK-base, IRQ-mux, initialization-replay and CLI-discovery
  guidance; retained implemented fixes and explicit source/physical gates.

- Independent prompts 37-39 audit: made shared board/initialization silence an
  exclusive controller reservation, counted explicit discovery through the core
  transport path, distinguished timed lock failure from immediate contention, and
  applied per-call timing bounds to SDK reconciliation and MAC/PHY flow policy.
  Failed board readiness stays closed after adapter destruction.

- Prompt 39: completed native PHY-entry and conditional timing conformance tables.
  Notification and start receipts survive callback contention; late accepted
  notifications/events preserve effects while reporting timeout. Added cached
  native link evidence, link-period overflow validation and conservative reset
  completion anchors. Expanded actual-driver regressions, including 100Hz ticks;
  SDK link and physical timing qualification remain separate.

- Prompt 38: added SDK-aware AN/forced configuration and explicit public cache
  reconciliation, shared initialization silence, raw/collector owner routing and
  narrow BMCR maintenance. Added separately typed MAC/PHY flow control with
  retained partial effects; direct framework starts after invalidation now retain
  FSM/event effects for cleanup, including gate contention. Extended real-driver
  tests and P4 fixture while preserving the installed RX teardown gate.

- Prompt 37: added the native ESP-IDF v6.1 PHY owner, explicit board readiness,
  direct MAC batches, typed initialization/AN configuration, shared terminal
  AN restart, qualified link notifications, rejecting raw ioctl hooks and bounded
  discovery. Added hash-checked real-driver/netif-glue host tests and a linked P4
  fixture. Stop/fence failures preserve evidence; installed resources remain
  retained under the unresolved RX-quiescence teardown gate.

- Independent prompt 36 audit: quarantined link refreshes mark historical data
  stale; configuration stops on an observed active reset instead of restoring
  verified trust. Collision entry/exit also stops on reset in its RMW read.
  Primary failures and accepted-write effects remain retained.

- Prompt 36 integration review: all direct/vendor-alias reset observations now
  invalidate current evidence and interrupt dependent operations, preserving
  frozen results. Quarantined EEE refreshes retain explicitly stale evidence.
  Primary closure remains blocked by required source-gated branches.

- Audit prompts 34-35: retain quality reset/mode evidence and stale MSE status;
  complete analog handoff to later owned restoration; clear electrical debt on
  external reset; distinguish reset-only effects from stimulus activation.
  Stop maintenance after an interleaved reset observation before dependent writes.

- Added persistent analog/electrical profiles with legal split-field transitions,
  finite compliance/collision sessions and conditional calibration recovery.
  Shared reset/replay retains partial effects and explicit cleanup; unsupported
  100M compliance and gain mapping remain source-gated.

- Added bounded quality/TX10/TX100 and hardware link-loss profiles, shared reset
  restoration, raw MSE snapshots and single-collector polarity evidence.
  Physical detector timing and MSE interpretation remain unqualified.

- Audit prompts 32-33: defer initialized readiness through owned pin/interface
  restoration; propagate pin-owned LED1 clock uncertainty into IRQ/WoL views;
  require live MII/RMII evidence before LED2/3 or LED2 IRQ/WoL routing. Add
  failure, cancellation, collector and mode-mismatch regressions.

- Added bounded MII/RMII, clock, compatibility, repeater-side and isolation
  profiles with collector-routed RCSR readback and shared reset/power replay.
  Validate wiring, packet/clock allowances and pin ownership before I/O; retain
  partial effects on failure. Isolation composes independent software power intent.
  Clock-mux and RCSR-status interpretation gates remain explicit.

- Added bounded exact-part LED/GPIO profiles, complete admitted sources and
  shared blink/stretch, reserved-safe force/polarity/mux updates, passive evidence
  and shared reset/power restoration. IRQ/WoL/clock owners cannot steal pins;
  the IRQ source now configures/restores its own mux after disabling delivery.
  Contradictory routes stay gated for Q12.

- Audit prompts 30–31: stop WoL mutation immediately after an independently
  observed mismatch, preserve disabled WoL route ownership during IRQ/EEE
  admission, and clear current validity after a failed requested word read.
  Add regression coverage for each case.

- Added exact DP83826I Wake-on-LAN configuration, asymmetric MAC/Secure-ON
  packing, CRC/pulse/level controls, bounded acknowledgement and status collection.
  Quiesced detector updates preserve pending W0C evidence and restore owned
  credentials/routes before IRQ and power policy. Physical Q3 remains unqualified.

- Added bounded forced TX LPI entry/exit, bit 12-only RMW/readback, retained
  REQUESTED/UNCERTAIN evidence and OP-9 dwell protection without occupying a job.
  Stop preserves unrelated EEE uncertainty; MAC drain and wake readiness remain
  application-owned. Independent failure/restoration tests and owner example added.

- Added exact opt-in DP83826I EEE setup/disable, two whole-word source checks,
  one HARD reset and shared owned-state replay. Explicit EEE ownership upgrades
  initialization to HARD; forced LPI is a separate explicit session.
- Added bounded EEE capability/local/partner/status acquisition, DEVAD-qualified
  collector retention, passive snapshots, coherent negotiation validity and
  independent recipe/failure/replay tests. Wire/control Q1 remains unqualified.

- Fixed interrupt/power operations accepting obsolete identity, selector or control
  evidence after observations between polls. Preserve contradictory RMW prefixes
  beside transport failures and require other owned policies to remain verified.
- Corrected IRQ/power admission error precedence and added independent regression
  coverage for immediate invalidation, partial reads and cross-policy restoration.

- Added opt-in active-sleep policy, explicit IEEE power request/wake and PLL
  control with retained partial observations, single-write blocked-wake behavior
  and shared reset restoration. All BMCR writers use owned software intent.
- Added live PLL preparation before non-HARD initialization, independent failure
  regressions and a linked power owner example. Deep-power-down remains rejected
  pending exact-part exit evidence; physical clock/power qualification is NOT RUN.

- Added complete opt-in interrupt policy, pin-conflict validation, retained test
  start/stop and bounded deferred service through the existing collector.
- Added selector-at-consumption history, pending half-full counter requests,
  shared reset replay, independent failure regressions and executed owner example.
  Physical IRQ delivery remains unqualified.

- Added exact DP83826I manual cable capture: SNLA428 six-run/subset recipes,
  typed raw A/B controls, freshness-qualified per-word results, explicit reread
  and separately budgeted HARD restoration through the existing owner.
- Added pure source-implied cable interpretation, independent literal/failure
  tests and an executed portable raw-capture consumer. Physical accuracy remains
  unqualified; calibration recovery retains its later owner.

### 2026-11-15

- Audited prompts 25-26 independently against current source and original TI
  tables. Added multi-run failure/cancellation and partial-reacquisition
  regressions; no production behavior correction was required.

- Added explicit cable autorun policy with separate desired/readback/trust,
  verified disable handoff, shared reset replay and read-only verification.
- Added bounded single-run autorun raw acquisition with per-word partial evidence
  and explicit unknown freshness/coherence, plus independent tests and an executed
  portable owner example. Hardware trigger timing and cable accuracy remain unqualified.

- Audited prompts 23-24 independently. Corrected MII line-transmit ordering,
  required exact identity before BIST stop writes, retained dwell collector
  activity/loss in typed BIST evidence, and replayed all saved session fields
  after HARD recovery from a partially completed stop. Added regressions.

- Implemented prompt 24 bounded BIST start/acquire/stop, all documented mode and
  packet/IPG controls, single-collector partial counter evidence and shared
  verified restoration. Added independent failure/encoding tests and portable
  owner example. Exact interval/PASS and physical evidence remain source-gated.

- Implemented prompt 23 loopback start/stop sessions for every documented path
  and legal MII/RMII speed, with explicit maintenance/fixture admission, bounded
  restart/relink, retained temporary-state barriers and shared verified restoration.
- Added exact digital recipe/HARD exit, reverse FIFO save/restore, independent
  mode/sequence/failure tests and a linked maintenance-owner example. Source
  conflicts and physical packet/electrical qualification remain explicit.

- Audited prompts 21-22 independently. Fixed initialization retaining stale
  readiness/configuration after valid active-reset readback followed by release
  or timing failure; added regressions for reset checks and completion waits.
  Corrected initialization guide bounds to distinguish profile replay.

- Implemented prompt 22 startVerify: finite identity/VOD/owned-profile checks
  in the existing slot, with no semantic writes, automatic recovery or cleanup
  barrier clearance. Retain raw mandatory-state evidence and the first mismatch
  through later errors; observed reset invalidates readiness even on late or
  failed release. Restoration reuses the existing initialization/replay path.
- Added literal verification/failure/restoration regressions and a linked owner
  example retaining a failed restoration and an explicitly admitted retry.

- Implemented prompt 21 passive managed health: UNINIT/READY/DEGRADED/OFFLINE,
  cached snapshots, saturating batch counts and retained signed errors/timestamps.
  OFFLINE never blocks explicit attempts; raw diagnostics cannot clear a managed
  failure streak. Statistics reset clears only cumulative health totals.
- Added health transition/failure/clock/restoration tests and a linked portable
  cached-health example; physical wire health remains unqualified.

- Rechecked every independent promptset-audit finding against current code and
  source contracts; recorded individual dispositions and corrected remaining
  health, discovery, metadata, lifetime and source-citation instructions.
- Fixed BMCR power-pin readback being echoed into software power-down during
  link configuration. Active-profile preflight now rejects an existing power-down
  indication; later pin changes cannot latch software intent. Added independent
  AN/forced/reset-replay regressions.
- Kept native reusable teardown gated until RX quiescence is proved, with
  explicit retained resources and acceptance cases in prompts 37 and 42.

### 2026-11-13

- Added `scripts\idf.cmd` and `scripts/idf.ps1`, which run `idf.py` with the pinned ESP-IDF v6.1
  installed side by side in `C:\esp\v6.1`, isolated from PlatformIO's ESP-IDF
  5.5. Prompt 37, finding G10-1 and AGENTS now point to it.
- Unified author, copyright and contact metadata: THYMOS Solution s.r.o.,
  Jan Havelka, info@thymos.cz. `library.json` gains the owner `authors`, which
  `scripts/generate_version.py` now requires exactly.

### 2026-11-11

- Fast-forwarded the prompts 1-20 execution branch into `main`. Until version 0
  is finished, all work happens directly on `main` with no new branches.

### 2026-11-10

- Implemented prompt 20 typed event/counter acquisition through the existing
  collector, with complete and partial cached views, explicit coalescing,
  FCSCR saturation/overflow/lower bounds and bus-silent host-history discard.
  Removed unqualified RECR summation; raw acquisition remains available.
  Added five host suites, arithmetic boundary tests and a linked cached renderer.
  Physical counter/RMII interpretation and SDK evidence remain separate gates.

- Implemented prompt 19 bounded live link observation, passive snapshots and
  retained change evidence through the existing destructive collector. Added
  forced-mode gates, IEEE pause resolution/cross-checks, coherent ten-read
  acquisition, historical failure timestamps and conservative cached-age bounds.
  Added nine Release suites and a linked cached-status example. SDK notifications,
  managed health and physical qualification remain separate work.

### 2026-11-08

- Audited prompts 13-18 independently. Preserved interrupted-verification errors
  alongside mismatch evidence, separated reset completion from AN replay, retained
  possible destructive-read/write effects from malformed receipts, and counted
  completed next-page loads even when later transport/deadline checks fail.
  Expanded verification, page-grammar and partial-evidence regressions.

- Implemented prompt 18 forced 10/100 half/full and complete coupled crossover,
  robust/manual routing, mirror and extended-duplex configuration/replay. Added
  seven Release suites and a linked forced owner example. Fixed robust-disable
  ordering on forced-to-AN transitions, ignored AN command handling in forced
  readback, and actual observed-tuple publication on complete mismatches.

- Implemented prompt 17 negotiated advertisement/pause, verified desired/observed
  settings and shared initialization replay. Added finite typed Clause 28 page
  exchange, consumed-page evidence, transient NP restoration and eleven focused
  Release suites. Generic and 100/full owner examples execute in the portable
  consumer. Physical handshake Q10 and SDK support remain NOT RUN.

- Implemented prompt 16 HARD/RESTART through the existing initialization slot,
  with persistent shared-bus quiet floors, literal PHYRCR commands, complete
  restoration and external-reset/POR reconciliation. Malformed command receipts
  retain possible effects and silence obligations. Added six reset suites and
  a linked owner example that carries silence across cancellation and teardown.

- Implemented prompt 15 initialization with NONE/PCS, literal reset and mandatory
  VOD write/verification, one caller-driven operation slot and lifetime tokens.
  Added finite poll/deadline handling, partial strap/identity evidence, immediate
  cancellation, retained results and a separate restoration barrier/operation.
  Added six Release suites and linked owner/restoration examples. SDK timing,
  link profiles, managed health and physical qualification remain separate work.

### 2026-11-07

- Implemented prompt 14's exact-address two-frame identity acquisition, partial/
  rejected snapshots and bus-silent foundational capabilities. Revision-1 admission
  distinguishes BASIC/ENHANCED without guessing I/E suffix. Failed probes block
  maintenance mutations; changed identity invalidates old device observations.
  Added five independent Release suites and a linked probe example.

- Implemented prompt 13: complete named register/mask catalog, explicit destructive
  acquisition with retained partial samples/history-loss evidence, atomic MISR
  enable updates and source-specific WoL W0C composition. Added nine independent
  access suites and a linked evidence example. Cached readers perform no I/O.
- Applied COMMON's prerequisite audit: explicit access policy and shared test
  harness, corrected quarantine reports, simpler transport bounds, operation/native
  contracts and future prompts. Added source-implied EEE mappings and SNLA428 TDR
  evidence while keeping physical qualification separate. Five-frame MMD RMW is
  inferred from state retention in SNLS647I 8.3.11.1, not a printed sequence or
  physically verified behavior. Raw access does not complete typed features.

### 2026-11-01

- Implemented prompt 12's atomic vendor-MMD read/write and stable LEDCR RMW,
  with one canonical direct/alias access policy and complete prefix accounting.
  Original-PDF review establishes the minimum five-frame nonincrementing RMW;
  updated the earlier eight-frame design, added phase-failure/ownership regressions
  and linked MMD examples. EEE mapping, mixed access and hardware timing remain gated.

- Implemented prompt 11's portable binding, safe Clause 22 reads and restricted
  LEDCR maintenance writes through one bounded terminal batch. Added staged
  outputs, signed diagnostic retention, release/quarantine and clock-fault
  handling, five host suites and a linked direct-read consumer example.
  Explicitly resolved pre-initialization write admission for the safe LEDCR
  subset; selectors, destructive accesses and typed feature procedures remain
  gated. SDK and physical timing qualification remain NOT RUN.

### 2026-10-29

- Fresh prompts 9–10 audit corrected header counts and CTest instructions so
  zero registered tests fail, and distinguished CMake 3.16 target support from
  guarded CTest 3.26 commands. Replaced a stale planning-only verification
  link. No portable or version behavior changed.

- Added prompt 10's `library.json` release-version truth, strict deterministic
  `sync`/read-only `check`, generated public `Version.h` and temporary-fixture
  tests. The portable consumer uses the committed header without generation;
  firmware Git identity and SDK/package metadata remain separate later work.

- Added prompt 9's header-only C++17 portable foundation: Status helpers,
  passive configuration/transport values, pure link validators and selected
  source-proved register masks. Added four registered host test suites, isolated
  header builds, a separate tests-OFF consumer and a Doxygen checker with two
  rejected negative fixtures. Device behavior and SDK support remain planned.

- Independently audited prompt 8: made invalidation state-specific without
  reopening terminal results, bound example owners to one PHY and disabled
  owner copying/moving, and corrected the native terminal status journal.

- Completed prompt 8's cooperative contract: one retained operation slot,
  lifetime tokens, finite primary/cleanup budgets, owner-assisted disposition,
  acyclic restoration and exact portable/native design examples. Added eight
  executable timing probes and qualified clock-return lag; no operation engine
  or successful hardware stub was introduced.

- Independently audited prompt 7: corrected BMSR example words, error timestamp
  validity, clock-error precedence and health accounting for admission/release
  failures. Clarified callback reports and simplified the plain-owner examples.

- Completed prompt 7's foundational value and direct-call contracts: exact
  Status/details, identity/revision admission, borrowed batch/deadline values,
  non-copyable/non-movable ownership, passive health and staged output semantics.
  Mapped all 35 feature families and 52 procedures to typed responsibilities;
  recorded live sibling/TI evidence and independent review. Cooperative lifetime
  contracts remain prompt 8; no future facade methods or headers were shipped.

- Completed prompt 6 portable/native ownership design: one shared chip core,
  terminal MDIO batches, protected raw entry, SDK cache reconciliation, single
  collector, notification failure handling and controller-wide reset silence.
  Re-audited current peers and pinned SDK sources; specified required native
  lifecycle corrections and qualified base reuse. Added a hash-checked poisoned
  SDK-register encoding probe, API/example notes and independent review evidence.
  Driver, SDK-link and physical qualification remain future work.

### 2026-10-28

- Fresh independent prompts 4–5 audit corrected split amplitude updates and
  restoration to keep every intermediate code legal, with revised finite
  budgets. Added exact field/access and procedure-ownership oracles and rejected
  malformed optional disposition text. Diagnostic source facts remain unchanged.

- Completed prompt 5 primary source closure: all 79 detailed entries and 17
  opaque recipe records, five blocked EEE associations, 52 procedures and
  independent power/WoL/pin/electrical vectors. Added exact action ownership,
  source-gate propagation and scope/owner guards; corrected MAC byte numbering
  during cross-review. Kept nominal timings, unresolved access/exit conflicts,
  later-family boundaries and physical qualification distinct from support.

- Completed prompt 4 diagnostic/event/counter research: 34 detailed register
  records, three exact recipe records, 16 procedures and independent PDF-derived
  tests. Added narrowly checked overlapping write actions, retained raw/partial
  evidence rules, fixture plans and explicit unresolved silicon gates.
- Fresh prompt-3 audit corrected VOD_CFG3 access and removed its false conflict
  and validator exception, narrowed two nominal timings to reset, clarified
  next-page framing, and closed alias/procedure coverage gaps with new tests.
- Completed prompt 3's DP83826I core silicon research: authoritative JSON ledger,
  schema validator and independent literal tests; identity, access, reset, link,
  crossover, next-page and pause procedures; complete feature inventory ownership
  for prompts 4–5. Recorded original-PDF conflicts and conservative gates without
  claiming driver implementation or physical qualification.
- Completed prompt 2's live read-only comparison of sibling CLIs, HIL runners,
  examples, package/version/documentation checks and prompt workflows. Recorded
  exact revisions, a broader path inventory, installed tool availability and
  future file/consumer owners. No buildable driver or hardware result was added.
- Completed prompt 1's live, read-only comparison of seven sibling drivers and
  FieldCore-node API, health, lifecycle and owner behavior. Recorded exact source
  revisions, registered tests and planned DP8382x decisions. No driver or
  hardware support was added.

### 2026-10-26

- Refined execution instructions and completed ownership, lifecycle, feature,
  runner and installed-package requirements across the 80-stage promptset.
  Retained exact-source evidence and independent acceptance checks; all stages
  remain unstarted.
- Extracted pinned TI RTOS, MCU+, Linux/Zephyr and ESP-IDF source into four
  detailed evidence catalogues. Added concrete register, field, procedure,
  timing and regression inputs to 60 prompts, with mandatory exact-datasheet
  comparison and correction rules for all 80. Recorded confirmed source defects
  and unresolved identity/address conflicts without inventing hardware evidence.
- Audited prompt workload and replaced the 34 broad stages with 80 complete
  prompts numbered 1 through 80. Split source research, feature procedures,
  SDK/CLI/runner work and family qualification into scoped outcomes with their
  own tests, documentation and checkpoints. Updated execution/dependency and
  coverage maps; all implementation remains NOT STARTED. The sole first-set
  remote-CI checkpoint is now prompt 71 (formerly 29).
- Corrected prompt/source guidance for WoL W0C and mixed-register writes,
  consumed-event retention, remote-fault restrictions, active sleep, loopback,
  family interfaces and ESP-IDF ownership/state/pause limitations. Simplified
  short operations, shared variant records and local tool scope; retained named
  EEE-address and A-generation strap/EMC evidence conflicts.
- Require a new branch before every new promptset execution, retained through
  all stages and resumed sessions; do not reuse main or an earlier set's branch.
- Deepened all implementation prompts with source-backed sibling conventions,
  selected PHY Status/health/function names, concrete file responsibilities,
  failure cases, per-stage Doxygen/general documentation and model selection by
  task complexity. Implementation remains unstarted.
- Expanded AGENTS guidance from sibling conventions, with one branch per
  implementation execution, authorized commit/sync checkpoints per prompt and
  coherent intermediate unit, and a single first-set remote-CI checkpoint.
- Extended the promptset to require complete documented chip functionality,
  broad manufacturer-first public-source research and exhaustive register,
  field and programming-procedure coverage beyond the first board's use case.
- Audited sibling chip-library structure, ownership, APIs, health, examples,
  CLI, Python runners, tests, packaging and CI, plus FieldCore-node promptsets.
- Initially added a 34-stage implementation promptset with shared engineering, CLI,
  verification and fresh sibling-audit requirements, coverage and progress records.
- Made the requested portable C++17 core explicit alongside ESP-IDF framework
  reuse, generic 10/100 operation and separate application/board ownership.
- Recorded PHY-specific implementation risks and per-variant qualification gates.

This change prepares implementation; it adds no driver, firmware or HIL result.

### 2026-10-24

- Created the DP8382x workspace using the sibling library directory conventions.
- Archived official TI device specifications and supporting technical documents,
  with source URLs, retrieval timestamps, checksums and searchable text.
- Recorded official TI and Espressif source references and licensing boundaries.
- Defined DP83826I-first scope, general 10/100 operation, ESP32-P4 integration and
  separate application policy for 100/full operation.
- Added local reference integrity and documentation-link verification.

No driver release or hardware qualification is associated with this setup.
