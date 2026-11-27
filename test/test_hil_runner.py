"""Test host verdicts, offline replay and the compiled DP8382x producer."""

import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import dp8382x_hil_runner as runner


FIXTURE = None
EXPECTED = ["--target", "host", "--backend", "sim-mdio", "--variant", "DP83826I",
            "--interface", "RMII", "--address", "1"]
REVISION = "0123456789abcdef0123456789abcdef01234567"
EXPECTED_CONTEXT = dict(target="host", backend="sim-mdio", variant="DP83826I",
                        interface="RMII", address=1)
NONCE = "replay-48"


def wire(req, cmd, extra="", **changed):
    """Render literal schema fields for synthetic transcript mutations."""
    common = dict(schema="1", boot="42", req=str(req), type="result", cmd=cmd, op="0",
                  state="terminal", primary="OK", detail="0", cleanup="none", cleanup_detail="0",
                  time_valid="1", time_us=str(req + 1000), deadline_valid="0", deadline_us="0", reason="none")
    common.update(changed)
    return ("@DP8382X " + " ".join(f"{key}={value}" for key, value in common.items()) +
            (" " + extra if extra else "") + "\r\n").encode("ascii")


def passive(**changes):
    """Successful cached-only plan with one independently attested exact rejection."""
    health = ("health=READY initialized=1 successes=9 failures=0 consecutive=0 last_error=OK last_detail=0 "
              "last_ok_valid=1 last_ok_us=42 last_error_valid=0 last_error_us=0")
    transport = "batches=9 attempted=30 completed=30 stopped=0 clock_invalid=0 controller_reusable=1"
    slot = "slot=idle token=0 slot_status=RESULT_NOT_AVAILABLE slot_detail=0 cleanup_owed=0 temporary=0 slot_deadline_us=0"
    settings = ("trust=DIRTY desired_valid=0 desired_mode=unknown desired_modes=0 desired_speed_mbps=0 "
                "desired_duplex=unknown observed_valid=0 observed_stale=1 observed_mode=unknown observed_modes=0 "
                "observed_speed_mbps=0 observed_duplex=unknown")
    identity = (f"nonce={NONCE} library=0.1.0 firmware=unknown tree=unknown sdk=unknown target=host "
                "backend=sim-mdio clock=simulated variant=DP83826I interface=RMII address=1 console=host-stdio baud=0")
    rows = [
        wire(1, "session", identity, type="session"),
        wire(2, "cached.transport", transport), wire(3, "cached.health", health),
        wire(4, "cached.operation", slot), wire(5, "cached.settings", settings),
        wire(6, "registers.write", **(dict(primary="INVALID_PARAM", reason="range") | changes)),
        wire(7, "cached.settings", settings), wire(8, "cached.operation", slot),
        wire(9, "cached.health", health), wire(10, "cached.transport", transport),
    ]
    return rows


def invoke(*args):
    """Call the canonical runner and capture only its local diagnostics."""
    with contextlib.redirect_stdout(io.StringIO()) as out, contextlib.redirect_stderr(io.StringIO()) as err:
        status = runner.main(list(map(str, args)))
    return status, out.getvalue(), err.getvalue()


class RunnerTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.path = Path(directory.name)

    def recorded(self, data, name="run", extra=()):
        source = self.path / "supplied.txt"
        source.write_bytes(data)
        status, out, err = invoke("verify-transcript", source, "--suite", "passive", "--nonce", NONCE,
                                  *EXPECTED, "--run", name, "--out-dir", self.path / "runs", *extra)
        manifest = self.path / "runs" / f"{name}.json"
        return status, out + err, json.loads(manifest.read_text(encoding="utf-8"))

    def test_passive_replays_and_manifest_case_claims_are_checked(self):
        rows = passive()
        # Echo, ANSI help and a valid older reply precede a fresh handshake.
        text = b"session echoed\r\n\x1b[36m> \x1b[0m\r\n" + rows[1] + b"".join(rows) + b"> "
        status, message, data = self.recorded(text)
        self.assertEqual(status, 0, message)
        self.assertEqual(data["scope"], "host_protocol")
        self.assertEqual(data["cases"][1]["primary"], {"code": "INVALID_PARAM", "detail": 0})
        target = self.path / "runs/run.json"
        self.assertEqual(target.with_suffix(".txt").read_bytes(), text)
        self.assertEqual(invoke("verify-artifacts", target)[0], 0)
        data["cases"][1]["primary"]["code"] = "OK"
        target.write_text(json.dumps(data), encoding="utf-8")
        status, _, error = invoke("verify-artifacts", target)
        self.assertEqual(status, 2)
        self.assertIn("case claims", error)

    def test_duplicate_reboot_missing_wrong_order_and_malformed_tail(self):
        rows = passive()
        for name, text in {
            "duplicate": b"".join(rows + [rows[-1]]),
            "reboot": b"".join(rows[:5] + [rows[5].replace(b"boot=42", b"boot=43")] + rows[6:]),
            "missing": b"".join(rows[:-1]),
            "wrong-order": b"".join(rows[:7] + rows[7:9][::-1] + rows[9:]),
            "malformed-tail": b"".join(rows) + b"@DP8382X schema=1\x1b[0m\r\n",
            "bad-separator-tail": b"".join(rows) + rows[-1].replace(b"@DP8382X ", b"@DP8382X\t", 1),
            "partial-tail": b"".join(rows) + b"@DP8382X schema=1",
            "malformed-stale": wire(19, "diagnostics.selftest", "bogus_field=malformed") + b"".join(rows),
            "rollover": rows[0].replace(b"req=1 ", b"req=18446744073709551615 ") + b"".join(rows[1:]),
        }.items():
            with self.subTest(name=name):
                status, message, data = self.recorded(text, name)
                self.assertEqual(status, 2, message)
                self.assertFalse(data["complete"])
                self.assertTrue(data["issues"])

    def test_wrong_firmware_and_truncation_cannot_pass(self):
        text = b"".join(passive())
        status, _, data = self.recorded(text, "wrong-identity", ("--firmware", REVISION))
        self.assertEqual(status, 2)
        self.assertEqual(data["identity"]["firmware"], "unknown")
        self.assertEqual(data["identity"]["firmware_strength"], "unverified")
        status, _, data = self.recorded(text, "bounded", ("--max-bytes", "120"))
        self.assertEqual(status, 2)
        self.assertTrue(data["transcript"]["truncated"])
        self.assertEqual(data["transcript"]["length"], 120)

    def test_schema_one_before_console_identity_was_added(self):
        rows = passive()
        rows[0] = rows[0].replace(b" console=host-stdio baud=0", b"")
        text = b"".join(rows)
        status, message, data = self.recorded(text)
        self.assertEqual(status, 0, message)
        self.assertNotIn("console", data["identity"])
        self.assertNotIn("baud", data["identity"])
        self.assertEqual(invoke("verify-artifacts", self.path / "runs/run.json")[0], 0)
        for option, value in (("--console", "host-stdio"), ("--baud", "0")):
            with self.subTest(option=option):
                status, _, data = self.recorded(text, option[2:], (option, value))
                self.assertEqual(status, 2)
                self.assertIn("firmware identity mismatch", data["issues"][0])

    def test_clock_revival_and_inconsistent_cached_health_cannot_pass(self):
        for name, old, new in (
            ("clock-revival", b"time_valid=1 time_us=1003", b"time_valid=0 time_us=0"),
            ("health-streak", b"consecutive=0", b"consecutive=1"),
            ("health-initialized", b"initialized=1", b"initialized=0"),
            ("cached-deadline", b"deadline_valid=0 deadline_us=0", b"deadline_valid=1 deadline_us=999"),
        ):
            with self.subTest(name=name):
                rows = passive()
                rows[2] = rows[2].replace(old, new)
                status, message, data = self.recorded(b"".join(rows), name)
                self.assertEqual(status, 2, message)
                self.assertFalse(data["complete"])
                self.assertEqual(len(data["issues"]), 1)

    def test_untimed_passive_replay(self):
        rows = [row.replace(f"time_valid=1 time_us={1000 + request} ".encode(), b"time_valid=0 time_us=0 ")
                for request, row in enumerate(passive(), 1)]
        status, message, _ = self.recorded(b"".join(rows))
        self.assertEqual(status, 0, message)

    def test_exact_rejection_requires_unchanged_state_unsaturated_counts_and_clean_status(self):
        rows = passive()
        cases = {
            "transport-changed": b"".join(rows[:-1] + [rows[-1].replace(b"attempted=30", b"attempted=31")]),
            "slot-changed": b"".join(rows[:7] + [rows[7].replace(b"cleanup_owed=0", b"cleanup_owed=1")] + rows[8:]),
            "saturated": b"".join(rows).replace(b"attempted=30", b"attempted=4294967295"),
            "cleanup-failed": b"".join(passive(cleanup="MDIO_TIMEOUT", cleanup_detail="-2")),
            "wrong-reason": b"".join(passive(reason="arity")),
        }
        for name, text in cases.items():
            with self.subTest(name=name):
                status, message, data = self.recorded(text, name)
                self.assertEqual(status, 1, message)
                self.assertEqual(data["cases"][1]["verdict"], "FAIL")

    def test_primary_failure_survives_successful_cleanup_and_malformed_trailer(self):
        text = b"".join(passive(primary="MDIO_ERROR", detail="-43", reason="none", cleanup="OK"))
        status, _, data = self.recorded(text + b"@DP8382X invalid\r\n")
        self.assertEqual(status, 1)
        self.assertFalse(data["complete"])
        self.assertEqual(data["cases"][1]["primary"], {"code": "MDIO_ERROR", "detail": -43})

    def test_known_failures_dominate_incomplete_before_and_after_evidence(self):
        rows = passive()
        status, _, data = self.recorded(rows[0] + wire(2, "cached.transport", primary="MDIO_ERROR",
                                                      detail="-43", cleanup="OK"), "partial-baseline")
        self.assertEqual(status, 1)
        self.assertEqual(data["cases"][0]["primary"], {"code": "MDIO_ERROR", "detail": -43})
        status, _, data = self.recorded(b"".join(rows[:6]) + wire(7, "cached.settings", primary="MDIO_TIMEOUT",
                                                                 detail="-12"), "partial-after")
        self.assertEqual(status, 1)
        self.assertEqual(data["cases"][1]["primary"]["code"], "INVALID_PARAM")
        self.assertIn("MDIO_TIMEOUT/-12", data["cases"][1]["message"])

    def test_dry_run_has_no_effect_and_rejects_bad_options(self):
        with patch.object(runner.subprocess, "Popen", side_effect=AssertionError("dry-run opened a process")):
            status, out, err = invoke("dry-run", "--suite", "passive", *EXPECTED)
        self.assertEqual(status, 0, err)
        self.assertFalse(json.loads(out)["executed"])
        self.assertEqual(list(self.path.iterdir()), [])
        with contextlib.redirect_stderr(io.StringIO()):
            for option in ("nan", "inf", "-1", "0"):
                with self.subTest(option=option), self.assertRaises(SystemExit) as rejected:
                    invoke("run-fixture", "unused", "--suite", "passive", *EXPECTED, "--run", "x",
                           "--out-dir", self.path, "--timeout", option)
                self.assertEqual(rejected.exception.code, 2)
        status, _, _ = invoke("dry-run", "--suite", "passive", *EXPECTED,
                              "--firmware", REVISION, "--firmware-prefix", "0123456")
        self.assertEqual(status, 2)
        with self.assertRaises(runner.ProtocolError):
            runner.expectations(dict(EXPECTED_CONTEXT, address=True), "passive")
        self.assertFalse(list(self.path.iterdir()))

    def test_interruption_preserves_a_nonpassing_pair_and_exit_code(self):
        with patch.object(runner, "read_capture", side_effect=KeyboardInterrupt):
            status, _, data = self.recorded(b"")
        self.assertEqual(status, 130)
        self.assertEqual(data["verdict"], "INCONCLUSIVE")
        self.assertIn("interrupted by operator", data["issues"])
        self.assertEqual(invoke("verify-artifacts", self.path / "runs/run.json")[0], 2)

    def test_missing_fixture_preserves_setup_failure(self):
        status, _, error = invoke("run-fixture", self.path / "missing", "--suite", "passive", *EXPECTED,
                                  "--run", "missing", "--out-dir", self.path / "runs")
        self.assertEqual(status, 2)
        self.assertIn("regular executable", error)
        target = self.path / "runs/missing.json"
        self.assertEqual(target.with_suffix(".txt").read_bytes(), b"")
        self.assertEqual(invoke("verify-artifacts", target)[0], 2)

    def test_live_duplicate_during_closure_invalidates_pass(self):
        rows = passive()
        script = ("import sys\n"
                  f"rows = {rows!r}\n"
                  "for row in rows:\n"
                  "    if not sys.stdin.buffer.readline(): sys.exit(4)\n"
                  "    sys.stdout.buffer.write(row)\n"
                  "sys.stdin.buffer.read()\n"
                  "sys.stdout.buffer.write(rows[-1])\n")
        fixture = runner.FixtureProcess
        with patch.object(runner, "FixtureProcess",
                          side_effect=lambda _, *args: fixture([sys.executable, "-u", "-c", script], *args)):
            status, _, message = invoke("run-fixture", sys.executable, "--suite", "passive", "--nonce", NONCE,
                                        *EXPECTED, "--out-dir", self.path, "--run", "late")
        self.assertEqual(status, 2, message)
        self.assertIn("duplicate record", message)
        self.assertEqual((self.path / "late.txt").read_bytes(), b"".join(rows + rows[-1:]))

    def test_dp83822_compiled_identity_and_copper_replay(self):
        if FIXTURE is None:
            self.skipTest("compiled fixture supplied by cli_hil_runner CTest")
        for grade in ("DP83822I", "DP83822H", "DP83822IF", "DP83822HF"):
            for interface in ("MII", "RMII", "RGMII"):
                name = f"{grade}-{interface}"
                options = ["--variant", grade, "--medium", "copper", "--interface", interface,
                           "--backend", "sim-mdio", "--target", "host", "--address", 1]
                with self.subTest(name=name):
                    status, _, error = invoke("run-fixture", FIXTURE, "--suite", "fixture-operations",
                                              *options, "--fixture-variant", grade, "--run", name,
                                              "--nonce", NONCE, "--out-dir", self.path)
                    self.assertEqual(status, 0, error)
                    manifest = self.path / f"{name}.json"
                    self.assertEqual(invoke("verify-artifacts", manifest)[0], 0)
                    self.assertEqual(runner.verify(manifest)["identity"]["medium"], "copper")
        for variant, interface, medium in (("DP83822IF", "MII", "fiber"), ("DP83822H", "RMII", ""),
                                           ("DP83826I", "RGMII", ""), ("DP83822", "MII", "copper")):
            expected = {**EXPECTED_CONTEXT, "variant": variant, "interface": interface}
            if medium:
                expected["medium"] = medium
            with self.assertRaises(runner.ProtocolError):
                runner.validate_expected_identity(expected)

    def test_selftest_generation_matches_firmware_grade(self):
        if FIXTURE is None:
            self.skipTest("compiled fixture supplied by cli_hil_runner CTest")
        for grade in ("DP83826AI", "DP83826AE"):
            options = [*EXPECTED, "--variant", grade, "--out-dir", self.path]
            status, _, error = invoke("run-fixture", FIXTURE, "--suite", "fixture-operations",
                                      "--nonce", NONCE, *options, "--run", grade, "--fixture-variant", grade)
            self.assertEqual(status, 0, error)
            target = self.path / f"{grade}.json"
            self.assertEqual(runner.verify(target)["cases"][3]["verdict"], "PASS")
            raw = target.with_suffix(".txt").read_bytes()
            changed = raw.replace(b"id2=41268 ", b"id2=41265 ")
            self.assertNotEqual(changed, raw)
            source = self.path / f"{grade}-wrong.input"
            source.write_bytes(changed)
            status, _, error = invoke("verify-transcript", source, "--suite", "fixture-operations",
                                      "--nonce", NONCE, *options, "--run", grade + "-wrong")
            self.assertEqual(status, 1, error)
            evidence = runner.verify(self.path / f"{grade}-wrong.json")
            self.assertEqual(evidence["cases"][3]["verdict"], "FAIL")

    def test_compiled_fixture_and_full_identity(self):
        if FIXTURE is None:
            self.skipTest("compiled fixture supplied by cli_hil_runner CTest")
        status, _, error = invoke("run-fixture", FIXTURE, "--suite", "fixture-operations", "--nonce", NONCE,
                                  *EXPECTED, "--out-dir", self.path, "--run", "fixture",
                                  "--fixture-identity", REVISION, "--firmware", REVISION)
        self.assertEqual(status, 0, error)
        target = self.path / "fixture.json"
        data = json.loads(target.read_text(encoding="utf-8"))
        self.assertEqual(data["identity"]["firmware_strength"], "exact")
        self.assertEqual(data["cases"][2]["restoration"], {"required": True, "verdict": "PASS"})
        self.assertEqual({case["verdict"] for case in data["cases"]}, {"PASS"})
        self.assertEqual(invoke("verify-artifacts", target)[0], 0)

        # Mutate framing and correlation on the compiled producer's transcript.
        original = target.with_suffix(".txt").read_bytes()
        decoder = runner.Decoder()
        records = decoder.feed(original)
        decoder.finish()
        terminal = next(record for record in records
                        if record.cmd == "connection.initialize" and record.state == "terminal")
        line = next(line for line in original.splitlines(keepends=True)
                    if b"cmd=connection.initialize " in line and b"state=terminal " in line)
        admission = next(line for line in original.splitlines(keepends=True)
                         if b"cmd=connection.initialize " in line and b"state=admitted " in line)
        for name, sample in {
            "admission-only": original[:original.index(line)],
            "admitted-cleanup": original.replace(admission, admission.replace(
                b"cleanup=none cleanup_detail=0", b"cleanup=MDIO_ERROR cleanup_detail=-51")),
            "wrong-op": original.replace(line, line.replace(f"op={terminal.op} ".encode(), b"op=99 ")),
        }.items():
            source = self.path / f"{name}.input"
            source.write_bytes(sample)
            status, _, error = invoke("verify-transcript", source, "--suite", "fixture-operations", "--nonce", NONCE,
                                      *EXPECTED, "--out-dir", self.path, "--run", name)
            self.assertEqual(status, 2, error)
        sample = next(line for line in original.splitlines(keepends=True)
                      if b"cmd=diagnostics.selftest " in line and b"state=progress " in line)
        source = self.path / "failed-sample.input"
        source.write_bytes(original[:original.index(sample)] +
                           sample.replace(b"primary=OK detail=0", b"primary=MDIO_ERROR detail=-43"))
        status, _, error = invoke("verify-transcript", source, "--suite", "fixture-operations", "--nonce", NONCE,
                                  *EXPECTED, "--out-dir", self.path, "--run", "failed-sample")
        self.assertEqual(status, 1, error)
        failed = json.loads((self.path / "failed-sample.json").read_text(encoding="utf-8"))
        self.assertFalse(failed["complete"])
        self.assertEqual(failed["cases"][3]["primary"], {"code": "MDIO_ERROR", "detail": -43})


if __name__ == "__main__":
    if len(sys.argv) > 2 and sys.argv[1] == "--fixture":
        FIXTURE = Path(sys.argv[2]).resolve()
        del sys.argv[1:3]
    unittest.main()
