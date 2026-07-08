#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

// ==============================================================================
// RoomProcessor — algorithmic room simulation via synthetic IR convolution.
//
// • Early reflections: discrete delayed + filtered taps (6–12 taps)
// • Late reverberation: exponentially decaying filtered noise tail
// • Three room presets: Small (~0.4s), Medium (~0.9s), Large (~1.8s)
// • Stereo decorrelation via independent L/R IR generation
//
// Convolution uses juce::dsp::Convolution (partitioned overlap-save).
// ==============================================================================
class RoomProcessor
{
public:
    enum RoomType : int
    {
        Small  = 0,
        Medium = 1,
        Large  = 2,
        Count  = 3
    };

    RoomProcessor() = default;
    ~RoomProcessor() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer, float roomMix, int roomType);
    void reset();

private:
    // --------------------------------------------------------------------------
    // Generate a synthetic stereo room IR for the given room type.
    // IR length = sampleRate * RT60 (seconds), stored as stereo.
    // --------------------------------------------------------------------------
    void generateSyntheticIR(int roomType);

    // --------------------------------------------------------------------------
    // Early reflection tap: delay in ms, relative gain, LP cutoff Hz
    // --------------------------------------------------------------------------
    struct ERTap
    {
        float delayMs;
        float gain;
        float cutoffHz; // post-tap low-pass cutoff
    };

    // Generate mono early reflection IR into a buffer at sampleRate
    static void bakeER(const std::vector<ERTap>& taps,
                       float* dest, int irLength, double sr);

    // Generate exponentially decaying filtered noise tail
    static void bakeTail(float* dest, int startSample, int irLength,
                         double sr, float rt60, float hfDamping);

    // --------------------------------------------------------------------------
    // state
    // --------------------------------------------------------------------------
    double sampleRate   = 44100.0;
    int    maxBlockSize = 512;
    int    currentRoom  = -1; // force regenerate on first use

    juce::dsp::Convolution convL;
    juce::dsp::Convolution convR;

    // Pre-allocated mono scratch buffers for convolution
    juce::AudioBuffer<float> wetBufferL;
    juce::AudioBuffer<float> wetBufferR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoomProcessor)
};
