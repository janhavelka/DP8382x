"""Repeat explicitly admitted cases in one correlated session and retain their observations."""

if __package__:
    from .dp8382x_hil_campaign import (
        AssertionFailed, Campaign, Command, I32, MAX_COMMANDS, WORD, bits, clean, feature_fields, payload, require,
    )
    from .dp8382x_hil_protocol import ProtocolError, TERMINAL_STATUS, U32_MAX, U64_MAX
else:
    from dp8382x_hil_campaign import (
        AssertionFailed, Campaign, Command, I32, MAX_COMMANDS, WORD, bits, clean, feature_fields, payload, require,
    )
    from dp8382x_hil_protocol import ProtocolError, TERMINAL_STATUS, U32_MAX, U64_MAX


NATIVE_CONSOLE = ("input_queued output_queued lines_queued input_dropped input_gaps lines_dropped "
                  "lines_truncated uart_stalls output_backpressure").split()
CONSOLE = dict.fromkeys(("queued", "dropped_human", "stalled_writes"), (0, U32_MAX))
EVENTS = {**bits("valid stale lower_bound saturated observed_valid"), "event_mask": (0, 63),
          **dict.fromkeys("physts flds misr1 misr2 fcscr recr".split(), WORD),
          "false_carrier_total": (0, U64_MAX), "observed_us": (0, U64_MAX),
          "last_status": tuple(TERMINAL_STATUS), "last_detail": I32}


def _typed_payload(record):
    """Keep integer wire fields numeric in the artifact and retain their request attribution."""
    return {"req": record.req, **{key: int(value) if value.lstrip("-").isdigit() else value
                                  for key, value in payload(record).items()}}


class _MeasuredCase(Campaign):
    """Use the normal procedure and restoration, collecting only owner-routed snapshots."""

    def __init__(self, declaration, expected):
        self.telemetry = dict.fromkeys(("console", "transport", "health", "chip"))
        self.primary_case = None
        self.before_restore = None
        self.telemetry_error = None
        self.telemetry_complete = False
        self.command_count = 0
        self.attempted_command = None
        super().__init__(declaration, expected)

    def console_and_chip(self):
        console = yield from self.read("local console")
        native = self.backend != "sim-mdio"
        schema = {**CONSOLE, **dict.fromkeys(NATIVE_CONSOLE, (0, U32_MAX))} if native else CONSOLE
        feature_fields(console, schema)
        fields = _typed_payload(console)
        fields["native"] = {key: fields.pop(key) for key in NATIVE_CONSOLE} if native else None
        self.telemetry["console"] = fields

        chip = yield from self.read("cached events")
        feature_fields(chip, EVENTS)
        fields = _typed_payload(chip)
        require(fields["observed_valid"] or fields["observed_us"] == 0, chip,
                "cached chip timestamp has no validity flag")
        require(not fields["saturated"] or fields["false_carrier_total"] == U64_MAX, chip,
                "cached false-carrier overflow disagrees with the accumulated count")
        self.telemetry["chip"] = fields

    def entry(self):
        # The passive range-rejection proof calls entry twice. Its snapshots must
        # stay contiguous, with both sets acquired through the original procedure.
        if self.telemetry["console"] is None:
            yield from self.console_and_chip()
        return (yield from super().entry())

    def advance(self, value):
        if not isinstance(value, dict) and clean(value):
            group = {"cached.transport": "transport", "cached.health": "health"}.get(value.cmd)
            if group:
                self.telemetry[group] = _typed_payload(value)
        return super().advance(value)

    def execute(self):
        # Campaign.execute yields None after its final restoration decision.
        # Preserve the primary/restoration result before the exit measurements.
        procedure = super().execute()
        step = next(procedure)
        while step is not None:
            if step.cleanup and self.before_restore is None:
                self.before_restore = super().cases()[0]["primary"]
            answer = yield step
            step = procedure.send(answer)
        self.primary_case, = super().cases()
        if self.before_restore is not None:
            self.primary_case["primary"] = self.before_restore
        if self.primary_case["verdict"] == "PASS" and (
                not self.primary_case["restoration"]["required"] or self.restoration == "PASS"):
            try:
                yield from self.console_and_chip()
                yield from self.transport()
                yield from self.ready_health()
                self.telemetry_complete = True
            except AssertionFailed as error:
                self.telemetry_error = error
        yield None

    def result(self):
        if self.primary_case is None:
            row = super().cases()[0]
            if self.before_restore is not None:
                row["primary"] = self.before_restore
            return row
        row = self.primary_case.copy()
        if row["restoration"]["required"] and row["restoration"]["verdict"] != "PASS":
            return row
        if row["verdict"] == "PASS" and not self.telemetry_complete:
            row["verdict"] = self.telemetry_error.verdict if self.telemetry_error else "INCONCLUSIVE"
            row["message"] += ". Exit telemetry: " + (str(self.telemetry_error) if self.telemetry_error
                                                      else "incomplete")
        return row


