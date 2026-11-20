#!/usr/bin/env python3
"""Verify DP8382x console protocol and preserve host-only evidence."""

import argparse
import base64
from dataclasses import dataclass, field
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import secrets
import subprocess
import sys

from dp8382x_hil_artifacts import MAX_TRANSCRIPT_BYTES, EvidenceError, EvidenceFiles, verify_artifacts
from dp8382x_hil_fixture import FixtureError, FixtureProcess
from dp8382x_hil_protocol import (
    COMMON_FIELDS, TERMINAL_STATUS, U32_MAX, U64_MAX, Decoder, ProtocolError, Record,
    identity, integer, observed_identity, validate_expected_identity,
)


TOKEN = re.compile(r"[A-Za-z0-9_.:-]{1,64}\Z")
REVISION = re.compile(r"(?:[0-9a-f]{40}|[0-9a-f]{64})\Z")
TARGET_FIELDS = ("target", "backend", "variant", "interface", "address")
OPTIONAL_FIELDS = ("firmware", "firmware_prefix", "tree", "sdk", "clock", "console", "baud")


@dataclass(frozen=True)
class Command:
    """Known input line and its allowed response form."""

    text: str
    form: str = "terminal"

    @property
    def name(self):
        """Return the canonical schema command name used by this plan."""
        return ".".join(self.text.split()[:2]) if not self.text.startswith("session ") else "session"


PASSIVE = tuple(map(Command, (
    "cached transport", "cached health", "cached operation", "cached settings",
    "registers write c22 24 65536",
    "cached settings", "cached operation", "cached health", "cached transport",
)))
SUITES = {
    "passive": PASSIVE,
    "fixture-operations": PASSIVE + (
        Command("connection initialize none", "operation"),
        Command("diagnostics selftest", "samples"), Command("cached operation"),
    ),
}


@dataclass
class Response:
    """Evidence attached to one command, including any admission and sample prefix."""

    terminal: Record
    admission: Record | None
    samples: list[Record] = field(default_factory=list)


def payload(record):
    """Return command fields without the common record envelope."""
    return {key: value for key, value in record.fields.items() if key not in COMMON_FIELDS}


def check_payload(record):
    """Check typed operation and sequencer data consumed by the built-in plans."""
    fields = payload(record)
    if record.cmd == "registers.write" and record.primary == "INVALID_PARAM":
        if fields:
            raise ProtocolError("a syntactic write rejection has unexpected effect evidence")
    elif record.cmd == "connection.initialize":
        if record.state == "admitted" or record.op == 0:
            if fields:
                raise ProtocolError("initialization admission has unexpected fields")
            return
        flags = ("reset_required reset_accepted reset_complete persistent_restored mutation_completed "
                 "mutation_possible cleanup_owed temporary clock_invalid link_valid sor1_valid sor2_valid straps_stale")
        checks = {name: ("0", "1") for name in flags.split()}
        checks.update({name: ("none", "pcs", "hard", "restart", "unknown") for name in
                       ("requested_reset", "effective_reset")})
        checks.update({"core_status": tuple(TERMINAL_STATUS), "core_detail": (-2**31, 2**31-1),
                       "sor1": (0, 65535), "sor2": (0, 65535)})
        exact_fields(fields, checks)
    elif record.cmd == "diagnostics.selftest":
        if record.state == "progress":
            exact_fields(fields, {"check": ("identity", "reset_status"),
                                  "id1_valid": ("0", "1"), "id2_valid": ("0", "1"),
                                  "id1": (0, 65535), "id2": (0, 65535), "value": (0, 65535),
                                  "attempted": (0, 2)})
        elif record.primary == "OK" or fields:
            exact_fields(fields, {"rounds": (0, 2), "requested": (2, 2), "attempted": (0, 3),
                                  "attempted_known": ("0", "1"), "managed": ("0",)})


