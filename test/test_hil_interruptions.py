"""Exercise interruption at the actual compiled campaign's command boundaries."""

import contextlib
from dataclasses import dataclass, field
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from test_hil_campaign import decoded, row_lines
from test_hil_runner import NONCE, REVISION, runner
from test_hil_serial import Port, arguments as serial_arguments, device_identity, invoke
import dp8382x_hil_artifacts as artifacts
from dp8382x_hil_campaign import all_cases, feature_modules


FIXTURE = None
WORK = Path(__file__).resolve().parents[1] / "build" / "hil-interruption-tests"


def fixture_args(folder, case, name):
    module = feature_modules().get(case, (None, None))[1]
    args = ["run-fixture", str(FIXTURE), "--suite", "campaign", "--case", case,
            "--soak-count", "2", "--soak-duration-ms", "8000", "--timeout", "15",
            "--target", "host", "--backend", "sim-mdio", "--variant", "DP83826I",
            "--interface", "MII" if case in getattr(module, "MII_CASES", ()) else "RMII",
            "--address", "1", "--firmware", REVISION, "--fixture-identity", REVISION,
            "--clock", "simulated", "--console", "host-stdio", "--baud", "0",
            "--operating-mode", getattr(module, "MODES", {}).get(case, "enhanced"),
            "--polling", "exclusive", "--board", "compiled-P52", "--authorization", "offline-test",
            "--nonce", NONCE, "--out-dir", str(folder), "--run", name, "--reinitialize"]
    peer = all_cases()[case].peer
    if peer:
        args += ["--peer", "simulated-peer", "--cable", "simulated-cable", "--peer-profile", peer]
    return args


@dataclass
class Exchange:
    line: bytes
    response: bytearray = field(default_factory=bytearray)


class CutFixture:
    """Replay a compiled command prefix; lose the reply at the selected cut."""

    stderr = b""
    truncated = False
    cleanup_issue = ""

    def __init__(self, tape, cut, capture, admission):
        self.tape = tape
        self.cut = cut
        self.capture = capture
        self.admission = admission
        self.wire = []
        self.reply = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return False

    def send(self, line, deadline=None):
        index = len(self.wire)
        if index > self.cut or line != self.tape[index].line:
            raise AssertionError(f"unexpected write at {index} after cut {self.cut}: {line!r}")
        self.wire.append(line)
        self.reply = bytes(self.tape[index].response)
        if index == self.cut and line.startswith(b"$"):
            raise runner.FixtureError("injected loss after fixture action")

    def read(self, deadline=None):
        if len(self.wire) == self.cut + 1:
            if self.admission:
                self.admission = False
                reply = b"\r\n" + next(line for line in row_lines(self.reply) if b"state=admitted " in line)
                self.capture(reply)
                return reply
            raise runner.FixtureError("injected loss after command")
        if not self.reply:
            raise AssertionError("unexpected read without a recorded command reply")
        reply, self.reply = self.reply, None
        self.capture(reply)
        return reply

    def finish(self, *args):
        raise AssertionError("normal closure reached past the injected cut")


