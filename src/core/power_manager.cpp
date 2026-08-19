#include "core/power_manager.h"
#include "network/fluidnc_client.h"
#include "config.h"
#include <Preferences.h>
#include <Arduino.h>
#include <WiFi.h>
#include <esp_sleep.h>
#ifdef HARDWARE_TAB5
#include <M5Unified.h>  // M5.Imu (BMI270) for shake-to-wake
#endif

// Static member initialization
DisplayDriver* PowerManager::display_driver = nullptr;
bool PowerManager::enabled = true;
uint32_t PowerManager::dim_timeout_sec = 30;         // Default: dim after 30 seconds
uint32_t PowerManager::sleep_timeout_sec = 300;      // Default: screen off after 5 minutes
uint32_t PowerManager::deep_sleep_timeout_sec = 900; // Default: deep sleep after 15 minutes
uint8_t PowerManager::normal_brightness = 100;       // Default: 100% brightness when active
uint8_t PowerManager::dim_brightness = 25;           // Default: 25% brightness when dimmed
uint32_t PowerManager::last_activity_ms = 0;
PowerManager::PowerState PowerManager::current_state = PowerManager::FULL_BRIGHTNESS;
bool PowerManager::state_changed = false;
bool PowerManager::shake_wake_enabled = true;

void PowerManager::init(DisplayDriver* driver) {
    display_driver = driver;
    last_activity_ms = millis();
    current_state = FULL_BRIGHTNESS;
    loadSettings();
    
    Serial.println("\n=== Power Manager Initialized ===");
    Serial.printf("Enabled: %s\n", enabled ? "YES" : "NO");
    Serial.printf("Dim timeout: %d seconds\n", dim_timeout_sec);
    Serial.printf("Sleep timeout: %d seconds\n", sleep_timeout_sec);
    Serial.printf("Deep sleep timeout: %d seconds\n", deep_sleep_timeout_sec);
    Serial.printf("Normal brightness: %d%%\n", normal_brightness);
    Serial.printf("Dim brightness: %d%%\n", dim_brightness);
    
    // Apply the loaded brightness immediately
    if (display_driver) {
        display_driver->setBacklight(normal_brightness);
        Serial.printf("Applied initial brightness: %d%%\n", normal_brightness);
    }
}

void PowerManager::onUserActivity() {
    last_activity_ms = millis();
    
    // If we were dimmed or screen off, return to full brightness
    if (current_state != FULL_BRIGHTNESS) {
        enterFullBrightness();
    }
}

void PowerManager::update(int machine_state) {
    // Skip if power management disabled
    if (!enabled || display_driver == nullptr) {
        return;
    }
    
    // Power management ONLY applies to IDLE and DISCONNECTED states
    // All other states (RUN, ALARM, HOLD, JOG, etc.) stay at full brightness
    if (machine_state != STATE_IDLE && machine_state != STATE_DISCONNECTED) {
        if (current_state != FULL_BRIGHTNESS) {
            enterFullBrightness();
        }
        // Reset activity timer when not in IDLE/DISCONNECTED to prevent immediate dim when returning
        last_activity_ms = millis();
        return;
    }
    
    // Calculate time since last activity
    uint32_t idle_ms = millis() - last_activity_ms;
    uint32_t idle_sec = idle_ms / 1000;
    
#ifndef HARDWARE_TAB5
    // Check for deep sleep timeout (if enabled)
    if (deep_sleep_timeout_sec > 0 && idle_sec >= deep_sleep_timeout_sec) {
        enterDeepSleep();
        return;  // Never returns, but good practice
    }
#endif
    // (Tab5: deep sleep not supported by its PMU power architecture -
    //  screen-off light idle is the deepest state; power off via power button)
    
    // State machine for screen power management
    switch (current_state) {
        case FULL_BRIGHTNESS:
            // Check if we should dim (only if dimming is enabled)
            if (dim_timeout_sec > 0 && idle_sec >= dim_timeout_sec) {
                enterDimmed();
            }
            break;
            
        case DIMMED:
            // Check if we should turn off screen (only if sleep is enabled)
            if (sleep_timeout_sec > 0 && idle_sec >= sleep_timeout_sec) {
                enterScreenOff();
            }
            break;
            
        case SCREEN_OFF:
            // Stay off until user activity or deep sleep timeout
            break;
    }
}

