"""DP8382x campaign plans and assertions shared by every runner backend."""

from dataclasses import dataclass, replace
from functools import cache
import ipaddress

if __package__:
    from .dp8382x_hil_protocol import (
        COMMON_FIELDS, DP83822, PHY_ID2, TERMINAL_STATUS, U32_MAX, U64_MAX, ProtocolError, identity, integer,
    )
    from .dp8382x_hil_serial import fixture_identifier
else:
    from dp8382x_hil_protocol import (
        COMMON_FIELDS, DP83822, PHY_ID2, TERMINAL_STATUS, U32_MAX, U64_MAX, ProtocolError, identity, integer,
    )
    from dp8382x_hil_serial import fixture_identifier


MAX_COMMANDS = 64
MAX_INTERVENTIONS = 4
I32 = (-2**31, 2**31 - 1)
BIT = ("0", "1")
WORD = (0, 65535)
SPEED = ("0", "10", "100")
DUPLEX = ("unknown", "half", "full")
NEGOTIATION = ("unknown", "disabled", "in_progress", "complete")
TRUST = ("UNKNOWN", "DIRTY", "VERIFIED")
OBSERVATION_CHANGED = 6  # BusyDetail::OBSERVATION_CHANGED in Status.h
TECHNOLOGIES = {1: (10, "half"), 2: (10, "full"), 4: (100, "half"), 8: (100, "full")}
BASIC = "p4-netlist-basic"
OWNER = "P4-raw-frame"


@dataclass(frozen=True)
class Command:
    """Known input line and its allowed response form."""

    text: str
    form: str = "terminal"
    cleanup: bool = False
    delay: float = 0

    @property
    def name(self):
        return ".".join(self.text.split()[:2]) if not self.text.startswith("session ") else "session"


@dataclass(frozen=True)
class Profile:
    mode: str
    modes: int
    speed: int
    duplex: str
    pause_sym: int = 0
    pause_asym: int = 0
    pair: str = "auto"
    flags: int = 0

    def apply(self):
        if self.mode == "an":
            pause = ("none", "asymmetric", "symmetric", "both")[2 * self.pause_sym + self.pause_asym]
            return f"configuration apply an {self.modes} {pause} {self.pair} {self.flags}"
        return f"configuration apply forced {self.speed} {self.duplex} {self.pair} {self.flags}"

    @classmethod
    def read(cls, fields):
        return cls(fields["mode"], int(fields["modes"]), int(fields["speed_mbps"]), fields["duplex"],
                   int(fields["pause_sym"]), int(fields["pause_asym"]), fields["pair"], int(fields["flags"]))

    def replayable(self, robust_optional=False):
        if self.mode == "an":
            return self.modes > 0 and (self.speed, self.duplex) == (0, "unknown") and self.pair != "unknown"
        return (self.mode == "forced" and self.modes == self.pause_sym == self.pause_asym == 0 and
                self.pair != "unknown" and self.duplex in ("half", "full") and self.speed in (10, 100) and (robust_optional or self.flags & 1) and
                (self.speed == 100 or not self.flags & 4))


def an(mask, sym=0, asym=0):
    return Profile("an", mask, 0, "unknown", sym, asym)


@dataclass(frozen=True)
class Case:
    """A single explicitly selected case with an independent artifact verdict."""

    id: str
    group: str
    fixture: str
    target: Profile | None = None
    peer: str = ""
    resolved: int = 0
    partner_modes: int = 0
    partner_pause: int = 0
    reset: str = ""
    fault: str = ""
    policy: int = 0
    feature_mutation: bool = False

    @property
    def mutation(self):
        return bool(self.reset or self.target or self.feature_mutation)


CASES = {
    row.id: row for row in (
        Case("observe", "observation", "board and exclusive management owner"),
        Case("cold-boot", "observation", "authorized power/boot record and prior boot"),
        Case("int-pin", "observation", "wired INT pin and identified edge instrument"),
        Case("owned-apply", "configuration", "board in maintenance", an(9, 1)),
        Case("pcs", "configuration", "board in maintenance", reset="pcs"),
        Case("hard", "configuration", "board in maintenance", reset="hard"),
        Case("restart", "configuration", "board in maintenance", reset="restart"),
        Case("10base-te", "modes", "10 Mbit/s AN peer", an(1), "an-10-half", 1, 1),
        Case("100-full-only", "modes", "configurable AN peer with asymmetric pause",
             an(8, 1, 1), "an-100-full-asym", 8, 8, 2),
        Case("absent-peer", "faults", "declared open copper connection", an(8), "open", fault="down"),
        Case("wrong-policy", "faults", "AN peer restricted to 10/half", an(9), "an-10-half", 1, 1,
             fault="wrong_policy", policy=8),
        Case("no-common", "faults", "AN-only 10/half peer without fallback",
             an(8), "an-10-half", fault="no-common"),
        Case("unplug-replug", "faults", "100/full AN peer and removable cable",
             an(8), "an-100-full", 8, 8, fault="unplug"),
        Case("peer-reset", "faults", "100/full AN peer with controlled reset hold/release",
             an(8), "an-100-full", 8, 8, fault="hold-peer-reset"),
        Case("bounded-recovery", "faults", "100/full AN peer with observable reacquisition",
             an(8), "an-100-full", 8, 8, fault="unplug"),
    )
}
for mask, (speed, duplex) in TECHNOLOGIES.items():
    name = f"{speed}-{duplex}"
    CASES["an-" + name] = Case("an-" + name, "modes", "AN peer with matching restricted modes",
                              an(mask), "an-" + name, mask, mask)
    CASES["forced-" + name] = Case("forced-" + name, "modes", "matched forced copper peer",
                                  Profile("forced", 0, speed, duplex, flags=1), "forced-" + name, mask)