@unittest.skipUnless("--fixture" in sys.argv, "requires the compiled CLI fixture")
class Interruptions(unittest.TestCase):
    def setUp(self):
        WORK.mkdir(parents=True, exist_ok=True)
        directory = tempfile.TemporaryDirectory(dir=WORK)
        self.addCleanup(directory.cleanup)
        self.folder = Path(directory.name)
        provenance = patch.object(runner, "runner_identity",
                                  return_value={"revision": "unknown", "tracked_tree": "unknown"})
        provenance.start()
        self.addCleanup(provenance.stop)

    def run_case(self, case, name):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()) as err:
            status = runner.main(fixture_args(self.folder, case, name))
        path = self.folder / (name + ".json")
        self.assertTrue(path.is_file(), err.getvalue())
        return status, json.loads(path.read_bytes()), path, err.getvalue()

    def compile_tape(self, case):
        tape, boundaries = [], []
        send, read, live_steps = runner.FixtureProcess.send, runner.FixtureProcess.read, runner.live_steps

        def record_send(fixture, line, deadline=None):
            tape.append(Exchange(line))
            return send(fixture, line, deadline)

        def record_read(fixture, deadline=None):
            chunk = read(fixture, deadline)
            tape[-1].response.extend(chunk)
            return chunk

        def record_steps(session):
            for step in live_steps(session):
                boundaries.append((len(tape), session.campaign.completed, step))
                yield step

        with (patch.object(runner.FixtureProcess, "send", record_send),
              patch.object(runner.FixtureProcess, "read", record_read),
              patch.object(runner, "live_steps", record_steps)):
            _, data, path, err = self.run_case(case, case)
        self.assertEqual(data["cases"][0]["verdict"], "PASS", (case, err, data["cases"]))
        self.assertEqual(data, runner.verify(path))
        return tape, boundaries, data

    def test_every_mutating_case_and_restore_boundary(self):
        # The table includes the real prelude, controls with no reply, all primary
        # commands (including destructive reads), and every restore/readback step.
        # Build each row from the actual child and the live decisions for that case.
        from test_hil_soak import assert_soak_required_fields
        cuts = admissions = interventions = preludes = mutations = 0
        for case, spec in all_cases().items():
            module = feature_modules().get(case, (None, None))[1]
            if not spec.mutation or case in getattr(module, "BLOCKED_CASES", ()):
                continue
            with self.subTest(case=case):
                tape, boundaries, positive = self.compile_tape(case)
                if positive["verdict"] == "PASS":
                    raw = (self.folder / (case + ".txt")).read_bytes()
                    mutations += assert_soak_required_fields(self, case, positive, raw)
                start = boundaries[0][0]
                first_end = next(i for i, completed, _ in boundaries if completed == 1)
                self.assertTrue(any(step.cleanup for _, completed, step in boundaries if not completed))
                for at in range(first_end):
                    row = tape[at]
                    if row.line.startswith(b"$"):
                        interventions += 1
                    if at < start:
                        preludes += 1
                    admitted = b"state=admitted " in row.response
                    for with_admission in (False, True) if admitted else (False,):
                        cuts += 1
                        admissions += int(with_admission)

                        def carrier(argv, timeout, limit, capture):
                            nonlocal driver
                            driver = CutFixture(tape, at, capture, with_admission)
                            return driver

                        driver = None
                        with patch.object(runner, "FixtureProcess", carrier):
                            status, result, path, err = self.run_case(case, f"cut-{cuts}")
                        self.assertEqual(status, 2, (case, at, row.line, err))
                        self.assertEqual(driver.wire, [entry.line for entry in tape[:at + 1]])
                        self.assertIn("injected loss", result["issues"][0])
                        self.assertEqual(result["verdict"], "INCONCLUSIVE")
                        self.assertEqual(result["cases"][1]["verdict"], "NOT RUN")
                        self.assertFalse(result["cases"][1]["executed"])
                        if row.line.startswith(b"$"):
                            self.assertEqual(result["campaign"]["interventions"][-1]["outcome"], "failed")
                        # These lost action acknowledgments themselves terminate the
                        # procedure. The last qualified primary still survives.
                        terminal_action = ((case, row.line) in {
                            ("calibration-recovery", b"$raise-calibration\n"),
                            ("cable-autorun", b"$campaign-peer an-100-full\n"),
                        })
                        self.assertEqual(result["soak"]["completed"], int(terminal_action),
                                         (row.line, result["cases"][0]))
                        self.assertEqual(result["soak"]["stop_reason"], "fault")
                        if at < start:
                            self.assertEqual(result["soak"]["attempted"], 0)
                            self.assertEqual(result["soak"]["setup"][-1]["state"], "written")
                        if at == first_end - 1:
                            self.assertEqual(result["cases"][0]["restoration"]["verdict"], "PASS")
                            self.assertEqual(result["cases"][0]["verdict"], "INCONCLUSIVE")
                        self.assertEqual(result, runner.verify(path))
                        # Keep disk use independent of the number of cut points.
                        path.unlink()
                        path.with_suffix(".txt").unlink()
        print(f"{cuts} interrupted prefixes, {admissions} after admission, {interventions} peer/stimulus, "
              f"{preludes} setup, {mutations} field mutations", flush=True)
        self.assertGreater(mutations, 100_000)
        self.assertGreater(cuts, 1000)
        self.assertGreater(admissions, 100)
        self.assertGreater(interventions, 5)
        self.assertGreater(preludes, 10)

    def test_tdr_service_under_full_output_and_cancel(self):
        # These fixture controls schedule owner work before its output can drain.
        for expected, mid, stalls in (
            ("CANCELLED", b"$tick 20\nowner cancel 3\n$tick 512\n", 512),
            ("OPERATION_TIMEOUT", b"$tick 1\n" + b"drv\n" * 28 + b"$tick 4096\n", 4096),
        ):
            with self.subTest(expected=expected):
                stdout = self.fixture_stream(
                    b"configuration apply an 15 none auto 1\n$pause\n$output-stall 4096\n"
                    b"diagnostics cable-start 1 3\n" + mid + b"$output-stall 0\n$resume\n"
                    b"diagnostics cable-stop\ncached operation\nlocal console\n",
                    "--hold-tdr", "--attest-features", "--partner", "down")
                records = decoded(stdout)
                admitted, terminal = [r for r in records if r.cmd == "diagnostics.cable-start"]
                self.assertEqual((admitted.state, terminal.state, terminal.primary, terminal.op),
                                 ("admitted", "terminal", expected, admitted.op))
                self.assertEqual([terminal.fields[k] for k in ("complete", "temporary", "cleanup_owed")],
                                 ["0", "1", "1"])
                self.assertLessEqual(int(terminal.fields["time_us"]), int(admitted.fields["deadline_us"]))
                stop, recovered = [r for r in records if r.cmd == "diagnostics.cable-stop"]
                self.assertNotEqual(stop.op, terminal.op)
                self.assertEqual((recovered.op, recovered.primary, recovered.fields["stopped"],
                                  recovered.fields["cleanup_owed"]), (stop.op, "OK", "1", "0"))
                slot, console = records[-2:]
                self.assertEqual((slot.fields["slot"], slot.fields["temporary"], slot.fields["cleanup_owed"]),
                                 ("idle", "0", "0"))
                self.assertGreaterEqual(int(console.fields["stalled_writes"]), stalls)

    def fixture_stream(self, stream, *flags):
        result = subprocess.run([str(FIXTURE), *flags], input=stream, capture_output=True,
                                timeout=5, check=True)
        self.assertLess(len(result.stdout) + len(result.stderr), 16 * 1024)
        return result.stdout

    def test_stopped_watch_and_stress_preserve_partial_count(self):
        for cmd, frames in ((b"link watch 20 1000 8", 0), (b"test stress 20 1000", 1)):
            with self.subTest(cmd=cmd):
                records = decoded(self.fixture_stream(b"cached transport\n$pause\n" + cmd +
                                                      b"\n$tick 4\ntest stop\n$resume\ntest stats\ncached transport\n"))
                sample, terminal = [r for r in records if r.req == 2]
                self.assertEqual((sample.state, terminal.state, terminal.primary),
                                 ("progress", "terminal", "CANCELLED"))
                self.assertEqual((terminal.fields["rounds"], terminal.fields["requested"]), ("1", "20"))
                self.assertEqual(records[-2].fields["outcome"], "CANCELLED")
                self.assertEqual(int(records[-1].fields["attempted"]) - int(records[0].fields["attempted"]), frames)

    def test_primary_restore_and_close_failures_survive_together(self):
        send, finish = runner.FixtureProcess.send, runner.FixtureProcess.finish
        writes = []

        def fail_writes(fixture, line, deadline=None):
            writes.append(line)
            if line == b"diagnostics bist-read\n":
                send(fixture, b"$fail-next 3\n", deadline)
            elif line == b"diagnostics bist-stop\n":
                send(fixture, b"$fail-next 0\n", deadline)
            return send(fixture, line, deadline)

        def fail_close(fixture, *args):
            finish(fixture, *args)
            raise runner.FixtureError("injected close failure")

        with (patch.object(runner.FixtureProcess, "send", fail_writes),
              patch.object(runner.FixtureProcess, "finish", fail_close)):
            code, data, path, err = self.run_case("bist-both", "three-failures")
        self.assertEqual(code, 1, err)
        first, remaining = data["cases"]
        self.assertEqual(first["primary"], {"code": "MDIO_ERROR", "detail": -43})
        self.assertEqual((first["verdict"], first["restoration"]["verdict"], remaining["verdict"]),
                         ("FAIL", "FAIL", "NOT RUN"))
        self.assertIn("injected close failure", data["issues"])
        self.assertEqual(writes.count(b"diagnostics bist-stop\n"), 1)
        self.assertEqual(sum(line.startswith(b"diagnostics bist-start ") for line in writes), 1)
        self.assertEqual(data, runner.verify(path))

    def test_capture_overflow_stops_at_exact_prefix(self):
        tape, _, good = self.compile_tape("owned-apply")
        target = good["commands"].index("configuration apply an 9 symmetric auto 0")
        # Inject excess bytes into the real pipe-drain accounting while a possibly
        # mutating request is in flight. Use the configured combined stream limit.
        send = runner.FixtureProcess.send
        wire = []

        def exhaust(fixture, line, deadline=None):
            delivered = send(fixture, line, deadline)
            wire.append(line)
            if len(wire) == target + 1:
                fixture._retain(b"x" * (artifacts.MAX_TRANSCRIPT_BYTES + 1), stderr=True)
            return delivered

        with patch.object(runner.FixtureProcess, "send", exhaust):
            status, data, path, err = self.run_case("owned-apply", "overflow")
        self.assertEqual(status, 2, err)
        self.assertEqual(wire, [row.line for row in tape[:target + 1]])
        self.assertTrue(data["transcript"]["truncated"])
        self.assertEqual(data["cases"][1]["verdict"], "NOT RUN")
        self.assertIn("restoration was not proved", data["cases"][0]["message"])
        self.assertLessEqual(data["transcript"]["length"], artifacts.MAX_TRANSCRIPT_BYTES)
        self.assertLessEqual(path.stat().st_size, artifacts.MAX_MANIFEST_BYTES)
        self.assertEqual(data, runner.verify(path))

    def test_publication_fault_preserves_prior_pair_and_separate_outcomes(self):
        _, _, prior, _ = self.run_case("owned-apply", "prior")
        prior_contents = [p.read_bytes() for p in (prior, prior.with_suffix(".txt"))]
        send = runner.FixtureProcess.send

        def failed(fixture, line, deadline=None):
            if line == b"configuration apply an 9 symmetric auto 0\n":
                send(fixture, b"$fail-next 7\n", deadline)
            return send(fixture, line, deadline)

        with (patch.object(runner.FixtureProcess, "send", failed),
              patch.object(artifacts.os, "replace", side_effect=OSError("injected replace fault"))):
            code, result, path, err = self.run_case("owned-apply", "unpublished")
        self.assertNotEqual(code, 0)
        for token in ("injected replace fault", "MDIO_ERROR", "PASS"):
            self.assertIn(token, err)
        self.assertEqual(result["verdict"], "INCONCLUSIVE")
        self.assertFalse(result["complete"])
        self.assertGreater(path.with_suffix(".txt").stat().st_size, 0)
        with self.assertRaises(artifacts.EvidenceError):
            runner.verify(path)
        self.assertEqual(sorted(p.name for p in self.folder.iterdir()),
                         ["prior.json", "prior.txt", "unpublished.json", "unpublished.txt"])
        self.assertEqual([p.read_bytes() for p in (prior, prior.with_suffix(".txt"))], prior_contents)

    def test_serial_loss_during_target_or_restore(self):
        tape, _, reference = self.compile_tape("owned-apply")
        commands = reference["commands"]
        traces = [entry for entry in tape if not entry.line.startswith(b"$")]
        for fault, target in (
                ("short", "configuration apply an 9 symmetric auto 0"),
                ("unknown", "configuration apply an 15 asymmetric mdix 3"),
                ("reboot", "configuration apply an 9 symmetric auto 0"),
                ("incomplete", "configuration apply an 15 asymmetric mdix 3"),
                ("cancel", "configuration apply an 9 symmetric auto 0"),
        ):
            with self.subTest(fault=fault):
                at = commands.index(target)
                port = None

                class InjectedPort(Port):
                    def write(self, line):
                        index = len(self.lines)
                        if index > at or line != traces[index].line:
                            raise AssertionError(f"unexpected write after {fault}: {line!r}")
                        self.lines.append(line)
                        raw = device_identity(b"".join(row_lines(bytes(traces[index].response))))
                        if line == b"local console\n":
                            native = (b" input_queued=0 output_queued=0 lines_queued=0 input_dropped=0 input_gaps=0"
                                      b" lines_dropped=0 lines_truncated=0 uart_stalls=0 output_backpressure=0")
                            raw = b"".join(r.rstrip(b"\r\n") + native + b"\r\n" for r in row_lines(raw))
                        if line == b"application status\n":
                            raw = raw.replace(b"primary=UNSUPPORTED ", b"primary=OK ").replace(
                                b"reason=none\r\n", b"reason=none installed=1 traffic=0 maintenance=1 "
                                b"retired=0 shutdown=0 carrier=0 ip=0.0.0.0 events=0\r\n")
                        if index == at:
                            if fault == "short":
                                return 3
                            if fault == "unknown":
                                raise OSError("injected serial disappearance")
                            if fault == "cancel":
                                raise KeyboardInterrupt
                            if fault == "incomplete":
                                raw = b"@DP8382X schema=1 boot="
                            if fault == "reboot":
                                boot = reference["identity"]["boot"]
                                raw = raw.replace(f"boot={boot} ".encode("ascii"),
                                                  f"boot={boot ^ 1} ".encode("ascii"))
                        self.rx.extend(raw)
                        return len(line)

                    def read(self, size):
                        chunk = bytes(self.rx[:size])
                        del self.rx[:size]
                        return chunk

                def factory(**opts):
                    nonlocal port
                    port = InjectedPort(**opts)
                    return port

                args = serial_arguments(self.folder, fault) + [
                    "--suite", "campaign", "--case", "owned-apply", "--clock", "simulated",
                    "--soak-count", "2", "--soak-duration-ms", "4000", "--operating-mode", "enhanced",
                    "--polling", "exclusive", "--session-timeout", "10", "--duration", "4", "--cleanup-timeout", "3",
                    "--command-timeout", "0.15", "--reinitialize"]
                status, _, err = invoke(args, factory)
                self.assertEqual(status, 130 if fault == "cancel" else 2, err)
                data = runner.verify(self.folder / (fault + ".json"))
                self.assertEqual(port.lines, [entry.line for entry in traces[:at + 1]])
                self.assertEqual((port.opens, port.closes), (1, 1))
                self.assertEqual(data["serial"]["recovery"]["state"], "not_started")
                self.assertEqual(data["cases"][1]["verdict"], "NOT RUN")
                self.assertIn("restoration was not proved", data["cases"][0]["message"])
                self.assertEqual(data["cases"][0]["restoration"]["verdict"],
                                 "INCONCLUSIVE" if target.startswith("configuration apply an 15") else "NOT RUN")
                if fault in ("short", "unknown", "cancel"):
                    self.assertTrue(data["serial"]["framing_uncertain"])
                    self.assertEqual(data["serial"]["writes"][-1]["bytes"], 3 if fault == "short" else None)
                    self.assertEqual(data["commands"], commands[:at])
                else:
                    self.assertEqual(data["commands"], commands[:at + 1])


if __name__ == "__main__":
    for flag in ("--fixture", "--work-dir"):
        if flag in sys.argv:
            at = sys.argv.index(flag)
            value = Path(sys.argv[at + 1]).resolve()
            del sys.argv[at:at + 2]
            if flag == "--fixture":
                FIXTURE = value
            else:
                WORK = value
    unittest.main()