void PowerManager::loadSettings() {
    Preferences prefs;
    prefs.begin(PREFS_SYSTEM_NAMESPACE, true);  // Read-only
    
    enabled = prefs.getBool("pm_enabled", true);
    dim_timeout_sec = prefs.getUInt("pm_dim_to", 30);
    sleep_timeout_sec = prefs.getUInt("pm_sleep_to", 300);
    deep_sleep_timeout_sec = prefs.getUInt("pm_deepsleep", 900);
    normal_brightness = prefs.getUChar("pm_norm_bri", 100);  // 0-100 percentage
    dim_brightness = prefs.getUChar("pm_dim_bri", 25);       // 0-100 percentage
    shake_wake_enabled = prefs.getBool("pm_shake_wake", true);
    
    prefs.end();
    
    // Validate ranges (0 = disabled is valid)
    if (dim_timeout_sec > 0 && dim_timeout_sec < 10) dim_timeout_sec = 10;
    if (dim_timeout_sec > 600) dim_timeout_sec = 600;
    if (sleep_timeout_sec > 0 && sleep_timeout_sec < 10) sleep_timeout_sec = 10;
    if (sleep_timeout_sec > 3600) sleep_timeout_sec = 3600;
    // If both dim and sleep are enabled, ensure sleep > dim
    if (dim_timeout_sec > 0 && sleep_timeout_sec > 0 && sleep_timeout_sec < dim_timeout_sec + 10) {
        sleep_timeout_sec = dim_timeout_sec + 10;
    }
    if (deep_sleep_timeout_sec > 0 && deep_sleep_timeout_sec < 300) deep_sleep_timeout_sec = 300;
    if (deep_sleep_timeout_sec > 7200) deep_sleep_timeout_sec = 7200;  // Max 2 hours
    if (normal_brightness > 100) normal_brightness = 100;  // Validate percentage range
    if (dim_brightness > 100) dim_brightness = 25;         // Validate percentage range
}

void PowerManager::saveSettings() {
    Preferences prefs;
    prefs.begin(PREFS_SYSTEM_NAMESPACE, false);  // Read-write
    
    prefs.putBool("pm_enabled", enabled);
    prefs.putUInt("pm_dim_to", dim_timeout_sec);
    prefs.putUInt("pm_sleep_to", sleep_timeout_sec);
    prefs.putUInt("pm_deepsleep", deep_sleep_timeout_sec);
    prefs.putUChar("pm_norm_bri", normal_brightness);
    prefs.putUChar("pm_dim_bri", dim_brightness);
    prefs.putBool("pm_shake_wake", shake_wake_enabled);
    
    prefs.end();
    
    Serial.println("\n=== Power Manager Settings Saved ===");
    Serial.printf("Enabled: %s\n", enabled ? "YES" : "NO");
    Serial.printf("Dim timeout: %d seconds\n", dim_timeout_sec);
    Serial.printf("Sleep timeout: %d seconds\n", sleep_timeout_sec);
    Serial.printf("Deep sleep timeout: %d seconds\n", deep_sleep_timeout_sec);
    Serial.printf("Normal brightness: %d/255\n", normal_brightness);
    Serial.printf("Dim brightness: %d/255\n", dim_brightness);
}

void PowerManager::setEnabled(bool enable) {
    enabled = enable;
    if (!enabled && current_state != FULL_BRIGHTNESS) {
        // If disabling, restore full brightness
        enterFullBrightness();
    }
}

void PowerManager::setDimTimeout(uint32_t seconds) {
    // 0 = disabled, otherwise must be between 10 and 600 seconds
    if (seconds == 0 || (seconds >= 10 && seconds <= 600)) {
        dim_timeout_sec = seconds;
        // Ensure sleep timeout is always greater (if both are enabled)
        if (dim_timeout_sec > 0 && sleep_timeout_sec > 0 && sleep_timeout_sec < dim_timeout_sec + 10) {
            sleep_timeout_sec = dim_timeout_sec + 10;
        }
    }
}

void PowerManager::setSleepTimeout(uint32_t seconds) {
    // 0 = disabled, otherwise must be greater than dim timeout (if dim is enabled)
    if (seconds == 0 || seconds <= 3600) {
        // If dim is enabled and sleep is enabled, ensure sleep > dim
        if (dim_timeout_sec > 0 && seconds > 0 && seconds < dim_timeout_sec + 10) {
            return;  // Invalid: sleep must be at least 10 seconds after dim
        }
        sleep_timeout_sec = seconds;
        // Ensure deep sleep timeout is always greater (if both are enabled)
        if (sleep_timeout_sec > 0 && deep_sleep_timeout_sec > 0 && deep_sleep_timeout_sec < sleep_timeout_sec + 60) {
            deep_sleep_timeout_sec = sleep_timeout_sec + 60;
        }
    }
}

