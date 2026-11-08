#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "quantum.h"
#define QMK_KEYBOARD_H "quantum.h"
unsigned short bregmi_turbo_period(void);
unsigned short bregmi_turbo_key(void);
#define MOUSE_TURBO_CLICK_PERIOD bregmi_turbo_period()
#define MOUSE_TURBO_CLICK_KEY bregmi_turbo_key()
#include "../../modules/getreuer/mouse_turbo_click/mouse_turbo_click.c"
#include "../../keyboards/keychron/v6/ansi_encoder/keymaps/bregmi/features/toggle_feedback.c"

static void event(uint16_t code, bool pressed, uint32_t delay) {
    now += delay;
    keyrecord_t record = {.event={.pressed=pressed,.time=(uint16_t)now}};
    if (pre_process_record_user(code,&record)) process_record_mouse_turbo_click(code,&record);
    if (code == MOUSE_TURBO_CLICK && !pressed) {
        assert(turbo_locked == (click_token != INVALID_DEFERRED_TOKEN));
        if (!turbo_locked) assert(!held_button);
    }
}

static void dial(uint16_t direction) {
    unsigned before=presses;
    event(direction,true,1); event(direction,false,1);
    assert(presses==before); // configuration must never generate a new click
    assert(!held_button && !click_token && !turbo_locked);
}

static void expire(void) {
    layer_state=0; now+=1500; housekeeping_task_user();
}