RECOVERY_ACTIONS = {"unplug": "replug", "hold-peer-reset": "release-peer-reset"}
FIXTURE_ACTIONS = {
    "bist-errors": "$inject-bist-errors",
    "calibration-flag": "$raise-calibration",
    "inject-events": "$inject-events",
    "inject-wake": "$inject-wake",
}
DECLARATION_FIELDS = {"case", "board", "authorization", "mode", "polling", "peer", "peer_profile", "cable",
                      "power_record", "previous_boot", "intervention_timeout_ms", "reinitialize"}


@cache
def feature_modules():
    """Load procedure families after the shared campaign primitives are defined."""
    if __package__:
        from . import dp8382x_hil_controls as controls, dp8382x_hil_diagnostics as diagnostics
    else:
        import dp8382x_hil_controls as controls
        import dp8382x_hil_diagnostics as diagnostics
    return {name: (group, module) for group, module in (("diagnostics", diagnostics), ("controls", controls))
            for name in module.CASES}


@cache
def all_cases():
    """Return the observation and feature cases accepted by live and replay entry points."""
    return {**CASES, **{
        name: Case(name, group, module.CASES[name][0], feature_mutation=module.CASES[name][1])
        for name, (group, module) in feature_modules().items()
    }}


def validate_declaration(values):
    """Check admission inputs before I/O and again during evidence replay."""
    if (type(values) is not dict or values.keys() != DECLARATION_FIELDS or
            type(values["case"]) is not str or values["case"] not in all_cases() or
            values["mode"] not in ("basic", "enhanced", "none") or values["polling"] not in ("exclusive", "concurrent") or
            not all(fixture_identifier(values[key]) for key in ("board", "authorization")) or
            any(values[key] != "" and not fixture_identifier(values[key])
                for key in ("peer", "peer_profile", "cable", "power_record")) or
            type(values["intervention_timeout_ms"]) is not int or
            not 1 <= values["intervention_timeout_ms"] <= 20_000 or type(values["reinitialize"]) is not bool or
            (values["previous_boot"] is not None and
             (type(values["previous_boot"]) is not int or not 0 <= values["previous_boot"] <= U32_MAX))):
        raise ProtocolError("invalid campaign fixture declaration")
    return all_cases()[values["case"]]


@dataclass(frozen=True)
class Intervention:
    action: str
    cleanup: bool


def validate_intervention(row, declaration):
    """Validate bounded declarations before binding them to a particular request."""
    if (type(row) is not dict or row.keys() != {
            "action", "source", "outcome", "after_req", "boot", "elapsed_ms", "budget_ms", "cleanup"} or
            row["action"] not in (*RECOVERY_ACTIONS, *RECOVERY_ACTIONS.values(), *FIXTURE_ACTIONS) or
            row["source"] not in ("operator", "compiled-fixture") or
            (row["action"] in FIXTURE_ACTIONS and row["source"] != "compiled-fixture") or
            row["outcome"] not in ("acknowledged", "declined", "timed_out", "failed") or
            type(row["cleanup"]) is not bool or
            any(type(row[key]) is not int for key in ("after_req", "boot", "elapsed_ms", "budget_ms")) or
            not 1 <= row["after_req"] <= U64_MAX or not 0 <= row["boot"] <= U32_MAX or
            not 0 <= row["elapsed_ms"] <= row["budget_ms"] <= declaration["intervention_timeout_ms"] or
            (row["outcome"] == "timed_out") != (row["elapsed_ms"] == row["budget_ms"])):
        raise ProtocolError("invalid campaign intervention receipt")


def payload(record):
    """Return command fields without the common record envelope."""
    return {key: value for key, value in record.fields.items() if key not in COMMON_FIELDS}


def exact_fields(fields, checks):
    """Require every supplied field once and validate each value."""
    if fields.keys() != checks.keys():
        raise ProtocolError("missing or unexpected command fields")
    for name, allowed in checks.items():
        value = fields[name]
        if isinstance(allowed[0], int):
            integer(value, *allowed)
        elif value not in allowed:
            raise ProtocolError(f"invalid {name}: {value}")


def clean(record):
    """Check a successful command with no independent cleanup failure."""
    return (record.primary, record.fields["detail"], record.fields["cleanup"],
            record.fields["reason"]) == ("OK", "0", "none", "none")


