"""Exercise the real CLI subprocess. These are software transcripts, not HIL."""
import re
import subprocess
import sys
import unittest


PREFIX = b"@DP8382X "
FIELD = re.compile(rb"[a-z][a-z0-9_]*=[A-Za-z0-9_.:-]+")
FIXTURE = sys.argv.pop(1)


def run(data, *options):
    return subprocess.run((FIXTURE, *options), input=data, capture_output=True,
                          check=True, timeout=5).stdout


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


if __name__ == "__main__":
    unittest.main()
