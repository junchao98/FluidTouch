#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include <lvgl.h>
#include "config.h"

#ifdef HARDWARE_TAB5
// Tab5: LGFX is a typedef of M5GFX, include the full definition
#include "core/display_driver.h"
#else
// Forward declaration
class LGFX;
#endif

// Touch driver class
class TouchDriver {
public:
    TouchDriver();
    bool init(LGFX *lcd);
    lv_indev_t* getInputDevice() { return indev; }
    
private:
    lv_indev_t *indev;
    
    static void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);
};

#endif // TOUCH_DRIVER_H
