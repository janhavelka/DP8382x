"""Finite direct-child transport for the compiled offline CLI fixture.

The caller owns protocol parsing and transcript files. This module only moves
bytes, bounds both pipes, and reaps one child. No shell or serial port is used.
"""

from __future__ import annotations

import math
import os
import queue
import subprocess
import threading
import time
from collections.abc import Callable, Sequence


class FixtureError(ValueError):
    """A fixture start, transfer, limit, deadline, or exit failed."""


class FixtureProcess:
    """One bounded fixture process and its two concurrently drained pipes.

    ``timeout`` is the aggregate lifetime from launch through normal closure.
    ``limit`` bounds the sum of accepted stdout and stderr bytes. ``on_stdout``
    receives accepted bytes once, on the calling thread, before ``read`` returns
    them or ``finish`` delivers trailing bytes. On failure, captured unread
    stdout is delivered to ``on_stdout`` before cleanup when possible.
    """

    _CHUNK = 4096
    _POLL = 0.02
    _TERM_GRACE = 0.2

    def __init__(self, argv: Sequence[str], timeout: float, limit: int,
                 on_stdout: Callable[[bytes], None]):
        if (not argv or isinstance(argv, (str, bytes)) or
                any(not isinstance(part, str) or not part for part in argv)):
            raise FixtureError("fixture argv must contain nonempty strings")
        if not isinstance(timeout, (int, float)) or isinstance(timeout, bool) or not math.isfinite(timeout) or timeout <= 0:
            raise FixtureError("fixture timeout must be finite and positive")
        if not isinstance(limit, int) or isinstance(limit, bool) or limit <= 0:
            raise FixtureError("fixture byte limit must be a positive integer")
        if not callable(on_stdout):
            raise FixtureError("fixture stdout callback is required")
        self._argv = tuple(argv)
        self._timeout = float(timeout)
        self._limit = limit
        self._on_stdout = on_stdout
        self._process: subprocess.Popen[bytes] | None = None
        self._deadline = 0.0
        self._chunks: queue.Queue[bytes | None] = queue.Queue()
        self._threads: list[threading.Thread] = []
        self._writer: threading.Thread | None = None
        self._lock = threading.Lock()
        self._bytes = 0
        self._stderr = bytearray()
        self._truncated = False
        self._reader_error: FixtureError | None = None
        self._stdout_eof = False
        self._finished = False
        self.returncode: int | None = None

    @property
    def stderr(self) -> bytes:
        """Bounded stderr prefix, including after a failed transfer."""
        with self._lock:
            return bytes(self._stderr)

    @property
    def truncated(self) -> bool:
        """Whether the shared stdout/stderr byte limit discarded any data."""
        with self._lock:
            return self._truncated

    def __enter__(self) -> FixtureProcess:
        if self._process is not None:
            raise FixtureError("fixture already started")
        self._deadline = time.monotonic() + self._timeout
        try:
            self._process = subprocess.Popen(self._argv, stdin=subprocess.PIPE,
                                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                             bufsize=0)
        except (OSError, ValueError) as exc:
            raise FixtureError(f"fixture start failed: {exc}") from exc
        for name, pipe in (("stdout", self._process.stdout), ("stderr", self._process.stderr)):
            if pipe is None:
                self._abort()
                raise FixtureError(f"fixture {name} pipe unavailable")
            thread = threading.Thread(target=self._drain, args=(name, pipe), daemon=True)
            self._threads.append(thread)
            thread.start()
        return self

    def __exit__(self, error_type, error, traceback) -> bool:
        if not self._finished:
            if error_type is None:
                self.finish()
            else:
                self._abort()
        return False

    def _remaining(self, local_deadline: float | None = None) -> float:
        end = min(self._deadline, local_deadline) if local_deadline is not None else self._deadline
        return end - time.monotonic()

    def _reserve(self, chunk: bytes, stderr: bool) -> bytes:
        with self._lock:
            available = max(0, self._limit - self._bytes)
            accepted = chunk[:available]
            self._bytes += len(accepted)
            if stderr:
                self._stderr.extend(accepted)
            if len(accepted) != len(chunk) and self._reader_error is None:
                self._reader_error = FixtureError("fixture output byte limit exceeded")
            if len(accepted) != len(chunk):
                self._truncated = True
        return accepted

    def _drain(self, name: str, pipe) -> None:
        try:
            while True:
                chunk = os.read(pipe.fileno(), self._CHUNK)
                if not chunk:
                    break
                accepted = self._reserve(chunk, name == "stderr")
                if name == "stdout" and accepted:
                    self._chunks.put(accepted)
        except OSError as exc:
            with self._lock:
                if self._reader_error is None:
                    self._reader_error = FixtureError(f"fixture {name} read failed: {exc}")
        finally:
            if name == "stdout":
                self._chunks.put(None)
            try:
                pipe.close()
            except OSError:
                pass

    def _failure(self) -> FixtureError | None:
        with self._lock:
            return self._reader_error

    def _capture(self, chunk: bytes) -> None:
        try:
            self._on_stdout(chunk)
        except Exception as exc:
            raise FixtureError(f"fixture transcript write failed: {exc}") from exc

    def _next(self, local_deadline: float | None = None) -> bytes | None:
        while True:
            failure = self._failure()
            if failure is not None:
                raise failure
            remaining = self._remaining(local_deadline)
            if remaining <= 0:
                label = "fixture closure deadline expired" if local_deadline is not None and local_deadline <= self._deadline else "fixture deadline expired"
                raise FixtureError(label)
            try:
                return self._chunks.get(timeout=min(self._POLL, remaining))
            except queue.Empty:
                continue

    def _require_live(self) -> subprocess.Popen[bytes]:
        if self._process is None or self._finished:
            raise FixtureError("fixture is not running")
        return self._process

    def send(self, line: bytes) -> None:
        """Write all bytes or fail by the aggregate deadline; no implicit newline."""
        process = self._require_live()
        if not isinstance(line, bytes) or not line:
            raise FixtureError("fixture command must be nonempty bytes")
        if self._remaining() <= 0:
            self._abort()
            raise FixtureError("fixture deadline expired")
        done = threading.Event()
        failure: list[BaseException] = []

        def write() -> None:
            try:
                if process.stdin is None:
                    raise OSError("fixture stdin pipe unavailable")
                descriptor = process.stdin.fileno()
                count = os.write(descriptor, line)
                if count != len(line):
                    raise OSError(f"short fixture stdin write ({count}/{len(line)} bytes)")
            except BaseException as exc:
                failure.append(exc)
            finally:
                done.set()

        worker = threading.Thread(target=write, daemon=True)
        self._writer = worker
        worker.start()
        try:
            while not done.wait(min(self._POLL, max(0, self._remaining()))):
                reader_failure = self._failure()
                if reader_failure is not None:
                    raise reader_failure
                if self._remaining() <= 0:
                    raise FixtureError("fixture stdin write deadline expired")
            if failure:
                raise FixtureError(f"fixture stdin write failed: {failure[0]}")
            if self._remaining() <= 0:
                raise FixtureError("fixture stdin write completed after the deadline")
        except FixtureError:
            self._abort()
            raise

    def read(self) -> bytes:
        """Return the next stdout chunk, or empty bytes after its EOF."""
        self._require_live()
        if self._stdout_eof:
            return b""
        try:
            chunk = self._next()
            if chunk is None:
                self._stdout_eof = True
                return b""
            self._capture(chunk)
            return chunk
        except FixtureError:
            self._abort()
            raise

    def finish(self, closure: float = 0.5,
               on_trailing: Callable[[bytes], None] | None = None) -> bytes:
        """Close input, deliver trailing stdout, and reap before both deadlines.

        ``on_trailing`` receives each unread stdout chunk after ``on_stdout``.
        The returned bytes are the bounded stderr prefix. A nonzero exit or an
        incomplete closure raises ``FixtureError`` and preserves that prefix.
        """
        process = self._require_live()
        if not isinstance(closure, (int, float)) or isinstance(closure, bool) or not math.isfinite(closure) or closure <= 0:
            raise FixtureError("fixture closure must be finite and positive")
        local_deadline = time.monotonic() + closure
        try:
            if process.stdin is not None and not process.stdin.closed:
                process.stdin.close()
            while not self._stdout_eof:
                chunk = self._next(local_deadline)
                if chunk is None:
                    self._stdout_eof = True
                else:
                    self._capture(chunk)
                    if on_trailing is not None:
                        on_trailing(chunk)
            while process.poll() is None or any(thread.is_alive() for thread in self._threads):
                failure = self._failure()
                if failure is not None:
                    raise failure
                if self._remaining(local_deadline) <= 0:
                    raise FixtureError("fixture closure deadline expired")
                time.sleep(min(self._POLL, self._remaining(local_deadline)))
            failure = self._failure()
            if failure is not None:
                raise failure
            self.returncode = process.returncode
            self._finished = True
            if self.returncode != 0:
                raise FixtureError(f"fixture exited with status {self.returncode}")
            return self.stderr
        except FixtureError:
            self._abort()
            raise
        except Exception as exc:
            self._abort()
            raise FixtureError(f"fixture closure failed: {exc}") from exc

    def _abort(self) -> None:
        """Best-effort bounded direct-child cleanup without hiding primary errors."""
        process = self._process
        if process is None or self._finished:
            return
        if process.poll() is None:
            try:
                process.terminate()
                process.wait(timeout=self._TERM_GRACE)
            except (OSError, subprocess.TimeoutExpired):
                try:
                    process.kill()
                    process.wait(timeout=self._TERM_GRACE)
                except (OSError, subprocess.TimeoutExpired):
                    pass
        self.returncode = process.poll()
        if process.stdin is not None and not process.stdin.closed:
            try:
                process.stdin.close()
            except OSError:
                pass
        for thread in self._threads:
            thread.join(timeout=self._TERM_GRACE)
        if self._writer is not None:
            self._writer.join(timeout=self._TERM_GRACE)
        # Preserve already drained stdout, including bytes after the last read.
        while True:
            try:
                chunk = self._chunks.get_nowait()
            except queue.Empty:
                break
            if chunk:
                try:
                    self._capture(chunk)
                except FixtureError:
                    break
        self._finished = True
