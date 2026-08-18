#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <lvgl.h>
#ifdef HARDWARE_TAB5
// M5Stack Tab5: MIPI-DSI panel, graphics + touch via M5GFX board auto-detection
// (supports both ILI9881C+GT911 early units and ST7123/ST7121 TDDI units)
#include <M5GFX.h>
typedef M5GFX LGFX;
#else
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/touch/Touch_GT911.hpp>

// LovyanGFX configuration for Elecrow CrowPanel 7"
class LGFX : public lgfx::LGFX_Device
{
public:
  lgfx::Bus_RGB     _bus_instance;
  lgfx::Panel_RGB   _panel_instance;
  lgfx::Touch_GT911 _touch_instance;

  LGFX(void);
};
#endif
#include "config.h"

// Display driver class
class DisplayDriver {
public:
    DisplayDriver();
    bool init();
    lv_display_t* getDisplay() { return disp; }
    
    // Direct screen buffer access for screenshots
    LGFX* getLCD() { return &lcd; }
    
    // Backlight control (brightness 0-100 percentage, or use setBacklightOn/Off for simple on/off)
    void setBacklight(uint8_t brightness_percent);
    void setBacklightOn();
    void setBacklightOff();
    
    // Power management - deep sleep preparation
    void powerDown();
    
    // Display rotation (0 = normal, 2 = 180 degrees)
    void setRotation(uint8_t rotation);
    uint8_t getRotation() const;
    
private:
#ifdef HARDWARE_TAB5
    LGFX &lcd;   // shared M5.Display instance (owned by M5Unified - must be
                 // a single instance or the LEDA backlight PWM double-attaches)
#else
    LGFX lcd;
#endif
    lv_display_t *disp;
    lv_color_t *disp_draw_buf;
    lv_color_t *disp_draw_buf2;
    uint8_t current_rotation;
    
    static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
};

#endif // DISPLAY_DRIVER_H
