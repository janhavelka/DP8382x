"""Single-owner native serial I/O with bounded daemon calls."""

from dataclasses import dataclass
import io
import os
import re
import threading
import time


UART_CONSOLE = "uart0"
USB_CONSOLE = "usb-serial-jtag"
USB_DRIVER_BAUD = 115200
NATIVE_BACKEND = "pySerial 3.5 native"
INJECTED_BACKEND = "injected serial backend"
READ_CHUNK = 256
IDLE_POLL = 0.005
CALL_STATES = frozenset({"not_started", "ok", "error", "late", "stalled", "interrupted"})
LIMITS = ("session_timeout", "duration", "command_timeout", "cleanup_timeout",
          "finalize_timeout", "tail_timeout")
DECLARATION = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.:/-]{0,127}")
PORT_PATTERNS = {
    "nt": re.compile(r"COM[1-9][0-9]{0,3}", re.IGNORECASE),
    "posix": re.compile(r"/dev/(?:tty[A-Za-z0-9_.:-]+|cu\.[A-Za-z0-9_.:-]+|"
                        r"serial/(?:by-id|by-path)/[A-Za-z0-9][A-Za-z0-9_.:-]*)"),
}


class SerialError(ValueError):
    """A required host-side serial operation or declaration failed."""


class ReplyTimeout(SerialError):
    """The outstanding request was written in full, but no timely reply arrived."""

    def __init__(self, message, late=b""):
        super().__init__(message)
        self.late = late


@dataclass
class CallResult:
    """A deadline-bounded call receipt and any result observed by the owner."""

    state: str
    detail: str = ""
    value: object = None
    pending: bool = False
    cancelled: bool = False

    def receipt(self):
        return {"state": self.state, "detail": self.detail}


def bounded_call(work, deadline, cancel=None):
    """Run one call until its absolute monotonic deadline.

    A pending result means the worker may still own its resource. The caller
    must leave it alone, finalize independent evidence, and exit the process.
    """
    if time.monotonic() >= deadline:
        return CallResult("not_started", "deadline expired before admission")
    done = threading.Event()
    completed = []

    def run():
        try:
            if time.monotonic() >= deadline or (cancel is not None and cancel.is_set()):
                result = CallResult("not_started", "deadline expired before execution")
            else:
                result = CallResult("ok", value=work())
        except KeyboardInterrupt:
            result = CallResult("interrupted", "operation interrupted", cancelled=True)
        except BaseException as error:
            result = CallResult("error", f"{type(error).__name__}: {error}"[:512])
        completed.append((time.monotonic(), result))
        done.set()

    worker = threading.Thread(target=run, daemon=True)
    try:
        worker.start()
        while not done.is_set():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                if cancel is not None:
                    cancel.set()
                return CallResult("stalled", "deadline expired with a worker pending", pending=True)
            done.wait(min(remaining, 0.05))
        worker.join(timeout=0)
    except KeyboardInterrupt:
        if cancel is not None:
            cancel.set()
        returned = done.is_set()
        value = completed[0][1].value if returned else None
        return CallResult("interrupted", "interrupted by operator", value,
                          pending=not returned, cancelled=True)
    except RuntimeError as error:
        return CallResult("not_started", f"cannot start worker: {error}"[:512])
    ended, result = completed[0]
    if ended >= deadline and result.state == "ok":
        result.state = "late"
        result.detail = "completed after deadline"
    return result


def valid_port(port, system=None):
    """Check a local port name for admission or cross-platform evidence replay."""
    return (type(port) is str and len(port) <= 240 and
            any(pattern.fullmatch(port) for name, pattern in PORT_PATTERNS.items()
                if system is None or system == name))


def fixture_identifier(value):
    """Recognize a supplied board or authorization identifier."""
    return (type(value) is str and bool(DECLARATION.fullmatch(value)) and
            value.casefold() not in ("none", "unknown"))


def serial_settings(args, expected):
    """Validate the declared local fixture and limits without importing pySerial."""
    if not valid_port(args.port, os.name):
        raise SerialError("use an explicit local COM port or local /dev tty path for this OS")
    if not all(map(fixture_identifier, (args.board, args.authorization))):
        raise SerialError("board and authorization must be explicit identifiers, at most 128 characters")
    if not args.reset_silent or not args.clean_input:
        raise SerialError("require qualified reset-silent open/close and exclusive attachment at a clean input line")
    console, baud = expected.get("console"), expected.get("baud")
    baud_applies = console == UART_CONSOLE
    if not ((baud_applies and baud and 1 <= baud <= 4_000_000) or (console == USB_CONSOLE and baud == 0)):
        raise SerialError("use --console uart0 with UART baud 1..4000000 or --console usb-serial-jtag with baud 0")
    if "firmware" not in expected and "firmware_prefix" not in expected:
        raise SerialError("serial requires --firmware or --firmware-prefix")
    if args.suite not in ("passive", "campaign") or expected["target"] == "host":
        raise SerialError("serial requires a device target and the passive or campaign suite")
    limits = {name: getattr(args, name) for name in LIMITS}
    if (limits["cleanup_timeout"] + limits["finalize_timeout"] >= limits["session_timeout"] or
            limits["tail_timeout"] >= min(limits["duration"], limits["session_timeout"] -
                                           limits["cleanup_timeout"] - limits["finalize_timeout"])):
        raise SerialError("session must reserve cleanup and finalization and leave room for the active receive tail")
    return {
        "port": args.port, "board": args.board, "authorization": args.authorization,
        "reset_silent": True, "clean_input": True, "console": console, "baud_applies": baud_applies,
        "driver_baud": baud if baud_applies else USB_DRIVER_BAUD, "requested_dtr": False,
        "requested_rts": False, "backend": NATIVE_BACKEND, "limits": limits,
        "max_bytes": args.max_bytes,
    }


