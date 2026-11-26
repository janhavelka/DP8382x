"""Verify selected feature cases with the compiled CLI and bounded artifact replay."""

import contextlib
import io
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest.mock import patch

from test_hil_campaign import assert_required_fields, decoded, replay, row_lines
from test_hil_runner import NONCE, REVISION, runner
from test_hil_serial import Port, arguments as serial_arguments, invoke as serial_invoke
from dp8382x_hil_campaign import MAX_COMMANDS, MAX_INTERVENTIONS, feature_modules


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = None
WORK = ROOT / "build" / "hil-feature-tests"

CONTRADICTIONS = {
    "loopback-digital": (b"diagnostics.loopback-stop", b"hard_exit=1", b"hard_exit=0"),
    "bist-both": (b"diagnostics.bist-read", b"counter_valid=1", b"counter_valid=0"),
    "cable-manual": (b"diagnostics.cable-read", b"complete=1", b"complete=0"),
    "compliance-tx100": (b"diagnostics.compliance-stop", b"cleanup_owed=1", b"cleanup_owed=0"),
    "irq": (b"irq.service", b"recr_valid=1", b"recr_valid=0"),
    "pins": (b"controls.pins", b"release_status=OK", b"release_status=MDIO_ERROR"),
    "interface-mii": (b"controls.interface", b"mode_valid=1", b"mode_valid=0"),
    "power-ieee": (b"power.read", b"indicated_down=1", b"indicated_down=0"),
    "eee": (b"eee.read", b"negotiated=1", b"negotiated=0"),
    "wol-magic": (b"wol.read", b"magic=1", b"magic=0"),
    "wol-pulse": (b"wol.read", b"rxfs=4097", b"rxfs=4096"),
    "mse": (b"controls.mse", b"raw_mse=4660", b"raw_mse=4661"),
}


class FeaturePlanTests(unittest.TestCase):
    def test_hardware_guide_names_each_feature_case(self):
        guide = (ROOT / "docs/hardware-validation.md").read_text(encoding="utf-8").split("## Feature fixtures", 1)[1]
        cases = {name for line in guide.splitlines() if line.startswith("| `")
                 for name in re.findall(r"`([^\x60]+)`", line.split("|")[1])}
        self.assertEqual(cases, feature_modules().keys())

    def test_feature_scheduler_matches_cli_command_tables(self):
        modes = {}
        for path in (ROOT / "examples/common").glob("Dp8382xCli*Commands.inc"):
            for name, mode in re.findall(r'FEATURE\([A-Z0-9_]+, "([^"]+)", \d+, \d+, ([A-Z]+),',
                                         path.read_text(encoding="utf-8")):
                modes[name.replace(" ", ".")] = mode
        self.assertEqual(runner.FEATURE_READS, {name for name, mode in modes.items() if mode == "READ"})
        self.assertEqual(runner.FEATURE_WRITES, {name for name, mode in modes.items() if mode == "WRITE"})
        self.assertEqual(runner.FEATURE_OPERATIONS, {name for name, mode in modes.items()
                                                     if mode in ("START", "RESET", "OBSERVE")})

    def test_native_scope_retains_blocked_disposition(self):
        with tempfile.TemporaryDirectory() as folder:
            args = serial_arguments(Path(folder))
            args[args.index("passive")] = "campaign"
            args += ["--case", "loopback-mii", "--operating-mode", "enhanced",
                     "--polling", "exclusive", "--clock", "simulated"]
            port = None

            def factory(**options):
                nonlocal port
                port = Port(**options)
                return port

            code, _, err = serial_invoke(args, factory)
            self.assertEqual(code, 2, err)
            self.assertEqual(port.lines, [f"session {NONCE}\n".encode("ascii")])
            result = runner.verify(Path(folder) / "live.json")
            self.assertEqual((result["verdict"], result["cases"][0]["verdict"]), ("INCONCLUSIVE", "BLOCKED"))
            self.assertEqual(port.closes, 1)

            port = None
            args[args.index("--backend") + 1] = "sim-mdio"
            args[args.index("--run") + 1] = "invalid-device-simulation"
            code, _, err = serial_invoke(args, factory)
            self.assertEqual(code, 2, err)
            self.assertIsNone(port)


