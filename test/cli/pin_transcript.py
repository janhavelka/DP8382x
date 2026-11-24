"""Execute typed pin, interface, quality and analog CLI paths on the host fixture."""
import unittest

from fixture_transcript import invoke, records, request


ATTEST = "--attest-features"
TRACE = "--trace-writes"


class PinTranscript(unittest.TestCase):
    def assert_silent(self, before, after):
        for key in ("batches", "attempted", "completed"):
            self.assertEqual(before[key], after[key], key)

    def test_led0_source_bit_6_and_reserved_led3_drive(self):
        result = invoke(b"controls pins led,8,keep,normal keep keep keep keep keep keep keep\n"
                        b"controls pins-status\ncached transport\n"
                        b"controls pins keep keep keep led,0,keep,high keep keep keep keep\n"
                        b"controls pins clock keep keep keep keep keep keep keep\n"
                        b"controls pins led,15,keep,normal keep keep keep keep keep keep keep\n"
                        b"cached transport\n", ATTEST, TRACE)
        applied, cached, before, drive, mux, reserved, after = records(result.stdout)
        self.assertEqual((applied["primary"], applied["verified"], applied["mledcr"]), ("OK", "1", "65"))
        self.assertEqual((cached["primary"], cached["stale"], cached["mledcr"]), ("OK", "0", "65"))
        # Table 8-49: mode eight occupies bit 6 and leaves the source enabled.
        self.assertIn(b"mdio-write 1f 0025 0041", result.stderr.splitlines())
        self.assertNotIn(b"mdio-write 1f 0469", result.stderr)
        self.assertEqual((drive["primary"], mux["primary"], reserved["primary"]),
                         ("INVALID_CONFIG", "UNSUPPORTED", "INVALID_PARAM"))
        self.assert_silent(before, after)

    def test_rmii_repeater_side_and_odd_nibble_disable_polarity(self):
        result = invoke(b"controls interface rmii25 12 0 1518 50 2 keep keep\n"
                        b"controls interface-status\n"
                        b"controls interface rmii25 12 0 1518 50 0 keep keep\n"
                        b"cached transport\n"
                        b"controls interface rmii25 8 0 1518 50 0 keep keep\n"
                        b"controls interface rmii25 1 0 1518 50 0 keep keep\n"
                        b"cached transport\n", ATTEST, TRACE)
        disabled, cached, enabled, before, repeater, shift, after = records(result.stdout)
        for row in (disabled, cached, enabled):
            self.assertEqual((row["primary"], row["io_cfg1"], row["mode_valid"]), ("OK", "256", "1"))
        self.assertEqual((disabled["cr2"], cached["cr2"], enabled["cr2"]), ("2", "2", "0"))
        trace = result.stderr.splitlines()
        # Table 8-29: one disables odd-nibble handling, zero enables it.
        self.assertLess(trace.index(b"mdio-write 1f 000a 0002"), trace.index(b"mdio-write 1f 000a 0000"))
        self.assertIn(b"mdio-write 1f 0302 0100", trace)
        self.assertEqual((repeater["primary"], shift["primary"]), ("INVALID_CONFIG", "INVALID_CONFIG"))
        self.assert_silent(before, after)

    def test_a_pin30_recovered_clock_and_independent_pin31_read(self):
        for grade in ("DP83826AI", "DP83826AE"):
            command = b"controls interface rmii25 0 0 1518 50 16 recovered off\n"
            out = invoke(command + b"controls interface-status\n", "--variant", grade, ATTEST, TRACE)
            for row in records(out.stdout):
                self.assertEqual((row["primary"], row["valid_mask"], row["clk_led_status"], row["pin30_mux"]),
                                 ("OK", "511", "2", "49"))
            self.assertIn(b"mdio-write 1f 0303 0031", out.stderr.splitlines())
            down = invoke(command, "--variant", grade, "--partner", "down", ATTEST, TRACE)
            self.assertEqual(records(down.stdout)[0]["primary"], "INVALID_CONFIG")
            self.assertNotIn(b"mdio-write 1f 0303", down.stderr)

    def test_a_hard_reset_reloads_unowned_control_words(self):
        stream = (b"controls interface rmii25 0 0 1518 50 0 pll off\n"
                  b"controls interface rmii25 0 0 1518 50 0 keep off\n"
                  b"registers read mmd 31 0x0303\nrecovery reset hard\n"
                  b"registers read mmd 31 0x0303\nregisters read c22 0x11\n"
                  b"registers read mmd 31 0x0131\nregisters read c22 0x0b\n")
        for grade in ("DP83826AI", "DP83826AE"):
            for mode, physcr, cr3 in (("basic", 0x010B, 0x006D), ("enhanced", 0x0108, 0x0060)):
                with self.subTest(grade=grade, mode=mode):
                    rows = records(invoke(stream, "--variant", grade, "--campaign", mode, ATTEST).stdout)
                    self.assertEqual(request(rows, 4)[-1]["persistent_restored"], "1")
                    reads = [request(rows, n)[0] for n in (3, 5, 6, 7, 8)]
                    self.assertTrue(all(row["primary"] == "OK" for row in reads))
                    self.assertEqual([int(row["value"]) for row in reads], [0x29, 8, physcr, 0x0288, cr3])

    def test_mii_compatibility_clock_and_isolation(self):
        result = invoke(b"controls interface mii 0 0 1518 50 15 led1 on\n"
                        b"controls interface-status\n", "--mii", ATTEST, TRACE)
        applied, cached = records(result.stdout)
        for row in (applied, cached):
            self.assertEqual((row["primary"], row["cr1"], row["cr2"], row["physcr"], row["gencfg"],
                              row["io_cfg1"], row["clk_led_status"], row["bmcr"]),
                             ("OK", "2", "2", "16", "8", "64", "1", "13568"))
        self.assertIn(b"mdio-write 1f 0302 0040", result.stderr.splitlines())
        self.assertIn(b"mdio-write 1f 0000 3500", result.stderr.splitlines())

    def test_quality_fld_and_raw_mse(self):
        result = invoke(b"controls quality 0x404,2,5,9 1,3,1 1,8,1,1,1 on\n"
                        b"controls quality-status\ncached transport\ncontrols mse\n"
                        b"controls mse-status\ncached transport\n", "--mii", ATTEST, TRACE)
        rows = records(result.stdout)
        admitted, terminal = request(rows, 1)
        self.assertEqual((admitted["state"], terminal["state"], terminal["primary"], terminal["op"]),
                         ("admitted", "terminal", "OK", admitted["op"]))
        for row in (terminal, request(rows, 2)[0]):
            self.assertEqual((row["cr3"], row["fld_cfg1"], row["fld_cfg2"], row["physcr"], row["cr2"],
                              row["btscr"], row["cr1"]), ("1028", "2135", "8841", "2816", "4", "12417", "128"))
        trace = result.stderr.splitlines()
        writes = [b"mdio-write 1f 000b 0000", b"mdio-write 1f 0117 0857", b"mdio-write 1f 0131 2289",
                  b"mdio-write 1f 000b 0404"]
        positions = [trace.index(write) for write in writes]
        self.assertEqual(positions, sorted(positions))
        for number in (4, 5):
            row = request(rows, number)[0]
            self.assertEqual((row["primary"], row["valid"], row["raw_mse"]), ("OK", "1", "4660"))
        before, after = request(rows, 3)[0], request(rows, 6)[0]
        self.assertEqual(int(after["attempted"]) - int(before["attempted"]), 4)

    def test_mse_failure_keeps_cached_word_historical(self):
        result = invoke(b"cached transport\ncontrols mse\n$fail-next 3\ncontrols mse\n"
                        b"controls mse-status\ncached transport\n")
        before, good, failure, cached, after = records(result.stdout)
        self.assertEqual(failure["primary"], "MDIO_ERROR")
        self.assertNotIn("raw_mse", failure)
        self.assertEqual((good["stale"], cached["stale"], cached["last_status"]), ("0", "1", "MDIO_ERROR"))
        self.assertEqual((good["raw_mse"], cached["raw_mse"]), ("4660", "4660"))
        self.assertEqual(good["observed_us"], cached["observed_us"])
        self.assertEqual(int(after["attempted"]) - int(before["attempted"]), 8)
        self.assertEqual(int(after["completed"]) - int(before["completed"]), 7)

    def test_analog_split_transition_uses_safe_bridge(self):
        result = invoke(b"controls analog 0x28,0x2f,0x18,8,0 4,4,1,1 slow fast keep\n"
                        b"controls analog-status\ncached transport\n"
                        b"controls analog keep keep keep keep 0\ncached transport\n", ATTEST, TRACE)
        rows = records(result.stdout)
        admitted, terminal = request(rows, 1)
        self.assertEqual((admitted["state"], terminal["state"], terminal["primary"], terminal["op"]),
                         ("admitted", "terminal", "OK", admitted["op"]))
        trace = result.stderr.splitlines()
        writes = [b"mdio-write 1f 030c 8410", b"mdio-write 1f 030b 2a00", b"mdio-write 1f 030c f218"]
        positions = [trace.index(write) for write in writes]
        self.assertEqual(positions, sorted(positions))
        for row in (terminal, request(rows, 2)[0]):
            self.assertEqual((row["vod1"], row["vod2"], row["vod3"], row["ioctrl"], row["io_cfg1"]),
                             ("10752", "61976", "19008", "17", "16384"))
        self.assertEqual(request(rows, 4)[0]["primary"], "INVALID_CONFIG")
        self.assert_silent(request(rows, 3)[0], request(rows, 5)[0])

    def test_pin_target_failure_retains_uncertain_effect_and_debt(self):
        result = invoke(b"cached transport\n$fail-next 8\n"
                        b"controls pins led,8,keep,normal keep keep keep keep keep keep keep\n"
                        b"cached operation\ncached transport\n", ATTEST)
        before, failure, slot, after = records(result.stdout)
        self.assertEqual((failure["primary"], failure["batch_primary"], failure["release_status"]),
                         ("MDIO_ERROR", "MDIO_ERROR", "OK"))
        self.assertEqual((failure["mutation_attempted"], failure["mutation_possible"],
                          failure["verified"], slot["cleanup_owed"]), ("1", "1", "0", "1"))
        self.assertEqual(int(after["attempted"]) - int(before["attempted"]), 9)
        self.assertEqual(int(after["completed"]) - int(before["completed"]), 8)

    def test_analog_bridge_cancellation_and_independent_restoration(self):
        result = invoke(b"$pause\ncontrols analog 0x28,0x2f,0x18,8,0 keep keep keep keep\n"
                        b"$tick 6\nowner cancel 2\n$resume\n$fail-next 0\n"
                        b"recovery reinitialize none\nrecovery reinitialize none\n", ATTEST)
        rows = records(result.stdout)
        cancelled, failed, restored = (request(rows, number)[-1] for number in (1, 3, 4))
        self.assertEqual((cancelled["primary"], cancelled["mutation_completed"], cancelled["cleanup_owed"],
                          cancelled["vod2"]), ("CANCELLED", "1", "1", "33808"))
        self.assertEqual((failed["primary"], failed["cleanup_owed"]), ("MDIO_ERROR", "1"))
        self.assertEqual((restored["primary"], restored["cleanup_owed"], restored["persistent_restored"]),
                         ("OK", "0", "1"))
        self.assertEqual(len({cancelled["op"], failed["op"], restored["op"]}), 3)

    def test_board_permission_is_separate_from_cli_parameters(self):
        stream = (b"cached transport\n"
                  b"controls pins led,8,keep,normal keep keep keep keep keep keep keep\n"
                  b"controls interface rmii25 0 0 1518 50 0 keep keep\n"
                  b"controls quality keep keep keep on\n"
                  b"controls analog keep keep slow keep keep\n"
                  b"cached transport\n")
        for options in ((), (ATTEST, "--deny-maintenance")):
            rows = records(invoke(stream, *options).stdout)
            for row in rows[1:-1]:
                self.assertEqual(row["primary"], "BUSY")
                self.assertEqual(row.get("gate"), None if options else "board")
            self.assert_silent(rows[0], rows[-1])

    def test_expanded_help_remains_complete_in_fixed_console(self):
        output = invoke(b"help\nlocal console\nhelp controls\nlocal console\n").stdout
        self.assertIn(b"<clock> <isolate>", output)
        self.assertIn(b"tx-error or clock\r\n", output)
        self.assertIn(b"Cached analog setting validity and raw words\r\n", output)
        console_rows = [row for row in records(output) if row["cmd"] == "local.console"]
        self.assertEqual([row["dropped_human"] for row in console_rows], ["0", "0"])


if __name__ == "__main__":
    unittest.main()
