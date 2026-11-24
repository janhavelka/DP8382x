"""Decode bounded schema-1 CLI bytes and verify stated firmware identity."""

from __future__ import annotations

from dataclasses import dataclass
import re


PREFIX = b"@DP8382X "
MAX_RECORD_BYTES = 1024
U32_MAX = (1 << 32) - 1
U64_MAX = (1 << 64) - 1
I32_MIN = -(1 << 31)
I32_MAX = (1 << 31) - 1

COMMON_FIELDS = frozenset("schema boot req type cmd op state primary detail cleanup cleanup_detail "
                          "time_valid time_us deadline_valid deadline_us reason".split())
STATUS_NAMES = frozenset("OK IN_PROGRESS INVALID_PARAM INVALID_CONFIG NOT_BOUND NOT_INITIALIZED UNSUPPORTED "
                         "DEVICE_NOT_FOUND DEVICE_MISMATCH MDIO_ERROR MDIO_TIMEOUT OPERATION_TIMEOUT "
                         "CONFIG_MISMATCH BUSY CANCELLED RESULT_NOT_AVAILABLE TOKEN_MISMATCH".split())
TERMINAL_STATUS = STATUS_NAMES - {"IN_PROGRESS"}
SYNTAX_REASONS = frozenset("empty command topic arity enum number nonce token_count token_length line_length "
                           "byte input_loss profile range space access".split())
_IDENTITY_FIELDS = frozenset("nonce library firmware tree sdk target backend clock variant interface address".split())
_CONSOLE_FIELDS = frozenset({"console", "baud"})
_IDENTITY_REQUIRED = frozenset({"target", "backend", "variant", "interface", "address"})
_IDENTITY_OPTIONAL = frozenset({"firmware", "firmware_prefix", "tree", "sdk", "clock", "console", "baud"})
_FIELD = re.compile(rb"([a-z][a-z0-9_]*)=([A-Za-z0-9_.:-]+)")
_COMMAND = re.compile(r"[a-z][a-z0-9-]*(?:\.[a-z][a-z0-9-]*)?", re.ASCII)
_TOKEN = re.compile(r"[A-Za-z0-9_.:-]{1,64}", re.ASCII)
_DECIMAL = re.compile(r"0|-?[1-9][0-9]*", re.ASCII)
_REVISION = re.compile(r"[0-9a-f]{7,64}", re.ASCII)
_PAGES = re.compile(r"(?:[0-9A-F]{4}|----)+", re.ASCII)
_TREES = {"clean", "dirty", "unknown"}
_INTERFACES = {"MII", "RMII"}
# SNLS647I and SNLS783A PHYIDR2: BASIC then ENHANCED. AI and AE have
# indistinguishable wire words. The firmware/board declaration binds the grade.
PHY_ID2 = {"DP83826I": (0xA111, 0xA131), "DP83826E": (0xA111, 0xA131),
           "DP83826AI": (0xA114, 0xA134), "DP83826AE": (0xA114, 0xA134)}
VARIANTS = frozenset(PHY_ID2)


class ProtocolError(ValueError):
    """A stream, payload, or identity cannot supply valid protocol evidence."""


def integer(value: str, lo: int, hi: int) -> int:
    """Read a canonical decimal token in the inclusive range, before arithmetic."""
    if (type(value) is not str or len(value) > max(len(str(lo)), len(str(hi))) or
            _DECIMAL.fullmatch(value) is None):
        raise ProtocolError("malformed decimal integer")
    result = int(value)
    if not lo <= result <= hi:
        raise ProtocolError("decimal integer out of range")
    return result


@dataclass(frozen=True, slots=True)
class Record:
    """A decoded record, with original protocol strings and typed correlation fields."""

    fields: dict[str, str]

    @property
    def boot(self) -> int:
        return integer(self.fields["boot"], 0, U32_MAX)

    @property
    def req(self) -> int:
        return integer(self.fields["req"], 1, U64_MAX)

    @property
    def op(self) -> int:
        return integer(self.fields["op"], 0, U64_MAX)

    @property
    def cmd(self) -> str:
        return self.fields["cmd"]

    @property
    def state(self) -> str:
        return self.fields["state"]

    @property
    def primary(self) -> str:
        return self.fields["primary"]


