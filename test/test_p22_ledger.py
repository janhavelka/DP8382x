"""Independent DP83822 research vectors from the original TI register tables."""

from copy import deepcopy
import json
from pathlib import Path
import re
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from verify_p22_ledger import verify


def cell(text):
    return "".join(text.split())


class P22LedgerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ledger = json.loads((ROOT / "docs/registers/dp83822.json").read_text(encoding="utf-8"))
        cls.registers = {int(r["printedAddress"], 16): r for r in cls.ledger["registers"]}
        cls.owners = {p["id"]: p["owner"]["implementationStage"] for p in cls.ledger["procedures"]}

    def field(self, address, msb, lsb):
        return next(f for f in self.registers[address]["fields"] if (f["msb"], f["lsb"]) == (msb, lsb))

    def test_complete_records(self):
        self.assertEqual(verify(self.ledger), [])

    def test_original_pdf_field_cells(self):
        # Compare physical cell boundaries and all four source columns, including
        # continuations. The PDF dependency is optional for offline installations.
        try:
            import pymupdf
        except ImportError:
            self.skipTest("PyMuPDF is optional for the original-PDF comparison")
        actual = {}
        pdf = ROOT / "docs/reference/source/TI-DP83822-SNLS505H.pdf"
        with pymupdf.open(pdf) as doc:
            for page in doc[52:102]:
                captions = [(w[1], int(w[4][2:-1])) for w in page.get_text("words")
                            if re.fullmatch(r"8-\d+\.", w[4])]
                for table in page.find_tables().tables:
                    rows = table.extract()
                    if rows[0][:4] != ["BIT", "NAME", "TYPE", "DEFAULT"]:
                        continue
                    number = max((y, n) for y, n in captions if y < table.bbox[1])[1]
                    for row in rows[1:]:
                        if row[0] is None:  # grid nested in the FUNCTION cell
                            continue
                        bits = cell(row[0]).split(":")
                        key = number, int(bits[0]), int(bits[-1])
                        self.assertNotIn(key, actual)
                        # The PDF places subscript underscores on separate baselines.
                        actual[key] = cell(row[1]).replace("_", ""), cell(row[2]), cell(row[3])
        self.assertEqual(len(actual), 576)
        expected = {}
        for reg in self.ledger["registers"]:
            for f in reg["fields"]:
                if f["name"] == "UNLISTED":
                    continue
                key = reg["table"], f["msb"], f["lsb"]
                access = "" if key == (38, 11, 11) else cell(f["access"])
                expected[key] = cell(f["name"]).replace("_", ""), access, cell(f["reset"])
        self.assertEqual(actual.keys(), expected.keys())
        for key, value in actual.items():
            with self.subTest(table_and_bits=key):
                self.assertEqual(expected[key], value)

    def test_identity_and_selectors(self):
        self.assertEqual(int(cell(self.field(2, 15, 0)["reset"]), 2), 0x2000)
        identity = sum(int(cell(f["reset"]), 2) << f["lsb"] for f in self.registers[3]["fields"])
        self.assertEqual(identity, 0xA240)
        for printed, devad, offset in ((0x000D, None, 13), (0x000E, None, 14),
                                       (0x0465, 31, 0x465), (0x3000, 3, 0),
                                       (0x3016, 3, 0x16), (0x703D, 7, 0x3D)):
            reg = self.registers[printed]
            self.assertEqual((reg["devad"], int(reg["offset"], 16)), (devad, offset))
        self.assertEqual(self.ledger["encoding"]["mmd7ReadSelectors"], ["0x0007", "0x003D", "0x4007"])
        self.assertEqual([self.field(0x1F, bit, bit)["access"] for bit in (15, 14)],
                         ["R/W, W1S", "R/W, W1S"])

    def test_field_owners_and_adverse_reads(self):
        # Tables 8-11, 8-18/19/23, 8-29, 8-36, 8-55/60, 8-68, and 8-87/88.
        vectors = [
            (0x000A, 14, 14, 65, "R/W, RH"), (0x0012, 9, 9, 63, "R, RC"),
            (0x0012, 1, 1, 63, "RC/W"), (0x0017, 12, 12, 64, "R/W"),
            (0x0017, 11, 11, 64, "R/W"), (0x0042, 4, 4, 64, "R/W, W1S"),
            (0x0042, 3, 0, 64, "R/W"), (0x001E, 1, 1, 66, "R"),
            (0x003F, 9, 8, 67, "R/W"), (0x003F, 7, 4, 67, "R/W"),
            (0x0180, 7, 0, 66, "R"), (0x0185, 6, 0, 66, "R"),
            (0x0403, 11, 8, 65, "R/W"), (0x0403, 7, 4, 66, "R/W"),
            (0x04A0, 11, 11, 67, "W, RH/W1S"), (0x04A1, 5, 5, 67, "R, RC, RH/W1S"),
        ]
        for addr, high, low, stage, access in vectors:
            with self.subTest(address=hex(addr), bits=(high, low)):
                f = self.field(addr, high, low)
                self.assertEqual((self.owners[f["procedure"]], f["access"]), (stage, access))
        self.assertEqual(self.field(0x1E, 1, 1)["reset"], "1")
        self.assertEqual(self.field(0x04A0, 12, 12)["reset"], "1")
        self.assertEqual(self.field(0x04CC, 5, 0)["reset"], "01100")

    def test_strap_modes_and_diagnostic_observations(self):
        # Table 7-10, including mode 3 of RX_ER when COL selects FX.
        pins = self.ledger["strapModes"]["pins"]
        a = ["00", "10", "11", "01"]
        b = ["10", "00", "01", "11"]
        expected = {"COL": (29, 4, a), "RX_D0": (30, 1, b), "RX_D1": (31, 1, a),
                    "RX_D2": (32, 1, a), "RX_D3": (1, 1, b), "CRS": (27, 4, a),
                    "RX_ER": (28, 4, a), "RX_DV": (26, 1, ["00", "10", "01", "11"]),
                    "LED_0": (17, 4, ["X0", "TEST", "TEST", "X1"]),
                    "LED_1": (24, 1, ["0", "TEST", "TEST", "1"])}
        self.assertEqual(len(pins), len(expected))
        self.assertEqual({p["pin"]: (p["number"], p["defaultMode"], p["modes"]) for p in pins}, expected)
        self.assertEqual(self.field(0x0467, 11, 10)["name"], "COL Strap Mode")
        self.assertEqual(self.field(0x0467, 9, 8)["name"], "RX_ER Strap Mode")

    def test_conflict_and_timing_rejections(self):
        for section, identity, key, value in (
            ("resetDomains", "phyrcr-restart", "action", "C22 001F=8000"),
            ("resetDomains", "phyrcr-restart", "effect", "all"),
            ("resetDomains", "bmcr-pcs", "strapAction", "relatch"),
            ("gates", "fiber-interface", "nextEvidence", ""),
            ("gates", "regcr-functions", "procedure", "unused"),
            ("timings", "SNLS505H 6.8 T4", "start", "RESET_N release"),
            ("timings", "SNLS505H 6.6 T2", "qualifier", "TYP"),
            ("timings", "SNLS505H figure 6-1 note", "value", 1),
            ("timings", "SNLS505H 6.6 T4", "start", "power application"),
            ("timings", "SNLS505H 6.6 T5", "qualifier", "MAX"),
            ("timings", "SNLS505H 6.7 T2", "end", "SMI-ready for MDC preamble"),
            ("timings", "SNLS505H 6.20 T1 RGMII TYP", "conditions", "100BASE-TX MII"),
            ("timings", "SNLS505H 6.20 T1 MII MAX", "value", 48),
            ("timings", "SNLS505H 6.21 T2 RMII TYP", "conditions", "FAST_RXDV footnote 2"),
        ):
            with self.subTest(section=section, identity=identity, member=key):
                bad = deepcopy(self.ledger)
                row = next(r for r in bad[section] if r.get("id", r["source"]) == identity)
                row[key] = value
                self.assertTrue(verify(bad))

    def test_mixed_write_and_collector_masks(self):
        neutral = {int(r["printedAddress"], 16): int(r["neutralZeroMask"], 16)
                   for r in self.registers.values() if int(r["neutralZeroMask"], 16)}
        self.assertEqual(neutral, {0x0000: 0x8200, 0x001B: 0x8000, 0x001E: 0x8000,
                                   0x001F: 0xC000, 0x0042: 0x0010, 0x0155: 0x8000,
                                   0x04A0: 0x0800, 0x04A1: 0x00FF, 0x3000: 0x8000})
        self.assertEqual(self.registers[0x04A1]["ordinaryWriteMask"], "0x1000")
        self.assertEqual(0x101C & ~neutral[0x04A1], 0x1000)
        self.assertEqual(self.field(0x04A1, 4, 2)["writePolicy"], "reserved")
        collectors = {address for address, reg in self.registers.items() if reg["collector"]}
        self.assertEqual(collectors, {0x01, 0x06, 0x0F, 0x10, 0x12, 0x13, 0x14,
                                      0x15, 0x16, 0x17, 0x1A, 0x1B, 0x04A1, 0x3016})

    def test_full_pattern_engine(self):
        for index in range(64):
            address = 0x04A8 + index // 2
            low = 8 * (index % 2)
            pattern = self.field(address, low + 7, low)
            self.assertEqual(pattern["name"], f"Pattern Byte {index}")
            self.assertEqual(self.owners[pattern["procedure"]], 67)
        for index in range(4):
            mask = self.field(0x04C8 + index, 15, 0)
            self.assertEqual(mask["name"], f"Mask Bytes {16 * index} to {16 * index + 15}")

    def test_wol_packet_octets(self):
        # The printed MAC suffixes count from the opposite end to SNLA261A
        # tables 2-6 and 2-10. Compare the actual register words to packet bytes.
        wol = next(p for p in self.ledger["procedures"] if p["id"] == "wol")["wireExamples"]
        for words, octets, sample in (
            ("mac04A2to04A4", "macOctets", "00 17 83 F3 A1 38"),
            ("secureOn04A5to04A7", "secureOnOctets", "3C 41 9D 44 BB 5E"),
            ("pattern04A8to04AB", "patternOctets", "01 23 45 67 89 AB CD EF"),
        ):
            expected = bytes.fromhex(sample)
            self.assertEqual(bytes.fromhex(wol[octets].replace(":", "")), expected)
            self.assertEqual(b"".join(int(w, 16).to_bytes(2, "little") for w in wol[words]), expected)
        self.assertEqual(wol["mask04C8to04CB"], ["0xFF00", "0xFFFF", "0xFFFF", "0xFFFF"])

    def test_fiber_debug_cases(self):
        procs = {p["id"]: p for p in self.ledger["procedures"]}
        sd = procs["fiber-mode"]["sdBeforeModuleInsertion"]
        self.assertEqual((sd["pin"], sd["validHigh0465Bit0"], sd["validLow0465Bit0"]),
                         ("LED_1/GPIO1", 0, 1))
        faults = procs["fiber-fault"]["singleFiberLoss"]
        self.assertEqual([(r["dutLostPath"], r["dutDisableMask"], r["dut"], r["peer"]) for r in faults],
                         [("RX", "0x0040", "down", "may stay up"), ("TX", "0x0020", "may stay up", "down")])
        self.assertEqual(self.field(0x16, 4, 0)["procedure"], "loopback")

    def test_sfd_compatibility_and_optional_wake(self):
        bad = deepcopy(self.ledger)
        recipe = next(r for r in bad["recipes"] if r["id"] == "sfd-timing")
        self.assertEqual(recipe["interfaces"], ["MII"])
        self.assertEqual(int(recipe["value"], 16) & 0x0008, 0x0008)
        recipe["interfaces"].append("RMII")
        self.assertTrue(verify(bad))
        wake = next(p for p in self.ledger["procedures"] if p["id"] == "sfd")["optionalWakeExample"]
        self.assertEqual((wake["address"], wake["sourceWord"], wake["ordinaryWholeWord"]),
                         ("0x04A0", "0x1080", False))
        # CRC gate off, pattern enabled, and a previously sampled clear action:
        # adding the optional aid must preserve the filter, leave CRC off and emit zero to bit 11.
        neutral = int(self.registers[0x04A0]["neutralZeroMask"], 16)
        self.assertEqual((0x0802 & ~neutral) | int(wake["enableMask"], 16), 0x0082)

    def test_electrical_code_vectors(self):
        # Tables 8-68 and 8-80. The published FX code D is 0.976 V and the
        # impedance sequence has fractional steps; interpolation loses both.
        fx = "0.533 0.567 0.600 0.633 0.667 0.700 0.733 0.767 0.800 0.833 0.867 0.900 0.933 0.976 1.000 1.033"
        tx = "1.600 1.633 1.667 1.700 1.733 1.767 1.800 1.833 1.867 1.900 1.933 1.967 2.000 2.033 2.067 2.100"
        te = "3.200 3.233 3.267 3.300 3.333 3.367 3.400 3.433 3.467 3.500 3.533 3.567 3.600 3.633 3.667 3.700"
        termination = "99.25 91.13 84.24 78.31 73.17 68.65 64.66 61.11 58.07 55.18 52.57 50.20 48.03 46.04 44.20 42.51"
        for address, high, low, unit, values in ((0x403, 11, 8, "V", fx), (0x403, 7, 4, "V", tx),
                                                 (0x403, 3, 0, "V", te), (0x461, 4, 1, "ohm", termination)):
            with self.subTest(address=hex(address), bits=(high, low)):
                self.assertEqual(self.field(address, high, low)["codes"],
                                 {f"{i:04b}": f"{value} {unit}" for i, value in enumerate(values.split())})

    def test_missing_or_misassigned_evidence(self):
        for member in ("parts", "registers", "procedures"):
            with self.subTest(member=member):
                bad = deepcopy(self.ledger)
                bad[member].clear()
                self.assertTrue(verify(bad))
        bad = deepcopy(self.ledger)
        reg = next(r for r in bad["registers"] if r["printedAddress"] == "0x0012")
        reg["fields"].pop(0)
        self.assertTrue(verify(bad))
        bad = deepcopy(self.ledger)
        bad["procedures"][0]["owner"].pop("implementationStage")
        self.assertTrue(verify(bad))
        bad = deepcopy(self.ledger)
        reg = next(r for r in bad["registers"] if r["printedAddress"] == "0x0002")
        reg["fields"][0]["procedure"] = "unassigned"
        self.assertTrue(verify(bad))
        bad = deepcopy(self.ledger)
        next(p for p in bad["parts"] if p["part"] == "DP83822H")["fiber"] = True
        self.assertTrue(verify(bad))
        bad = deepcopy(self.ledger)
        next(p for p in bad["procedures"] if p["id"] == "fiber-mode")["owner"]["implementationStage"] = 64
        self.assertTrue(verify(bad))

    def test_wrong_wire_words_and_encodings(self):
        for key, value in (("devad", 31), ("offset", "0x703D"), ("space", "vendor")):
            bad = deepcopy(self.ledger)
            reg = next(r for r in bad["registers"] if r["printedAddress"] == "0x703D")
            reg[key] = value
            self.assertTrue(verify(bad))
        for key, value in (("fxEnableValue", "0x0001"), ("patternMaskIgnoredBit", 0),
                           ("rgmiiShiftWords", ["0x0000", "0x1000", "0x0800", "0x1800"]),
                           ("dataFunction", 2), ("wakeAckMask", "0x0400")):
            with self.subTest(encoding=key):
                bad = deepcopy(self.ledger)
                bad["encoding"][key] = value
                self.assertTrue(verify(bad))

    def test_reserved_command_and_recipe_protection(self):
        for address, mask in ((0x000B, 0x0800), (0x0101, 0x007F), (0x0107, 1),
                              (0x0017, 0x0008), (0x04A0, 0x0800), (0x000D, 1)):
            with self.subTest(address=hex(address)):
                bad = deepcopy(self.ledger)
                reg = next(r for r in bad["registers"] if int(r["printedAddress"], 16) == address)
                reg["ordinaryWriteMask"] = f"0x{mask:04X}"
                self.assertTrue(verify(bad))
        for key, value in (("ordinaryWrite", True), ("address", "0x0410"), ("value", "0x0457")):
            bad = deepcopy(self.ledger)
            recipe = next(r for r in bad["recipes"] if r["address"] == "0x0457")
            recipe[key] = value
            self.assertTrue(verify(bad))
        # A self-consistent new mask must still respect the open programming gates.
        for address, bit in ((0x0171, 0), (0x04A0, 6)):
            bad = deepcopy(self.ledger)
            reg = next(r for r in bad["registers"] if int(r["printedAddress"], 16) == address)
            field = next(f for f in reg["fields"] if f["lsb"] == bit)
            field["writePolicy"] = "owned-setting"
            mask = ((1 << (field["msb"] - bit + 1)) - 1) << bit
            reg["ordinaryWriteMask"] = f"0x{int(reg['ordinaryWriteMask'], 16) | mask:04X}"
            self.assertTrue(verify(bad))

    def test_snla383_trace_and_mse_read(self):
        for change in ("remove", "add", "swap", "optional-value", "mandatory-eee", "optional-reset",
                       "owner", "off-table-write"):
            with self.subTest(change=change):
                bad = deepcopy(self.ledger)
                trace = bad["recipes"]
                pos = next(i for i, r in enumerate(trace) if r["id"] == "cable-break-21")
                if change == "remove":
                    trace.pop(pos)
                elif change == "add":
                    trace.append(dict(trace[pos], id="new-step"))
                elif change == "swap":
                    trace[pos], trace[pos + 1] = trace[pos + 1], trace[pos]
                elif change == "optional-value":
                    trace[pos]["value"] = "0x0000"
                elif change == "mandatory-eee":
                    trace[pos]["optional"] = False
                elif change == "optional-reset":
                    trace[pos + 1]["optional"] = True
                elif change == "owner":
                    trace[pos]["procedure"] = "eee"
                else:
                    bad["reads"][0]["ordinaryWrite"] = True
                self.assertTrue(verify(bad))