void PowerManager::setDeepSleepTimeout(uint32_t seconds) {
    // 0 = disabled, otherwise must be greater than sleep timeout
    if (seconds == 0 || (seconds >= sleep_timeout_sec + 60 && seconds <= 7200)) {
        deep_sleep_timeout_sec = seconds;
    }
}

void PowerManager::setNormalBrightness(uint8_t brightness) {
    if (brightness <= 100) {  // Validate percentage range
        normal_brightness = brightness;
        // If currently at full brightness, apply new brightness immediately
        if (current_state == FULL_BRIGHTNESS && display_driver) {
            display_driver->setBacklight(normal_brightness);
        }
    }
}

void PowerManager::setDimBrightness(uint8_t brightness) {
    if (brightness <= 100) {  // Validate percentage range
        dim_brightness = brightness;
        // If currently dimmed, apply new brightness immediately
        if (current_state == DIMMED && display_driver) {
            display_driver->setBacklight(dim_brightness);
        }
    }
}

void PowerManager::applyNormalBrightness() {
    if (display_driver) {
        display_driver->setBacklight(normal_brightness);
        // Also update state to full brightness
        current_state = FULL_BRIGHTNESS;
    }
}

void PowerManager::enterFullBrightness() {
    if (current_state != FULL_BRIGHTNESS) {
        Serial.printf("PowerManager: Entering FULL_BRIGHTNESS (brightness=%d)\n", normal_brightness);
        display_driver->setBacklight(normal_brightness);
        current_state = FULL_BRIGHTNESS;
        state_changed = true;
    }
}

void PowerManager::enterDimmed() {
    if (current_state != DIMMED) {
        Serial.printf("PowerManager: Entering DIMMED (brightness=%d)\n", dim_brightness);
        display_driver->setBacklight(dim_brightness);
        current_state = DIMMED;
        state_changed = true;
    }
}

void PowerManager::enterScreenOff() {
    if (current_state != SCREEN_OFF) {
        Serial.println("PowerManager: Entering SCREEN_OFF");
        display_driver->setBacklightOff();
        current_state = SCREEN_OFF;
        state_changed = true;
    }
}

void PowerManager::enterDeepSleep() {
    Serial.println("PowerManager: Entering DEEP SLEEP due to inactivity");
    
    // Save clean shutdown flag
    Preferences prefs;
    prefs.begin(PREFS_SYSTEM_NAMESPACE, false);
    prefs.putBool("clean_shutdown", true);
    prefs.end();
    
    // Power down display (backlight only - see display_driver.cpp for details)
    display_driver->powerDown();
    delay(100);
    
#ifdef HARDWARE_TAB5
    // Tab5: no SoC deep sleep - the PMU/battery architecture requires clean
    // shutdown via the power button. Stay in screen-off light idle instead.
    FluidNCClient::disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    current_state = SCREEN_OFF;
    Serial.println("Tab5: entering screen-off light idle (no deep sleep)");
    return;
#else
    // Shutdown network and radios for power savings
    FluidNCClient::disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    
    // Disable all wakeup sources except reset button
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    
    // Use default RTC power domain settings for maximum compatibility
    // (No esp_sleep_pd_config calls needed - defaults work fine)
    
    // Enter deep sleep (only reset button can wake)
    Serial.println("Entering deep sleep with maximum power savings...");
    Serial.flush();  // Ensure message is sent
    delay(100);
    
    esp_deep_sleep_start();
    // Never returns
#endif
}

#ifdef HARDWARE_TAB5
// Shake-to-wake: poll the BMI270 accelerometer and restore brightness on a
// firm shake while the screen is dimmed or off. Orientation-independent:
// gravity always integrates to ~1g in the total magnitude, so we trigger on
// | ‖a‖ - 1g | spiking above SHAKE_THRESHOLD_G for a few consecutive
// samples. Hand shakes a 5" panel spike well past 2g; stepper/cutting
// vibration stays far below the threshold, so a machine running beside the
// controller does not keep waking the screen.
// Shake-to-wake via the Tab5's BMI270, following the official M5Stack example
// (M5.Imu.update() + getImuData(), docs.m5stack.com/zh_CN/arduino/m5tab5/imu).
// M5Unified's begin() uploads the BMI270 config file but leaves PWR_CTRL=0
// (it only enables sensors when a BMM150 aux magnetometer answers, which the
// Tab5 lacks), so we additionally power the accelerometer on. Do NOT soft-
// reset the chip here - that wipes the config file and blocks all config
// register writes until M5Unified re-uploads it (which it never does).
// Registers (BMI270): ACC_CONF=0x40, PWR_CTRL=0x7D; accel data 0x0B-0x10.
static constexpr uint8_t SHAKE_BMI_ADDR = 0x68;
static constexpr uint32_t SHAKE_I2C_FREQ = 400000;

