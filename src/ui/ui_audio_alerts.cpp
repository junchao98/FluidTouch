#include "ui/ui_audio_alerts.h"
#include "config.h"
#include <Arduino.h>
#ifdef HARDWARE_TAB5
#include <M5Unified.h>  // M5.Speaker
#endif

bool UIAudioAlerts::alarm_active = false;
uint32_t UIAudioAlerts::last_beep_ms = 0;
bool UIAudioAlerts::hold_played = false;

// Alert patterns as tone-burst sequences played on a single speaker channel.
// M5.Speaker's virtual channels mix concurrently (no ordering), so sequences
// are stepped from update() by watching isPlaying():
//  - ALARM loop: high-high-low siren pair ("warning, warning"), repeating
//    while the machine stays in ALARM.
//  - HOLD: one falling 3-step cue ("paused"), once per HOLD entry.
static constexpr int ALERT_CH = 0;

// step timing: [freq Hz, duration ms]
struct ToneStep { uint16_t freq; uint16_t dur; };

static const ToneStep ALARM_SEQ[] = {
    {1200, 180}, {1200, 180}, {700, 240}, {0, 420},  // gap before repeat
};
static const int ALARM_SEQ_LEN = sizeof(ALARM_SEQ) / sizeof(ALARM_SEQ[0]);

static const ToneStep HOLD_SEQ[] = {
    {880, 130}, {660, 130}, {440, 170},
};
static const int HOLD_SEQ_LEN = sizeof(HOLD_SEQ) / sizeof(HOLD_SEQ[0]);

// Sequencer state (single pending sequence at a time)
static const ToneStep *seq = nullptr;
static int seq_len = 0;
static int seq_idx = 0;

static void seq_start(const ToneStep *steps, int len) {
    seq = steps;
    seq_len = len;
    seq_idx = 0;
}

static void seq_tick() {
    if (!seq) return;
#ifdef HARDWARE_TAB5
    if (M5.Speaker.isPlaying(ALERT_CH)) return;  // current tone still sounding

    const ToneStep &t = seq[seq_idx];
    if (t.freq) {
        M5.Speaker.tone(t.freq, t.dur, ALERT_CH, true);
    }
    // A {0, gap} step just waits one isPlaying==false poll (loop period),
    // which approximates the pause before restarting the alarm siren.
    if (++seq_idx >= seq_len) {
        seq_idx = 0;
        if (seq != ALARM_SEQ) seq = nullptr;  // one-shot finished; alarm loops
    }
#endif
}

void UIAudioAlerts::init() {
#ifdef HARDWARE_TAB5
    // Speaker output enabled via M5.begin cfg.internal_spk (main.cpp).
    M5.Speaker.setVolume(128);  // moderate: audible on a shop floor, not startling
    Serial.println("[AudioAlerts] initialized (Tab5 speaker)");
#endif
}

void UIAudioAlerts::onMachineState(const char *state) {
#ifdef HARDWARE_TAB5
    if (strcmp(state, "ALARM") == 0) {
        if (!alarm_active) {
            alarm_active = true;
            seq_start(ALARM_SEQ, ALARM_SEQ_LEN);
        }
    } else {
        alarm_active = false;

        if (strcmp(state, "HOLD") == 0) {
            if (!hold_played) {
                hold_played = true;
                seq_start(HOLD_SEQ, HOLD_SEQ_LEN);
                Serial.println("[AudioAlerts] pause cue");
            }
        } else {
            hold_played = false;  // re-arm after leaving HOLD
        }
    }
#endif
}

void UIAudioAlerts::update() {
#ifdef HARDWARE_TAB5
    if (seq && !alarm_active && seq == ALARM_SEQ) seq = nullptr;  // alarm cleared mid-loop
    seq_tick();
#else
    (void)last_beep_ms;
#endif
}