def exact_fields(fields, checks):
    """Require exactly the supplied field names and check every corresponding value."""
    if fields.keys() != checks.keys():
        raise ProtocolError("missing or unexpected command fields")
    for name, allowed in checks.items():
        value = fields[name]
        if isinstance(allowed[0], int):
            integer(value, *allowed)
        elif value not in allowed:
            raise ProtocolError(f"invalid {name}: {value}")


def plan(suite, nonce):
    """Return the known suite preceded by its correlated session command."""
    if suite not in SUITES or not isinstance(nonce, str) or not TOKEN.fullmatch(nonce):
        raise ProtocolError("unknown suite or invalid session nonce")
    return (Command("session " + nonce),) + SUITES[suite]


class Session:
    """One boot and one outstanding request; examine every record through closure."""

    def __init__(self, commands, nonce, expected):
        self.commands = commands
        self.nonce = nonce
        self.expected = expected
        self.issued = 0
        self.results = []
        self.identity = {}
        self.admission = None
        self.samples = []
        self.last_time = None
        self.clock_unavailable = False
        self.used_tokens = set()

    def issue(self):
        """Mark the next full command write. Never advance an outstanding request."""
        if self.issued != len(self.results) or self.issued == len(self.commands):
            raise ProtocolError("command sent while another request is outstanding")
        self.issued += 1

    def accept(self, record):
        """Accept one decoded record, or reject it before changing case evidence."""
        check_payload(record)
        index = len(self.results)
        if not self.identity:
            # Well-formed prior traffic cannot supply the nonce for this run.
            if record.cmd != "session" or record.fields.get("nonce") != self.nonce:
                return
            if self.issued == 0:
                raise ProtocolError("session reply preceded the command")
            try:
                observed = identity(record, self.expected)
            except ProtocolError:
                self.identity = dict(observed_identity(record), boot=record.boot, session_req=record.req)
                raise
            if record.req > U64_MAX - (len(self.commands) - 1):
                raise ProtocolError("request counter cannot accommodate the plan without rollover")
            self.identity = dict(observed, boot=record.boot, session_req=record.req)
        if index >= self.issued:
            raise ProtocolError("unsolicited or duplicate record after the last terminal")
        if record.boot != self.identity["boot"] or record.req != self.identity["session_req"] + index:
            raise ProtocolError("reboot, stale request, duplicate or out-of-order record")
        command = self.commands[index]
        if record.cmd != command.name:
            raise ProtocolError(f"expected {command.name}, received {record.cmd}")
        if record.fields["time_valid"] == "1":
            timestamp = int(record.fields["time_us"])
            if self.clock_unavailable:
                raise ProtocolError("device record clock became valid again in the same boot")
            if self.last_time is not None and timestamp < self.last_time:
                raise ProtocolError("device record clock regressed")
            self.last_time = timestamp
        else:
            self.clock_unavailable = True
        if record.state == "admitted":
            if command.form != "operation" or self.admission or record.op in self.used_tokens:
                raise ProtocolError("unplanned, duplicate or reused operation admission")
            if (record.fields["deadline_valid"] != "1" or record.fields["time_valid"] != "1" or
                    int(record.fields["time_us"]) >= int(record.fields["deadline_us"])):
                raise ProtocolError("operation admission has no usable deadline")
            self.used_tokens.add(record.op)
            self.admission = record
        elif record.state == "progress":
            if command.form != "samples" or len(self.samples) >= 2 or record.op != 0:
                raise ProtocolError("unexpected or oversized sample sequence")
            if record.fields["deadline_valid"] != "1" or (record.primary == "OK" and
                                                          record.fields["time_valid"] != "1"):
                raise ProtocolError("sample has no usable timing evidence")
            self.samples.append(record)
        else:
            source = self.admission or (self.samples[0] if self.samples else None)
            if record.op != (self.admission.op if self.admission else 0):
                raise ProtocolError("terminal operation token differs from admission")
            if command.form == "operation" and not self.admission and record.primary == "OK":
                raise ProtocolError("successful operation has no admission")
            if source and (source.fields["deadline_valid"], source.fields["deadline_us"]) != (
                    record.fields["deadline_valid"], record.fields["deadline_us"]):
                raise ProtocolError("deadline changed after admission or sampling")
            if any(sample.fields["deadline_us"] != record.fields["deadline_us"] for sample in self.samples):
                raise ProtocolError("sample deadlines differ")
            self.results.append(Response(record, self.admission, self.samples))
            self.admission = None
            self.samples = []

    def finish(self):
        """Require every planned terminal after EOF or a completed closure."""
        if len(self.results) != len(self.commands):
            raise ProtocolError("missing session reply or terminal evidence")


