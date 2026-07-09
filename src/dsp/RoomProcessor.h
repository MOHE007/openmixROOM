#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

// ==============================================================================
// RoomProcessor — algorithmic room simulation via Image Source Method (ISM).
//
// Replaces hand-tuned early reflection tap tables with physically grounded
// image-source ray tracing. Computes up to 2nd-order reflections across six
// surfaces (floor/ceiling + four walls) with frequency-dependent absorption.
//
// • ISM generates reflection delay/amplitude/direction from room geometry
// • Late reverberation: frequency-dependent decaying filtered noise (FDN tail)
// • Three presets: Small (3×4×2.5m), Medium (5×7×3m), Large (8×12×4m)
// • Channel decorrelation: slightly offset virtual receiver positions per ear
// • Convolution via juce::dsp::Convolution (partitioned overlap-save)
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
    // Room geometry definition (meters)
    // --------------------------------------------------------------------------
    struct RoomGeometry
    {
        float width, depth, height;              // in meters
        float wallAbsorption[6];                  // octave-band avg for 6 surfaces (0..1)
        float listenerX, listenerY, listenerZ;    // listener position
        float speakerLx, speakerRx;               // virtual speaker X positions (±30°)

        // Absorption coefficient at a specific frequency via 3-band interpolation
        // (low < 500Hz, mid 500–4kHz, high > 4kHz)
        float absorptionAt(float freqHz) const;
    };

    // --------------------------------------------------------------------------
    // Image source result: delay, gain, and arrival direction for stereo panning
    // --------------------------------------------------------------------------
    struct ImageSource
    {
        float delaySec;     // arrival delay in seconds
        float amplitude;    // amplitude after wall reflections
        float azimuth;      // horizontal arrival angle (radians, 0=front)
        int   order;        // reflection order
    };

    // --------------------------------------------------------------------------
    // Generate synthetic stereo room IR using Image Source Method.
    // --------------------------------------------------------------------------
    void generateISM_IR(int roomType);

    // Compute all image sources for a room geometry (up to maxOrder)
    static void traceImageSources(const RoomGeometry& room,
                                   std::vector<ImageSource>& outSources,
                                   int maxOrder);

    // Bake image sources into an IR buffer (per-ear, uses azimuth for ILD/ITD)
    static void bakeImageSources(const std::vector<ImageSource>& sources,
                                  float earOffset, float* dest, int irLength,
                                  double sr);

    // Generate frequency-dependent filtered noise tail
    static void bakeFDNTail(float* dest, int startSample, int irLength,
                             double sr, float rt60, float hfRatio);

    // --------------------------------------------------------------------------
    // Room presets
    // --------------------------------------------------------------------------
    static RoomGeometry roomPreset(int type);

    // --------------------------------------------------------------------------
    // state
    // --------------------------------------------------------------------------
    double sampleRate   = 44100.0;
    int    maxBlockSize = 512;
    int    currentRoom  = -1;

    juce::dsp::Convolution convL;
    juce::dsp::Convolution convR;

    juce::AudioBuffer<float> wetBufferL;
    juce::AudioBuffer<float> wetBufferR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoomProcessor)
};
