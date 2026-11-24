"""Independent wire records for the bounded schema and firmware verifier."""

import json
import unittest

from tools.dp8382x_hil_protocol import (
    Decoder, ProtocolError, identity, integer, validate_expected_identity,
)


COMMON = {
    "schema": "1", "boot": "17", "req": "1", "type": "result", "cmd": "help", "op": "0",
    "state": "terminal", "primary": "OK", "detail": "0", "cleanup": "none", "cleanup_detail": "0",
    "time_valid": "1", "time_us": "0", "deadline_valid": "0", "deadline_us": "0", "reason": "none",
}
IDENTITY = {
    "cmd": "session", "type": "session", "nonce": "test.17", "library": "0.1.0", "firmware": "unknown",
    "tree": "unknown", "sdk": "unknown", "target": "host", "backend": "sim-mdio", "clock": "simulated",
    "variant": "DP83826I", "interface": "RMII", "address": "1", "console": "host-stdio", "baud": "0",
}
EXPECT = {"target": "host", "backend": "sim-mdio", "variant": "DP83826I", "interface": "RMII", "address": 1}
HEALTH = {"cmd": "cached.health", "health": "OFFLINE", "initialized": "1", "successes": "3", "failures": "4",
          "consecutive": "4", "last_error": "MDIO_ERROR", "last_detail": "-2147483648", "last_ok_valid": "1",
          "last_ok_us": "0", "last_error_valid": "1", "last_error_us": "5"}
TRANSPORT = {"cmd": "cached.transport", "batches": "2", "attempted": "7", "completed": "6", "stopped": "0",
             "clock_invalid": "0", "controller_reusable": "1"}
OPERATION = {"cmd": "cached.operation", "slot": "idle", "token": "0", "slot_status": "RESULT_NOT_AVAILABLE",
             "slot_detail": "0", "cleanup_owed": "1", "temporary": "1", "slot_deadline_us": "0"}
SETTINGS = {"cmd": "cached.settings", "trust": "VERIFIED", "desired_valid": "1", "desired_mode": "forced",
            "desired_modes": "0", "desired_speed_mbps": "100", "desired_duplex": "full", "observed_valid": "1",
            "observed_stale": "0", "observed_mode": "forced", "observed_modes": "0", "observed_speed_mbps": "100",
            "observed_duplex": "full"}
VERIFY = {"cmd": "configuration.verify", "op": "93", "deadline_valid": "1", "deadline_us": "5000",
          "core_status": "OK", "core_detail": "0", "sdk_error": "0", "mutation_completed": "0",
          "mutation_possible": "0", "cleanup_owed": "0", "temporary": "0", "an_complete": "0",
          "np_requested": "0", "np_complete": "0", "np_owed": "0", "np_refused": "0", "np_base_event": "0",
          "np_base_valid": "0", "np_base": "0", "np_missing": "0", "np_tx": "0", "np_rx": "0",
          "np_duplicates": "0", "np_words": "none"}
SHA40 = "b184d991031c82242f148378bfd84a3f9423ee12"
SHA64 = SHA40 + "3c4e501622bbeefadcb45328"


def frame(**fields):
    values = {**COMMON, **fields}
    return b"@DP8382X " + " ".join(f"{key}={value}" for key, value in values.items()).encode("ascii") + b"\r\n"


def decode(data):
    decoder = Decoder()
    result = decoder.feed(data)
    decoder.finish()
    return result


def session(**fields):
    result, = decode(frame(**{**IDENTITY, **fields}))
    return result


