"""Exercise the real CLI subprocess. These are software transcripts, not HIL."""
import re
import subprocess
import sys
import unittest


PREFIX = b"@DP8382X "
FIELD = re.compile(rb"[a-z][a-z0-9_]*=[A-Za-z0-9_.:-]+")
FIXTURE = sys.argv.pop(1)


def invoke(data, *options):
    return subprocess.run((FIXTURE, *options), input=data, capture_output=True,
                          check=True, timeout=5)


def run(data, *options):
    return invoke(data, *options).stdout


def records(transcript):
    """Read strict, complete candidate lines for fixture assertions."""
    lines = transcript.split(b"\n")
    found = []
    for index, line in enumerate(lines):
        if not line.startswith(PREFIX):
            continue
        if index == len(lines) - 1 or not line.endswith(b"\r") or len(line) + 1 > 1024:
            raise ValueError("unterminated or oversized candidate record")
        fields = {}
        for item in line[len(PREFIX):-1].split(b" "):
            if not FIELD.fullmatch(item):
                raise ValueError("invalid field")
            key, value = item.decode("ascii").split("=", 1)
            if key in fields:
                raise ValueError("duplicate field")
            fields[key] = value
        found.append(fields)
    return found


def request(rows, number):
    return [row for row in rows if row["req"] == str(number)]


