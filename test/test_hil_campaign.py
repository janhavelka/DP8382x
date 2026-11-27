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
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from test_hil_runner import NONCE, REVISION, runner
from test_hil_serial import Port, arguments as serial_arguments, device_identity, invoke
from dp8382x_hil_artifacts import EvidenceError, aggregate
from dp8382x_hil_campaign import CASES, Profile
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


def assert_required_fields(test, name, data, raw, replay_fn=replay, include_setup=False):
    """Mutate every required field, including setup when the replay consumes it."""
    lines = row_lines(raw)
    original = decoded(raw)
    test.assertEqual(len(lines), len(original))
    nonce = data["commands"][0].split(" ", 1)[1]
    start = next(i for i, row in enumerate(original) if row.fields.get("nonce") == nonce)
    mutations = 0
    for index in range(0 if include_setup else start, len(lines)):
        fields = lines[index][len(PREFIX):-2].split(b" ")
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
                if replay_fn(data, [*original[:index], row, *original[index + 1:]]) == "PASS":
                    test.fail(f"{name} accepted altered {key!r} at record {index}: {altered!r}")
    return mutations


class PlanTests(unittest.TestCase):
    def test_p22_saved_profiles_can_be_restored_through_the_cli(self):
        for mode, speed, flags, allowed in (
                ("an", 0, 128 + 512 + 2048, True), ("forced", 100, 64 + 2048, True),
                ("forced", 10, 64 + 2048, False), ("forced", 100, 32, False),
                ("an", 0, 128, False), ("an", 0, 2048, False), ("an", 0, 256, False),
                ("an", 0, 128 + 768 + 2048, False), ("an", 0, 24, False)):
            profile = Profile(mode, 8 if mode == "an" else 0, speed,
                              "unknown" if mode == "an" else "full", flags=flags)
            self.assertEqual(profile.replayable(robust_optional=True, p22=True), allowed, profile)

    def test_guide_lists_the_exact_cases(self):
        guide = (ROOT / "docs" / "hardware-validation.md").read_text(encoding="utf-8")
        original = guide.split("## Feature fixtures", 1)[0]
        names = [line.split("`")[1] for line in original.splitlines() if line.startswith("| `")]
        self.assertEqual(set(names), CASES.keys())
        self.assertEqual(len(names), len(CASES))
        self.assertEqual({case.group for case in CASES.values()}, {"observation", "configuration", "modes", "faults"})

    def test_zero_case_and_skip_only_cannot_pass(self):
        base = {"identity": {"boot": 1}, "runner": {"revision": "unknown"}, "complete": True, "issues": [],
                "commands": ["session example"], "expectations": dict.fromkeys(runner.TARGET_FIELDS)}
        for cases in ([], [{"id": "int-pin", "verdict": "BLOCKED", "executed": False, "primary": None,
                            "restoration": {"required": False, "verdict": "NOT RUN"}}]):
            self.assertEqual(aggregate({**base, "cases": cases}), "INCONCLUSIVE")

    def test_operator_acknowledgment_is_specific_to_the_current_prompt(self):
        for response, verdict in (("replug 0123456789ab", "acknowledged"),
                                  ("replug abcdef012345", "declined"), ("replug", "declined")):
            with self.subTest(response=response), contextlib.ExitStack() as stack:
                stdin = SimpleNamespace(isatty=lambda: True, fileno=lambda: 7)
                stack.enter_context(patch.object(runner.sys, "stdin", stdin))
                stack.enter_context(patch.object(runner.secrets, "token_hex", return_value="0123456789ab"))
                stack.enter_context(contextlib.redirect_stderr(io.StringIO()))
                if runner.os.name == "nt":
                    import msvcrt
                    stack.enter_context(patch.object(msvcrt, "kbhit", return_value=True))
                    stack.enter_context(patch.object(msvcrt, "getwch", side_effect=response + "\r"))
                else:
                    import select
                    stack.enter_context(patch.object(select, "select", return_value=([stdin], [], [])))
                    stack.enter_context(patch.object(runner.os, "read",
                                                     side_effect=[c.encode("ascii") for c in response + "\n"]))
                self.assertEqual(runner.operator_acknowledgment("replug", runner.time.monotonic() + 1), verdict)

    def test_operator_read_failure_supplies_a_recoverable_receipt(self):
        receipts = []
        campaign = SimpleNamespace(declaration={"intervention_timeout_ms": 1_000}, last=SimpleNamespace(req=3, boot=2))
        session = SimpleNamespace(campaign=campaign, intervene=receipts.append)

        def disconnected(_action, _deadline):
            raise OSError("simulated console input failure")

        runner.intervention(session, runner.Intervention("unplug", False), "operator",
                            runner.time.monotonic() + 1, disconnected)
        self.assertEqual(len(receipts), 1)
        self.assertEqual((receipts[0]["outcome"], receipts[0]["after_req"], receipts[0]["boot"]), ("failed", 3, 2))

    def test_campaign_mutation_option_cannot_select_the_passive_suite(self):
        args = ["dry-run", "--suite", "passive", "--target", "host", "--backend", "sim-mdio",
                "--variant", "DP83826I", "--interface", "RMII", "--address", "1", "--reinitialize"]
        with contextlib.redirect_stderr(io.StringIO()) as stderr:
            self.assertEqual(runner.main(args), 2)
        self.assertIn("campaign options require --suite campaign", stderr.getvalue())


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
        mutations = sum(assert_required_fields(self, name, data, raw)
                        for name, (data, raw) in self.positives.items())
        self.assertGreater(mutations, 30_000)

    def test_grade_mode_and_artifact_identity(self):
        for variant, other, basic, enhanced in (
                ("DP83826E", "DP83826I", 0xA111, 0xA131),
                ("DP83826AI", "DP83826AE", 0xA114, 0xA134),
                ("DP83826AE", "DP83826AI", 0xA114, 0xA134)):
            for mode, id2 in (("basic", basic), ("enhanced", enhanced)):
                with self.subTest(variant=variant, mode=mode):
                    run = f"-{variant}-{mode}"
                    args = ("--fixture-variant", variant, "--variant", variant, "--operating-mode", mode)
                    result, data, raw = self.run_case("observe", run, args)
                    self.assertEqual((result, data["verdict"]), (0, "PASS"), data["issues"])
                    self.assertEqual(data["identity"]["variant"], variant)
                    device = next(row for row in decoded(raw) if row.cmd == "connection.identity")
                    self.assertEqual((device.fields["id2"], device.fields["mode"]), (str(id2), mode))
                    path = self.folder / ("observe" + run + ".json")
                    self.assertEqual(runner.verify(path), data)

                    # Recompute the digest; the decoded observation must independently reject
                    # a swapped grade or a valid ID and revision from the other generation.
                    wrong_id = (id2 & 0xFFF0) | (1 if (id2 & 15) == 4 else 4)
                    for changed in (raw.replace(variant.encode(), other.encode()),
                                    raw.replace(f"id2={id2} ".encode(), f"id2={wrong_id} ".encode())
                                       .replace(f"revision={id2 & 15} ".encode(), f"revision={wrong_id & 15} ".encode())):
                        self.assertNotEqual(changed, raw)
                        path.with_suffix(".txt").write_bytes(changed)
                        forged = deepcopy(data)
                        forged["transcript"]["sha256"] = hashlib.sha256(changed).hexdigest()
                        path.write_text(json.dumps(forged), encoding="utf-8")
                        with self.assertRaises((EvidenceError, ProtocolError)):
                            runner.verify(path)

                    path.with_suffix(".txt").write_bytes(raw)
                    for field in ("identity", "expectations"):
                        forged = deepcopy(data)
                        forged[field]["variant"] = other
                        path.write_text(json.dumps(forged), encoding="utf-8")
                        with self.assertRaises((EvidenceError, ProtocolError)):
                            runner.verify(path)

            result, data, _ = self.run_case("observe", "-wrong-" + variant,
                                            ("--fixture-variant", other, "--variant", variant))
            self.assertEqual(result, 2)
            self.assertEqual(data["identity"]["variant"], other)
            self.assertIn("identity mismatch: variant", " ".join(data["issues"]))
            self.assertEqual(len(data["commands"]), 1)

    def test_p22_copper_campaign_receipts(self):
        for name, grade, interface in (("observe", "DP83822I", "RGMII"),
                                       ("hard", "DP83822H", "RMII"),
                                       ("forced-10-half", "DP83822HF", "MII"),
                                       ("an-100-full", "DP83822IF", "RMII")):
            with self.subTest(grade=grade, case=name):
                args = ("--fixture-variant", grade, "--variant", grade, "--medium", "copper",
                        "--interface", interface, "--operating-mode", "none")
                result, data, raw = self.run_case(name, "-822-" + grade, args)
                self.assertEqual((result, data["verdict"]), (0, "PASS"), (data["issues"], data["cases"]))
                self.assertEqual(replay(data, decoded(raw)), "PASS")
                self.assertEqual(data["identity"]["medium"], "copper")
                self.assertNotIn("registers read mmd 31 0x030E", data["commands"])
                if name == "hard":
                    for field, value in (("sor1", 0x0BE3), ("sor1", 0x0FC3), ("sor1", 0x0FE1),
                                         ("sor1", 0x0DE3 | 0x10), ("sor2", 2), ("sor2", 8)):
                        records = decoded(raw)
                        reset = next(r for r in records if r.cmd == "recovery.reset" and r.state == "terminal")
                        reset.fields[field] = str(value)
                        self.assertNotEqual(replay(data, records), "PASS", (field, value))

    def test_p25_roles_and_exact_part_replay(self):
        for role, other in (("leader", "follower"), ("follower", "leader")):
            args = ("--fixture-variant", "DP83825I", "--variant", "DP83825I", "--operating-mode", "none",
                    "--fixture-rmii-role", role, "--rmii-role", role)
            for name in ("observe", "cold-boot", "pcs", "hard", "restart", "10base-te", "forced-100-half"):
                with self.subTest(role=role, case=name):
                    result, data, raw = self.run_case(name, "-825-" + role, args)
                    self.assertEqual((result, data["verdict"]), (0, "PASS"), (data["issues"], data["cases"]))
                    self.assertEqual(replay(data, decoded(raw)), "PASS")
                    self.assertNotIn("registers read mmd 31 0x030E", data["commands"])
                    self.assertEqual(data["identity"]["rmii_role"], role)
                    source = next(row for row in decoded(raw) if row.cmd == "connection.identity")
                    self.assertEqual((source.fields["model"], source.fields["revision"], source.fields["mode"]),
                                     ("20", "0", "unknown"))
                    for key, value in (("rcsr_valid", "0"), ("sor1_valid", "0"), ("straps_stale", "1"),
                                       ("rcsr", str(int(source.fields["rcsr"]) ^ 128)),
                                       ("sor1", str(int(source.fields["sor1"]) ^ 8))):
                        changed = decoded(raw)
                        current = next(row for row in changed if row.cmd == "connection.identity")
                        current.fields[key] = value
                        self.assertNotEqual(replay(data, changed), "PASS", key)
                    if name == "10base-te":
                        # Tables 7-7/8 prohibit local remote fault while allowing
                        # the partner to advertise it in its own base page.
                        for field in ("anar", "anlpar"):
                            changed = decoded(raw)
                            link = next(row for row in changed if row.cmd in ("link.read", "link.partner")
                                        and row.fields.get("partner_valid") == "1")
                            link.fields[field] = str(int(link.fields[field]) | 0x2000)
                            self.assertEqual(replay(data, changed) == "PASS", field == "anlpar", field)
            result, data, _ = self.run_case("observe", "-wrong-role-" + role, args + ("--rmii-role", other))
            self.assertEqual(result, 2)
            self.assertIn("identity mismatch: rmii_role", " ".join(data["issues"]))
            self.assertEqual(len(data["commands"]), 1)

    def test_reset_shape_before_and_after_session_identity(self):
        _, p25, p25_raw = self.run_case("hard", "-825-prefix", (
            "--fixture-variant", "DP83825I", "--variant", "DP83825I", "--operating-mode", "none",
            "--fixture-rmii-role", "leader", "--rmii-role", "leader"))
        p26, p26_raw = self.positives["hard"]
        for data, raw, prior in ((p25, p25_raw, p26_raw), (p26, p26_raw, p25_raw)):
            with self.subTest(variant=data["expectations"]["variant"]):
                previous = next(r for r in decoded(prior) if r.cmd == "recovery.reset" and r.state == "terminal")
                self.assertEqual(replay(data, [previous, *decoded(raw)]), "PASS")
                malformed = deepcopy(previous)
                del malformed.fields["sor1_valid"]
                self.assertNotEqual(replay(data, [malformed, *decoded(raw)]), "PASS")
                rows = decoded(raw)
                bound = next(r for r in rows if r.cmd == "recovery.reset" and r.state == "terminal")
                for field in ("rcsr_valid", "rcsr"):
                    if field in previous.fields:
                        bound.fields[field] = previous.fields[field]
                    else:
                        del bound.fields[field]
                self.assertNotEqual(replay(data, rows), "PASS")

    def test_a_campaigns_replay_link_reset_and_recovery(self):
        for grade in ("DP83826AI", "DP83826AE"):
            for name in CASES:
                if name in ("observe", "int-pin"):
                    continue
                with self.subTest(grade=grade, case=name):
                    args = ("--fixture-variant", grade, "--variant", grade)
                    result, data, raw = self.run_case(name, "-" + grade, args)
                    self.assertEqual((result, data["verdict"]), (0, "PASS"), data["cases"])
                    self.assertEqual(replay(data, decoded(raw)), "PASS")

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
            lambda result: result["campaign"]["interventions"][0].update(
                elapsed_ms=result["campaign"]["interventions"][0]["budget_ms"]),
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

    def test_real_link_bookend_change_can_retry_within_the_observation_budget(self):
        send = runner.FixtureProcess.send

        def inject(fixture, line, deadline=None):
            if line == b"$campaign-peer-delayed an-100-full\n":
                line = b"$campaign-peer-change an-100-full\n"
            return send(fixture, line, deadline)

        with patch.object(runner.FixtureProcess, "send", inject):
            code, data, raw = self.run_case("bounded-recovery", "-bookends")
        self.assertEqual(code, 0, data)
        self.assertEqual(data["commands"].count("link read 8"), 4)
        self.assertEqual(runner.verify(self.folder / "bounded-recovery-bookends.json"), data)
        row = next(line for line in row_lines(raw) if b"primary=BUSY detail=6 " in line)
        self.assertIn(b"observation=unavailable valid=0 stale=1", row)
        for change in (row.replace(b"primary=BUSY detail=6 ", b"primary=BUSY detail=3 "),
                       row.replace(b"observation=unavailable valid=0 ", b"observation=unavailable valid=1 ")):
            self.assertNotEqual(replay(data, decoded(raw.replace(row, change, 1))), "PASS")

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

    def serial_case(self, case, run, *, app="traffic=0 maintenance=1", failure_at=None,
                    late=False, extra=()):
        data, raw = self.positives[case]
        requests = {}
        for line in row_lines(device_identity(raw)):
            (record,) = decoded(line)
            if record.cmd == "application.status":
                line = line.replace(b"primary=UNSUPPORTED ", b"primary=OK ").replace(
                    b"reason=none\r\n", b"reason=none installed=1 " + app.encode("ascii") +
                    b" retired=0 shutdown=0 carrier=0 ip=0.0.0.0 events=0\r\n")
            requests[record.req] = requests.get(record.req, b"") + line

        class CampaignPort(Port):
            def write(self, line):
                index = len(self.lines)
                if line != (data["commands"][index] + "\n").encode("ascii"):
                    raise AssertionError(f"unexpected serial command {line!r}")
                self.lines.append(line)
                if index == failure_at and not late:
                    return 5
                self.rx.extend(requests[index + 1])
                return len(line)

        port = None

        def factory(**options):
            nonlocal port
            port = CampaignPort(**options)
            return port

        read = runner.SerialPort.read

        def deliver(serial, size, deadline, retain):
            if late and len(port.lines) == failure_at + 1:
                fragment = port.read(size)
                retain(fragment)
                raise runner.ReplyTimeout("late partial mutation receipt", fragment)
            return read(serial, size, deadline, retain)

        args = serial_arguments(self.folder, run) + [
            "--suite", "campaign", "--case", case, "--clock", "simulated", "--operating-mode", "enhanced",
            "--polling", "exclusive", "--session-timeout", "10", "--duration", "4", "--cleanup-timeout", "3",
        ]
        if CASES[case].peer:
            args += ["--peer", "injected-peer", "--peer-profile", CASES[case].peer, "--cable", "injected-cable"]
        with (patch.object(runner, "operator_acknowledgment", return_value="acknowledged") as acknowledge,
              patch.object(runner.SerialPort, "read", deliver)):
            code, _, err = invoke(args + list(extra), factory)
        receipt = runner.verify(self.folder / (run + ".json"))
        return code, err, receipt, port.lines, acknowledge.call_count

    def test_injected_serial_runs_the_complete_case_without_fixture_controls(self):
        code, err, receipt, _, acknowledgments = self.serial_case("unplug-replug", "injected-campaign")
        self.assertEqual(code, 0, err)
        self.assertEqual(acknowledgments, 2)
        self.assertEqual([x["source"] for x in receipt["campaign"]["interventions"]], ["operator", "operator"])
        self.assertEqual(receipt["scope"], "serial_session")

    def test_injected_serial_cold_boot_needs_stop_for_exclusive_vod_read(self):
        extra = ("--power-record", "simulated-independent-power-sequence", "--previous-boot", "0")
        for polling, status, verdict in (("exclusive", 2, "BLOCKED"), ("concurrent", 0, "PASS")):
            with self.subTest(polling=polling):
                code, err, data, lines, _ = self.serial_case(
                    "cold-boot", "injected-cold-" + polling, app="traffic=1 maintenance=0",
                    extra=(*extra, "--polling", polling))
                self.assertEqual((code, data["cases"][0]["verdict"]), (status, verdict), err)
                self.assertFalse(any(line.startswith(b"registers read ") for line in lines))

    def test_framing_loss_during_mutation_or_cleanup_stops_serial_writes(self):
        commands = self.positives["owned-apply"][0]["commands"]
        apply = commands.index("configuration apply an 9 symmetric auto 0")
        restore = commands.index("configuration apply an 15 asymmetric mdix 3")
        for name, at, late, expected_restoration in (
            ("short-apply", apply, False, "NOT RUN"),
            ("late-apply", apply, True, "NOT RUN"),
            ("short-restore", restore, False, "INCONCLUSIVE"),
        ):
            with self.subTest(name=name):
                code, err, data, lines, _ = self.serial_case("owned-apply", name, failure_at=at, late=late)
                self.assertEqual((code, data["verdict"]), (2, "INCONCLUSIVE"), err)
                self.assertEqual(lines, [(item + "\n").encode("ascii") for item in commands[:at + 1]])
                self.assertEqual(data["cases"][0]["restoration"]["verdict"], expected_restoration)
                self.assertIn("restoration was not proved", data["cases"][0]["message"])
                if late:
                    self.assertIn("incomplete record", data["serial"]["recovery"]["detail"])
                else:
                    self.assertTrue(data["serial"]["framing_uncertain"])

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
