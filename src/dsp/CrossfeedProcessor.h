#pragma once

#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SofaLoader.h"

// ==============================================================================
// CrossfeedProcessor — headphone crossfeed simulation for virtual speaker monitoring.
//
// Algorithm modes (selectable):
//   • Bauer   — simple low-pass + attenuated cross-mix (fastest)
//   • Meier   — low-pass with frequency-dependent crossfeed (natural roll-off)
//   • Chu Moy — additional phase manipulation for improved imaging
//   • HRTF    — full HRTF-based binaural rendering via libmysofa
//
// Parameters:
//   amount     — crossfeed intensity (0.0 = none, 1.0 = full)
//   cutoffHz   — low-pass cutoff for crossfeed path (100–2000 Hz)
//   algorithm   — 0 = Bauer, 1 = Meier, 2 = Chu Moy, 3 = HRTF
// ==============================================================================
class CrossfeedProcessor
{
public:
    CrossfeedProcessor() = default;

    // --------------------------------------------------------------------------
    // Initialise at given sample rate and max block size.
    // --------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize);

    // --------------------------------------------------------------------------
    // Process a stereo buffer in place.
    // dryBuffer is the unprocessed buffer (used for dry/wet mixing).
    // --------------------------------------------------------------------------
    void process(juce::AudioBuffer<float>& buffer,
                 const juce::AudioBuffer<float>& dryBuffer,
                 float amount, float cutoffHz, int algorithm);

    // --------------------------------------------------------------------------
    // Set the SOFA loader for HRTF mode. Pass nullptr to disable HRTF.
    // --------------------------------------------------------------------------
    void setSofaLoader(const SofaLoader* sofa) { sofaLoader = sofa; }

    // --------------------------------------------------------------------------
    // Update listener head orientation (for HRTF mode).
    // azimuthDeg: 0 = front, +90 = right, –90 = left
    // elevationDeg: 0 = horizon, + = up
    // --------------------------------------------------------------------------
    void setHeadOrientation(double azimuthDeg, double elevationDeg)
    {
        headAzimuth = azimuthDeg;
        headElevation = elevationDeg;
    }

    void reset();

private:
    // Bauer / Meier / Chu Moy crossfeed (no HRTF)
    void processBauer(float* L, float* R, int numSamples, float amount, float cutoffHz);
    void processMeier(float* L, float* R, int numSamples, float amount, float cutoffHz);
    void processChuMoy(float* L, float* R, int numSamples, float amount, float cutoffHz);

    // HRTF-based rendering
    void processHRTF(float* L, float* R, int numSamples, float amount);

    // Smooth parameter update
    void updateLowPass(float cutoffHz);
    void updateAmount(float amount);

    // State
    double   sampleRate = 44100.0;
    int      maxBlockSize = 512;

    // 1st-order Butterworth LP filter state (per-channel crossfeed path)
    struct LPState { float b0 = 0.0f, b1 = 0.0f, a1 = 0.0f, zL = 0.0f, zR = 0.0f; };
    LPState lp;

    // Smoothed parameters
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmount;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCutoff;
    float currentCutoff = 700.0f;
    float currentAmount = 0.0f;

    // --------------------------------------------------------------------------
    // HRTF: pre-load HRIRs at ±30° for Phase 2 virtual speaker rendering.
    // Returns true if all 4 HRIRs were loaded successfully.
    // --------------------------------------------------------------------------
    bool loadHRIRs();

    // --------------------------------------------------------------------------
    // Simple time-domain FIR convolver for HRTF rendering.
    // --------------------------------------------------------------------------
    struct FIR
    {
        std::vector<float> coeffs;
        std::vector<float> delayLine;
        int writePos = 0;

        void reset()
        {
            std::fill(delayLine.begin(), delayLine.end(), 0.0f);
            writePos = 0;
        }

        float process(float input)
        {
            delayLine[writePos] = input;
            float sum = 0.0f;
            int pos = writePos;
            for (size_t i = 0; i < coeffs.size(); ++i)
            {
                sum += coeffs[i] * delayLine[pos];
                if (--pos < 0) pos = static_cast<int>(delayLine.size()) - 1;
            }
            if (++writePos >= static_cast<int>(delayLine.size())) writePos = 0;
            return sum;
        }
    };

    // HRTF state
    const SofaLoader* sofaLoader = nullptr;
    double headAzimuth = 0.0;
    double headElevation = 0.0;

    bool hrtfReady = false;
    FIR hrirLL, hrirLR, hrirRL, hrirRR;  // L-ear/R-ear × L-src/R-src

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrossfeedProcessor)
};
