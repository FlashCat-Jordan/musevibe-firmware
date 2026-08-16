#include "karplus_strong.h"

#include <esp_log.h>
#include <esp_system.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define TAG "KarplusStrong"

// 6-string chord table — frequencies in Hz.
// Tuned around the guitar's middle range so the delay line fits comfortably
// within kMaxDelay at 16 kHz sample rate (N = 16000 / 220 ≈ 73 samples).
static const KarplusStrongSynth::ChordDef kChords[] = {
    {"C",  {261.63f, 329.63f, 392.00f, 523.25f}, 4},   // C major
    {"Dm", {293.66f, 349.23f, 440.00f, 0.0f},    3},   // D minor
    {"Em", {329.63f, 392.00f, 493.88f, 0.0f},    3},   // E minor
    {"F",  {174.61f, 261.63f, 349.23f, 440.00f}, 4},   // F major
    {"G",  {196.00f, 246.94f, 293.66f, 392.00f}, 4},   // G major
    {"Am", {220.00f, 261.63f, 329.63f, 440.00f}, 4},   // A minor
};

bool KarplusString::Init(float frequency, int sample_rate) {
    if (frequency <= 0.1f) {
        delay_length_ = 0;
        return false;
    }
    delay_length_ = int(sample_rate / frequency + 0.5f);
    if (delay_length_ < 2) delay_length_ = 2;
    if (delay_length_ > kMaxDelay) {
        delay_length_ = 0;
        return false;
    }
    std::memset(buffer_, 0, delay_length_ * sizeof(int16_t));
    index_ = 0;
    return true;
}

void KarplusString::Pluck(int velocity) {
    if (delay_length_ == 0) return;
    // Seed the delay line with white noise of amplitude ±velocity.
    for (int i = 0; i < delay_length_; i++) {
        buffer_[i] = int16_t((esp_random() % (2 * velocity + 1)) - velocity);
    }
    index_ = 0;
}

KarplusStrongSynth::KarplusStrongSynth(int sample_rate)
    : sample_rate_(sample_rate > 0 ? sample_rate : kDefaultSampleRate) {}

const KarplusStrongSynth::ChordDef* KarplusStrongSynth::FindChord(const char* name) {
    if (!name) return nullptr;
    // Case-insensitive compare (chord names are 1-2 ASCII chars).
    char n[3] = {0, 0, 0};
    int i = 0;
    for (; name[i] && i < 2; i++) {
        char c = name[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        n[i] = c;
    }
    for (const auto& chord : kChords) {
        if (strcmp(n, chord.name) == 0) return &chord;
    }
    return nullptr;
}

void KarplusStrongSynth::PlayChordByName(const char* name, int duration_ms) {
    const ChordDef* chord = FindChord(name);
    if (!chord) {
        ESP_LOGW(TAG, "Unknown chord: %s", name ? name : "(null)");
        return;
    }
    active_strings_ = chord->count > kMaxStrings ? kMaxStrings : chord->count;
    for (int i = 0; i < active_strings_; i++) {
        strings_[i].Init(chord->freqs[i], sample_rate_);
        strings_[i].Pluck(velocity_);
    }
    samples_remaining_ = (duration_ms * sample_rate_) / 1000;
    ESP_LOGI(TAG, "Play chord %s (%d strings, %d ms)", name, active_strings_, duration_ms);
}

void KarplusStrongSynth::PlayNote(float frequency, int duration_ms) {
    active_strings_ = 1;
    strings_[0].Init(frequency, sample_rate_);
    strings_[0].Pluck(velocity_);
    samples_remaining_ = (duration_ms * sample_rate_) / 1000;
}

void KarplusStrongSynth::Stop() {
    samples_remaining_ = 0;
    for (int i = 0; i < active_strings_; i++) {
        strings_[i].Mute();
    }
    active_strings_ = 0;
}

int KarplusStrongSynth::Generate(int16_t* out, int num_samples) {
    if (active_strings_ == 0 || samples_remaining_ <= 0) {
        if (samples_remaining_ > 0) samples_remaining_ = 0;
        return 0;
    }
    int n = num_samples;
    if (n > samples_remaining_) n = samples_remaining_;
    for (int i = 0; i < n; i++) {
        int32_t mix = 0;
        for (int s = 0; s < active_strings_; s++) {
            mix += strings_[s].Process();
        }
        // Average across active strings to avoid clipping when many are summed.
        int16_t sample = int16_t(mix / (active_strings_ > 0 ? active_strings_ : 1));
        out[i] = sample;
    }
    samples_remaining_ -= n;
    if (samples_remaining_ <= 0) {
        active_strings_ = 0;
    }
    return n;
}
