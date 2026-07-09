#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

// ==============================================================================
// RoomProcessor — FDN (Feedback Delay Network) plate reverb.
//
// High-quality algorithmic plate reverb inspired by Valhalla Plate / classic
// EMT 250 / Lexicon 224:
//   • 8-delay FDN with Hadamard mixing matrix
//   • 4-stage allpass diffuser on input for dense early reflections
//   • 2 nested allpass per FDN channel for density
//   • Low/high shelf damping in feedback path
//   • Pre-delay (0–50ms)
//   • Three presets: Small / Medium / Large
//
// Replaces the ISM convolution approach with a smoother, denser plate sound
// that is immediately audible and musical at any mix level.
// ==============================================================================
class RoomProcessor
{
public:
    enum RoomType : int
    {
        Small       = 0,
        Medium      = 1,
        Large       = 2,
        ExtraLarge  = 3,
        Count       = 4
    };

    RoomProcessor() = default;
    ~RoomProcessor() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer, float roomMix, bool enabled);
    void reset();

    // Preset loader — sets all target params from RoomType
    void loadPreset(int roomType);

    // Individual param overrides (call from APVTS / DAW automation)
    void setRoomSize(float s);
    void setPreDelay(float ms);
    void setDamping(float hz);
    void setERLevel(float level);

    float getCurrentRt60()   const noexcept { return currentRt60; }
    float getCurrentDampLp() const noexcept { return currentDampLp; }
    float getCurrentSize()   const noexcept { return currentSize; }
    float getCurrentERLevel() const noexcept { return currentERLevel; }

    // Preset lookup — public for APVTS → DSP sync
    struct Preset
    {
        float rt60;          // decay time (seconds)
        float size;          // size multiplier (0.5–2.0)
        float preDelayMs;    // pre-delay in ms
        float lpfHz;         // low-pass damping cutoff (higher = brighter)
        float hpfHz;         // high-pass damping cutoff
        float diffusion;     // input allpass diffusion amount (0–1)
        float erLevel;       // early reflections level (0–1)
    };
    static Preset presetFor(int type);

private:
    // --------------------------------------------------------------------------
    // Algorithm
    // --------------------------------------------------------------------------
    void applyPreDelayLength();

    // --------------------------------------------------------------------------
    // State
    // --------------------------------------------------------------------------
    double sampleRate   = 44100.0;
    int    maxBlockSize = 512;

    // FDN: 8 delay lines with prime-numbered lengths
    static constexpr int fdnCount = 8;
    static constexpr int maxDelay = 16384;  // ~371ms @ 44.1k

    // ER tap network: 8 discrete taps for early reflections
    static constexpr int erTapCount   = 8;
    static constexpr int erBufferSize = 4096;  // ~93ms @ 44.1k
    float erBuffer[erBufferSize] = {};
    int   erWritePos = 0;
    int   erDelaySamps[erTapCount] = {};

    // Buffer per channel (L/R use independent FDNs for stereo decorrelation).
    // Each of the 8 taps has its own independent delay line to avoid
    // cross-tap buffer corruption caused by prime-length offsets colliding.
    struct FDNChannel
    {
        float delayLine[fdnCount][maxDelay] = {};
        int   delayLen[fdnCount];
        int   writePos[fdnCount];

        // Nested allpass inside each FDN tap (2 stages)
        float apMem1[fdnCount] = {};
        float apMem2[fdnCount] = {};

        // Damping filter state
        float lpMem[fdnCount] = {};
        float hpMem[fdnCount] = {};
    };

    FDNChannel fdnL, fdnR;

    // Randomized Hadamard feedback matrix — row/col sign flips break
    // the all-ones correlation that causes channel-0 amplification
    float randHadamard[fdnCount][fdnCount] = {};

    // Input allpass diffusers (4 stages, shared by L/R or per channel)
    static constexpr int apCount = 4;
    float apMemL[apCount] = {};
    float apMemR[apCount] = {};

    // Pre-delay
    static constexpr int maxPreDelay = 2205;  // 50ms @ 44.1k
    float preDelayL[maxPreDelay] = {};
    float preDelayR[maxPreDelay] = {};
    int   preDelayLen = 0;
    int   preDelayPos = 0;

    // Smoothed parameters
    float targetRt60   = 1.5f;
    float targetSize   = 1.0f;
    float targetDampLp = 8000.0f;
    float targetDampHp = 200.0f;
    float targetERLevel = 0.5f;
    int   targetPreMs  = 0;

    // Current smoothed values
    float currentRt60   = 1.5f;
    float currentSize   = 1.0f;
    float currentDampLp = 8000.0f;
    float currentDampHp = 200.0f;
    float currentERLevel = 0.5f;
    int   currentPreMs  = 0;

    void recalcDelays();
    void recalcERDelays();
    void smoothParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoomProcessor)
};
