#!/usr/bin/env python3
"""Verify DP8382x console protocol and preserve bounded evidence."""

import argparse
import base64
from contextlib import contextmanager
from dataclasses import dataclass, field
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import re
import secrets
import subprocess
import sys
import threading
import time

from dp8382x_hil_artifacts import (
    MAX_TRANSCRIPT_BYTES, SERIAL_SCOPE, EvidenceError, EvidenceFiles, soak_plan, validate_destination, verify_artifacts,
)
from dp8382x_hil_campaign import (
    FIXTURE_ACTIONS, MAX_COMMANDS, MAX_INTERVENTIONS, AssertionFailed, Campaign, Command, Intervention,
    all_cases, bits, clean, exact_fields, link_changed, payload, validate_declaration, validate_intervention,
)
from dp8382x_hil_fixture import FixtureError, FixtureProcess
from dp8382x_hil_protocol import (
    DP83822, PHY_ID2, TERMINAL_STATUS, U32_MAX, U64_MAX, VARIANTS, Decoder, ProtocolError, Record,
    copper_straps_match, identity, integer, observed_identity, validate_expected_identity,
)
from dp8382x_hil_serial import (
    IDLE_POLL, INJECTED_BACKEND, READ_CHUNK, ReplyTimeout, SerialError, SerialPort, bounded_call, native_port,
    serial_settings,
)
from dp8382x_hil_soak import SoakCampaign


TOKEN = re.compile(r"[A-Za-z0-9_.:-]{1,64}\Z")
REVISION = re.compile(r"(?:[0-9a-f]{40}|[0-9a-f]{64})\Z")
TARGET_FIELDS = ("target", "backend", "variant", "interface", "address")
OPTIONAL_FIELDS = ("firmware", "firmware_prefix", "tree", "sdk", "clock", "console", "baud", "rmii_role", "medium")
FEATURE_EXCHANGE_SECONDS = 2.0


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


def check_payload(record, variant):
    """Check typed operation and sequencer data consumed by the built-in plans."""
    fields = payload(record)
    if record.cmd == "registers.write" and record.primary == "INVALID_PARAM":
        if fields:
            raise ProtocolError("a syntactic write rejection has unexpected effect evidence")
    elif record.cmd in ("connection.initialize", "recovery.reset", "recovery.restart", "recovery.reinitialize"):
        if record.state == "admitted" or record.op == 0:
            if fields:
                raise ProtocolError("initialization admission has unexpected fields")
            return
        flags = ("reset_required reset_accepted reset_complete persistent_restored mutation_completed "
                 "mutation_possible cleanup_owed temporary clock_invalid link_valid sor1_valid sor2_valid straps_stale")
        checks = {name: ("0", "1") for name in flags.split()}
        checks.update({name: ("none", "pcs", "hard", "restart", "eee-pcs", "unknown") for name in
                       ("requested_reset", "effective_reset")})
        checks.update({"core_status": tuple(TERMINAL_STATUS), "core_detail": (-2**31, 2**31-1),
                       "sor1": (0, 65535), "sor2": (0, 65535)})
        # Before the nonce binds, prior firmware can emit either complete reset shape.
        if variant == "DP83825I" or (variant is None and "rcsr" in fields):
            checks.update({"rcsr_valid": ("0", "1"), "rcsr": (0, 65535)})
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
        self.request_limit = len(commands)

    def issue(self):
        """Mark the next full command write. Never advance an outstanding request."""
        if self.issued != len(self.results) or self.issued == len(self.commands):
            raise ProtocolError("command sent while another request is outstanding")
        self.issued += 1

    def accept(self, record):
        """Accept one decoded record, or reject it before changing case evidence."""
        check_payload(record, self.identity.get("variant"))
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
            if record.req > U64_MAX - (self.request_limit - 1):
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


# Dp8382xCli.cpp CONFIG_US and DIRECT_US. Admission and terminal clocks
# may be sampled later than the scheduling timestamp.
FEATURE_READS = frozenset("irq.service power.read eee.read wol.read controls.mse".split())
FEATURE_WRITES = frozenset("lpi.start lpi.stop controls.pins controls.interface".split())
FEATURE_OPERATIONS = frozenset(("""
    diagnostics.loopback-start diagnostics.loopback-stop diagnostics.bist-start diagnostics.bist-read
    diagnostics.bist-stop diagnostics.cable-start diagnostics.cable-raw diagnostics.cable-read
    diagnostics.cable-stop diagnostics.cable-autorun diagnostics.cable-autorun-read
    diagnostics.compliance-start diagnostics.compliance-stop diagnostics.collision-start
    diagnostics.collision-stop diagnostics.calibration-recover irq.configure irq.test-start irq.test-stop
    power.configure eee.configure wol.configure wol.acknowledge controls.quality controls.analog
""").split())
TIMED_COMMANDS = {**dict.fromkeys(("configuration.apply", "configuration.verify", "recovery.reset",
                                 "recovery.restart", "recovery.reinitialize", "connection.initialize",
                                 *FEATURE_OPERATIONS, *FEATURE_WRITES), 5_000_000),
                  **dict.fromkeys(("link.read", "link.partner", "registers.read", *FEATURE_READS), 2_000_000)}


def check_feature_envelope(record):
    """Validate the shared operation fields while retaining feature-specific partial samples."""
    if record.cmd not in FEATURE_OPERATIONS:
        return
    if record.state == "admitted":
        exact_fields(payload(record), {})
    elif record.op:
        checks = {**bits("mutation_completed mutation_possible cleanup_owed temporary persistent_restored "
                         "reset_accepted reset_complete"), "core_status": tuple(TERMINAL_STATUS),
                  "core_detail": (-2**31, 2**31 - 1),
                  "effective_reset": ("none", "pcs", "hard", "restart", "eee-pcs", "unknown")}
        fields = {key: record.fields[key] for key in checks if key in record.fields}
        exact_fields(fields, checks)
        if record.primary == "OK" and (fields["core_status"], fields["core_detail"]) != ("OK", "0"):
            raise ProtocolError("feature terminal disagrees with the successful core status")