class AssertionFailed(Exception):
    """A correlated command or intervention supplied a nonpassing result."""

    def __init__(self, message, record=None, verdict="FAIL"):
        super().__init__(message)
        self.record = record
        self.verdict = verdict


def require(condition, record, message, verdict="FAIL"):
    if not condition:
        raise AssertionFailed(message, record, verdict)


def feature_fields(record, checks, verdict="FAIL"):
    """Check the payload of an already correlated reply without losing its cleanup route."""
    try:
        exact_fields(payload(record), checks)
    except ProtocolError as error:
        raise AssertionFailed(f"{record.cmd} {error}", record, verdict) from error


def matches(fields, **expected):
    return all(fields.get(key) == str(value) for key, value in expected.items())


def link_changed(record):
    """Recognize the native owner's exact inconsistent-sample terminal."""
    return matches(record.fields, primary="BUSY", detail=OBSERVATION_CHANGED, cleanup="none", reason="none")


def bits(names):
    return dict.fromkeys(names.split(), BIT)


CONFIG = {"valid": BIT, "stale": BIT, "trust": TRUST, "mode": ("unknown", "an", "forced"),
          "modes": (0, 15), "speed_mbps": SPEED, "duplex": DUPLEX, "pause_sym": BIT, "pause_asym": BIT,
          "pair": ("unknown", "auto", "mdi", "mdix"), "flags": (0, 7)}
DEVICE = {**bits("id1_valid id2_valid decoded_valid stale"), "id1": WORD, "id2": WORD, "oui": (0, 2**22-1),
          "model": (0, 63), "revision": (0, 15), "mode": ("basic", "enhanced", "unknown"),
          "identity_state": ("accepted", "rejected", "unassessed"),
          "identity_status": tuple(TERMINAL_STATUS), "identity_detail": I32}
LINK = {**bits("valid stale up resolved pause_valid pause_rx pause_tx observed_valid advertised_valid partner_valid"),
        "observation": ("up", "down", "pending", "wrong_policy", "unresolved"),
        "speed_mbps": SPEED, "duplex": DUPLEX, "negotiation": NEGOTIATION, "observed_us": (0, U64_MAX),
        "policy_mask": (0, 15), "advertised_modes": (0, 15), "partner_modes": (0, 15),
        **dict.fromkeys("bmsr_first bmsr_current anar anlpar aner".split(), WORD),
        "last_status": ("OK",), "last_detail": (0, 0), "round": (0, 0)}


