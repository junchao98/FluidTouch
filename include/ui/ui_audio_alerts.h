#ifndef UI_AUDIO_ALERTS_H
#define UI_AUDIO_ALERTS_H

#include <cstdint>

// Machine-state audio alerts on the Tab5 speaker (M5Unified Speaker_Class).
//  - ALARM: repeating two-tone warning beep until the state clears
//  - HOLD (pause): single confirmation beep, once per entry
// Non-Tab5 targets compile to no-ops (no speaker hardware).
class UIAudioAlerts {
public:
    // Call once from setup (after M5.begin).
    static void init();

    // Call from the main loop; drives the repeating ALARM pattern.
    static void update();

    // Call on every machine-state change (e.g. from updateState hooks).
    // state strings: "ALARM", "HOLD", "IDLE", "RUN", ...
    static void onMachineState(const char *state);

private:
    static bool alarm_active;
    static uint32_t last_beep_ms;
    static bool hold_played;
};

#endif // UI_AUDIO_ALERTS_H