int main(void) {
    for (int r=0;r<MATRIX_ROWS;++r) for (int c=0;c<MATRIX_COLS;++c) g_led_config.matrix_co[r][c]=NO_LED;
    for (int c=0;c<16;++c) g_led_config.matrix_co[0][c]=c;
    keyboard_post_init_user(); housekeeping_task_user(); assert(!flash_active);
    assert(turbo_dial==-5 && bregmi_turbo_key()==MS_BTN1 && bregmi_turbo_period()==16);
    event(MOUSE_TURBO_CLICK,true,1); assert(held_button==MS_BTN1);
    event(MOUSE_TURBO_CLICK,false,20); assert(!flash_active);
    event(MOUSE_TURBO_CLICK,true,40); event(MOUSE_TURBO_CLICK,false,20);
    assert(flash_active && flash_on && turbo_locked);
    event(MOUSE_TURBO_CLICK,true,250); event(MOUSE_TURBO_CLICK,false,20);
    assert(flash_active && !flash_on && !turbo_locked);
    end_flash(); rgb=false; ac=false; housekeeping_task_user();
    assert(rgb && flash_active && !flash_on);
    assert(!rgb_matrix_indicators_advanced_user(0,109));
    assert(colors[0][0]==100 && colors[108][0]==100);
    now+=350; housekeeping_task_user(); assert(!rgb && !flash_active);
    keymap_config.nkro=true; housekeeping_task_user(); assert(flash_on && rgb);
    ac=true; housekeeping_task_user(); now+=350; housekeeping_task_user(); assert(!rgb);
    ac=false; housekeeping_task_user(); event(102,true,1); assert(!flash_active && !rgb);
    rgb=true; keymap_config.nkro=false; housekeeping_task_user(); now+=350; housekeeping_task_user(); assert(rgb);

    // Left/right modes, a silent neutral point, and clamped dial limits.
    for (int i=0;i<5;++i) dial(TURBO_RIGHT);
    assert(turbo_dial==0 && bregmi_turbo_key()==KC_NO);
    unsigned before=presses;
    event(MOUSE_TURBO_CLICK,true,1); event(MOUSE_TURBO_CLICK,false,20);
    event(MOUSE_TURBO_CLICK,true,40); event(MOUSE_TURBO_CLICK,false,20);
    assert(presses==before && !click_token);
    dial(TURBO_RIGHT); assert(turbo_dial==1 && bregmi_turbo_period()==200);
    event(MOUSE_TURBO_CLICK,true,250); assert(held_button==MS_BTN2);
    assert(turbo_click_callback(0,NULL)==100 && !held_button);
    assert(turbo_click_callback(0,NULL)==100 && held_button==MS_BTN2);
    // Reconfigure while held: release old right button before selecting neutral.
    dial(TURBO_LEFT); assert(turbo_dial==0 && !held_button);
    event(MOUSE_TURBO_CLICK,false,10);
    for (int i=0;i<20;++i) dial(TURBO_LEFT);
    assert(turbo_dial==-6 && bregmi_turbo_period()==10);
    event(MOUSE_TURBO_CLICK,true,250); event(MOUSE_TURBO_CLICK,false,10);
    event(MOUSE_TURBO_CLICK,true,40); event(MOUSE_TURBO_CLICK,false,10);
    assert(turbo_locked && held_button==MS_BTN1);
    // Reconfigure while latched: no new clicks and no orphaned pressed button.
    for (int i=0;i<20;++i) dial(TURBO_RIGHT);
    assert(turbo_dial==6 && bregmi_turbo_key()==MS_BTN2 && bregmi_turbo_period()==10);

    // Blue left bar, purple right bar, neutral white, and preview restoration.
    rgb_matrix_indicators_advanced_user(0,109);
    assert(colors[7][0]==85 && colors[12][2]==100 && colors[1][2]!=100);
    for (int i=0;i<6;++i) dial(TURBO_LEFT);
    rgb_matrix_indicators_advanced_user(0,109);
    assert(colors[15][0]==70 && colors[15][1]==70 && colors[15][2]==70);
    assert(colors[6][2]!=100 && colors[7][2]!=100);
    dial(TURBO_LEFT); rgb_matrix_indicators_advanced_user(0,109);
    assert(colors[6][0]==0 && colors[6][1]==45 && colors[6][2]==100 && colors[5][2]!=100);
    expire(); rgb=false; dial(TURBO_LEFT); assert(rgb && restore_rgb_off);
    layer_on(MAC_FN); now+=2000; housekeeping_task_user(); assert(dial_preview);
    expire(); assert(!dial_preview && !rgb);
    dial(TURBO_RIGHT); event(102,true,1); assert(!dial_preview && !rgb);

    // Compare observer with real pinned module through interruptions, rollover,
    // configuration changes during press/release phases, and rapid taps.
    now=65500; srand(7);
    for (int i=0;i<4000;++i) {
        if (rand()%3==0) { event(103,true,1); event(103,false,1); }
        event(MOUSE_TURBO_CLICK,true,rand()%400);
        if (click_token && rand()%2) turbo_click_callback(0,NULL);
        if (rand()%3==0) dial(rand()%2 ? TURBO_LEFT : TURBO_RIGHT);
        event(MOUSE_TURBO_CLICK,false,rand()%400);
    }
    while (turbo_dial!= -1) dial(turbo_dial > -1 ? TURBO_LEFT : TURBO_RIGHT);
    dial(TURBO_LEFT); // ensures stopped even if random sequence ended latched
    expire(); rgb=true; layer_on(MAC_FN);
    rgb_matrix_set_color(0, 17, 23, 31);
    rgb_matrix_set_color(1, 29, 37, 41);
    assert(rgb_matrix_indicators_advanced_user(0,109));
    assert(colors[0][0]==17 && colors[0][1]==23 && colors[0][2]==31);
    assert(colors[1][0]==29 && colors[1][1]==37 && colors[1][2]==41); // Fn alone preserves effect
    confirm_toggle(false); rgb_matrix_indicators_advanced_user(0,109);
    assert(colors[0][0]==100 && colors[1][1]==0);
    expire(); rgb=false;
    event(103,true,1); event(MOUSE_TURBO_CLICK,true,250); event(MOUSE_TURBO_CLICK,false,20);
    event(MOUSE_TURBO_CLICK,true,40); event(MOUSE_TURBO_CLICK,false,20);
    now+=350; housekeeping_task_user(); assert(rgb && turbo_locked && !flash_active);
    assert(!rgb_matrix_indicators_advanced_user(0,109));
    assert(colors[15][0]==0 && colors[15][1]==100 && colors[108][2]==0);
    event(MOUSE_TURBO_CLICK,true,250); event(MOUSE_TURBO_CLICK,false,1);
    now+=350; housekeeping_task_user(); assert(!rgb && !turbo_locked);
    dial(TURBO_RIGHT); suspended=true; housekeeping_task_user();
    assert(!dial_preview && !flash_active && !rgb);
    puts("Feedback/dial tests passed: actual module, both buttons, neutral, stop-before-change, RGB restoration.");
}
