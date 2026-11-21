"""Run every campaign through the compiled producer and attack each evidence field."""

import contextlib
from copy import deepcopy
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from test_hil_runner import NONCE, REVISION, runner
from test_hil_serial import Port, arguments as serial_arguments, device_identity, invoke
from dp8382x_hil_artifacts import EvidenceError, aggregate
from dp8382x_hil_campaign import CASES
from dp8382x_hil_protocol import Decoder, PREFIX, ProtocolError


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = None
WORK = ROOT / "build" / "hil-campaign-tests"


def decoded(raw):
    decoder = Decoder()
    rows = decoder.feed(raw)
    decoder.finish()
    return rows


def row_lines(raw):
    return [line for line in raw.splitlines(keepends=True) if line.startswith(PREFIX)]


def replay(receipt, records):
    """Run the same state transitions used for replay of complete run manifests."""
    evidence = receipt["campaign"]
    session = runner.CampaignSession(NONCE, receipt["expectations"], evidence["declaration"], evidence["interventions"])
    try:
        for record in records:
            session.accept(record)
        session.finish()
    except ProtocolError:
        return "INCONCLUSIVE"
    return aggregate({**receipt, **session.evidence()})


class PlanTests(unittest.TestCase):
    def test_guide_lists_the_exact_cases(self):
        guide = (ROOT / "docs" / "hardware-validation.md").read_text(encoding="utf-8")
        names = [line.split("`")[1] for line in guide.splitlines() if line.startswith("| `")]
        self.assertEqual(set(names), CASES.keys())
        self.assertEqual(len(names), len(CASES))
        self.assertEqual({case.group for case in CASES.values()}, {"observation", "configuration", "modes", "faults"})

    def test_zero_case_and_skip_only_cannot_pass(self):
        base = {"identity": {"boot": 1}, "runner": {"revision": "unknown"}, "complete": True, "issues": [],
                "commands": ["session example"], "expectations": dict.fromkeys(runner.TARGET_FIELDS)}
        for cases in ([], [{"id": "int-pin", "verdict": "BLOCKED", "executed": False, "primary": None,
                            "restoration": {"required": False, "verdict": "NOT RUN"}}]):
            self.assertEqual(aggregate({**base, "cases": cases}), "INCONCLUSIVE")


