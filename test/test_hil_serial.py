"""Exercise serial sessions with injected drivers and the compiled CLI fixture."""

import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

from test_hil_runner import NONCE, REVISION, passive, runner
import dp8382x_hil_artifacts as artifacts
import dp8382x_hil_serial as serial_transport
from dp8382x_hil_fixture import FixtureProcess


FIXTURE = None
RECOVERY = "recover-49"
PORT = "COM11" if os.name == "nt" else "/dev/ttyUSB11"


def arguments(path, name="live", flags=("--reset-silent", "--clean-input")):
    return ["run-serial", "--suite", "passive", "--target", "esp32p4",
            "--backend", "p4-netlist-basic", "--variant", "DP83826I", "--interface", "RMII",
            "--address", "1", "--firmware", REVISION, "--console", "uart0", "--baud", "115200",
            "--port", PORT, "--board", "host-test-board", "--authorization", "SIM-49",
            *flags, "--out-dir", str(path), "--run", name, "--session-timeout", "2",
            "--duration", "0.85", "--command-timeout", "0.20", "--cleanup-timeout", "0.32",
            "--finalize-timeout", "0.45", "--tail-timeout", "0.035"]


def device_identity(raw):
    """Project the compiled host identity onto the simulated serial declaration."""
    return (raw.replace(b"target=host ", b"target=esp32p4 ")
            .replace(b"backend=sim-mdio ", b"backend=p4-netlist-basic ")
            .replace(b"console=host-stdio baud=0", b"console=uart0 baud=115200"))


def responses():
    rows = passive()
    rows[0] = device_identity(rows[0].replace(b"firmware=unknown", b"firmware=" + REVISION.encode("ascii")))
    return rows


def recovery_reply(boot=42, req=3):
    return (responses()[0].replace(NONCE.encode("ascii"), RECOVERY.encode("ascii"))
            .replace(b"boot=42 ", f"boot={boot} ".encode("ascii"))
            .replace(b"req=1 ", f"req={req} ".encode("ascii"))
            .replace(b"time_us=1001 ", f"time_us={1000 + req} ".encode("ascii")))


class Port:
    """A finite driver double with no flush, drain, reset or reconnect methods."""

    def __init__(self, **options):
        self.options = options
        self.dtr = self.rts = True
        self.port = None
        self.opens = self.closes = 0
        self.lines = []
        self.rx = bytearray()
        self.rows = responses()

    def open(self):
        assert self.options["port"] is None and self.port == PORT
        assert self.dtr is self.rts is False
        assert self.options["timeout"] == 0 and self.options["write_timeout"] > 0
        assert all(self.options[flag] is False for flag in ("xonxoff", "rtscts", "dsrdtr"))
        self.opens += 1

    def write(self, line):
        self.lines.append(line)
        self.rx.extend(self.rows[len(self.lines) - 1])
        return len(line)

    def read(self, size):
        count = min(size, 23)
        chunk = bytes(self.rx[:count])
        del self.rx[:count]
        return chunk

    def close(self):
        self.closes += 1


def invoke(args, factory):
    """Run with deterministic nonces and no Git or serial package lookup."""
    with (patch.object(runner.secrets, "token_hex", side_effect=[NONCE, RECOVERY]),
          patch.object(runner, "runner_identity", return_value={"revision": "unknown", "tracked_tree": "unknown"}),
          contextlib.redirect_stdout(io.StringIO()) as out,
          contextlib.redirect_stderr(io.StringIO()) as err):
        code = runner.main(args, serial_factory=factory)
    return code, out.getvalue(), err.getvalue()


class NeverReturns(Port):
    def open(self):
        if self.phase == "open":
            threading.Event().wait()
        super().open()

    def read(self, size):
        if self.phase == "read":
            threading.Event().wait()
        return super().read(size)

    def write(self, line):
        if self.phase == "write":
            threading.Event().wait()
        return super().write(line)

    def close(self):
        if self.phase == "close":
            threading.Event().wait()
        super().close()


class SerialTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.path = Path(directory.name)
        self.backend = None

    def run_port(self, kind=Port, name="live", extra=()):
        def factory(**settings):
            self.backend = kind(**settings)
            return self.backend

        status, _, error = invoke(arguments(self.path, name) + list(extra), factory)
        path = self.path / (name + ".json")
        data = json.loads(path.read_text(encoding="utf-8"))
        return status, error, data, path.with_suffix(".txt").read_bytes()

    def test_preview_and_invalid_fixture_do_not_import_or_open(self):
        factory = Mock(side_effect=AssertionError("port accessed"))
        with patch.object(runner, "native_port", side_effect=AssertionError("native port accessed")):
            code, out, err = invoke(arguments(self.path) + ["--dry-run"], factory)
            self.assertEqual(code, 0, err)
            preview = json.loads(out)
            self.assertEqual(preview["scope"], "serial_session")
            self.assertFalse(preview["executed"])
            self.assertEqual(preview["commands"][0], "session preview")
            for args in (
                arguments(self.path, flags=()),
                arguments(self.path, flags=("--reset-silent",)),
                arguments(self.path) + ["--port", "socket://localhost:7000"],
                arguments(self.path) + ["--authorization", ""],
                arguments(self.path) + ["--authorization", "unknown"],
                arguments(self.path) + ["--suite", "fixture-operations"],
                arguments(self.path) + ["--console", "usb-serial-jtag"],
                arguments(self.path) + ["--baud", "0"],
                arguments(self.path) + ["--session-timeout", "0.5"],
                arguments(self.path) + ["--duration", "0.01"],
                arguments(self.path) + ["--run", "../outside"],
            ):
                with self.subTest(args=args):
                    self.assertEqual(invoke(args, factory)[0], 2)
        factory.assert_not_called()
        self.assertFalse(list(self.path.iterdir()))

    def test_reservation_collision_does_not_touch_port(self):
        path = self.path / "live.txt"
        path.write_bytes(b"another run")
        factory = Mock(side_effect=AssertionError("port accessed"))
        code, _, error = invoke(arguments(self.path), factory)
        self.assertEqual(code, 2)
        self.assertIn("reserve evidence", error)
        factory.assert_not_called()
        self.assertEqual(path.read_bytes(), b"another run")
        self.assertFalse(path.with_suffix(".json").exists())

    def test_preopen_provisional_and_fragmented_pass_replay(self):
        path = self.path / "live.json"

        class ObserveReservation(Port):
            def open(inner):
                provisional, raw = artifacts.verify_artifacts(path)
                self.assertEqual((provisional["verdict"], raw), ("INCONCLUSIVE", b""))
                self.assertEqual(provisional["serial"]["open"]["state"], "not_started")
                super().open()

        status, error, data, raw = self.run_port(ObserveReservation)
        self.assertEqual(status, 0, error)
        self.assertEqual(raw, b"".join(responses()))
        self.assertEqual(self.backend.lines[0], f"session {NONCE}\n".encode())
        self.assertEqual((self.backend.opens, self.backend.closes), (1, 1))
        self.assertEqual(self.backend.options["baudrate"], 115200)
        self.assertEqual(data["scope"], "serial_session")
        self.assertEqual(data["identity"]["firmware_strength"], "exact")
        self.assertEqual(data["serial"]["writes"][0]["bytes"], len(self.backend.lines[0]))
        self.assertEqual(data["serial"]["restoration"], "not_requested")
        self.assertEqual(runner.verify(path), data)
        for key, invalid in (("port", "socket://localhost:7000"), ("authorization", "unknown"),
                             ("backend", "unidentified")):
            with self.subTest(key=key):
                changed = {**data, "serial": {**data["serial"], key: invalid}}
                path.write_text(json.dumps(changed), encoding="utf-8")
                with self.assertRaisesRegex(artifacts.EvidenceError, "fixture declarations"):
                    runner.verify(path)
        data["serial"]["close"]["state"] = "not_started"
        path.write_text(json.dumps(data), encoding="utf-8")
        with self.assertRaisesRegex(artifacts.EvidenceError, "closure"):
            runner.verify(path)

    def test_usb_records_inapplicable_baud_and_requested_states(self):
        class Usb(Port):
            def __init__(self, **options):
                super().__init__(**options)
                self.rows[0] = self.rows[0].replace(b"console=uart0 baud=115200",
                                                    b"console=usb-serial-jtag baud=0")

        status, error, data, _ = self.run_port(Usb, extra=("--console", "usb-serial-jtag", "--baud", "0"))
        self.assertEqual(status, 0, error)
        serial = data["serial"]
        self.assertFalse(serial["baud_applies"])
        self.assertTrue(serial["reset_silent"])
        self.assertFalse(serial["requested_dtr"] or serial["requested_rts"])
        self.assertEqual(data["identity"]["baud"], 0)
        self.assertEqual(self.backend.options["baudrate"], serial["driver_baud"])

    def test_control_line_rejected_and_open_error_both_close_once(self):
        class Unsupported(Port):
            def __setattr__(self, name, value):
                if name == "rts" and not value:
                    raise OSError("cannot set RTS")
                super().__setattr__(name, value)

        class OpenError(Port):
            def open(self):
                super().open()
                raise OSError("device disappeared during open")

        for kind, opens, message in ((Unsupported, 0, "cannot set RTS"), (OpenError, 1, "disappeared")):
            with self.subTest(kind=kind):
                code, error, data, raw = self.run_port(kind, name=kind.__name__)
                self.assertEqual(code, 2, error)
                self.assertIn(message, error)
                self.assertEqual(data["serial"]["open"]["state"], "error")
                self.assertEqual((self.backend.opens, self.backend.closes, raw), (opens, 1, b""))
                self.assertFalse(self.backend.lines)

    def test_short_zero_or_failed_write_never_replays_or_sends_cleanup(self):
        for result in (0, 3, OSError("write disconnected"), KeyboardInterrupt()):
            class Short(Port):
                def write(self, line):
                    if len(self.lines) == 5:
                        self.lines.append(line)
                        if isinstance(result, BaseException):
                            raise result
                        return result
                    return super().write(line)

            with self.subTest(result=result):
                name = "short-" + str(result) if type(result) is int else type(result).__name__
                code, error, data, raw = self.run_port(Short, name=name)
                self.assertEqual(code, 130 if isinstance(result, KeyboardInterrupt) else 2, error)
                self.assertEqual(len(self.backend.lines), 6)
                self.assertTrue(self.backend.lines[-1].startswith(b"registers write "))
                self.assertEqual(len(data["commands"]), 5)
                self.assertEqual(data["serial"]["writes"][-1]["bytes"], result if type(result) is int else None)
                self.assertTrue(data["serial"]["framing_uncertain"])
                self.assertEqual(data["serial"]["restoration"], "unavailable")
                self.assertEqual(data["serial"]["recovery"]["state"], "not_started")
                self.assertEqual(self.backend.closes, 1)
                self.assertEqual(raw, b"".join(responses()[:5]))

    def test_posix_native_short_write_and_would_block_have_no_retry(self):
        class NativePosix:
            __module__ = "serial.serialposix"

            def __init__(self, **options):
                self.options = options

            def open(self):
                pass

            def fileno(self):
                return 17

            def write(self, line):
                raise AssertionError("pySerial's retrying writer was invoked")

        serial = SimpleNamespace(__version__="3.5", Serial=NativePosix)
        text = f"session {NONCE}"
        for count in (3, None):
            with self.subTest(count=count):
                write = Mock(return_value=count, side_effect=BlockingIOError("would block") if count is None else None)
                platform = SimpleNamespace(name="posix", write=write)
                with (patch.dict(sys.modules, {"serial": serial}), patch.object(serial_transport, "os", platform)):
                    owner = serial_transport.SerialPort({"port": "/dev/ttyUSB11", "driver_baud": 115200,
                                                         "limits": {"command_timeout": 0.2}},
                                                        serial_transport.native_port)
                    deadline = time.monotonic() + 1
                    owner.open(deadline)
                    self.assertEqual(owner.port.options["write_timeout"], 0)
                    with self.assertRaises(serial_transport.SerialError):
                        owner.send(text, deadline)
                    with self.assertRaisesRegex(serial_transport.SerialError, "framing is uncertain"):
                        owner.send(f"session {RECOVERY}", deadline)
                    write.assert_called_once_with(17, (text + "\n").encode("ascii"))
                    self.assertEqual(owner.writes[0]["bytes"], count)

    def test_record_started_before_write_cannot_supply_next_terminal(self):
        prefix_length = len(b"@DP8382X schema=1")

        class Unsolicited(Port):
            def __init__(self, **options):
                super().__init__(**options)
                self.rows[0] += self.rows[1][:prefix_length]
                self.rows[1] = self.rows[1][prefix_length:]

            def read(self, size):
                chunk = bytes(self.rx[:size])
                del self.rx[:size]
                return chunk

        code, error, data, raw = self.run_port(Unsolicited)
        self.assertEqual(code, 2, error)
        self.assertIn("unsolicited record began", error)
        self.assertEqual(self.backend.lines, [f"session {NONCE}\n".encode()])
        self.assertFalse(data["complete"])
        self.assertEqual(raw, responses()[0] + responses()[1][:prefix_length])

    def test_disconnect_and_identity_mismatch_keep_available_evidence(self):
        class Disconnect(Port):
            def read(self, size):
                if len(self.lines) == 2:
                    raise OSError("device lost")
                return super().read(size)

        class Wrong(Port):
            def __init__(self, **options):
                super().__init__(**options)
                self.rows[0] = self.rows[0].replace(REVISION.encode("ascii"), b"unknown")

        code, error, data, raw = self.run_port(Disconnect)
        self.assertEqual(code, 2, error)
        self.assertIn("device lost", error)
        self.assertEqual(len(self.backend.lines), 2)
        self.assertEqual(raw, responses()[0])
        self.assertFalse(data["serial"]["framing_uncertain"])
        code, error, data, _ = self.run_port(Wrong, "wrong")
        self.assertEqual(code, 2, error)
        self.assertIn("identity mismatch", error)
        self.assertEqual(data["identity"]["firmware"], "unknown")
        self.assertEqual(len(self.backend.lines), 1)
        self.assertTrue(all(not case["executed"] for case in data["cases"]))

    def test_timeout_never_promotes_late_fragment_and_recovers_once(self):
        for name, reply, state in (
            ("same-boot", recovery_reply(), "continuous"),
            ("changed-boot", recovery_reply(boot=43), "failed"),
            ("wrong-request", recovery_reply(req=4), "failed"),
            ("missing", b"", "failed"),
        ):
            original = responses()[1]
            split = original.index(b"attempted=30") + len(b"attempted=3")

            class Late(Port):
                def __init__(self, **options):
                    super().__init__(**options)
                    self.rows[1] = original[:split]
                    self.rows[2] = original[split:] + reply

            with self.subTest(name=name):
                code, error, data, raw = self.run_port(Late, name=name)
                self.assertEqual(code, 2, error)
                self.assertIn("receive deadline", error)
                self.assertEqual(self.backend.lines, [f"session {NONCE}\n".encode(), b"cached transport\n",
                                                       f"session {RECOVERY}\n".encode()])
                self.assertEqual(raw, responses()[0] + original + reply)
                self.assertEqual(data["serial"]["recovery"]["state"], state)
                self.assertTrue(all(not case["executed"] for case in data["cases"]))
                self.assertFalse(data["complete"])
                self.assertEqual(self.backend.closes, 1)

    def test_remaining_active_budget_clips_commands(self):
        class NoReply(Port):
            def __init__(self, **options):
                super().__init__(**options)
                self.rows = [b""] * len(self.rows)

        start = time.monotonic()
        code, error, data, _ = self.run_port(NoReply, extra=("--duration", "0.07", "--command-timeout", "8"))
        self.assertEqual(code, 2, error)
        self.assertLess(time.monotonic() - start, 1.5)
        self.assertEqual(data["serial"]["recovery"]["state"], "failed")
        self.assertEqual(len(self.backend.lines), 2)

    def test_closure_duplicate_and_byte_cap_retain_observed_rx(self):
        class Surplus(Port):
            def __init__(self, **options):
                super().__init__(**options)
                self.rows[-1] *= 2

        code, error, data, raw = self.run_port(Surplus)
        self.assertEqual(code, 2, error)
        self.assertIn("duplicate record", error)
        self.assertEqual(raw, b"".join(self.backend.rows))
        self.assertFalse(data["complete"])
        code, error, data, raw = self.run_port(Port, "bounded", ("--max-bytes", "120"))
        self.assertEqual(code, 2, error)
        self.assertTrue(data["transcript"]["truncated"])
        self.assertEqual(raw, responses()[0][:120])

    def test_close_and_case_failure_are_retained_independently(self):
        class CloseError(Port):
            def __init__(self, **options):
                super().__init__(**options)
                self.rows[5] = self.rows[5].replace(b"primary=INVALID_PARAM detail=0",
                                                   b"primary=MDIO_ERROR detail=-7").replace(
                                                       b"reason=range", b"reason=none")

            def close(self):
                super().close()
                raise OSError("port close failed")

        code, error, data, _ = self.run_port(CloseError)
        self.assertEqual(code, 1, error)
        self.assertIn("port close failed", error)
        self.assertEqual(data["serial"]["close"]["state"], "error")
        self.assertEqual(data["cases"][1]["primary"], {"code": "MDIO_ERROR", "detail": -7})
        self.assertEqual(self.backend.closes, 1)

    def test_replace_error_keeps_provisional_manifest_and_transcript(self):
        with patch.object(artifacts.os, "replace", side_effect=OSError("publish denied")):
            code, error, data, raw = self.run_port()
        self.assertEqual(code, 2, error)
        self.assertIn("publish denied", error)
        self.assertEqual(data["issues"], ["Run not finalized"])
        self.assertEqual(data["verdict"], "INCONCLUSIVE")
        self.assertEqual(data["serial"]["open"]["state"], "not_started")
        self.assertEqual(raw, b"".join(responses()))
        self.assertEqual(len(list(self.path.iterdir())), 2)
        with self.assertRaisesRegex(artifacts.EvidenceError, "SHA-256"):
            runner.verify(self.path / "live.json")

    def test_stalled_driver_calls_finalize_then_hard_exit(self):
        for phase in ("open", "write", "read", "close", "finalize", "manifest-sync"):
            with self.subTest(phase=phase):
                child = subprocess.run([sys.executable, "-S", __file__, "--stall", phase, str(self.path)],
                                       capture_output=True, text=True, timeout=6)
                self.assertEqual(child.returncode, 2, child.stderr)
                self.assertIn("worker is pending", child.stderr)
                if phase in ("finalize", "manifest-sync"):
                    path = self.path / (phase + ".json")
                    data = json.loads(path.read_text(encoding="utf-8"))
                    self.assertEqual(data["issues"], ["Run not finalized"])
                    self.assertEqual(data["verdict"], "INCONCLUSIVE")
                    self.assertEqual(path.with_suffix(".txt").read_bytes(), b"".join(responses()))
                    self.assertEqual(list(self.path.glob("*.tmp")), [])
                    continue
                data, raw = artifacts.verify_artifacts(self.path / f"{phase}.json")
                self.assertEqual(data["verdict"], "INCONCLUSIVE")
                self.assertTrue(data["serial"]["hard_exit"])
                self.assertEqual(data["serial"]["close"]["state"],
                                 "stalled" if phase == "close" else "not_started")
                self.assertEqual(raw, b"".join(responses()) if phase == "close" else b"")

    def test_stalled_provenance_and_diagnostics_cannot_extend_session(self):
        for phase in ("provenance", "stderr"):
            with self.subTest(phase=phase):
                child = subprocess.run([sys.executable, "-S", __file__, "--stall", phase, str(self.path)],
                                       capture_output=True, text=True, timeout=6)
                self.assertEqual(child.returncode, 2, child.stderr)
                path = self.path / f"{phase}.json"
                if phase == "provenance":
                    self.assertIn("evidence reservation stalled", child.stderr)
                    self.assertFalse(path.exists() or path.with_suffix(".txt").exists())
                else:
                    data, raw = artifacts.verify_artifacts(path)
                    self.assertEqual((data["verdict"], data["serial"]["open"]["state"], raw),
                                     ("INCONCLUSIVE", "stalled", b""))

    def test_compiled_producer_through_serial_owner(self):
        if FIXTURE is None:
            self.skipTest("compiled fixture supplied by cli_hil_serial CTest")

        with FixtureProcess([str(FIXTURE), "--identity", REVISION], 5, 65536, lambda _: None) as fixture:
            class Bridge(Port):
                def write(self, line):
                    self.lines.append(line)
                    fixture.send(line)
                    raw = bytearray()
                    # The passive suite emits one machine terminal per line.
                    while not any(row.startswith(b"@DP8382X ") and row.endswith(b"\n")
                                  for row in raw.splitlines(keepends=True)):
                        chunk = fixture.read()
                        if not chunk:
                            raise OSError("fixture closed before response")
                        raw.extend(chunk)
                    self.rx.extend(device_identity(bytes(raw)))
                    return len(line)

                def close(self):
                    fixture.finish()
                    super().close()

            code, error, data, raw = self.run_port(Bridge, extra=("--command-timeout", "1"))
        self.assertEqual(code, 0, error)
        self.assertIn(b"cmd=session", raw)
        self.assertEqual(data["identity"]["firmware"], REVISION)
        self.assertEqual(data["serial"]["backend"], "injected serial backend")
        self.assertEqual(runner.verify(self.path / "live.json")["verdict"], "PASS")


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "--stall":
        phase = sys.argv[2]
        NeverReturns.phase = "open" if phase == "stderr" else phase
        args = arguments(Path(sys.argv[3]), phase)

        def stall(*args):
            threading.Event().wait()

        if phase == "provenance":
            with patch.object(runner, "runner_identity", side_effect=stall):
                raise SystemExit(runner.main(args, serial_factory=NeverReturns))
        if phase == "stderr":
            sys.stderr = SimpleNamespace(write=stall, flush=stall)
        with (patch.object(runner.secrets, "token_hex", side_effect=[NONCE, RECOVERY]),
              patch.object(runner, "runner_identity",
                           return_value={"revision": "unknown", "tracked_tree": "unknown"})):
            if phase == "finalize":
                with patch.object(artifacts, "_publish", side_effect=stall):
                    raise SystemExit(runner.main(args, serial_factory=NeverReturns))
            if phase == "manifest-sync":
                sync = artifacts._sync

                def stall_manifest(stream):
                    if str(stream.name).endswith(".tmp"):
                        stall()
                    sync(stream)

                with patch.object(artifacts, "_sync", stall_manifest):
                    raise SystemExit(runner.main(args, serial_factory=NeverReturns))
            raise SystemExit(runner.main(args, serial_factory=NeverReturns))
    if len(sys.argv) == 3 and sys.argv[1] == "--fixture":
        FIXTURE = Path(sys.argv[2]).resolve()
        del sys.argv[1:3]
    unittest.main()
