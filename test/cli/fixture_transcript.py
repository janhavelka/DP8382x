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


if __name__ == "__main__":
    unittest.main()
