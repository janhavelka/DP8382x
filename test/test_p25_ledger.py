"""Independent SNLS638C research vectors. No MDIO or fixture operations."""

from collections import defaultdict
from copy import deepcopy
from pathlib import Path
import re
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from verify_ledger import load_ledger
from verify_p25_ledger import validate


class P25LedgerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = load_ledger(ROOT / "docs/registers/dp83825i.json")
        cls.reg = {r["offset"]: r for r in cls.data["registers"]}

    def test_every_table_and_procedure(self):
        validate(self.data)
        for member, value in (("devad", 31), ("wireOffset", "1000")):
            trial = deepcopy(self.data)
            next(r for r in trial["registers"] if r["offset"] == "1000")[member] = value
            with self.assertRaisesRegex(ValueError, "wire address"):
                validate(trial)

    def test_identity_role_and_effects(self):
        identity = self.data["identity"]
        self.assertEqual((identity["id1"], identity["id2"], identity["model"],
                          identity["revisions"], identity["strapAddresses"]),
                         ("2000", "A140", 0x14, [0], [0, 1, 2, 3]))
        self.assertEqual(self.reg["0002"]["headingReset"], "2000h")
        fields = {f["bits"]: int(f["reset"][:-1], 16) for f in self.reg["0003"]["fields"]}
        self.assertEqual(fields["15-10"] << 10 | fields["9-4"] << 4 | fields["3-0"], 0xA140)
        role = self.data["role"]
        self.assertEqual(role["allowed"], {"LEADER": [0, 0], "FOLLOWER": [1, 1]})
        self.assertEqual(role["strap"], {"devad": 31, "wireOffset": "0467", "mask": "0008"})
        self.assertEqual(role["confirmation"], {"direct": "17", "mask": "0080", "fifoRcMask": "000C"})
        self.assertEqual(role["clockLossGuards"], {"0000": "0400", "0305": "0007", "0428": "0004"})
        self.assertEqual(role["settingsMask"], "0013")
        self.assertEqual(self.reg["0017"]["headingReset"], "0061h")
        fifo = {f["bits"] for f in self.reg["0017"]["fields"] if f["access"] == "RC"}
        self.assertEqual(fifo, {"3", "2"})
        self.assertEqual(self.data["ownerAccess"]["irqSource"], ["0012", "0013"])
        self.assertEqual(self.data["ownerAccess"]["selectorsDirectOnly"], ["000D", "000E"])
        self.assertTrue({"000C", "000F", "0012", "0013", "0017"}
                        <= set(self.data["ownerAccess"]["collectorDirect"]))
        self.assertEqual(self.data["timing"],
                         {"mdcMaxHz": 20000000, "resetPulseMinUs": 25, "releaseToSmiMaxUs": 2000,
                          "porToSmiMaxUs": 50000, "lateClockStableBeforeReleaseMinUs": 100,
                          "resetToLeaderClockNominalUs": 200, "resetToFlpNominalMs": 1500})

    def test_functional_mode_sequences(self):
        # Tables 6-5..7, including repeated common settings and terminal restart.
        common = """0416:1F30 040D:000D 0429:0200 030B:0BC0 030C:0011
                    033C:0001 0311:0000 0313:06E3 033A:579C"""
        tail = "033D:8110 031B:0048 001F:4000"
        eee = """0130:4750 0123:0800 030F:0400 04D4:6633 04D5:027F 04D6:01B0
                 04D7:01B0 031F:FC36 031C:1103 0101:0882 010A:2010 04CE:00FF
                 04CD:A5A5 0308:0982 04CF:231D 04D0:0F8F 033E:861E 04D1:00C2 04D2:215B"""
        for name, middle in (("powerOptimized", "0404:0000"), ("reachOptimized", "0404:0080"),
                             ("reachWithEee", "0404:0080 " + eee)):
            expected = [s.split(":") for s in (common + " " + middle + " " + tail).split()]
            self.assertEqual(self.data["modeRecipes"][name], expected)
        self.assertEqual(self.data["vodCoarse"][-1], ["50", "00E0", "0008"])
        self.assertEqual(self.data["secureOnExample"]["words"],
                         {"04A5": "413C", "04A6": "449D", "04A7": "5EBB"})

    def test_original_pdf_cells(self):
        # The ordinary ledger checker has no third-party dependencies. When the
        # research parser is installed, compare every source row independently.
        try:
            import pymupdf
        except ImportError:
            self.skipTest("PyMuPDF is optional for original-PDF cross-checking")
        pdf = ROOT / "docs/reference/source/TI-DP83825I-SNLS638C.pdf"
        actual = []
        detailed_headings = []
        with pymupdf.open(pdf) as document:
            section = 0
            field = None
            for page in document[38:81]:
                lines = defaultdict(list)
                for x, y, _, _, text, *_ in page.get_text("words"):
                    if 45 < y < 733:
                        lines[round(y, 1)].append((x, text))
                for _, words in sorted(lines.items()):
                    words.sort()
                    left = words[0][0]
                    text = " ".join(s for x, s in words)
                    heading = re.fullmatch(
                        r"(7\.\d+) (.+?) \(Offset = ([0-9A-F]+)h\) \[Reset = ([0-9A-F]+h)\]", text)
                    if left < 60 and heading:
                        section_id, name, offset, reset = heading.groups()
                        detailed_headings.append((section_id, name, offset.zfill(4), reset))
                        section += 1
                        field = None
                    bits = next((s for x, s in words if 68 < x < 83
                                 and re.fullmatch(r"\d+(?:-\d+)?", s)), None)
                    name = "".join(s for x, s in words if 106 < x < 204)
                    if section and bits and name:
                        access = "".join(s for x, s in words if 205 < x < 254)
                        reset = "".join(s for x, s in words if 255 < x < 302)
                        field = [section, bits, name, access, reset]
                        actual.append(field)
                    elif field and name != "Field" and 106 < left < 111:
                        field[2] += name
            summary = "\n".join(page.get_text() for page in document[36:39])
            headings = re.findall(r"\n([A-F0-9]+)h\s*\n[^\n]+\nSection 7\.(\d+)", summary)
        self.assertEqual([(a.zfill(4), int(s)) for a, s in headings],
                         [(r["offset"], i) for i, r in enumerate(self.data["registers"], 1)])
        self.assertEqual(detailed_headings,
                         [(r["section"], r["name"], r["offset"], r["headingReset"])
                          for r in self.data["registers"]])
        expected = [[i, f["bits"], f["name"].replace(" ", ""), f["access"], f["reset"]]
                    for i, r in enumerate(self.data["registers"], 1) for f in r["fields"]]
        self.assertEqual(actual, expected)
