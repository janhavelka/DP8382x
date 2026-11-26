"""Typed diagnostic campaigns for the exact-part compiled MDIO fixture.

The fixed stimuli and raw words below belong to the simulated fixture. The
campaign's device and application gates apply before any of these commands.
"""

from dataclasses import dataclass, replace

if __package__:
    from .dp8382x_hil_campaign import (
        AssertionFailed, Intervention, Profile, clean, matches, payload, require,
    )
    from .dp8382x_hil_protocol import ProtocolError
else:
    from dp8382x_hil_campaign import (
        AssertionFailed, Intervention, Profile, clean, matches, payload, require,
    )
    from dp8382x_hil_protocol import ProtocolError


@dataclass(frozen=True)
class Loopback:
    arguments: str
    mode: str
    speed: int
    equipment: str
    reset: str = "none"
    hard_exit: bool = False
    physts: int = 0


LOOPBACKS = {
    "mii": Loopback("mii 100", "mii", 100, "MAC frame generator and receive capture"),
    "mii-line": Loopback("mii 100 1", "mii", 100, "MAC frame capture and loaded line monitor"),
    "pcs-in": Loopback("pcs-in 10", "pcs_in", 10, "10 Mbit/s MAC stimulus and receive capture"),
    "pcs-out": Loopback("pcs-out 100", "pcs_out", 100, "100 Mbit/s MAC stimulus and receive capture"),
    "digital": Loopback("digital 100 16", "digital", 100, "MAC frame source and capture, HARD exit",
                        "restart", True),
    "analog": Loopback("analog 100 2", "analog", 100, "MAC stimulus and 100-ohm MDI termination", "restart"),
    "reverse": Loopback("reverse 100 4 1", "reverse", 100, "live 100/full peer exercising the MDI return path",
                        "restart", False, 0x15),
    "external": Loopback("external 100 8", "external", 100, "qualified external loop and MAC frame capture"),
}


@dataclass(frozen=True)
class Bist:
    arguments: str
    mode: int
    biscr: int
    counter: int
    outcome: str
    read_arguments: str = ""
    reset: str = "none"
    hard_exit: bool = False


BISTS = {
    "off": Bist("off saturate 0 0 1000 100 line", 0, 0x0100, 0, "disabled"),
    "single": Bist("single saturate 123 16 1000 100 line", 1, 0x1300, 4, "single_packet_observed", " single"),
    "checker": Bist("checker saturate 0 0 1000 100 line", 2, 0x2900, 0x0500, "errors_observed", " stimulus"),
    "both": Bist("both wrap 123 16 1000 100 line", 3, 0x7B00, 0x0504, "errors_observed"),
    "digital": Bist("both saturate 123 16 1000 100 digital 16", 3, 0x3B04, 0x0504, "errors_observed",
                    reset="restart", hard_exit=True),
    "analog": Bist("both saturate 123 16 1000 100 analog 2", 3, 0x3B08, 0x0504, "errors_observed", reset="restart"),
    "external": Bist("both wrap 123 16 1000 100 external 8", 3, 0x7B00, 0x0504, "errors_observed"),
}

COMPLIANCE = {
    "tx100": (0, "mdi", 1, 0, "restart"),
    "nlp10": (1, "mdi", 1, 0, "restart"),
    "prbs10": (2, "mdix", 2, 0, "restart"),
    "pattern10": (3, "mdix", 2, 3, "hard"),
}

CASES = {
    **{"loopback-" + mode: (entry.equipment, True) for mode, entry in LOOPBACKS.items()},
    **{"bist-" + name: (
        "bounded frame or checker stimulus and capture, with selected loopback path or termination", True)
       for name in BISTS},
    "cable-manual": ("quiet peer and characterized known-good/open/short cable with calibration", True),
    "cable-raw": ("quiet peer and characterized cable for raw-only observation", True),
    "cable-autorun": ("known up peer, isolated link-loss action and identifiable capture provenance", True),
    **{"compliance-" + name: (
        "calibrated differential scope and selected MDI/MDIX compliance load", True) for name in COMPLIANCE},
    "collision-10": ("wired MII TX_EN/COL and logic analyzer, 10 Mbit/s peer", True),
    "collision-100": ("wired MII TX_EN/COL and logic analyzer, 100 Mbit/s peer", True),
    "calibration-recovery": ("observed CDCR calibration condition and known HARD/replay route", True),
}

