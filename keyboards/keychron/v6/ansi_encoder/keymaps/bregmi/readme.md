# bregmi — Keychron V6 ANSI with encoder

Ported from origin/my_stuff (57bb3a72e6) to QMK 0.34.7
(upstream d194c077ed). The original branch remains available for reference.

## Behavior

- Preserves the four layers, Mac/Windows switch, and volume/brightness
  encoder mappings. F13–F16 are available on both base layers.
- Mac mode restores Option/Command positions, with Command next to Space,
  and Fn activates MAC_FN. The top row provides brightness/media controls;
  Fn accesses F1–F12. F3/F4 positions are unassigned without Fn, as upstream.
- Windows mode retains its custom layout with F1–F12 directly available.
- Turbo click remains on the key between Print Screen and the lighting key.
  Hold for repeated left clicks; double tap to latch; tap again to stop.
  The original 16 ms click period is retained via MOUSE_TURBO_CLICK_PERIOD.
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

## Build

Initialize the pinned module dependency after cloning:

```sh
git submodule update --init --recursive
qmk compile -kb keychron/v6/ansi_encoder -km bregmi
```

Output: keychron_v6_ansi_encoder_bregmi.bin in the QMK repository root.

## Hardware verification after flashing

Check both switch positions, all keys and Fn bindings, F13–F16 in both modes,
encoder volume and Fn brightness, RGB controls, and Caps Lock indication.
Verify turbo click stops on release, latches on a double tap, and stops on
another tap. Verify W+S and A+D report both keys normally.

The build does not flash the keyboard. Use this firmware only for the original
wired V6 ANSI with encoder, not V6 Max or another variant.