class FixtureTranscript(unittest.TestCase):
    def test_real_cached_views_and_zero_frames(self):
        stream = (b"session A.b:43-Thing\r\ncached transport\n"
                  b"drv\ncached link\r\ncfg\ncaps\ncached operation\n"
                  b"unknown\x00\x08\nlocal quiet 1\n?\ncached transport\n")
        output = run(stream, "--identity", "abc123", "--partner", "10-half")
        reply = records(output)
        self.assertEqual([int(row["req"]) for row in reply], list(range(1, 12)))
        self.assertEqual(len({row["boot"] for row in reply}), 1)
        self.assertEqual(reply[0]["nonce"], "A.b:43-Thing")
        self.assertEqual(reply[0]["firmware"], "abc123")
        self.assertEqual(reply[0]["type"], "session")
        self.assertEqual(reply[2]["health"], "READY")
        self.assertEqual((reply[3]["speed_mbps"], reply[3]["duplex"]), ("10", "half"))
        self.assertEqual((reply[4]["trust"], reply[4]["desired_valid"],
                          reply[4]["observed_valid"]), ("DIRTY", "0", "0"))
        self.assertEqual(reply[5]["valid"], "1")
        self.assertEqual(reply[6]["slot"], "idle")
        self.assertEqual(reply[7]["reason"], "byte")
        self.assertNotIn(b"=== DP8382x CLI Help ===", output)
        self.assertEqual(reply[9]["primary"], "OK")
        for field in ("batches", "attempted", "completed"):
            self.assertGreater(int(reply[1][field]), 0)
            self.assertEqual(reply[1][field], reply[10][field])

    def test_faults_remain_evidence(self):
        failed = records(run(b"drv\ncaps\n", "--fail-after", "0"))
        self.assertEqual((failed[0]["health"], failed[0]["last_error"],
                          failed[0]["last_detail"]), ("UNINIT", "MDIO_ERROR", "-43"))
        self.assertEqual(failed[1]["valid"], "0")
        down = records(run(b"cached link\n", "--partner", "down"))[0]
        self.assertEqual((down["valid"], down["up"], down["resolved"]), ("1", "0", "0"))

    def test_reboot_drops_in_flight_record_and_changes_boot(self):
        for eol in (b"\r", b"\n", b"\r\n"):
            with self.subTest(eol=eol):
                stream = eol.join((b"session first", b"drv", b"session next", b""))
                first, next_boot = records(run(stream, "--reboot-after", "2"))
                self.assertNotEqual(first["boot"], next_boot["boot"])
                self.assertEqual(first["req"], "1")
                self.assertEqual(next_boot["req"], "1")
                self.assertEqual(next_boot["nonce"], "next")

    def test_stalled_owner_delivers_every_terminal_record(self):
        stream = (b"$pause\r\n$output-stall 3\n" + b"drv\n" * 40
                  + b"$tick 3\n$resume\nlocal console\n$bad\n")
        reply = records(run(stream))
        self.assertEqual([int(row["req"]) for row in reply], list(range(1, 43)))
        self.assertTrue(all(row["primary"] == "OK" for row in reply[:41]))
        self.assertEqual(reply[40]["stalled_writes"], "3")
        self.assertEqual(reply[41]["reason"], "command")

    def test_truncated_or_corrupt_records_are_missing_evidence(self):
        with self.assertRaises(ValueError):
            records(run(b"session lost\n", "--partial-record"))
        clean = run(b"session exact\n", "--identity", "archive")
        candidate = next(line for line in clean.splitlines() if line.startswith(PREFIX))
        self.assertEqual(records(b"> " + candidate + b"\r\n"), [])
        with self.assertRaises(ValueError):
            records(candidate + b" boot=2\r\n")
        with self.assertRaises(ValueError):
            records(candidate + b"\x1b[0m\r\n")

    def test_malformed_test_control_reaches_cli_unchanged(self):
        stream = b"$tick \x00\n$" + b"x" * 192 + b"\nver\n"
        rejected, oversized, version = records(run(stream))
        self.assertEqual(rejected["reason"], "byte")
        self.assertEqual(oversized["reason"], "line_length")
        self.assertEqual((version["req"], version["cmd"]), ("3", "version"))

    def test_invalid_line_cannot_execute_a_mutation_suffix(self):
        stream = (b"cached transport\n" + b"x" * 193 + b"\bregisters write c22 24 16\n"
                  b"registers write c22 24 1\x00\b6\n"
                  b"cached transport\nregisters read c22 24\n")
        before, overlong, poisoned, after, led = records(run(stream))
        self.assertEqual((overlong["reason"], poisoned["reason"]), ("line_length", "byte"))
        for field in ("batches", "attempted", "completed"):
            self.assertEqual(before[field], after[field])
        self.assertEqual(led["value"], "0")

    def test_identity_discovery_and_safe_dump(self):
        rows = records(run(b"connection identity\nconnection probe\nconnection discover 0 2\n"
                           b"registers dump c22 0x0c 0x10\nregisters read mmd 31 0x0467\n"
                           b"registers read mmd 3 0x14\nregisters read c22 0x1f\n"))
        cached, probe, *more = rows
        for receipt in (cached, probe):
            self.assertEqual((receipt["id1"], receipt["id2"], receipt["mode"]),
                             ("8192", "41265", "enhanced"))
        discovery = [row for row in more if row["cmd"] == "connection.discover"]
        self.assertEqual([(r["address"], r["id1"], r["candidate"]) for r in discovery[:-1]],
                         [("0", "0", "0"), ("1", "8192", "1"), ("2", "65535", "0")])
        self.assertEqual((discovery[-1]["rows"], discovery[-1]["attempted"]), ("3", "6"))
        dump = [row for row in more if row["cmd"] == "registers.dump"]
        self.assertEqual([(r["address"], r["skipped"]) for r in dump[:-1]],
                         [(str(a), "1") for a in range(12, 17)])
        self.assertEqual((dump[-1]["primary"], dump[-1]["attempted"]), ("OK", "0"))
        self.assertEqual([int(r["value"]) for r in more[-3:]], [0xA567, 0x0002, 0x0000])
        default_scan = records(run(b"connection discover\n"))
        self.assertEqual([r["address"] for r in default_scan[:-1]], [str(a) for a in range(8)])
        self.assertEqual(default_scan[-1]["attempted"], "16")

    def test_mmd_dump_reports_atomic_reads_and_skips(self):
        stream = (b"registers dump mmd 31 0x0467 0x0468\n"
                  b"registers dump mmd 31 0x0c 0x0e\n")
        first, second, summary, *skips, skipped = records(run(stream))
        for row, address, literal in ((first, 0x0467, 0xA567), (second, 0x0468, 0xA568)):
            self.assertEqual((row["req"], row["type"], row["state"]), ("1", "sample", "progress"))
            self.assertEqual((row["devad"], row["valid"], row["attempted"]), ("31", "1", "4"))
            self.assertEqual((int(row["address"]), int(row["value"])), (address, literal))
            self.assertEqual(row["deadline_us"], summary["deadline_us"])
        self.assertEqual((summary["primary"], summary["attempted"], summary["complete"]), ("OK", "8", "1"))
        self.assertEqual([(r["address"], r["skipped"], r["attempted"]) for r in skips],
                         [(str(a), "1", "0") for a in range(12, 15)])
        self.assertEqual((skipped["primary"], skipped["skipped"], skipped["attempted"]), ("OK", "3", "0"))

    def test_profile_verify_and_guarded_mutations(self):
        reply = records(run(
            b"configuration apply an 0xf both auto 3 wait\nconfiguration verify\n"
            b"configuration apply forced 10 half mdi 1\nconfiguration desired\n"
            b"configuration observed\nregisters bmcr 0x1000 0x1000\n"
            b"configuration invalidate\nconnection identity\n"))
        for req in (1, 2, 3):
            admission, result = [r for r in reply if r["req"] == str(req)]
            self.assertEqual((admission["state"], admission["primary"]), ("admitted", "IN_PROGRESS"))
            self.assertEqual((result["state"], result["primary"], result["op"]),
                             ("terminal", "OK", admission["op"]))
        self.assertEqual(reply[1]["an_complete"], "1")
        for row in reply[6:8]:
            self.assertEqual((row["trust"], row["mode"], row["speed_mbps"], row["duplex"], row["pair"]),
                             ("VERIFIED", "forced", "10", "half", "mdi"))
        self.assertEqual(reply[8]["effect"], "applied")
        self.assertEqual((reply[9]["primary"], reply[10]["stale"]), ("OK", "1"))
        writes = records(run(b"registers write c22 24 16\nregisters write mmd 31 24 128\n"
                             b"registers modify 31 24 2 2\nregisters read c22 24\n"))
        self.assertEqual([r["effect"] for r in writes[:3]], ["applied"] * 3)
        self.assertEqual(writes[3]["value"], str(0x0082))

    def test_rejection_preserves_core_state_and_frames(self):
        reply = records(run(
            b"cached transport\nconfiguration validate an 15 both auto 1 2 20040000\n"
            b"configuration validate an 15 both auto 1 2 2004\n"
            b"configuration apply forced 10 half auto 5\n"
            b"connection discover 0 32\nregisters read c22 0x8000\n"
            b"registers read mmd 3 0x3c\nregisters read c22 0x0d\n"
            b"registers write c22 0x1f 0x8000\nregisters bmcr 0x8000 0x8000\n"
            b"registers dump mmd 31 0 32\n"
            b"configuration apply an 8 none auto 1\nregisters write c22 24 0\n"
            b"configuration invalidate\ncached transport\nconfiguration desired\n",
            "--deny-maintenance"))
        by_req = {r["req"]: r for r in reply}
        self.assertEqual(by_req["2"]["primary"], "OK")
        for req in ("3", "4"):
            self.assertEqual(by_req[req]["primary"], "INVALID_CONFIG")
        for req in range(5, 12):
            self.assertEqual(by_req[str(req)]["primary"], "INVALID_PARAM")
        for req in ("12", "13", "14"):
            self.assertEqual(by_req[req]["primary"], "BUSY")
        for field in ("batches", "attempted", "completed"):
            self.assertEqual(by_req["1"][field], by_req["15"][field])
        self.assertEqual(by_req["16"]["valid"], "0")

    def test_partial_prefix_and_failed_write_are_retained(self):
        baseline = int(records(run(b"cached transport\n"))[0]["attempted"])
        failed = records(run(b"connection discover 1 1\n",
                             "--fail-after", str(baseline + 1)))
        self.assertEqual((failed[0]["id1_valid"], failed[0]["id2_valid"], failed[0]["detail"]),
                         ("1", "0", "-43"))
        self.assertEqual((failed[1]["rows"], failed[1]["errors"], failed[1]["primary"]),
                         ("1", "1", "MDIO_ERROR"))
        dump = records(run(b"registers dump c22 2 3\n", "--fail-after", str(baseline + 1)))
        self.assertEqual((dump[0]["valid"], dump[1]["valid"], dump[2]["errors"]),
                         ("1", "0", "1"))
        write = records(run(b"registers write mmd 31 24 0x10\n",
                            "--fail-after", str(baseline + 3)))[0]
        self.assertEqual((write["primary"], write["detail"], write["effect"], write["release"]),
                         ("MDIO_ERROR", "-43", "possible", "OK"))

    def test_negotiation_wait_cannot_report_admission_as_success(self):
        admission, result = records(run(b"configuration apply an 8 none auto 1 wait\n",
                                        "--partner", "down"))
        self.assertEqual((admission["state"], admission["primary"]), ("admitted", "IN_PROGRESS"))
        self.assertEqual((result["state"], result["primary"], result["an_complete"]),
                         ("terminal", "OPERATION_TIMEOUT", "0"))
        self.assertEqual((result["op"], result["deadline_us"]),
                         (admission["op"], admission["deadline_us"]))

    def test_packed_next_page_apply_preserves_cleanup_on_timeout(self):
        admission, result = records(run(b"configuration apply an 15 both auto 1 2 20040003\n"))
        self.assertEqual((admission["state"], admission["primary"]), ("admitted", "IN_PROGRESS"))
        for field in ("boot", "req", "op", "deadline_us"):
            self.assertEqual(result[field], admission[field])
        self.assertEqual((result["state"], result["primary"], result["core_status"], result["sdk_error"]),
                         ("terminal", "OPERATION_TIMEOUT", "OPERATION_TIMEOUT", "0"))
        self.assertEqual((result["np_requested"], result["np_complete"], result["np_owed"], result["cleanup_owed"]),
                         ("1", "0", "1", "1"))

    def test_link_observation_latch_and_cached_failure(self):
        before, live, partner, cached, sampled, failed, stale, after = records(run(
            b"cached transport\n$latch-link-low\nlink read 8\nlink partner 1\n"
            b"link cached 8\ncached transport\n$fail-next 8\nlink read 8\n"
            b"link cached 8\ncached transport\n"))
        for row in (live, partner, cached):
            self.assertEqual((row["valid"], row["up"], row["resolved"], row["speed_mbps"], row["duplex"]),
                             ("1", "1", "1", "100", "full"))
        self.assertEqual((live["observation"], partner["observation"], cached["observation"]),
                         ("up", "wrong_policy", "up"))
        self.assertEqual((live["bmsr_first"], live["bmsr_current"], partner["bmsr_first"]), ("32", "36", "36"))
        self.assertEqual((live["advertised_modes"], live["partner_valid"], live["partner_modes"]), ("15", "1", "8"))
        self.assertEqual((failed["primary"], failed["observation"], failed["valid"]),
                         ("MDIO_ERROR", "management_failure", "0"))
        self.assertEqual((stale["valid"], stale["stale"], stale["observation"], stale["resolved"]),
                         ("1", "1", "stale", "0"))
        self.assertEqual(int(sampled["attempted"]) - int(before["attempted"]), 20)
        self.assertEqual(int(sampled["batches"]) - int(before["batches"]), 4)
        self.assertEqual(int(after["attempted"]) - int(sampled["attempted"]), 9)
        self.assertEqual(int(after["completed"]) - int(sampled["completed"]), 8)

    def test_pending_and_down_do_not_project_resolved_mode_or_pause(self):
        for state in ("pending", "down"):
            with self.subTest(state=state):
                live, cached = records(run(b"link read\nlink cached\n", "--partner", state))
                for row in (live, cached):
                    self.assertEqual(row["observation"], state)
                    self.assertEqual((row["resolved"], row["speed_mbps"], row["duplex"], row["pause_valid"],
                                      row["partner_valid"]), ("0", "0", "unknown", "0", "0"))

    def test_event_failure_preserves_consumed_prefix(self):
        before, failed, partial, after, collected, cached, final = records(run(
            b"cached transport\n$inject-events\n$fail-next 3\nlink collect\ncached events\n"
            b"cached transport\nlink collect\ncached events\ncached transport\n"))
        self.assertEqual((failed["primary"], failed["detail"]), ("MDIO_ERROR", "-43"))
        self.assertEqual((failed["event_mask"], failed["misr1"], failed["misr2"]), ("7", "42240", "0"))
        self.assertEqual((partial["valid"], partial["stale"], partial["event_mask"], partial["misr1"]),
                         ("0", "1", "7", "42240"))
        for row in (collected, cached):
            self.assertEqual((row["primary"], row["event_mask"], row["misr1"], row["misr2"]),
                             ("OK", "63", "0", "23040"))
        self.assertEqual(int(after["attempted"]) - int(before["attempted"]), 4)
        self.assertEqual(int(after["completed"]) - int(before["completed"]), 3)
        self.assertEqual(int(final["attempted"]) - int(after["attempted"]), 6)
        self.assertEqual(int(final["batches"]) - int(before["batches"]), 2)

    def test_link_field_routes(self):
        phycr, link, events = records(run(b"registers read c22 0x19\nlink read\nlink collect\n"))
        self.assertEqual((phycr["primary"], phycr["address"], phycr["value"]), ("OK", "25", "32769"))
        self.assertEqual((link["primary"], link["bmsr_first"], link["bmsr_current"]), ("OK", "36", "36"))
        self.assertEqual((events["primary"], events["event_mask"], events["physts"]), ("OK", "63", "21"))

    def test_reset_distinctions_and_local_end(self):
        result = invoke(b"link read\nbegin none\nlink cached\ninit pcs\n"
                        b"registers write c22 24 16\nrecovery restart\nregisters read c22 24\n"
                        b"recovery reset hard\nregisters read c22 24\n"
                        b"recovery reinitialize none\nconnection end\nconnection probe\n", "--trace-resets")
        rows = records(result.stdout)
        self.assertEqual(result.stderr.splitlines(), [b"reset 00 8000", b"reset 1f 4000", b"reset 1f 8000"])
        for number, kind in ((2, "none"), (4, "pcs"), (6, "restart"), (8, "hard"), (10, "none")):
            admitted, terminal = request(rows, number)
            self.assertEqual((admitted["state"], admitted["primary"]), ("admitted", "IN_PROGRESS"))
            self.assertEqual((terminal["state"], terminal["primary"], terminal["op"]),
                             ("terminal", "OK", admitted["op"]))
            self.assertEqual((terminal["requested_reset"], terminal["effective_reset"]), (kind, kind))
            self.assertEqual(terminal["reset_accepted"], "0" if kind == "none" else "1")
            self.assertEqual((terminal["persistent_restored"], terminal["link_valid"]), ("1", "0"))
        self.assertEqual(request(rows, 3)[0]["observation"], "stale")
        self.assertEqual(request(rows, 7)[0]["value"], "16")
        self.assertEqual(request(rows, 9)[0]["value"], "0")
        self.assertEqual(request(rows, 11)[0]["primary"], "OK")
        self.assertEqual(request(rows, 12)[0]["primary"], "NOT_BOUND")

    def test_lifecycle_rejections_are_bus_silent(self):
        stream = (b"cached transport\nconnection initialize none\nrecovery reset pcs\n"
                  b"recovery restart\nrecovery reinitialize\nconnection end\nowner abandon\n"
                  b"owner start initialize hard\nowner cancel 0\nrecovery reset restart\n"
                  b"link watch 0 100\ntest stress 1 0\ncached transport\n")
        result = invoke(stream, "--deny-maintenance", "--trace-resets")
        denied = records(result.stdout)
        self.assertEqual(result.stderr, b"")
        self.assertEqual([row["primary"] for row in denied[1:8]], ["BUSY"] * 7)
        self.assertEqual([row["primary"] for row in denied[8:12]], ["INVALID_PARAM"] * 4)
        for key in ("batches", "attempted", "completed"):
            self.assertEqual(denied[0][key], denied[-1][key])

        before, unavailable, after, verify, verified = records(run(
            b"cached transport\nrecovery restart\ncached transport\nowner start verify\n", "--no-maintenance"))
        self.assertEqual((unavailable["state"], unavailable["primary"]), ("terminal", "UNSUPPORTED"))
        for key in ("batches", "attempted", "completed"):
            self.assertEqual(before[key], after[key])
        self.assertEqual((verify["state"], verify["primary"]), ("admitted", "IN_PROGRESS"))
        self.assertEqual((verified["primary"], verified["mutation_completed"]), ("OK", "0"))

    def test_cancel_after_accepted_reset_and_reapply_separately(self):
        # Fixture setup takes its own token 1. Token 2 is the first command.
        result = invoke(b"$pause\n$output-stall 2\nrecovery reset pcs\n$tick 8\n"
                        b"owner status\nconnection end\nowner cancel 2\n$resume\n"
                        b"owner result 2\nrecovery reinitialize none\n", "--trace-resets")
        rows = records(result.stdout)
        self.assertEqual(result.stderr.splitlines(), [b"reset 00 8000"])
        admitted, cancelled = request(rows, 1)
        self.assertEqual((admitted["state"], admitted["op"]), ("admitted", "2"))
        self.assertEqual((cancelled["state"], cancelled["primary"], cancelled["op"]),
                         ("terminal", "CANCELLED", "2"))
        self.assertEqual((cancelled["mutation_completed"], cancelled["cleanup_owed"]), ("1", "1"))
        self.assertEqual(request(rows, 2)[0]["slot"], "running")
        self.assertEqual(request(rows, 3)[0]["primary"], "BUSY")
        self.assertEqual(request(rows, 4)[0]["primary"], "OK")
        queried = request(rows, 5)[0]
        self.assertEqual((queried["origin_req"], queried["op"], queried["outcome"]), ("1", "2", "CANCELLED"))
        restore_admission, restored = request(rows, 6)
        self.assertNotEqual(restore_admission["op"], cancelled["op"])
        self.assertEqual((restored["primary"], restored["cleanup_owed"]), ("OK", "0"))

    def test_nonce_during_reset_and_fragmented_cancel_with_stalled_output(self):
        result = invoke(b"$pause\n$output-stall 100\nrecovery reset pcs\n$tick 8\n"
                        b"cached transport\nsession P54A\ncached transport\nowner status\n"
                        b"$fragment owner can\n$tick 1\ncel 2\n$tick 4\n"
                        b"$output-stall 0\n$resume\nowner result 2\n"
                        b"recovery reinitialize none\nlocal console\n", "--trace-resets")
        self.assertEqual(result.stderr.splitlines(), [b"reset 00 8000"])
        rows = records(result.stdout)
        self.assertEqual(len({row["boot"] for row in rows}), 1)
        self.assertEqual([int(row["req"]) for row in rows], [1, 2, 3, 4, 5, 6, 1, 7, 8, 8, 9])
        admission, before, nonce, after, status, cancel, terminal, saved, restore, restored, console = rows
        self.assertEqual((nonce["type"], nonce["nonce"], nonce["primary"]), ("session", "P54A", "OK"))
        for field in ("attempted", "completed", "batches"):
            self.assertEqual(before[field], after[field])
        self.assertEqual((admission["state"], admission["op"], status["slot"], status["token"]),
                         ("admitted", "2", "running", "2"))
        self.assertEqual((cancel["cmd"], cancel["primary"], cancel["origin_req"], cancel["op"]),
                         ("owner.cancel", "OK", "1", "2"))
        self.assertEqual((terminal["state"], terminal["primary"], terminal["op"], terminal["reset_accepted"],
                          terminal["mutation_completed"], terminal["cleanup_owed"]),
                         ("terminal", "CANCELLED", "2", "1", "1", "1"))
        self.assertEqual((saved["origin_req"], saved["op"], saved["outcome"]), ("1", "2", "CANCELLED"))
        self.assertEqual((restore["state"], restore["op"], restored["state"], restored["op"],
                          restored["primary"], restored["persistent_restored"], restored["cleanup_owed"]),
                         ("admitted", "3", "terminal", "3", "OK", "1", "0"))
        # Cancellation ran on the first of four stalled ticks. The first
        # resumed tick is 4,000 us later and is already outside this window.
        elapsed = int(terminal["time_us"]) - int(cancel["time_us"])
        self.assertGreaterEqual(elapsed, 0)
        self.assertLess(elapsed, 4000)
        self.assertGreater(int(console["stalled_writes"]), 4)

    def test_reset_timeout_and_recovery_failure_are_separate(self):
        result = invoke(b"recovery reset hard\nrecovery reinitialize none\nowner status\n"
                        b"cached transport\nlink watch 1 10\ntest stress 1 10\ncached transport\n",
                        "--hold-reset", "--trace-resets")
        rows = records(result.stdout)
        self.assertEqual(result.stderr.splitlines(), [b"reset 1f 8000"])
        _, reset = request(rows, 1)
        _, restore = request(rows, 2)
        for terminal in (reset, restore):
            self.assertEqual(terminal["primary"], "OPERATION_TIMEOUT")
            self.assertEqual(terminal["cleanup_owed"], "1")
        self.assertNotEqual(reset["op"], restore["op"])
        for number in (5, 6):
            rejected, = request(rows, number)
            self.assertEqual((rejected["primary"], rejected["detail"]), ("BUSY", "5"))
        self.assertEqual(request(rows, 4)[0]["attempted"], request(rows, 7)[0]["attempted"])

    def test_accepted_hard_reset_obeys_exact_quiet_floor(self):
        rows = records(run(b"$pause\nrecovery reset hard\n$tick 7\ncached transport\n"
                           b"$before-quiet-floor\ncached transport\n$at-quiet-floor\n"
                           b"cached transport\n$resume\n"))
        _, terminal = request(rows, 1)
        before, below, at = (request(rows, number)[0] for number in (2, 3, 4))
        self.assertEqual(terminal["primary"], "OK")
        self.assertEqual((before["attempted"], before["batches"]), (below["attempted"], below["batches"]))
        self.assertEqual(int(at["attempted"]) - int(below["attempted"]), 1)
        self.assertEqual(int(at["batches"]) - int(below["batches"]), 1)

    def test_hard_reset_with_regressed_anchor_blocks_followup_wire(self):
        rows = records(run(b"cached transport\nrecovery reset hard\nowner status\ncached transport\n"
                           b"connection discover 1 1\ncached transport\n", "--regress-after-hard"))
        _, terminal = request(rows, 2)
        self.assertEqual((terminal["primary"], terminal["reset_accepted"], terminal["reset_complete"],
                          terminal["clock_invalid"], terminal["cleanup_owed"]),
                         ("INVALID_CONFIG", "1", "0", "1", "1"))
        self.assertEqual(request(rows, 3)[0]["cleanup_owed"], "1")
        before, after = (request(rows, number)[0] for number in (4, 6))
        self.assertEqual((before["clock_invalid"], before["attempted"]), ("1", after["attempted"]))
        self.assertEqual(request(rows, 5)[-1]["primary"], "INVALID_CONFIG")

    def test_abandon_requires_separate_disposition_before_local_end(self):
        rows = records(run(b"$pause\nrecovery reset pcs\n$tick 8\nowner cancel 2\n$resume\n"
                           b"cached transport\nowner abandon\nconnection end\n$isolate\n"
                           b"owner abandon\ncached transport\nconnection end\n"))
        acquired = request(rows, 1)[-1]
        self.assertEqual((acquired["primary"], acquired["cleanup_owed"]), ("CANCELLED", "1"))
        self.assertEqual([request(rows, number)[0]["primary"] for number in (4, 5, 6, 8)],
                         ["BUSY", "BUSY", "OK", "OK"])
        for key in ("batches", "attempted", "completed"):
            self.assertEqual(request(rows, 3)[0][key], request(rows, 7)[0][key])

    def test_explicit_poll_does_one_owner_step_and_keeps_origin_result(self):
        rows = records(run(b"$pause\ncached transport\nowner start initialize none\n$tick 1\n"
                           b"owner status\nowner poll 3\nowner poll 2\n$tick 1\n"
                           b"cached transport\n$resume\nowner result 2\n"))
        self.assertEqual(request(rows, 3)[0]["slot"], "running")
        self.assertEqual(request(rows, 4)[0]["primary"], "TOKEN_MISMATCH")
        poll = request(rows, 5)[0]
        self.assertEqual((poll["primary"], poll["op"], poll["origin_req"], poll["poll_status"]),
                         ("OK", "2", "2", "IN_PROGRESS"))
        before, after = (request(rows, number)[0] for number in (1, 6))
        self.assertEqual(int(after["batches"]) - int(before["batches"]), 1)
        self.assertEqual(int(after["attempted"]) - int(before["attempted"]), 2)
        admitted, terminal = request(rows, 2)
        queried = request(rows, 7)[0]
        self.assertEqual((admitted["op"], terminal["op"], queried["op"]), ("2", "2", "2"))
        self.assertEqual((queried["origin_req"], queried["outcome"]), ("2", "OK"))

    def test_active_operations_survive_full_console(self):
        rows = records(run(b"$pause\n$output-stall 2000\nowner start initialize none\n$tick 1\n"
                           b"connection end\n" + b"drv\n" * 40 + b"$tick 30\n$output-stall 0\n$resume\n"
                           b"owner result\ncached transport\n"))
        admitted, terminal = request(rows, 1)
        self.assertEqual((admitted["state"], terminal["state"], terminal["primary"]), ("admitted", "terminal", "OK"))
        self.assertEqual(terminal["op"], admitted["op"])
        self.assertEqual(request(rows, 2)[0]["primary"], "BUSY")
        for number in range(2, 45):
            self.assertEqual(len(request(rows, number)), 1)

    def test_finite_watch_and_stress_budget(self):
        rows = records(run(b"cached transport\nlink watch 3 30 8\ncached transport\n"
                           b"test stress 3 30\ntest stats\ncached transport\n"))
        before, watched, after = (request(rows, number)[0] for number in (1, 3, 6))
        self.assertEqual(before["attempted"], watched["attempted"])
        self.assertEqual(int(after["attempted"]) - int(watched["attempted"]), 3)
        self.assertEqual(int(after["batches"]) - int(watched["batches"]), 3)
        for number, attempted in ((2, "0"), (4, "3")):
            run_rows = request(rows, number)
            self.assertEqual([row["round"] for row in run_rows if row["state"] == "progress"], ["1", "2", "3"])
            self.assertEqual((run_rows[-1]["state"], run_rows[-1]["primary"]), ("terminal", "OK"))
            self.assertEqual((run_rows[-1]["rounds"], run_rows[-1]["attempted"], run_rows[-1]["managed"]),
                             ("3", attempted, "0"))
        stats = request(rows, 5)[0]
        self.assertEqual((stats["origin_req"], stats["rounds"], stats["attempted"], stats["outcome"]),
                         ("4", "3", "3", "OK"))

    def test_stress_stop_and_first_fault_stop_the_run(self):
        stopped = records(run(b"cached transport\n$pause\ntest stress 20 1000\n$tick 4\n"
                              b"test stop\n$resume\ncached transport\n"))
        self.assertEqual(request(stopped, 2)[-1]["primary"], "CANCELLED")
        self.assertEqual(request(stopped, 3)[0]["primary"], "OK")
        self.assertEqual(int(request(stopped, 4)[0]["attempted"]) - int(stopped[0]["attempted"]), 1)
        faulted = records(run(b"cached transport\n$fail-next 0\ntest stress 5 100\ncached transport\n"))
        before, after = (request(faulted, number)[0] for number in (1, 3))
        self.assertEqual((request(faulted, 2)[-1]["primary"], request(faulted, 2)[-1]["detail"]),
                         ("MDIO_ERROR", "-43"))
        self.assertEqual(int(after["attempted"]) - int(before["attempted"]), 1)
        self.assertEqual(after["completed"], before["completed"])


if __name__ == "__main__":
    unittest.main()
