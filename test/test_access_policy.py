"""Check policy/catalog coverage, not numeric silicon correctness.

Original-PDF literals in the C++ suites remain the independent behavioral oracle.
"""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class AccessPolicyTests(unittest.TestCase):
    def test_named_addresses_and_every_field_mask_match_ledger(self):
        ledger = json.loads((ROOT / "docs/registers/dp83826i.json").read_text())
        header = (ROOT / "include/DP8382x/CommandTable.h").read_text()
        rows = re.findall(r"constexpr uint(?:8|16)_t (\w+) = (0x[0-9A-Fa-f]+);", header)
        self.assertEqual(len(rows), len(dict(rows)), "Duplicate numeric symbol")
        constants = {name: int(value, 16) for name, value in rows}
        for register in ledger["registers"]:
            self.assertEqual(constants["REG_" + register["id"]], int(register["address"], 16))
            for field in register["fields"]:
                symbol = "MASK_" + re.sub(r"[^a-zA-Z0-9]", "_", field["id"]).upper()
                if field["id"] == "ANAR.pause":
                    symbol = "MASK_ANAR_SYMMETRIC_PAUSE"  # Existing PAUSE names the pair.
                self.assertEqual(constants[symbol], int(field["mask"], 16), field["id"])

    def test_every_ledger_address_has_one_policy(self):
        ledger = json.loads((ROOT / "docs/registers/dp83826i.json").read_text())
        source = (ROOT / "src/DP8382x.cpp").read_text()
        rows = re.findall(r"\{(0x[0-9A-Fa-f]+), Access::(\w+), [^}]+\}, // (\w+)", source)
        self.assertEqual(len(rows), len({int(a, 16) for a, _, _ in rows}))
        self.assertEqual({(name, int(a, 16)) for a, _, name in rows},
                         {(r["id"], int(r["address"], 16)) for r in ledger["registers"]})
        exceptions = {"CDCR", "ANER", "10BTSCR", "REGCR", "ADDAR"}
        safe = {r["id"] for r in ledger["registers"]
                if r["status"] != "RECIPE" and int(r["reservedMask"], 16) != 65535
                and r["id"] not in exceptions
                and not any(f["access"] in {"RC", "RCH"} for f in r["fields"])}
        self.assertEqual({name for _, kind, name in rows if kind == "SAFE_READ"}, safe)


if __name__ == "__main__":
    unittest.main()