class SoakCampaign:
    """Issue each requested repetition at most once under one session handshake.

    The carrier calls attempt before submitting a case command and stop after a
    host fault or expired deadline. A replay also recognizes attempts from the
    confirmed record stream. Unstarted iterations never acquire an outcome.
    """

    def __init__(self, declaration, expected, count):
        if type(count) is not int or count < 1:
            raise ProtocolError("soak count must be a positive integer")
        if declaration.get("case") == "cold-boot" and count > 1:
            raise ProtocolError("one session can attest only one cold boot")
        self.declaration = declaration
        self.expected = expected
        self.count = count
        self.current = _MeasuredCase(declaration, expected)
        self.spec = self.current.spec
        self.feature = self.current.feature
        self.baseline = self.current.baseline
        self.session = None
        self.last = None
        self.interventions = []
        self.finished = []
        self.measurements = []
        self.attempted = 0
        self.completed = 0
        self.stop_reason = "pending"

    def feature_gate(self):
        return self.current.feature_gate()

    @property
    def next(self):
        return self.current.next if self.stop_reason == "pending" else None

    def attempt(self):
        """Count the first attempted case write even when its receipt never arrives."""
        step = self.next
        if self.session is not None and isinstance(step, Command) and step is not self.current.attempted_command:
            if self.current.command_count >= MAX_COMMANDS + 6:
                raise ProtocolError("soak iteration command ceiling reached")
            self.current.attempted_command = step
            self.current.command_count += 1
            self.attempted = len(self.finished) + 1

    def advance(self, value):
        if self.next is None:
            raise ProtocolError("soak received a step after stopping")
        if self.session is None:
            self.session = value
        else:
            self.attempt()
        if not isinstance(value, dict):
            self.last = value
        self.current.advance(value)
        if isinstance(value, dict):
            self.interventions.append(value)
        if self.current.next is not None:
            return

        row = self.current.result()
        self.finished.append(row)
        self.measurements.append(self.current.telemetry)
        if row["executed"] and self.attempted == len(self.finished):
            self.completed += 1
        restore = row["restoration"]
        if row["verdict"] != "PASS" or (restore["required"] and restore["verdict"] != "PASS"):
            self.stop_reason = "fault"
        elif len(self.finished) == self.count:
            self.stop_reason = "count"
        else:
            self.current = _MeasuredCase(self.declaration, self.expected)
            self.current.advance(self.session)
            if self.current.next is None:
                raise ProtocolError("a repeated case lost its original admission")

    def stop(self, reason):
        """Freeze the first stop cause and preserve terminal results on a later carrier fault."""
        if reason not in ("fault", "duration", "cancelled"):
            raise ProtocolError("unknown soak stop cause")
        if self.stop_reason in ("pending", "count"):
            self.stop_reason = reason

    def _id(self, index):
        return f"{self.spec.id}-{index + 1}"

    def cases(self):
        rows = []
        for index in range(self.count):
            if index < len(self.finished):
                row = self.finished[index]
            elif index < self.attempted:
                row = self.current.result()
                if self.current.last is self.session:
                    row = {**row, "primary": None, "executed": False}
            else:
                row = {"executed": False, "verdict": "NOT RUN", "primary": None,
                       "restoration": {"required": False, "verdict": "NOT RUN"},
                       "message": "iteration was not attempted"}
            rows.append({**row, "id": self._id(index)})
        return rows

    def receipt(self):
        return {"declaration": self.declaration, "interventions": self.interventions}

    def summary(self):
        observations = list(self.measurements)
        if self.attempted > len(observations):
            observations.append(self.current.telemetry)
        return {"requested": self.count, "attempted": self.attempted, "completed": self.completed,
                "remaining": self.count - self.completed, "stop_reason": self.stop_reason,
                "telemetry": [{"id": self._id(index), **fields} for index, fields in enumerate(observations)]}