def _choice(value: str, choices: set[str] | frozenset[str], field: str) -> str:
    if type(value) is not str or value not in choices:
        raise ProtocolError(f"invalid {field}")
    return value


def _token(value: str, field: str) -> str:
    if type(value) is not str or _TOKEN.fullmatch(value) is None:
        raise ProtocolError(f"invalid identity {field} token")
    return value


def _numbers(fields: dict[str, str], names: str, lo: int, hi: int) -> None:
    for name in names.split():
        try:
            integer(fields[name], lo, hi)
        except ProtocolError as error:
            raise ProtocolError(f"{name}: {error}") from error


def _timestamp(fields: dict[str, str], flag: str, value: str) -> None:
    _numbers(fields, flag, 0, 1)
    _numbers(fields, value, 0, U64_MAX)
    if fields[flag] == "0" and fields[value] != "0":
        raise ProtocolError(f"{value} present without {flag}")


def _keys(fields: dict[str, str], payload: frozenset[str] = frozenset()) -> None:
    missing = (COMMON_FIELDS | payload) - fields.keys()
    extra = fields.keys() - (COMMON_FIELDS | payload)
    if missing or extra:
        raise ProtocolError(f"wrong fields for {fields.get('cmd', 'record')}: "
                            f"missing={sorted(missing)}, extra={sorted(extra)}")


def _wire_identity(fields: dict[str, str]) -> None:
    has_console = bool(fields.keys() & _CONSOLE_FIELDS)
    _keys(fields, _IDENTITY_FIELDS | _CONSOLE_FIELDS if has_console else _IDENTITY_FIELDS)
    for name in ("nonce", "firmware", "sdk", "target", "backend", "clock"):
        _token(fields[name], name)
    version = fields["library"].split(".")
    if len(version) != 3:
        raise ProtocolError("invalid library version")
    for part in version:
        integer(part, 0, 65535)
    _choice(fields["tree"], _TREES, "firmware tree")
    _choice(fields["variant"], VARIANTS | {"unknown"}, "PHY variant")
    _choice(fields["interface"], _INTERFACES, "MAC interface")
    _numbers(fields, "address", 0, 31)
    if has_console:
        _token(fields["console"], "console")
        _numbers(fields, "baud", 0, U32_MAX)
    if fields["firmware"] == "unknown" and fields["tree"] != "unknown":
        raise ProtocolError("unknown firmware requires unknown tree evidence")
    if fields["cmd"] != "session" and fields["nonce"] != "none":
        raise ProtocolError("nonce outside session handshake")


def validate_expected_identity(expected: dict[str, object]) -> None:
    """Check operator identity expectations before opening a fixture or serial port."""
    if (type(expected) is not dict or not _IDENTITY_REQUIRED.issubset(expected) or
            expected.keys() - (_IDENTITY_REQUIRED | _IDENTITY_OPTIONAL)):
        raise ProtocolError("expected identity requires target, backend, variant, interface and address")
    for name in ("target", "backend"):
        if _token(expected[name], name) == "unknown":
            raise ProtocolError(f"expected {name} must be known")
    _choice(expected["variant"], VARIANTS, "expected PHY variant")
    _choice(expected["interface"], _INTERFACES, "expected MAC interface")
    address = expected["address"]
    if type(address) is not int or not 0 <= address <= 31:
        raise ProtocolError("expected address must be an integer in 0..31")
    if "baud" in expected:
        baud = expected["baud"]
        if type(baud) is not int or not 0 <= baud <= U32_MAX:
            raise ProtocolError("expected baud must be a uint32 integer")
    if "tree" in expected:
        _choice(expected["tree"], _TREES, "expected firmware tree")
    for name in ("sdk", "clock", "console"):
        if name in expected:
            _token(expected[name], name)
    firmware, prefix = expected.get("firmware"), expected.get("firmware_prefix")
    if "firmware" in expected and "firmware_prefix" in expected:
        raise ProtocolError("choose an exact firmware revision or a prefix")
    if "firmware" in expected and (type(firmware) is not str or len(firmware) not in (40, 64) or
                                   _REVISION.fullmatch(firmware) is None):
        raise ProtocolError("expected exact firmware must be a 40- or 64-digit lowercase Git revision")
    if "firmware_prefix" in expected and (type(prefix) is not str or _REVISION.fullmatch(prefix) is None):
        raise ProtocolError("expected firmware prefix must contain 7..64 lowercase hex digits")


