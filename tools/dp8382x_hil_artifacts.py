"""Write and verify bounded two-file console evidence."""

from __future__ import annotations

import base64
from datetime import datetime, timedelta, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re
import stat
import tempfile
import time

if __package__:
    from .dp8382x_hil_campaign import MAX_COMMANDS, MAX_INTERVENTIONS, validate_declaration, validate_intervention
    from .dp8382x_hil_protocol import MAX_RECORD_BYTES, U64_MAX, ProtocolError
    from .dp8382x_hil_serial import (
        CALL_STATES, INJECTED_BACKEND, LIMITS, NATIVE_BACKEND, UART_CONSOLE, USB_CONSOLE, USB_DRIVER_BAUD,
        fixture_identifier, valid_port,
    )
else:
    from dp8382x_hil_campaign import MAX_COMMANDS, MAX_INTERVENTIONS, validate_declaration, validate_intervention
    from dp8382x_hil_protocol import MAX_RECORD_BYTES, U64_MAX, ProtocolError
    from dp8382x_hil_serial import (
        CALL_STATES, INJECTED_BACKEND, LIMITS, NATIVE_BACKEND, UART_CONSOLE, USB_CONSOLE, USB_DRIVER_BAUD,
        fixture_identifier, valid_port,
    )


MAX_TRANSCRIPT_BYTES = 1_048_576
MAX_MANIFEST_BYTES = 2_097_152
VERDICTS = frozenset({"PASS", "FAIL", "INCONCLUSIVE", "BLOCKED", "NOT RUN"})
RUN_NAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,95}")
HASH = re.compile(r"[0-9a-f]{64}")
REVISION = re.compile(r"[0-9a-f]{40}|[0-9a-f]{64}")
TERMINAL_CODES = frozenset({
    "OK", "INVALID_PARAM", "INVALID_CONFIG", "NOT_BOUND", "NOT_INITIALIZED", "UNSUPPORTED",
    "DEVICE_NOT_FOUND", "DEVICE_MISMATCH", "MDIO_ERROR", "MDIO_TIMEOUT", "OPERATION_TIMEOUT",
    "CONFIG_MISMATCH", "BUSY", "CANCELLED", "RESULT_NOT_AVAILABLE", "TOKEN_MISMATCH",
})
DATA_FIELDS = {"suite", "fixture", "started_utc", "ended_utc", "complete", "commands",
               "cases", "identity", "expectations", "runner", "stderr", "issues"}
ARTIFACT_FIELDS = {"schema", "scope", "run", "verdict", "transcript"}
TARGET_FIELDS = {"target", "backend", "variant", "interface", "address"}
SERIAL_SCOPE = "serial_session"
SERIAL_FIELDS = {"port", "board", "authorization", "reset_silent", "clean_input", "console",
                 "baud_applies", "driver_baud", "requested_dtr", "requested_rts", "backend",
                 "limits", "max_bytes", "open", "close", "writes", "recovery",
                 "framing_uncertain", "hard_exit", "restoration"}


class EvidenceError(ValueError):
    """Evidence could not be written, finalized, or verified."""


