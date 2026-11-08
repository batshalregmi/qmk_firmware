# bregmi — Keychron V6 ANSI with encoder

Ported from origin/my_stuff (57bb3a72e6) to QMK 0.34.7
(upstream d194c077ed). The original branch remains available for reference.

## Behavior

- Preserves the four original layers, Mac/Windows switch, and base-layer
  volume encoder mappings. F13–F16 are available on both base layers.
- Mac mode restores Option/Command positions, with Command next to Space,
  and Fn activates MAC_FN. The top row provides brightness/media controls;
  Fn accesses F1–F12. F3 opens Mission Control; F4 sends Launchpad.
- Windows mode retains its custom layout with F1–F12 directly available.
- NKRO defaults to on when EEPROM settings are initialized. Fn+N toggles it
  in either mode and saves that choice across reconnects. Existing EEPROM
  settings take precedence until reset or changed with the toggle.
- Turbo click remains on the key between Print Screen and the lighting key.
  Hold for repeated clicks of the selected button; double tap to latch; tap again to stop.
  Left click at the original 16 ms period is the startup preset. Fn+knob
  selects the button and rate for the current session.
  Uses the getreuer/mouse_turbo_click community module, enabled in keymap.json.
  The module supplies TURBO and enables mouse keys and deferred execution.
- SOCD/Snap Tap is removed; its former Fn+P binding is transparent.
- Autocorrect uses the drashna/autocorrect module and its bundled dictionary.
  Fn+turbo toggles autocorrect in both Mac and Windows modes; it does not
  start turbo clicking. Autocorrect starts enabled after settings initialization
  and remembers its on/off state across power cycles. A keymap pre-processing
  hook mirrors the module state into the legacy QMK enable flag to work around
  the pinned module checking that flag instead of its own state.
- RGB controls use current RM_* keycodes. The toggle now uses upstream full
  RGB on/off behavior rather than the old V6 indicator-preserving override.
- Typing heatmap is the default when RGB settings are initialized. Existing
  EEPROM settings may select another effect.
- VIA is enabled. Existing VIA mappings can override these compiled defaults;
  assign AC_TOGG at the Fn+turbo position on layers 1 and 3, or reset saved
  mappings after exporting any custom VIA layout.

## Toggle confirmation

Autocorrect and NKRO state changes, plus autoclicker double-tap lock/unlock,
flash the whole keyboard green for on or red for off for 350 ms. Ordinary
hold-to-click use does not flash. Feedback also works with RGB off, then restores
the off state (except for the green autoclicker indicator while locked). Normal effects and saved lighting settings are preserved. A new
confirmation replaces an ongoing flash; RGB control keys cancel it first.

The autoclicker module does not expose its lock state, so the feedback observer
tracks its pinned double-tap logic. Check the feedback regression test when
updating that module. No module source is modified.

## Autoclicker dial

- **Hold Fn and rotate the knob:** configure the autoclicker on a signed dial:
  six left-click speeds, neutral off, and six right-click speeds.
  Counterclockwise moves toward left click; clockwise toward right click.
  Toward neutral slows clicking; farther away increases speed. The six presets
  on either side are **5, 10, 20, 25, 62.5, 100 clicks/sec**.
- **Configuration only:** turning never starts clicking. If already clicking
  (held or latched), turning stops it and releases the old mouse button before
  changing settings. Use the autoclicker key again to start with the new setup.
  At neutral the autoclicker key does nothing except show a red confirmation.
- **Dial display:** after turning, a blue bar grows from F6 toward F1 for left
  click; a purple bar grows from F7 toward F12 for right click. One through six
  lit keys indicate the speed step. The turbo key shows the mode color, or white
  at neutral. The preview remains while Fn is held and for at least 1.2 seconds
  after the last turn. It works with RGB off and restores off afterward.
- **Persistent lock indicator:** the original autoclicker key beside Print
  Screen stays green while clicking is latched. With RGB off, only that key
  remains lit after confirmation. Ordinary hold-to-click has no persistent light.
  Full-board confirmations take priority over the preview and other indicators.

The dial resets to **left click at 62.5 clicks/sec** on reboot. Fn+knob no longer
adjusts lighting brightness; Fn+W/S still do. F13/F14 retain their normal actions,
including with Fn. There are only the four original VIA layers (Mac base/Fn,
Windows base/Fn); no extra speed or gaming layers.

**Upgrade:** export VIA customizations before flashing, then reset EEPROM to
load the new encoder bindings, especially if trying the earlier five-layer
build. Holding Esc while reconnecting to enter DFU does this. Reapply mappings
selectively so an old VIA export does not restore the old Fn encoder bindings.
Custom encoder keycodes can appear unnamed in VIA.

## Build

Initialize the pinned module dependency after cloning:

```sh
git submodule update --init --recursive
qmk compile -kb keychron/v6/ansi_encoder -km bregmi
```

Output: keychron_v6_ansi_encoder_bregmi.bin in the QMK repository root.

## Hardware verification after flashing

Check both switch positions, all keys and Fn bindings, F13–F16 in both modes,
encoder volume and Fn click configuration, RGB controls, and Caps Lock indication.
Verify turbo click stops on release, latches on a double tap, and stops on
another tap. Verify W+S and A+D report both keys normally.
Test both button modes, neutral, both speed limits, and the colored light meter.
Verify turning while held or latched stops clicking without a stuck mouse button.
Verify holding Fn alone leaves the normal lighting unchanged, and check persistent
turbo indication with RGB both on and off.

The build does not flash the keyboard. Use this firmware only for the original
wired V6 ANSI with encoder, not V6 Max or another variant.