class CampaignSession(Session):
    """Correlate the dynamic case with the same one-request record owner."""

    def __init__(self, nonce, expected, declaration, recorded=None, soak=None):
        if not isinstance(nonce, str) or not TOKEN.fullmatch(nonce):
            raise ProtocolError("invalid session nonce")
        super().__init__([Command("session " + nonce)], nonce, expected)
        self.soak = soak
        self.setup = []
        self.setup_tail = None
        self.received_records = 0
        self.unconfirmed_command = None
        self.cleanup_boundary = None
        self.campaign = (SoakCampaign(declaration, expected, soak["count"]) if soak else
                         Campaign(declaration, expected))
        self.request_limit = soak["command_limit"] if soak else MAX_COMMANDS - len(self.campaign.baseline)
        self.recorded = recorded

    @property
    def next(self):
        if not self.results:
            return self.commands[0]
        return self.campaign.next

    def intervene(self, row):
        self.campaign.advance(row)
        self._queue()

    def attempting(self):
        """A short, late or uncertain write can already have changed the device."""
        if self.soak and self.results:
            self.campaign.attempt()

    def expire_primary(self):
        """Drop the unissued primary command and queue only the owner's cleanup route."""
        self.cleanup_boundary = {"records": self.received_records, "interventions": len(self.campaign.interventions)}
        del self.commands[self.issued:]
        self.campaign.expire_primary()
        self._queue()

    def _queue(self):
        if isinstance(self.next, Command):
            if len(self.commands) >= self.request_limit:
                raise ProtocolError("campaign command ceiling reached")
            self.commands.append(self.next)

    def accept(self, record):
        if self.soak and (self.identity or (record.cmd == "session" and record.fields.get("nonce") == self.nonce)):
            self.received_records += 1
        if self.setup_tail and not self.identity and (
                record.cmd != "session" or record.fields.get("nonce") != self.nonce or
                record.boot != self.setup_tail.boot or record.req != self.setup_tail.req + 1):
            raise ProtocolError("nonce does not follow the compiled setup in the same boot")
        if self.recorded is not None and self.issued == len(self.results):
            if not self.identity and (record.cmd != "session" or record.fields.get("nonce") != self.nonce):
                super().accept(record)
                return
            while isinstance(self.next, Intervention):
                index = len(self.campaign.interventions)
                if index >= len(self.recorded):
                    raise ProtocolError("missing intervention receipt before next command")
                self.intervene(self.recorded[index])
            if self.next is not None:
                self.attempting()
                self.issue()
        index = len(self.results)
        super().accept(record)
        if not self.identity:
            return
        fields = record.fields
        limit = TIMED_COMMANDS.get(record.cmd)
        timed_success = record.primary == "OK" and limit is not None
        retryable_link = record.cmd in ("link.read", "link.partner") and link_changed(record)
        check_feature_envelope(record)
        if record.state == "admitted" or timed_success or retryable_link:
            duration = int(fields["deadline_us"]) - int(fields["time_us"])
            if limit is None or fields["deadline_valid"] != "1" or not 0 < duration <= limit:
                raise ProtocolError("campaign command has no valid bounded deadline")
        if len(self.results) == index:
            return
        if fields["time_valid"] != "1":
            raise ProtocolError("campaign record clock is unavailable")
        if record.cmd not in TIMED_COMMANDS and fields["deadline_valid"] != "0":
            raise ProtocolError("campaign cached record carries a deadline")
        self.campaign.advance(record)
        self._queue()

    def finish(self):
        if self.recorded is not None:
            while isinstance(self.next, Intervention) and len(self.campaign.interventions) < len(self.recorded):
                self.intervene(self.recorded[len(self.campaign.interventions)])
            if len(self.recorded) != len(self.campaign.interventions):
                raise ProtocolError("unexpected intervention receipt after case")
        super().finish()
        if self.next is not None:
            raise ProtocolError("missing campaign command or intervention")

    def evidence(self):
        return {"cases": self.campaign.cases(), "campaign": self.campaign.receipt()}


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
                "clock_invalid": "0", "link_valid": "0", "sor1_valid": "1", "sor2_valid": "0" if rows[0].fields["variant"] == "DP83825I" else "1", "straps_stale": "0",
            }.items())
            if rows[0].fields["variant"] == "DP83825I":
                follower = rows[0].fields["rmii_role"] == "follower"
                passed = (passed and fields.get("rcsr_valid") == "1" and
                          bool(int(fields["sor1"]) & 8) == bool(int(fields["rcsr"]) & 128) == follower)
            restore = "PASS" if restored else "NOT RUN"
            if rows[0].fields["variant"] in DP83822 and restored:
                board = rows[0].fields
                if not (fields.get("sor1_valid") == fields.get("sor2_valid") == "1" and
                        fields.get("straps_stale") == "0" and
                        copper_straps_match(int(fields["sor1"]), int(fields["sor2"]),
                                            board["interface"], int(board["address"]))):
                    passed, restore = False, "FAIL"
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
                           "id2": str(model), "value": "0", "attempted": "2"}
                          for model in PHY_ID2[rows[0].fields["variant"]]) and
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