void PowerManager::pollShakeWake() {
    static bool imu_checked = false;
    static bool imu_available = false;
    if (!imu_checked) {
        imu_checked = true;
        if (M5.Imu.getType() != m5::imu_t::imu_bmi270) {
            Serial.printf("ShakeWake: no BMI270 (type=%d), disabled\n", (int)M5.Imu.getType());
            return;
        }
        // Config file is loaded by M5.Imu.begin(); just enable the accel.
        // ACC_CONF 0x28 = bwp normal, ODR 200Hz. PWR_CTRL 0x04 = ACC on only.
        M5.In_I2C.writeRegister8(SHAKE_BMI_ADDR, 0x40, 0x28, SHAKE_I2C_FREQ);
        M5.In_I2C.writeRegister8(SHAKE_BMI_ADDR, 0x7D, 0x04, SHAKE_I2C_FREQ);
        delay(5);
        Serial.printf("ShakeWake: BMI270 accel on (PWR=0x%02X ACCCONF=0x%02X)\n",
                      M5.In_I2C.readRegister8(SHAKE_BMI_ADDR, 0x7D, SHAKE_I2C_FREQ),
                      M5.In_I2C.readRegister8(SHAKE_BMI_ADDR, 0x40, SHAKE_I2C_FREQ));

        // Boot self-test: log 5 samples to prove the data path before relying on it
        for (int i = 0; i < 5; i++) {
            M5.Imu.update();
            m5::IMU_Class::imu_data_t d = M5.Imu.getImuData();
            Serial.printf("ShakeWake: selftest |a|=%.3f (x=%.2f y=%.2f z=%.2f)\n",
                          sqrtf(d.accel.x * d.accel.x + d.accel.y * d.accel.y + d.accel.z * d.accel.z),
                          d.accel.x, d.accel.y, d.accel.z);
            delay(50);
        }
        imu_available = true;
    }
    if (!imu_available || !shake_wake_enabled) return;

    // Only meaningful from DIMMED / SCREEN_OFF - while the screen is already
    // on, motion must NOT reset the idle timer (vibration would keep it lit).
    if (!enabled || current_state == FULL_BRIGHTNESS) return;

    static uint32_t last_poll_ms = 0;
    uint32_t now = millis();
    if (now - last_poll_ms < 10) return;  // ~100 Hz sample rate
    last_poll_ms = now;

    auto mask = M5.Imu.update();
    if (!(mask & m5::IMU_Class::sensor_mask_accel)) return;

    m5::IMU_Class::imu_data_t d = M5.Imu.getImuData();
    float x = d.accel.x, y = d.accel.y, z = d.accel.z;
    if (x == 0.0f && y == 0.0f && z == 0.0f) return;  // no data yet
    float mag = sqrtf(x * x + y * y + z * z);

    static const float SHAKE_THRESHOLD_G = 0.6f;  // |a| outside 0.4g..1.6g
    static const uint8_t SHAKE_SAMPLES = 4;       // ~40ms sustained spike

    static uint8_t spike_count = 0;
    if (fabsf(mag - 1.0f) > SHAKE_THRESHOLD_G) {
        if (++spike_count >= SHAKE_SAMPLES) {
            spike_count = 0;
            Serial.printf("ShakeWake: wake (|a|=%.2fg)\n", mag);
            onUserActivity();
            return;
        }
    } else {
        spike_count = 0;
    }

    // Debug: dump every 2s while dimmed/off (remove once sensitivity tuned)
    static uint32_t last_dbg_ms = 0;
    if (now - last_dbg_ms > 2000) {
        last_dbg_ms = now;
        Serial.printf("ShakeWake: state=%d |a|=%.3fg spk=%u\n",
                      (int)current_state, mag, spike_count);
    }
}
#else
void PowerManager::pollShakeWake() {
    // No IMU on this target
}
#endif
