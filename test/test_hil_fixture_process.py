"""Failure and byte-identity checks for the direct compiled-fixture transport."""

import pathlib
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from tools.dp8382x_hil_fixture import FixtureError, FixtureProcess


def child(source):
    return [sys.executable, "-u", "-c", source]


def stop_test_child(fixture):
    process = fixture._process
    if process is not None and process.poll() is None:
        process.kill()
        process.wait(timeout=2)


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

    @unittest.skipUnless(sys.platform == "win32" and sys.implementation.name == "cpython",
                         "exercises CPython's Windows child pipe handles")
    def test_closure_with_retained_pipe_setup_frame(self):
        # A diagnostic reference can keep Popen's temporary pipe handles alive
        # past child exit. Hold its setup frame until both streams have drained.
        setup_code = subprocess.Popen._get_handles.__code__
        retained = []

        def retain_setup(frame, event, arg):
            if event == "return" and frame.f_code is setup_code:
                retained.append(frame)

        script = ("import sys; sys.stdout.buffer.write(b'last stdout'); "
                  "sys.stderr.buffer.write(b'last stderr')")
        captured = []
        trailing = []
        process = FixtureProcess(child(script), 3, 100, captured.append)
        previous_profile = sys.getprofile()
        try:
            sys.setprofile(retain_setup)
            with process:
                sys.setprofile(previous_profile)
                self.assertEqual(len(retained), 1)
                self.assertEqual(process._process.wait(timeout=2), 0)
                stderr = process.finish(on_trailing=trailing.append)
        finally:
            sys.setprofile(previous_profile)
            for frame in retained:
                frame.clear()
            for reader in process._readers.values():
                reader.join(timeout=1)
        self.assertEqual(process.returncode, 0)
        self.assertEqual(process.cleanup_issue, "")
        self.assertEqual(b"".join(captured), b"last stdout")
        self.assertEqual(b"".join(trailing), b"last stdout")
        self.assertEqual(stderr, b"last stderr")

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

    def test_output_limit_delivers_queued_prefix_before_failure(self):
        captured = []
        decoded = []
        prefix = b"completed evidence\r\n"
        script = ("import sys; sys.stdin.buffer.readline(); "
                  f"sys.stdout.buffer.write({prefix!r} + b'x' * 8192)")
        process = FixtureProcess(child(script), 3, 32, captured.append)
        with self.assertRaisesRegex(FixtureError, "byte limit"):
            with process:
                process.send(b"query\n")
                while chunk := process.read():
                    decoded.append(chunk)
        expected = prefix + b"x" * (32 - len(prefix))
        self.assertEqual(b"".join(decoded), expected)
        self.assertEqual(b"".join(captured), expected)
        self.assertIsNotNone(process.returncode)

    def test_known_reader_failure_stops_new_commands(self):
        script = ("import sys; sys.stdout.buffer.write(b'x' * 8192); "
                  "sys.stdout.buffer.flush(); sys.stdin.buffer.readline()")
        process = FixtureProcess(child(script), 3, 32, lambda _: None)
        with process:
            deadline = time.monotonic() + 2
            while not process.truncated and time.monotonic() < deadline:
                time.sleep(0.01)
            self.assertTrue(process.truncated)
            with mock.patch("tools.dp8382x_hil_fixture.os.write") as write:
                with self.assertRaisesRegex(FixtureError, "byte limit"):
                    process.send(b"do not send\n")
            write.assert_not_called()
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

    def test_late_exit_after_last_wait_is_not_success(self):
        script = ("import os, pathlib, sys, time; os.close(1); sys.stdin.buffer.read();\n"
                  "while not pathlib.Path(sys.argv[1]).exists(): time.sleep(0.01)")
        with tempfile.TemporaryDirectory() as directory:
            ready = pathlib.Path(directory) / "may-exit"
            process = FixtureProcess(child(script) + [str(ready)], 3, 100, lambda _: None)
            with process:
                self.assertEqual(process.read(), b"")
                now = time.monotonic()

                def delayed_wake(_):
                    nonlocal now
                    ready.touch()
                    process._process.wait(timeout=2)
                    for reader in process._readers.values():
                        reader.join(timeout=1)
                    now += 0.08

                with mock.patch("tools.dp8382x_hil_fixture.time") as clock:
                    clock.monotonic.side_effect = lambda: now
                    clock.sleep.side_effect = delayed_wake
                    with self.assertRaisesRegex(FixtureError, "closure deadline"):
                        process.finish(closure=0.04)
                    clock.sleep.assert_called_once()
            self.assertEqual(process.returncode, 0)  # The correct exit was outside the closure allowance.

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

    def test_thread_start_failures_keep_the_primary_and_reap(self):
        script = child("import time; time.sleep(20)")
        start = threading.Thread.start
        for failing_index in (0, 1):
            with self.subTest(reader=failing_index):
                process = FixtureProcess(script, 3, 100, lambda _: None)
                self.addCleanup(stop_test_child, process)
                starts = 0

                def start_reader(thread):
                    nonlocal starts
                    index = starts
                    starts += 1
                    if index == failing_index:
                        raise RuntimeError("simulated reader thread exhaustion")
                    start(thread)

                with mock.patch.object(threading.Thread, "start", autospec=True, side_effect=start_reader):
                    with self.assertRaisesRegex(FixtureError, "reader start failed: simulated"):
                        with process:
                            self.fail("fixture should not enter")
                self.assertIsNotNone(process.returncode)
                self.assertTrue(process._process.stdout.closed and process._process.stderr.closed)

        process = FixtureProcess(script, 3, 100, lambda _: None)
        self.addCleanup(stop_test_child, process)
        with process:
            with mock.patch.object(threading.Thread, "start", side_effect=RuntimeError("simulated writer")):
                with self.assertRaisesRegex(FixtureError, "writer start failed: simulated"):
                    process.send(b"line\n")
        self.assertIsNotNone(process.returncode)

    def test_unreaped_child_has_separate_cleanup_evidence(self):
        child_process = mock.Mock(stdin=None, stdout=None, stderr=None)
        child_process.poll.return_value = None
        child_process.wait.side_effect = [subprocess.TimeoutExpired("fixture", 0.2), OSError("wait failed")]
        process = FixtureProcess(["fixture"], 3, 100, lambda _: None)
        process._process = child_process
        self.assertFalse(process.__exit__(RuntimeError, RuntimeError("primary failure"), None))
        self.assertIn("wait failed", process.cleanup_issue)
        self.assertIn("child was not reaped", process.cleanup_issue)
        self.assertIsNone(process.returncode)

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
