"""Exercise typed diagnostic routes through the compiled CLI and core bus model."""
import unittest

from fixture_transcript import invoke, records, request, run


PROFILE = b"configuration apply an 15 none auto 1\n"
ATTEST = "--attest-features"
TRACE = "--trace-writes"


def writes(output):
    return [tuple(int(word, 16) for word in line.split()[1:])
            for line in output.splitlines() if line.startswith(b"mdio-write ")]


class DiagnosticTranscript(unittest.TestCase):
    def terminal(self, rows, number, primary="OK", cooperative=True):
        matched = request(rows, number)
        self.assertEqual(len(matched), 2 if cooperative else 1, matched)
        final = matched[-1]
        self.assertEqual((final["state"], final["primary"]), ("terminal", primary), final)
        if cooperative:
            admitted = matched[0]
            self.assertEqual((admitted["state"], admitted["primary"]), ("admitted", "IN_PROGRESS"), admitted)
            self.assertGreater(int(admitted["op"]), 0)
            self.assertEqual(admitted["op"], final["op"])
        return final

    def assert_writes_in_order(self, actual, expected):
        remaining = iter(actual)
        for word in expected:
            self.assertIn(word, remaining, f"Missing ordered write {word!r} in {actual!r}")

    def assert_same_counters(self, left, right):
        for key in ("batches", "attempted", "completed"):
            self.assertEqual(left[key], right[key])

    def test_mii_loopback_transmit_and_ordered_restore(self):
        result = invoke(PROFILE + b"diagnostics loopback-start mii 100 1\n"
                        b"cached transport\ndiagnostics loopback-status\ncached transport\n"
                        b"diagnostics loopback-stop\n", ATTEST, TRACE)
        rows = records(result.stdout)
        opened = self.terminal(rows, 2)
        state = self.terminal(rows, 4, cooperative=False)
        stopped = self.terminal(rows, 6)
        self.assertEqual((opened["mode"], opened["speed_mbps"], opened["controls_verified"], opened["temporary"]),
                         ("mii", "100", "1", "1"))
        self.assertEqual((state["session_op"], state["stimulus_possible"]), (opened["op"], "1"))
        self.assertEqual((stopped["stopped"], stopped["cleanup_owed"], stopped["temporary"]), ("1", "0", "0"))
        self.assertNotEqual(opened["op"], stopped["op"])
        self.assert_same_counters(request(rows, 3)[0], request(rows, 5)[0])
        self.assert_writes_in_order(writes(result.stderr), [(31, 0, 0x6100), (31, 0x16, 0x0144),
                                                           (31, 0x16, 0x0100), (31, 0, 0x2100)])

    def test_digital_loopback_exact_companion_recipe_and_hard_exit(self):
        result = invoke(PROFILE + b"diagnostics loopback-start digital 100 16\n"
                        b"diagnostics loopback-stop\n", ATTEST, TRACE)
        rows = records(result.stdout)
        self.assertEqual(self.terminal(rows, 2)["hard_exit"], "1")
        self.assertEqual(self.terminal(rows, 3)["stopped"], "1")
        self.assert_writes_in_order(writes(result.stderr), [
            (31, 0x00, 0x2100), (31, 0x16, 0x0104), (31, 0x0122, 0x2000),
            (31, 0x0123, 0x2000), (31, 0x0130, 0x47FF), (31, 0x1F, 0x4000), (31, 0x1F, 0x8000)])

    def test_cancelled_loopback_restore_remains_owed(self):
        # The fixture setup uses token 1, then profile, start and stop use 2, 3 and 4.
        rows = records(run(PROFILE + b"diagnostics loopback-start mii 100\n$pause\n"
                           b"diagnostics loopback-stop\n$tick 3\nowner cancel 4\n$resume\n"
                           b"diagnostics loopback-stop\n", ATTEST))
        cancelled = self.terminal(rows, 3, "CANCELLED")
        self.terminal(rows, 4, cooperative=False)
        stopped = self.terminal(rows, 5)
        self.assertEqual((cancelled["cleanup_owed"], cancelled["temporary"], cancelled["stopped"]), ("1", "1", "0"))
        self.assertEqual((stopped["cleanup_owed"], stopped["stopped"]), ("0", "1"))
        self.assertNotEqual(cancelled["op"], stopped["op"])

    def test_bist_capture_and_stop_report_independent_outcomes(self):
        result = invoke(PROFILE + b"diagnostics bist-start both wrap 123 16 1000 100 line\n"
                        b"$inject-bist-errors\ndiagnostics bist-read\ndiagnostics bist-status\n"
                        b"diagnostics bist-stop\n", ATTEST, TRACE)
        rows = records(result.stdout)
        started, observed, cached, stopped = (self.terminal(rows, number, cooperative=number != 4)
                                               for number in (2, 3, 4, 5))
        for capture in (observed, cached):
            self.assertEqual(capture["session_op"], started["op"])
            self.assertEqual((capture["outcome"], capture["counter_valid"], capture["counter"], capture["errors"],
                              capture["measurement_valid"], capture["wrap_possible"]),
                             ("errors_observed", "1", str(0x0504), "5", "1", "1"))
        self.assertEqual((cached["active_valid"], cached["active"], stopped["cleanup_owed"]), ("1", "1", "0"))
        self.assert_writes_in_order(writes(result.stderr), [(31, 0x1B, 0x0004), (31, 0x1C, 0x007B),
                                                           (31, 0x1B, 0x8004), (31, 0x16, 0x7100),
                                                           (31, 0x1B, 0x8004), (31, 0x1B, 0x007D)])

    def test_bist_failed_capture_and_failed_restore_keep_effects(self):
        rows = records(run(PROFILE + b"diagnostics bist-start both saturate 1518 500 1000 100\n"
                           b"$inject-bist-errors\n$fail-next 3\ndiagnostics bist-read\n"
                           b"diagnostics bist-status\n$fail-next 0\ndiagnostics bist-stop\n"
                           b"diagnostics bist-stop\n", ATTEST))
        failed = self.terminal(rows, 3, "MDIO_ERROR")
        self.assertEqual((failed["before_valid"], failed["counter_valid"], failed["after_valid"],
                          failed["measurement_valid"], failed["temporary"]), ("1", "1", "0", "0", "1"))
        cached = self.terminal(rows, 4, cooperative=False)
        self.assertEqual((cached["stale"], cached["errors"]), ("1", "5"))
        cleanup = self.terminal(rows, 5, "MDIO_ERROR")
        retry = self.terminal(rows, 6)
        self.assertEqual((cleanup["cleanup_owed"], retry["cleanup_owed"]), ("1", "0"))
        self.assertEqual(len({failed["op"], cleanup["op"], retry["op"]}), 3)

    def test_cable_recipe_capture_interpret_and_restore(self):
        result = invoke(PROFILE + b"diagnostics cable-start 1 3\ndiagnostics cable-read\n"
                        b"cached transport\ndiagnostics cable-status\ndiagnostics cable-interpret 5000\n"
                        b"cached transport\ndiagnostics cable-stop\n", ATTEST, TRACE, "--partner", "down")
        rows = records(result.stdout)
        for number in (2, 3, 5):
            row = self.terminal(rows, number, cooperative=number != 5)
            self.assertEqual((row["complete"], row["stale"], row["requested_mask"]), ("1", "0", "1"))
            flags, *words = (int(part, 16) for part in row["r1a"].split(":"))
            self.assertEqual(flags & 0x35FF, 0x250F)
            self.assertEqual(words, [0x0102, 6, 68, 0])
        diagnosis = self.terminal(rows, 6, cooperative=False)
        self.assertEqual((diagnosis["conclusion"], diagnosis["distance_valid"], diagnosis["distance_um"]),
                         ("open", "1", "800000"))
        self.assert_same_counters(request(rows, 4)[0], request(rows, 7)[0])
        self.assertEqual(self.terminal(rows, 8)["stopped"], "1")
        self.assert_writes_in_order(writes(result.stderr), [
            (31, 0x0170, 0x5C12), (31, 0x0173, 0x0D00), (31, 0x0175, 0x1007),
            (31, 0x0178, 2), (31, 0x0416, 0x1FA0), (31, 0x0411, 0x0813), (31, 0x1E, 0x8102),
            (31, 0x1F, 0x8000)])

    def test_raw_cable_boundary_values_and_no_invented_distance(self):
        result = invoke(PROFILE + b"diagnostics cable-raw aa 1 4 1 0x0d07 0x24 0x1f7a 3\n"
                        b"diagnostics cable-interpret 5000\ndiagnostics cable-stop\n",
                        ATTEST, TRACE, "--partner", "down")
        rows = records(result.stdout)
        self.assertEqual(self.terminal(rows, 2)["recipe"], "raw")
        self.terminal(rows, 3, "INVALID_CONFIG", cooperative=False)
        self.assertEqual(self.terminal(rows, 4)["stopped"], "1")
        trace = writes(result.stderr)
        for expected in [(31, 0x0170, 0x5410), (31, 0x0173, 0x0D07), (31, 0x0175, 0x1004),
                         (31, 0x0176, 0x001F), (31, 0x0177, 0x0F00), (31, 0x0178, 0x0002)]:
            self.assertIn(expected, trace)
        self.assertFalse(any(reg in (0x0411, 0x0416) for _, reg, _ in trace))

    def test_autorun_policy_is_independent_of_raw_capture_freshness(self):
        result = invoke(PROFILE + b"diagnostics cable-autorun on\ndiagnostics cable-autorun-status\n"
                        b"diagnostics cable-autorun-read\ndiagnostics cable-autorun off\n", ATTEST, TRACE)
        rows = records(result.stdout)
        for number in (2, 3):
            policy = self.terminal(rows, number, cooperative=number == 2)
            self.assertEqual((policy["desired"], policy["observed"], policy["observed_valid"], policy["stale"]),
                             ("1", "1", "1", "0"))
        raw = self.terminal(rows, 4)
        self.assertEqual((raw["autorun"], raw["complete"], raw["stale"]), ("1", "1", "1"))
        self.assertEqual(self.terminal(rows, 5)["observed"], "0")
        trace = writes(result.stderr)
        cr1 = [word for _, address, word in trace if address == 9]
        self.assertEqual(cr1[-2] & 0x0100, 0x0100)
        self.assertEqual(cr1[-1], cr1[-2] & 0xFEFF)
        self.assertNotIn((31, 0x1E, 0x8102), trace)

    def test_manual_timeout_retains_terminal_under_console_backpressure(self):
        rows = records(run(PROFILE + b"$pause\n$output-stall 4096\ndiagnostics cable-start 1 3\n$tick 1\n"
                           + b"drv\n" * 28 + b"$tick 4096\n$output-stall 0\n$resume\n"
                           b"diagnostics cable-stop\n", ATTEST, "--hold-tdr", "--partner", "down"))
        failed = self.terminal(rows, 2, "OPERATION_TIMEOUT")
        self.assertEqual((failed["temporary"], failed["cleanup_owed"], failed["complete"]), ("1", "1", "0"))
        stopped = self.terminal(rows, 31)
        self.assertEqual((stopped["stopped"], stopped["cleanup_owed"]), ("1", "0"))
        self.assertNotEqual(failed["op"], stopped["op"])

    def test_compliance_collision_and_calibration_use_literal_writes(self):
        result = invoke(PROFILE + b"diagnostics compliance-start pattern10 mdi 3 3\n"
                        b"diagnostics electrical-status\ndiagnostics compliance-stop\n"
                        b"recovery reinitialize none\n"
                        b"diagnostics collision-start 100 3\ndiagnostics collision-stop\n"
                        b"diagnostics calibration-recover 1\n$raise-calibration\ndiagnostics calibration-recover 1\n",
                        ATTEST, TRACE, "--mii")
        rows = records(result.stdout)
        for number in (2, 3, 6):
            session = self.terminal(rows, number, cooperative=number != 3)
            self.assertEqual((session["controls_verified"], session["stimulus_possible"]), ("1", "1"))
        compliance_stop = self.terminal(rows, 4)
        self.assertEqual((compliance_stop["stopped"], compliance_stop["cleanup_owed"]), ("1", "1"))
        restored = self.terminal(rows, 5)
        self.assertEqual((restored["persistent_restored"], restored["cleanup_owed"]), ("1", "0"))
        self.assertNotEqual(restored["op"], compliance_stop["op"])
        self.assertEqual(self.terminal(rows, 7)["stopped"], "1")
        untouched = self.terminal(rows, 8)
        recovered = self.terminal(rows, 9)
        self.assertEqual((untouched["cdcr_valid"], untouched["recovery_needed"]), ("1", "0"))
        self.assertEqual((recovered["cdcr"], recovered["recovery_needed"], recovered["electrical_reset_complete"],
                          recovered["cleanup_owed"]), (str(0x4102), "1", "1", "1"))
        trace = writes(result.stderr)
        self.assert_writes_in_order(trace, [(31, 0x1F, 0x8000), (31, 0x00, 0x0100), (31, 0x0027, 0x0013),
                                            (31, 0x1F, 0x8000), (31, 0x00, 0x1080), (31, 0x00, 0x1000)])
        calibration = trace.index((31, 0x1E, 0x0102))
        self.assertEqual(trace[calibration + 1], (31, 0x1F, 0x8000))

    def test_invalid_missing_and_source_gated_diagnostics_are_bus_silent(self):
        rows = records(run(b"cached transport\ndiagnostics loopback-start pcs-in 100\n"
                           b"diagnostics loopback-start digital 10 16\ndiagnostics loopback-start analog 100\n"
                           b"diagnostics bist-start both wrap 2048 16 1 100\n"
                           b"diagnostics bist-start both wrap 2047 15 1 100\n"
                           b"diagnostics cable-start 64 3\ndiagnostics cable-raw aa 1 7 1 0 0 0 3\n"
                           b"diagnostics cable-interpret 0\ndiagnostics compliance-start pattern10 mdi 14 3\n"
                           b"diagnostics collision-start 100 3\ndiagnostics compliance-start count100 mdi 31 3\n"
                           b"cached transport\n", ATTEST))
        for number in range(2, 11):
            self.terminal(rows, number, "INVALID_PARAM", cooperative=False)
        self.terminal(rows, 11, "INVALID_CONFIG", cooperative=False)  # No MII COL on this RMII fixture.
        self.terminal(rows, 12, "INVALID_CONFIG", cooperative=False)  # Exact-part 0428 source gate.
        self.assert_same_counters(request(rows, 1)[0], request(rows, 13)[0])

        missing = records(run(PROFILE + b"cached transport\ndiagnostics loopback-start analog 100 2\n"
                              b"diagnostics compliance-start pattern10 mdi 0 3\ncached transport\n"))
        for number in (3, 4):
            self.terminal(missing, number, "BUSY", cooperative=False)
        self.assert_same_counters(request(missing, 2)[0], request(missing, 5)[0])

    def test_active_session_precludes_each_disruptive_start(self):
        rows = records(run(b"cached transport\ndiagnostics loopback-start mii 100\n"
                           b"diagnostics bist-start off wrap 0 0 1 100\ndiagnostics cable-start 1 3\n"
                           b"diagnostics cable-autorun on\ndiagnostics compliance-start tx100 mdi 0 3\n"
                           b"diagnostics collision-start 100 3\ndiagnostics calibration-recover 1\ncached transport\n",
                           ATTEST, "--deny-maintenance", "--mii"))
        for number in range(2, 9):
            self.terminal(rows, number, "BUSY", cooperative=False)
        self.assert_same_counters(request(rows, 1)[0], request(rows, 9)[0])


if __name__ == "__main__":
    unittest.main()
