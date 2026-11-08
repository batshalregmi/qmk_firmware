#include QMK_KEYBOARD_H
#include "autocorrect.h"
#include "custom_features.h"
#include "community_modules.h"

#define CONFIRMATION_MS 350
#define CONFIRMATION_BRIGHTNESS 100
#define DIAL_PREVIEW_MS 1200

static bool flash_active;
static bool flash_on;
static bool restore_rgb_off;
static uint32_t flash_started;
static bool previous_autocorrect;
static bool previous_nkro;
static const uint16_t turbo_periods[] = {200, 100, 50, 40, 16, 10};
// Negative = left click, zero = disabled, positive = right click.
static int8_t turbo_dial = -5;
static bool dial_preview;
static uint32_t dial_changed;

unsigned short bregmi_turbo_period(void) {
    uint8_t step = turbo_dial < 0 ? -turbo_dial : turbo_dial;
    return turbo_periods[step ? step - 1 : 0];
}

unsigned short bregmi_turbo_key(void) {
    return turbo_dial < 0 ? MS_BTN1 : turbo_dial > 0 ? MS_BTN2 : KC_NO;
}

// Observe the pinned getreuer module's double-tap state machine. The module
// owns clicking; keep this observer in sync when updating that dependency.
static bool turbo_locked;
static bool turbo_held;
static bool turbo_tapped;
static uint16_t turbo_deadline;

static bool fn_held(void) {
    return layer_state_is(MAC_FN) || layer_state_is(WIN_FN);
}

static void restore_lighting_if_idle(void) {
    if (restore_rgb_off && !flash_active && !dial_preview && !turbo_locked) {
        rgb_matrix_disable_noeeprom();
        restore_rgb_off = false;
    }
}

static void enable_feedback_lighting(void) {
    restore_rgb_off = restore_rgb_off || !rgb_matrix_is_enabled();
    rgb_matrix_enable_noeeprom();
}

static void end_flash(void) {
    flash_active = false;
    restore_lighting_if_idle();
}

static void confirm_toggle(bool enabled) {
    flash_active = true;
    flash_on = enabled;
    flash_started = timer_read32();
    enable_feedback_lighting();
}

// The module has no public stop API. Reset its pending double-tap first, then
// send the event that stops the current run. Do this BEFORE changing the button
// expression so its release always matches the button it previously pressed.
static void stop_turbo_for_configuration(keyrecord_t *record) {
    keyrecord_t stop = *record;
    process_record_mouse_turbo_click(KC_NO, &stop);
    if (turbo_locked || turbo_held) {
        stop.event.pressed = turbo_locked;
        process_record_mouse_turbo_click(MOUSE_TURBO_CLICK, &stop);
    }
    turbo_locked = turbo_held = turbo_tapped = false;
}

void keyboard_post_init_user(void) {
    previous_autocorrect = autocorrect_is_enabled();
    previous_nkro = keymap_config.nkro;
}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == TURBO_LEFT || keycode == TURBO_RIGHT) {
        if (record->event.pressed) {
            stop_turbo_for_configuration(record);
            if (keycode == TURBO_LEFT && turbo_dial > -6) --turbo_dial;
            if (keycode == TURBO_RIGHT && turbo_dial < 6) ++turbo_dial;
            flash_active = false;
            dial_preview = true;
            dial_changed = timer_read32();
            enable_feedback_lighting();
        }
        return false;
    }
    // Restore the real RGB setting before a lighting key changes/saves it.
    if (record->event.pressed && IS_RGB_MATRIX_KEYCODE(keycode)) {
        dial_preview = false;
        end_flash();
        if (restore_rgb_off) {
            rgb_matrix_disable_noeeprom();
            restore_rgb_off = false;
        }
    }
    if (keycode != MOUSE_TURBO_CLICK) {
        turbo_tapped = false;
        return true;
    }
    if (!turbo_dial) {
        // Neutral configures no clicks, even if the turbo key is double tapped.
        if (record->event.pressed) confirm_toggle(false);
        return false;
    }
    turbo_held = record->event.pressed;
    if (!record->event.pressed) return true;
    bool was_locked = turbo_locked;
    if (turbo_tapped && !timer_expired(record->event.time, turbo_deadline)) {
        turbo_locked = true;
    } else if (turbo_locked) {
        turbo_locked = false;
        turbo_tapped = false;
        confirm_toggle(false);
        return true;
    }
    turbo_tapped = true;
    turbo_deadline = record->event.time + TAPPING_TERM;
    if (turbo_locked != was_locked) confirm_toggle(turbo_locked);
    return true;
}

void housekeeping_task_user(void) {
    if (dial_preview && !fn_held() && timer_elapsed32(dial_changed) >= DIAL_PREVIEW_MS) {
        dial_preview = false;
    }
    if (turbo_locked) enable_feedback_lighting();
    bool autocorrect = autocorrect_is_enabled();
    if (autocorrect != previous_autocorrect) {
        previous_autocorrect = autocorrect;
        confirm_toggle(autocorrect);
    }
    if (keymap_config.nkro != previous_nkro) {
        previous_nkro = keymap_config.nkro;
        confirm_toggle(previous_nkro);
    }
    if (rgb_matrix_get_suspend_state()) dial_preview = false;
    if (flash_active && (timer_elapsed32(flash_started) >= CONFIRMATION_MS || rgb_matrix_get_suspend_state())) end_flash();
    restore_lighting_if_idle();
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (flash_active) {
        for (uint8_t i = led_min; i < led_max; ++i) {
            rgb_matrix_set_color(i, flash_on ? 0 : CONFIRMATION_BRIGHTNESS,
                                 flash_on ? CONFIRMATION_BRIGHTNESS : 0, 0);
        }
        return false;
    }
    if (restore_rgb_off || dial_preview) {
        for (uint8_t i = led_min; i < led_max; ++i) {
            rgb_matrix_set_color(i, restore_rgb_off ? 0 : 3, restore_rgb_off ? 0 : 5, restore_rgb_off ? 0 : 7);
        }
    }
    uint8_t turbo_led = g_led_config.matrix_co[0][15];
    if (dial_preview) {
        uint8_t steps = turbo_dial < 0 ? -turbo_dial : turbo_dial;
        // Grow outward from the F6/F7 midpoint: blue left, purple right.
        for (uint8_t i = 0; i < 6; ++i) {
            uint8_t col = turbo_dial < 0 ? 6 - i : 7 + i;
            uint8_t led = g_led_config.matrix_co[0][col];
            if (led == NO_LED || led < led_min || led >= led_max || i >= steps) continue;
            rgb_matrix_set_color(led, turbo_dial < 0 ? 0 : 85, turbo_dial < 0 ? 45 : 0, 100);
        }
        if (turbo_led != NO_LED && turbo_led >= led_min && turbo_led < led_max) {
            rgb_matrix_set_color(turbo_led, steps ? (turbo_dial < 0 ? 0 : 85) : 70,
                                 steps ? (turbo_dial < 0 ? 45 : 0) : 70, steps ? 100 : 70);
        }
    }
    if (turbo_locked && turbo_led != NO_LED && turbo_led >= led_min && turbo_led < led_max) {
        rgb_matrix_set_color(turbo_led, 0, 100, 0);
    }
    return !restore_rgb_off && !dial_preview;
}
