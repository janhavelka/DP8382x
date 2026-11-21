"""Exercise bounded repetitions and verify every correlated snapshot against the compiled producer."""

from contextlib import contextmanager
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile
import threading
import time
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from test_hil_campaign import assert_required_fields, decoded, row_lines
from test_hil_runner import EXPECTED, NONCE, REVISION, invoke, runner
from test_hil_serial import (
    Port, RECOVERY, arguments as serial_arguments, device_identity, invoke as serial_invoke, recovery_reply,
)
from dp8382x_hil_artifacts import EvidenceError, aggregate
from dp8382x_hil_protocol import ProtocolError


ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "build" / "hil-soak-tests"
FIXTURE = os.environ.get("DP8382X_CLI_FIXTURE")


def soak_replay(data, records):
    """Evaluate mutated wire records with the same repetition and session owners as live capture."""
    campaign = data["campaign"]
    nonce = data["commands"][0].split(" ", 1)[1]
    session = runner.CampaignSession(nonce, data["expectations"], campaign["declaration"],
                                     campaign["interventions"], data["soak"]["plan"])
    setup = runner.SetupReplay(session)
    try:
        for record in records:
            if not setup.accept(record):
                session.accept(record)
        session.finish()
    except ProtocolError:
        return "INCONCLUSIVE"
    summary = session.campaign.summary()
    if session.setup != data["soak"]["setup"] or any(data["soak"][key] != value for key, value in summary.items()):
        return "INCONCLUSIVE"
    return aggregate({**data, **session.evidence()})


def assert_soak_required_fields(test, name, data, raw):
    return assert_required_fields(test, name, data, raw, replay_fn=soak_replay, include_setup=True)


def arguments(name, count=2, duration_ms=4000):
    return ["--suite", "campaign", "--case", name, *EXPECTED, "--operating-mode", "enhanced",
            "--polling", "exclusive", "--board", "compiled-soak", "--authorization", "offline-soak",
            "--firmware", REVISION, "--clock", "simulated", "--console", "host-stdio", "--baud", "0",
            "--soak-count", str(count), "--soak-duration-ms", str(duration_ms)]


class SoakPlanTests(unittest.TestCase):
    def test_recovery_requires_an_outstanding_full_write_and_the_fresh_identity(self):
        for fault, outcome in (("timeout", "continuous"), ("no-handshake", "unproven"),
                                ("changed-boot", "failed"), ("damaged-tail", "failed"), ("short", "not_started")):
            with self.subTest(fault=fault), tempfile.TemporaryDirectory() as directory:
                class RecoveringPort(Port):
                    def write(port, line):
                        port.lines.append(line)
                        if line == f"session {NONCE}\n".encode():
                            if fault != "no-handshake":
                                port.rx.extend(port.rows[0])
                        elif line == f"session {RECOVERY}\n".encode():
                            port.rx.extend(recovery_reply(boot=43 if fault == "changed-boot" else 42,
                                                          req=2 if fault == "no-handshake" else 3))
                            if fault == "damaged-tail":
                                port.rx.extend(b"@DP8382X bad")
                        elif line == b"local console\n":
                            if fault == "short":
                                return 1
                        else:
                            raise AssertionError(line)
                        return len(line)

                    def read(port, size):
                        chunk = bytes(port.rx[:size])
                        del port.rx[:size]
                        return chunk

                args = serial_arguments(Path(directory), fault) + [
                    "--suite", "campaign", "--case", "observe", "--operating-mode", "enhanced",
                    "--polling", "exclusive", "--clock", "simulated", "--soak-count", "2",
                    "--soak-duration-ms", "2000"]
                code, _, err = serial_invoke(args, RecoveringPort)
                self.assertEqual(code, 2, err)
                path = Path(directory) / (fault + ".json")
                data = runner.verify(path)
                self.assertEqual(data["serial"]["recovery"]["state"], outcome)
                data["serial"]["recovery"].update(state="continuous", identity={"boot": 1234})
                path.write_text(json.dumps(data), encoding="utf-8")
                with self.assertRaises((EvidenceError, ProtocolError)):
                    runner.verify(path)

    def test_rejected_plans_never_reserve_or_open(self):
        with (tempfile.TemporaryDirectory() as folder,
              patch.object(runner, "EvidenceFiles") as reserve,
              patch.object(runner, "FixtureProcess") as carrier):
            for case, count, duration, extra in (
                ("observe", 0, 100, []), ("observe", 2, 0, []), ("observe", 100_000, 100, []),
                ("observe", 2, 100, ["--max-bytes", "8192"]), ("cold-boot", 2, 100, []),
            ):
                with self.subTest(count=count, duration=duration, extra=extra):
                    code, _, error = invoke("run-fixture", "unused.exe", *arguments(case, count, duration),
                                            "--out-dir", folder, "--run", "rejected", *extra)
                    self.assertEqual(code, 2, error)
            reserve.assert_not_called()
            carrier.assert_not_called()
            self.assertEqual(list(Path(folder).iterdir()), [])

    def test_primary_and_cleanup_clock_edges(self):
        now = [10.0]
        session = SimpleNamespace(soak={"duration_ms": 1000})
        with patch.object(runner, "time", SimpleNamespace(monotonic=lambda: now[0])):
            budget = runner.SoakBudget(session, 10.0, 20.0, 2.0, 15.0, 16.0)
            with budget.step(False) as limit:
                self.assertEqual(limit, 11.0)
                now[0] = 10.5
            now[0] = 11.0
            with self.assertRaises(runner.SoakDuration), budget.step(False):
                self.fail("request submitted at its deadline")
            with budget.step(True) as limit:
                self.assertEqual(limit, 13.0)
                now[0] = 12.25
            with budget.step(True) as limit:
                self.assertEqual(limit, 13.0)
            with self.assertRaises(runner.SoakDuration), budget.step(False):
                self.fail("cleanup renewed the primary budget")
            with self.assertRaises(ProtocolError):
                with budget.step(True):
                    now[0] = 13.0
            self.assertEqual(budget.cleanup_left, 0)


