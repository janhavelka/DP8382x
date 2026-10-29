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
        raise OverflowError("time/token representation exhausted")
    return total


class OperationTimingDesignTests(unittest.TestCase):
    def test_atomic_batch_and_reset_subsequence_reserve(self):
        # Independent synthetic bounds: acquire 20, frame 100, release 20,
        # fixed overhead 10 us. These are not ESP-IDF timing measurements.
        bounds = {frames: 20 + 100 * frames + 20 + 10
                  for frames in (1, 2, 4, 8)}
        self.assertEqual(bounds, {1: 150, 2: 250, 4: 450, 8: 850})
        # Reset RMW, conservative wait-anchor lag, 10 ms policy interval,
        # first completion read. Full initialization/restore costs are additional.
        reserve = bounds[2] + 1 + 10_000 + bounds[1]
        self.assertEqual(reserve, 10_401)
        self.assertFalse(reserve < 10_401)  # Exclusive deadline, equality rejects.
        self.assertTrue(reserve < 10_402)
        self.assertFalse(reserve < 9_999)   # Cannot issue then omit completion poll.
        self.assertFalse(2 <= 1)           # One batch/poll: command needs a later read.
        self.assertTrue(2 <= 2)

    def test_zero_and_insufficient_frame_allowance_cannot_split_mmd(self):
        for allowance in range(4):
            with self.subTest(allowance=allowance):
                self.assertFalse(4 <= allowance)
        self.assertTrue(4 <= 4)
        self.assertFalse(8 <= 7)            # Reselected MMD RMW stays indivisible.
        self.assertTrue(8 <= 8)

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

    def test_deadline_uncertainty_is_not_proven_lateness(self):
        raw, lag, deadline = 99, 2, 100
        self.assertLess(raw, deadline)
        self.assertGreaterEqual(checked_add(raw, lag), deadline)
        self.assertFalse(raw >= deadline)  # Deny publication without late=true.
        self.assertTrue(100 >= deadline)   # Exact lower deadline is proved late.
        # A quantized conservative completion stamp cannot alone prove freshness.
        recorded_ms = 10_201 // 1_000
        observation_upper = recorded_ms * 1_000 + 999
        lower_age = max(0, 12_000 - observation_upper)
        upper_age = 12_001 - 9_900          # Owner's actual pre-acquisition lower bound.
        self.assertEqual((lower_age, upper_age), (1_001, 2_101))
        self.assertLess(lower_age, 1_500)
        self.assertGreater(upper_age, 1_500)  # A 1500-us freshness limit is unproved.

    def test_nonwrapping_time_and_lifetime_token_exhaustion(self):
        self.assertEqual(checked_add(UINT64_MAX - 1, 1), UINT64_MAX)
        with self.assertRaises(OverflowError):
            checked_add(UINT64_MAX, 1)      # No time wrap or token reuse as zero.
        with self.assertRaises(OverflowError):
            checked_add(UINT64_MAX - 999, 1_000)  # Unrepresentable wait anchor.
        self.assertLess(0, UINT64_MAX)      # Wrap is a backward clock observation.

    def test_frozen_clock_cannot_satisfy_wait_in_either_finite_phase(self):
        # Enumerate a finite call recipe, not a fake operation state machine.
        primary_polls, cleanup_polls, frozen, due = 3, 2, 100, 200
        samples = [frozen] * (primary_polls + cleanup_polls)
        eligible = [sample for sample in samples if sample >= due]
        self.assertEqual(len(samples), 5)
        self.assertEqual(eligible, [])      # Cleanup cannot bypass the same wait.
        self.assertEqual(primary_polls - len(samples[:3]), 0)
        self.assertEqual(cleanup_polls - len(samples[3:]), 0)

    def test_cleanup_keeps_its_original_absolute_deadline(self):
        primary_deadline, cleanup_deadline = 100, 200
        self.assertGreater(cleanup_deadline, primary_deadline)
        upper_now = checked_add(199, 2)
        self.assertEqual(max(0, cleanup_deadline - upper_now), 0)
        # No fresh 100-unit window is granted at this late cleanup attempt.
        self.assertNotEqual(cleanup_deadline, upper_now + 100)
        # Minimum cleanup route must fit the reserved window before mutation.
        self.assertFalse(150 < cleanup_deadline - primary_deadline)


if __name__ == "__main__":
    unittest.main()
