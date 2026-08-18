#ifndef UI_FONT_SCALE_H
#define UI_FONT_SCALE_H

// Font scaling for M5Stack Tab5 (1280x720, 5" panel)
//
// IMPORTANT: do NOT remap `lv_font_montserrat_NN` onto another
// `lv_font_montserrat_MM` name via #define - the preprocessor chains
// expansions (14 -> 18 -> 24 -> 32 ...) and the final size balloons far
// beyond what was intended. Instead, usage sites reference the distinct
// ui_font_NN macros below (see the sed note in docs/development.md).

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scaled custom fonts (regenerated with lv_font_conv, see scripts/)
LV_FONT_DECLARE(jetbrains_mono_20);
LV_FONT_DECLARE(fontawesome_icons_24);

#ifdef __cplusplus
}
#endif

// Custom font aliases (single-step renames - target names are not macros,
// so no chaining is possible)
#define jetbrains_mono_16    jetbrains_mono_20
#define fontawesome_icons_20 fontawesome_icons_24

// Montserrat remaps under distinct macro names (no overlap with any
// lv_font_montserrat_* macro, so expansion is exactly one step)
#define ui_font_12 (&lv_font_montserrat_16)
#define ui_font_14 (&lv_font_montserrat_18)
#define ui_font_16 (&lv_font_montserrat_22)
#define ui_font_18 (&lv_font_montserrat_24)
#define ui_font_20 (&lv_font_montserrat_26)
#define ui_font_22 (&lv_font_montserrat_28)
#define ui_font_24 (&lv_font_montserrat_32)
#define ui_font_26 (&lv_font_montserrat_34)
#define ui_font_32 (&lv_font_montserrat_44)

#endif // UI_FONT_SCALE_H