class Campaign:
    """Advance one bounded case only after its typed evidence is available."""

    def __init__(self, declaration, expected):
        self.spec = validate_declaration(declaration)
        self.feature = feature_modules().get(self.spec.id, (None, None))[1]
        self.baseline = getattr(self.feature, "BASELINES", {}).get(self.spec.id, ())
        self.declaration = declaration
        self.expected = expected
        self.backend = expected["backend"]
        self.p22 = expected["variant"] in DP83822
        self.p25 = expected["variant"] == "DP83825I"
        if self.p25 and self.spec.id not in getattr(self.feature, "P25_CASES", ()):
            self.baseline = ()
        if (declaration["mode"] == "none") != (self.p25 or self.p22):
            raise ProtocolError("operating mode disagrees with declared register map")
        self.a_feature = self.feature is not None and expected["variant"] in ("DP83826AI", "DP83826AE")
        if self.p25 and self.spec.id == "power-deep":
            self.spec = replace(self.spec, feature_mutation=True)
        self.part_control = None
        if self.a_feature and self.spec.group == "controls":
            self.part_control = self.feature.a_control(self.spec.id, declaration["mode"])
        elif self.p25 and self.spec.group == "controls" and self.spec.id in self.feature.P25_CASES:
            self.part_control = self.feature.p25_control(self.spec.id, expected["rmii_role"])
        if self.part_control:
            self.baseline = (self.part_control.baseline.command,)
        self.primary_record = None
        self.interventions = []
        self.last = None
        self.session = None
        self.restoration = "NOT RUN"
        self.failed = None
        self.cleanup_failure = ""
        self.success = False
        self.may_change = False
        self.peer_may_change = False
        self.last_observed = 0
        self.active = self.execute()
        self.next = next(self.active)

    def feature_gate(self):
        """Return the prerequisite that prevents the selected feature from running."""
        if self.p22:
            return "DP83822 feature campaign needs its exact-part baseline and verifier"
        if self.p25 and self.spec.id not in getattr(self.feature, "P25_CASES", ()):
            return "No DP83825I campaign baseline and verifier for this procedure"
        if (self.expected["target"], self.backend) != ("host", "sim-mdio"):
            return "qualified native feature grant and independent measurement route unavailable"
        rmii_supported = self.p25 and self.spec.id in ("collision-10", "collision-100", "quality")
        if self.spec.id in getattr(self.feature, "MII_CASES", ()) and self.expected["interface"] != "MII" and not rmii_supported:
            return "case requires the MII fixture"
        if self.spec.id in getattr(self.feature, "RMII_CASES", ()) and self.expected["interface"] != "RMII":
            return "case requires the RMII fixture"
        mode = getattr(self.feature, "MODES", {}).get(self.spec.id, self.declaration["mode"])
        if not self.p25 and self.declaration["mode"] != mode:
            return f"case requires {mode} operating mode"
        return None

    def advance(self, value):
        if isinstance(value, dict):
            step = self.next
            validate_intervention(value, self.declaration)
            if (not isinstance(step, Intervention) or len(self.interventions) >= MAX_INTERVENTIONS or
                    (value["action"], value["cleanup"], value["after_req"], value["boot"]) !=
                    (step.action, step.cleanup, self.last.req, self.last.boot) or
                    (value["source"] == "compiled-fixture") != (self.backend == "sim-mdio")):
                raise ProtocolError("intervention does not match the next admitted step")
            self.interventions.append(value)
        else:
            self.last = value
        self.next = self.active.send(value)

    def receipt(self):
        return {"declaration": self.declaration, "interventions": self.interventions}

    def cases(self):
        failure = self.failed
        record = self.primary_record if self.feature else self.last
        if failure:
            record = failure.record
        verdict = failure.verdict if failure else "PASS" if self.success else "INCONCLUSIVE"
        executed = record is not None and verdict not in ("NOT RUN", "BLOCKED")
        message = str(failure) if failure else "typed campaign assertions" if self.success else "campaign interrupted"
        if self.cleanup_failure:
            message += ". Cleanup: " + self.cleanup_failure
        elif self.may_change and self.restoration in ("NOT RUN", "INCONCLUSIVE"):
            message += ". Mutation effects are unresolved and restoration was not proved"
        return [{"id": self.spec.id, "executed": executed, "verdict": verdict,
                 "primary": {"code": record.primary, "detail": int(record.fields["detail"])} if executed else None,
                 "restoration": {"required": self.spec.mutation or self.spec.fault in RECOVERY_ACTIONS,
                                 "verdict": self.restoration}, "message": message}]

    def command(self, text, form="terminal", cleanup=False, delay=0):
        row = yield Command(text, form, cleanup, delay)
        if self.feature and not cleanup and clean(row):
            self.primary_record = row
        return row

    def read(self, text, checks=None, form="terminal", cleanup=False, delay=0):
        row = yield from self.command(text, form, cleanup, delay)
        require(clean(row), row, f"{text} did not complete cleanly")
        if checks is not None:
            exact_fields(payload(row), checks)
        return row

    def ready_health(self, cleanup=False):
        row = yield from self.read("cached health", cleanup=cleanup)
        fields = row.fields
        require(matches(fields, health="READY", initialized=1, consecutive=0, last_ok_valid=1) and
                0 < int(fields["successes"]) < U32_MAX and int(fields["failures"]) < U32_MAX and
                0 < int(fields["last_ok_us"]) <= int(fields["time_us"]) + 1 and
                (fields["last_error_valid"] == "0" or
                 0 < int(fields["last_error_us"]) <= int(fields["last_ok_us"])),
                row, "initialized healthy management evidence required")
        return row

    def slot(self, cleanup=False):
        row = yield from self.read("cached operation", cleanup=cleanup)
        require(matches(row.fields, slot="idle", token=0, slot_status="RESULT_NOT_AVAILABLE", slot_detail=0,
                        temporary=0, slot_deadline_us=0), row, "a clean idle slot is required")
        return row

    def transport(self, cleanup=False):
        row = yield from self.read("cached transport", cleanup=cleanup)
        fields = row.fields
        require(matches(fields, stopped=0, clock_invalid=0, controller_reusable=1) and
                0 < int(fields["batches"]) <= int(fields["completed"]) <= int(fields["attempted"]) < U32_MAX,
                row, "usable unsaturated management transport required")
        return row

    def application(self, cleanup=False):
        row = yield from self.command("application status", cleanup=cleanup)
        fields = payload(row)
        if self.backend == "sim-mdio":
            require(not fields and matches(row.fields, primary="UNSUPPORTED", detail=0, cleanup="none",
                                           reason="none", deadline_valid=0),
                    row, "host fixture must report the unavailable application route")
            return row
        require(clean(row), row, "application state unavailable")
        if self.backend == BASIC:
            address = fields.get("ip")
            try:
                ipaddress.IPv4Address(address)
            except (ValueError, TypeError):
                raise ProtocolError("invalid application IP field") from None
            exact_fields(fields, {**bits("installed traffic maintenance retired shutdown carrier"),
                                  "ip": (address,), "events": (0, U32_MAX)})
            available = matches(fields, installed=1, retired=0, shutdown=0)
            stopped = available and matches(fields, traffic=0, maintenance=1)
        else:
            exact_fields(fields, {**bits("ready restart maintenance txattempted retired"),
                                  "phase": ("unconfigured", "waiting", "ready", "active", "withdrawn", "maintenance"),
                                  **dict.fromkeys("rx rxd tx txd".split(), (0, U32_MAX)),
                                  **dict.fromkeys("gen rxexp rxother dma txdenied txbp txdown txfailed".split(),
                                                  (0, U64_MAX)), "txfirst": I32, "txsdk": I32})
            available = fields["retired"] == "0" and fields["phase"] not in ("unconfigured", "withdrawn")
            stopped = available and matches(fields, phase="maintenance", maintenance=1)
        running_boot = self.spec.id == "cold-boot" and self.declaration["polling"] == "concurrent"
        require(stopped or (available and running_boot), row,
                "application stop/fence and maintenance must be proved", "BLOCKED")
        return row

    def check_device(self, row):
        basic = self.declaration["mode"] == "basic"
        id2 = PHY_ID2[self.expected["variant"]][0 if basic or self.p25 or self.p22 else 1]
        require(matches(row.fields, id1_valid=1, id1=0x2000, id2_valid=1, id2=id2,
                        decoded_valid=1, oui=524328, model=(id2 >> 4) & 63, revision=id2 & 0xF,
                        mode="unknown" if self.p25 or self.p22 else self.declaration["mode"], identity_state="accepted", stale=0,
                        identity_status="OK", identity_detail=0), row,
                f"declared {self.expected['variant']} identity is not current")

    def check_role(self, row):
        follower = self.expected["rmii_role"] == "follower"
        require(matches(row.fields, sor1_valid=1, rcsr_valid=1, straps_stale=0) and
                bool(int(row.fields["sor1"]) & 8) == bool(int(row.fields["rcsr"]) & 128) == follower,
                row, "SOR1 and RCSR must both confirm the declared RMII role")

    def entry(self):
        first = yield from self.transport()
        local = yield from self.read("local board")
        require(identity(local, self.expected) == identity(self.session, self.expected), local,
                "local board and session identity disagree")
        caps = yield from self.read("capabilities")
        require(matches(caps.fields, valid=1, modes=15, interfaces=7 if self.p22 else 2 if self.p25 else 3, copper=1,
                        fiber=int(self.expected["variant"] in ("DP83822IF", "DP83822HF")), gigabit=0), caps,
                "current exact-part capability evidence required")
        identity_fields = DEVICE
        if self.p25:
            identity_fields = {**DEVICE, **bits("sor1_valid rcsr_valid straps_stale"), "sor1": WORD, "rcsr": WORD}
        device = yield from self.read("connection identity", identity_fields)
        if self.p25:
            self.check_role(device)
        self.check_device(device)
        health = yield from self.ready_health()
        slot = yield from self.slot()
        require(slot.fields["cleanup_owed"] == "0", slot, "pre-existing cleanup debt", "BLOCKED")
        settings = yield from self.read("cached settings")
        link = yield from self.read("cached link")
        app = yield from self.application()
        after = yield from self.transport()
        if self.declaration["polling"] == "exclusive":
            require(payload(first) == payload(after), after, "cached snapshot group changed whole-bus counters")
        return (after, local, caps, device, health, slot, settings, link, app)

    def passive(self, before):
        rejection = yield from self.command("registers write c22 24 65536")
        require(not payload(rejection) and matches(rejection.fields, primary="INVALID_PARAM", detail=0, reason="range",
                                                   cleanup="none", deadline_valid=0),
                rejection, "exact local write range rejection required")
        after = yield from self.entry()
        require(all(payload(left) == payload(right) for left, right in zip(before, after)), rejection,
                "rejection changed cached owner, transport, application or PHY evidence")

    def profile(self, scope, cleanup=False):
        schema = {**CONFIG, "flags": (0, 4095)} if self.p22 else CONFIG
        return (yield from self.read("configuration " + scope, schema, cleanup=cleanup))

    def profiles(self, cleanup=False, unavailable="FAIL"):
        desired = yield from self.profile("desired", cleanup)
        observed = yield from self.profile("observed", cleanup)
        candidate = Profile.read(desired.fields)
        expected_readback = candidate
        if self.p22:
            # Peer attestation lives with intent. Robust/fast MDIX also sets CR2[6].
            flags = candidate.flags & ~2048
            if flags & (1 | 64):
                flags |= 1024
            expected_readback = replace(candidate, flags=flags)
        require(all(matches(row.fields, valid=1, stale=0, trust="VERIFIED") for row in (desired, observed)) and
                expected_readback == Profile.read(observed.fields) and candidate.replayable(robust_optional=self.p25 or self.p22), observed,
                "complete, equal and VERIFIED desired and observed profiles required", unavailable)
        return candidate

    def config(self, text, changed, cleanup=False):
        row = yield from self.command(text, "operation", cleanup)
        if changed and not cleanup and row.op == 0:
            self.may_change = False
        require(clean(row), row, f"{text} did not complete cleanly")
        fields = row.fields
        require(matches(fields, core_status="OK", core_detail=0, sdk_error=0, mutation_completed=int(changed),
                        mutation_possible=int(changed), cleanup_owed=0, temporary=0, an_complete=0,
                        np_requested=0, np_complete=0,
                        np_owed=0, np_refused=0, np_base_event=0, np_base_valid=0, np_base=0, np_missing=0,
                        np_tx=0, np_rx=0, np_duplicates=0, np_words="none"),
                row, f"{text} lacks a complete effect and next-page receipt")

    def guard_feature_cleanup(self):
        """Recheck the stopped owner, usable bus and available operation slot before cleanup."""
        yield from self.application(True)
        yield from self.transport(True)
        row = yield from self.read("cached operation", cleanup=True)
        require(matches(row.fields, slot="idle", token=0, slot_status="RESULT_NOT_AVAILABLE",
                        slot_detail=0, slot_deadline_us=0), row, "cleanup route has an occupied slot")
        return row

    def prove_feature_exit(self, saved, entry_link):
        """Verify the saved profile, ordinary link state and released ownership after restoration."""
        # A failed feature restore cannot be repaired by verifying only the link settings.
        yield from self.config("configuration verify", False, True)
        require((yield from self.profiles(True)) == saved, self.last, "entry link profile was not restored")
        row = yield from self.link(saved, saved.modes, cleanup=True)
        keys = ("observation", "up", "resolved", "speed_mbps", "duplex", "negotiation",
                "partner_valid", "partner_modes", "pause_valid", "pause_rx", "pause_tx")
        require(matches(row.fields, **{key: entry_link.fields[key] for key in keys}), row,
                "ordinary link state differs from the entry observation")
        require((yield from self.slot(True)).fields["cleanup_owed"] == "0", self.last, "cleanup debt remains")
        yield from self.application(True)
        yield from self.transport(True)
        yield from self.ready_health(True)

    def run_feature(self, saved):
        """Keep the feature's primary result separate from independent exit proof."""
        entry_link = yield from self.link(saved, saved.modes)
        yield from self.feature.run(self, saved)
        if self.restoration == "PASS":
            self.restoration = "INCONCLUSIVE"
            try:
                yield from self.prove_feature_exit(saved, entry_link)
            except AssertionFailed as error:
                self.restoration = error.verdict
                self.cleanup_failure = str(error)
            else:
                self.restoration = "PASS"

    def reset(self, kind, cleanup=False):
        command = {"pcs": "recovery reset pcs", "hard": "recovery reset hard", "restart": "recovery restart",
                   "none": "connection initialize none"}[kind]
        row = yield from self.command(command, "operation", cleanup)
        if not cleanup and row.op == 0:
            self.may_change = False
        require(clean(row), row, f"{command} did not complete cleanly")
        reset = int(kind != "none")
        require(matches(row.fields, core_status="OK", core_detail=0, requested_reset=kind, effective_reset=kind,
                        reset_required=reset, reset_accepted=reset, reset_complete=reset, persistent_restored=1,
                        mutation_completed=1, mutation_possible=1, cleanup_owed=0, temporary=0, clock_invalid=0,
                        link_valid=0, sor1_valid=1, sor2_valid=0 if self.p25 else 1, straps_stale=0),
                row, f"{kind} reset/replay evidence incomplete")
        if self.p25:
            self.check_role(row)

    def vod(self):
        row = yield from self.read("registers read mmd 31 0x030E")
        require(matches(row.fields, address=0x030E, devad=31, value=0x4A40), row,
                "10BASE-Te VOD initialization readback must be DEVAD 31 offset 0x030E value 0x4A40")

    def link(self, profile, policy, *, cleanup=False, delay=0, partner=False, retry_changed=False):
        verb = "link partner" if partner else "link read"
        previous = int(self.last.fields["time_us"])
        row = yield from self.command(f"{verb} {policy}", cleanup=cleanup, delay=delay)
        f = row.fields
        if retry_changed and link_changed(row):
            exact_fields(payload(row), {
                **LINK, "observation": ("unavailable",), "last_status": ("BUSY",),
                "last_detail": (OBSERVATION_CHANGED, OBSERVATION_CHANGED),
            })
            require(matches(f, valid=0, stale=1, up=0, resolved=0, speed_mbps=0, duplex="unknown",
                            negotiation="unknown", pause_valid=0, pause_rx=0, pause_tx=0,
                            observed_valid=0, observed_us=0, policy_mask=policy, advertised_valid=0,
                            advertised_modes=0, partner_valid=0, partner_modes=0, bmsr_first=0,
                            bmsr_current=0, anar=0, anlpar=0, aner=0),
                    row, "inconsistent link sample supplied usable evidence")
            return row
        require(clean(row), row, f"{verb} {policy} did not complete cleanly")
        exact_fields(payload(row), LINK)
        at = int(f["observed_us"])
        current = int(f["bmsr_current"])
        envelope_time = int(f["time_us"])
        require(matches(f, valid=1, stale=0, observed_valid=1, policy_mask=policy) and
                max(previous, self.last_observed) < at <= envelope_time + 1 and at < int(f["deadline_us"]) and
                bool(current & 4) == (f["up"] == "1") and
                (f["resolved"] == "1" or matches(f, speed_mbps=0, duplex="unknown")) and
                (f["pause_valid"] == "1" or matches(f, pause_rx=0, pause_tx=0)), row, "live link is stale or incoherent")
        self.last_observed = at
        if profile.mode == "an":
            local = int(f["anar"])
            # SNLS638C table 7-7 requires the local remote-fault bit to be zero.
            local_mask = 0x2DFF if self.p25 or self.p22 else 0x0DFF
            remote_fault = 0x2000 if self.p22 and profile.flags & 32 else 0
            complete = bool(current & 32)
            require(matches(f, advertised_valid=1, advertised_modes=profile.modes,
                            negotiation="complete" if complete else "in_progress") and
                    (local & local_mask) == (1 | profile.modes << 5 | profile.pause_sym << 10 | profile.pause_asym << 11 | remote_fault),
                    row, "local advertisement or AN state differs from readback")
        else:
            require(matches(f, negotiation="disabled", advertised_valid=0, advertised_modes=0, partner_valid=0,
                            partner_modes=0, pause_valid=0, pause_rx=0, pause_tx=0),
                    row, "forced link exposes unqualified AN evidence")
        return row

    def link_down(self, row, profile, pending=False, allow_completed=False):
        f = row.fields
        mode = ("down", "pending") if pending else ("down",)
        negotiation = ("disabled",) if profile.mode == "forced" else ("in_progress",)
        if allow_completed and profile.mode == "an" and f["observation"] == "down":
            negotiation += ("complete",)
        require(f["observation"] in mode and matches(f, resolved=0, partner_valid=0, partner_modes=0,
                                                     pause_valid=0, pause_rx=0, pause_tx=0,
                                                     up=int(f["observation"] == "pending")) and f["negotiation"] in negotiation,
                row, "down or pending link must withhold resolved mode, partner ability and pause")

    def link_up(self, row, profile, wrong=False):
        f = row.fields
        spec = self.spec
        speed, duplex = TECHNOLOGIES[spec.resolved]
        require(matches(f, observation="wrong_policy" if wrong else "up", up=1, resolved=1,
                        speed_mbps=speed, duplex=duplex) and bool(int(f["policy_mask"]) & spec.resolved) != wrong,
                row, "resolved speed/duplex and policy evidence disagree")
        if profile.mode == "forced":
            return
        peer = int(f["anlpar"])
        expected_word = 1 | spec.partner_modes << 5 | spec.partner_pause << 10
        common = profile.modes & spec.partner_modes
        require(matches(f, negotiation="complete", partner_valid=1, partner_modes=spec.partner_modes) and
                int(f["aner"]) & 0x11 == 1 and (peer & 0x0DFF) == expected_word and
                bool(common) and (1 << (common.bit_length() - 1)) == spec.resolved,
                row, "qualified partner ability does not account for the resolved mode")
        asym = bool(profile.pause_asym and spec.partner_pause & 2)
        sym = bool(profile.pause_sym and spec.partner_pause & 1)
        rx, tx = sym or (asym and profile.pause_sym), sym or (asym and spec.partner_pause & 1)
        require(matches(f, pause_valid=int(duplex == "full"), pause_rx=int(bool(rx)), pause_tx=int(bool(tx))),
                row, "directional pause evidence differs from qualified base pages")

    def intervene(self, action, cleanup=False):
        # The physical effect may have happened even if acknowledgment is lost.
        previous = self.peer_may_change
        self.peer_may_change = True
        try:
            receipt = yield Intervention(action, cleanup)
        except AssertionFailed:
            # The host stopped at a clean boundary before offering this action.
            self.peer_may_change = previous
            raise
        require(receipt["outcome"] == "acknowledged", self.last, f"{action}: {receipt['outcome']}", "INCONCLUSIVE")

    def wait_link(self, profile, policy, wanted, *, cleanup=False, partner=False):
        # Include rejected, inconsistent bookends in the observation window.
        # Operator acknowledgment and host exchange each have separate bounds.
        anchor = None
        for index in range(8):
            row = yield from self.link(profile, policy, cleanup=cleanup, delay=0.25 if index else 0,
                                       partner=partner, retry_changed=True)
            finished = int(row.fields["time_us"])
            if anchor is None:
                anchor = finished
            require(finished - anchor <= 3_000_000, row, "link observation window expired")
            if link_changed(row):
                continue
            observation = row.fields["observation"]
            if observation in ("up", "wrong_policy"):
                self.link_up(row, profile, wrong=wanted == "wrong_policy")
                if observation == wanted:
                    self.peer_may_change = False
                    return
            else:
                self.link_down(row, profile, pending=True, allow_completed=True)
                if observation == wanted:
                    return
        raise AssertionFailed("link did not reach the requested state after eight live observations", row)

    def mode_or_fault(self, profile):
        spec = self.spec
        pre = yield from self.ready_health()
        policy = spec.policy or spec.resolved or profile.modes
        if spec.fault in ("down", "no-common"):
            row = yield from self.link(profile, policy)
            self.link_down(row, profile, pending=spec.fault == "no-common")
        else:
            yield from self.wait_link(profile, policy, "wrong_policy" if spec.fault == "wrong_policy" else "up",
                                      partner=profile.mode == "an" and not spec.fault)
        if spec.fault in RECOVERY_ACTIONS:
            yield from self.intervene(spec.fault)
            yield from self.wait_link(profile, policy, "down")
            yield from self.intervene(RECOVERY_ACTIONS[spec.fault])
            yield from self.wait_link(profile, policy, "up")
        post = yield from self.ready_health()
        require(post.fields["failures"] == pre.fields["failures"] and
                int(post.fields["successes"]) > int(pre.fields["successes"]), post,
                "link evidence must retain successful management and unchanged failures")
        yield from self.application()

    def restore(self, saved):
        self.restoration = "INCONCLUSIVE"
        if self.peer_may_change:
            try:
                yield from self.intervene(RECOVERY_ACTIONS[self.spec.fault], True)
                yield from self.wait_link(self.spec.target, 8, "up", cleanup=True)
            except AssertionFailed as error:
                self.cleanup_failure = str(error)
                self.restoration = error.verdict
        try:
            yield from self.application(True)
            yield from self.transport(True)
            slot = yield from self.slot(True)
            if slot.fields["cleanup_owed"] == "1":
                desired = yield from self.profile("desired", True)
                require(self.declaration["reinitialize"] and matches(desired.fields, valid=1, stale=0) and
                        Profile.read(desired.fields) in (saved, self.spec.target), desired,
                        "cleanup debt requires an authorized reinitialize of a known current target")
                yield from self.reset("none", True)
            if self.spec.target or self.failed:
                yield from self.config(saved.apply(), True, True)
            yield from self.config("configuration verify", False, True)
            require((yield from self.profiles(True)) == saved, self.last, "saved entry profile was not restored")
            require((yield from self.slot(True)).fields["cleanup_owed"] == "0", self.last, "cleanup debt remains")
            yield from self.application(True)
            yield from self.ready_health(True)
            if not self.cleanup_failure:
                self.restoration = "PASS"
        except AssertionFailed as error:
            self.cleanup_failure = ". ".join(filter(None, (self.cleanup_failure, str(error))))
            if self.restoration != "FAIL":
                self.restoration = error.verdict

    def execute(self):
        self.session = yield Command("session")
        spec, d = self.spec, self.declaration
        saved = None
        try:
            require(clean(self.session), self.session, "session is not current")
            require(spec.id != "int-pin", None, "wired INT edge evidence unavailable in this campaign", "BLOCKED")
            require(self.backend in ("sim-mdio", BASIC, OWNER), None, "application owner schema unavailable", "BLOCKED")
            require(d["polling"] == "exclusive" or spec.id == "cold-boot", None,
                    "concurrent polling requires command-attributed accounting", "BLOCKED")
            if self.feature:
                gate = self.feature_gate()
                require(gate is None, None, gate, "BLOCKED")
            require(not spec.peer or (d["peer_profile"] == spec.peer and
                    (spec.peer == "open" or (d["peer"] and d["cable"]))), None,
                    f"missing compatible fixture declaration for {spec.peer}", "NOT RUN")
            if spec.id == "cold-boot":
                require(d["power_record"] and d["previous_boot"] is not None, None,
                        "missing authorized cold-start record or prior boot identity", "NOT RUN")
                require(self.session.boot != d["previous_boot"] and self.session.req == 1, self.session,
                        "cold-start record requires a distinct boot and first request")
            before = yield from self.entry()
            if spec.id == "observe":
                yield from self.passive(before)
            elif spec.id == "cold-boot":
                yield from self.profiles(unavailable="BLOCKED")
                if d["polling"] == "exclusive" and not (self.p25 or self.p22):
                    yield from self.vod()
            else:
                saved = yield from self.profiles(unavailable="BLOCKED")
                require(self.backend == "sim-mdio" or not (saved.mode == "forced" and saved.duplex == "half" and
                                                           saved.flags & 4), self.last,
                        "native route cannot restore this extended half-duplex profile", "BLOCKED")
                yield from self.config("configuration verify", False)
                require((yield from self.profiles()) == saved, self.last, "entry profile changed during verification")
                require((yield from self.slot()).fields["cleanup_owed"] == "0", self.last, "cleanup debt bars mutation")
                if self.feature:
                    yield from self.run_feature(saved)
                elif spec.reset:
                    self.may_change = True
                    yield from self.reset(spec.reset)
                else:
                    self.may_change = True
                    yield from self.config(spec.target.apply(), True)
                    yield from self.config("configuration verify", False)
                    require((yield from self.profiles()) == spec.target, self.last, "target profile readback differs")
                    if spec.resolved or spec.fault:
                        yield from self.mode_or_fault(spec.target)
                    if spec.id == "10base-te" and not (self.p25 or self.p22):
                        yield from self.vod()
            self.success = self.failed is None
        except AssertionFailed as error:
            self.failed = error
        if self.may_change and self.feature is None:
            yield from self.restore(saved)
        yield None
