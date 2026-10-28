"""Final source-coverage guards and original-PDF power/interface oracles."""

import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import verify_ledger as ledger


class RemainingLedgerTests(unittest.TestCase):
    def setUp(self):
        self.data = ledger.load_ledger(ledger.DEFAULT_LEDGER)

    def register(self, name):
        return next(r for r in self.data['registers'] if r['name'] == name)

    def field(self, name, mask):
        return next(f for f in self.register(name)['fields'] if int(f['mask'], 16) == mask)

    def rejected_file(self, message):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'malformed.json'
            path.write_text(json.dumps(self.data), encoding='utf-8')
            result = subprocess.run([sys.executable, str(ledger.ROOT / 'tools/verify_ledger.py'), str(path)],
                                    text=True, capture_output=True, check=False, timeout=20)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(message, result.stderr)

    def test_full_inventory_has_researched_dispositions(self):
        self.assertEqual(len(self.data['registers']), 96)
        self.assertEqual(sum(r['status'] == 'COMPLETE' for r in self.data['registers']), 79)
        self.assertEqual(sum(r['status'] == 'RECIPE' for r in self.data['registers']), 17)
        self.assertEqual(len(self.data['printedAddresses']), 5)
        self.assertTrue(all(f['evidence']['documented'] == 'COMPLETE' for f in self.data['features']))
        for row in self.data['registers']:
            if row['status'] == 'RECIPE':
                self.assertEqual(row['fields'], [])
                self.assertIsNone(row['resetValue'])
                self.assertIsNone(row['resetKnownMask'])
                self.assertIsNone(row['reservedMask'])

    def test_independent_remaining_field_and_reset_partitions(self):
        # Original SNLS647I tables8-42,49-51,73-80,82-86,89-90,92-97;
        # literals independently reviewed before integration, not generated from JSON.
        # name: reset value, known mask, reserved mask, ordinary writable mask
        expected = {
            'LEDCR': (0x0400, 0xFF7F, 0xF96D, 0x0692),
            'MLEDCR': (0x0041, 0xFFFF, 0xFD86, 0x0279),
            'COMPT': (0, 0xFFFF, 0xFFF0, 0x000F),
            '10M_CFG': (0x7998, 0xFFFF, 0xBFFF, 0x4000),
            'IO_CFG1': (0, 0xFEFF, 0x3EBF, 0xC140),
            'LED0_GPIO_CFG': (8, 0xFFFF, 0xFFC0, 0x0038),
            'LED1_GPIO_CFG': (8, 0xFFF8, 0xFFC0, 0x003F),
            'LED2_GPIO_CFG': (8, 0xFFFF, 0xFFF8, 0x0007),
            'LED3_GPIO_CFG': (8, 0xFFFF, 0xFFF8, 0),
            'CLK_OUT_LED_STATUS': (2, 0xFFFF, 0xFFFE, 0),
            'VOD_CFG1': (0x3C00, 0xFFFF, 0xC000, 0x3FFF),
            'VOD_CFG2': (0x0410, 0xFFFF, 0, 0xFFFF),
            'ANA_LD_PROG_SL': (0x0080, 0xFFFF, 0xFFFF, 0),
            'ANA_RX10BT_CTRL': (8, 0xFFFF, 0xFFE0, 0x001F),
            'GENCFG': (8, 0xFFFF, 0xFFF7, 8),
            'LEDCFG': (0x5665, 0xFFFF, 0x000F, 0xFFF0),
            'IOCTRL': (0x0010, 0xFFFF, 0xFFE0, 0x001F),
            'LEDCFG2': (0, 0xFBBB, 0xFF88, 0x0077),
            'RXFCFG1': (0x1080, 0xFFFE, 0xE01E, 0x17E1),
            'RXFPMD1': (0, 0xFFFF, 0, 0xFFFF),
            'RXFPMD2': (0, 0xFFFF, 0, 0xFFFF),
            'RXFPMD3': (0, 0xFFFF, 0, 0xFFFF),
            'RXFSOP1': (0, 0xFFFF, 0, 0xFFFF),
            'RXFSOP2': (0, 0xFFFF, 0, 0xFFFF),
            'RXFSOP3': (0, 0xFFFF, 0, 0xFFFF),
        }
        for name, values in expected.items():
            with self.subTest(register=name):
                row = self.register(name)
                stable = sum(int(f['mask'], 16) for f in row['fields']
                             if f['access'] in ('RW', 'RHW'))
                actual = tuple(int(row[k], 16) for k in
                               ('resetValue', 'resetKnownMask', 'reservedMask')) + (stable,)
                self.assertEqual(actual, values)

    def test_independent_led_placement_and_legal_pin_codes(self):
        self.assertEqual(int(self.register('MLEDCR')['address'], 16), 0x0025)
        self.assertIn('0x000F', self.field('MLEDCR', 0x0078)['prohibited'])
        self.assertEqual(self.field('MLEDCR', 1)['legal'], ['0x0001'])
        self.assertEqual((14 << 3) | 1, 0x0071)
        self.assertEqual(self.field('LEDCFG', 0x00F0)['name'], 'led1_source')
        self.assertEqual(self.field('LEDCFG', 0xF000)['name'], 'led3_source')
        self.assertEqual((0x5665 & ~0x00F0) | (10 << 4), 0x56A5)
        self.assertEqual((0x5665 & ~0xF000) | (14 << 12), 0xE665)
        for name in ('LED0_GPIO_CFG', 'LED1_GPIO_CFG'):
            self.assertEqual([int(v, 16) for v in self.field(name, 0x0038)['legal']], [5, 6])
        self.assertEqual([int(v, 16) for v in self.field('LED1_GPIO_CFG', 7)['legal']], [0, 2, 4, 5, 6])
        self.assertEqual([int(v, 16) for v in self.field('LED2_GPIO_CFG', 7)['legal']], [0, 2, 3, 4, 5, 6, 7])
        for name in ('LED0_GPIO_CFG', 'LED3_GPIO_CFG'):
            self.assertEqual(self.field(name, 7)['access'], 'R')

    def test_independent_odd_nibble_and_electrical_encodings(self):
        self.assertIn('disable', self.field('CR2', 2)['validity'].lower())
        self.assertEqual(0x0100 | 2, 0x0102)  # disable, not enable
        self.assertEqual(0x0102 & ~2, 0x0100)
        self.assertEqual([int(v, 16) for v in self.field('IOCTRL', 0x001F)['legal']], [0x10, 0x11])
        self.assertEqual([int(v, 16) for v in self.field('IO_CFG1', 0xC000)['legal']], [0, 1])
        self.assertEqual([int(v, 16) for v in self.field('VOD_CFG1', 0x0FC0)['legal']], list(range(0x28, 0x39)))
        for name, mask in (('VOD_CFG2', 0x0FC0), ('VOD_CFG2', 0x003F)):
            self.assertEqual([int(v, 16) for v in self.field(name, mask)['legal']], list(range(8, 0x19)))
        # Six-bit MDIX-minus spans two words; preserve the zero code here.
        for minus, plus, expected in ((0x28, 0x18, (0x2A00, 0x8618)),
                                      (0x38, 0x08, (0x3E00, 0x8208))):
            self.assertEqual(((minus >> 4) << 12 | minus << 6,
                              (minus & 15) << 12 | plus << 6 | plus), expected)
        self.assertIn('analog_rx_gain', self.register('ANA_RX10BT_CTRL')['gates'])
        compt = self.field('COMPT', 0x000F)
        self.assertEqual((compt['legal'], compt['prohibited']), ('range', []))
        # E/F are not 10M patterns but are valid low nibbles in the 100M count.
        self.assertIn('E/F not10M', compt['validity'])
        for high, low, count in ((0, 14, 14), (0, 15, 15), (1, 14, 30), (1, 15, 31)):
            self.assertEqual(high * 16 + low, count)

    def test_independent_wol_neutral_and_acknowledgement(self):
        field = self.field('RXFCFG1', 0x0800)
        self.assertEqual((field['access'], int(field['neutral'], 16)), ('W0C', 1))
        for stable, ordinary, ack in ((0x1181, 0x1981, 0x1181),
                                      (0x11A1, 0x19A1, 0x11A1),
                                      (0x0181, 0x0981, 0x0181)):
            self.assertEqual(stable | 0x0800, ordinary)
            self.assertEqual(ordinary & ~0x0800, ack)
        self.assertEqual(int(self.field('RXFCFG1', 0x0600)['mask'], 16), 0x0600)
        self.assertEqual([8 * (2 ** code) * 8 for code in range(4)], [64, 128, 256, 512])

    def test_independent_wol_octet_mapping(self):
        # Table8-92 explicitly calls TI byte5 MSB: API octet0 is first MAC octet.
        # Secure-ON tables8-95..97 instead number the first password octet byte0.
        mac = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66]
        password = [0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F]
        for name, high, low, value in (
                ('RXFPMD1', 'byte4', 'byte5', 0x2211),
                ('RXFPMD2', 'byte2', 'byte3', 0x4433),
                ('RXFPMD3', 'byte0', 'byte1', 0x6655),
                ('RXFSOP1', 'byte1', 'byte0', 0x2B2A),
                ('RXFSOP2', 'byte3', 'byte2', 0x2D2C),
                ('RXFSOP3', 'byte5', 'byte4', 0x2F2E)):
            self.assertEqual(self.field(name, 0xFF00)['name'], high)
            self.assertEqual(self.field(name, 0x00FF)['name'], low)
            if name.startswith('RXFPMD'):
                actual = mac[5 - int(high[-1])] << 8 | mac[5 - int(low[-1])]
            else:
                actual = password[int(high[-1])] << 8 | password[int(low[-1])]
            self.assertEqual(actual, value)

    def test_independent_power_masks_and_nominal_eee_timings(self):
        self.field('PHYSCR', 0x4000)
        self.assertEqual([int(v, 16) for v in self.field('PHYSCR', 0x3000)['legal']], [0, 2])
        self.field('BISCR', 0x0100)
        self.field('BMCR', 0x0800)
        self.field('PHYSCR', 0x8000)
        self.assertEqual(0x6108 & 0x7000, 0x6000)
        self.assertEqual(0x0100 & 0x0100, 0x0100)  # enabled policy can remain awake
        for tid, value, unit in (('eee_sleep_nominal', 210, 'us'),
                                 ('eee_wake_nominal', 36, 'us'),
                                 ('eee_quiet_nominal', 20, 'ms'),
                                 ('eee_refresh_nominal', 200, 'us')):
            timing = next(t for t in self.data['timings'] if t['id'] == tid)
            self.assertEqual((timing['value'], timing['unit'], timing['qualifier']), (value, unit, 'NOM'))

    def test_independent_eee_and_compliance_action_literals(self):
        # SNLS647I8.3.3.2 and table8-50; SNLA266Btable5-5.
        expected = {
            'EEECFG3_eee': (0x04D1, 0xFFFF, 0x008B),
            'EEE_04D3_eee': (0x04D3, 0xFFFF, 0x4F12),
            'EEE_04DF_eee': (0x04DF, 0xFFFF, 0x0180),
            'EEE_033E_eee': (0x033E, 0xFFFF, 0xA681),
            'EEE_033F_eee': (0x033F, 0xFFFF, 3),
            'RECIPE_0123_eee': (0x0123, 0xFFFF, 0x0800),
            'EEE_031B_eee': (0x031B, 0xFFFF, 0x8848),
            'EEE_0466_eee': (0x0466, 0xFFFF, 0xFE00),
            'EEE_04CF_eee': (0x04CF, 0xFFFF, 0x261D),
            'EEE_0416_eee': (0x0416, 0xFFFF, 0x1F30),
            'EEE_04F5_eee': (0x04F5, 0xFFFF, 0x2864),
            'EEE_04E0_eee': (0x04E0, 0xFFFF, 0xFFF2),
            'EEE_031F_eee': (0x031F, 0xFFFF, 0xFE36),
            'CLK_OUT_LED_STATUS_eee': (0x0308, 0xFFFF, 0),
            'EEE_04F4_eee': (0x04F4, 0xFFFF, 0x0800),
            'EEECFG3_enable_bypass_controls': (0x04D1, 9, 9),
            'EEECFG3_lpi_start': (0x04D1, 0x1000, 0x1000),
            'EEECFG3_lpi_stop': (0x04D1, 0x1000, 0),
            'RECIPE_0428_dpd_enable': (0x0428, 4, 4),
            'compliance_100_disable': (0x0428, 0x0010, 0),
            'compliance_100_enable': (0x0428, 0x0010, 0x0010),
            'compliance_100_count_low': (0x0428, 0x0020, 0),
            'compliance_100_count_high': (0x0428, 0x0020, 0x0020),
            'compliance_10_harmonic': (0x0027, 0xFFFF, 0x0013),
        }
        for aid, values in expected.items():
            action = next(a for a in self.data['writeActions'] if a['id'] == aid)
            self.assertEqual((int(self.register(action['register'])['address'], 16),
                              int(action['mask'], 16), int(action['value'], 16)), values)
            self.assertIsNone(action['neutral'])
        recipe = next(p for p in self.data['procedures'] if p['id'] == 'eee_initialize')
        self.assertEqual(recipe['writeActions'], list(expected)[:15])
        self.assertIn({'register': 'BMCR', 'mask': '0x7F80'}, recipe['ownedMasks'])

    def test_new_actions_cannot_drop_gates_or_change_literals(self):
        original = copy.deepcopy(self.data)
        for aid in ('EEECFG3_eee', 'RECIPE_0428_dpd_enable', 'compliance_10_harmonic'):
            for change in ('mask', 'value', 'gates'):
                with self.subTest(action=aid, change=change):
                    self.data = copy.deepcopy(original)
                    action = next(a for a in self.data['writeActions'] if a['id'] == aid)
                    action[change] = [] if change == 'gates' else '0x0000'
                    self.rejected_file('FAIL:')
        self.data = copy.deepcopy(original)
        clock_action = next(a for a in self.data['writeActions'] if a['id'] == 'CLK_OUT_LED_STATUS_eee')
        clock_action['gates'].remove('pin_fields')
        self.rejected_file('clock recipe must retain its pin conflict gate')

    def test_remaining_rows_cannot_revert_to_pending(self):
        names = ('LEDCR MLEDCR COMPT 10M_CFG IO_CFG1 LED0_GPIO_CFG LED1_GPIO_CFG '
                 'LED2_GPIO_CFG LED3_GPIO_CFG CLK_OUT_LED_STATUS VOD_CFG1 VOD_CFG2 '
                 'ANA_LD_PROG_SL ANA_RX10BT_CTRL GENCFG LEDCFG IOCTRL LEDCFG2 '
                 'RXFCFG1 RXFPMD1 RXFPMD2 RXFPMD3 RXFSOP1 RXFSOP2 RXFSOP3 '
                 'EEECFG3 EEE_04D3 EEE_04DF EEE_033E EEE_033F EEE_031B EEE_0466 '
                 'EEE_04CF EEE_0416 EEE_04F5 EEE_04E0 EEE_031F EEE_04F4 RECIPE_0428').split()
        original = copy.deepcopy(self.data)
        for name in names:
            with self.subTest(register=name):
                self.data = copy.deepcopy(original)
                self.register(name).update(status='PENDING', fields=[], resetValue=None,
                                           resetKnownMask=None, reservedMask=None)
                self.rejected_file('remaining record must have completed stage-5 research')

    def test_remaining_procedures_cannot_disappear(self):
        names = ('power_ieee power_active_sleep power_pll_control power_deep '
                 'eee_initialize eee_negotiation_status legacy_lpi wol_configure wol_acknowledge '
                 'led_configure pin_configure clock_configure mii_configure rmii_configure '
                 'repeater_configure tx100_configure tx10_configure analog_configure '
                 'compliance_session compatibility_configure link_recovery_configure '
                 'extended_link_configure isolation_configure collision_test').split()
        original = copy.deepcopy(self.data)
        for name in names:
            with self.subTest(procedure=name):
                self.data = copy.deepcopy(original)
                self.data['procedures'] = [p for p in self.data['procedures'] if p['id'] != name]
                for feature in self.data['features']:
                    feature['procedures'] = [p for p in feature['procedures'] if p != name]
                self.rejected_file('missing remaining procedure')

    def test_primary_software_owners_stay_in_implementation_stages(self):
        original = copy.deepcopy(self.data)
        for category, name in (('features', 'eee'), ('procedures', 'eee_initialize')):
            for stage in (10, 37):
                with self.subTest(category=category, stage=stage):
                    self.data = copy.deepcopy(original)
                    next(x for x in self.data[category] if x['id'] == name)['owner']['implementationStage'] = stage
                    self.rejected_file('primary software owner')
        self.data = copy.deepcopy(original)
        self.field('CR2', 0x0020)['owner'] = 'negotiation'
        self.rejected_file('control field has the wrong implementation owner')

    def test_programmable_feature_cannot_be_reclassified_as_hardware(self):
        feature = next(f for f in self.data['features'] if f['id'] == 'power')
        feature['scope'] = 'HARDWARE_ONLY'
        self.rejected_file('feature scope must retain documented responsibility')

    def test_exception_cannot_be_borrowed_by_another_procedure(self):
        next(p for p in self.data['procedures'] if p['id'] == 'collision_test')['writeActions'] = ['compliance_10_harmonic']
        self.rejected_file('required procedure write actions are missing or changed')

    def test_exception_gates_follow_actions_into_procedures(self):
        procedure = next(p for p in self.data['procedures'] if p['id'] == 'eee_initialize')
        procedure['gates'].remove('eee_wire')
        self.rejected_file('procedure must retain its write action evidence gates')

    def test_printed_eee_disposition_cannot_claim_wire_resolution(self):
        original = copy.deepcopy(self.data)
        self.data['printedAddresses'][0]['status'] = 'COMPLETE'
        self.rejected_file('printed address must retain blocked wire disposition')
        self.data = copy.deepcopy(original)
        self.data['printedAddresses'][0]['controls'] = ['']
        self.rejected_file('expected nonempty text')

    def test_eee_digits_cannot_be_stripped_into_wire_offsets(self):
        original = copy.deepcopy(self.data)
        for devad, offset in (('0x03', '0x0001'), ('0x03', '0x0014'), ('0x07', '0x003C')):
            with self.subTest(devad=devad, offset=offset):
                self.data = copy.deepcopy(original)
                row = copy.deepcopy(self.register('CDSCR2'))
                row.update(id='guessed_offset', name='GUESSED_OFFSET', space='MMD',
                           devad=devad, address=offset, aliases=[])
                self.data['registers'].append(row)
                self.rejected_file('undocumented primary address')


if __name__ == '__main__':
    unittest.main()