class DecoderTests(unittest.TestCase):
    def test_every_split_and_one_byte_reads(self):
        wire = (b"echo session test.17\n\x1b[34m> @DP8382X no-candidate\x1b[0m\r\n" +
                frame(**IDENTITY) + b"> \r\n" + frame(req="2", **HEALTH))
        for split in range(len(wire) + 1):
            with self.subTest(split=split):
                decoder = Decoder(max_bytes=len(wire))
                output = decoder.feed(wire[:split]) + decoder.feed(wire[split:])
                decoder.finish()
                self.assertEqual([(row.boot, row.req, row.op, row.cmd) for row in output],
                                 [(17, 1, 0, "session"), (17, 2, 0, "cached.health")])
        decoder = Decoder()
        output = [row for byte in wire for row in decoder.feed(bytes((byte,)))]
        decoder.finish()
        self.assertEqual(len(output), 2)
        self.assertEqual(output[0].fields["nonce"], "test.17")

    def test_noise_is_discarded_at_column_zero_without_repair(self):
        good = frame()
        noise = (b" \r" + good + b"\x1b[0m" + good + b"> " + good + b"\xff" * 10000 + b"\n")
        output = decode(noise + good + b"incomplete human message")
        self.assertEqual([row.cmd for row in output], ["help"])
        self.assertEqual(decode(b"\x1b[31m> @DP8382X bad"), [])

    def test_corrupt_candidates_are_not_repaired(self):
        valid = frame()
        for candidate in (valid[:-2] + b"\n", valid[:-2] + b" \r\n", valid[:-2] + b"\x1b[0m\r\n",
                          b"@DP8382X\r\n", valid.replace(b"@DP8382X ", b"@DP8382X\t", 1),
                          valid.replace(b"cmd=help", b"cmd=he\x00lp"), valid.replace(b"cmd=help", b"cmd=h\xfflp"),
                          valid.replace(b" boot=17", b"\tboot=17"), valid.replace(b" boot=17", b"  boot=17"),
                          valid.replace(b" boot=17", b" Boot=17"), valid.replace(b" boot=17", b" bo-ot=17"),
                          valid.replace(b" boot=17", b""), valid[:-2] + b" boot=18\r\n",
                          valid[:-2] + b" surprising=extra\r\n", valid + b"@DP8382X malformed\r\n"):
            with self.subTest(candidate=candidate):
                with self.assertRaises(ProtocolError):
                    decode(candidate)

    def test_partial_candidate_and_noise_at_closure(self):
        for prefix in (b"@", b"@DP8382X", b"@DP8382X ", frame()[:-1], frame()[:-2]):
            with self.subTest(prefix=prefix):
                decoder = Decoder()
                self.assertEqual(decoder.feed(prefix), [])
                with self.assertRaisesRegex(ProtocolError, "unterminated"):
                    decoder.finish()
        self.assertEqual(decode(b"@not-the-protocol\xff"), [])

    def test_line_limit_and_total_limit_include_noise_and_delimiter(self):
        base = frame(cmd="diagnostics.selftest", payload="x")
        exact = frame(cmd="diagnostics.selftest", payload="x" * (1025 - len(base)))
        self.assertEqual(len(exact), 1024)
        self.assertEqual(len(decode(exact)), 1)
        decoder = Decoder()
        self.assertEqual(decoder.feed(exact[:-1]), [])
        with self.assertRaisesRegex(ProtocolError, "1024"):
            decoder.feed(b"x\n")
        for limit in (0, -1, True, 1.0):
            with self.subTest(limit=limit):
                with self.assertRaises(ProtocolError):
                    Decoder(limit)
        decoder = Decoder(max_bytes=6)
        self.assertEqual(decoder.feed(b"noise\xff"), [])
        with self.assertRaisesRegex(ProtocolError, "byte budget"):
            decoder.feed(b"\n")
        with self.assertRaises(ProtocolError):
            decoder.feed(b"")

    def test_unsigned_signed_bounds_and_exact_decimal_types(self):
        maximum, = decode(frame(boot="4294967295", req="18446744073709551615", time_us="18446744073709551615",
                                primary="MDIO_ERROR", detail="-2147483648"))
        minimum, = decode(frame(boot="0"))
        self.assertEqual((maximum.boot, maximum.req, minimum.boot), (4294967295, 18446744073709551615, 0))
        for overrides in ({"boot": "4294967296"}, {"req": "0"}, {"req": "18446744073709551616"},
                          {"req": "-1"}, {"req": "01"}, {"req": "1.0"}, {"req": "+1"},
                          {"detail": "-0"}, {"detail": "2147483648"}, {"detail": "-2147483649"},
                          {"op": "-1"}):
            with self.subTest(overrides=overrides):
                with self.assertRaises(ProtocolError):
                    decode(frame(**overrides))
        for value in (True, False, 1, None, "ï¼‘", "9" * 10000):
            with self.subTest(value=repr(value)[:30]):
                with self.assertRaises(ProtocolError):
                    integer(value, 0, 2)

    def test_common_enums_and_consistent_phase_clock_and_cleanup_fields(self):
        for overrides in ({"schema": "2"}, {"schema": "01"}, {"state": "pending"}, {"type": "diagnostic"},
                          {"primary": "UNKNOWN"}, {"cleanup": "IN_PROGRESS"}, {"primary": "IN_PROGRESS"},
                          {"primary": "OK", "detail": "2"}, {"cleanup": "OK", "cleanup_detail": "-5"},
                          {"cmd": "configuration.verify", "state": "admitted", "primary": "IN_PROGRESS", "op": "3",
                           "deadline_valid": "1", "deadline_us": "200", "detail": "5"},
                          {"cmd": "configuration.verify", "state": "admitted", "primary": "IN_PROGRESS", "op": "3",
                           "deadline_valid": "1", "deadline_us": "200", "cleanup": "OK"},
                          {"time_valid": "0", "time_us": "7"}, {"time_valid": "2"},
                          {"deadline_valid": "1"}, {"deadline_us": "1"}, {"cleanup_detail": "-7"},
                          {"reason": "new_reason"}, {"reason": "arity"}, {"type": "sample"},
                          {"reason": "callback", "primary": "BUSY"}, {"type": "session"}, {"cmd": "a..b"}):
            with self.subTest(overrides=overrides):
                with self.assertRaises(ProtocolError):
                    decode(frame(**overrides))
        regressed, = decode(frame(time_valid="0", time_us="0", deadline_valid="1", deadline_us="6",
                                  primary="INVALID_CONFIG", detail="5", cleanup="MDIO_ERROR", cleanup_detail="-4"))
        self.assertEqual((regressed.fields["deadline_us"], regressed.fields["cleanup_detail"]), ("6", "-4"))

    def test_selected_payloads_require_exact_fields_and_values(self):
        verify_admission = frame(cmd="configuration.verify", state="admitted", primary="IN_PROGRESS", op="93",
                                 deadline_valid="1", deadline_us="5000")
        expected_rejection = frame(cmd="unknown", primary="INVALID_PARAM", reason="command")
        output = decode(frame(**HEALTH) + frame(**TRANSPORT) + frame(**OPERATION) + frame(**SETTINGS) + verify_admission +
                        frame(**VERIFY) + expected_rejection)
        self.assertEqual([row.op for row in output], [0, 0, 0, 0, 93, 93, 0])
        for data in ({**HEALTH, "health": "ONLINE"}, {**HEALTH, "consecutive": "-1"},
                     {**HEALTH, "last_error_valid": "0"}, {**HEALTH, "extra": "1"},
                     {**TRANSPORT, "completed": "8"}, {**TRANSPORT, "controller_reusable": "yes"},
                     {**OPERATION, "token": "9"}, {**OPERATION, "slot_status": "OK"},
                     {**SETTINGS, "desired_valid": "0"}, {**SETTINGS, "observed_mode": "unknown"},
                     {"cmd": "cached.settings"},
                     {**VERIFY, "core_status": "IN_PROGRESS"}, {**VERIFY, "np_rx": "1"},
                     {**VERIFY, "np_words": "ABCD"}, {**VERIFY, "sdk_error": "-2147483649"},
                     {"cmd": "registers.read", "address": "2", "devad": "0", "value": "65536"}):
            with self.subTest(data=data):
                with self.assertRaises(ProtocolError):
                    decode(frame(**data))


