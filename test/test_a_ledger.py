"""AI/AE research oracles from SNLS783A. These tests perform no PHY access."""

import copy
import unittest
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from verify_a_ledger import validate
from verify_ledger import DEFAULT_LEDGER, load_ledger


class AGenerationLedgerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original = load_ledger(DEFAULT_LEDGER)
        cls.comparison = load_ledger(DEFAULT_LEDGER.with_name("dp83826a.json"))
        cls.procedures = load_ledger(DEFAULT_LEDGER.with_name("dp83826a-procedures.json"))
        cls.regs = {reg["id"]: reg for reg in cls.comparison["registers"]}
        cls.fields = {f["id"]: f for r in cls.regs.values() for f in r["fields"]}

    def test_every_table_and_original_semantic_id(self):
        validate(self.original, self.comparison, self.procedures)
        self.assertEqual(len(self.regs), 79)
        self.assertEqual(len(self.fields), 285)

    def test_both_declared_grades_share_only_the_exact_revision_and_models(self):
        policy = self.comparison["identity"]
        self.assertEqual(self.comparison["variants"], ["DP83826AI", "DP83826AE"])
        self.assertEqual(policy["acceptedRevisions"], [4])
        self.assertEqual(policy["originalAcceptedRevisions"], [1])
        self.assertEqual(policy["owner"], 58)
        self.assertEqual(policy["id1"], "0x2000")
        self.assertEqual(self.fields["PHYIDR2.model"]["modeDefault"], policy["models"])
        oui_low = int(self.fields["PHYIDR2.oui_low"]["reset"].rstrip("h"), 16)
        for mode, expected in (("BASIC", 0x2000A114), ("ENHANCED", 0x2000A134)):
            model = int(policy["models"][mode], 16)
            self.assertEqual(int(policy["id1"], 16) << 16 | oui_low << 10 | model << 4 |
                             policy["acceptedRevisions"][0], expected)

    def test_reset_headers_and_usable_known_bits(self):
        # Each triplet is the printed header, board-independent mask and value.
        for name, header, known, value in (
                ("PHYIDR2", "A134", 0xFFDF, 0xA114), ("PHYSTS", "0002", 0xFFF9, 0),
                ("CR1", "00X0", 0xFFDF, 0), ("CR3", "0X00", 0xFBF0, 0),
                ("PHYSCR", "010X", 0xFFFC, 0x0108), ("FLD_CFG2", "0288", 0xFFFF, 0x0288),
                ("LEDCFG", "5565", 0xFFFF, 0x5565), ("LED1_GPIO_CFG", "000X", 0xFFF8, 8),
                ("LED2_GPIO_CFG", "0008", 0xFFF8, 8), ("RXFCFG1", "10X1", 0xFF7E, 0x1000),
                ("VOD_CFG1", "3C00", 0xC000, 0), ("VOD_CFG2", "0410", 0, 0),
                ("VOD_CFG3", "8400", 0xFFFF, 0x8400), ("CLK_OUT_LED_STATUS", "0002", 0xFFFE, 2),
                ("SOR1", "0000", 0xF000, 0), ("SOR2", "0287", 0xA040, 0)):
            reg = self.regs[name]
            with self.subTest(register=name):
                self.assertEqual((reg["headerReset"], int(reg["resetKnownMask"], 16),
                                  int(reg["resetValue"], 16)), (header, known, value))
        for field, basic, enhanced in (
                ("CR1.robust_mdix", 0, "strap"), ("CR3.descrambler_drop", 0, "strap"),
                ("PHYSCR.interrupt_enable", 1, 0), ("PHYSCR.interrupt_output_enable", 1, 0),
                ("RXFCFG1_enable", "strap", 1), ("RXFCFG1_magicPacketEnable", "strap", "strap"),
                ("BMCR.duplex", "strap", 0), ("ANAR.full100", "strap", 1)):
            with self.subTest(field=field):
                self.assertEqual(self.fields[field]["modeDefault"], {"BASIC": basic, "ENHANCED": enhanced})

    def test_numeric_reset_cells_with_board_or_mode_dependencies(self):
        # Use the printed data under each enlarged mask. A value mismatch alone
        # would miss the case where the particular board agrees with the table.
        for name, mask, value in (
                ("VOD_CFG1", 0xF000, 0x3000), ("VOD_CFG1", 0xCFC0, 0x0C00),
                ("VOD_CFG1", 0xC03F, 0), ("VOD_CFG2", 0xF000, 0),
                ("VOD_CFG2", 0x0FC0, 0x0400), ("VOD_CFG2", 0x003F, 0x0010),
                ("VOD_CFG2", 0xFFFF, 0x0410), ("LED2_GPIO_CFG", 0xFFFF, 8),
                ("RXFCFG1", 0xFF7F, 0x1001), ("CR3", 0xFBFF, 0),
                ("SOR1", 0xFFFF, 0), ("SOR2", 0xFFFF, 0x0287),
                ("CLK_OUT_LED_STATUS", 0xFFFF, 2), ("PHYSTS", 0xFFFF, 2),
                ("PHYIDR2", 0xFFFF, 0xA134), ("PHYIDR2", 0xFC0F, 0xA004)):
            with self.subTest(register=name, mask=hex(mask)):
                comparison = copy.deepcopy(self.comparison)
                reg = next(r for r in comparison["registers"] if r["id"] == name)
                reg.update(resetKnownMask=f"0x{mask:04X}", resetValue=f"0x{value:04X}")
                with self.assertRaisesRegex(ValueError, "reset knowledge"):
                    validate(self.original, comparison, self.procedures)

    def test_otp_annotations_match_each_field_in_tables_8_80_and_8_81(self):
        otp_fields = {f["id"] for f in self.fields.values() if f.get("defaultFromOtp")}
        self.assertEqual(otp_fields, {
            "VOD_CFG1.minus_mdix_high", "VOD_CFG1.minus_mdi", "VOD_CFG1.zero",
            "VOD_CFG2.minus_mdix_low", "VOD_CFG2.plus_mdix", "VOD_CFG2.plus_mdi"})
        comparison = copy.deepcopy(self.comparison)
        reg = next(r for r in comparison["registers"] if r["id"] == "VOD_CFG2")
        del reg["fields"][0]["defaultFromOtp"]
        with self.assertRaisesRegex(ValueError, "OTP"):
            validate(self.original, comparison, self.procedures)

    def test_recipe_word_has_no_documented_reset(self):
        comparison = copy.deepcopy(self.comparison)
        comparison["recipeRegisters"][0].update(resetKnownMask="0xFFFF", resetValue="0x008B")
        with self.assertRaisesRegex(ValueError, "recipe register reset"):
            validate(self.original, comparison, self.procedures)

    def test_explicit_field_moves_access_and_assignment(self):
        for field, mask, access, owner in (
                ("FLDS.causes", 0x01F0, "RCH", 58), ("SOR1.strap11", 0x0800, "RH", 58),
                ("PHYSTS.mdix", 0x4000, "RH", 58), ("PHYSTS.remote_fault", 0x0040, "RCH", 58),
                ("FLD_CFG2.descrambler_window", 0x003F, "R/W", 58),
                ("LEDCFG.led3_source", 0x0F00, "R/W", 59), ("LEDCFG.led2_source", 0x00F0, "R/W", 59),
                ("LEDCFG.led1_source", 0x000F, "R/W", 59), ("LEDCFG2.led3_polarity", 0x0400, "RH/W", 59),
                ("LEDCFG2.led3_drive_value", 0x0200, "R/W", 59), ("LEDCFG2.led3_drive_enable", 0x0100, "R/W", 59),
                ("LED0_GPIO_CFG.mux", 7, "R/W", 59), ("LED1_GPIO_CFG.mux", 7, "RH/W", 59),
                ("LED2_GPIO_CFG.mux", 7, "RH/W", 59), ("LED3_GPIO_CFG.mux", 7, "R/W", 59),
                ("RXFCFG1_levelAcknowledge", 0x0800, "W0C", 59),
                ("BISCR_checkerLock", 0x0800, "RH", 59), ("CDCR_done", 2, "RH", 59)):
            with self.subTest(field=field):
                row = self.fields[field]
                self.assertEqual((int(row["mask"], 16), row["access"], row["owner"]), (mask, access, owner))
        for name, reserved in (("IO_CFG1", 0x3EFF), ("PHYCR", 0x0760), ("LEDCFG", 0xF000)):
            self.assertEqual(int(self.regs[name]["reservedMask"], 16), reserved)

    def test_field_joins_reject_renames_and_owner_swaps(self):
        for name, key, value in (
                ("SOR1.strap11", "id", "SOR1.strap12"),
                ("LEDCFG2.led3_drive_enable", "id", "RXFS.magic_packet"),
                ("BMCR.reset", "owner", 59), ("FLDS.causes", "owner", 59),
                ("PHYSCR.interrupt_enable", "owner", 59), ("IO_CFG1.rx_dv", "owner", 58),
                ("LEDCFG2.led3_polarity", "owner", 58), ("GENCFG.min_ipg", "owner", 59),
                ("LED0_GPIO_CFG.clock_source", "owner", 59),
                ("CLK_OUT_LED_STATUS.clock_off_status", "owner", 59)):
            with self.subTest(name=name, key=key):
                comparison = copy.deepcopy(self.comparison)
                field = next(f for r in comparison["registers"] for f in r["fields"] if f["id"] == name)
                field[key] = value
                with self.assertRaises(ValueError):
                    validate(self.original, comparison, self.procedures)
        self.assertEqual(self.fields["GENCFG.min_ipg"]["owner"], 58)

    def test_removed_fields_keep_original_register_mask_and_a_table(self):
        for changes in ({"mask": "0x0000"}, {"mask": "0x0001"}, {"register": "CDLRR2"},
                        {"table": "8-44"}, None):
            with self.subTest(changes=changes):
                comparison = copy.deepcopy(self.comparison)
                removed = comparison["removedFields"]
                row = next(f for f in removed if f["originalField"] == "IO_CFG1.clock_off")
                if changes is None:
                    removed.append(row.copy())
                else:
                    row.update(changes)
                with self.assertRaises(ValueError):
                    validate(self.original, comparison, self.procedures)

    def test_all_eee_wire_writes_in_printed_order(self):
        # Independently transcribed from the original PDF across pp. 27–28.
        # Device -1 denotes a Clause 22 register. D2 selects the low MMD7 offset.
        expected = (
            (-1, 0x001F, 0x8000), (7, 0x003C, 0x0002), (31, 0x04D1, 0x008B),
            (31, 0x04DF, 0x0180), (31, 0x033E, 0xA681), (31, 0x033F, 0x0003),
            (31, 0x04F5, 0x2864), (31, 0x04E0, 0xFFF2), (31, 0x031F, 0xFE36),
            (31, 0x0308, 0x0000), (31, 0x04F4, 0x0800), (31, 0x0123, 0x0800),
            (31, 0x0126, 0x5D23), (31, 0x0132, 0x0183), (31, 0x04D4, 0x3322),
            (31, 0x04CF, 0x261D), (31, 0x0310, 0x76E1), (31, 0x0416, 0x1830),
            (31, 0x0311, 0x01FC), (31, 0x0313, 0x06E3), (31, 0x033A, 0x579B),
            (31, 0x033C, 0xEC00), (31, 0x0317, 0x1000), (31, 0x031B, 0x8848),
            (31, 0x04F5, 0x2864), (-1, 0x0000, 0x3300), (31, 0x0466, 0xFD00))
        actual = tuple((-1 if r["space"] == "C22" else int(r["devad"], 16),
                        int(r["address"], 16), int(r["value"], 16)) for r in self.procedures["eeeRecipe"])
        self.assertEqual(actual, expected)
        self.assertEqual(self.procedures["eeeRecipe"][1]["printed"], "7:203C")

    def test_eee_frame_floor_replaces_original_timeout(self):
        procedures = copy.deepcopy(self.procedures)
        proc = next(p for p in procedures["procedures"] if p["id"] == "eee_initialize")
        self.assertEqual(proc["frameFloor"], {"recipeWrites": 102, "withCapabilityAndAdvertisementReads": 110})
        proc["frameFloor"] = {"recipeWrites": 66, "withCapabilityAndAdvertisementReads": 74}
        with self.assertRaisesRegex(ValueError, "EEE frame floor"):
            validate(self.original, self.comparison, procedures)

    def test_procedure_comparison_preserves_owners_gates_and_effects(self):
        changes = (
            ("procedures", "configure_fast_drop", "owner", 59),
            ("features", "clock", "gates", []),
            ("features", "power", "gates", []),
            ("procedures", "configure_fast_drop", "ownedMasks",
             [{"register": "CR3", "mask": "0x046F"}]),
            ("procedures", "interrupt_configuration", "ownedMasks", []),
            ("writeActions", "LOOP_0130_digital", "value", "0xFFFF"),
            ("writeActions", "RECIPE_0428_dpd_enable", "mask", "0xFFFF"),
            ("writeActions", "TDR_0411_tdr_0813", "value", "0xFFFF"),
            ("timings", "pin_reset_smi", "value", 0),
            ("timings", "eee_wake_nominal", "qualifier", "MAX"),
        )
        for group, name, key, value in changes:
            with self.subTest(name=name, key=key):
                procedures = copy.deepcopy(self.procedures)
                row = next(r for r in procedures[group] if r["id"] == name)
                row[key] = value
                with self.assertRaises(ValueError):
                    validate(self.original, self.comparison, procedures)

    def test_separate_rmii_and_pin30_clock_transactions(self):
        procedures = {r["id"]: r for r in self.procedures["procedures"]}
        for name in ("clock_configure", "mii_configure", "rmii_configure", "repeater_configure",
                     "compatibility_configure", "isolation_configure"):
            self.assertEqual(procedures[name]["timeoutPlan"], "interface")
        for name in ("led_configure", "pin_configure", "wol_configure", "interrupt_configuration"):
            self.assertEqual(procedures[name]["timeoutPlan"], "pinRoutes")
        example = self.procedures["timeoutPlans"]["interface"]["rmiiExample"]
        # Two direct preflight reads in one batch. Five direct rows, three
        # distinct indirect rows (0456, 0302 and 0303), each verified.
        self.assertEqual((example["applyFrames"], example["applyCallbacks"]),
                         (2 + 5 * 3 + 3 * 9, 1 + 2 * (5 + 3)))
        self.assertEqual((example["verifyFrames"], example["verifyCallbacks"]),
                         (2 + 5 + 3 * 4, 1 + 5 + 3))
        self.assertEqual((example["pin31AdditionalFrames"], example["pin31AdditionalCallbacks"]), (4, 1))

    def test_troubleshooting_examples_against_detailed_fields(self):
        procedures = {r["id"]: r for r in self.procedures["procedures"]}
        # SNLS783A table 8-13 and figure 8-13, indexed by the two named strap bits.
        straps = procedures["rmii_configure"]["startupSelections"]
        self.assertEqual(straps["basicStrap10Strap4"], ["MII", "RMII_LEADER", "RESERVED", "RMII_FOLLOWER"])
        self.assertEqual(straps["enhancedStrap1Strap8"], ["MII", "MII", "MII", "RMII"])
        # SNLA491 pp19-20 and SNLS783A table 8-41. BISCR[14] changes
        # counter behavior. Checker enable is part of the mode in [13:12].
        single, analog = procedures["bist_run"]["guideCases"]
        self.assertEqual((int(single["guideWord"], 16), single["generator"], single["checker"]),
                         (0x5000, "single", False))
        self.assertEqual((int(analog["guideWord"], 16), analog["generator"], analog["checker"]),
                         (0x3108, "continuous", True))
        self.assertEqual(int(analog["withLockAndActivity"], 16), 0x3108 | 0x0800 | 0x0200)

    def test_link_acquisition_observes_mirrors_before_clearers(self):
        proc = next(r for r in self.procedures["procedures"] if r["id"] == "observe_link")
        first, last = [[int(word, 16) for word in batch] for batch in proc["readBatches"]]
        # Table 8-35: BMSR clears PHYSTS[6]; ANER clears PHYSTS[8].
        self.assertEqual(first, [0x10, 1, 1, 0, 4, 5, 6, 0x19])
        self.assertEqual(last, [0x10, 1])

    def test_source_implied_and_blocked_addresses_keep_their_routes(self):
        for group, name, key, value in (
                ("printedAddresses", "eee_printed_1016", "status", "VERIFIED"),
                ("debugAddresses", "0x0225", "status", "SAFE_READ"),
                ("recipeRegisters", "EEE_04D3", "status", "PROCEDURE_ONLY"),
                ("recipeRegisters", "TDR_0411", "devad", "0x07")):
            with self.subTest(name=name):
                comparison = copy.deepcopy(self.comparison)
                row = next(r for r in comparison[group] if r.get("id", r.get("address")) == name)
                row[key] = value
                with self.assertRaises(ValueError):
                    validate(self.original, comparison, self.procedures)

    def test_10base_te_fresh_address_and_no_increment_on_both_transfers(self):
        proc = next(r for r in self.procedures["procedures"] if r["id"] == "initialize_10base")
        wire = proc["wireVectors"]
        for name, last_frame in (("write", ("W", 14, 0x4A40)), ("read", ("R", 14))):
            frames = [tuple([action] + [int(word, 16) for word in words])
                      for action, *words in wire[name]]
            self.assertEqual(frames, [("W", 13, 31), ("W", 14, 0x030E), ("W", 13, 0x401F), last_frame])
        self.assertEqual((wire["readbackMask"], wire["readbackValue"]), ("0xFFC0", "0x4A40"))

    def test_strap_dependent_fast_drop_and_mirror_bits(self):
        straps = self.procedures["fldStraps"]
        # Tables 8-5/6. In option 5 Strap8 is low. Order is Strap7/1/8/11.
        self.assertEqual([(x[:4], int(x[4], 16)) for x in straps["enhanced"]], [
            (list("0XXX"), 0), (list("1100"), 0x40F), (list("1000"), 0x00D),
            (list("1010"), 0x00C), (list("1001"), 0x001), (list("1101"), 0x403)])
        self.assertEqual(straps["basic"], [["0", "0x000D"], ["1", "0x0001"]])
        mask = (int(self.fields["CR3.fast_drop_modes"]["mask"], 16)
                | int(self.fields["CR3.descrambler_drop"]["mask"], 16))
        self.assertEqual(mask, 0x040F)
        for observed, expected in ((0x042F, 0x0021), (0x044F, 0x0041)):
            self.assertEqual((observed & ~mask) | 1, expected)

    def test_local_fault_prohibition_and_separate_remote_observations(self):
        fields = {f["id"]: f for r in self.original["registers"] for f in r["fields"]}
        field = self.fields["ANAR.remote_fault"]
        self.assertEqual(field["mask"], "0x2000")
        base = fields[field["originalField"]]
        self.assertIn("0x0001", field.get("prohibited", base["prohibited"]))
        self.assertEqual(self.fields["ALNPAR.remote_fault"]["mask"], "0x2000")
        self.assertEqual(self.fields["BMSR.remote_fault"]["mask"], "0x0010")

    def test_validator_rejects_corrupt_research_records(self):
        for fault, message in (("missing field", "missing field"), ("reserved", "reserved"),
                               ("known", "reset knowledge"), ("missing owner", "owner"),
                               ("missing offset", "address missing"), ("merge", "27 ordered"),
                               ("reorder", "EEE row")):
            with self.subTest(fault=fault):
                registers = copy.deepcopy(self.comparison)
                procedures = copy.deepcopy(self.procedures)
                by_name = {r["id"]: r for r in registers["registers"]}
                recipe = procedures["eeeRecipe"]
                if fault == "missing field":
                    by_name["BMCR"]["fields"].pop()
                elif fault == "reserved":
                    by_name["LEDCFG"]["reservedMask"] = "0x0000"
                elif fault == "known":
                    by_name["CR1"]["resetKnownMask"] = "0xFFFF"
                elif fault == "missing owner":
                    procedures["procedures"][0]["owner"] = None
                elif fault == "missing offset":
                    registers["recipeRegisters"].remove(next(
                        r for r in registers["recipeRegisters"] if r["address"] == "0x0126"))
                elif fault == "merge":
                    recipe.pop(24)
                else:
                    recipe[-1], recipe[-2] = recipe[-2], recipe[-1]
                with self.assertRaisesRegex(ValueError, message):
                    validate(self.original, registers, procedures)
