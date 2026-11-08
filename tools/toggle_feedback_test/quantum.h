#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(a,b,c)
#define MOUSE_TURBO_CLICK 100
#define MS_BTN1 101
#define MS_BTN2 104
#define TAPPING_TERM 200
#define INVALID_DEFERRED_TOKEN 0
#define IS_RGB_MATRIX_KEYCODE(k) ((k) == 102)
typedef uint8_t deferred_token;
typedef struct { struct { bool pressed; uint16_t time; } event; } keyrecord_t;
static struct { bool nkro; } keymap_config;
static uint32_t now;
static bool rgb = true, suspended, ac = true;
static uint8_t colors[109][3];
static bool autocorrect_is_enabled(void) { return ac; }
static uint32_t timer_read32(void) { return now; }
static uint32_t timer_elapsed32(uint32_t t) { return now-t; }
static bool timer_expired(uint16_t t, uint16_t deadline) { return (int16_t)(t-deadline) >= 0; }
static bool rgb_matrix_is_enabled(void) { return rgb; }
static void rgb_matrix_enable_noeeprom(void) { rgb=true; }
static void rgb_matrix_disable_noeeprom(void) { rgb=false; }
static bool rgb_matrix_get_suspend_state(void) { return suspended; }
static void rgb_matrix_set_color(uint8_t i,uint8_t r,uint8_t g,uint8_t b) { colors[i][0]=r; colors[i][1]=g; colors[i][2]=b; }
static uint16_t held_button;
static unsigned presses;
static void register_code16(uint16_t k) {
    assert(k == MS_BTN1 || k == MS_BTN2);
    assert(!held_button); held_button=k; ++presses;
}
static void unregister_code16(uint16_t k) {
    assert(held_button==k); held_button=0;
}
static deferred_token defer_exec(uint32_t delay, uint32_t (*cb)(uint32_t,void*),void* arg) { (void)delay;(void)cb;(void)arg; return 1; }
static void cancel_deferred_exec(deferred_token token) { (void)token; }
#define QK_USER 2000
#define QK_AUTOCORRECT_ON 110
#define QK_AUTOCORRECT_OFF 111
#define QK_AUTOCORRECT_TOGGLE 112
#define MO(layer) (500+(layer))
static uint32_t layer_state;
static bool layer_state_is(uint8_t layer) { return (layer_state & (1UL<<layer)) != 0; }
static void layer_on(uint8_t layer) { layer_state |= 1UL<<layer; }
#define MATRIX_ROWS 6
#define MATRIX_COLS 20
#define NO_LED 255
#define KC_TRNS 1
#define KC_NO 0
#define KC_A 4
#define KC_Z 29
#define AC_TOGG QK_AUTOCORRECT_TOGGLE
#define NK_TOGG 113
static struct { uint8_t matrix_co[MATRIX_ROWS][MATRIX_COLS]; } g_led_config;
