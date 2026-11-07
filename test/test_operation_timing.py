"""Executable prompt-8 timing design vectors, not a PHY operation implementation.

Synthetic callback bounds exercise the arithmetic in docs/api-contracts.md.
State/token/result scenarios remain acceptance recipes for the first real driver
consumer. No stub reports successful hardware behavior here.
"""

import unittest


UINT64_MAX = (1 << 64) - 1


def checked_add(left, right):
    """Use unbounded Python integers to check the proposed uint64 wire-free math."""
    total = left + right
    if left < 0 or right < 0 or total > UINT64_MAX:
        raise OverflowError("time representation overflow")
    return total


class OperationTimingDesignTests(unittest.TestCase):
    def test_reset_admission_four_lags_and_hard_floor(self):
        # OP-6: literal one-frame command/read, fixed50 + frame100, lag1.
        bound = 150
        self.assertEqual(4 * 1 + bound + 2000 + bound, 2304)
        self.assertFalse(2304 < 2304)
        self.assertTrue(2304 < 2305)
        # 100 Hz clock: quantization/overshoot require four full lags.
        self.assertEqual(4 * 10000 + bound + 2000 + bound, 42300)
        self.assertFalse(42300 < 5000 + 2 * bound)
        self.assertTrue(42300 < 42301)

    def test_zero_and_insufficient_frame_allowance_cannot_split_mmd(self):
        for allowance in range(4):
            with self.subTest(allowance=allowance):
                self.assertFalse(4 <= allowance)
        self.assertTrue(4 <= 4)
        self.assertFalse(5 <= 4)            # Nonincrementing MMD RMW stays indivisible.
        self.assertTrue(5 <= 5)

    def test_post_callback_anchor_crosses_tick_boundary(self):
        before, command_completed, after = 9_900, 10_150, 10_200
        upper = checked_add(after, 1)       # Proven synthetic clock-return lag.
        due = checked_add(upper, 2_000)
        self.assertEqual(due, 12_201)
        self.assertLess(before + 2_000, command_completed + 2_000)
        self.assertFalse(12_200 >= due)
        self.assertTrue(12_201 >= due)

    def test_floor_tick_requires_lag_and_elapsed_recheck(self):
        actual_completion, raw_after, proved_lag = 10_150, 10_000, 10_000
        self.assertLess(raw_after, actual_completion)
        due = checked_add(checked_add(raw_after, proved_lag), 2_000)
        self.assertEqual(due, 22_000)
        self.assertFalse(20_000 >= due)
        self.assertTrue(30_000 >= due)      # Next eligible lower tick sample.
        self.assertEqual(2 // 10, 0)        # 2 ms rounded down at a 100 Hz tick.

    def test_nonwrapping_time_overflow(self):
        self.assertEqual(checked_add(UINT64_MAX - 1, 1), UINT64_MAX)
        with self.assertRaises(OverflowError):
            checked_add(UINT64_MAX, 1)      # No time wrap.
        with self.assertRaises(OverflowError):
            checked_add(UINT64_MAX - 999, 1_000)  # Unrepresentable wait anchor.
        self.assertLess(0, UINT64_MAX)      # Wrap is a backward clock observation.

    def test_frozen_clock_poll_bound(self):
        # Brute force placement of available one-frame batches between stalls.
        # The first poll never stalls; each batch resets the consecutive count.
        for frames in range(5):
            for limit in range(1, 6):
                polls, stalled = 1, 0
                for _ in range(frames):
                    polls += limit - 1  # Spend all nonterminal wait polls.
                    polls += 1          # A batch resets the stall counter.
                polls += limit          # Final frozen wait terminates.
                self.assertEqual(polls, (frames + 1) * limit + 1)

    def test_normal_tick_wait_does_not_count_all_polls(self):
        stalled = 0
        previous = None
        for raw in range(0, 2001, 100):
            stalled = stalled + 1 if raw == previous else 0
            self.assertLess(stalled, 3)
            previous = raw
        self.assertEqual(previous, 2000)


if __name__ == "__main__":
    unittest.main()