def clean(record):
    """Check a successful command with no independent cleanup failure."""
    return (record.primary, record.fields["detail"], record.fields["cleanup"],
            record.fields["reason"]) == ("OK", "0", "none", "none")


def case(name, record=None, passed=None, message="missing terminal evidence", restore=None):
    """Keep device status separate from the assertion and restoration verdicts."""
    return {"id": name, "executed": record is not None,
            "verdict": {True: "PASS", False: "FAIL", None: "INCONCLUSIVE"}[passed] if record else "NOT RUN",
            "primary": {"code": record.primary, "detail": int(record.fields["detail"])} if record else None,
            "restoration": {"required": restore is not None, "verdict": restore or "NOT RUN"},
            "message": message}


def cases_for(suite, results, pending_samples=()):
    """Evaluate only the assertions for the known plan and retain the failed primary."""
    rows = [result.terminal for result in results]
    baseline = rows[1:5]
    failed = next((row for row in baseline if not clean(row)), None)
    if failed:
        cases = [case("passive", failed, False, f"{failed.cmd} did not complete cleanly")]
    elif len(baseline) == 4:
        health = rows[2]
        transport = payload(rows[1])
        ready = (health.fields["health"] == "READY" and health.fields["initialized"] == "1" and
                 transport["controller_reusable"] == "1" and
                 transport["clock_invalid"] == "0" and transport["stopped"] == "0" and
                 payload(rows[3])["slot"] == "idle" and payload(rows[3])["cleanup_owed"] == "0" and
                 payload(rows[3])["temporary"] == "0")
        cases = [case("passive", health, ready, "current cached owner and reusable controller required")]
    else:
        cases = [case("passive", baseline[-1] if baseline else None)]
    if len(rows) >= 6:
        rejection = rows[5]
        exact = (rejection.primary, rejection.fields["detail"], rejection.fields["reason"],
                 rejection.fields["cleanup"], rejection.fields["deadline_valid"]) == (
                    "INVALID_PARAM", "0", "range", "none", "0")
        after = rows[6:10]
        changed = next((right for left, right in zip(reversed(baseline), after)
                        if not clean(right) or payload(left) != payload(right)), None)
        counters = payload(baseline[0])
        unsaturated = clean(baseline[0]) and all(int(counters[key]) < U32_MAX
                                                for key in ("batches", "attempted", "completed"))
        valid = exact and changed is None and unsaturated and cases[0]["verdict"] == "PASS"
        message = "exact range rejection requires unchanged state, slot and unsaturated transport totals"
        if changed:
            detail = changed.fields["detail"]
            message = f"post-rejection {changed.cmd} changed or failed ({changed.primary}/{detail})"
        passed = False
        if valid:
            passed = True if len(after) == 4 else None
        cases.append(case("rejected-write", rejection, passed, message))
    else:
        cases.append(case("rejected-write"))
    if suite == "fixture-operations":
        if len(rows) >= 11:
            record = rows[10]
            fields = record.fields
            restored = fields.get("persistent_restored") == "1" and fields.get("cleanup_owed") == "0"
            passed = clean(record) and restored and all(fields.get(key) == value for key, value in {
                "core_status": "OK", "core_detail": "0", "requested_reset": "none", "effective_reset": "none",
                "reset_required": "0", "reset_accepted": "0", "reset_complete": "0", "temporary": "0",
                "clock_invalid": "0", "link_valid": "0", "sor1_valid": "1", "sor2_valid": "1", "straps_stale": "0",
            }.items())
            restore = "PASS" if restored else "NOT RUN"
            if fields["cleanup"] not in ("none", "OK"):
                restore = "FAIL"
            cases.append(case("initialize", record, passed, "bounded initialization and persistent replay", restore))
        else:
            cases.append(case("initialize"))
        if len(rows) >= 12:
            record = rows[11]
            samples = results[11].samples
            passed = (clean(record) and len(samples) == 2 and all(map(clean, samples)) and
                      payload(record) == {"rounds": "2", "requested": "2", "attempted": "3",
                                          "attempted_known": "1", "managed": "0"} and
                      payload(samples[0]) in (
                          {"check": "identity", "id1_valid": "1", "id1": "8192", "id2_valid": "1",
                           "id2": model, "value": "0", "attempted": "2"}
                          for model in ("41233", "41265")) and
                      samples[1].fields["check"] == "reset_status" and
                      samples[1].fields["id1_valid"] == samples[1].fields["id2_valid"] == "0" and
                      samples[1].fields["attempted"] == "1" and not int(samples[1].fields["value"]) & 0xC000)
            cases.append(case("selftest", record, passed, "two passive samples and three audited MDIO frames"))
        else:
            failed_sample = next((sample for sample in pending_samples if not clean(sample)), None)
            if failed_sample:
                cases.append(case("selftest", failed_sample, False, "selftest sample failed before terminal delivery"))
            else:
                cases.append(case("selftest"))
        if len(rows) >= 13:
            slot = rows[12]
            passed = clean(slot) and slot.fields["slot"] == "idle" and slot.fields["cleanup_owed"] == "0" and slot.fields["temporary"] == "0"
            cases.append(case("idle", slot, passed, "no occupied slot or unresolved temporary effects"))
        else:
            cases.append(case("idle"))
    return cases