PEERS = {
    "loopback-reverse": "an-100-full",
    "bist-analog": "forced-100-full",
    "bist-digital": "forced-100-full",
    "cable-autorun": "an-100-full",
    "collision-10": "an-10-half",
    "collision-100": "an-100-full",
}
MII_CASES = {"collision-10", "collision-100"}
# SNLS638C 6.3.14 needs forced speed for reverse and a terminated MDI.
# The exact-part digital, analog and reverse selectors take effect without the
# DP83826 opaque companions or restart. All cases exercise the common owner.
P25_LOOPBACKS = dict(LOOPBACKS, digital=replace(LOOPBACKS["digital"], arguments="digital 100",
                                                reset="none", hard_exit=False),
                     analog=replace(LOOPBACKS["analog"], reset="none"),
                     reverse=replace(LOOPBACKS["reverse"], arguments="reverse 100 6 1", reset="none", physts=5))
P25_BISTS = dict(BISTS, digital=replace(BISTS["digital"], arguments="both saturate 123 16 1000 100 digital 0",
                                       reset="none", hard_exit=False),
                 analog=replace(BISTS["analog"], reset="none"))
P25_CASES = ({"loopback-" + name for name in P25_LOOPBACKS} | {"bist-" + name for name in P25_BISTS} |
             {"compliance-" + name for name in COMPLIANCE} |
             {"cable-raw", "cable-autorun", "collision-10", "collision-100"})
BASELINES = {"cable-autorun": ("diagnostics cable-autorun off",)}


def equal_fields(row, expected):
    """Check the entire supplied schema and the independent fixture values."""
    fields = payload(row)
    require(fields.keys() == expected.keys(), row, f"{row.cmd} missing or unexpected evidence fields")
    require(matches(fields, **expected), row, f"{row.cmd} evidence differs from the selected procedure")


def effect(changed, *, debt=0, temporary=0, restored=0, reset="none"):
    performed_reset = int(reset != "none")
    return dict(core_status="OK", core_detail=0, mutation_completed=changed, mutation_possible=changed,
                cleanup_owed=debt, temporary=temporary, persistent_restored=restored, effective_reset=reset,
                reset_accepted=performed_reset, reset_complete=performed_reset)


def result(row, expected, **effects):
    """Validate a clean correlated operation and its complete effect receipt."""
    require(clean(row), row, f"{row.cmd} did not complete cleanly")
    equal_fields(row, {**effects, **expected})


def begin(c, text, *, persistent=False):
    """Retain possible mutation before sending, including an interrupted reply."""
    c.may_change = True
    row = yield from c.command(text, "operation")
    if row.op == 0:
        if row.primary != "OK" and row.fields["cleanup"] == "none" and not payload(row):
            c.may_change = False
    elif not persistent and not clean(row):
        fields = row.fields
        indicators = ("mutation_completed", "mutation_possible", "cleanup_owed", "temporary", "reset_accepted")
        if fields["cleanup"] == "none" and matches(fields, **dict.fromkeys(indicators, 0)):
            c.may_change = False
    return row


def cleanup_failure(c, error):
    c.cleanup_failure = ". ".join(filter(None, (c.cleanup_failure, str(error))))
    if c.restoration != "FAIL":
        c.restoration = error.verdict


def temporary_session(c, text, observe, stop):
    """Run one typed stimulus and make at most one owning stop attempt."""
    started = yield from begin(c, text)
    try:
        require(clean(started), started, f"{started.cmd} did not complete cleanly")
        yield from observe(started)
    except AssertionFailed as error:
        c.failed = error
    if c.may_change:
        c.restoration = "INCONCLUSIVE"
        try:
            require(started.op != 0, started, "admitted owner token is unavailable")
            yield from c.guard_feature_cleanup()
            yield from stop(started)
            c.restoration = "PASS"
        except AssertionFailed as error:
            cleanup_failure(c, error)


