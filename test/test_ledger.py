"""Executing research-ledger checks; no driver or hardware behavior is claimed.

Literal numeric expectations were independently derived from SNLS647I original
PDF tables by the prompt-3 reviewer. Never generate these constants from JSON.
"""

from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools/verify_ledger.py"
SPEC = importlib.util.spec_from_file_location("verify_ledger", TOOL)
assert SPEC is not None and SPEC.loader is not None
ledger = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ledger)


class LedgerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = ledger.load_ledger(ledger.DEFAULT_LEDGER)

    def setUp(self):
        self.data = copy.deepcopy(self.original)

    def register(self, name):
        return next(r for r in self.data["registers"] if r["name"] == name)

    def field(self, name, mask):
        return next(f for f in self.register(name)["fields"] if int(f["mask"], 16) == mask)

    def rejected(self, expected):
        with self.assertRaisesRegex(ledger.LedgerError, expected):
            ledger.validate(self.data)

    def run_cli(self, contents):
        with tempfile.TemporaryDirectory(prefix="dp83826-ledger-") as directory:
            path = Path(directory) / "ledger.json"
            path.write_text(contents, encoding="utf-8")
            return subprocess.run([sys.executable, str(TOOL), str(path)],
                                  capture_output=True, text=True, timeout=20,
                                  check=False)

    def test_real_ledger_is_valid(self):
        ledger.validate(self.data)

    def test_cli_accepts_valid_temporary_copy(self):
        result = self.run_cli(json.dumps(self.data))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("structure and coverage only", result.stdout)

    def test_cli_rejects_malformed_temporary_records(self):
        mutations = {
            "missing schema": lambda d: d.pop("schemaVersion"),
            "wrong root type": lambda d: None,
            "duplicate semantic id": lambda d: d["registers"].append(copy.deepcopy(d["registers"][0])),
            "out-of-range C22": lambda d: d["registers"][0].update(address="0x8000"),
            "missing fields": lambda d: d["registers"][0].pop("fields"),
            "bad source": lambda d: d["registers"][0]["sources"][0].update(source="absent-source"),
            "unknown owner": lambda d: d["procedures"][0].update(feature="absent-feature"),
            "wrong scalar type": lambda d: d["registers"][0].update(width=True),
            "wrong address type": lambda d: d["registers"][0].update(space=[]),
            "wrong status type": lambda d: d["registers"][0].update(status={}),
            "wrong feature list": lambda d: d["features"][0].update(registers=None),
        }
        for case, change in mutations.items():
            with self.subTest(case=case):
                data = copy.deepcopy(self.original)
                change(data)
                if case == "wrong root type":
                    data = []
                result = self.run_cli(json.dumps(data))
                self.assertEqual(result.returncode, 1, result.stdout)
                self.assertIn("FAIL:", result.stderr)
                self.assertNotIn("Traceback", result.stderr)

    def test_cli_rejects_invalid_json_and_duplicate_keys(self):
        for contents in ("{", '{"schemaVersion":1,"schemaVersion":1}', '{"bad":NaN}'):
            with self.subTest(contents=contents):
                result = self.run_cli(contents)
                self.assertEqual(result.returncode, 1)
                self.assertNotIn("Traceback", result.stderr)

    def test_cli_rejects_timing_integer_beyond_numeric_encoding_range(self):
        self.data["timings"][0]["value"] = 10 ** 400
        result = self.run_cli(json.dumps(self.data))
        self.assertEqual(result.returncode, 1)
        self.assertIn("binary64 range", result.stderr)
        self.assertNotIn("Traceback", result.stderr)

    def test_duplicate_field_id_is_rejected(self):
        fields = self.register("BMCR")["fields"]
        fields[1]["id"] = fields[0]["id"]
        self.rejected("duplicate semantic ID")

    def test_c22_range_rejects_mask_as_address(self):
        for value in ("0x20", "0x8000", "0x10000"):
            with self.subTest(value=value):
                self.register("PHYRCR")["address"] = value
                self.rejected("value exceeds 0x1F")

    def test_alias_must_be_explicit(self):
        self.register("BMCR")["aliases"] = []
        self.rejected("alias must explicitly match")

    def test_selector_indirect_alias_is_rejected(self):
        self.register("REGCR")["aliases"] = [{"space": "MMD", "devad": "0x1F", "address": "0x0D"}]
        self.rejected("direct-only REGCR/ADDAR")

    def test_duplicate_address_alias_is_rejected(self):
        self.register("VOD_CFG3")["aliases"] = [{"space": "C22", "devad": None, "address": "0x00"}]
        self.rejected("address alias collision")

    def test_unproved_mmd_aliases_are_rejected(self):
        aliases = [
            {"space": "MMD", "devad": "0x03", "address": "0x1001"},
            {"space": "C22", "devad": None, "address": "0x0C"},
            {"space": "MMD", "devad": "0x1F", "address": "0x0D"},
            {"space": "MMD", "devad": "0x1F", "address": "0x9999"},
        ]
        for alias in aliases:
            with self.subTest(alias=alias):
                self.register("VOD_CFG3")["aliases"] = [alias]
                result = self.run_cli(json.dumps(self.data))
                self.assertEqual(result.returncode, 1)
                self.assertIn("MMD aliases require", result.stderr)

    def test_each_core_procedure_is_required(self):
        # Fixed prompt-3 outcomes, independent of the validator's required set.
        required = ("identify", "direct_access", "mmd_access", "pcs_reset",
                    "hard_reset", "restart", "external_reset", "initialize_10base",
                    "configure_an", "configure_forced", "next_page_exchange", "observe_link")
        for pid in required:
            with self.subTest(procedure=pid):
                self.data = copy.deepcopy(self.original)
                self.data["procedures"] = [p for p in self.data["procedures"] if p["id"] != pid]
                for feature in self.data["features"]:
                    feature["procedures"] = [p for p in feature["procedures"] if p != pid]
                self.rejected("missing core procedure")

    def test_procedure_owned_register_requires_feature_coverage(self):
        procedure = next(p for p in self.data["procedures"] if p["id"] == "pcs_reset")
        procedure["ownedMasks"] = [{"register": "ANAR", "mask": "0x0400"}]
        self.rejected("owned register is missing from feature coverage")

    def test_field_bounds_overlap_and_partition(self):
        for mask, message in (("0x10000", "value exceeds"), ("0x0000", "must not be zero"),
                              ("0xC000", "overlaps")):
            with self.subTest(mask=mask):
                self.data = copy.deepcopy(self.original)
                self.field("BMCR", 0x8000)["mask"] = mask
                self.rejected(message)
        self.data = copy.deepcopy(self.original)
        self.register("BMCR")["fields"].pop()
        self.rejected("partition does not cover")

    def test_noncontiguous_mask_is_rejected(self):
        target = self.register("PHYIDR1")
        target["fields"][0]["mask"] = "0xA000"
        self.rejected("must be contiguous")

    def test_unknown_reset_bits_cannot_be_set_in_partial_value(self):
        self.register("BMCR")["resetValue"] = "0x2000"
        self.rejected("unknown reset bit")

    def test_pending_record_has_no_invented_fields_or_defaults(self):
        pending = next(r for r in self.data["registers"] if r["status"] == "PENDING")
        pending["resetValue"] = "0x0000"
        self.rejected("pending/recipe records must retain unknown")

    def test_completed_all_reserved_record_is_valid_but_not_writable(self):
        # All-reserved detailed tables grant no write permission.
        reserved = self.register("CDSCR2")
        reserved.update(status="COMPLETE", fields=[], resetValue="0x0000",
                        resetKnownMask="0xFFFF", reservedMask="0xFFFF")
        ledger.validate(self.data)
        procedure = next(p for p in self.data["procedures"] if p["ownedMasks"])
        feature = next(f for f in self.data["features"] if f["id"] == procedure["feature"])
        feature["registers"].append(reserved["id"])
        procedure["ownedMasks"] = [{"register": reserved["id"], "mask": "0x0001"}]
        self.rejected("owned mask includes reserved")

    def test_source_reference_must_resolve(self):
        self.field("BMCR", 0x8000)["sources"][0]["source"] = "missing"
        self.rejected("unknown source reference")

    def test_source_path_cannot_escape_repository(self):
        self.data["sources"][0]["path"] = "../AGENTS.md"
        self.rejected("inside the repository")

    def test_missing_procedure_owner_is_rejected(self):
        self.data["procedures"][0]["owner"].pop("test")
        self.rejected("missing keys: test")

    def test_procedure_owned_reserved_mask_is_rejected(self):
        procedure = next(p for p in self.data["procedures"] if p["id"] == "pcs_reset")
        procedure["ownedMasks"] = [{"register": self.register("BMCR")["id"], "mask": "0x0040"}]
        self.rejected("owned mask includes reserved")

    def test_procedure_cannot_claim_read_only_identity(self):
        procedure = next(p for p in self.data["procedures"] if p["id"] == "identify")
        procedure["ownedMasks"] = [{"register": self.register("PHYIDR1")["id"], "mask": "0xFFFF"}]
        self.rejected("owned mask includes read-only")

    def test_identity_policy_and_timing_qualifier_are_required(self):
        self.data["identity"].pop("policy")
        self.rejected("missing keys: policy")
        self.data = copy.deepcopy(self.original)
        self.data["timings"][0]["qualifier"] = "about this long"
        self.rejected("explicit MIN, NOM or MAX")
        self.data["timings"][0]["qualifier"] = "MAX"
        self.data["timings"][0]["value"] = True
        self.rejected("finite and nonnegative")

    def test_pause_matrix_shape_and_result_mask(self):
        self.data["pauseResolution"]["phycrRxTx"][0][0] = "0x4000"
        self.rejected("non-pause bits")
        self.data = copy.deepcopy(self.original)
        self.data["pauseResolution"]["phycrRxTx"].pop()
        self.rejected("four matrix rows")

    def test_command_neutral_and_encodings_are_checked(self):
        field = self.field("BMCR", 0x8000)
        field["neutral"] = "0x1"
        self.rejected("command-neutral")
        field["neutral"] = "0x0"
        field["legal"] = ["0x0", "0x1"]
        field["prohibited"] = ["0x1"]
        self.rejected("both legal and prohibited")

    def test_w0c_and_w1c_neutral_semantics(self):
        # Synthetic structural cases, not claims that BMCR has these semantics.
        field = self.field("BMCR", 0x8000)
        field.update(access="W0C", neutral="0x0")
        self.rejected("command-neutral")
        field["neutral"] = "0x1"
        ledger.validate(self.data)
        field.update(access="W1C", neutral="0x1")
        self.rejected("command-neutral")
        field["neutral"] = "0x0"
        ledger.validate(self.data)

    def test_wrong_scope_and_evidence_are_rejected(self):
        self.data["features"][0]["evidence"]["sdkBuilt"] = "PASS"
        self.rejected("unknown evidence state")

    def test_inventory_removal_is_rejected(self):
        self.data["registers"] = [r for r in self.data["registers"] if r["name"] != "CDLAR5"]
        self.rejected("missing or renamed MMD inventory")

    def test_undocumented_c22_hole_is_not_a_register(self):
        record = copy.deepcopy(next(r for r in self.data["registers"] if r["status"] == "PENDING"))
        record.update(id="bad_c22_hole", name="BAD_HOLE", space="C22", devad=None,
                      address="0x0C", aliases=[{"space": "MMD", "devad": "0x1F", "address": "0x0C"}])
        self.data["registers"].append(record)
        self.rejected("undocumented primary address")

    def test_feature_family_removal_is_rejected(self):
        self.data["features"] = [f for f in self.data["features"] if f["id"] != "outside_phy"]
        self.rejected("missing feature family")

    def test_unresolved_printed_address_is_not_a_wire_offset(self):
        record = next(r for r in self.data["registers"] if r["status"] == "PENDING")
        record = copy.deepcopy(record)
        record.update(id="bad_eee_wire", name="BAD_EEE_WIRE", space="MMD",
                      devad="0x03", address="0x1001", aliases=[])
        self.data["registers"].append(record)
        next(f for f in self.data["features"] if f["id"] == "eee")["registers"].append("bad_eee_wire")
        self.rejected("incorrectly claimed as a wire offset")

    def test_independent_identity_field_vectors(self):
        # SNLS647I tables 8-21/22, pp56-57. OUI is split across two words.
        identity = self.data["identity"]
        expected = {"oui": 0x080028, "id1": 0x2000, "ouiLowMask": 0xFC00,
                    "modelMask": 0x03F0, "revisionMask": 0x000F, "documentedRevision": 0x01}
        self.assertEqual({key: int(identity[key], 16) for key in expected}, expected)
        self.assertEqual({mode: int(value, 16) for mode, value in identity["models"].items()},
                         {"BASIC": 0x11, "ENHANCED": 0x13})
        self.assertEqual({mode: int(value, 16) for mode, value in identity["combinedIds"].items()},
                         {"BASIC": 0x2000A111, "ENHANCED": 0x2000A131})
        self.assertEqual(int(self.register("PHYIDR1")["address"], 16), 2)
        self.assertEqual(int(self.register("PHYIDR1")["resetValue"], 16), 0x2000)
        self.assertEqual(int(self.register("PHYIDR2")["address"], 16), 3)
        masks = {int(f["mask"], 16) for f in self.register("PHYIDR2")["fields"]}
        self.assertEqual(masks, {0xFC00, 0x03F0, 0x000F})
        for id2, model in ((0xA111, 0x11), (0xA131, 0x13)):
            with self.subTest(id2=id2):
                self.assertEqual((0x2000 << 6) | ((id2 & 0xFC00) >> 10), 0x080028)
                self.assertEqual((id2 & 0x03F0) >> 4, model)
                self.assertEqual(id2 & 0x000F, 1)

    def test_independent_reset_polling_arithmetic_vector(self):
        # Table8-48 p73: register number and command mask are different namespaces.
        # This tests the research oracle; no driver polling implementation exists.
        register = self.register("PHYRCR")
        self.assertEqual(int(register["address"], 16), 0x001F)
        hard = int(self.field("PHYRCR", 0x8000)["mask"], 16)
        restart = int(self.field("PHYRCR", 0x4000)["mask"], 16)
        self.assertEqual((hard, restart), (0x8000, 0x4000))
        samples = (0xC000, 0x8000, 0x4000, 0x0000)
        self.assertEqual([sample & hard == 0 for sample in samples], [False, False, True, True])
        self.assertEqual([sample & restart == 0 for sample in samples], [False, True, False, True])
        for mask in (0x8000, 0x4000):
            field = self.field("PHYRCR", mask)
            self.assertEqual(field["access"], "W1S")
            self.assertEqual(int(field["neutral"], 16), 0)

    def test_independent_reset_known_masks(self):
        # Tables8-19,20,23,25-29,31/32,43,48,81. Unknown strap bits stay unknown.
        expected = {"BMCR": (0x0000, 0xCAFF), "BMSR": (0x7849, 0xFFFF),
                    "ANAR": (0x0001, 0xFE1F), "ALNPAR": (0x0000, 0xFFFF),
                    "ANER": (0x0004, 0xFFFF), "ANNPTR": (0x2001, 0xFFFF),
                    "ANLNPTR": (0x0000, 0xFFFF), "CR1": (0x0000, 0xFFFF),
                    "CR2": (0x0100, 0xFFFD), "REGCR": (0x0000, 0xFFFF),
                    "ADDAR": (0x0000, 0xFFFF), "PHYCR": (0x0000, 0x3FE0),
                    "PHYRCR": (0x0000, 0xFFFF), "VOD_CFG3": (0x8400, 0xFFFF),
                    "PHYIDR1": (0x2000, 0xFFFF), "PHYSTS": (0x0002, 0xFFFF),
                    # Conservative interpretations: mode/strap bits are not fixed
                    # by table headers. CR3's conflicting low nibble stays unknown.
                    "PHYIDR2": (0xA111, 0xFFDF), "CR3": (0x0000, 0xFFF0),
                    "SOR1": (0x0000, 0xF800), "SOR2": (0x0000, 0xA040)}
        for name, pair in expected.items():
            with self.subTest(register=name):
                register = self.register(name)
                self.assertEqual((int(register["resetValue"], 16),
                                  int(register["resetKnownMask"], 16)), pair)

    def test_independent_all_core_field_partitions(self):
        # Original PDF tables8-19..32,34,43,48,81,87/88. Masks were derived by
        # the independent reviewer before inspecting the ledger; source names
        # are mapped to these documented semantic names. Do not generate this
        # table from the ledger. Reserved fields are aggregated, never writable.
        expected = {
            "BMCR": (0x007F, {"reset": 0x8000, "loopback": 0x4000,
                "speed": 0x2000, "an_enable": 0x1000, "power_down": 0x0800,
                "isolate": 0x0400, "an_restart": 0x0200, "duplex": 0x0100,
                "collision_test": 0x0080}),
            "BMSR": (0x0780, {"t4": 0x8000, "full100": 0x4000,
                "half100": 0x2000, "full10": 0x1000, "half10": 0x0800,
                "preamble": 0x0040, "an_complete": 0x0020, "remote_fault": 0x0010,
                "an_ability": 0x0008, "link": 0x0004, "jabber": 0x0002, "extended": 0x0001}),
            "PHYIDR1": (0x0000, {"oui_high": 0xFFFF}),
            "PHYIDR2": (0x0000, {"oui_low": 0xFC00, "model": 0x03F0, "revision": 0x000F}),
            "ANAR": (0x5000, {"next_page": 0x8000, "remote_fault": 0x2000,
                "asym_pause": 0x0800, "pause": 0x0400, "t4": 0x0200,
                "full100": 0x0100, "half100": 0x0080, "full10": 0x0040,
                "half10": 0x0020, "selector": 0x001F}),
            "ALNPAR": (0x1000, {"next_page": 0x8000, "ack": 0x4000,
                "remote_fault": 0x2000, "asym_pause": 0x0800, "pause": 0x0400,
                "t4": 0x0200, "full100": 0x0100, "half100": 0x0080,
                "full10": 0x0040, "half10": 0x0020, "selector": 0x001F}),
            "ANER": (0xFFE0, {"parallel_fault": 0x0010, "partner_next": 0x0008,
                "local_next": 0x0004, "page": 0x0002, "partner_an": 0x0001}),
            "ANNPTR": (0x4000, {"next_page": 0x8000, "message": 0x2000,
                "ack2": 0x1000, "toggle": 0x0800, "code": 0x07FF}),
            "ANLNPTR": (0x0000, {"next_page": 0x8000, "ack": 0x4000,
                "message": 0x2000, "ack2": 0x1000, "toggle": 0x0800, "code": 0x07FF}),
            "CR1": (0xFE5D, {"tdr_auto": 0x0100, "link_recovery": 0x0080,
                "robust_mdix": 0x0020, "fast_rxdv": 0x0002}),
            "CR2": (0xFFD9, {"extended_duplex": 0x0020,
                "idle_rx_error": 0x0004, "odd_nibble_disable": 0x0002}),
            "CR3": (0xFB90, {"descrambler_drop": 0x0400, "polarity_swap": 0x0040,
                "pair_swap": 0x0020, "fast_drop_modes": 0x000F}),
            "REGCR": (0x3FE0, {"function": 0xC000, "devad": 0x001F}),
            "ADDAR": (0x0000, {"address_data": 0xFFFF}),
            "PHYSTS": (0x8000, {"mdix": 0x4000, "rx_error": 0x2000,
                "polarity": 0x1000, "false_carrier": 0x0800, "signal": 0x0400,
                "descrambler": 0x0200, "page": 0x0100, "interrupt": 0x0080,
                "remote_fault": 0x0040, "jabber": 0x0020, "an_complete": 0x0010,
                "loopback": 0x0008, "duplex": 0x0004, "speed": 0x0002, "link": 0x0001}),
            "PHYCR": (0x0740, {"auto_mdix": 0x8000, "force_mdix": 0x4000,
                "pause_rx": 0x2000, "pause_tx": 0x1000, "mii_link": 0x0800,
                "led_stretch": 0x0080, "led_config": 0x0020, "phy_address": 0x001F}),
            "PHYRCR": (0x3FFF, {"hard_reset": 0x8000, "restart": 0x4000}),
            "VOD_CFG3": (0x003F, {"mdi_termination": 0xF000, "mdi_enable": 0x0800,
                "mdix_termination": 0x0780, "mdix_enable": 0x0040}),
            "SOR1": (0xF800, {"strap0": 0x0001, "strap1": 0x0002,
                "strap2": 0x0004, "strap3": 0x0008, "strap4": 0x0010,
                "strap5": 0x0020, "strap6": 0x0040, "strap7": 0x0080,
                "strap8": 0x0100, "strap9": 0x0200, "strap10": 0x0400}),
            "SOR2": (0xA040, {"isolate": 0x4000, "crs_rxdv": 0x1000,
                "led3_polarity": 0x0800, "led2_polarity": 0x0400,
                "led0_polarity": 0x0200, "fast_drop": 0x0100, "auto_mdix": 0x0080,
                "led1_polarity": 0x0020, "rmii": 0x0010, "follower": 0x0008,
                "an1": 0x0004, "an0": 0x0002, "an_enable": 0x0001}),
        }
        self.assertEqual(set(expected), ledger.CORE)
        for name, (reserved, fields) in expected.items():
            with self.subTest(register=name):
                actual = self.register(name)
                self.assertEqual(int(actual["reservedMask"], 16), reserved)
                self.assertEqual({f["name"]: int(f["mask"], 16) for f in actual["fields"]}, fields)

    def test_independent_prohibited_advertisement_and_crossover(self):
        # Table8-23: named R/W RF is nevertheless application-prohibited.
        fault = self.field("ANAR", 0x2000)
        self.assertEqual(fault["access"], "RW")
        self.assertIn(1, [int(v, 16) for v in fault["prohibited"]])
        self.assertEqual(self.field("ALNPAR", 0x2000)["access"], "R")
        self.assertEqual(self.field("CR1", 0x0020)["access"], "RW")
        self.assertIn(self.field("PHYCR", 0x8000)["access"], {"RW", "RHW"})
        self.assertIn(self.field("PHYCR", 0x4000)["access"], {"RW", "RHW"})
        self.assertEqual(self.field("PHYCR", 0x2000)["access"], "R")
        self.assertEqual(self.field("PHYCR", 0x1000)["access"], "R")

    def test_independent_mmd_and_vod_vectors(self):
        # Sections8.3.11.1-.5 and8.3.13. Literal full transaction writes4A40.
        self.assertEqual(int(self.register("REGCR")["address"], 16), 0x0D)
        self.assertEqual(int(self.register("ADDAR")["address"], 16), 0x0E)
        register = self.register("VOD_CFG3")
        self.assertEqual((register["space"], int(register["devad"], 16),
                          int(register["address"], 16)), ("MMD", 31, 0x030E))
        self.assertEqual(self.field("REGCR", 0xC000)["access"], "RW")
        self.assertEqual(self.field("VOD_CFG3", 0xF000)["access"], "RW")
        self.assertEqual(int(register["reservedMask"], 16), 0x003F)

    def test_independent_all_core_access_masks(self):
        # Fresh original-PDF oracle: SNLS647I tables 8-19..32,34,43,48,81,87/88.
        # Access is distinct from legal writes (ANAR RF) and read-clear ownership.
        expected = {
            "BMCR": {"W1S": 0x8200, "RW": 0x4880, "RHW": 0x3500},
            "BMSR": {"R": 0xF849, "RH": 0x0022, "RC": 0x0014},
            "PHYIDR1": {"R": 0xFFFF}, "PHYIDR2": {"R": 0xFFFF},
            "ANAR": {"RW": 0xAC1F, "RHW": 0x01E0, "R": 0x0200},
            "ALNPAR": {"R": 0xEFFF}, "ANER": {"RH": 0x0012, "R": 0x000D},
            "ANNPTR": {"RW": 0xB7FF, "R": 0x0800}, "ANLNPTR": {"R": 0xFFFF},
            "CR1": {"RW": 0x01A2}, "CR2": {"RW": 0x0024, "RHW": 0x0002},
            "CR3": {"RW": 0x0460, "RHW": 0x000F}, "REGCR": {"RW": 0xC01F},
            "ADDAR": {"RW": 0xFFFF},
            "PHYSTS": {"R": 0x4019, "RC": 0x3FE0, "RH": 0x0006},
            "PHYCR": {"RHW": 0xC000, "R": 0x3800, "RW": 0x00A0, "RH": 0x001F},
            "PHYRCR": {"W1S": 0xC000}, "VOD_CFG3": {"RW": 0xFFC0},
            "SOR1": {"R": 0x07FF}, "SOR2": {"R": 0x4007, "RH": 0x1FB8},
        }
        self.assertEqual(set(expected), ledger.CORE)
        for name, masks in expected.items():
            with self.subTest(register=name):
                actual = {}
                for field in self.register(name)["fields"]:
                    actual[field["access"]] = actual.get(field["access"], 0) | int(field["mask"], 16)
                self.assertEqual(actual, masks)

    def test_nominal_signaling_times_do_not_claim_power_up(self):
        # Section 7.6 p15: these two NOM rows specify reset, not power-up.
        by_id = {timing["id"]: timing for timing in self.data["timings"]}
        for tid, value in (("forced_signal_nominal", 0.5), ("rmii_clock_nominal", 0.2)):
            with self.subTest(timing=tid):
                timing = by_id[tid]
                self.assertEqual((timing["value"], timing["unit"], timing["qualifier"]),
                                 (value, "ms", "NOM"))
                self.assertEqual(timing["start"], "reset (table does not specify edge)")

    def test_independent_pause_matrix(self):
        # IEEE802.3-2012 Table28B-3, TI Table8-43: RX2000/TX1000.
        # Rows are local advertisements; columns are partner advertisements.
        pause = self.data["pauseResolution"]
        self.assertEqual([int(v, 16) for v in pause["fields"]], [0x0000, 0x0400, 0x0800, 0x0C00])
        self.assertEqual([[int(v, 16) for v in row] for row in pause["phycrRxTx"]],
                         [[0x0000, 0x0000, 0x0000, 0x0000],
                          [0x0000, 0x3000, 0x0000, 0x3000],
                          [0x0000, 0x0000, 0x0000, 0x1000],
                          [0x0000, 0x3000, 0x2000, 0x3000]])


if __name__ == "__main__":
    unittest.main()