def observed_identity(record: Record) -> dict[str, object]:
    """Copy the identity stated on the wire without assigning verification strength."""
    fields = record.fields
    if (fields.get("cmd") not in {"session", "version", "local.board"} or
            fields.get("primary") != "OK" or fields.get("state") != "terminal"):
        raise ProtocolError("identity requires a successful identity record")
    _wire_identity(fields)
    keys = fields.keys() - COMMON_FIELDS - {"nonce"}
    observed: dict[str, object] = {key: fields[key] for key in sorted(keys)}
    observed["address"] = integer(fields["address"], 0, 31)
    if "baud" in fields:
        observed["baud"] = integer(fields["baud"], 0, U32_MAX)
    observed["firmware_strength"] = "unverified"
    return observed


def identity(record: Record, expected: dict[str, object]) -> dict[str, object]:
    """Return verified observed identity, with explicit exact, prefix, or unverified strength."""
    validate_expected_identity(expected)
    observed = observed_identity(record)
    for name in ("target", "backend", "variant", "interface", "address", "tree", "sdk", "clock", "console", "baud"):
        value = expected.get(name)
        if value is not None and observed.get(name) != value:
            raise ProtocolError(f"firmware identity mismatch: {name}")

    firmware = record.fields["firmware"]
    exact, prefix = expected.get("firmware"), expected.get("firmware_prefix")
    strength = "unverified"
    if exact is not None:
        if firmware != exact:
            raise ProtocolError("firmware identity mismatch: exact revision")
        strength = "exact"
    elif prefix is not None:
        if _REVISION.fullmatch(firmware) is None or not firmware.startswith(prefix):
            raise ProtocolError("firmware identity mismatch: prefix is not proved by the observed revision")
        strength = "prefix"
        observed["firmware_prefix"] = prefix
    observed["firmware_strength"] = strength
    return observed


_HEALTH = frozenset("health initialized successes failures consecutive last_error last_detail last_ok_valid "
                    "last_ok_us last_error_valid last_error_us".split())
_TRANSPORT = frozenset("batches attempted completed stopped clock_invalid controller_reusable".split())
_OPERATION = frozenset("slot token slot_status slot_detail cleanup_owed temporary slot_deadline_us".split())
_CONFIG_RESULT = frozenset("core_status core_detail sdk_error mutation_completed mutation_possible cleanup_owed "
                          "temporary an_complete np_requested np_complete np_owed np_refused np_base_event "
                          "np_base_valid np_base np_missing np_tx np_rx np_duplicates np_words".split())
_LINK = frozenset("valid stale up resolved speed_mbps duplex negotiation pause_valid pause_rx pause_tx "
                   "observed_valid observed_us".split())
_SETTINGS = frozenset("trust desired_valid desired_mode desired_modes desired_speed_mbps desired_duplex "
                       "observed_valid observed_stale observed_mode observed_modes observed_speed_mbps "
                       "observed_duplex".split())