@unittest.skipUnless(FIXTURE or "--fixture" in sys.argv, "run with the compiled CLI fixture")
class CompiledCampaignTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        WORK.mkdir(parents=True, exist_ok=True)
        directory = tempfile.TemporaryDirectory(dir=WORK)
        cls.addClassCleanup(directory.cleanup)
        cls.folder = Path(directory.name)
        cls.positives = {}
        for case in CASES.values():
            if case.id == "int-pin":
                continue
            result, data, raw = cls.run_case(case.id)
            if result or data["verdict"] != "PASS":
                raise AssertionError((case.id, result, data["issues"], data["cases"]))
            cls.positives[case.id] = (data, raw)

    @classmethod
    def run_case(cls, name, suffix="", extra=(), peer=None):
        run = name + suffix
        args = ["run-fixture", str(FIXTURE), "--suite", "campaign", "--case", name,
                "--target", "host", "--backend", "sim-mdio", "--variant", "DP83826I", "--interface", "RMII",
                "--address", "1", "--firmware", REVISION, "--fixture-identity", REVISION, "--clock", "simulated",
                "--console", "host-stdio", "--baud", "0", "--operating-mode", "enhanced", "--polling", "exclusive",
                "--board", "compiled-P50", "--authorization", "offline-qualification", "--nonce", NONCE,
                "--out-dir", str(cls.folder), "--run", run]
        spec = CASES[name]
        if spec.peer:
            args += ["--peer", "simulated-peer", "--cable", "simulated-cable", "--peer-profile",
                     spec.peer if peer is None else peer]
        if name == "cold-boot":
            args += ["--power-record", "simulated-power-sequence", "--previous-boot", "0"]
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            result = runner.main(args + list(extra))
        path = cls.folder / (run + ".json")
        return result, json.loads(path.read_text(encoding="utf-8")), path.with_suffix(".txt").read_bytes()

    def test_complete_real_transcripts_and_restore(self):
        for name, (data, raw) in self.positives.items():
            with self.subTest(case=name):
                self.assertEqual(runner.verify(self.folder / (name + ".json")), data)
                self.assertEqual(replay(data, decoded(raw)), "PASS")
                if CASES[name].target:
                    self.assertIn("configuration apply an 15 asymmetric mdix 3", data["commands"])
                    self.assertEqual(data["cases"][0]["restoration"]["verdict"], "PASS")
                self.assertFalse(any(text.startswith("$") for text in data["commands"]))
        delayed, _ = self.positives["bounded-recovery"]
        self.assertGreater(delayed["commands"].count("link read 8"), 3)
        result, data, _ = self.run_case("observe", "-basic", ("--operating-mode", "basic", "--interface", "MII"))
        self.assertEqual((result, data["verdict"]), (0, "PASS"))

    def test_every_field_occurrence_is_required(self):
        mutations = 0
        for name, (data, raw) in self.positives.items():
            lines = row_lines(raw)
            original = decoded(raw)
            self.assertEqual(len(lines), len(original))
            for index, line in enumerate(lines):
                fields = line[len(PREFIX):-2].split(b" ")
                for field_index, field in enumerate(fields):
                    key = field.split(b"=", 1)[0]
                    before, after = fields[:field_index], fields[field_index + 1:]
                    for altered in (before + after, before + [key + b"=__bad__"] + after,
                                    before + [field, field] + after):
                        mutations += 1
                        try:
                            row, = decoded(PREFIX + b" ".join(altered) + b"\r\n")
                        except ProtocolError:
                            continue
                        if replay(data, [*original[:index], row, *original[index + 1:]]) == "PASS":
                            self.fail(f"{name} accepted altered {key!r} at record {index}: {altered!r}")
        self.assertGreater(mutations, 30_000)

    def test_forged_aggregate_and_intervention_order_are_rejected(self):
        data, raw = self.positives["unplug-replug"]
        name = "forged"
        path = self.folder / (name + ".json")
        path.with_suffix(".txt").write_bytes(raw)
        for mutate in (
            lambda result: result["cases"].clear(),
            lambda result: result["commands"].pop(),
            lambda result: result["campaign"]["interventions"].reverse(),
            lambda result: result["campaign"]["interventions"].pop(),
            lambda result: result["campaign"]["interventions"][0].update(after_req=1),
            lambda result: result["campaign"]["interventions"][0].update(elapsed_ms=20001),
            lambda result: result["cases"][0]["restoration"].update(required=False),
        ):
            changed = deepcopy(data)
            changed["run"] = name
            mutate(changed)
            path.write_text(json.dumps(changed), encoding="utf-8")
            with self.assertRaises((EvidenceError, ProtocolError)):
                runner.verify(path)
        for line in row_lines(raw):
            if b"observation=down " not in line:
                continue
            for altered in (b"", line + line, line.replace(b"partner_valid=0 ", b"partner_valid=1 ")):
                changed_raw = raw.replace(line, altered, 1)
                path.with_suffix(".txt").write_bytes(changed_raw)
                path.write_text(json.dumps({**data, "run": name, "transcript": {
                    "length": len(changed_raw), "sha256": hashlib.sha256(changed_raw).hexdigest(),
                    "truncated": False}}), encoding="utf-8")
                with self.assertRaises((EvidenceError, ProtocolError)):
                    runner.verify(path)
            break

    def test_legal_valued_contradictions_fail(self):
        for name, command, old, new in (
            ("observe", b"cmd=cached.transport ", b"controller_reusable=1", b"controller_reusable=0"),
            ("observe", b"cmd=registers.write ", b"reason=range", b"reason=profile"),
            ("pcs", b"state=terminal ", b"requested_reset=pcs", b"requested_reset=hard"),
            ("hard", b"state=terminal ", b"persistent_restored=1", b"persistent_restored=0"),
            ("restart", b"state=terminal ", b"straps_stale=0", b"straps_stale=1"),
            ("100-full-only", b"cmd=link.partner ", b"pause_rx=1", b"pause_rx=0"),
            ("100-full-only", b"cmd=link.partner ", b"partner_valid=1", b"partner_valid=0"),
            ("forced-100-half", b"cmd=link.read ", b"duplex=half", b"duplex=full"),
            ("owned-apply", b"cmd=configuration.observed ", b"flags=3\r\n", b"flags=1\r\n"),
            ("wrong-policy", b"cmd=link.read ", b"policy_mask=8", b"policy_mask=1"),
            ("10base-te", b"cmd=registers.read ", b"value=19008", b"value=33792"),
        ):
            data, raw = self.positives[name]
            line = next(x for x in row_lines(raw) if command in x and old in x)
            changed = raw.replace(line, line.replace(old, new, 1), 1)
            self.assertNotEqual(replay(data, decoded(changed)), "PASS", (name, old))

    def test_missing_partner_firmware_and_denied_mutation(self):
        for name, suffix, extra, peer, expected, wanted in (
            ("owned-apply", "-denied", ("--fixture-deny-maintenance",), None, 1, "FAIL"),
            ("100-full-only", "-wrong-peer", (), "an-10-half", 2, "NOT RUN"),
            ("observe", "-wrong-build", ("--fixture-identity", "1234567"), None, 2, "INCONCLUSIVE"),
            ("observe", "-concurrent", ("--polling", "concurrent"), None, 2, "BLOCKED"),
            ("int-pin", "", (), None, 2, "BLOCKED"),
        ):
            with self.subTest(case=name, suffix=suffix):
                result, data, _ = self.run_case(name, suffix, extra, peer)
                self.assertEqual((result, data["cases"][0]["verdict"]), (expected, wanted), data)
                self.assertNotIn("configuration apply an 15 asymmetric mdix 3", data["commands"])
                if suffix == "-denied":
                    self.assertEqual(data["cases"][0]["primary"]["code"], "BUSY")
                    self.assertEqual(data["commands"].count("configuration apply an 9 symmetric auto 0"), 1)
        result, data, _ = self.run_case("cold-boot", "-no-boot-record", ("--power-record", ""))
        self.assertEqual((result, data["cases"][0]["verdict"]), (2, "NOT RUN"))

    def test_partial_apply_preserves_primary_and_requires_separate_restore(self):
        send = runner.FixtureProcess.send

        def issue(fixture, line, deadline=None):
            if line == b"configuration apply an 9 symmetric auto 0\n":
                send(fixture, b"$fail-next 7\n", deadline)
            return send(fixture, line, deadline)

        with patch.object(runner.FixtureProcess, "send", issue):
            for suffix, flags, recovery in (("-unsafe", (), "FAIL"), ("-restore", ("--reinitialize",), "PASS")):
                result, data, raw = self.run_case("owned-apply", suffix, flags)
                self.assertEqual(result, 1, data)
                row = data["cases"][0]
                self.assertEqual(row["primary"], {"code": "MDIO_ERROR", "detail": -43})
                self.assertEqual(row["restoration"]["verdict"], recovery, row)
                self.assertIn(b"mutation_possible=1 cleanup_owed=1", raw)
                self.assertEqual("connection initialize none" in data["commands"], bool(flags))
                self.assertEqual(runner.verify(self.folder / ("owned-apply" + suffix + ".json")), data)

    def test_uncertain_intervention_gets_separate_bounded_recovery(self):
        receipt = runner.intervention
        for outcome in ("timed_out", "declined"):
            def perform(session, step, source, deadline, act):
                def actor(action, until):
                    observed = act(action, until)
                    return outcome if action == "unplug" else observed
                return receipt(session, step, source, deadline, actor)

            with patch.object(runner, "intervention", perform):
                code, data, _ = self.run_case("unplug-replug", "-" + outcome)
            self.assertEqual((code, data["cases"][0]["verdict"]), (2, "INCONCLUSIVE"))
            self.assertEqual(data["cases"][0]["restoration"]["verdict"], "PASS")
            self.assertEqual([(row["action"], row["outcome"], row["cleanup"])
                              for row in data["campaign"]["interventions"]],
                             [("unplug", outcome, False), ("replug", "acknowledged", True)])
            self.assertEqual(runner.verify(self.folder / f"unplug-replug-{outcome}.json"), data)

    def test_offline_transcript_uses_the_same_intervention_receipts(self):
        data, _ = self.positives["peer-reset"]
        receipts = self.folder / "interventions.json"
        receipts.write_text(json.dumps(data["campaign"]["interventions"]), encoding="utf-8")
        args = ["verify-transcript", str(self.folder / "peer-reset.txt"), "--suite", "campaign", "--case", "peer-reset",
                "--target", "host", "--backend", "sim-mdio", "--variant", "DP83826I", "--interface", "RMII",
                "--address", "1", "--firmware", REVISION, "--clock", "simulated", "--console", "host-stdio", "--baud", "0",
                "--operating-mode", "enhanced", "--polling", "exclusive", "--board", "compiled-P50",
                "--authorization", "offline-qualification", "--peer", "simulated-peer", "--cable", "simulated-cable",
                "--peer-profile", "an-100-full", "--nonce", NONCE, "--out-dir", str(self.folder)]
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(runner.main(args + ["--run", "missing-interventions"]), 2)
            self.assertEqual(runner.main(args + ["--run", "offline-replay", "--interventions", str(receipts)]), 0)
        self.assertEqual(runner.verify(self.folder / "offline-replay.json")["cases"], data["cases"])

    def test_injected_serial_runs_the_complete_case_without_fixture_controls(self):
        data, raw = self.positives["unplug-replug"]
        requests = {}
        for line in row_lines(device_identity(raw)):
            (record,) = decoded(line)
            if record.cmd == "application.status":
                line = line.replace(b"primary=UNSUPPORTED ", b"primary=OK ").replace(
                    b"reason=none\r\n", b"reason=none installed=1 traffic=0 maintenance=1 retired=0 shutdown=0 "
                    b"carrier=0 ip=0.0.0.0 events=0\r\n")
            requests[record.req] = requests.get(record.req, b"") + line

        class CampaignPort(Port):
            def write(self, line):
                index = len(self.lines)
                if line != (data["commands"][index] + "\n").encode("ascii"):
                    raise AssertionError(f"unexpected serial command {line!r}")
                self.lines.append(line)
                self.rx.extend(requests[index + 1])
                return len(line)

        args = serial_arguments(self.folder, "injected-campaign") + [
            "--suite", "campaign", "--case", "unplug-replug", "--clock", "simulated", "--operating-mode", "enhanced",
            "--polling", "exclusive", "--peer", "injected-peer", "--peer-profile", "an-100-full",
            "--cable", "injected-cable", "--session-timeout", "10", "--duration", "4", "--cleanup-timeout", "3",
        ]
        with patch.object(runner, "operator_acknowledgment", return_value="acknowledged") as acknowledge:
            code, _, err = invoke(args, CampaignPort)
        self.assertEqual(code, 0, err)
        receipt = runner.verify(self.folder / "injected-campaign.json")
        self.assertEqual(acknowledge.call_count, 2)
        self.assertEqual([x["source"] for x in receipt["campaign"]["interventions"]], ["operator", "operator"])
        self.assertEqual(receipt["scope"], "serial_session")

    def test_vod_trace_from_real_core(self):
        proc = subprocess.run([str(FIXTURE), "--campaign", "enhanced", "--campaign-peer", "an-10-half", "--trace-writes"],
                              input=b"registers read mmd 31 0x030E\n", capture_output=True, check=True, timeout=5)
        writes = [line for line in proc.stderr.splitlines() if line.startswith(b"mdio-write ")]
        vod, advertisement, restart = (writes.index(line) for line in
                                       (b"mdio-write 1f 030e 4a40", b"mdio-write 1f 0004 09e1",
                                        b"mdio-write 1f 0000 1200"))
        self.assertLess(vod, advertisement)
        self.assertLess(advertisement, restart)
        self.assertIn(b"address=782 devad=31 value=19008", proc.stdout)


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
