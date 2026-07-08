#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/HeadphoneCalibration.h"

// ==============================================================================
// FrequencyResponseGraph — draws headphone calibration frequency response
// curve. Renders the combined EQ correction as a filled area on a log-frequency
// axis (20 Hz – 20 kHz), referenced to a flat 0 dB line.
//
// Design: dark graph background, cyan correction curve, subtle grid lines.
// ==============================================================================
class FrequencyResponseGraph : public juce::Component
{
public:
    FrequencyResponseGraph();
    ~FrequencyResponseGraph() override = default;

    void setCalibration(const HeadphoneCalibration* cal);
    void paint(juce::Graphics& g) override;

private:
    const HeadphoneCalibration* calibration = nullptr;

    // Frequency grid points (log spaced 20 Hz – 20 kHz)
    static constexpr int numPoints = 256;
    float freqs[numPoints];
    float magnitudes[numPoints];

    void recalcMagnitudes();

    // Helper to convert frequency to horizontal pixel position (log scale)
    float freqToX(float freqHz, float plotWidth) const;

    static constexpr float minFreq  = 20.0f;    // Hz
    static constexpr float maxFreq  = 20000.0f; // Hz
    static constexpr float minDB    = -12.0f;
    static constexpr float maxDB    = +12.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyResponseGraph)
};