_READ = frozenset("address devad value".split())
_CAPS = frozenset("valid modes interfaces copper fiber gigabit".split())
_SIMPLE = {"help": frozenset(), "unknown": frozenset(), "local.color": frozenset({"color"}),
           "local.quiet": frozenset({"quiet"}), "cached.health": _HEALTH, "cached.transport": _TRANSPORT,
           "cached.operation": _OPERATION, "owner.status": _OPERATION, "cached.link": _LINK,
           "cached.settings": _SETTINGS,
           "registers.read": _READ, "capabilities": _CAPS}


def _validate_payload(record: Record) -> None:
    fields, cmd = record.fields, record.cmd
    if fields["deadline_valid"] == "1" and (
            cmd.startswith("cached.") or cmd in ("session", "version", "local.board", "owner.status")):
        raise ProtocolError("passive record carries a command deadline")
    if cmd in ("session", "version", "local.board"):
        if record.state != "terminal" or record.op != 0:
            raise ProtocolError("identity query cannot carry an operation token")
        if record.primary == "OK":
            _wire_identity(fields)
        else:
            _keys(fields)
        return
    if cmd in ("configuration.verify", "configuration.apply"):
        if record.state != "terminal" or record.op == 0:
            _keys(fields)
            return
        _keys(fields, _CONFIG_RESULT)
        _choice(fields["core_status"], TERMINAL_STATUS, "core status")
        _numbers(fields, "core_detail sdk_error", I32_MIN, I32_MAX)
        _numbers(fields, "mutation_completed mutation_possible cleanup_owed temporary an_complete np_requested "
                         "np_complete np_owed np_refused np_base_event np_base_valid np_missing", 0, 1)
        _numbers(fields, "np_base", 0, 65535)
        _numbers(fields, "np_tx", 0, 255)
        _numbers(fields, "np_rx", 0, 16)
        _numbers(fields, "np_duplicates", 0, U32_MAX)
        count = int(fields["np_rx"])
        words = fields["np_words"]
        if (count == 0 and words != "none") or (count != 0 and (len(words) != 4 * count or
                                                               _PAGES.fullmatch(words) is None)):
            raise ProtocolError("next-page words do not match the retained count")
        return
    if cmd not in _SIMPLE:
        return  # Other command-specific contracts belong to their suite assertions.
    if record.state != "terminal" or record.op != 0:
        raise ProtocolError("terminal local, cached or read command cannot carry an operation token")
    _keys(fields, _SIMPLE[cmd] if record.primary == "OK" else frozenset())
    if record.primary != "OK":
        return
    if cmd == "cached.health":
        _choice(fields["health"], {"UNINIT", "READY", "DEGRADED", "OFFLINE"}, "health")
        _choice(fields["last_error"], TERMINAL_STATUS, "last health error")
        _numbers(fields, "successes failures consecutive", 0, U32_MAX)
        _numbers(fields, "initialized", 0, 1)
        health = fields["health"]
        initialized = fields["initialized"] == "1"
        if initialized != (health != "UNINIT") or (
                initialized and (health == "READY") != (fields["consecutive"] == "0")):
            raise ProtocolError("transport health conflicts with initialization or failure streak")
        _numbers(fields, "last_detail", I32_MIN, I32_MAX)
        _timestamp(fields, "last_ok_valid", "last_ok_us")
        _timestamp(fields, "last_error_valid", "last_error_us")
    elif cmd == "cached.transport":
        _numbers(fields, "batches attempted completed", 0, U32_MAX)
        _numbers(fields, "stopped clock_invalid controller_reusable", 0, 1)
        if int(fields["completed"]) > int(fields["attempted"]):
            raise ProtocolError("completed MDIO frames exceed attempts")
    elif cmd in ("cached.operation", "owner.status"):
        slot = _choice(fields["slot"], {"idle", "running", "result_pending"}, "operation slot")
        status = _choice(fields["slot_status"], STATUS_NAMES, "slot status")
        _numbers(fields, "token slot_deadline_us", 0, U64_MAX)
        _numbers(fields, "slot_detail", I32_MIN, I32_MAX)
        _numbers(fields, "cleanup_owed temporary", 0, 1)
        occupied = slot != "idle"
        if (occupied != (fields["token"] != "0") or occupied != (fields["slot_deadline_us"] != "0") or
                (slot == "running") != (status == "IN_PROGRESS") or
                (slot == "idle" and status != "RESULT_NOT_AVAILABLE")):
            raise ProtocolError("operation slot evidence conflicts with its state")
    elif cmd == "cached.link":
        _numbers(fields, "valid stale up resolved pause_valid pause_rx pause_tx", 0, 1)
        _timestamp(fields, "observed_valid", "observed_us")
        _choice(fields["negotiation"], {"disabled", "in_progress", "complete", "unknown"}, "negotiation")
        _choice(fields["duplex"], {"half", "full", "unknown"}, "duplex")
        _choice(fields["speed_mbps"], {"0", "10", "100"}, "link speed")
    elif cmd == "cached.settings":
        _choice(fields["trust"], {"UNKNOWN", "DIRTY", "VERIFIED"}, "configuration trust")
        _numbers(fields, "desired_valid observed_valid observed_stale", 0, 1)
        _numbers(fields, "desired_modes observed_modes", 0, 15)
        for scope in ("desired", "observed"):
            mode = _choice(fields[f"{scope}_mode"], {"an", "forced", "unknown"}, "settings mode")
            speed = _choice(fields[f"{scope}_speed_mbps"], {"0", "10", "100"}, "settings speed")
            duplex = _choice(fields[f"{scope}_duplex"], {"half", "full", "unknown"}, "settings duplex")
            if fields[f"{scope}_valid"] == "0":
                if (mode, fields[f"{scope}_modes"], speed, duplex) != ("unknown", "0", "0", "unknown"):
                    raise ProtocolError("invalid settings snapshot carries usable fields")
            elif mode == "unknown":
                raise ProtocolError("valid settings snapshot has unknown mode")
    elif cmd == "registers.read":
        _numbers(fields, "address value", 0, 65535)
        _numbers(fields, "devad", 0, 31)
    elif cmd == "capabilities":
        _numbers(fields, "valid copper fiber gigabit", 0, 1)
        _numbers(fields, "modes", 0, 15)
        _numbers(fields, "interfaces", 0, 7)
    elif cmd in ("local.color", "local.quiet"):
        _numbers(fields, cmd.split(".")[1], 0, 1)


