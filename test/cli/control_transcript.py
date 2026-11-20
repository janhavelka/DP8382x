"""Execute the typed control families through the real portable CLI and core."""
import unittest

from fixture_transcript import invoke, records, request


class ControlTranscript(unittest.TestCase):
    def run_controls(self, data, *options):
        result = invoke(data, "--attest-features", *options)
        return records(result.stdout), result

    def terminal(self, rows, req, primary="OK"):
        result, = [row for row in request(rows, req) if row["state"] == "terminal"]
        self.assertEqual(result["primary"], primary, result)
        return result

    def test_irq_policy_stimulus_and_single_service(self):
        rows, result = self.run_controls(
            b"irq configure 0x20 2 wake intr 2\nirq status\nirq test-start\n"
            b"irq status\nirq test-stop\n$inject-events\nirq service\n"
            b"cached transport\nirq cached-service\ncached transport\n", "--trace-writes")
        for req in range(1, 10):
            self.terminal(rows, req)
        configured = self.terminal(rows, 2)
        self.assertEqual((configured["physcr"], configured["misr1"], configured["misr2"]), ("11", "32", "2"))
        self.assertEqual(self.terminal(rows, 4)["test_active"], "1")
        sampled = self.terminal(rows, 6)
        self.assertEqual((sampled["misr1"], sampled["misr2"], sampled["recr_requested"]),
                         (str(0xA520), str(0x5A02), "1"))
        self.assertEqual(self.terminal(rows, 8)["misr1"], sampled["misr1"])
        self.assertEqual(self.terminal(rows, 7)["attempted"], self.terminal(rows, 9)["attempted"])
        self.assertIn(b"mdio-write 1f 0011 000f", result.stderr)
        self.assertIn(b"mdio-write 1f 0011 000b", result.stderr)

    def test_power_policy_and_source_gate(self):
        rows, result = self.run_controls(
            b"power configure down 1 1\npower read\npower configure normal 0 0\npower status\n"
            b"cached transport\npower configure deep 0 0\npower configure normal 0 1\ncached transport\n",
            "--trace-writes")
        self.assertEqual(self.terminal(rows, 2)["requested_down"], "1")
        self.assertEqual(self.terminal(rows, 2)["indicated_down"], "1")
        self.assertEqual(self.terminal(rows, 4)["requested_down"], "0")
        self.terminal(rows, 6, "UNSUPPORTED")
        self.terminal(rows, 7, "INVALID_CONFIG")
        self.assertEqual(self.terminal(rows, 5)["attempted"], self.terminal(rows, 8)["attempted"])
        writes = result.stderr.splitlines()
        self.assertLess(writes.index(b"mdio-write 1f 0011 6000"), writes.index(b"mdio-write 1f 0000 3900"))
        self.assertLess(writes.index(b"mdio-write 1f 0000 3900"), writes.index(b"mdio-write 1f 0011 e000"))

    def test_typed_reads_require_fresh_accepted_identity(self):
        rows, _ = self.run_controls(
            b"configuration invalidate\ncached transport\npower read\nirq service\nwol read\n"
            b"cached transport\nconnection probe\npower read\ncached transport\n")
        for req in (3, 4, 5):
            self.terminal(rows, req, "NOT_INITIALIZED")
        self.assertEqual(self.terminal(rows, 2)["attempted"], self.terminal(rows, 6)["attempted"])
        self.terminal(rows, 7)
        self.terminal(rows, 8)
        self.assertEqual(int(self.terminal(rows, 9)["attempted"]) - int(self.terminal(rows, 6)["attempted"]), 5)

    def test_eee_recipe_and_forced_lpi_separate_exit(self):
        rows, result = self.run_controls(
            b"configuration apply an 8 none auto 1\neee configure on 2 0\nlink read\neee read\n"
            b"power read\nlpi start\nlpi status\nlpi stop\nlpi status\neee configure off 0 0\n",
            "--trace-writes")
        for req in range(1, 11):
            self.terminal(rows, req)
        self.assertEqual((self.terminal(rows, 4)["negotiated_valid"], self.terminal(rows, 4)["negotiated"]), ("1", "1"))
        self.assertEqual(self.terminal(rows, 6)["lpi"], "1")
        self.assertEqual(self.terminal(rows, 6)["attempted"], "9")
        self.assertEqual(self.terminal(rows, 7)["lpi"], "1")
        self.assertEqual(self.terminal(rows, 8)["lpi"], "0")
        self.assertEqual(self.terminal(rows, 9)["lpi"], "0")
        writes = result.stderr.splitlines()
        # Bit 1 established by the opaque recipe is preserved by the LPI RMW.
        expected = (b"mdio-write 1f 001f 8000", b"mdio-write 1f 0123 0800", b"mdio-write 1f 04d1 108b",
                    b"mdio-write 1f 04d1 008b")
        after = 0
        for write in expected:
            after = writes.index(write, after) + 1

    def test_secure_wake_reads_and_w0c_without_secret_output(self):
        rows, result = self.run_controls(
            b"wol configure secure status 1 level 0 11:22:33:44:55:66 2a:2b:2c:2d:2e:2f\n"
            b"wol read\nwol acknowledge\nwol status\ncached transport\nregisters read mmd 31 0x04a5\n"
            b"registers dump mmd 31 0x04a4 0x04a6\ncached transport\n", "--trace-writes")
        self.assertEqual(self.terminal(rows, 2)["match_word_mask"], "63")
        self.assertEqual(int(self.terminal(rows, 2)["control"]) & 0x0800, 0x0800)
        self.assertEqual(int(self.terminal(rows, 4)["control"]) & 0x0800, 0)
        self.terminal(rows, 6, "INVALID_PARAM")
        self.terminal(rows, 7, "INVALID_PARAM")
        self.assertEqual(self.terminal(rows, 5)["attempted"], self.terminal(rows, 8)["attempted"])
        writes = result.stderr.splitlines()
        self.assertLess(writes.index(b"mdio-write 1f 04a2 2211"), writes.index(b"mdio-write 1f 04a3 4433"))
        self.assertLess(writes.index(b"mdio-write 1f 04a3 4433"), writes.index(b"mdio-write 1f 04a4 6655"))
        self.assertIn(b"mdio-write 1f 04a0 11a1", writes)
        self.assertIn(b"mdio-write 1f 04a5 redacted", writes)
        for secret in (b"2a:2b:2c:2d:2e:2f", b"2b2a", b"2d2c", b"2f2e"):
            self.assertNotIn(secret, result.stdout)
            self.assertNotIn(secret, result.stderr)

    def test_reject_invalid_or_unapproved_before_transferring(self):
        data = (b"cached transport\nirq configure 0x100 2 wake intr 2\n"
                b"eee configure on 1 0\nwol configure secure status 1 level 0 11:22:33:44:55:66\n"
                b"irq service 1 2\ncached transport\n")
        rows, _ = self.run_controls(data)
        for req in range(2, 6):
            self.terminal(rows, req, "INVALID_PARAM")
        self.assertEqual(self.terminal(rows, 1)["attempted"], self.terminal(rows, 6)["attempted"])
        data = (b"cached transport\nirq configure 32 2 wake intr 2\npower configure down 0 0\n"
                b"eee configure off 0 0\nlpi start\nwol configure magic status 1 level 0 11:22:33:44:55:66\n"
                b"cached transport\n")
        for options in ((), ("--attest-features", "--deny-maintenance")):
            rows = records(invoke(data, *options).stdout)
            for req in range(2, 7):
                self.terminal(rows, req, "BUSY")
            self.assertEqual(self.terminal(rows, 1)["attempted"], self.terminal(rows, 7)["attempted"])

    def test_irq_cancellation_partial_service_and_cleanup_fault(self):
        rows, _ = self.run_controls(
            b"$pause\nirq configure 32 2 wake intr 2\n$tick 1\nowner cancel 2\n$resume\n"
            b"irq configure 32 2 wake intr 2\nirq test-start\n$fail-next 1\nirq test-stop\nirq status\n"
            b"irq test-stop\nrecovery reset hard\n$inject-events\nirq service 1 5 5\ncached events\n")
        self.terminal(rows, 1, "CANCELLED")
        self.terminal(rows, 2)
        self.terminal(rows, 3)
        self.terminal(rows, 4)
        stopped = self.terminal(rows, 5, "MDIO_ERROR")
        self.assertEqual(stopped["cleanup_owed"], "1")
        self.assertEqual(self.terminal(rows, 6)["test_active"], "1")
        cleared = self.terminal(rows, 7, "INVALID_CONFIG")
        self.assertEqual((cleared["test_active"], cleared["cleanup_owed"]), ("0", "1"))
        self.terminal(rows, 8)
        self.terminal(rows, 9, "OPERATION_TIMEOUT")
        self.assertEqual(self.terminal(rows, 10)["misr1"], str(0xA520))

    def test_irq_cleanup_advances_while_console_sink_stalls(self):
        rows, _ = self.run_controls(
            b"irq configure 32 2 wake intr 2\nirq test-start\n$pause\n$output-stall 50\n"
            b"irq test-stop\n$tick 12\nowner status\n$resume\n")
        restored = self.terminal(rows, 3)
        self.assertEqual((restored["cleanup_owed"], restored["persistent_restored"]), ("0", "1"))
        sampled = self.terminal(rows, 4)
        self.assertEqual((sampled["slot"], sampled["cleanup_owed"]), ("idle", "0"))


if __name__ == "__main__":
    unittest.main()