def native_port(**settings):
    """Load only the audited local native backend, without URL handlers."""
    try:
        import serial
    except ImportError as error:
        raise SerialError("live serial requires optional pySerial 3.5") from error
    backend = {"nt": "serial.serialwin32", "posix": "serial.serialposix"}[os.name]
    if (getattr(serial, "__version__", None) != "3.5" or
            getattr(getattr(serial, "Serial", None), "__module__", None) != backend):
        raise SerialError(f"live serial requires pySerial 3.5 with {backend}")
    if os.name == "posix":
        class SingleWriteSerial(serial.Serial):
            def write(self, data):
                # pySerial's POSIX writer can loop after a short count or EAGAIN.
                # The native descriptor is already nonblocking. Let the owner
                # retire the session on either outcome instead of retrying.
                return os.write(self.fileno(), data)

        settings["write_timeout"] = 0
        return SingleWriteSerial(**settings)
    return serial.Serial(**settings)


class SerialPort:
    """Own one port and retire it on any unbounded or framing-unsafe effect."""

    def __init__(self, settings, factory):
        self.settings = settings
        self.factory = factory
        self.port = None
        self.pending = False
        self.cancelled = False
        self.framing_uncertain = False
        self.opened = CallResult("not_started")
        self.closed = CallResult("not_started")
        self.writes = []

    @property
    def exit_reason(self):
        """Keep a pending port, or an IOBase with a failed close, alive through exit."""
        if self.pending:
            return "a serial worker is pending"
        if isinstance(self.port, io.IOBase) and self.closed.state not in ("ok", "late"):
            # IOBase destruction may call close again after a failed close.
            return "backend destruction could reenter close"
        return ""

    def _call(self, work, deadline, cancel=None):
        if self.pending:
            return CallResult("not_started", "a backend worker still owns the port")
        result = bounded_call(work, deadline, cancel)
        self.pending = result.pending
        self.cancelled |= result.cancelled
        return result

    def open(self, deadline):
        """Configure a closed port, then open once before the active deadline."""
        cancel = threading.Event()

        def attach():
            self.port = self.factory(port=None, baudrate=self.settings["driver_baud"],
                                     timeout=0, write_timeout=self.settings["limits"]["command_timeout"],
                                     xonxoff=False, rtscts=False, dsrdtr=False)
            for name in ("dtr", "rts"):
                setattr(self.port, name, False)
                if getattr(self.port, name) is not False:
                    raise SerialError(f"backend did not retain requested {name.upper()} state")
            self.port.port = self.settings["port"]
            if cancel.is_set() or time.monotonic() >= deadline:
                raise SerialError("setup ended before port open")
            self.port.open()

        self.opened = self._call(attach, deadline, cancel)
        if self.opened.state != "ok":
            raise SerialError(f"serial setup/open {self.opened.state}: {self.opened.detail}")

    def send(self, command, deadline):
        """Make one full-line write attempt and record its extent if known."""
        if self.framing_uncertain:
            raise SerialError("serial input framing is uncertain")
        line = command.encode("ascii") + b"\n"
        result = self._call(lambda: self.port.write(line), deadline)
        count = result.value
        valid = type(count) is int and 0 <= count <= len(line)
        if result.state == "ok" and (not valid or count != len(line)):
            result.state = "error"
            result.detail = f"short write ({count}/{len(line)} bytes)" if valid else "invalid write count"
        self.writes.append({"command": command, "bytes": count if valid else None, **result.receipt()})
        if result.state != "ok":
            self.framing_uncertain = result.state != "not_started"
            raise SerialError(f"serial write {result.state}: {result.detail}")

    def read(self, size, deadline, retain):
        """Retain every returned byte, even if it arrived after the deadline."""
        result = self._call(lambda: self.port.read(size), deadline)
        if type(result.value) is bytes:
            retain(result.value)
        if result.state in ("late", "not_started") and time.monotonic() >= deadline:
            late = result.value if type(result.value) is bytes else b""
            raise ReplyTimeout("serial receive deadline expired", late)
        if result.state != "ok":
            raise SerialError(f"serial read {result.state}: {result.detail}")
        if type(result.value) is not bytes or len(result.value) > size:
            raise SerialError("invalid serial read result")
        return result.value

    def close(self, deadline):
        """Close once after the last call returned, including a failed open."""
        if self.pending:
            self.closed = CallResult("not_started", "a backend worker still owns the port")
        elif self.port is not None:
            self.closed = self._call(lambda: self.port.close(), deadline)
        return self.closed