def _decode(line: bytearray) -> Record:
    if not line.endswith(b"\r\n"):
        raise ProtocolError("record is not CRLF-terminated")
    fields: dict[str, str] = {}
    for chunk in line[len(PREFIX):-2].split(b" "):
        match = _FIELD.fullmatch(chunk)
        if match is None:
            raise ProtocolError("malformed machine field")
        name, value = (part.decode("ascii") for part in match.groups())
        if name in fields:
            raise ProtocolError(f"duplicate record field: {name}")
        fields[name] = value
    if not COMMON_FIELDS.issubset(fields):
        raise ProtocolError("record is missing common fields")
    if fields["schema"] != "1":
        raise ProtocolError("unsupported schema")
    _numbers(fields, "boot", 0, U32_MAX)
    _numbers(fields, "req", 1, U64_MAX)
    _numbers(fields, "op", 0, U64_MAX)
    _numbers(fields, "detail cleanup_detail", I32_MIN, I32_MAX)
    if len(fields["cmd"]) > 64 or _COMMAND.fullmatch(fields["cmd"]) is None:
        raise ProtocolError("invalid command name")
    primary = _choice(fields["primary"], STATUS_NAMES, "primary status")
    cleanup = _choice(fields["cleanup"], TERMINAL_STATUS | {"none"}, "cleanup status")
    if primary in {"OK", "IN_PROGRESS"} and fields["detail"] != "0":
        raise ProtocolError("successful or admitted primary status requires zero detail")
    if cleanup in {"none", "OK"} and fields["cleanup_detail"] != "0":
        raise ProtocolError("absent or successful cleanup requires zero detail")
    _timestamp(fields, "time_valid", "time_us")
    _timestamp(fields, "deadline_valid", "deadline_us")
    if fields["deadline_valid"] == "1" and fields["deadline_us"] == "0":
        raise ProtocolError("deadline must be positive when valid")
    reason = _choice(fields["reason"], SYNTAX_REASONS | {"none", "backend", "callback", "format"}, "reason")
    if reason in SYNTAX_REASONS and (primary != "INVALID_PARAM" or fields["detail"] != "0"):
        raise ProtocolError("syntax reason conflicts with status or detail")
    if ((reason in {"backend", "callback"} and primary != "INVALID_CONFIG") or
            (reason == "format" and primary in {"OK", "IN_PROGRESS"})):
        raise ProtocolError("failure reason conflicts with status")

    kind, state, command = fields["type"], fields["state"], fields["cmd"]
    if state == "admitted":
        if (kind != "result" or primary != "IN_PROGRESS" or cleanup != "none" or
                fields["op"] == "0" or fields["deadline_valid"] != "1"):
            raise ProtocolError("invalid operation admission")
    elif state == "progress":
        if kind != "sample" or primary == "IN_PROGRESS" or fields["op"] != "0":
            raise ProtocolError("invalid progress record")
    elif state == "terminal":
        session = command == "session" and primary == "OK"
        if kind != ("session" if session else "result") or primary == "IN_PROGRESS":
            raise ProtocolError("invalid terminal record")
        if session and (fields["op"] != "0" or cleanup != "none"):
            raise ProtocolError("session handshake carries an operation or cleanup")
    else:
        raise ProtocolError("invalid record state")
    if command == "unknown" and primary not in {"INVALID_PARAM", "INVALID_CONFIG"}:
        raise ProtocolError("unknown command cannot succeed")

    record = Record(fields)
    _validate_payload(record)
    return record