def loopback(c, spec):
    mode = dict(mode=spec.mode, speed_mbps=spec.speed)
    stop_evidence = dict(**mode, controls_verified=0, stopped=1, hard_exit=int(spec.hard_exit),
                         physts_valid=0, physts=0)

    def observe(started):
        result(started, dict(**mode, controls_verified=1, stopped=0, hard_exit=int(spec.hard_exit),
                             physts_valid=int(bool(spec.physts)), physts=spec.physts),
               **effect(1, debt=1, temporary=1, reset=spec.reset))
        state = yield from c.read("diagnostics loopback-status")
        equal_fields(state, dict(**mode, session_op=started.op, controls_verified=1, stimulus_possible=1))

    def stop(started):
        row = yield from c.command("diagnostics loopback-stop", "operation", True)
        result(row, stop_evidence, **effect(1, restored=1, reset="hard" if spec.hard_exit else "none"))
        state = yield from c.read("diagnostics loopback-status", cleanup=True)
        equal_fields(state, dict(**mode, session_op=started.op, controls_verified=0, stimulus_possible=0))

    yield from temporary_session(c, "diagnostics loopback-start " + spec.arguments, observe, stop)


def sim_stimulus(c, action):
    receipt = yield Intervention(action, False)
    require(receipt["outcome"] == "acknowledged", c.last, f"{action}: {receipt['outcome']}", "INCONCLUSIVE")


def bist(c, spec):
    measurement = None

    def observe(started):
        nonlocal measurement
        initial = dict(session_op=started.op, mode=spec.mode, outcome="inconclusive", active_valid=0, active=0,
                       stale=1, before_valid=0, before=0, counter_valid=0, counter=0, after_valid=0, after=0,
                       measurement_valid=0, activity=0, lock=0, sync_loss=0, errors=0, saturated=0,
                       wrap_possible=0, interval_uncertain=1, history_lost=0)
        result(started, initial, **effect(1, debt=1, temporary=1, reset=spec.reset))
        if spec.counter >> 8:
            yield from sim_stimulus(c, "bist-errors")
        row = yield from c.command("diagnostics bist-read" + spec.read_arguments, "operation")
        require(clean(row), row, "three-word BIST acquisition failed")
        require(int(row.fields["time_us"]) - int(started.fields["time_us"]) >= 1000,
                row, "BIST observation interval was not reached")
        require(row.fields.get("outcome") != "inconclusive", row,
                "BIST acquisition did not qualify the selected observation", "INCONCLUSIVE")
        expected = dict(initial, outcome=spec.outcome, stale=0, before_valid=1, before=spec.biscr,
                        counter_valid=1, counter=spec.counter, after_valid=1, after=spec.biscr,
                        measurement_valid=1, activity=int(bool(spec.biscr & 0x0200)),
                        lock=int(bool(spec.biscr & 0x0800)), errors=spec.counter >> 8,
                        wrap_possible=int(bool(spec.biscr & 0x4000)))
        result(row, expected, **effect(1, debt=1, temporary=1))
        measurement = expected
        cached = yield from c.read("diagnostics bist-status")
        equal_fields(cached, dict(expected, active_valid=1, active=1))

    def stop(started):
        row = yield from c.command("diagnostics bist-stop", "operation", True)
        require(clean(row), row, "BIST stop failed")
        effects = effect(1, restored=1, reset="hard" if spec.hard_exit else "none")
        require(matches(row.fields, **effects, session_op=started.op, mode=spec.mode,
                        active_valid=0, active=0, stale=1), row, "BIST session stop or restoration evidence differs")
        if measurement is not None:
            equal_fields(row, dict(measurement, stale=1, **effects))
        cached = yield from c.read("diagnostics bist-status", cleanup=True)
        require(matches(cached.fields, session_op=started.op, mode=spec.mode, active_valid=1, active=0, stale=1),
                cached, "BIST status has no stopped-session evidence")
        if measurement is not None:
            equal_fields(cached, dict(measurement, active_valid=1, stale=1))

    yield from temporary_session(c, "diagnostics bist-start " + spec.arguments, observe, stop)


CABLE_ROWS = ("r1a", "r1b", "r2", "r3", "r4", "r5")


def cable_words(token, mask, recipe, *, autorun=False, p25=False):
    """Return the fixed raw fixture vector with per-row validity and freshness."""
    cdcr = 0x0002 if p25 else 0x0102
    flags = "200F" if autorun else "270F"
    words = f"{flags}:{cdcr:04X}:0006:0044:0000"
    return dict(session_op=token, recipe=recipe, requested_mask=mask, complete=1, stale=int(autorun), stopped=0,
                autorun=int(autorun), status_changed=0, prerequisite_valid=1, prerequisite=cdcr,
                **{key: words if mask & (1 << index) else "00F0:0000:0000:0000:0000"
                   for index, key in enumerate(CABLE_ROWS)})