def runner_identity(deadline=None):
    """Record Git provenance for this runner checkout, or unknown for an archive."""
    def git_timeout():
        if deadline is None:
            return 2
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise subprocess.TimeoutExpired("runner provenance", 0)
        # Leave time for subprocess.run to terminate and reap a timed-out Git query.
        return min(2, remaining / 2)

    root = Path(__file__).resolve().parents[1]
    unknown = {"revision": "unknown", "tracked_tree": "unknown"}
    if not (root / ".git").exists():
        return unknown
    try:
        git = ["git", "-C", str(root)]
        head = subprocess.run(git + ["rev-parse", "HEAD"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                              timeout=git_timeout(), check=True, text=True).stdout.strip()
        diff = subprocess.run(git + ["diff-index", "--quiet", "HEAD", "--"], stdout=subprocess.DEVNULL,
                              stderr=subprocess.DEVNULL, timeout=git_timeout(), check=False)
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
    if suite == "campaign" and (
            not ({"firmware", "firmware_prefix"} & values.keys()) or
            not all(key in values and values[key] != "unknown" for key in ("clock", "console", "baud")) or
            (values["target"] == "host") != (values["backend"] == "sim-mdio")):
        raise ProtocolError("campaign requires verified firmware, clock, console and baud")
    return values


def decode_chunk(decoder, accept, chunk):
    """Keep complete prior receipts even when a later line in the same chunk is invalid."""
    for part in chunk.splitlines(keepends=True):
        for record in decoder.feed(part):
            accept(record)


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


def make_session(suite, nonce, expected, declaration=None, recorded=None, soak=None):
    if suite == "campaign":
        return CampaignSession(nonce, expected, declaration, recorded, soak)
    return Session(plan(suite, nonce), nonce, expected)


def live_steps(session):
    """Yield one admitted step at a time after its predecessor has a receipt."""
    if isinstance(session, CampaignSession):
        while session.next is not None:
            yield session.next
    else:
        yield from session.commands


def case_evidence(suite, session):
    if isinstance(session, CampaignSession):
        return session.evidence()
    return {"cases": cases_for(suite, session.results, session.samples)}


def intervention(session, step, source, ceiling, act):
    """Bind the bounded external declaration to its preceding correlated terminal."""
    started = time.monotonic()
    budget = session.campaign.declaration["intervention_timeout_ms"]
    budget = min(budget, max(0, int((ceiling - started) * 1000)))
    deadline = started + budget / 1000
    failure = None
    try:
        outcome = act(step.action, deadline) if budget else "timed_out"
    except OSError as error:
        # Console input failure need not break the PHY session or bar recovery.
        if source != "operator":
            failure = error
        outcome = "failed"
    except (FixtureError, KeyboardInterrupt) as error:
        failure = error
        outcome = "failed"
    finished = time.monotonic()
    if finished >= deadline:
        outcome = "timed_out"
    elapsed = budget if outcome == "timed_out" else min(budget - 1, max(0, int((finished - started) * 1000)))
    last = session.campaign.last
    session.intervene({"action": step.action, "source": source, "outcome": outcome, "after_req": last.req,
                       "boot": last.boot, "elapsed_ms": elapsed, "budget_ms": budget, "cleanup": step.cleanup})
    if failure is not None:
        raise failure


def operator_acknowledgment(action, deadline, cancelled=None):
    """Read at most one bounded console line. The answer declares operator action."""
    def active():
        return time.monotonic() < deadline and (cancelled is None or not cancelled.is_set())

    if not sys.stdin.isatty():
        return "declined"
    response = f"{action} {secrets.token_hex(6)}"
    if not active():
        return "timed_out"
    print(f"Perform {action} on the declared fixture. Type '{response}' to acknowledge.",
          file=sys.stderr, flush=True)
    line = ""
    if os.name == "nt":
        import msvcrt
        while active():
            if not msvcrt.kbhit():
                time.sleep(min(IDLE_POLL, max(0, deadline - time.monotonic())))
                continue
            char = msvcrt.getwch()
            if char in ("\r", "\n"):
                return "acknowledged" if line == response else "declined"
            line += char
            if len(line) > 64:
                return "declined"
    else:
        import select
        while active():
            if not select.select([sys.stdin], [], [], max(0, deadline - time.monotonic()))[0]:
                break
            char = os.read(sys.stdin.fileno(), 1)
            if not char or char == b"\n":
                return "acknowledged" if char and line == response else "declined"
            line += char.decode("ascii", "replace")
            if len(line) > 64:
                return "declined"
    return "timed_out"


def baseline_reply(record, command, prior, admission, session):
    """Check a pre-nonce fixture response using the same rules during execution and replay."""
    if (record.cmd != command.name or
            (prior and (record.boot != prior.boot or record.req != prior.req + 1))):
        raise ProtocolError("compiled feature baseline reply is out of sequence")
    check_payload(record, session.expected["variant"])
    check_feature_envelope(record)
    fields = record.fields
    timestamp = int(fields["time_us"])
    if (fields["time_valid"] != "1" or fields["deadline_valid"] != "1" or
            not 0 < int(fields["deadline_us"]) - timestamp <= TIMED_COMMANDS[command.name] or
            (session.last_time is not None and timestamp < session.last_time)):
        raise ProtocolError("invalid clock or deadline in compiled setup")
    session.last_time = timestamp
    if record.state == "admitted" and command.name in FEATURE_OPERATIONS and admission is None:
        if record.op in session.used_tokens:
            raise ProtocolError("reused compiled setup operation token")
        session.used_tokens.add(record.op)
        return record
    if (record.state != "terminal" or not clean(record) or
            (command.name in FEATURE_OPERATIONS) != (admission is not None) or
            record.op != (admission.op if admission else 0) or
            (admission and (record.req, record.boot, record.fields["deadline_us"]) !=
             (admission.req, admission.boot, admission.fields["deadline_us"]))):
        raise ProtocolError(f"compiled feature baseline did not complete: {command.text} ({record.primary})")
    campaign = session.campaign.current if session.soak else session.campaign
    try:
        spec = campaign.part_control
        if spec is not None:
            if command.text != spec.baseline.command:
                raise ProtocolError("unsupported declared-part baseline")
            check = campaign.feature.operation if spec.form == "operation" else campaign.feature.direct
            check(record, spec.schema, spec.baseline.fields)
        else:
            campaign.feature.check_baseline(record, command.text, campaign.declaration["mode"], campaign.a_feature)
    except AssertionFailed as error:
        raise ProtocolError(str(error)) from error
    return None


def prepare_feature_fixture(fixture, decoder, commands, deadline, session, setup=None):
    """Install a fixed offline baseline and drain its receipts before opening the nonce session."""
    if len(commands) > 8:
        raise FixtureError("compiled feature baseline exceeds eight commands")
    prior = None
    for text in commands:
        command = Command(text)
        exchange_end = min(deadline, time.monotonic() + FEATURE_EXCHANGE_SECONDS)
        if time.monotonic() >= exchange_end:
            raise FixtureError("compiled feature setup deadline expired")
        if setup is not None:
            setup.append({"command": text, "state": "uncertain"})
        fixture.send(text.encode("ascii") + b"\n", exchange_end)
        if setup is not None:
            setup[-1]["state"] = "written"
        admission = None
        complete = False

        def accept(record):
            nonlocal prior, admission, complete
            if complete:
                raise ProtocolError("compiled feature baseline reply is out of sequence")
            admission = baseline_reply(record, command, prior, admission, session)
            if admission is None:
                prior = record
                complete = True
                if setup is not None:
                    setup[-1]["state"] = "terminal"

        while not complete:
            chunk = fixture.read(exchange_end)
            if not chunk:
                raise FixtureError("compiled feature baseline closed before completion")
            decode_chunk(decoder, accept, chunk)
        if decoder.has_partial_record:
            raise ProtocolError("unfinished record after compiled feature baseline")
    return prior


class SoakDuration(ProtocolError):
    """The primary window closed before the next request completed."""


class SetupReplay:
    """Account separately for the simulator's fixed pre-nonce setup commands."""

    def __init__(self, session):
        self.session = session
        self.commands = session.campaign.baseline if session.expected["backend"] == "sim-mdio" else ()
        self.index = 0
        self.last = self.admission = None

    def accept(self, record):
        if self.session.identity or not self.commands:
            return False
        if record.cmd == "session" and record.fields.get("nonce") == self.session.nonce:
            if self.admission or (self.last and (self.last.boot != record.boot or self.last.req + 1 != record.req)):
                raise ProtocolError("nonce does not follow the compiled setup")
            return False
        if self.index >= len(self.commands):
            raise ProtocolError("unexpected record after compiled setup")
        command = Command(self.commands[self.index])
        if self.admission is None:
            self.session.setup.append({"command": command.text, "state": "written"})
        self.admission = baseline_reply(record, command, self.last, self.admission, self.session)
        if self.admission is None:
            self.session.setup[-1]["state"] = "terminal"
            self.index += 1
            self.last = record
            self.session.setup_tail = record
        return True


class SoakBudget:
    """One host clock for setup, primary work, cumulative cleanup, close and publication."""

    def __init__(self, session, start, primary_end, cleanup_seconds, close_start, end):
        self.session = session
        self.start = start
        self.primary_end = min(primary_end, start + session.soak["duration_ms"] / 1000)
        self.cleanup_left = cleanup_seconds
        self.close_start = close_start
        self.end = end

    @contextmanager
    def step(self, cleanup):
        entered = time.monotonic()
        limit = min(self.close_start, entered + self.cleanup_left) if cleanup else self.primary_end
        if entered >= limit:
            if not cleanup:
                raise SoakDuration("soak primary duration expired before next request")
            raise ProtocolError("soak restoration allowance exhausted")
        try:
            yield limit
            if time.monotonic() >= limit:
                raise (ProtocolError("soak restoration allowance expired") if cleanup else
                       SoakDuration("soak primary duration expired during request"))
        except (FixtureError, SerialError) as error:
            if not cleanup and "deadline" in str(error) and time.monotonic() >= limit:
                raise SoakDuration(str(error)) from error
            raise
        finally:
            if cleanup:
                self.cleanup_left = max(0, self.cleanup_left - (time.monotonic() - entered))

    def evidence(self):
        return {"soak": {**self.session.campaign.summary(), "plan": self.session.soak,
                         **({"cleanup_boundary": self.session.cleanup_boundary} if self.session.cleanup_boundary else {}),
                         "setup": self.session.setup,
                         "received_records": self.session.received_records,
                         "unconfirmed_command": self.session.unconfirmed_command,
                         "primary_window_ms": max(0, round((self.primary_end - self.start) * 1000)),
                         "elapsed_ms": max(0, int((time.monotonic() - self.start) * 1000))}}


def run_soak_steps(session, budget, execute, decoder, issues):
    """Only a clean boundary can transfer the expired primary to its cleanup route."""
    try:
        for step in live_steps(session):
            with budget.step(step.cleanup) as deadline:
                execute(step, deadline)
    except SoakDuration as error:
        if (error.__cause__ or not session.identity or session.unconfirmed_command or
                session.issued != len(session.results) or decoder.has_partial_record or session.next is None):
            raise
        issues.append("soak primary duration expired")
        session.expire_primary()
        for step in live_steps(session):
            with budget.step(True) as deadline:
                execute(step, deadline)
        raise


def finalization_diagnostic(data):
    """Retain the first failure and each primary/restoration outcome if JSON publication fails."""
    findings = [issue[:256] for issue in data["issues"][:2]]
    for row in data["cases"]:
        if row["verdict"] != "NOT RUN":
            findings.append(f'{row["id"]} {row["verdict"]} primary={row["primary"]} '
                            f'restoration={row["restoration"]["verdict"]} {row["message"][:256]}')
    return " | ".join(findings)


def host_io(work, deadline, phase, cancel=None):
    """Bound filesystem work without operating on a file still owned by a worker."""
    result = bounded_call(work, deadline)
    if result.pending:
        if cancel is not None:
            bounded_call(cancel, time.monotonic() + 0.05)
        try:
            bounded_call(lambda: print(f"INCONCLUSIVE: {phase}: {result.detail}", file=sys.stderr, flush=True),
                         time.monotonic() + 0.05)
        finally:
            os._exit(2)
    if result.cancelled:
        raise KeyboardInterrupt
    if result.state != "ok":
        if isinstance(result.value, EvidenceFiles):
            host_io(result.value.abort, time.monotonic() + 0.05, "close late reservation")
        raise EvidenceError(f"{phase}: {result.state}: {result.detail}")
    return result.value


def capture(args, suite, nonce, expected, declaration=None, recorded=None):
    """Execute a host fixture or verify a supplied transcript and finalize its pair."""
    start = time.monotonic()
    soak = getattr(args, "soak", None)
    session = make_session(suite, nonce, expected, declaration, recorded, soak)
    decoder = Decoder(max_bytes=args.max_bytes)
    started = utc_now()
    live = args.mode == "run-fixture"
    executable = str(args.path.absolute())
    label = declaration["board"] if declaration else executable if live else "recorded-offline"
    budget = None
    if soak:
        total = getattr(args, "timeout", 120.0)
        closure = getattr(args, "closure", 0.5)
        work = total - closure
        finalize = min(2.0, work / 4)
        restore = min(3.0, work / 4)
        close_start = start + work - finalize
        budget = SoakBudget(session, start, close_start - restore, restore, close_start, start + total)
    provenance = (host_io(lambda: runner_identity(budget.primary_end), budget.close_start,
                          "collect soak provenance") if budget else runner_identity())
    base = {"suite": suite, "fixture": label, "started_utc": started,
            "expectations": expected, "runner": provenance, "stderr": ""}
    if budget:
        provisional = {**base, **session.evidence(), **budget.evidence()}
        files = host_io(lambda: EvidenceFiles(args.out_dir, args.run, args.max_bytes, provisional=provisional),
                        budget.close_start, "soak reservation")
    else:
        files = EvidenceFiles(args.out_dir, args.run, args.max_bytes)
    # The live carrier owns its bounded RAM prefix. Disk I/O takes place
    # after child teardown, under the separate publication deadline.
    captured = bytearray()

    def retain(data):
        count = min(len(data), args.max_bytes - len(captured))
        captured.extend(data[:count])
        if count < len(data):
            files.mark_truncated()
        return count

    issues = []
    sent = []
    fixture = None
    interrupted = False

    setup_replay = SetupReplay(session) if soak and not live else None

    def accept(record):
        if setup_replay is None or not setup_replay.accept(record):
            session.accept(record)

    def consume(data):
        decode_chunk(decoder, accept, data)

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
            argv = [executable]
            if args.fixture_identity:
                argv += ["--identity", args.fixture_identity]
            if args.fixture_variant:
                argv += ["--variant", args.fixture_variant]
            if (args.fixture_variant == "DP83825I") != bool(args.fixture_rmii_role):
                raise FixtureError("DP83825I fixture requires an explicit --fixture-rmii-role")
            if args.fixture_rmii_role:
                argv += ["--rmii-role", args.fixture_rmii_role]
            if args.fixture_variant in DP83822 and expected["interface"] in ("MII", "RGMII"):
                argv.append("--mii" if expected["interface"] == "MII" else "--rgmii")
            baseline = ()
            feature = None
            if declaration:
                spec = session.campaign.spec
                feature = session.campaign.feature
                peer = spec.peer or "an-100-full"
                if feature:
                    peer = getattr(feature, "PEERS", {}).get(spec.id, "open")
                    # A mismatched simulated declaration may only establish the
                    # failed session identity. Keep pre-nonce feature setup idle.
                    grade_matches = (args.fixture_variant or "DP83826I") == expected["variant"]
                    if grade_matches and declaration["polling"] == "exclusive" and session.campaign.feature_gate() is None:
                        argv.append("--attest-features")
                        if not args.fixture_deny_maintenance:
                            baseline = session.campaign.baseline
                argv += ["--campaign", declaration["mode"], "--campaign-peer", peer]
                if expected["interface"] == "MII" and args.fixture_variant not in DP83822:
                    argv.append("--mii")
                if args.fixture_deny_maintenance:
                    argv.append("--deny-maintenance")
            if budget and time.monotonic() >= budget.primary_end:
                raise SoakDuration("soak primary duration expired before fixture launch")
            lifetime = budget.close_start + args.closure - time.monotonic() if budget else args.timeout
            fixture = FixtureProcess(argv, lifetime, args.max_bytes, retain if budget else files.write)
            with fixture:
                end = time.monotonic() + args.timeout - args.closure
                active = end - min(1, (args.timeout - args.closure) / 3)
                if budget:
                    with budget.step(False) as deadline:
                        session.setup_tail = prepare_feature_fixture(fixture, decoder, baseline, deadline,
                                                                    session, session.setup)
                else:
                    prepare_feature_fixture(fixture, decoder, baseline, active, session)

                def fixture_action(action, deadline):
                    if action in FIXTURE_ACTIONS:
                        line = FIXTURE_ACTIONS[action]
                    else:
                        selected = peer if action in ("replug", "release-peer-reset") else "open"
                        control = ("$campaign-peer-delayed" if spec.id == "bounded-recovery" and selected != "open"
                                   else "$campaign-peer")
                        line = f"{control} {selected}"
                    fixture.send(f"{line}\n".encode("ascii"), deadline)
                    return "acknowledged"

                def execute(command, deadline):
                    if declaration and decoder.has_partial_record:
                        raise ProtocolError("unsolicited record began before the next step")
                    if isinstance(command, Intervention):
                        intervention(session, command, "compiled-fixture", deadline, fixture_action)
                        return
                    if feature:
                        deadline = min(deadline, time.monotonic() + FEATURE_EXCHANGE_SECONDS)
                    if budget:
                        session.attempting()
                        session.unconfirmed_command = command.text
                    fixture.send(command.text.encode("ascii") + b"\n", deadline)
                    if budget:
                        session.unconfirmed_command = None
                    sent.append(command.text)
                    session.issue()
                    while len(session.results) < session.issued:
                        chunk = fixture.read(deadline)
                        if not chunk:
                            raise ProtocolError("fixture stdout closed before a correlated terminal")
                        consume(chunk)
                if budget:
                    run_soak_steps(session, budget, execute, decoder, issues)
                else:
                    for command in live_steps(session):
                        execute(command, (end if command.cleanup else active) if declaration else None)
                fixture.finish(args.closure, consume)
        else:
            if declaration is None:
                sent = [command.text for command in session.commands]
                session.issued = len(sent)
            if budget:
                with budget.step(False):
                    host_io(lambda: read_capture(args.path, args.max_bytes, retain, offline_consume),
                            budget.primary_end, "read soak transcript")
            else:
                read_capture(args.path, args.max_bytes, files.write, offline_consume)
            if declaration is not None:
                sent = [command.text for command in session.commands[:session.issued]]
        if not issues:
            decoder.finish()
            session.finish()
            if (setup_replay and session.campaign.summary()["stop_reason"] == "count" and
                    setup_replay.index != len(setup_replay.commands)):
                raise ProtocolError("successful soak lacks its compiled baseline evidence")
    except KeyboardInterrupt:
        interrupted = True
        issues.append("interrupted by operator")
    except (OSError, ProtocolError, FixtureError, EvidenceError) as error:
        issues.append(str(error))
        if budget:
            if isinstance(error, SoakDuration):
                if "soak primary duration expired" not in issues:
                    issues.append("soak primary duration expired")
                session.campaign.stop("duration")
            else:
                session.campaign.stop("fault")
    try:
        if fixture:
            if fixture.truncated:
                files.mark_truncated()
                issues.append("fixture capture limit reached")
            if fixture.cleanup_issue:
                issues.append(fixture.cleanup_issue)
        if budget and (interrupted or issues) and session.campaign.summary()["stop_reason"] != "duration":
            session.campaign.stop("cancelled" if interrupted else "fault")
        data = {**base, "ended_utc": utc_now(),
            "complete": not issues, "commands": sent, **case_evidence(suite, session),
            "identity": session.identity,
            "stderr": base64.b64encode(fixture.stderr).decode("ascii") if fixture else "",
            "issues": issues,
        }
        if budget:
            data.update(budget.evidence())
            def publish():
                try:
                    files.write(bytes(captured))
                    return files.finish(data, budget.end)
                finally:
                    files.abort()
            data = host_io(publish, budget.end, "soak publication " + finalization_diagnostic(data),
                           files.cancel_publication)
        else:
            data = files.finish(data)
    except BaseException:
        if budget:
            # A publication worker closes its own stream. If admission itself
            # expired, leave the reserved placeholder for the standalone caller.
            if time.monotonic() < budget.end:
                host_io(files.abort, budget.end, "close soak transcript")
        else:
            files.abort()
        raise
    return data, interrupted


class SerialExchange:
    """Use the canonical decoder and retain a bounded raw RX prefix."""

    def __init__(self, port, limit):
        self.port = port
        self.limit = limit
        self.capture = bytearray()
        self.truncated = False
        self.decoder = Decoder(max_bytes=limit)

    def _retain(self, data):
        remaining = self.limit - len(self.capture)
        self.capture.extend(data[:remaining])
        self.truncated |= len(data) > remaining

    def receive(self, deadline, accept, done=None):
        """Read through a terminal or observe a timed tail with no new request."""
        while time.monotonic() < deadline:
            remaining = self.limit - len(self.capture)
            chunk = self.port.read(min(READ_CHUNK, remaining + 1), deadline, self._retain)
            if chunk:
                decode_chunk(self.decoder, accept, chunk[:remaining])
                if self.truncated:
                    raise SerialError("serial capture limit reached")
                if done is not None and done():
                    return
            else:
                time.sleep(min(IDLE_POLL, max(0, deadline - time.monotonic())))
        if done is not None:
            raise ReplyTimeout("serial receive deadline expired")

    def resynchronize(self, session, deadline, receipt, late, *, require_boundary=False):
        """Try one fresh reply without accepting late records into the case results."""
        nonce = secrets.token_hex(12)
        if nonce == session.nonce:
            nonce += "-recovery"
        receipt["state"] = "failed"

        if self.truncated:
            raise SerialError("serial capture limit reached before resynchronization")
        if session.identity and session.identity["session_req"] + session.issued > U64_MAX:
            raise ProtocolError("request counter cannot accommodate a recovery nonce")
        # Advance partial RX framing without crediting anything to the old request.
        def check_late(record):
            if session.identity and record.boot != session.identity["boot"]:
                raise ProtocolError("boot changed before resynchronization")
        decode_chunk(self.decoder, check_late, late)
        if require_boundary and self.decoder.has_partial_record:
            raise ProtocolError("late data left an incomplete record before resynchronization")
        self.port.send("session " + nonce, deadline)
        self.receive(deadline, lambda row: accept_recovery(row, session, nonce, receipt),
                     lambda: receipt["state"] != "failed")
        if require_boundary:
            self.decoder.finish()


def accept_recovery(record, session, nonce, receipt):
    """Validate the fresh identity and the one outstanding request against the original session."""
    if record.cmd != "session" or record.fields.get("nonce") != nonce:
        if session.identity and record.boot != session.identity["boot"]:
            raise ProtocolError("boot changed during resynchronization")
        return
    if receipt["identity"]:
        raise ProtocolError("duplicate resynchronization reply")
    receipt["identity"] = dict(observed_identity(record), boot=record.boot, session_req=record.req)
    verified = identity(record, session.expected)
    receipt["identity"] = dict(verified, boot=record.boot, session_req=record.req)
    if not session.identity:
        receipt["state"] = "unproven"
        receipt["detail"] = "fresh identity observed without prior boot/request evidence"
        return
    previous = dict(session.identity)
    if record.boot != previous.pop("boot"):
        raise ProtocolError("boot changed during resynchronization")
    if record.req != previous.pop("session_req") + session.issued:
        raise ProtocolError("unexpected request number during resynchronization")
    if verified != previous:
        raise ProtocolError("identity changed during resynchronization")
    receipt["state"] = "continuous"


def serial_hard_exit(issues, reason, deadline):
    """Exit without waiting for stalled I/O or invoking backend destruction."""

    def report():
        for issue in issues:
            print(issue, file=sys.stderr, flush=True)
        print(f"INCONCLUSIVE (serial session): {reason}", file=sys.stderr, flush=True)

    try:
        bounded_call(report, min(deadline, time.monotonic() + 0.05))
    finally:
        os._exit(2)


def capture_serial(args, nonce, expected, settings, factory, declaration=None):
    """Run one admitted serial session and publish before releasing its receipt."""
    start = time.monotonic()
    soak = getattr(args, "soak", None)
    session = make_session(args.suite, nonce, expected, declaration, soak=soak)
    port = SerialPort(settings, factory)
    exchange = SerialExchange(port, args.max_bytes)
    limits = settings["limits"]
    recovery = {"state": "not_started", "detail": "", "identity": {}}
    issues = []
    interrupted = False
    console_pending = False

    def request_action(action, deadline):
        nonlocal console_pending, interrupted
        if console_pending:
            return "declined"
        cancel = threading.Event()
        result = bounded_call(lambda: operator_acknowledgment(action, deadline, cancel), deadline, cancel)
        console_pending = result.pending
        if console_pending:
            issues.append("operator console worker still pending")
        if result.cancelled:
            interrupted = True
            raise KeyboardInterrupt
        if result.state == "ok":
            return result.value
        return "timed_out" if result.state in ("late", "stalled") else "failed"

    def receipt(complete):
        return {**settings, "open": port.opened.receipt(), "close": port.closed.receipt(),
                "writes": port.writes, "recovery": recovery, "framing_uncertain": port.framing_uncertain,
                "hard_exit": bool(port.exit_reason or console_pending), "restoration": (
                    "reported_by_case" if declaration else "not_requested" if complete else "unavailable")}

    end = start + limits["session_timeout"]
    cleanup_ceiling = end - limits["finalize_timeout"]
    active = min(start + limits["duration"], cleanup_ceiling - limits["cleanup_timeout"])
    budget = None
    if soak:
        restore = limits["cleanup_timeout"] * 0.75
        budget = SoakBudget(session, start, active, restore,
                            cleanup_ceiling - limits["cleanup_timeout"] + restore, end)
        active = budget.primary_end
    base = {"suite": args.suite, "fixture": settings["board"], "started_utc": utc_now(),
            "expectations": expected, "stderr": ""}

    def reserve():
        base["runner"] = runner_identity(active)
        if not budget and time.monotonic() >= active:
            raise EvidenceError("active deadline expired during provenance collection")
        campaign = session.evidence() if declaration else {}
        if budget:
            campaign.update(budget.evidence())
        return EvidenceFiles(args.out_dir, args.run, args.max_bytes,
                             provisional={**base, "serial": receipt(False), **campaign})

    reservation = bounded_call(reserve, budget.close_start if budget else active)
    if reservation.pending:
        serial_hard_exit([f"evidence reservation {reservation.state}: {reservation.detail}"],
                         "an evidence reservation worker is pending", end)
    if reservation.state != "ok":
        if reservation.value is not None:
            aborted = bounded_call(reservation.value.abort, end)
            if aborted.pending:
                serial_hard_exit([f"evidence reservation cleanup {aborted.state}: {aborted.detail}"],
                                 "an evidence cleanup worker is pending", end)
        if reservation.cancelled:
            raise KeyboardInterrupt
        raise EvidenceError(f"evidence reservation {reservation.state}: {reservation.detail}")
    files = reservation.value
    timeout = None
    cleanup_start = None
    ceiling = active
    try:
        if budget:
            with budget.step(False) as deadline:
                port.open(deadline)
        else:
            port.open(active)

        def execute(command, ceiling):
            nonlocal timeout
            if exchange.decoder.has_partial_record:
                raise ProtocolError("unsolicited record began before the next command")
            if isinstance(command, Intervention):
                intervention(session, command, "operator", ceiling, request_action)
                return
            if command.delay:
                time.sleep(min(command.delay, max(0, ceiling - time.monotonic())))
            deadline = min(ceiling, time.monotonic() + limits["command_timeout"])
            if budget and time.monotonic() >= ceiling:
                raise (ProtocolError("soak restoration allowance expired") if command.cleanup else
                       SoakDuration("soak primary duration expired before write"))
            if budget:
                session.attempting()
                session.unconfirmed_command = command.text
            port.send(command.text, deadline)
            if budget:
                session.unconfirmed_command = None
            session.issue()
            try:
                exchange.receive(deadline, session.accept, lambda: len(session.results) == session.issued)
            except ReplyTimeout as error:
                timeout = error
                raise
        if budget:
            run_soak_steps(session, budget, execute, exchange.decoder, issues)
        else:
            for command in live_steps(session):
                if command.cleanup and cleanup_start is None:
                    cleanup_start = time.monotonic()
                    ceiling = min(cleanup_ceiling, cleanup_start + limits["cleanup_timeout"] * 0.75)
                execute(command, ceiling)
        if budget:
            ceiling = budget.close_start
        tail = time.monotonic() + limits["tail_timeout"]
        exchange.receive(min(ceiling, tail), session.accept)
        if tail > ceiling:
            raise SerialError("execution deadline clipped the receive tail")
        exchange.decoder.finish()
        session.finish()
    except KeyboardInterrupt:
        interrupted = True
        issues.append("interrupted by operator")
    except (ProtocolError, SerialError) as error:
        issues.append(str(error))
        if budget:
            if isinstance(error, SoakDuration):
                if "soak primary duration expired" not in issues:
                    issues.append("soak primary duration expired")
                session.campaign.stop("duration")
            else:
                session.campaign.stop("cancelled" if port.cancelled else "fault")
    finally:
        if cleanup_start is None:
            cleanup_start = time.monotonic()
        cleanup_end = min(cleanup_ceiling, cleanup_start + limits["cleanup_timeout"])
        recovery_end = min(cleanup_end, cleanup_start + limits["cleanup_timeout"] / 2)
        if budget:
            cleanup_end = min(cleanup_ceiling, cleanup_start + budget.cleanup_left + limits["cleanup_timeout"] / 4)
            recovery_end = min(budget.close_start, cleanup_start + budget.cleanup_left / 2)
        try:
            if timeout is not None and not (port.pending or port.framing_uncertain or port.cancelled or interrupted or
                                            (declaration and exchange.decoder.has_partial_record)):
                try:
                    exchange.resynchronize(session, recovery_end, recovery, timeout.late,
                                           require_boundary=bool(declaration))
                except (ProtocolError, SerialError, KeyboardInterrupt) as error:
                    interrupted |= isinstance(error, KeyboardInterrupt)
                    recovery.update(state="failed", detail=str(error) or "interrupted by operator")
        finally:
            closed = port.close(cleanup_end)
            if closed.detail:
                issues.append(f"serial close {closed.state}: {closed.detail}")
    if exchange.truncated and "serial capture limit reached" not in issues:
        issues.append("serial capture limit reached")
    interrupted |= port.cancelled
    if budget and interrupted and "interrupted by operator" not in issues:
        issues.append("interrupted by operator")
    complete = not issues and not interrupted
    if budget and not complete and session.campaign.summary()["stop_reason"] != "duration":
        session.campaign.stop("cancelled" if interrupted else "fault")
    data = {**base, "ended_utc": utc_now(), "complete": complete,
            "commands": [row["command"] for row in port.writes if row["state"] == "ok"],
            **case_evidence(args.suite, session), "identity": session.identity,
            "issues": list(issues), "serial": receipt(complete)}
    if budget:
        data.update(budget.evidence())
    final_deadline = min(end, time.monotonic() + limits["finalize_timeout"])

    def finalize():
        try:
            files.write(bytes(exchange.capture))
            if exchange.truncated:
                files.mark_truncated()
            return files.finish(data, final_deadline)
        finally:
            files.abort()

    finalized = bounded_call(finalize, final_deadline)
    if finalized.pending:
        bounded_call(files.cancel_publication, time.monotonic() + 0.05)
    if finalized.state != "ok":
        issues.append(f"evidence finalization {finalized.state}: {finalized.detail}")
        if budget:
            issues.append(finalization_diagnostic(data))
    exit_reason = port.exit_reason or ("an operator console worker is pending" if console_pending else "")
    if exit_reason or finalized.pending:
        serial_hard_exit(issues, exit_reason or "an evidence finalization worker is pending", end)
    if finalized.state != "ok":
        if interrupted or finalized.cancelled:
            raise KeyboardInterrupt("; ".join(issues))
        raise EvidenceError("; ".join(issues))
    return finalized.value, interrupted


def verify_soak(data, transcript):
    """Rebuild the admitted prefix, including one unanswered or possibly transmitted command."""
    soak, campaign = data["soak"], data["campaign"]
    expected = expectations(data["expectations"], "campaign")
    commands = list(data["commands"])
    uncertain = soak["unconfirmed_command"]
    serial = data.get("serial")
    recovery_nonce = None
    recovery_rows = []
    recovery_parse_error = None
    if serial:
        writes = serial["writes"]
        recovery = serial["recovery"]
        if recovery["state"] == "not_started" and (recovery["identity"] or recovery["detail"]):
            raise EvidenceError("recovery evidence without a recovery attempt")
        if recovery["state"] != "not_started" and len(writes) > 1 and writes[-1]["command"].startswith("session "):
            nonce = writes[-1]["command"][8:]
            if not TOKEN.fullmatch(nonce) or writes[-1]["command"] == writes[0]["command"]:
                raise EvidenceError("invalid recovery nonce")
            if writes[-1]["state"] == "ok":
                recovery_nonce = nonce
                commands.pop()
            writes = writes[:-1]
        failed_write = writes[-1]["command"] if writes and writes[-1]["state"] != "ok" else None
        claims_reply = recovery["state"] in ("continuous", "unproven") or bool(recovery["identity"])
        if ((recovery["state"] != "not_started" and (not writes or failed_write)) or
                (claims_reply and not recovery_nonce)):
            raise EvidenceError("serial recovery lacks its confirmed writes")
        if failed_write != uncertain:
            raise EvidenceError("unconfirmed soak command differs from serial write receipt")
    first = commands[0] if commands else uncertain or "session unissued"
    if not first.startswith("session "):
        raise EvidenceError("soak does not start with its nonce handshake")
    session = CampaignSession(first.split(" ", 1)[1], expected, campaign["declaration"], soak=soak["plan"])
    decoder = Decoder(max_bytes=soak["plan"]["max_bytes"])
    setup = SetupReplay(session)
    setup_rows = soak["setup"]
    planned_setup = list(session.campaign.baseline)
    if [row["command"] for row in setup_rows] != planned_setup[:len(setup_rows)]:
        raise EvidenceError("setup commands differ from the compiled case baseline")
    received = soak["received_records"]
    interventions = campaign["interventions"]

    def apply_interventions():
        while True:
            used = len(session.campaign.interventions)
            if (session.cleanup_boundary is None and soak.get("cleanup_boundary") ==
                    {"records": session.received_records, "interventions": used}):
                if not session.identity or session.issued != len(session.results) or session.next is None:
                    raise EvidenceError("primary expiry is outside a clean command boundary")
                session.expire_primary()
            if not isinstance(session.next, Intervention) or used == len(interventions):
                return
            session.intervene(interventions[used])

    def issue():
        apply_interventions()
        step = session.next
        if not isinstance(step, Command) or commands[session.issued] != step.text:
            raise EvidenceError("outbound soak prefix differs from the admitted step")
        session.attempting()
        session.issue()

    def accept(record):
        if (not session.identity and record.fields.get("nonce") != session.nonce and
                setup.index < len(setup_rows) and setup_rows[setup.index]["state"] == "terminal"):
            if setup.accept(record):
                return
        if not data["complete"] and session.received_records >= received:
            if recovery_nonce:
                recovery_rows.append(record)
            return  # Retained after timeout, serial recovery, or while reaping the child.
        if session.issued == len(session.results) and session.issued < len(commands):
            issue()
        session.accept(record)

    try:
        decode_chunk(decoder, accept, transcript)
        decoder.finish()
        if data["complete"]:
            apply_interventions()
            session.finish()
    except ProtocolError as error:
        if data["complete"] or (serial and recovery["state"] in ("continuous", "unproven")):
            raise
        if recovery_nonce and session.received_records >= received:
            recovery_parse_error = str(error)
    if session.received_records != received:
        raise EvidenceError("retained transcript does not contain the consumed record prefix")
    if session.issued == len(session.results) and session.issued < len(commands):
        issue()
    apply_interventions()
    if (session.issued != len(commands) or session.campaign.interventions != interventions or
            session.cleanup_boundary != soak.get("cleanup_boundary")):
        raise EvidenceError("outbound soak command or intervention has no matching admission")
    if uncertain:
        if (session.issued != len(session.results) or not isinstance(session.next, Command) or
                session.next.text != uncertain):
            raise EvidenceError("uncertain write differs from the pending command")
        session.attempting()
    if recovery_nonce and (recovery["state"] != "failed" or recovery["identity"]):
        if session.issued != len(session.results) + 1 or serial["framing_uncertain"]:
            raise EvidenceError("recovery requires a fully written unanswered request")
        replayed = {"state": "failed", "identity": {}, "detail": ""}
        try:
            for row in recovery_rows:
                accept_recovery(row, session, recovery_nonce, replayed)
            if recovery_parse_error:
                replayed.update(state="failed", detail=recovery_parse_error)
        except ProtocolError as error:
            replayed.update(state="failed", detail=str(error))
        if json.dumps(replayed, sort_keys=True) != json.dumps(recovery, sort_keys=True):
            raise EvidenceError("serial recovery differs from the correlated nonce reply")
    if not data["complete"] and data["ended_utc"]:
        if "soak primary duration expired" in data["issues"]:
            session.campaign.stop("duration")
        elif "interrupted by operator" in data["issues"]:
            session.campaign.stop("cancelled")
        else:
            session.campaign.stop("fault")
    summary = session.campaign.summary()
    # JSON distinguishes boolean values from numeric register words.
    if (json.dumps(summary, sort_keys=True) != json.dumps({k: soak[k] for k in summary}, sort_keys=True) or
            session.campaign.cases() != data["cases"] or session.identity != data["identity"]):
        raise EvidenceError("soak iterations, telemetry, stop or identity differ from the correlated evidence")
    terminal_setup = [row for row in setup_rows if row["state"] == "terminal"]
    if (setup.index != len(terminal_setup) or
            (data["verdict"] == "PASS" and expected["backend"] == "sim-mdio" and
             len(terminal_setup) != len(planned_setup))):
        raise EvidenceError("compiled setup lacks its terminal evidence")


def verify(path):
    """Verify the pair and replay complete evidence with the same protocol and assertions."""
    data, transcript = verify_artifacts(path)
    if "soak" in data:
        verify_soak(data, transcript)
    elif data["complete"]:
        commands = data["commands"]
        if not commands or not commands[0].startswith("session "):
            raise EvidenceError("complete evidence has no session command")
        nonce = commands[0][len("session "):]
        suite = data["suite"]
        expected = expectations(data["expectations"], suite)
        campaign = data.get("campaign")
        session = make_session(suite, nonce, expected, campaign["declaration"] if campaign else None,
                               campaign["interventions"] if campaign else None)
        if campaign is None:
            session.issued = len(session.commands)
        decoder = Decoder(max_bytes=data["serial"]["max_bytes"] if data["scope"] == SERIAL_SCOPE
                          else MAX_TRANSCRIPT_BYTES)
        for index in range(0, len(transcript), 4096):
            decode_chunk(decoder, session.accept, transcript[index:index + 4096])
        decoder.finish()
        session.finish()
        if commands != [command.text for command in session.commands]:
            raise EvidenceError("recorded commands differ from the known suite")
        if session.identity != data["identity"] or case_evidence(suite, session)["cases"] != data["cases"]:
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
        ("run-serial", "attach once to an explicitly qualified running serial console"),
    ):
        command = modes.add_parser(name, help=description)
        command.add_argument("--suite", choices=(*SUITES, "campaign"), required=True)
        command.add_argument("--case", choices=all_cases(), help="one case for the campaign suite")
        command.add_argument("--soak-count", type=int, help="explicit number of campaign iterations")
        command.add_argument("--soak-duration-ms", type=int, help="single primary window for all iterations")
        command.add_argument("--max-bytes", type=byte_limit, default=MAX_TRANSCRIPT_BYTES)
        command.add_argument("--operating-mode", choices=("basic", "enhanced", "none"))
        command.add_argument("--polling", choices=("exclusive", "concurrent"))
        command.add_argument("--board", help="identified board or fixture")
        command.add_argument("--authorization", help="record authorizing this exact attachment or campaign")
        command.add_argument("--peer", default="", help="identified compatible link partner")
        command.add_argument("--peer-profile", default="", help="declared peer profile or open topology")
        command.add_argument("--cable", default="", help="identified copper cable")
        command.add_argument("--power-record", default="", help="authorized power/boot and board-wait record")
        command.add_argument("--previous-boot", type=int, help="prior session boot cookie on this board")
        command.add_argument("--intervention-timeout-ms", type=int, default=10_000)
        command.add_argument("--reinitialize", action="store_true",
                             help="authorize cleanup-only no-reset initialization of the admitted profile")
        if name != "run-serial":
            command.add_argument("--nonce", required=name == "verify-transcript")
        for key in TARGET_FIELDS + OPTIONAL_FIELDS:
            required = key in TARGET_FIELDS or (name == "run-serial" and key in ("baud", "console"))
            command.add_argument("--" + key.replace("_", "-"), required=required,
                                 type=int if key in ("address", "baud") else str)
        if name != "dry-run":
            if name != "run-serial":
                command.add_argument("path", type=Path, help="raw transcript or compiled fixture path")
            command.add_argument("--out-dir", type=Path, required=True)
            command.add_argument("--run", required=True, help="exclusive output basename")
        if name == "run-fixture":
            command.add_argument("--timeout", type=positive_seconds, default=5.0)
            command.add_argument("--closure", type=positive_seconds, default=0.5)
            command.add_argument("--fixture-identity", help="explicit simulated identity reported by the host fixture")
            command.add_argument("--fixture-variant", choices=sorted(VARIANTS),
                                 help="part bound by the compiled fixture (default DP83826I); --variant checks its output")
            command.add_argument("--fixture-rmii-role", choices=("leader", "follower"),
                                 help="declared and simulated DP83825I board role, checked against --rmii-role")
            command.add_argument("--fixture-deny-maintenance", action="store_true",
                                 help="exercise the compiled fixture's denied mutation route")
        elif name == "verify-transcript":
            command.add_argument("--interventions", type=Path, help="bounded JSON array of recorded intervention receipts")
        elif name == "run-serial":
            command.add_argument("--port", required=True, help="explicit local COM port or /dev tty path")
            command.add_argument("--reset-silent", action="store_true",
                                 help="declare qualified open/close without reset on this exact setup")
            command.add_argument("--clean-input", action="store_true",
                                 help="declare exclusive access beginning at a clean CLI input line")
            command.add_argument("--dry-run", action="store_true", help="validate and print without I/O")
            for name, default, purpose in (
                ("session-timeout", 30.0, "whole session budget"),
                ("duration", 20.0, "active setup and command budget"),
                ("command-timeout", 1.5, "single write and response budget"),
                ("cleanup-timeout", 3.0, "resynchronization and close budget"),
                ("finalize-timeout", 2.0, "reserved evidence publication budget"),
                ("tail-timeout", 0.15, "final receive observation interval"),
            ):
                command.add_argument("--" + name, type=positive_seconds, default=default,
                                     help=f"{purpose} in seconds (default {default:g})")
    modes.add_parser("verify-artifacts", help="read-only verification of an existing evidence pair").add_argument(
        "path", type=Path, help="path to the run JSON")
    return cli