class Decoder:
    """Incremental byte decoder with a stream budget and one bounded candidate buffer.

    An undecodable candidate permanently invalidates this decoder. Human lines
    are skipped as bytes, even when they are oversized or contain invalid UTF-8.
    The byte budget still includes all received noise. The caller owns records
    returned from each chunk and the separate request/operation correlation.
    """

    def __init__(self, max_bytes: int = 1048576):
        if type(max_bytes) is not int or max_bytes <= 0:
            raise ProtocolError("decoder byte budget must be a positive integer")
        self._max_bytes = max_bytes
        self._total = 0
        self._line = bytearray()
        self._noise = False
        self._closed = False

    @property
    def has_partial_record(self) -> bool:
        """Whether bytes received since the last LF could form a machine record."""
        return bool(self._line)

    def feed(self, chunk: bytes) -> list[Record]:
        """Read raw bytes and return complete records in arrival order."""
        if self._closed:
            raise ProtocolError("decoder already closed")
        if type(chunk) is not bytes or len(chunk) > self._max_bytes - self._total:
            self._closed = True
            raise ProtocolError("invalid byte chunk or stream byte budget exceeded")
        self._total += len(chunk)
        records = []
        try:
            for byte in chunk:
                if self._noise:
                    self._noise = byte != 10
                    continue
                size = len(self._line)
                if size < len(PREFIX) and byte != PREFIX[size]:
                    if size == len(PREFIX) - 1:
                        raise ProtocolError("machine record marker requires one space")
                    self._line.clear()
                    self._noise = byte != 10
                    continue
                if size == MAX_RECORD_BYTES - 1 and byte != 10:
                    raise ProtocolError("machine record exceeds 1024 bytes")
                self._line.append(byte)
                if byte == 10:
                    records.append(_decode(self._line))
                    self._line.clear()
        except ProtocolError:
            self._closed = True
            raise
        return records

    def finish(self) -> None:
        """Close the stream and reject a partially received machine candidate."""
        if self._closed:
            raise ProtocolError("decoder already closed")
        self._closed = True
        if self.has_partial_record:
            raise ProtocolError("unterminated machine record or prefix")