def utc_now():
    """Return host UTC evidence, independent of the firmware's monotonic clock."""
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")


def runner_identity():
    """Record Git provenance for this runner checkout, or unknown for an archive."""
    root = Path(__file__).resolve().parents[1]
    unknown = {"revision": "unknown", "tracked_tree": "unknown"}
    if not (root / ".git").exists():
        return unknown
    try:
        git = ["git", "-C", str(root)]
        head = subprocess.run(git + ["rev-parse", "HEAD"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                              timeout=2, check=True, text=True).stdout.strip()
        diff = subprocess.run(git + ["diff-index", "--quiet", "HEAD", "--"], stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL, timeout=2, check=False)
    except (OSError, subprocess.SubprocessError):
        return unknown
    if not REVISION.fullmatch(head):
        return unknown
    return {"revision": head, "tracked_tree": {0: "clean", 1: "dirty"}.get(diff.returncode, "unknown")}


def expectations(values, suite):
    """Validate the explicit target fields and optional identity restrictions before I/O."""
    validate_expected_identity(values)
    if suite == "fixture-operations" and (values["target"], values["backend"]) != ("host", "sim-mdio"):
        raise ProtocolError("fixture-operations requires the host sim-mdio producer")
    return values


def decode_chunk(decoder, session, chunk):
    """Keep complete prior receipts even when a later line in the same chunk is invalid."""
    for part in chunk.splitlines(keepends=True):
        for record in decoder.feed(part):
            session.accept(record)


def read_capture(path, limit, save, consume):
    """Copy a regular transcript up to the run limit, including trailing evidence."""
    if not path.is_file():
        raise OSError("transcript must be a regular file")
    with path.open("rb") as source:
        remaining = limit + 1
        while remaining:
            data = source.read(min(4096, remaining))
            if not data:
                break
            save(data)
            consume(data)
            remaining -= len(data)


def capture(args, suite, nonce, expected):
    """Execute a host fixture or verify a supplied transcript and finalize its pair."""
    commands = plan(suite, nonce)
    session = Session(commands, nonce, expected)
    decoder = Decoder(max_bytes=args.max_bytes)
    started = utc_now()
    provenance = runner_identity()
    live = args.mode == "run-fixture"
    label = str(args.path.absolute()) if live else "recorded-offline"
    files = EvidenceFiles(args.out_dir, args.run, args.max_bytes)
    issues = []
    sent = []
    fixture = None
    interrupted = False

    def consume(data):
        decode_chunk(decoder, session, data)

    def offline_consume(data):
        if not issues:
            try:
                consume(data)
            except ProtocolError as error:
                issues.append(str(error))

    try:
        if live:
            if not args.path.is_file():
                raise FixtureError("compiled fixture must be a regular executable file")
            argv = [label]
            if args.fixture_identity:
                argv += ["--identity", args.fixture_identity]
            fixture = FixtureProcess(argv, args.timeout, args.max_bytes, files.write)
            with fixture:
                for command in commands:
                    fixture.send(command.text.encode("ascii") + b"\n")
                    sent.append(command.text)
                    session.issue()
                    while len(session.results) < session.issued:
                        chunk = fixture.read()
                        if not chunk:
                            raise ProtocolError("fixture stdout closed before a correlated terminal")
                        consume(chunk)
                fixture.finish(args.closure, consume)
        else:
            sent = [command.text for command in commands]
            session.issued = len(commands)
            read_capture(args.path, args.max_bytes, files.write, offline_consume)
        if not issues:
            decoder.finish()
            session.finish()
    except KeyboardInterrupt:
        interrupted = True
        issues.append("interrupted by operator")
    except (OSError, ProtocolError, FixtureError, EvidenceError) as error:
        issues.append(str(error))
    try:
        if fixture:
            if fixture.truncated:
                files.mark_truncated()
                issues.append("fixture capture limit reached")
            if fixture.cleanup_issue:
                issues.append(fixture.cleanup_issue)
        data = files.finish({
            "suite": suite, "fixture": label, "started_utc": started, "ended_utc": utc_now(),
            "complete": not issues, "commands": sent, "cases": cases_for(suite, session.results, session.samples),
            "identity": session.identity, "expectations": expected, "runner": provenance,
            "stderr": base64.b64encode(fixture.stderr).decode("ascii") if fixture else "",
            "issues": issues,
        })
    except BaseException:
        files.abort()
        raise
    return data, interrupted


def verify(path):
    """Verify the pair and replay complete evidence with the same protocol and assertions."""
    data, transcript = verify_artifacts(path)
    if data["complete"]:
        commands = data["commands"]
        if not commands or not commands[0].startswith("session "):
            raise EvidenceError("complete evidence has no session command")
        nonce = commands[0][len("session "):]
        suite = data["suite"]
        expected = expectations(data["expectations"], suite)
        selected = plan(suite, nonce)
        if commands != [command.text for command in selected]:
            raise EvidenceError("recorded commands differ from the known suite")
        session = Session(selected, nonce, expected)
        session.issued = len(selected)
        decoder = Decoder(max_bytes=MAX_TRANSCRIPT_BYTES)
        for index in range(0, len(transcript), 4096):
            decode_chunk(decoder, session, transcript[index:index + 4096])
        decoder.finish()
        session.finish()
        if session.identity != data["identity"] or cases_for(suite, session.results) != data["cases"]:
            raise EvidenceError("manifest identity or case claims do not match the transcript")
    return data


def positive_seconds(text):
    """Argparse converter for a finite positive duration within 120 seconds."""
    try:
        value = float(text)
    except ValueError:
        raise argparse.ArgumentTypeError("use a positive number of seconds") from None
    if not math.isfinite(value) or not 0 < value <= 120:
        raise argparse.ArgumentTypeError("duration must be finite and in (0, 120] seconds")
    return value


def byte_limit(text):
    """Argparse converter for a bounded positive capture byte count."""
    try:
        return integer(text, 1, MAX_TRANSCRIPT_BYTES)
    except ProtocolError:
        raise argparse.ArgumentTypeError(f"byte limit must be 1 through {MAX_TRANSCRIPT_BYTES}") from None


def parser():
    """Create the single runner CLI, with mutually exclusive operation modes."""
    cli = argparse.ArgumentParser(description=__doc__)
    modes = cli.add_subparsers(dest="mode", required=True)
    for name, description in (
        ("dry-run", "print a host-only suite without opening files or a device"),
        ("verify-transcript", "verify a recorded raw stdout transcript into a new evidence pair"),
        ("run-fixture", "run the explicit compiled host CLI fixture and write a new evidence pair"),
    ):
        command = modes.add_parser(name, help=description)
        command.add_argument("--suite", choices=SUITES, required=True)
        command.add_argument("--nonce", required=name == "verify-transcript")
        for key in TARGET_FIELDS + OPTIONAL_FIELDS:
            command.add_argument("--" + key.replace("_", "-"), required=key in TARGET_FIELDS,
                                 type=int if key in ("address", "baud") else str)
        if name != "dry-run":
            command.add_argument("path", type=Path, help="raw transcript or compiled fixture path")
            command.add_argument("--out-dir", type=Path, required=True)
            command.add_argument("--run", required=True, help="exclusive output basename")
            command.add_argument("--max-bytes", type=byte_limit, default=MAX_TRANSCRIPT_BYTES)
        if name == "run-fixture":
            command.add_argument("--timeout", type=positive_seconds, default=5.0)
            command.add_argument("--closure", type=positive_seconds, default=0.5)
            command.add_argument("--fixture-identity", help="explicit simulated identity reported by the host fixture")
    modes.add_parser("verify-artifacts", help="read-only verification of an existing evidence pair").add_argument(
        "path", type=Path, help="path to the run JSON")
    return cli


def main(argv=None):
    """Return 0 for verified host evidence, 1 for FAIL, 2 if incomplete, and 130 if interrupted."""
    cli = parser()
    args = cli.parse_args(argv)
    interrupted = False
    try:
        if args.mode == "verify-artifacts":
            data = verify(args.path)
        else:
            expected = expectations({key: getattr(args, key) for key in TARGET_FIELDS + OPTIONAL_FIELDS
                                     if getattr(args, key) is not None}, args.suite)
            nonce = args.nonce if args.nonce is not None else (
                "preview" if args.mode == "dry-run" else secrets.token_hex(12))
            commands = plan(args.suite, nonce)
            if args.mode == "run-fixture":
                if (expected["target"], expected["backend"]) != ("host", "sim-mdio"):
                    raise ProtocolError("compiled fixture must expect host sim-mdio")
                if not 0 < args.closure < args.timeout or args.closure > 5:
                    raise ProtocolError("closure must be at most 5 seconds and shorter than the total timeout")
                if args.fixture_identity is not None and not TOKEN.fullmatch(args.fixture_identity):
                    raise ProtocolError("invalid simulated fixture identity")
            if args.mode == "dry-run":
                print(json.dumps({"scope": "host_protocol", "executed": False,
                                  "suite": args.suite, "expectations": expected,
                                  "commands": [command.text for command in commands]}, indent=2))
                return 0
            data, interrupted = capture(args, args.suite, nonce, expected)
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        return 130
    except (OSError, ProtocolError, FixtureError, EvidenceError) as error:
        print(f"INCONCLUSIVE (host protocol): {error}", file=sys.stderr)
        return 2
    verdict = data["verdict"]
    print(f"{verdict} (host protocol)")
    for issue in data["issues"]:
        print(issue, file=sys.stderr)
    return 130 if interrupted else {"PASS": 0, "FAIL": 1}.get(verdict, 2)


if __name__ == "__main__":
    raise SystemExit(main())