def campaign_arguments(args):
    args.soak = None
    repeat = args.soak_count is not None or args.soak_duration_ms is not None
    if args.suite != "campaign":
        if (repeat or any((args.case, args.operating_mode, args.polling, args.peer, args.peer_profile, args.cable,
                 args.power_record, args.reinitialize, getattr(args, "interventions", None),
                 getattr(args, "fixture_deny_maintenance", False))) or args.previous_boot is not None or
                args.intervention_timeout_ms != 10_000):
            raise ProtocolError("campaign options require --suite campaign")
        return None, None
    declared = {"case": args.case, "mode": args.operating_mode, "polling": args.polling, "board": args.board,
                "authorization": args.authorization, "peer": args.peer, "peer_profile": args.peer_profile,
                "cable": args.cable, "power_record": args.power_record, "previous_boot": args.previous_boot,
                "intervention_timeout_ms": args.intervention_timeout_ms, "reinitialize": args.reinitialize}
    validate_declaration(declared)
    if repeat:
        args.soak = soak_plan(args.soak_count, args.soak_duration_ms,
                              getattr(args, "max_bytes", MAX_TRANSCRIPT_BYTES), declared)
    recorded = None
    if args.mode == "verify-transcript":
        recorded = []
        if args.interventions:
            def unique(pairs):
                values = dict(pairs)
                if len(values) != len(pairs):
                    raise ProtocolError("duplicate intervention JSON fields")
                return values

            if not args.interventions.is_file():
                raise ProtocolError("intervention receipts require a regular file")
            with args.interventions.open("rb") as stream:
                limit = 4096 * (args.soak["count"] if args.soak else 1)
                content = stream.read(limit + 1)
            if len(content) > limit:
                raise ProtocolError(f"intervention receipts exceed {limit} bytes")
            try:
                recorded = json.loads(content, object_pairs_hook=unique)
            except (ValueError, RecursionError) as error:
                raise ProtocolError(f"invalid intervention JSON: {error}") from error
            if type(recorded) is not list or len(recorded) > MAX_INTERVENTIONS * (args.soak_count or 1):
                raise ProtocolError("invalid intervention receipt count")
            for row in recorded:
                validate_intervention(row, declared)
    return declared, recorded