def reject_distance(c):
    row = yield from c.command("diagnostics cable-interpret 5000")
    require(not payload(row) and matches(row.fields, primary="INVALID_CONFIG", detail=4,
                                        cleanup="none", reason="none", deadline_valid=0),
            row, "raw capture accepted distance interpretation")


def cable(c, saved, raw):
    text = "cable-raw aa 1 4 1 0x0d07 0x24 0x1f7a 3" if raw else "cable-start 63 3"
    recipe, mask = ("raw", 1) if raw else ("snla428", 63)
    measurement = None

    def observe(started):
        nonlocal measurement
        expected = cable_words(started.op, mask, recipe, p25=c.p25)
        result(started, expected, **effect(1, debt=1, temporary=1))
        captured = yield from c.command("diagnostics cable-read", "operation")
        result(captured, expected, **effect(0, debt=1, temporary=1))
        measurement = expected
        cached = yield from c.read("diagnostics cable-status")
        equal_fields(cached, expected)
        if raw:
            yield from reject_distance(c)
        else:
            interpretation = yield from c.command("diagnostics cable-interpret 5000")
            result(interpretation, dict(conclusion="open", row=2, distance_valid=1, distance_um=800000,
                                        amplitude_limit=0, bit11=0, bit5=0))

    def stop(started):
        row = yield from c.command("diagnostics cable-stop", "operation", True)
        require(clean(row), row, "cable HARD stop failed")
        effects = effect(1, restored=1, reset="hard")
        require(matches(row.fields, **effects, session_op=started.op, recipe=recipe, requested_mask=mask,
                        stale=1, stopped=1, autorun=0), row, "cable HARD stop and restoration evidence differs")
        if measurement is not None:
            equal_fields(row, dict(measurement, stale=1, stopped=1, **effects))
        cached = yield from c.read("diagnostics cable-status", cleanup=True)
        require(matches(cached.fields, session_op=started.op, recipe=recipe, requested_mask=mask,
                        stale=1, stopped=1, autorun=0), cached, "cable status did not retain stopped session")
        if measurement is not None:
            equal_fields(cached, dict(measurement, stale=1, stopped=1))

    quiet = yield from c.link(saved, saved.modes)
    c.link_down(quiet, saved)
    yield from temporary_session(c, "diagnostics " + text, observe, stop)


def autorun_policy(enabled):
    return dict(desired_valid=1, desired=enabled, observed_valid=1, observed=enabled, stale=0, trust=1)


def check_baseline(row, text, mode, a_grade=False):
    """The cable fixture starts with a verified disabled autorun policy."""
    require(text == BASELINES["cable-autorun"][0], row, "unrecognized compiled diagnostic setup")
    result(row, autorun_policy(0), **effect(1, restored=1))


def autorun_readback(c, enabled, cleanup=False):
    row = yield from c.read("diagnostics cable-autorun-status", cleanup=cleanup)
    equal_fields(row, autorun_policy(enabled))
    row = yield from c.read("registers read c22 9", cleanup=cleanup)
    equal_fields(row, dict(address=9, devad=0, value=0x0020 | (enabled << 8)))


def autorun_up(c, saved, cleanup=False):
    row = yield from c.link(saved, saved.modes, cleanup=cleanup)
    require(matches(row.fields, observation="up", up=1, resolved=1, speed_mbps=100, duplex="full",
                    partner_valid=1, partner_modes=8, anlpar=0x0101), row,
            "the selected autorun peer did not resolve 100/full")


def replay_autorun(c, slot, saved):
    """Clear retained policy debt only through the authorized known-intent route."""
    require(c.declaration["reinitialize"], slot, "autorun debt requires a qualified no-reset replay route")
    desired = yield from c.profile("desired", True)
    require(matches(desired.fields, valid=1, stale=0) and Profile.read(desired.fields) == saved,
            desired, "saved link intent was lost before autorun replay")
    policy = yield from c.read("diagnostics cable-autorun-status", cleanup=True)
    require(matches(policy.fields, desired_valid=1, desired=1), policy, "autorun target intent is unavailable")
    yield from c.reset("none", True)
    yield from autorun_readback(c, 1, True)
    ready = yield from c.guard_feature_cleanup()
    require(matches(ready.fields, temporary=0, cleanup_owed=0), ready, "autorun replay left cleanup debt")


