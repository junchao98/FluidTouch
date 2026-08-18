#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Version
#define FLUIDTOUCH_VERSION "1.0.5"

#ifdef HARDWARE_TAB5
// M5Stack Tab5 - 5" 1280x720 MIPI-DSI display
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720
#else
// Elecrow CrowPanel 7" - 800x480 RGB display
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480
#endif

// Hardware-specific pin configurations
#ifdef HARDWARE_TAB5
// M5Stack Tab5 - display/touch/SD managed by M5GFX board config
// Touch: GT911 (0x14) on early units / ST7123 (0x55) on later units, I2C G31/G32
#define TOUCH_SDA  31
#define TOUCH_SCL  32
#define TOUCH_RST  -1  // Reset via PI4IO I2C GPIO expander, handled by M5GFX
#define TOUCH_INT  23
#elif defined(HARDWARE_ADVANCE)
// CrowPanel 7" Advance - per Elecrow example code
// https://www.elecrow.com/pub/wiki/ESP32_Display-7.0_inch%28Advance_Series%29wiki.html
#define TOUCH_SDA  15
#define TOUCH_SCL  16
#define TOUCH_RST  -1  // Reset handled by STC8H1K28 microcontroller via I2C
#define TOUCH_INT  -1  // Not used
#else
// CrowPanel 7" Basic
#define TOUCH_SDA  19
#define TOUCH_SCL  20
#define TOUCH_RST  38
#define TOUCH_INT  -1
#endif

// Touch controller I2C address
#define GT911_ADDR 0x5D

// GT911 register addresses
#define GT911_POINT_INFO  0x814E
#define GT911_POINT_1     0x814F
#define GT911_CONFIG_REG  0x8047
#define GT911_PRODUCT_ID  0x8140

// UI Layout constants (scaled for 720p on Tab5)
#ifdef HARDWARE_TAB5
#define STATUS_BAR_HEIGHT 96
#define TAB_BUTTON_HEIGHT 96
#else
#define STATUS_BAR_HEIGHT 60
#define TAB_BUTTON_HEIGHT 60
#endif

// Pixel scaling helpers for 720p migration (identity on 800x480 targets)
#ifdef HARDWARE_TAB5
#define UI_SCALE_X(v) ((((uint32_t)(v)) * SCREEN_WIDTH) / 800)
#define UI_SCALE_Y(v) ((((uint32_t)(v)) * SCREEN_HEIGHT) / 480)
#else
#define UI_SCALE_X(v) (v)
#define UI_SCALE_Y(v) (v)
#endif

// Keyboard button font (S3: matches LVGL default; Tab5: scaled via ui_font_20)
#ifdef HARDWARE_TAB5
#define UI_KBD_FONT ui_font_20
#else
#define UI_KBD_FONT (&lv_font_montserrat_14)
#endif

// Display buffer configuration
// Full screen buffer for smooth rendering (PSRAM available on all targets)
#define BUFFER_LINES SCREEN_HEIGHT

// Timing constants
#define SPLASH_DURATION_MS 2500
#define LIMIT_SWITCH_HOLD_MS 500  // Duration to keep limit switch indicators visible after trigger clears

// Preferences namespaces
#define PREFS_NAMESPACE "fluidtouch"        // Machine configurations
#define PREFS_SYSTEM_NAMESPACE "ft_system"  // System flags (clean_shutdown, etc.)

// Screenshot server configuration
#define ENABLE_SCREENSHOT_SERVER true

// SD Card Configuration
#ifdef HARDWARE_TAB5
// Tab5: microSD in SPI mode (G39-G44, per M5Stack pin map)
#define SD_MOSI  42
#define SD_MISO  39
#define SD_CLK   41
#define SD_CS    40
#elif defined(HARDWARE_ADVANCE)
// Advance: SPI mode SD card
#define SD_MOSI  6
#define SD_MISO  4
#define SD_CLK   5
#define SD_CS    0  // Not actually connected - CS tied to GND in hardware (per Elecrow example)
#else
// Basic: SPI mode SD card
#define SD_MOSI  11
#define SD_MISO  13
#define SD_CLK   12
#define SD_CS    10
#endif

// Upload Configuration
#define FLUIDNC_UPLOAD_PATH "/fluidtouch/uploads/"  // Automatically created if missing

// Font scaling for 720p (Tab5): remap to larger built-in font sizes.
// S3 builds get identity macros so shared UI code compiles unchanged.
#ifdef HARDWARE_TAB5
#include "ui_font_scale.h"
#else
#define ui_font_12 (&lv_font_montserrat_12)
#define ui_font_14 (&lv_font_montserrat_14)
#define ui_font_16 (&lv_font_montserrat_16)
#define ui_font_18 (&lv_font_montserrat_18)
#define ui_font_20 (&lv_font_montserrat_20)
#define ui_font_22 (&lv_font_montserrat_22)
#define ui_font_24 (&lv_font_montserrat_24)
#define ui_font_26 (&lv_font_montserrat_26)
#define ui_font_32 (&lv_font_montserrat_32)
#endif

#endif // CONFIG_H