class IdentityTests(unittest.TestCase):
    def test_console_identity_extension_requires_both_fields(self):
        for field in (b" console=host-stdio", b" baud=0"):
            with self.subTest(field=field), self.assertRaises(ProtocolError):
                decode(frame(**IDENTITY).replace(field, b""))

    def test_full_identity_and_version_serialize_as_stable_verified_values(self):
        for firmware in (SHA40, SHA64):
            with self.subTest(length=len(firmware)):
                record = session(firmware=firmware, tree="clean", sdk="v6.1", baud="115200", console="uart0")
                expected = {**EXPECT, "firmware": firmware, "tree": "clean", "sdk": "v6.1", "baud": 115200,
                            "console": "uart0", "clock": "simulated"}
                observed = identity(record, expected)
                self.assertEqual((observed["firmware"], observed["firmware_strength"], observed["baud"]),
                                 (firmware, "exact", 115200))
                self.assertNotIn("nonce", observed)
                self.assertEqual(json.loads(json.dumps(observed)), observed)
        version, = decode(frame(**{**IDENTITY, "cmd": "version", "type": "result", "nonce": "none"}))
        self.assertEqual(identity(version, EXPECT)["firmware_strength"], "unverified")

    def test_explicit_prefix_never_uses_a_short_report_to_prove_more(self):
        for observed in (SHA40[:7], SHA40[:12], SHA40, SHA64):
            checked = identity(session(firmware=observed), {**EXPECT, "firmware_prefix": SHA40[:7]})
            self.assertEqual((checked["firmware"], checked["firmware_strength"], checked["firmware_prefix"]),
                             (observed, "prefix", SHA40[:7]))
        for expected in ({"firmware": SHA40}, {"firmware_prefix": SHA40[:13]}, {"firmware_prefix": "0000000"}):
            with self.subTest(expected=expected):
                with self.assertRaisesRegex(ProtocolError, "mismatch"):
                    identity(session(firmware=SHA40[:12]), {**EXPECT, **expected})
        with self.assertRaises(ProtocolError):
            identity(session(firmware=SHA64), {**EXPECT, "firmware": SHA40})

    def test_declared_grade_requires_an_exact_match(self):
        for declared, other in (("DP83826E", "DP83826I"), ("DP83826I", "DP83826E"),
                                ("DP83826AI", "DP83826AE"), ("DP83826AE", "DP83826AI")):
            record = session(variant=declared)
            self.assertEqual(identity(record, {**EXPECT, "variant": declared})["variant"], declared)
            with self.assertRaisesRegex(ProtocolError, "identity mismatch: variant"):
                identity(record, {**EXPECT, "variant": other})
        for unsupported in ("DP83826", "DP83826A", "dp83826e"):
            with self.assertRaises(ProtocolError):
                session(variant=unsupported)
            with self.assertRaises(ProtocolError):
                validate_expected_identity({**EXPECT, "variant": unsupported})

    def test_unknown_and_opaque_revisions_cannot_satisfy_known_expectations(self):
        for reported in ("unknown", "fixture-image", "abc123"):
            record = session(firmware=reported)
            self.assertEqual(identity(record, EXPECT)["firmware"], reported)
            self.assertEqual(identity(record, EXPECT)["firmware_strength"], "unverified")
            with self.assertRaises(ProtocolError):
                identity(record, {**EXPECT, "firmware_prefix": SHA40[:7]})
        for pair in ({"tree": "clean"}, {"console": "uart0"}, {"variant": "DP83826I", "address": 0}):
            with self.subTest(pair=pair):
                with self.assertRaisesRegex(ProtocolError, "mismatch"):
                    identity(session(), {**EXPECT, **pair})

    def test_malformed_firmware_values_and_expected_set_are_rejected(self):
        for overrides in ({"address": "32"}, {"nonce": "q" * 65}, {"baud": "4294967296"},
                          {"tree": "clean"}, {"tree": "other"}, {"library": "0.01.0"}, {"library": "0.1.65536"},
                          {"interface": "RGMII"}, {"unexpected": "yes"}):
            with self.subTest(overrides=overrides):
                with self.assertRaises(ProtocolError):
                    session(**overrides)
        for overrides in ({"address": True}, {"baud": False}, {"address": "1"}, {"address": 32},
                          {"firmware": "unknown"}, {"firmware": SHA40.upper()}, {"firmware": SHA40[:7]},
                          {"firmware": SHA40, "firmware_prefix": SHA40[:7]}, {"firmware_prefix": SHA40[:6]},
                          {"firmware_prefix": "1" * 65}, {"backend": "unknown"}, {"target": "not asciiÃ©"},
                          {"firmware": None}, {"firmware_prefix": None}, {"tree": None}, {"sdk": None},
                          {"clock": None}, {"console": None}, {"baud": None},
                          {"wrong_option": None}):
            with self.subTest(overrides=overrides):
                with self.assertRaises(ProtocolError):
                    validate_expected_identity({**EXPECT, **overrides})
        with self.assertRaises(ProtocolError):
            identity(session(variant="unknown"), EXPECT)


if __name__ == "__main__":
    unittest.main()
