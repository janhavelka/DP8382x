# CLI host fixture

With `DP8382X_BUILD_TESTS=ON`, the build produces `test/cli/dp8382x_cli_fixture`
(`.exe` on Windows). Its stdin and stdout are byte streams. Each boot binds
the real portable core to a Clause 22 register array, MMD aliases and a small
fixed table of extended words. Strap address 1 answers with the enhanced
DP83826 ID, address 0 returns zeros, and remaining addresses return all ones.
Startup initializes without resetting the
PHY and samples link once through the public core. Configuration trust stays
`DIRTY` because the fixture has not applied and verified a desired profile.
Subsequent CLI snapshot commands copy cached values. The fixture models software
behavior only.

| Argument | Effect |
| --- | --- |
| `--identity TOKEN` | Reports a supplied firmware identity. Use 1 to 64 ASCII letters, digits, underscores, dots, colons or hyphens. Default `unknown`. |
| `--partner down\|100-full\|10-half` | Selects simulated partner and link register values. Default `100-full`. |
| `--fail-after N` | Permits N successful physical frames per simulated boot, then fails attempted frames with `MDIO_ERROR`, detail `-43`. Startup frames count. A failed write reports a possible effect. |
| `--reboot-after N` | Reboots once after accepting N complete CLI lines, counting rejected lines and treating CRLF as one boundary. Drops output queued in the old boot and starts the new request count at 1. |
| `--partial-record` | Writes the first machine record's prefix, drops the rest of the record and suppresses all later output. |
| `--deny-maintenance` | Models an active application session, so the owner rejects disruptive commands before PHY I/O. Default grants maintenance for the isolated model. |

The fixture intercepts exact lines containing `$pause`, `$resume`, `$tick N`
or `$output-stall N`. They do not consume CLI request numbers. These controls
exist only in the host executable. `$pause` suspends automatic ticks and
`$resume` resumes them. `$tick N` advances N owner ticks of 1 ms each.
`$output-stall N` makes the next N write offers accept zero bytes. N for those
two controls is decimal 0 through 4096. Controls work with CR, LF or CRLF.
All other bytes reach the CLI unchanged, including controls with invalid
arguments. The fixture pumps up to 8192 owner ticks at input boundaries and
EOF to cover the 5-second configuration deadline and final output. Staging is
limited to 4096 bytes under backpressure. Capacity exhaustion and undrained
output at EOF exit with status 1. Bad process arguments or setup exit with
status 2.

See the grammar and machine record requirements in [the CLI guide](../../docs/cli.md).
