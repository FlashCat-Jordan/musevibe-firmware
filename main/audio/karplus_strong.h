#ifndef _KARPLUS_STRONG_H_
#define _KARPLUS_STRONG_H_

#include <cstdint>
#include <cstring>

// Karplus-Strong physical modeling synthesis for plucked-string instruments.
// Each string is a delay line of length N = sample_rate / frequency, seeded
// with a noise burst (the "pluck"), with a one-pole averaging low-pass in the
// feedback loop that simulates string damping. Polyphony is achieved by summing
// multiple strings and dividing by their count.
//
// Reference: Karplus, K., & Strong, A. (1983). "Digital Synthesis of
// Plucked-String and Drum Timbres". Computer Music Journal, 7(2), 43-55.

class KarplusString {
public:
    KarplusString() = default;

    // Configure the string for a given fundamental frequency at the given sample rate.
    // Returns false if the frequency is too low (delay line exceeds kMaxDelay).
    bool Init(float frequency, int sample_rate);

    // Strike the string with a noise burst. `velocity` is amplitude 0..32767.
    void Pluck(int velocity = 28000);

    // Advance the simulation by one sample and return the output sample.
    inline int16_t Process() {
        if (delay_length_ == 0) return 0;
        int16_t y = buffer_[index_];
        int next = (index_ + 1) % delay_length_;
        // One-pole averaging low-pass in the feedback path -> energy decay.
        int32_t averaged = (int32_t(buffer_[index_]) + int32_t(buffer_[next])) >> 1;
        // Apply additional decay to simulate string damping over time.
        averaged = (averaged * decay_) >> 15;
        buffer_[index_] = int16_t(averaged);
        index_ = next;
        return y;
    }

    void Mute() {
        if (delay_length_ > 0) {
            std::memset(buffer_, 0, delay_length_ * sizeof(int16_t));
        }
    }

    bool IsActive() const { return delay_length_ > 0; }

private:
    static constexpr int kMaxDelay = 1024;
    int16_t buffer_[kMaxDelay] = {0};
    int delay_length_ = 0;
    int index_ = 0;
    // Decay factor in Q15 fixed point (32768 = 1.0). 32604 ≈ 0.9965.
    int32_t decay_ = 32604;
};

class KarplusStrongSynth {
public:
    static constexpr int kMaxStrings = 6;
    static constexpr int kDefaultSampleRate = 16000;

    KarplusStrongSynth(int sample_rate = kDefaultSampleRate);

    // Pluck a chord by name. Supported: "C", "Dm", "Em", "F", "G", "Am" (and case-insensitive).
    // `duration_ms` is how long the chord rings.
    void PlayChordByName(const char* name, int duration_ms);

    // Pluck a single note by frequency (Hz).
    void PlayNote(float frequency, int duration_ms);

    // Stop all sound immediately.
    void Stop();

    // Generate `num_samples` of 16-bit PCM into `out`. Returns the number of samples
    // actually written (may be less than requested if synthesis finished).
    int Generate(int16_t* out, int num_samples);

    bool IsPlaying() const { return samples_remaining_ > 0; }
    int sample_rate() const { return sample_rate_; }

private:
    struct ChordDef {
        const char* name;
        float freqs[4];
        int count;
    };

    static const ChordDef* FindChord(const char* name);

    int sample_rate_;
    KarplusString strings_[kMaxStrings];
    int active_strings_ = 0;
    int samples_remaining_ = 0;
    int velocity_ = 28000;
};

#endif  // _KARPLUS_STRONG_H_