def soak_plan(count, duration_ms, max_bytes, declaration):
    """Bound a requested plan before allocating cases or reserving either file."""
    if (type(count) is not int or not 1 <= count <= 120_000 or
            type(duration_ms) is not int or not 1 <= duration_ms <= 120_000 or
            type(max_bytes) is not int or not 1 <= max_bytes <= MAX_TRANSCRIPT_BYTES):
        raise EvidenceError("soak requires positive count, duration (1..120000 ms) and capture limit")
    if count > 1 and declaration["case"] == "cold-boot":
        raise EvidenceError("one boot session cannot attest repeated cold boots")
    # Include the handshake, six telemetry commands per case and the fixed
    # eight-command prelude. Every command can produce admission and terminal.
    commands = 1 + count * (MAX_COMMANDS + 6)
    records = 2 * (commands + 8) + 1  # One possible same-connection nonce.
    transcript = records * MAX_RECORD_BYTES
    # Reserve base64 stderr at the combined capture ceiling. Allow 640 bytes
    # per confirmed command + serial write receipt, one escaped error, typed
    # iteration snapshots and all four bounded intervention declarations.
    manifest = 32_768 + 4 * ((max_bytes + 2) // 3) + 640 * commands + 12_288 * count
    if transcript > max_bytes or manifest > MAX_MANIFEST_BYTES:
        raise EvidenceError("soak plan exceeds its transcript or manifest byte budget")
    # Sum the per-iteration ceilings in integer milliseconds. The actual
    # primary window is shared, and never grows by this product.
    planned_ms = count * duration_ms
    if planned_ms * 1000 > U64_MAX or commands + 1 > U64_MAX:
        raise EvidenceError("soak plan exceeds the duration or request number range")
    return {"count": count, "duration_ms": duration_ms, "max_bytes": max_bytes,
            "command_limit": commands, "record_limit": records, "transcript_bytes": transcript,
            "manifest_bytes": manifest, "planned_ms": planned_ms}


def _object(value, fields):
    return type(value) is dict and value.keys() == fields


def _text(value):
    return type(value) is str and bool(value.strip())


def _list_of_text(value):
    return type(value) is list and all(_text(item) for item in value)


def _verdict(value):
    return type(value) is str and value in VERDICTS


def _utc(value):
    if not _text(value) or len(value) > 40 or "T" not in value:
        return False
    try:
        timestamp = datetime.fromisoformat(value[:-1] + "+00:00" if value.endswith("Z") else value)
    except ValueError:
        return False
    return timestamp.utcoffset() == timedelta(0)


def _validate_case(case):
    if (not _object(case, {"id", "verdict", "executed", "primary", "restoration", "message"})
            or not _text(case["id"]) or not _verdict(case["verdict"])
            or type(case["executed"]) is not bool or type(case["message"]) is not str):
        raise EvidenceError("Invalid case fields")
    restoration = case["restoration"]
    if (not _object(restoration, {"required", "verdict"}) or type(restoration["required"]) is not bool
            or not _verdict(restoration["verdict"])):
        raise EvidenceError("Invalid restoration fields")
    primary = case["primary"]
    if primary is None:
        return
    if (not _object(primary, {"code", "detail"}) or type(primary["code"]) is not str
            or primary["code"] not in TERMINAL_CODES or type(primary["detail"]) is not int
            or not -(1 << 31) <= primary["detail"] < (1 << 31)
            or (primary["code"] == "OK" and primary["detail"] != 0)):
        raise EvidenceError("Invalid terminal status")


def _stderr_size(value):
    if type(value) is not str or len(value) > 4 * ((MAX_TRANSCRIPT_BYTES + 2) // 3):
        raise EvidenceError("Invalid stderr byte limit")
    try:
        content = base64.b64decode(value, validate=True)
    except ValueError as error:
        raise EvidenceError("Invalid stderr base64") from error
    if len(content) > MAX_TRANSCRIPT_BYTES or base64.b64encode(content).decode("ascii") != value:
        raise EvidenceError("Noncanonical or oversized stderr base64")
    return len(content)


def _validate_serial(data):
    serial = data["serial"]
    if not _object(serial, SERIAL_FIELDS):
        raise EvidenceError("Invalid serial receipt fields")
    expected = data["expectations"]
    uart = serial["console"] == UART_CONSOLE
    if (serial["console"] not in (UART_CONSOLE, USB_CONSOLE) or expected.get("console") != serial["console"]
            or serial["baud_applies"] is not uart or type(serial["driver_baud"]) is not int
            or (uart and (serial["driver_baud"] != expected.get("baud") or
                          not 1 <= serial["driver_baud"] <= 4_000_000))
            or (not uart and (serial["driver_baud"] != USB_DRIVER_BAUD or expected.get("baud") != 0))
            or ("firmware" in expected) == ("firmware_prefix" in expected)
            or expected.get("target") == "host" or not valid_port(serial["port"])
            or not all(fixture_identifier(serial[key]) for key in ("board", "authorization"))
            or serial["backend"] not in (NATIVE_BACKEND, INJECTED_BACKEND)
            or serial["reset_silent"] is not True or serial["clean_input"] is not True
            or serial["requested_dtr"] is not False or serial["requested_rts"] is not False
            or data["suite"] not in ("passive", "campaign") or data["fixture"] != serial["board"] or data["stderr"] != ""
            or type(serial["max_bytes"]) is not int or not 1 <= serial["max_bytes"] <= MAX_TRANSCRIPT_BYTES
            or any(type(serial[key]) is not bool for key in ("hard_exit", "framing_uncertain"))):
        raise EvidenceError("Invalid serial fixture declarations")
    limits = serial["limits"]
    if (not _object(limits, set(LIMITS)) or
            any(type(value) not in (int, float) or not 0 < value <= 120 for value in limits.values()) or
            limits["tail_timeout"] >= min(limits["duration"], limits["session_timeout"] -
                                          limits["cleanup_timeout"] - limits["finalize_timeout"])):
        raise EvidenceError("Invalid serial limits")
    for phase in ("open", "close"):
        result = serial[phase]
        if (not _object(result, {"state", "detail"}) or type(result["state"]) is not str
                or result["state"] not in CALL_STATES or type(result["detail"]) is not str
                or (result["state"] == "stalled" and not serial["hard_exit"])):
            raise EvidenceError("Invalid serial lifecycle outcome")
    recovery = serial["recovery"]
    if (not _object(recovery, {"state", "detail", "identity"}) or
            recovery["state"] not in ("not_started", "continuous", "unproven", "failed") or
            type(recovery["detail"]) is not str or type(recovery["identity"]) is not dict or
            (recovery["state"] == "continuous" and not recovery["identity"])):
        raise EvidenceError("Invalid serial resynchronization outcome")
    writes = serial["writes"]
    campaign = data.get("campaign")
    command_limit = data["soak"]["plan"]["command_limit"] if "soak" in data else MAX_COMMANDS
    if type(writes) is not list or len(writes) > (command_limit + 1 if campaign else 11):
        raise EvidenceError("Invalid serial write attempts")
    for row in writes:
        if (not _object(row, {"command", "bytes", "state", "detail"}) or not _text(row["command"])
                or not row["command"].isascii() or not 1 <= len(row["command"]) <= 96
                or type(row["state"]) is not str or row["state"] not in CALL_STATES
                or type(row["detail"]) is not str
                or (row["bytes"] is not None and
                    (type(row["bytes"]) is not int or not 0 <= row["bytes"] <= len(row["command"]) + 1))
                or (row["state"] == "ok" and row["bytes"] != len(row["command"]) + 1)):
            raise EvidenceError("Invalid serial write receipt")
    confirmed = [row["command"] for row in writes if row["state"] == "ok"]
    restoration = "reported_by_case" if campaign else "not_requested" if data["complete"] else "unavailable"
    if (confirmed != data["commands"] or any(row["state"] != "ok" for row in writes[:-1])
            or serial["restoration"] != restoration
            or (data["complete"] and
                (serial["hard_exit"] or serial["framing_uncertain"] or len(confirmed) != len(writes)
                 or recovery["state"] != "not_started" or
                 any(serial[phase]["state"] != "ok" for phase in ("open", "close"))))):
        raise EvidenceError("Serial receipt disagrees with execution and closure")
    if campaign and campaign["declaration"]["authorization"] != serial["authorization"]:
        raise EvidenceError("Serial and campaign authorization disagree")


def _validate_soak(data):
    soak = data["soak"]
    keys = {"plan", "requested", "attempted", "completed", "remaining", "stop_reason", "telemetry",
            "elapsed_ms", "primary_window_ms", "setup", "received_records", "unconfirmed_command"}
    if not _object(soak, keys) or type(soak["plan"]) is not dict:
        raise EvidenceError("invalid soak receipt")
    claim = soak["plan"]
    plan = soak_plan(claim.get("count"), claim.get("duration_ms"), claim.get("max_bytes"),
                     data["campaign"]["declaration"])
    if (claim != plan or any(type(value) is not int for value in claim.values()) or
            any(type(soak[key]) is not int for key in ("requested", "attempted", "completed", "remaining",
                                                       "elapsed_ms", "primary_window_ms", "received_records")) or
            soak["requested"] != plan["count"] or soak["remaining"] != plan["count"] - soak["completed"] or
            not 0 <= soak["completed"] <= soak["attempted"] <= plan["count"] or
            not 0 <= soak["received_records"] <= plan["record_limit"] or soak["elapsed_ms"] < 0 or
            not 0 <= soak["primary_window_ms"] <= plan["duration_ms"] or
            (soak["stop_reason"] == "duration" and soak["elapsed_ms"] + 1 < soak["primary_window_ms"]) or
            soak["stop_reason"] not in ("count", "duration", "cancelled", "fault", "pending") or
            (soak["stop_reason"] == "count" and soak["remaining"] != 0) or
            (soak["stop_reason"] == "pending" and (data["ended_utc"] or soak["attempted"])) or
            (data["complete"] and soak["stop_reason"] not in ("count", "fault"))):
        raise EvidenceError("inconsistent soak plan, iteration counts or stop reason")
    pending = soak["unconfirmed_command"]
    if pending is not None and (not _text(pending) or len(pending) > 96 or data["complete"]):
        raise EvidenceError("invalid uncertain command receipt")
    setup = soak["setup"]
    if (type(setup) is not list or len(setup) > 8 or
            any(not _object(item, {"command", "state"}) or not _text(item["command"]) or
                item["state"] not in ("uncertain", "written", "terminal") for item in setup) or
            any(item["state"] != "terminal" for item in (setup if data["complete"] else setup[:-1]))):
        raise EvidenceError("invalid compiled setup receipt")
    observations = soak["telemetry"]
    if (type(observations) is not list or not soak["attempted"] <= len(observations) <= plan["count"] or
            any(not _object(row, {"id", "console", "transport", "health", "chip"}) or
                row["id"] != f'{data["campaign"]["declaration"]["case"]}-{i}' or
                any(row[key] is not None and type(row[key]) is not dict for key in ("console","transport","health","chip"))
                for i, row in enumerate(observations, 1))):
        raise EvidenceError("invalid soak observations")
    if "serial" in data and (not isinstance(data["serial"], dict) or setup or
                             data["serial"].get("max_bytes") != plan["max_bytes"]):
        raise EvidenceError("soak plan disagrees with serial setup")
    return plan


def _validate_data(data, fields, scope):
    if type(data) is dict and data.get("suite") == "campaign":
        fields = fields | {"campaign"}
        if "soak" in data:
            fields = fields | {"soak"}
    if not _object(data, fields):
        raise EvidenceError("Invalid evidence fields")
    if (type(data["complete"]) is not bool or not _text(data["suite"]) or not _text(data["fixture"])
            or not _utc(data["started_utc"])
            or not (_utc(data["ended_utc"]) or (data["ended_utc"] == "" and not data["complete"]))
            or not _list_of_text(data["commands"]) or not _list_of_text(data["issues"])
            or (data["complete"] and data["issues"]) or type(data["cases"]) is not list
            or any(type(data[key]) is not dict for key in ("identity", "expectations", "runner"))):
        raise EvidenceError("Invalid run metadata")
    seen = set()
    for case in data["cases"]:
        _validate_case(case)
        if case["id"] in seen:
            raise EvidenceError("Duplicate case ID")
        seen.add(case["id"])
    if data["suite"] == "campaign":
        campaign = data["campaign"]
        if (not _object(campaign, {"declaration", "interventions"}) or
                type(campaign["interventions"]) is not list):
            raise EvidenceError("Invalid campaign receipt")
        try:
            spec = validate_declaration(campaign["declaration"])
            count = 1
            command_limit = MAX_COMMANDS
            case_ids = [spec.id]
            if "soak" in data:
                plan = _validate_soak(data)
                count, command_limit = plan["count"], plan["command_limit"] + 1
                case_ids = [f"{spec.id}-{i}" for i in range(1, count + 1)]
            if (len(data["commands"]) > command_limit or
                    len(campaign["interventions"]) > MAX_INTERVENTIONS * count):
                raise EvidenceError("campaign receipt exceeds declared limits")
            for receipt in campaign["interventions"]:
                validate_intervention(receipt, campaign["declaration"])
        except ProtocolError as error:
            raise EvidenceError(str(error)) from error
        if [case["id"] for case in data["cases"]] != case_ids or data["fixture"] != campaign["declaration"]["board"]:
            raise EvidenceError("Campaign case or board receipt disagrees with its declaration")
    identity, expectations, runner = data["identity"], data["expectations"], data["runner"]
    if identity and (type(identity.get("boot")) is not int or not 0 <= identity["boot"] < (1 << 32)
                     or type(identity.get("session_req")) is not int or not 1 <= identity["session_req"] < (1 << 64)
                     or identity.get("firmware_strength") not in ("exact", "prefix", "unverified")):
        raise EvidenceError("Invalid observed identity receipt")
    for values in (identity, expectations):
        if ("address" in values and (type(values["address"]) is not int or not 0 <= values["address"] <= 31)
                or "baud" in values and (type(values["baud"]) is not int or not 0 <= values["baud"] < (1 << 32))
                or any(key in values and not _text(values[key]) for key in TARGET_FIELDS - {"address"})):
            raise EvidenceError("Invalid target identity fields")
    if runner:
        revision = runner.get("revision")
        if (type(revision) is not str or (revision != "unknown" and not REVISION.fullmatch(revision))
                or runner.get("tracked_tree") not in ("clean", "dirty", "unknown")):
            raise EvidenceError("Invalid runner provenance")
    if scope == SERIAL_SCOPE:
        _validate_serial(data)
    return _stderr_size(data["stderr"])


def aggregate(data: dict) -> str:
    """Combine validated case evidence and closure into PASS, FAIL or INCONCLUSIVE."""
    hard_exit = data.get("scope") == SERIAL_SCOPE and data["serial"]["hard_exit"]
    if hard_exit and "soak" not in data:
        return "INCONCLUSIVE"
    cases = data["cases"]
    if any(case["verdict"] == "FAIL" or case["restoration"]["verdict"] == "FAIL" for case in cases):
        return "FAIL"
    transcript = data.get("transcript")
    if (hard_exit or ("soak" in data and data["soak"]["stop_reason"] != "count") or
            not data["complete"] or not cases or not data["commands"] or not data["identity"] or not data["runner"]
            or data["issues"]
            or not TARGET_FIELDS <= data["expectations"].keys()
            or (transcript is not None and (transcript["truncated"] or transcript["length"] == 0))):
        return "INCONCLUSIVE"
    if any(case["verdict"] != "PASS" or not case["executed"] or case["primary"] is None or
           (case["restoration"]["required"] and case["restoration"]["verdict"] != "PASS") for case in cases):
        return "INCONCLUSIVE"
    return "PASS"


def _transcript_info(content, truncated):
    return {"sha256": hashlib.sha256(content).hexdigest(), "length": len(content), "truncated": truncated}


def _encoded(data):
    try:
        content = (json.dumps(data, ensure_ascii=True, allow_nan=False, indent=2) + "\n").encode("utf-8")
    except (TypeError, ValueError, RecursionError) as error:
        raise EvidenceError(f"Invalid JSON data: {error}") from error
    if len(content) > MAX_MANIFEST_BYTES:
        raise EvidenceError("Evidence JSON exceeds its byte limit")
    # The standard encoder can coerce non-string object keys and emit duplicate
    # names. Check that the bytes we publish satisfy the same decoder limits.
    try:
        if _decode(content) != data:
            raise EvidenceError("JSON encoding changed evidence values")
    except RecursionError as error:
        raise EvidenceError("Evidence JSON is too deeply nested") from error
    return content


def _bounded_read(path, limit):
    if not stat.S_ISREG(path.lstat().st_mode):
        raise EvidenceError(f"Evidence path is not a regular file: {path}")
    with path.open("rb") as stream:
        content = stream.read(limit + 1)
    if len(content) > limit:
        raise EvidenceError(f"Evidence exceeds its byte limit: {path}")
    return content


def _write_all(stream, content):
    if stream.write(content) != len(content):
        raise EvidenceError("Short evidence write")


def _sync(stream):
    stream.flush()
    os.fsync(stream.fileno())


def _publish(path, content, deadline=None):
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="wb", dir=path.parent, prefix=path.name + ".",
                                         suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            _write_all(stream, content)
            _sync(stream)
        if deadline is not None and time.monotonic() >= deadline:
            raise EvidenceError("Evidence finalization deadline expired before publication")
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def validate_destination(run, limit):
    """Check evidence path components and limits without accessing the filesystem."""
    if type(run) is not str or not RUN_NAME.fullmatch(run):
        raise EvidenceError("Invalid evidence run name")
    if type(limit) is not int or not 1 <= limit <= MAX_TRANSCRIPT_BYTES:
        raise EvidenceError("Invalid transcript byte limit")


class EvidenceFiles:
    """Own a JSON placeholder and a bounded raw stdout transcript in one directory."""

    def __init__(self, directory: Path, run: str, limit: int = MAX_TRANSCRIPT_BYTES, *, provisional=None):
        validate_destination(run, limit)
        self.path = directory / f"{run}.json"
        self.transcript_path = directory / f"{run}.txt"
        self._run = run
        self._limit = limit
        self._length = 0
        self._digest = hashlib.sha256()
        self._write_failed = False
        self._truncated = False
        self._scope = SERIAL_SCOPE if provisional is not None and "serial" in provisional else "host_protocol"
        placeholder = {
            "schema": 1, "scope": self._scope, "run": run, "verdict": "INCONCLUSIVE",
            "suite": "unknown", "fixture": "unknown", "started_utc": datetime.now(timezone.utc).isoformat(),
            "ended_utc": "", "complete": False, "commands": [],
            "cases": [{"id": "pending", "executed": False, "verdict": "NOT RUN", "primary": None,
                       "restoration": {"required": False, "verdict": "NOT RUN"}, "message": "Run not finalized"}],
            "identity": {}, "expectations": {}, "runner": {}, "stderr": "", "issues": ["Run not finalized"],
            "transcript": _transcript_info(b"", False),
        }
        if provisional is not None:
            placeholder.update(provisional)
            fields = DATA_FIELDS | ARTIFACT_FIELDS
            _validate_data(placeholder, fields | ({"serial"} if self._scope == SERIAL_SCOPE else set()), self._scope)
        reserved_json = False
        try:
            directory.mkdir(parents=True, exist_ok=True)
            with self.path.open("xb") as stream:
                reserved_json = True
                _write_all(stream, _encoded(placeholder))
                _sync(stream)
            self._stream = self.transcript_path.open("xb")
        except (OSError, EvidenceError, KeyboardInterrupt) as error:
            # Never leave our JSON paired with somebody else's pre-existing transcript.
            if reserved_json:
                try:
                    self.path.unlink()
                except OSError as cleanup:
                    if isinstance(error, KeyboardInterrupt):
                        raise error from cleanup
                    raise EvidenceError(f"Cannot reserve evidence: {error}; {cleanup}") from error
            if isinstance(error, KeyboardInterrupt):
                raise
            raise EvidenceError(f"Cannot reserve evidence: {error}") from error

    def write(self, data: bytes) -> int:
        """Store at most the remaining byte allowance and report the stored prefix length."""
        if self._stream is None or self._write_failed:
            raise EvidenceError("Transcript is closed or failed")
        if type(data) is not bytes:
            raise EvidenceError("Transcript data must be bytes")
        prefix = data[:self._limit - self._length]
        self._truncated |= len(prefix) != len(data)
        try:
            if prefix:
                _write_all(self._stream, prefix)
        except (OSError, EvidenceError) as error:
            self._write_failed = True
            raise EvidenceError(f"Cannot write transcript: {error}") from error
        self._length += len(prefix)
        self._digest.update(prefix)
        return len(prefix)

    def mark_truncated(self):
        """Record stdout lost at an upstream capture limit."""
        self._truncated = True

    @property
    def truncated(self):
        """Whether the capture omitted stdout bytes."""
        return self._truncated

    def abort(self):
        """Close the transcript without replacing the INCONCLUSIVE placeholder."""
        stream, self._stream = self._stream, None
        if stream is None:
            return
        try:
            with stream:
                _sync(stream)
        except OSError as error:
            raise EvidenceError(f"Cannot sync or close transcript: {error}") from error

    def finish(self, data: dict, deadline=None) -> dict:
        """Close and hash stdout, then atomically publish validated outcome data."""
        if self._stream is None:
            raise EvidenceError("Transcript is already closed")
        self.abort()
        if self._write_failed:
            raise EvidenceError("Transcript write failed before finalization")
        fields = DATA_FIELDS | {"serial"} if self._scope == SERIAL_SCOPE else DATA_FIELDS
        stderr_size = _validate_data(data, fields, self._scope)
        try:
            content = _bounded_read(self.transcript_path, self._limit)
            if len(content) != self._length:
                raise EvidenceError("Transcript length changed before finalization")
            if len(content) + stderr_size > self._limit:
                raise EvidenceError("Combined stdout and stderr exceed the capture limit")
            info = _transcript_info(content, self.truncated)
            if info["sha256"] != self._digest.hexdigest():
                raise EvidenceError("Transcript bytes changed before finalization")
            result = {**data, "schema": 1, "scope": self._scope, "run": self._run, "transcript": info}
            result["verdict"] = aggregate(result)
            encoded = _encoded(result)
            if deadline is None:
                _publish(self.path, encoded)
            else:
                _publish(self.path, encoded, deadline)
        except OSError as error:
            raise EvidenceError(f"Cannot finalize evidence: {error}") from error
        return result


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise EvidenceError(f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def _integer(token):
    if len(token) > 21:
        raise EvidenceError("JSON integer exceeds its numeric limit")
    return int(token)


def _bad_number(token):
    raise EvidenceError(f"Non-finite JSON number: {token}")


def _decimal(token):
    value = float(token)
    if not math.isfinite(value):
        _bad_number(token)
    return value


def _decode(content):
    return json.loads(content.decode("utf-8"), object_pairs_hook=_unique_object,
                      parse_int=_integer, parse_float=_decimal, parse_constant=_bad_number)


def verify_artifacts(path: Path) -> tuple[dict, bytes]:
    """Verify JSON schema, verdict and sibling transcript bytes before replay."""
    try:
        raw = _bounded_read(path, MAX_MANIFEST_BYTES)
        data = _decode(raw)
        scope = data.get("scope") if type(data) is dict else None
        if scope not in ("host_protocol", SERIAL_SCOPE):
            raise EvidenceError("Invalid evidence scope")
        fields = DATA_FIELDS | ARTIFACT_FIELDS | ({"serial"} if scope == SERIAL_SCOPE else set())
        stderr_size = _validate_data(data, fields, scope)
        run = data["run"]
        if (type(data["schema"]) is not int or data["schema"] != 1
                or type(run) is not str or not RUN_NAME.fullmatch(run) or path.name != f"{run}.json"
                or not _verdict(data["verdict"])):
            raise EvidenceError("Invalid evidence schema or run identity")
        info = data["transcript"]
        if (not _object(info, {"sha256", "length", "truncated"})
                or type(info["sha256"]) is not str or not HASH.fullmatch(info["sha256"])
                or type(info["length"]) is not int or not 0 <= info["length"] <= MAX_TRANSCRIPT_BYTES
                or type(info["truncated"]) is not bool):
            raise EvidenceError("Invalid transcript receipt")
        content = _bounded_read(path.with_suffix(".txt"), MAX_TRANSCRIPT_BYTES)
        if scope == SERIAL_SCOPE and len(content) > data["serial"]["max_bytes"]:
            raise EvidenceError("Transcript exceeds its declared serial capture limit")
        if "soak" in data and len(content) + stderr_size > data["soak"]["plan"]["max_bytes"]:
            raise EvidenceError("Soak output exceeds its planned capture limit")
        if len(content) + stderr_size > MAX_TRANSCRIPT_BYTES:
            raise EvidenceError("Combined stdout and stderr exceed the capture limit")
        if _transcript_info(content, info["truncated"]) != info:
            raise EvidenceError("Transcript length or SHA-256 does not match")
        if aggregate(data) != data["verdict"]:
            raise EvidenceError("Verdict disagrees with retained evidence")
        return data, content
    except EvidenceError:
        raise
    except (OSError, ValueError, RecursionError) as error:
        raise EvidenceError(f"Cannot verify evidence: {error}") from error