def autorun(c, saved):
    try:
        yield from autorun_readback(c, 0)
        yield from autorun_up(c, saved)
        changed = yield from begin(c, "diagnostics cable-autorun on", persistent=True)
        result(changed, autorun_policy(1), **effect(1, restored=1))
        yield from autorun_readback(c, 1)
        yield from c.intervene("unplug")
        row = yield from c.link(saved, saved.modes)
        c.link_down(row, saved)
        capture = yield from c.command("diagnostics cable-autorun-read", "operation")
        # Raw words remain stale; the observed link loss cannot date their capture.
        expected = cable_words(capture.op, 1, "raw", autorun=True, p25=c.p25)
        result(capture, expected, **effect(0))
        status = yield from c.read("diagnostics cable-status")
        equal_fields(status, expected)
        yield from reject_distance(c)
    except AssertionFailed as error:
        c.failed = error
    if c.may_change:
        c.restoration = "INCONCLUSIVE"
        try:
            slot = yield from c.guard_feature_cleanup()
            require(slot.fields["temporary"] == "0", slot, "temporary activity bars autorun policy restore")
            if slot.fields["cleanup_owed"] == "1":
                yield from replay_autorun(c, slot, saved)
            restored = yield from c.command("diagnostics cable-autorun off", "operation", True)
            result(restored, autorun_policy(0), **effect(1, restored=1))
            yield from autorun_readback(c, 0, True)
        except AssertionFailed as error:
            cleanup_failure(c, error)
        if c.peer_may_change:
            try:
                yield from c.intervene("replug", True)
                yield from autorun_up(c, saved, True)
                c.peer_may_change = False
            except AssertionFailed as error:
                cleanup_failure(c, error)
        if not c.cleanup_failure:
            c.restoration = "PASS"


def electrical_state(**changes):
    return dict(dict(compliance=0, collision=0, controls_verified=0, stopped=0, stimulus_possible=0,
                     mode=0, pair=1, pattern=0, collision_mbps=0, cdcr_valid=0, cdcr=0, recovery_needed=0,
                     hard_owed=0, electrical_reset_complete=0), **changes)


def replay(c, saved):
    """Open the separate SDK/core replay route only after a proved HARD exit."""
    slot = yield from c.guard_feature_cleanup()
    require(matches(slot.fields, temporary=0, cleanup_owed=1), slot,
            "replay requires a stopped stimulus and retained restoration debt")
    desired = yield from c.profile("desired", True)
    require(matches(desired.fields, valid=1, stale=0, trust="DIRTY") and Profile.read(desired.fields) == saved,
            desired, "HARD reset left no known entry profile for replay")
    row = yield from c.command("recovery reinitialize none", "operation", True)
    sor1, sor2 = 0xA567, 0xA568
    if c.a_feature:
        rmii = c.expected["interface"] == "RMII"
        sor1 = (0x0010 if c.declaration["mode"] == "basic" else 0x0102) if rmii else 0
        sor2 = 0x0010 if rmii else 0
    exact = {}
    if c.p25:
        follower = c.expected["rmii_role"] == "follower"
        sor1, sor2 = (8 if follower else 0), 0
        exact = dict(rcsr_valid=1, rcsr=(0x00ED if follower else 0x006D))
    result(row, dict(**exact, core_status="OK", core_detail=0, requested_reset="none", effective_reset="none",
                     reset_required=0, reset_accepted=0, reset_complete=0, persistent_restored=1,
                     mutation_completed=1, mutation_possible=1, cleanup_owed=0, temporary=0,
                     clock_invalid=0, link_valid=0, sor1_valid=1, sor1=sor1,
                     sor2_valid=int(not c.p25), sor2=sor2, straps_stale=0))


