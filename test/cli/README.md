# CLI host fixture

With `DP8382X_BUILD_TESTS=ON`, the build produces `test/cli/dp8382x_cli_fixture`
(`.exe` on Windows). Its stdin and stdout are byte streams. Each boot binds
the real portable core to a Clause 22 register array, MMD aliases and a small
fixed table of extended words. Strap address 1 answers with the revision-1
DP83826 ID for the selected mode (default ENHANCED), address 0 returns zeros,
and remaining addresses return all ones. I and E return identical words.
Startup initializes without resetting the
PHY and samples link once through the public core. Configuration trust stays
`DIRTY` because the fixture has not applied and verified a desired profile.
Subsequent CLI snapshot commands copy cached values. The fixture models software
behavior only.

| Argument | Effect |
| --- | --- |
| `--identity TOKEN` | Reports a supplied firmware identity. Use 1 to 64 ASCII letters, digits, underscores, dots, colons or hyphens. Default `unknown`. |
| `--variant DP83826I\|DP83826E` | Selects the simulated ordering grade at bind and reports it through `session`, `version` and `local board`. Default `DP83826I`. The runner supplies this option with `--fixture-variant`. |
| `--partner down\|pending\|100-full\|10-half` | Selects simulated partner and link register values. Default `100-full`. |
| `--fail-after N` | Permits N successful physical frames per simulated boot, then fails attempted frames with `MDIO_ERROR`, detail `-43`. Startup frames count. A failed write reports a possible effect. |
| `--reboot-after N` | Reboots once after accepting N complete CLI lines, counting rejected lines and treating CRLF as one boundary. Drops output queued in the old boot and starts the new request count at 1. |
| `--partial-record` | Writes the first machine record's prefix, drops the rest of the record and suppresses all later output. |
| `--deny-maintenance` | Models an active application session, so the owner rejects disruptive commands before PHY I/O. Default grants maintenance for the isolated model. |
| `--no-maintenance` | Models an application without a maintenance mode. Disruptive commands return `UNSUPPORTED`. |
| `--hold-reset` | Leaves an accepted reset bit set to exercise timeout and cleanup debt. |
| `--regress-after-hard` | Moves the simulated clock backwards at the first hard-reset frame. |
| `--trace-resets` | Writes each literal reset register/value pair to stderr. Stdout remains the CLI byte stream. |
| `--trace-writes` | Writes each simulated target address and word to stderr. Secure-ON password words are redacted. |
| `--attest-features` | Supplies synthetic board and fixture proof for the typed command under test. Omit it to verify missing proof rejection. |
| `--mii` | Selects an MII fixture binding for MII-only routes. |
| `--hold-tdr` | Leaves the simulated manual capture pending to exercise deadline and restoration handling. |
| `--campaign basic\|enhanced` | Selects the revision-1 mode ID and prepares a restorable owned AN profile through the real core on startup. |
| `--campaign-peer PROFILE` | Supplies a modeled peer from the campaign case table. In campaign mode, coherent link registers follow the modeled local and peer profiles. |

The fixture intercepts exact lines containing `$pause`, `$resume`, `$tick N`
or `$output-stall N`. They do not consume CLI request numbers. These controls
exist only in the host executable. `$pause` suspends automatic ticks and
`$resume` resumes them. `$tick N` advances N owner ticks of 1 ms each.
While paused, `$fragment TEXT` feeds the text without its line terminator.
The next ordinary input continues that same CLI line after any explicit ticks.
`$output-stall N` makes the next N write offers accept zero bytes. N for those
two controls is decimal 0 through 4096. The following fixture-only controls
also work with CR, LF or CRLF.

| Control | Effect |
| --- | --- |
| `$latch-link-low` | Makes the next BMSR read return historical link-low evidence. |
| `$inject-events` | Sets simulated MISR1/MISR2 high-byte events. The collector consumes them. |
| `$inject-wake` | Latches the synthetic WoL indication for read and W0C tests. |
| `$fail-next N` | Fails the frame after N further successful simulated frames once. N is 0 through 4096. |
| `$before-quiet-floor`, `$at-quiet-floor` | While paused, service one tick at the current hard-reset floor minus 1 µs or exactly at the floor. |
| `$isolate` | Models external disposition and lets the application attest it for `owner abandon`. |
| `$campaign-peer PROFILE` | In campaign mode, changes the simulated connected peer. |
| `$campaign-peer-delayed PROFILE` | In campaign mode, delays the simulated peer signal for six milliseconds of fixture time. |

All other bytes reach the CLI unchanged, including controls with invalid
arguments. The fixture pumps up to 8192 owner ticks at input boundaries and
EOF to cover the 5-second configuration deadline and final output. Staging is
limited to 4096 bytes under backpressure. Capacity exhaustion and undrained
output at EOF exit with status 1. Bad process arguments or setup exit with
status 2.

See the grammar and machine record requirements in [the CLI guide](../../docs/cli.md).
The registered `cli_hil_runner` test drives this executable through the
[console runner](../../docs/hil-runner.md), verifies the artifact pair and
checks admission-only and wrong-token transcripts. `cli_hil_serial` routes
the passive suite through an injected serial backend connected to this fixture
and checks bounded I/O failure paths without using a serial device.
`cli_hil_campaign` covers each executable [observation/mode case](../../docs/hardware-validation.md),
the retained INT/equipment dispositions, per-field evidence mutations,
recovery after simulated failure, and serial/offline replay of the same plan.
`cli_hil_features` uses the compiled owner and core for the
[diagnostic and control cases](../../docs/hardware-validation.md#feature-fixtures).
It checks both direct and admitted receipts, blocked native and source
routes, separate restoration, worst verdicts and per-field evidence
mutations. The simulated event/error inputs are child controls, and the
typed baseline prelude appears before the correlated nonce.

`cli_hil_soak` checks repeated cases, primary and cleanup clock boundaries,
typed telemetry and manifest replay. `cli_hil_interruptions` cuts compiled
command streams during setup, mutation and restoration. It also exercises
short serial writes, failed publication, a held diagnostic under output
backpressure and explicit watch/stress stops.

The registered processor checks inject a refused poll, a temporarily refused
take, a stalled writer, a clock regression during final-sample recording and
an incomplete selftest receipt. The pinned SDK host checks also exercise the
real native stop obligation that can refuse a poll, and assert zero MDIO
frames when typed reads lack accepted identity.

The `cli_native_console_*` tests use the same bounded console queues as the
P4 applications. They exercise an ordered input gap, partial and withheld
sink writes, concurrent task logs between drains, and retained terminal
delivery during restoration. A host sink stands in for the UART.
