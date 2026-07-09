#pragma once

#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "SofaLoader.h"

// ==============================================================================
// CrossfeedProcessor — headphone crossfeed simulation for virtual speaker
// monitoring.  Replicates the acoustic crosstalk that occurs when listening
// to real speakers: each ear hears both the ipsilateral (same-side) and
// contralateral (opposite-side) speaker, with frequency-dependent attenuation
// and interaural time difference (ITD).
//
// Algorithm modes (selectable):
//   • Bauer   — 2nd-order LP crossfeed + interaural delay (natural imaging)
//   • Meier   — frequency-selective separation: bass mono, treble stereo
//   • Chu Moy — dual-filter cascade + explicit ITD ring-buffer delay
//   • HRTF    — full binaural convolution via SOFA HRIRs (FFT-convolution)
//
// Parameters:
//   amount     — crossfeed intensity (0.0 = bypass, 1.0 = full)
//   cutoffHz   — low-pass cutoff for crossfeed path (100–2000 Hz)
//   algorithm   — 0 = Bauer, 1 = Meier, 2 = Chu Moy, 3 = HRTF
// ==============================================================================
class CrossfeedProcessor
{
public:
    CrossfeedProcessor() = default;
    ~CrossfeedProcessor() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer,
                 const juce::AudioBuffer<float>& dryBuffer,
                 float amount, float cutoffHz, int algorithm);
    void setSofaLoader(const SofaLoader* sofa) { sofaLoader = sofa; }
    void setHeadOrientation(double azimuthDeg, double elevationDeg)
    {
        headAzimuth = azimuthDeg;
        headElevation = elevationDeg;
    }
    void reset();

private:
    void processBauer  (float* L, float* R, int numSamples, float amount, float cutoffHz);
    void processMeier  (float* L, float* R, int numSamples, float amount, float cutoffHz);
    void processChuMoy (float* L, float* R, int numSamples, float amount, float cutoffHz);
    void processHRTF   (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>& dry,
                        int numSamples, float amount);

    void updateLowPass(float cutoffHz);
    void updateAmount(float amount);

    // --- config ---
    double   sampleRate   = 44100.0;
    int      maxBlockSize = 512;

    // --- 2nd-order Butterworth LP for crossfeed path ---
    // biquad coefficients + per-channel state
    struct LP2
    {
        float b0 = 0, b1 = 0, b2 = 0;
        float a1 = 0, a2 = 0;
        float z1L = 0, z2L = 0;
        float z1R = 0, z2R = 0;
    };
    LP2 lp;

    // --- Interaural delay ring buffer (shared by Bauer / Chu Moy) ---
    // ITD ≈ 0.3 ms at 30° speaker angle → ~13 samples @ 44.1k
    static constexpr int maxDelaySamples = 64;
    float delayRing[maxDelaySamples] = {};
    int   delayWrite = 0;
    int   delayLen   = 13;            // recalculated in prepare()

    // --- Smoothed parameters ---
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmount;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCutoff;
    float currentCutoff = 700.0f;
    float currentAmount = 0.0f;

    // --- HRTF state ---
    const SofaLoader* sofaLoader = nullptr;
    double headAzimuth   = 0.0;
    double headElevation = 0.0;
    bool   hrtfReady = false;

    // FFT convolution — partitioned overlap-save, much faster than time-domain FIR
    juce::dsp::Convolution hrirLL, hrirLR, hrirRL, hrirRR;
    juce::dsp::ProcessSpec convSpec;

    bool loadHRIRs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrossfeedProcessor)
};
