"""Write and verify bounded two-file host protocol evidence."""

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


class EvidenceError(ValueError):
    """Evidence could not be written, finalized, or verified."""


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


def _validate_data(data, fields):
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
    return _stderr_size(data["stderr"])


def aggregate(data: dict) -> str:
    """Combine validated case evidence and closure into PASS, FAIL or INCONCLUSIVE."""
    cases = data["cases"]
    if any(case["verdict"] == "FAIL" or case["restoration"]["verdict"] == "FAIL" for case in cases):
        return "FAIL"
    transcript = data.get("transcript")
    if (not data["complete"] or not cases or not data["commands"] or not data["identity"] or not data["runner"]
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


def _publish(path, content):
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="wb", dir=path.parent, prefix=path.name + ".",
                                         suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            _write_all(stream, content)
            _sync(stream)
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


class EvidenceFiles:
    """Own a JSON placeholder and a bounded raw stdout transcript in one directory."""

    def __init__(self, directory: Path, run: str, limit: int = MAX_TRANSCRIPT_BYTES):
        if type(run) is not str or not RUN_NAME.fullmatch(run):
            raise EvidenceError("Invalid evidence run name")
        if type(limit) is not int or not 1 <= limit <= MAX_TRANSCRIPT_BYTES:
            raise EvidenceError("Invalid transcript byte limit")
        self.path = directory / f"{run}.json"
        self.transcript_path = directory / f"{run}.txt"
        self._run = run
        self._limit = limit
        self._length = 0
        self._digest = hashlib.sha256()
        self._write_failed = False
        self._truncated = False
        placeholder = {
            "schema": 1, "scope": "host_protocol", "run": run, "verdict": "INCONCLUSIVE",
            "suite": "unknown", "fixture": "unknown", "started_utc": datetime.now(timezone.utc).isoformat(),
            "ended_utc": "", "complete": False, "commands": [],
            "cases": [{"id": "pending", "executed": False, "verdict": "NOT RUN", "primary": None,
                       "restoration": {"required": False, "verdict": "NOT RUN"}, "message": "Run not finalized"}],
            "identity": {}, "expectations": {}, "runner": {}, "stderr": "", "issues": ["Run not finalized"],
            "transcript": _transcript_info(b"", False),
        }
        reserved_json = False
        try:
            directory.mkdir(parents=True, exist_ok=True)
            with self.path.open("xb") as stream:
                reserved_json = True
                _write_all(stream, _encoded(placeholder))
                _sync(stream)
            self._stream = self.transcript_path.open("xb")
        except (OSError, EvidenceError) as error:
            # Never leave our JSON paired with somebody else's pre-existing transcript.
            if reserved_json:
                try:
                    self.path.unlink()
                except OSError as cleanup:
                    raise EvidenceError(f"Cannot reserve evidence: {error}; {cleanup}") from error
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

    def finish(self, data: dict) -> dict:
        """Close and hash stdout, then atomically publish validated outcome data."""
        if self._stream is None:
            raise EvidenceError("Transcript is already closed")
        self.abort()
        if self._write_failed:
            raise EvidenceError("Transcript write failed before finalization")
        stderr_size = _validate_data(data, DATA_FIELDS)
        try:
            content = _bounded_read(self.transcript_path, self._limit)
            if len(content) != self._length:
                raise EvidenceError("Transcript length changed before finalization")
            if len(content) + stderr_size > self._limit:
                raise EvidenceError("Combined stdout and stderr exceed the capture limit")
            info = _transcript_info(content, self.truncated)
            if info["sha256"] != self._digest.hexdigest():
                raise EvidenceError("Transcript bytes changed before finalization")
            result = {**data, "schema": 1, "scope": "host_protocol", "run": self._run, "transcript": info}
            result["verdict"] = aggregate(result)
            _publish(self.path, _encoded(result))
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
        stderr_size = _validate_data(data, DATA_FIELDS | ARTIFACT_FIELDS)
        run = data["run"]
        if (type(data["schema"]) is not int or data["schema"] != 1 or data["scope"] != "host_protocol"
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