def electrical(c, saved, family, selection):
    compliance = family == "compliance"
    if compliance:
        if selection == "count100":
            # Count 19 crosses the 0027/0428 boundary. Bit 2 stays clear.
            mode, pair_name, pair, pattern, reset = 4, "mdi", 1, 19, "hard"
        else:
            mode, pair_name, pair, pattern, reset = COMPLIANCE[selection]
        arguments = f"{selection} {pair_name} {pattern} {7 if selection == 'count100' else 3}"
        chosen = dict(mode=mode, pair=pair, pattern=pattern)
    else:
        arguments, chosen, reset = f"{selection} {4 if c.p25 else 3}", dict(collision_mbps=int(selection)), "none"
    active = electrical_state(**chosen, **{family: 1}, stimulus_possible=1, controls_verified=1)
    stopped = electrical_state(**chosen, stopped=1, controls_verified=int(not compliance),
                               electrical_reset_complete=int(compliance))

    def observe(started):
        result(started, active, **effect(1, debt=1, temporary=1, reset=reset))
        row = yield from c.read("diagnostics electrical-status")
        equal_fields(row, active)
        if selection == "count100":
            for address, word in ((0x0027, 3), (0x0428, 0x0030)):
                row = yield from c.read(f"registers read mmd 31 {address}")
                equal_fields(row, dict(address=address, devad=31, value=word))
            row = yield from c.read("registers read c22 17")
            require(int(row.fields["value"]) & 0x0800, row, "100BASE-TX scrambler bypass missing")

    def stop(started):
        row = yield from c.command(f"diagnostics {family}-stop", "operation", True)
        result(row, stopped, **effect(1, debt=int(compliance), restored=int(not compliance),
                                      reset="hard" if compliance else "none"))
        if compliance:
            yield from replay(c, saved)
        cached = yield from c.read("diagnostics electrical-status", cleanup=True)
        equal_fields(cached, stopped)

    yield from temporary_session(c, f"diagnostics {family}-start {arguments}", observe, stop)


def calibration(c, saved):
    baseline = electrical_state(cdcr_valid=1, cdcr=0x0102)
    done = electrical_state(cdcr_valid=1, cdcr=0x4102, recovery_needed=1, stopped=1, electrical_reset_complete=1)
    recovered = None
    try:
        idle = yield from begin(c, "diagnostics calibration-recover 1")
        # A seeded fault before this first sample is an unexpected stimulus.
        # Preserve any resulting HARD receipt for eligible recovery.
        recovered = idle
        if clean(idle) and matches(idle.fields, **effect(0), **baseline):
            c.may_change = False
        result(idle, baseline, **effect(0))
        yield from sim_stimulus(c, "calibration-flag")
        recovered = yield from begin(c, "diagnostics calibration-recover 1")
        result(recovered, done, **effect(1, debt=1, reset="hard"))
    except AssertionFailed as error:
        c.failed = error
    if c.may_change:
        c.restoration = "INCONCLUSIVE"
        try:
            require(recovered is not None and recovered.op != 0 and matches(
                    recovered.fields, effective_reset="hard", reset_accepted=1, reset_complete=1,
                    stopped=1, stimulus_possible=0, electrical_reset_complete=1, hard_owed=0, temporary=0,
                    cleanup_owed=1), recovered, "calibration HARD exit remains uncertain")
            yield from replay(c, saved)
            cached = yield from c.read("diagnostics electrical-status", cleanup=True)
            equal_fields(cached, done)
            c.restoration = "PASS"
        except AssertionFailed as error:
            cleanup_failure(c, error)


def run(c, saved):
    """Run one selected case. The shared campaign verifies owner exit afterward."""
    family, selection = c.spec.id.split("-", 1)
    if family in ("compliance", "calibration"):
        require(c.declaration["reinitialize"], c.last, "explicit replay authorization is required", "BLOCKED")
    if family == "loopback":
        yield from loopback(c, (P25_LOOPBACKS if c.p25 else LOOPBACKS)[selection])
    elif family == "bist":
        yield from bist(c, (P25_BISTS if c.p25 else BISTS)[selection])
    elif c.spec.id == "cable-autorun":
        yield from autorun(c, saved)
    elif family == "cable":
        yield from cable(c, saved, selection == "raw")
    elif family in ("compliance", "collision"):
        yield from electrical(c, saved, family, selection)
        if c.p25 and selection == "tx100" and c.failed is None and c.restoration == "PASS":
            yield from electrical(c, saved, family, "count100")
    elif c.spec.id == "calibration-recovery":
        yield from calibration(c, saved)
    else:
        raise ProtocolError("unknown diagnostic fixture case")
