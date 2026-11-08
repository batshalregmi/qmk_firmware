"""Small regression checks for parsing and source-driven rendering."""
import json
from pathlib import Path
import tempfile
import unittest

from render_layers import DEFAULT, ROOT, generate, parse_source, split_args


class ViewerTests(unittest.TestCase):
    def test_nested_macro_arguments(self):
        self.assertEqual(split_args('KC_A, LT(2, KC_SPC), S(LGUI(KC_TAB))'),
                         ['KC_A', 'LT(2, KC_SPC)', 'S(LGUI(KC_TAB))'])

    def test_comments_and_duplicate_layers(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'keymap.c'
            path.write_text('const int keymaps[][1] = { /* [FAKE] = LAYOUT(KC_B), */\n'
                            '[BASE] = LAYOUT(KC_A, LT(2, KC_SPC)), };')
            self.assertEqual(parse_source(path)[0],
                             {'BASE': ('LAYOUT', ['KC_A', 'LT(2, KC_SPC)'])})
            path.write_text('const int keymaps[][1] = { [BASE] = LAYOUT(KC_A),'
                            '[BASE] = LAYOUT(KC_B), };')
            with self.assertRaises(ValueError):
                parse_source(path)

    def test_live_source_changes_and_transparency(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'keymap.c'
            output = Path(directory)/'images'
            keyboard = ROOT/'keyboards/keychron/v6/ansi_encoder/keyboard.json'
            path.write_text(DEFAULT.read_text())
            generate(path, keyboard, output)
            before = json.loads((output/'layers.json').read_text())
            self.assertEqual(before['MAC_FN']['keys'][15]['effective'], 'AC_TOGG')
            self.assertEqual(before['MAC_FN']['keys'][0]['effective'], 'KC_ESC')
            self.assertTrue(before['MAC_FN']['keys'][0]['inherited'])
            self.assertEqual(before['MAC_FN']['keys'][17]['effective'], 'KC_F13')
            self.assertEqual(before['WIN_FN']['keys'][18]['effective'], 'KC_F14')
            self.assertNotIn('MAC_GAME', before)
            self.assertEqual(before['MAC_FN']['encoder'], [['TURBO_LEFT', 'TURBO_RIGHT']])
            self.assertEqual(before['WIN_FN']['encoder'], [['TURBO_LEFT', 'TURBO_RIGHT']])
            self.assertEqual(len(before), 4)
            # Changing a base mapping must update both base and inherited Fn output.
            path.write_text(path.read_text().replace('KC_ESC,', 'KC_A,', 1))
            generate(path, keyboard, output)
            after = json.loads((output/'layers.json').read_text())
            self.assertEqual(after['MAC_FN']['keys'][0]['effective'], 'KC_A')
            self.assertEqual(after['WIN_BASE'], before['WIN_BASE'])
            self.assertTrue((output/'all_layers.png').is_file())


if __name__ == '__main__':
    unittest.main()
