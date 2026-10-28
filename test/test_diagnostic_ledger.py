"""Prompt-4 source oracles and malformed-record coverage, not driver tests."""

import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import verify_ledger as ledger


class DiagnosticLedgerTests(unittest.TestCase):
    def setUp(self):
        self.data = ledger.load_ledger(ledger.DEFAULT_LEDGER)

    def register(self, name):
        return next(r for r in self.data['registers'] if r['name'] == name)

    def rejected_file(self, message):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'malformed.json'
            path.write_text(json.dumps(self.data), encoding='utf-8')
            result = subprocess.run(
                [sys.executable, str(ledger.ROOT / 'tools/verify_ledger.py'), str(path)],
                text=True, capture_output=True, check=False)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(message, result.stderr)

    def test_diagnostic_rows_cannot_revert_to_pending(self):
        # Fixed scope independently transcribed from the prompt-4 inventory.
        names = ('FLDS PHYSCR MISR1 MISR2 FCSCR RECR BISCR RCSR 10BTSCR '
                 'BICSR1 BICSR2 CDCR FLD_CFG1 FLD_CFG2 CDSCR CDSCR2 CDSCR3 '
                 'TDR_175 TDR_176 CDSCR4 TDR_178 CDLRR1 CDLRR2 CDLRR3 CDLRR4 '
                 'CDLRR5 CDLAR1 CDLAR2 CDLAR3 CDLAR4 CDLAR5 CDLAR6 MSE_Val RXFS '
                 'RECIPE_0123 LOOP_0122 LOOP_0130').split()
        original = copy.deepcopy(self.data)
        for name in names:
            with self.subTest(register=name):
                self.data = copy.deepcopy(original)
                self.register(name).update(status='PENDING', fields=[], resetValue=None,
                                           resetKnownMask=None, reservedMask=None)
                self.rejected_file('diagnostic record must have completed')

    def test_diagnostic_features_cannot_claim_pending_research(self):
        original = copy.deepcopy(self.data)
        for name in ('loopback', 'bist', 'cable', 'interrupts', 'counters',
                     'quality', 'fast_drop', 'calibration'):
            with self.subTest(feature=name):
                self.data = copy.deepcopy(original)
                next(f for f in self.data['features'] if f['id'] == name)['evidence']['documented'] = 'PENDING'
                self.rejected_file('completed research stage requires documented coverage')

    def test_independent_fast_drop_fields(self):
        # Original SNLS647I pp63,76; separately reviewed by the event reviewer.
        expected = {
            'FLDS': (0x0000, 0xFE0F, {'RC': 0x01F0}),
            'FLD_CFG1': (0x8147, 0x000F, {'RW': 0xFFF0}),
            'FLD_CFG2': (0x2284, 0xFFC0, {'RW': 0x003F}),
        }
        for name, (reset, reserved, access) in expected.items():
            with self.subTest(register=name):
                row = self.register(name)
                self.assertEqual(int(row['resetValue'], 16), reset)
                self.assertEqual(int(row['resetKnownMask'], 16), 0xFFFF)
                self.assertEqual(int(row['reservedMask'], 16), reserved)
                actual = {}
                for field in row['fields']:
                    actual[field['access']] = actual.get(field['access'], 0) | int(field['mask'], 16)
                self.assertEqual(actual, access)
        for field in self.register('FLD_CFG1')['fields']:
            self.assertEqual(field['prohibited'], ['0x0'])
        # Section8.3.17's literal cannot be confused with prose20errors/10us.
        self.assertEqual((0x0417 & 0xFC00) >> 10, 1)
        self.assertEqual((0x0417 & 0x03F0) >> 4, 1)
        self.assertIn('fld_threshold_recipe', self.register('FLD_CFG1')['gates'])

    def test_independent_fast_drop_timing_qualifiers(self):
        # Original p15 MAX column, p17 footnote5; these are not host deadlines.
        timings = {t['id']: t for t in self.data['timings']}
        for name, value in (('fld_strap_150m', 10), ('fld_energy', 10), ('fld_snr', 10),
                            ('fld_mlt3', 10), ('fld_rx_error', 10), ('fld_descrambler', 11)):
            with self.subTest(timing=name):
                timing = timings[name]
                self.assertEqual((timing['value'], timing['unit'], timing['qualifier']),
                                 (value, 'us', 'MAX'))
        for name in ('fld_snr', 'fld_mlt3', 'fld_descrambler'):
            self.assertIn('footnote5 requires RX_Error count or Signal/Energy Loss', timings[name]['conditions'])

    def test_each_diagnostic_procedure_is_required(self):
        names = ('loopback_near loopback_digital loopback_reverse loopback_external '
                 'bist_run bist_capture cable_configure cable_manual cable_autorun '
                 'cable_capture configure_fast_drop interrupt_configuration '
                 'interrupt_test collect_event_counters signal_quality_sample calibration_recovery').split()
        original = copy.deepcopy(self.data)
        for name in names:
            with self.subTest(procedure=name):
                self.data = copy.deepcopy(original)
                self.data['procedures'] = [p for p in self.data['procedures'] if p['id'] != name]
                for feature in self.data['features']:
                    feature['procedures'] = [p for p in feature['procedures'] if p != name]
                self.rejected_file('missing diagnostic procedure')

    def test_write_action_is_an_exact_allowance(self):
        original = copy.deepcopy(self.data)
        changes = ({'mask': '0xFFFF'}, {'value': '0x0000'}, {'neutral': '0x8000'},
                   {'register': 'CDSCR2', 'feature': 'cable'},
                   {'register': 'PHYIDR1', 'feature': 'identity'})
        for change in changes:
            with self.subTest(change=change):
                self.data = copy.deepcopy(original)
                next(a for a in self.data['writeActions'] if a['id'] == 'BICSR1_freezeClear').update(change)
                self.rejected_file('write action differs from exact sourced allowance')

    def test_write_action_cannot_be_invented_or_removed(self):
        action = copy.deepcopy(self.data['writeActions'][0])
        action['id'] = 'invented_action'
        self.data['writeActions'].append(action)
        self.rejected_file('exact sourced action inventory')
        self.data['writeActions'].pop()
        self.data['writeActions'] = [a for a in self.data['writeActions'] if a['id'] != 'BICSR1_freezeClear']
        for procedure in self.data['procedures']:
            procedure['writeActions'] = [a for a in procedure.get('writeActions', []) if a != 'BICSR1_freezeClear']
        self.rejected_file('exact sourced action inventory')

    def test_capture_action_cannot_move_to_another_procedure(self):
        capture = next(p for p in self.data['procedures'] if p['id'] == 'bist_capture')
        capture['writeActions'] = []
        next(p for p in self.data['procedures'] if p['id'] == 'bist_run')['writeActions'] = ['BICSR1_freezeClear']
        self.rejected_file('required procedure write actions are missing')

    def test_recipe_unknown_fields_and_addresses_stay_explicit(self):
        original = copy.deepcopy(self.data)
        self.register('LOOP_0122')['resetValue'] = '0x0000'
        self.rejected_file('pending/recipe records must retain unknown')
        self.data = copy.deepcopy(original)
        self.register('LOOP_0122')['name'] = 'RENAMED_LOOP'
        self.rejected_file('recipe-only completion requires an explicit coverage update')
        self.data = copy.deepcopy(original)
        self.register('LOOP_0122')['address'], self.register('LOOP_0130')['address'] = '0x0130', '0x0122'
        self.rejected_file('diagnostic recipe address/name mapping changed')

    def test_cdcr_command_neutral_is_required(self):
        start = next(f for f in self.register('CDCR')['fields'] if f['mask'] == '0x8000')
        start['neutral'] = '0x1'
        self.rejected_file('CDCR start command-neutral encoding must be zero')

    def test_independent_diagnostic_reset_access_oracle(self):
        # Original-PDF source-review tables, independently cross-reviewed by the
        # event reviewer. No expected value is generated from the JSON fields.
        expected = {
            'BISCR': (0x0100, 0x80A0, {'RW': 0x705F, 'R': 0x0B00, 'RH': 0x0400}),
            'BICSR1': (0x007D, 0x0000, {'RW': 0x00FF, 'R': 0xFF00}),
            'BICSR2': (0x05EE, 0xF800, {'RW': 0x07FF}),
            'CDCR': (0x0102, 0x3FFC, {'RW': 0xC000, 'R': 0x0003}),
            'CDSCR': (0x0C12, 0x888F, {'RW': 0x7770}),
            'CDSCR2': (0xC850, 0xFFFF, {}),
            'CDSCR3': (0x0D04, 0x0000, {'RW': 0xFFFF}),
            'TDR_175': (0x1004, 0xC7F0, {'RW': 0x380F}),
            'TDR_176': (0x0005, 0xFFE0, {'RW': 0x001F}),
            'CDSCR4': (0x1E00, 0xE0FF, {'RW': 0x1F00}),
            'TDR_178': (0x0002, 0xFFF8, {'RW': 0x0007}),
            'CDLRR1': (0x0000, 0xFF00, {'R': 0x00FF}),
            'CDLAR1': (0x0000, 0xFF80, {'R': 0x007F}),
            'CDLAR6': (0x0000, 0xF7DF, {'R': 0x0820}),
            'PHYSCR': (0x0108, 0x04E0, {'RW': 0xFB1F}),
            'MISR1': (0x0000, 0x0000, {'RC': 0xFF00, 'RW': 0x00FF}),
            'MISR2': (0x0000, 0x0000, {'RC': 0xFF00, 'RW': 0x00FF}),
            'FCSCR': (0x0000, 0xFF00, {'RC': 0x00FF}),
            'RECR': (0x0000, 0x0000, {'RC': 0xFFFF}),
            '10BTSCR': (0x0000, 0xC16E, {'RW': 0x3E81, 'R': 0x0010}),
            'MSE_Val': (0x0000, 0x0000, {'R': 0xFFFF}),
            'RXFS': (0x1000, 0xEF1E, {'RW': 0x1000, 'RCH': 0x00E1}),
        }
        for name in ('CDLRR2', 'CDLRR3', 'CDLRR4', 'CDLRR5',
                     'CDLAR2', 'CDLAR3', 'CDLAR4', 'CDLAR5'):
            expected[name] = (0x0000, 0xFFFF, {})
        for name, (reset, reserved, access) in expected.items():
            with self.subTest(register=name):
                row = self.register(name)
                self.assertEqual(int(row['resetValue'], 16), reset)
                self.assertEqual(int(row['resetKnownMask'], 16), 0xFFFF)
                self.assertEqual(int(row['reservedMask'], 16), reserved)
                actual = {}
                for field in row['fields']:
                    actual[field['access']] = actual.get(field['access'], 0) | int(field['mask'], 16)
                self.assertEqual(actual, access)
        # RCSR's X bits are explicitly unknown, not zero-filled defaults.
        row = self.register('RCSR')
        self.assertEqual((int(row['resetValue'], 16), int(row['resetKnownMask'], 16),
                          int(row['reservedMask'], 16)), (0x0041, 0xFF5F, 0xFE40))
        self.assertEqual({(int(f['mask'], 16), f['access']) for f in row['fields']},
                         {(0x0100, 'RW'), (0x0080, 'RHW'), (0x0020, 'RHW'),
                          (0x0010, 'RW'), (0x0008, 'RC'), (0x0004, 'RC'), (0x0003, 'RW')})

    def test_independent_field_encodings(self):
        def field(name, mask):
            return next(f for f in self.register(name)['fields'] if int(f['mask'], 16) == mask)
        self.assertEqual({int(v, 16) for v in field('BISCR', 0x001F)['legal']},
                         {0, 1, 2, 4, 8, 0x10})
        self.assertEqual({int(v, 16) for v in field('BISCR', 0x3000)['legal']}, {0, 1, 2, 3})
        self.assertEqual({int(v, 16) for v in field('CDSCR', 0x0700)['prohibited']}, {7})
        self.assertEqual({int(v, 16) for v in field('CDSCR', 0x0070)['legal']}, {1, 2, 3, 4, 5})
        self.assertEqual({int(v, 16) for v in field('10BTSCR', 0x1E00)['legal']}, set(range(9)))
        self.assertEqual({int(v, 16) for v in field('PHYSCR', 0x3000)['legal']}, {0, 2})

    def test_independent_recipe_actions(self):
        expected = {
            'BICSR1_freezeClear': ('BICSR1', 0x001B, 0x8000, 0x8000),
            'LOOP_0122_digital': ('LOOP_0122', 0x0122, 0xFFFF, 0x2000),
            'RECIPE_0123_digital': ('RECIPE_0123', 0x0123, 0xFFFF, 0x2000),
            'LOOP_0130_digital': ('LOOP_0130', 0x0130, 0xFFFF, 0x47FF),
        }
        for action in self.data['writeActions']:
            name, address, mask, value = expected[action['id']]
            self.assertEqual(action['register'], name)
            self.assertEqual(int(self.register(name)['address'], 16), address)
            self.assertEqual((int(action['mask'], 16), int(action['value'], 16)), (mask, value))

    def test_independent_diagnostic_field_partitions(self):
        # Original tables8-33/35..41/44..47/52..72/91, not aggregate access alone.
        expected = {
            'FLDS': {0x01F0}, 'FLD_CFG1': {0xFC00, 0x03F0}, 'FLD_CFG2': {0x003F},
            'BISCR': {0x4000, 0x3000, 0x0800, 0x0400, 0x0200, 0x0100, 0x0040, 0x001F},
            'BICSR1': {0xFF00, 0x00FF}, 'BICSR2': {0x07FF},
            'CDCR': {0x8000, 0x4000, 0x0002, 0x0001},
            'CDSCR': {0x4000, 0x2000, 0x1000, 0x0700, 0x0070},
            'CDSCR3': {0xFF00, 0x00FF}, 'TDR_175': {0x3800, 0x000F},
            'TDR_176': {0x001F}, 'CDSCR4': {0x1F00}, 'TDR_178': {0x0007},
            'CDLRR1': {0x00FF}, 'CDLAR1': {0x007F}, 'CDLAR6': {0x0800, 0x0020},
            'PHYSCR': {0x8000, 0x4000, 0x3000, 0x0800, 0x0300, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001},
            'MISR1': {1 << bit for bit in range(16)},
            'MISR2': {1 << bit for bit in range(16)},
            'FCSCR': {0x00FF}, 'RECR': {0xFFFF},
            'RCSR': {0x0100, 0x0080, 0x0020, 0x0010, 0x0008, 0x0004, 0x0003},
            '10BTSCR': {0x2000, 0x1E00, 0x0080, 0x0010, 0x0001},
            'MSE_Val': {0xFFFF}, 'RXFS': {0x1000, 0x0080, 0x0040, 0x0020, 0x0001},
        }
        for name in ('CDSCR2', 'CDLRR2', 'CDLRR3', 'CDLRR4', 'CDLRR5',
                     'CDLAR2', 'CDLAR3', 'CDLAR4', 'CDLAR5'):
            expected[name] = set()
        self.assertEqual(set(expected), ledger.DIAGNOSTIC_REGISTERS)
        for name, masks in expected.items():
            with self.subTest(register=name):
                self.assertEqual({int(f['mask'], 16) for f in self.register(name)['fields']}, masks)

    def test_diagnostic_procedures_own_changed_controls(self):
        # Original-table masks applied to the independently reviewed procedure
        # actions. Whole-word digital recipe changes more than the loop selector.
        expected = {
            'configure_fast_drop': {('CR3', 0x040F), ('FLD_CFG1', 0xFFF0), ('FLD_CFG2', 0x003F)},
            'loopback_near': {('BMCR', 0x7100), ('BISCR', 0x005F)},
            'loopback_digital': {('BMCR', 0x7D80), ('BISCR', 0x705F), ('PHYRCR', 0x4000)},
            'loopback_reverse': {('BMCR', 0x4000), ('BISCR', 0x005F), ('PHYSCR', 0x0300)},
            'loopback_external': {('BMCR', 0x4000), ('BISCR', 0x005F)},
            'bist_run': {('BISCR', 0x7000), ('BICSR1', 0x00FF), ('BICSR2', 0x07FF)},
            'bist_capture': {('BICSR1', 0x00FF)},
            'cable_configure': {('CDSCR', 0x7770), ('CDSCR3', 0xFFFF), ('TDR_175', 0x380F),
                                ('TDR_176', 0x001F), ('CDSCR4', 0x1F00), ('TDR_178', 0x0007)},
            'cable_manual': {('CR1', 0x0100), ('CDCR', 0x8000)},
            'cable_autorun': {('CR1', 0x0100)}, 'cable_capture': set(),
            'collect_event_counters': set(), 'signal_quality_sample': set(),
            'interrupt_configuration': {('PHYSCR', 0x000B), ('MISR1', 0x00FF),
                                         ('MISR2', 0x00FF), ('RXFS', 0x1000)},
            'interrupt_test': {('PHYSCR', 0x0004)},
            'calibration_recovery': {('CDCR', 0x4000), ('PHYRCR', 0x8000)},
        }
        for pid, masks in expected.items():
            procedure = next(p for p in self.data['procedures'] if p['id'] == pid)
            self.assertEqual({(m['register'], int(m['mask'], 16)) for m in procedure['ownedMasks']}, masks)

    def test_exception_actions_retain_evidence_gates(self):
        original = copy.deepcopy(self.data)
        for aid in ('BICSR1_freezeClear', 'LOOP_0122_digital',
                    'RECIPE_0123_digital', 'LOOP_0130_digital'):
            with self.subTest(action=aid):
                self.data = copy.deepcopy(original)
                next(a for a in self.data['writeActions'] if a['id'] == aid)['gates'] = []
                self.rejected_file('write action must retain its evidence gate')


if __name__ == '__main__':
    unittest.main()
