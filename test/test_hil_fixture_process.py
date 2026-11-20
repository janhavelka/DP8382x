"""Failure and byte-identity checks for the direct compiled-fixture transport."""

import pathlib
import subprocess
import sys
import time
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from tools.dp8382x_hil_fixture import FixtureError, FixtureProcess


def child(source):
    return [sys.executable, "-u", "-c", source]


class FixtureProcessTest(unittest.TestCase):
    def test_roundtrip_and_trailing_capture_are_exact(self):
        script = ("import sys; line=sys.stdin.buffer.readline(); "
                  "sys.stdout.buffer.write(b'first:'+line); sys.stdout.buffer.flush(); "
                  "sys.stdin.buffer.read(); sys.stdout.buffer.write(b'last\\n'); "
                  "sys.stderr.buffer.write(b'warning\\n')")
        captured = []
        trailing = []
        with FixtureProcess(child(script), 3.0, 1024, captured.append) as process:
            process.send(b"abc\n")
            first = process.read()
            self.assertEqual(first, b"first:abc\n")
            stderr = process.finish(on_trailing=trailing.append)
        self.assertEqual(b"".join(captured), b"first:abc\nlast\n")
        self.assertEqual(b"".join(trailing), b"last\n")
        self.assertEqual(stderr, b"warning\n")
        self.assertEqual(process.returncode, 0)

    def test_stdout_eof_then_normal_finish(self):
        captured = []
        with FixtureProcess(child("import sys;sys.stdout.buffer.write(b'one')"), 3, 100,
                            captured.append) as process:
            self.assertEqual(process.read(), b"one")
            self.assertEqual(process.read(), b"")
            self.assertEqual(process.read(), b"")
            self.assertEqual(process.finish(), b"")
        self.assertEqual(captured, [b"one"])

    def test_nonzero_exit_preserves_stderr_and_reaps(self):
        captured = []
        script = "import sys;sys.stdout.buffer.write(b'data');sys.stderr.buffer.write(b'bad');sys.exit(7)"
        with self.assertRaisesRegex(FixtureError, "status 7"):
            with FixtureProcess(child(script), 3, 100, captured.append) as process:
                process.finish()
        self.assertEqual(b"".join(captured), b"data")
        self.assertEqual(process.stderr, b"bad")
        self.assertEqual(process.returncode, 7)

    def test_output_limit_keeps_bounded_prefix_and_reaps(self):
        captured = []
        script = "import sys;sys.stdout.buffer.write(b'x'*8192);sys.stderr.buffer.write(b'y'*8192)"
        with self.assertRaisesRegex(FixtureError, "byte limit"):
            with FixtureProcess(child(script), 3, 32, captured.append) as process:
                process.finish()
        self.assertLessEqual(len(b"".join(captured)) + len(process.stderr), 32)
        self.assertTrue(process.truncated)
        self.assertIsNotNone(process.returncode)

    def test_read_timeout_terminates_and_reaps(self):
        captured = []
        started = time.monotonic()
        with self.assertRaisesRegex(FixtureError, "deadline"):
            with FixtureProcess(child("import time;time.sleep(20)"), 0.35, 100,
                                captured.append) as process:
                process.read()
        self.assertLess(time.monotonic() - started, 2.0)
        self.assertIsNotNone(process.returncode)

    def test_blocked_write_terminates_and_reaps(self):
        captured = []
        started = time.monotonic()
        with self.assertRaisesRegex(FixtureError, "deadline"):
            with FixtureProcess(child("import time;time.sleep(20)"), 0.4, 1024,
                                captured.append) as process:
                process.send(b"z" * 2_000_000)
        self.assertLess(time.monotonic() - started, 2.0)
        self.assertIsNotNone(process.returncode)

    def test_short_write_is_typed_failure(self):
        captured = []
        with mock.patch("tools.dp8382x_hil_fixture.os.write", return_value=2) as write:
            with self.assertRaisesRegex(FixtureError, "short fixture stdin write"):
                with FixtureProcess(child("import time;time.sleep(20)"), 2, 100,
                                    captured.append) as process:
                    process.send(b"line\n")
        self.assertEqual(write.call_count, 1)
        self.assertIsNotNone(process.returncode)

    def test_closure_timeout_reaps_and_keeps_trailing_bytes(self):
        captured = []
        started = time.monotonic()
        script = "import sys,time;sys.stdout.buffer.write(b'partial');sys.stdout.flush();time.sleep(20)"
        with self.assertRaisesRegex(FixtureError, "closure deadline"):
            with FixtureProcess(child(script), 3, 100, captured.append) as process:
                process.finish(closure=0.3)
        self.assertLess(time.monotonic() - started, 2.0)
        self.assertEqual(b"".join(captured), b"partial")
        self.assertIsNotNone(process.returncode)

    def test_callback_failure_is_typed_and_child_reaped(self):
        def fail(_):
            raise OSError("disk full")

        with self.assertRaisesRegex(FixtureError, "transcript write failed"):
            with FixtureProcess(child("import sys;sys.stdout.buffer.write(b'data')"), 3, 100,
                                fail) as process:
                process.read()
        self.assertIsNotNone(process.returncode)

    def test_early_body_error_reaps_without_masking_it(self):
        captured = []
        with self.assertRaisesRegex(RuntimeError, "caller failed"):
            with FixtureProcess(child("import time;time.sleep(20)"), 3, 100,
                                captured.append) as process:
                raise RuntimeError("caller failed")
        self.assertIsNotNone(process.returncode)

    def test_interruption_reaps_and_termination_escalates(self):
        with self.assertRaises(KeyboardInterrupt):
            with FixtureProcess(child("import time;time.sleep(20)"), 3, 100, lambda _: None) as process:
                raise KeyboardInterrupt
        self.assertIsNotNone(process.returncode)
        child_process = mock.Mock(stdin=None)
        child_process.poll.side_effect = [None, -9]
        child_process.wait.side_effect = [subprocess.TimeoutExpired("fixture", 0.2), -9]
        process = FixtureProcess(["fixture"], 3, 100, lambda _: None)
        process._process = child_process
        process._abort()
        child_process.terminate.assert_called_once()
        child_process.kill.assert_called_once()
        self.assertEqual(child_process.wait.call_count, 2)
        self.assertEqual(process.returncode, -9)

    def test_invalid_arguments_and_start_error(self):
        with self.assertRaises(FixtureError):
            FixtureProcess([], 1, 1, lambda _: None)
        for timeout in (0, -1, float("nan"), float("inf"), True, 121, 10**400):
            with self.subTest(timeout=timeout), self.assertRaises(FixtureError):
                FixtureProcess(["program"], timeout, 1, lambda _: None)
        for limit in (0, -1, True, 1.5, 1_048_577):
            with self.subTest(limit=limit), self.assertRaises(FixtureError):
                FixtureProcess(["program"], 1, limit, lambda _: None)
        with self.assertRaisesRegex(FixtureError, "start failed"):
            with FixtureProcess(["this-program-cannot-exist-dp8382x"], 1, 1,
                                lambda _: None):
                pass


if __name__ == "__main__":
    unittest.main()