@unittest.skipUnless(FIXTURE or "--fixture" in sys.argv, "run with the compiled CLI fixture")
class CompiledFeatureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        WORK.mkdir(parents=True, exist_ok=True)
        folder = tempfile.TemporaryDirectory(dir=WORK)
        cls.addClassCleanup(folder.cleanup)
        cls.folder = Path(folder.name)

    def run_case(self, name, suffix="", extra=(), replay_authorized=True):
        module = feature_modules()[name][1]
        run = name + suffix
        args = ["run-fixture", str(FIXTURE), "--suite", "campaign", "--case", name,
                "--target", "host", "--backend", "sim-mdio", "--variant", "DP83826I",
                "--interface", "MII" if name in getattr(module, "MII_CASES", ()) else "RMII",
                "--address", "1", "--firmware", REVISION, "--fixture-identity", REVISION,
                "--clock", "simulated", "--console", "host-stdio", "--baud", "0",
                "--operating-mode", getattr(module, "MODES", {}).get(name, "enhanced"), "--polling", "exclusive",
                "--board", "compiled-P51", "--authorization", "offline-qualification", "--nonce", NONCE,
                "--out-dir", str(self.folder), "--run", run]
        if replay_authorized:
            args.append("--reinitialize")
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()) as err:
            result = runner.main(args + list(extra))
        path = self.folder / (run + ".json")
        self.assertTrue(path.exists(), err.getvalue())
        data = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(runner.verify(path), data)
        return result, data, path.with_suffix(".txt").read_bytes()

    def test_p25_feature_replay(self):
        for role in ("leader", "follower"):
            args = ("--fixture-variant", "DP83825I", "--variant", "DP83825I", "--operating-mode", "none",
                    "--fixture-rmii-role", role, "--rmii-role", role, "--interface", "RMII")
            for name, (_, module) in feature_modules().items():
                if name in module.P25_CASES:
                    with self.subTest(role=role, case=name):
                        result, data, raw = self.run_case(name, "-p25-" + role, args)
                        self.assertEqual((result, data["verdict"]), (0, "PASS"), (data["issues"], data["cases"]))

    def test_complete_cases_and_every_required_field(self):
        mutations = 0
        for name, (_, module) in feature_modules().items():
            with self.subTest(case=name):
                result, data, raw = self.run_case(name)
                prelude = getattr(module, "BASELINES", {}).get(name, ())
                self.assertLessEqual(len(data["commands"]) + len(prelude), MAX_COMMANDS)
                self.assertLessEqual(len(data["campaign"]["interventions"]), MAX_INTERVENTIONS)
                row = data["cases"][0]
                if name in getattr(module, "BLOCKED_CASES", ()):
                    self.assertEqual((result, row["verdict"]), (2, "BLOCKED"), data)
                    continue
                self.assertEqual((result, row["verdict"]), (0, "PASS"), data)
                if module.CASES[name][1]:
                    self.assertEqual(row["restoration"]["verdict"], "PASS")
                self.assertEqual(replay(data, decoded(raw)), "PASS")
                mutations += assert_required_fields(self, name, data, raw)
                if name in CONTRADICTIONS:
                    command, old, new = CONTRADICTIONS[name]
                    lines = row_lines(raw)
                    start = next(index for index, line in enumerate(lines) if b"nonce=" in line)
                    line = next(line for line in lines[start:] if b"cmd=" + command in line.split()
                                and old in line.split())
                    altered = raw.replace(line, line.replace(old, new, 1), 1)
                    self.assertNotEqual(replay(data, decoded(altered)), "PASS")
        self.assertGreater(mutations, 30_000)

    def test_a_feature_cases_require_their_generation_profiles(self):
        for grade in ("DP83826AI", "DP83826AE"):
            args = ("--fixture-variant", grade, "--variant", grade)
            for name, (_, module) in feature_modules().items():
                required_mode = getattr(module, "MODES", {}).get(name)
                for mode in ((required_mode,) if required_mode else ("basic", "enhanced")):
                    with self.subTest(grade=grade, case=name, mode=mode):
                        result, data, raw = self.run_case(name, f"-{grade}-{mode}",
                                                          args + ("--operating-mode", mode))
                        row, = data["cases"]
                        if name in getattr(module, "BLOCKED_CASES", ()):
                            self.assertEqual((result, row["verdict"]), (2, "BLOCKED"))
                        else:
                            self.assertEqual((result, row["verdict"]), (0, "PASS"), row)
                            if module.CASES[name][1]:
                                self.assertEqual(row["restoration"]["verdict"], "PASS")
                            self.assertEqual(replay(data, decoded(raw)), "PASS")

            mismatch = ("--fixture-variant", "DP83826AE" if grade == "DP83826AI" else "DP83826AI",
                        "--variant", grade)
            result, data, raw = self.run_case("quality", "-swap-" + grade, mismatch)
            self.assertEqual(result, 2)
            self.assertEqual(len(data["commands"]), 1)
            self.assertTrue(all(row.cmd == "session" for row in decoded(raw)))

    def test_a_wake_setup_interruption_and_replay(self):
        from test_hil_soak import assert_soak_required_fields
        for grade in ("DP83826AI", "DP83826AE"):
            args = ("--fixture-variant", grade, "--variant", grade, "--soak-count", "2",
                    "--soak-duration-ms", "8000", "--timeout", "15")
            code, data, raw = self.run_case("wol-secure", "-setup-" + grade, args)
            self.assertEqual(code, 0, data["issues"])
            self.assertEqual([step["state"] for step in data["soak"]["setup"]], ["terminal", "terminal"])
            self.assertGreater(assert_soak_required_fields(self, "wol-secure", data, raw), 0)

            send = runner.FixtureProcess.send
            def lose_ack(fixture, line, deadline=None):
                send(fixture, line, deadline)
                if line == b"wol acknowledge\n":
                    raise runner.FixtureError("injected loss after the pre-nonce acknowledgement")
            with patch.object(runner.FixtureProcess, "send", lose_ack):
                code, data, raw = self.run_case("wol-secure", "-lost-" + grade, args)
            self.assertEqual((code, data["verdict"], data["soak"]["attempted"]), (2, "INCONCLUSIVE", 0))
            self.assertEqual([step["state"] for step in data["soak"]["setup"]], ["terminal", "uncertain"])
            self.assertFalse(any(row.cmd == "session" for row in decoded(raw)))

    def test_e_feature_cases_use_the_shared_cli_and_keep_source_gates(self):
        for name, (_, module) in feature_modules().items():
            with self.subTest(case=name):
                result, data, _ = self.run_case(name, "-e",
                                                ("--fixture-variant", "DP83826E", "--variant", "DP83826E"))
                self.assertEqual(data["identity"]["variant"], "DP83826E")
                row = data["cases"][0]
                if name in getattr(module, "BLOCKED_CASES", ()):
                    self.assertEqual((result, row["verdict"]), (2, "BLOCKED"), data)
                else:
                    self.assertEqual((result, row["verdict"]), (0, "PASS"), data)
                    if module.CASES[name][1]:
                        self.assertEqual(row["restoration"]["verdict"], "PASS")

    def test_primary_partial_capture_and_separate_stop(self):
        send = runner.FixtureProcess.send
        for fail_stop in (False, True):
            def inject(fixture, line, deadline=None):
                if line.startswith(b"diagnostics bist-read"):
                    send(fixture, b"$fail-next 3\n", deadline)
                if fail_stop and line == b"diagnostics bist-stop\n":
                    send(fixture, b"$fail-next 0\n", deadline)
                return send(fixture, line, deadline)

            with patch.object(runner.FixtureProcess, "send", inject):
                result, data, raw = self.run_case("bist-both", f"-partial-{fail_stop}")
            self.assertEqual(result, 1, data)
            row = data["cases"][0]
            self.assertEqual(row["primary"]["code"], "MDIO_ERROR")
            self.assertEqual(row["restoration"]["verdict"], "FAIL" if fail_stop else "PASS")
            capture = next(r for r in decoded(raw) if r.cmd == "diagnostics.bist-read" and r.state == "terminal")
            self.assertEqual((capture.fields["before_valid"], capture.fields["counter_valid"],
                              capture.fields["after_valid"]), ("1", "1", "0"))
            self.assertEqual(data["commands"].count("diagnostics bist-stop"), 1)

    def test_denied_admission_and_missing_replay_permission(self):
        result, data, raw = self.run_case("loopback-mii", "-denied", ("--fixture-deny-maintenance",))
        row = data["cases"][0]
        self.assertEqual((result, row["primary"]["code"]), (1, "BUSY"))
        self.assertNotIn("diagnostics loopback-stop", data["commands"])
        statuses = [r for r in decoded(raw) if r.cmd == "diagnostics.loopback-start"]
        self.assertEqual([(r.state, r.op) for r in statuses], [("terminal", 0)])
        result, data, _ = self.run_case("compliance-tx100", "-no-replay", replay_authorized=False)
        self.assertEqual((result, data["cases"][0]["verdict"]), (2, "BLOCKED"), data)
        self.assertFalse(any(command.startswith("diagnostics compliance-start") for command in data["commands"]))

    def test_retained_irq_prefix_and_failed_stop(self):
        send = runner.FixtureProcess.send

        def inject(fixture, line, deadline=None):
            if line == b"irq service 2 5 5\n":
                send(fixture, b"$fail-next 3\n", deadline)
            if line == b"irq test-stop\n":
                send(fixture, b"$fail-next 1\n", deadline)
            return send(fixture, line, deadline)

        with patch.object(runner.FixtureProcess, "send", inject):
            result, data, raw = self.run_case("irq", "-partial-stop")
        row = data["cases"][0]
        self.assertEqual((result, row["primary"]["code"], row["restoration"]["verdict"]), (1, "MDIO_ERROR", "FAIL"))
        self.assertIn("cached events", data["commands"])
        self.assertIn("irq cached-service", data["commands"])
        self.assertIn("irq test-stop", data["commands"])
        self.assertNotIn("irq configure 0 0 polarity intr 0", data["commands"])
        rows = decoded(raw)
        events = next(r for r in rows if r.cmd == "cached.events")
        self.assertEqual((events.fields["misr1"], events.fields["misr2"]), (str(0xA520), str(0x5A02)))
        cached = next(r for r in rows if r.cmd == "irq.cached-service")
        self.assertEqual((cached.fields["valid"], cached.fields["stale"], cached.fields["last_status"]),
                         ("0", "1", "MDIO_ERROR"))
        self.assertTrue(any(r.cmd == "irq.test-stop" and r.primary == "MDIO_ERROR" for r in rows))

    def test_persistent_debt_requires_known_replay_before_baseline_restore(self):
        send = runner.FixtureProcess.send

        def inject(fixture, line, deadline=None):
            if line.startswith(b"controls analog 0x28,"):
                send(fixture, b"$fail-next 5\n", deadline)
            return send(fixture, line, deadline)

        with patch.object(runner.FixtureProcess, "send", inject):
            for permitted in (False, True):
                result, data, _ = self.run_case("analog", f"-debt-{permitted}", replay_authorized=permitted)
                row = data["cases"][0]
                self.assertEqual((result, row["primary"]["code"], row["restoration"]["verdict"]),
                                 (1, "MDIO_ERROR", "PASS" if permitted else "FAIL"), data)
                self.assertEqual(data["commands"].count("connection initialize none"), int(permitted))

    def test_successful_typed_stop_does_not_cover_failed_link_readback(self):
        send = runner.FixtureProcess.send
        stopping = False

        def inject(fixture, line, deadline=None):
            nonlocal stopping
            if line == b"diagnostics loopback-stop\n":
                stopping = True
            if stopping and line == b"link read 15\n":
                send(fixture, b"$fail-next 0\n", deadline)
            return send(fixture, line, deadline)

        with patch.object(runner.FixtureProcess, "send", inject):
            result, data, raw = self.run_case("loopback-mii", "-bad-exit-readback")
        row = data["cases"][0]
        self.assertEqual((result, row["restoration"]["verdict"]), (1, "FAIL"))
        self.assertEqual(row["primary"], {"code": "OK", "detail": 0})
        self.assertTrue(any(r.cmd == "diagnostics.loopback-stop" and r.primary == "OK" for r in decoded(raw)))
        self.assertIn("link read 15 did not complete cleanly", row["message"])

    def test_exit_requires_the_entry_peer_state(self):
        send = runner.FixtureProcess.send

        def inject(fixture, line, deadline=None):
            if line == b"diagnostics loopback-stop\n":
                send(fixture, b"$campaign-peer open\n", deadline)
            return send(fixture, line, deadline)

        with patch.object(runner.FixtureProcess, "send", inject):
            result, data, _ = self.run_case("loopback-reverse", "-peer-lost")
        row = data["cases"][0]
        self.assertEqual((result, row["primary"]["code"], row["restoration"]["verdict"]), (1, "OK", "FAIL"))
        self.assertIn("ordinary link state differs", row["message"])

    def test_correlated_payload_error_allows_guarded_stop(self):
        send = runner.FixtureProcess.send
        receive = runner.FixtureProcess._next
        cases = (("loopback-mii", "diagnostics loopback-status", b" stimulus_possible=1",
                  "diagnostics loopback-stop"),
                 ("irq", "irq service 2 5 5", b" recr_valid=1", "irq test-stop"))
        for name, command, field, stop in cases:
            with self.subTest(case=name):
                pending = False

                def arm(fixture, line, deadline=None):
                    nonlocal pending
                    if line == command.encode("ascii") + b"\n" and stop not in sent:
                        pending = True
                    sent.append(line.decode("ascii").strip())
                    return send(fixture, line, deadline)

                def omit(fixture, deadline=None):
                    nonlocal pending
                    chunk = receive(fixture, deadline)
                    if not pending or chunk is None:
                        return chunk
                    raw = bytearray(chunk)
                    while b"@DP8382X " not in raw or b"\r\n" not in raw.split(b"@DP8382X ", 1)[1]:
                        chunk = receive(fixture, deadline)
                        self.assertIsNotNone(chunk)
                        raw.extend(chunk)
                    pending = False
                    self.assertIn(field, raw)
                    return bytes(raw).replace(field, b"", 1)

                sent = []
                with (patch.object(runner.FixtureProcess, "send", arm),
                      patch.object(runner.FixtureProcess, "_next", omit)):
                    result, data, _ = self.run_case(name, "-bad-payload")
                row = data["cases"][0]
                self.assertEqual((result, row["verdict"], row["restoration"]["verdict"]), (1, "FAIL", "PASS"), data)
                self.assertIn("missing or unexpected", row["message"])
                self.assertEqual(sent.count(stop), 1)

    def test_autorun_retains_admitted_intent_and_requires_authorized_debt_replay(self):
        send = runner.FixtureProcess.send
        for offset, permitted, restoration in ((3, False, "PASS"), (8, False, "FAIL"), (8, True, "PASS")):
            with self.subTest(offset=offset, replay=permitted):
                def inject(fixture, line, deadline=None):
                    if line == b"diagnostics cable-autorun on\n":
                        send(fixture, f"$fail-next {offset}\n".encode("ascii"), deadline)
                    return send(fixture, line, deadline)

                with patch.object(runner.FixtureProcess, "send", inject):
                    result, data, _ = self.run_case("cable-autorun", f"-mdio-{offset}-{permitted}",
                                                    replay_authorized=permitted)
                row = data["cases"][0]
                self.assertEqual((result, row["primary"]["code"], row["restoration"]["verdict"]),
                                 (1, "MDIO_ERROR", restoration), data)
                self.assertEqual(data["commands"].count("connection initialize none"), int(permitted))
                self.assertEqual(data["commands"].count("diagnostics cable-autorun off"), int(restoration == "PASS"))

    def test_failed_start_without_temporary_effect_does_not_send_stop(self):
        send = runner.FixtureProcess.send
        for name, start, stop in (("lpi", b"lpi start\n", "lpi stop"), ("irq", b"irq test-start\n", "irq test-stop")):
            with self.subTest(case=name):
                def inject(fixture, line, deadline=None):
                    if line == start:
                        send(fixture, b"$fail-next 0\n", deadline)
                    return send(fixture, line, deadline)

                with patch.object(runner.FixtureProcess, "send", inject):
                    result, data, _ = self.run_case(name, "-no-temporary-effect")
                row = data["cases"][0]
                self.assertEqual((result, row["primary"]["code"]), (1, "MDIO_ERROR"), data)
                self.assertNotIn(stop, data["commands"])
                if name == "lpi":
                    self.assertEqual(row["restoration"]["verdict"], "PASS")

    def test_incompatible_interface_blocks_before_baseline_prelude(self):
        cases = (("collision-10", "RMII"), ("repeater-leader", "MII"), ("pins", "MII"))
        for name, interface in cases:
            with self.subTest(case=name):
                result, data, raw = self.run_case(name, "-wrong-interface", ("--interface", interface))
                self.assertEqual((result, data["cases"][0]["verdict"]), (2, "BLOCKED"), data)
                self.assertEqual([row.cmd for row in decoded(raw)], ["session"])

    def test_timeout_with_output_pressure_keeps_primary_and_releases_cable(self):
        start = runner.FixtureProcess.__init__
        send = runner.FixtureProcess.send

        def hold(fixture, argv, *args):
            start(fixture, [*argv, "--hold-tdr"], *args)

        def stall(fixture, line, deadline=None):
            if line.startswith(b"diagnostics cable-start"):
                send(fixture, b"$output-stall 256\n", deadline)
            return send(fixture, line, deadline)

        with (patch.object(runner.FixtureProcess, "__init__", hold),
              patch.object(runner.FixtureProcess, "send", stall)):
            result, data, raw = self.run_case("cable-manual", "-timeout")
        row = data["cases"][0]
        self.assertEqual((result, row["primary"]["code"], row["restoration"]["verdict"]),
                         (1, "OPERATION_TIMEOUT", "PASS"), data)
        terminal = next(r for r in decoded(raw) if r.cmd == "diagnostics.cable-start" and r.state == "terminal")
        self.assertEqual((terminal.fields["complete"], terminal.fields["cleanup_owed"]), ("0", "1"))

    def test_fixture_reboot_prevents_hazardous_commands(self):
        start = runner.FixtureProcess.__init__
        _, good, _ = self.run_case("loopback-mii", "-reboot-reference")
        index = good["commands"].index("diagnostics loopback-status")

        def reboot(fixture, argv, *args):
            start(fixture, [*argv, "--reboot-after", str(index + 1)], *args)

        with patch.object(runner.FixtureProcess, "__init__", reboot):
            result, data, _ = self.run_case("loopback-mii", "-reboot", ("--timeout", "2"))
        self.assertEqual((result, data["verdict"]), (2, "INCONCLUSIVE"), data)
        self.assertEqual(data["commands"], good["commands"][:index + 1])
        self.assertIn("restoration was not proved", data["cases"][0]["message"])

    def test_interrupted_fixture_retains_possible_effect_and_stops_sending(self):
        send = runner.FixtureProcess.send

        def interrupt(fixture, line, deadline=None):
            if line == b"diagnostics loopback-status\n":
                raise KeyboardInterrupt
            return send(fixture, line, deadline)

        with patch.object(runner.FixtureProcess, "send", interrupt):
            result, data, _ = self.run_case("loopback-mii", "-interrupted")
        self.assertEqual((result, data["verdict"]), (130, "INCONCLUSIVE"), data)
        self.assertIn("restoration was not proved", data["cases"][0]["message"])
        self.assertNotIn("diagnostics loopback-stop", data["commands"])


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
