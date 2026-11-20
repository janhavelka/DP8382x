"""Check the actual read-only CLI selftest and its partial failure records."""
import unittest

from fixture_transcript import records, request, run


class SelftestTranscript(unittest.TestCase):
    def test_selftest_uses_two_safe_batches_even_without_maintenance(self):
        out = records(run(b"cached transport\ndiagnostics selftest\ncached transport\n", "--deny-maintenance"))
        first, second, terminal = request(out, 2)
        self.assertEqual((first["check"], first["attempted"], int(first["id1"]), int(first["id2"])),
                         ("identity", "2", 0x2000, 0xA131))
        self.assertEqual((second["check"], second["attempted"], second["value"]), ("reset_status", "1", "0"))
        self.assertEqual((terminal["primary"], terminal["rounds"], terminal["attempted"], terminal["managed"]),
                         ("OK", "2", "3", "0"))
        self.assertEqual(int(out[-1]["attempted"]) - int(out[0]["attempted"]), 3)
        self.assertEqual(int(out[-1]["batches"]) - int(out[0]["batches"]), 2)

    def test_failed_second_id_stops_before_the_next_check(self):
        baseline, = records(run(b"cached transport\n"))
        out = records(run(b"diagnostics selftest\n", "--fail-after", str(int(baseline["attempted"]) + 1)))
        sample, terminal = out
        self.assertEqual((sample["id1_valid"], sample["id2_valid"], sample["primary"]), ("1", "0", "MDIO_ERROR"))
        self.assertEqual((terminal["primary"], terminal["detail"], terminal["rounds"], terminal["attempted"]),
                         ("MDIO_ERROR", "-43", "0", "2"))


if __name__ == "__main__":
    unittest.main()