def preview_plan(args, expected, nonce, declaration, **details):
    initial = make_session(args.suite, nonce, expected, declaration, soak=args.soak)
    result = {"scope": "serial_session" if args.mode == "run-serial" else "host_protocol", "executed": False,
              "suite": args.suite, "expectations": expected, "commands": [step.text for step in initial.commands]}
    if args.soak:
        result["soak"] = args.soak
    if declaration:
        spec = initial.campaign.spec
        result["campaign"] = {"declaration": declaration, "group": spec.group, "equipment": spec.fixture,
                              "peer_profile": spec.peer, "target": spec.target.apply() if spec.target else None,
                              "reset": spec.reset or None, "mutates_phy": spec.mutation,
                              "command_ceiling": MAX_COMMANDS, "conditional": True}
    print(json.dumps({**result, **details}, indent=2))


def report_exit(code, status, issues, bounded):
    """A blocked terminal cannot keep an executed soak alive after evidence publication."""
    def emit():
        if status:
            print(status, flush=True)
        for issue in issues:
            print(issue, file=sys.stderr, flush=True)

    if bounded:
        result = bounded_call(emit, time.monotonic() + 0.05)
        if result.pending:
            os._exit(code)
    else:
        emit()
    return code


def main(argv=None, *, serial_factory=None):
    """Return 0 for PASS, 1 for FAIL, 2 if incomplete, and 130 if interrupted."""
    cli = parser()
    args = cli.parse_args(argv)
    interrupted = False
    label = "serial session" if args.mode == "run-serial" else "console evidence"
    try:
        if args.mode == "verify-artifacts":
            data = verify(args.path)
        else:
            expected = expectations({key: getattr(args, key) for key in TARGET_FIELDS + OPTIONAL_FIELDS
                                     if getattr(args, key) is not None}, args.suite)
            preview = args.mode == "dry-run" or getattr(args, "dry_run", False)
            nonce = getattr(args, "nonce", None)
            if nonce is None:
                nonce = "preview" if preview else secrets.token_hex(12)
            declaration, recorded = campaign_arguments(args)
            if args.mode == "run-fixture":
                if (expected["target"], expected["backend"]) != ("host", "sim-mdio"):
                    raise ProtocolError("compiled fixture must expect host sim-mdio")
                if not 0 < args.closure < args.timeout or args.closure > 5:
                    raise ProtocolError("closure must be at most 5 seconds and shorter than the total timeout")
                if args.fixture_identity is not None and not TOKEN.fullmatch(args.fixture_identity):
                    raise ProtocolError("invalid simulated fixture identity")
            if args.mode == "run-serial":
                settings = serial_settings(args, expected)
                validate_destination(args.run, args.max_bytes)
                if preview:
                    preview_plan(args, expected, nonce, declaration, serial=settings,
                                 destination={"directory": str(args.out_dir), "run": args.run})
                    return 0
                if serial_factory is not None:
                    settings["backend"] = INJECTED_BACKEND
                data, interrupted = capture_serial(args, nonce, expected, settings, serial_factory or native_port,
                                                   declaration)
            else:
                if preview:
                    preview_plan(args, expected, nonce, declaration)
                    return 0
                data, interrupted = capture(args, args.suite, nonce, expected, declaration, recorded)
    except KeyboardInterrupt as error:
        return report_exit(130, None, [str(error) or "interrupted"], bool(getattr(args, "soak", None)))
    except (OSError, ProtocolError, FixtureError, EvidenceError, SerialError) as error:
        return report_exit(2, None, [f"INCONCLUSIVE ({label}): {error}"], bool(getattr(args, "soak", None)))
    verdict = data["verdict"]
    label = "serial session" if data["scope"] == SERIAL_SCOPE else "host protocol"
    code = 130 if interrupted else {"PASS": 0, "FAIL": 1}.get(verdict, 2)
    return report_exit(code, f"{verdict} ({label})", data["issues"], "soak" in data)


if __name__ == "__main__":
    raise SystemExit(main())
