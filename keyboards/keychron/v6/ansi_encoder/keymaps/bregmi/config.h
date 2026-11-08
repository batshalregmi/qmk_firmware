#pragma once

#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_TYPING_HEATMAP

// Getreuer's module uses this expression on each deferred callback.
#ifndef __ASSEMBLER__
unsigned short bregmi_turbo_period(void);
unsigned short bregmi_turbo_key(void);
#endif
#define MOUSE_TURBO_CLICK_PERIOD bregmi_turbo_period()
#define MOUSE_TURBO_CLICK_KEY bregmi_turbo_key()

// Default to NKRO when EEPROM settings are initialized; Fn+N still toggles it.
#define NKRO_DEFAULT_ON true

// Mac and Windows base/Fn pairs.
#define DYNAMIC_KEYMAP_LAYER_COUNT 4