@unittest.skipUnless(FIXTURE or "--fixture" in sys.argv, "run with the compiled CLI fixture")
class CompiledSoakTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        WORK.mkdir(parents=True, exist_ok=True)
        directory = tempfile.TemporaryDirectory(dir=WORK)
        cls.addClassCleanup(directory.cleanup)
        cls.folder = Path(directory.name)
        cls.positives = {}
        for name in ("observe", "owned-apply", "irq"):
            result, manifest, raw, error = cls.run_case(name, name)
            if result or manifest["verdict"] != "PASS":
                raise AssertionError((name, manifest, error))
            cls.positives[name] = (manifest, raw)

    @classmethod
    def run_case(cls, name, run, extra=()):
        code, _, error = invoke("run-fixture", FIXTURE, *arguments(name), "--fixture-identity", REVISION,
                                "--nonce", NONCE, "--out-dir", cls.folder, "--run", run,
                                "--timeout", "12", "--closure", "1", *extra)
        path = cls.folder / (run + ".json")
        if not path.exists():
            raise AssertionError((name, code, error))
        return code, json.loads(path.read_bytes()), path.with_suffix(".txt").read_bytes(), error

    def test_distinct_counts_and_evidence_per_iteration(self):
        for name, (data, raw) in self.positives.items():
            with self.subTest(case=name):
                self.assertEqual(runner.verify(self.folder / (name + ".json")), data)
                self.assertEqual(soak_replay(data, decoded(raw)), "PASS")
                soak = data["soak"]
                self.assertEqual([soak[k] for k in ("requested", "attempted", "completed", "remaining")],
                                 [2, 2, 2, 0])
                self.assertEqual(soak["stop_reason"], "count")
                self.assertEqual(len(soak["telemetry"]), 2)
                self.assertLessEqual(len(raw), soak["plan"]["transcript_bytes"])
                self.assertEqual(sum(cmd.startswith("session ") for cmd in data["commands"]), 1)
                self.assertEqual(len(data["commands"]), len(set(record.req for record in decoded(raw)
                                                                 if record.req >= data["identity"]["session_req"])))
                self.assertEqual(len(list(self.folder.glob(name + ".*"))), 2)
                for i, measures in enumerate(soak["telemetry"]):
                    self.assertEqual(measures["id"], f"{name}-{i + 1}")
                    self.assertEqual(measures["id"], data["cases"][i]["id"])
                    self.assertIsNone(measures["console"]["native"])
                    self.assertEqual(measures["console"]["dropped_human"], 0)
                    self.assertGreaterEqual(measures["transport"]["attempted"], measures["transport"]["completed"])
                    self.assertEqual(measures["health"]["health"], "READY")
                    self.assertIn("recr", measures["chip"])
                    self.assertIn("fcscr", measures["chip"])
                    self.assertIn("event_mask", measures["chip"])
                    if i:
                        for key in ("console", "transport", "health", "chip"):
                            self.assertGreater(measures[key]["req"], soak["telemetry"][i - 1][key]["req"])
                if name == "observe":
                    self.assertEqual(data["commands"].count("registers write c22 24 65536"), 2)
                if name == "owned-apply":
                    self.assertEqual(data["commands"].count("configuration apply an 15 asymmetric mdix 3"), 2)
                    self.assertTrue(all(row["restoration"]["verdict"] == "PASS" for row in data["cases"]))
                if name == "irq":
                    self.assertEqual([x["action"] for x in data["campaign"]["interventions"]], ["inject-events"] * 2)

    def test_every_required_field_including_later_iterations(self):
        n = sum(assert_soak_required_fields(self, name, data, raw)
                for name, (data, raw) in self.positives.items())
        self.assertGreater(n, 15_000)
        print(f"Rejected {n} required-field mutations across three repeated-case transcripts")

    def test_raw_irq_transcript_replays_one_setup_and_all_interventions(self):
        data, raw = self.positives["irq"]
        receipts = self.folder / "irq-interventions.json"
        receipts.write_text(json.dumps(data["campaign"]["interventions"]), encoding="utf-8")
        code, _, error = invoke("verify-transcript", self.folder / "irq.txt", *arguments("irq"),
                                "--interventions", receipts, "--nonce", NONCE,
                                "--out-dir", self.folder, "--run", "raw-irq")
        self.assertEqual(code, 0, error)
        replay = runner.verify(self.folder / "raw-irq.json")
        self.assertEqual((self.folder / "raw-irq.txt").read_bytes(), raw)
        self.assertTrue(data["soak"]["setup"])
        for key in ("setup", "telemetry", "completed", "stop_reason"):
            self.assertEqual(replay["soak"][key], data["soak"][key])
        for key in ("commands", "cases", "identity", "campaign", "transcript"):
            self.assertEqual(replay[key], data[key])

    def test_corrupt_compiled_setup_fails_with_an_accurate_transcript_hash(self):
        data, raw = self.positives["irq"]
        damaged = raw.replace(b" rxfs_valid=1", b"", 1)
        self.assertNotEqual(damaged, raw)
        path = self.folder / "irq-missing-setup-field.json"
        receipt = deepcopy(data)
        receipt["run"] = path.stem
        receipt["transcript"].update(sha256=hashlib.sha256(damaged).hexdigest(), length=len(damaged))
        path.with_suffix(".txt").write_bytes(damaged)
        path.write_text(json.dumps(receipt), encoding="utf-8")
        self.assertEqual(invoke("verify-artifacts", path)[0], 2)

    def test_injected_serial_repetitions_and_failure_before_stalled_close(self):
        data, raw = self.positives["observe"]
        # Supply the distinct native-owner payloads through an injected backend.
        # All common records, order, request IDs and PHY data come from the fixture.
        rows = {}
        for line in row_lines(device_identity(raw)):
            record, = decoded(line)
            if record.cmd == "application.status":
                line = line.replace(b"primary=UNSUPPORTED ", b"primary=OK ").replace(
                    b"reason=none\r\n", b"reason=none installed=1 traffic=0 maintenance=1 retired=0 "
                    b"shutdown=0 carrier=0 ip=0.0.0.0 events=0\r\n")
            elif record.cmd == "local.console":
                line = line.rstrip(b"\r\n") + (b" input_queued=0 output_queued=0 lines_queued=0 input_dropped=3 "
                    b"input_gaps=2 lines_dropped=1 lines_truncated=4 uart_stalls=7 output_backpressure=9\r\n")
            rows[record.req] = rows.get(record.req, b"") + line

        for fail in (False, True):
            with self.subTest(failed=fail):
                run = "serial-hard-failure" if fail else "serial-positive"
                release = threading.Event()
                close_workers = []
                commands = data["commands"]

                class InjectedPort(Port):
                    def write(port, line):
                        index = len(port.lines)
                        self.assertEqual(line, (commands[index] + "\n").encode("ascii"))
                        port.lines.append(line)
                        response = rows[data["identity"]["session_req"] + index]
                        if fail and commands[index].startswith("registers write"):
                            response = response.replace(b"reason=range", b"reason=profile")
                        port.rx.extend(response)
                        return len(line)

                    def read(port, size):
                        chunk = bytes(port.rx[:size])
                        del port.rx[:size]
                        return chunk

                    def close(port):
                        if fail:
                            close_workers.append(threading.current_thread())
                            release.wait(3)
                        super().close()

                args = serial_arguments(self.folder, run) + [
                    "--suite", "campaign", "--case", "observe", "--operating-mode", "enhanced",
                    "--polling", "exclusive", "--clock", "simulated", "--soak-count", "2",
                    "--soak-duration-ms", "3000", "--duration", "3", "--session-timeout", "5",
                    "--cleanup-timeout", "0.32", "--finalize-timeout", "0.6"]
                if fail:
                    try:
                        with (patch.object(runner.os, "_exit", side_effect=SystemExit(2)) as hard_exit,
                              self.assertRaises(SystemExit) as stop):
                            serial_invoke(args, InjectedPort)
                        self.assertEqual(stop.exception.code, 2)
                        hard_exit.assert_called_once_with(2)
                    finally:
                        release.set()
                        self.assertEqual(len(close_workers), 1)
                        close_workers[0].join(timeout=3)
                        self.assertFalse(close_workers[0].is_alive())
                else:
                    code, _, err = serial_invoke(args, InjectedPort)
                    self.assertEqual(code, 0, err)
                receipt = runner.verify(self.folder / (run + ".json"))
                self.assertEqual(receipt["serial"]["hard_exit"], fail)
                self.assertEqual(receipt["soak"]["telemetry"][0]["console"]["native"]["lines_truncated"], 4)
                self.assertFalse(receipt["transcript"]["truncated"])
                if fail:
                    self.assertEqual(receipt["serial"]["close"]["state"], "stalled")
                    self.assertEqual(receipt["verdict"], "FAIL")
                    self.assertEqual(receipt["commands"], commands[:commands.index("registers write c22 24 65536") + 1])
                    self.assertEqual([case["verdict"] for case in receipt["cases"]], ["FAIL", "NOT RUN"])
                    self.assertEqual(receipt["cases"][0]["primary"], {"code": "INVALID_PARAM", "detail": 0})
                    self.assertEqual(aggregate({k: v for k, v in receipt.items() if k != "soak"}), "INCONCLUSIVE")
                    self.assertEqual(invoke("verify-artifacts", self.folder / (run + ".json"))[0], 1)
                else:
                    self.assertEqual(receipt["commands"], commands)
                    self.assertEqual(receipt["soak"]["completed"], 2)
                    n = assert_soak_required_fields(self, run, receipt,
                                                    (self.folder / (run + ".txt")).read_bytes())
                    self.assertGreater(n, 3500)
                    print(f"Rejected {n} required-field mutations in the injected native-console transcript")

    def test_manifest_fields_are_bound_to_their_record_stream(self):
        data, raw = self.positives["owned-apply"]
        path = self.folder / "forged.json"
        path.with_suffix(".txt").write_bytes(raw)
        for change in (
            lambda d: d["soak"].update(attempted=1),
            lambda d: d["soak"].update(completed=1, remaining=1),
            lambda d: d["soak"].update(stop_reason="duration"),
            lambda d: d["soak"]["telemetry"][0]["console"].update(native={}),
            lambda d: d["soak"]["telemetry"][1]["transport"].update(attempted=0),
            lambda d: d["soak"]["telemetry"][1]["health"].update(successes=1),
            lambda d: d["soak"]["telemetry"][1]["chip"].update(event_mask=0),
            lambda d: d["soak"]["telemetry"].reverse(),
            lambda d: d["cases"][0]["restoration"].update(required=False),
            lambda d: d.update(verdict="FAIL"),
        ):
            changed = deepcopy(data)
            changed["run"] = "forged"
            change(changed)
            self.assertNotEqual(changed["soak"] | {"cases": changed["cases"], "verdict": changed["verdict"]},
                                data["soak"] | {"cases": data["cases"], "verdict": data["verdict"]})
            path.write_text(json.dumps(changed), encoding="utf-8")
            with self.assertRaises((EvidenceError, ProtocolError)):
                runner.verify(path)

    def test_deadline_between_iterations_stops_at_exact_prefix(self):
        original = runner.SoakBudget.step
        boundary = []
        clock = SimpleNamespace(monotonic=lambda: boundary[0] if boundary else time.monotonic(), sleep=time.sleep)

        @contextmanager
        def expire_after_first(budget, cleanup):
            if budget.session.campaign.completed == 1 and not boundary:
                boundary.append(budget.primary_end)
            with original(budget, cleanup) as deadline:
                yield deadline

        with patch.object(runner, "time", clock), patch.object(runner.SoakBudget, "step", expire_after_first):
            code, data, _, err = self.run_case("observe", "expired")
        self.assertEqual((code, data["verdict"]), (2, "INCONCLUSIVE"), err)
        soak = data["soak"]
        self.assertEqual((soak["attempted"], soak["completed"], soak["remaining"], soak["stop_reason"]),
                         (1, 1, 1, "duration"))
        self.assertEqual([case["verdict"] for case in data["cases"]], ["PASS", "NOT RUN"])
        positive, _ = self.positives["observe"]
        last = positive["soak"]["telemetry"][0]["health"]["req"]
        first = positive["identity"]["session_req"]
        self.assertEqual(data["commands"], positive["commands"][:last - first + 1])
        self.assertEqual(runner.verify(self.folder / "expired.json"), data)

    def test_clean_expiry_after_effect_executes_only_the_guarded_restoration(self):
        original = runner.SoakBudget.step
        for case, target, restore, at_cleanup in (
                ("owned-apply", "configuration apply an 9 symmetric auto 0",
                 "configuration apply an 15 asymmetric mdix 3", False),
                ("owned-apply", "configuration apply an 9 symmetric auto 0",
                 "configuration apply an 15 asymmetric mdix 3", True),
                ("irq", "irq test-start", "irq test-stop", False)):
            with self.subTest(case=case, at_cleanup=at_cleanup):
                run = f"clean-expiry-{case}-{at_cleanup}"
                prefix = []

                @contextmanager
                def expire(budget, cleanup):
                    with original(budget, cleanup) as deadline:
                        yield deadline
                        session = budget.session
                        sent = [cmd.text for cmd in session.commands[:session.issued]]
                        if (not prefix and not cleanup and target in sent and session.next and
                                (session.next.cleanup if at_cleanup else sent[-1] == target)):
                            prefix.extend(sent)
                            budget.primary_end = time.monotonic()
                            raise runner.SoakDuration("primary expired after the correlated terminal")

                with patch.object(runner.SoakBudget, "step", expire):
                    code, result, _, error = self.run_case(case, run)
                self.assertEqual(code, 2, error)
                self.assertTrue(prefix)
                suffix = result["commands"][len(prefix):]
                self.assertEqual(suffix[0], "application status")
                self.assertEqual(suffix.count(restore), 1)
                self.assertNotIn(target, suffix)
                self.assertEqual(result["soak"]["stop_reason"], "duration")
                self.assertEqual(result["cases"][0]["restoration"]["verdict"], "PASS")
                self.assertEqual(result["cases"][1]["verdict"], "NOT RUN")
                self.assertEqual(result, runner.verify(self.folder / (run + ".json")))

    def test_skipped_case_and_definite_failure_stop_later_admission(self):
        for name, run, extra, code, attempted, completed, verdict in (
                ("int-pin", "skip", (), 2, 0, 0, "BLOCKED"),
                ("owned-apply", "failed", ("--fixture-deny-maintenance",), 1, 1, 1, "FAIL")):
            actual, data, _, err = self.run_case(name, run, extra)
            self.assertEqual(actual, code, err)
            self.assertEqual([case["verdict"] for case in data["cases"]], [verdict, "NOT RUN"])
            self.assertEqual([data["soak"][k] for k in ("attempted", "completed", "stop_reason")],
                             [attempted, completed, "fault"])
            self.assertEqual(runner.verify(self.folder / (run + ".json")), data)


if __name__ == "__main__":
    if "--fixture" in sys.argv:
        at = sys.argv.index("--fixture")
        FIXTURE = Path(sys.argv[at + 1]).resolve()
        del sys.argv[at:at + 2]
    if "--work-dir" in sys.argv:
        at = sys.argv.index("--work-dir")
        WORK = Path(sys.argv[at + 1]).resolve()
        del sys.argv[at:at + 2]
    unittest.main()
