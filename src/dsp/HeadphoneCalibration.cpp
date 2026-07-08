#include "HeadphoneCalibration.h"
#include <cmath>

// ==============================================================================
// AutoEq (oratory1990) headphone calibration profiles
// Source: https://github.com/jaakkopasanen/AutoEq
// Target: Harman over-ear 2018
// Generated from AutoEq ParametricEQ.txt files (10-band PEQ per headphone)
// ==============================================================================

const std::vector<HeadphoneCalibration::Profile> HeadphoneCalibration::profiles =
{
    {
        "Beyerdynamic DT 770 Pro",
        "Closed-back studio staple. Bass cut -3dB shelf, +6.8dB at 3.7kHz.",
        -5.1f,
        {
            { FilterBand::LowShelf,  105, -3.0f, 0.70f },
            { FilterBand::Peak, 6430, -4.2f, 0.97f },
            { FilterBand::Peak, 3684, 6.8f, 2.88f },
            { FilterBand::Peak, 213, 3.5f, 2.46f },
            { FilterBand::Peak, 139, -3.9f, 4.92f },
            { FilterBand::HighShelf, 10000, -5.4f, 0.70f },
            { FilterBand::Peak, 93, 3.5f, 3.00f },
            { FilterBand::Peak, 9561, 2.3f, 1.86f },
            { FilterBand::Peak, 45, -1.3f, 1.74f },
            { FilterBand::Peak, 119, -2.1f, 5.83f }
        }
    },
    {
        "Beyerdynamic DT 990 Pro",
        "Open-back V-shaped. +10.1dB bass shelf, -9.8dB high shelf.",
        -6.7f,
        {
            { FilterBand::LowShelf, 105, 10.1f, 0.70f },
            { FilterBand::Peak, 541, 3.1f, 0.52f },
            { FilterBand::Peak, 64, -7.4f, 0.38f },
            { FilterBand::Peak, 7731, -4.6f, 1.06f },
            { FilterBand::Peak, 640, 0.6f, 2.47f },
            { FilterBand::HighShelf, 10000, -9.8f, 0.70f },
            { FilterBand::Peak, 7136, 3.1f, 2.08f },
            { FilterBand::Peak, 5897, -4.8f, 5.30f },
            { FilterBand::Peak, 4193, 1.1f, 1.51f },
            { FilterBand::Peak, 9436, 2.7f, 3.53f }
        }
    },
    {
        "Sennheiser HD 600",
        "Open-back reference. +6.5dB sub shelf, subtle mid sculpting.",
        -6.3f,
        {
            { FilterBand::LowShelf, 105, 6.5f, 0.70f },
            { FilterBand::Peak, 125, -2.7f, 0.55f },
            { FilterBand::Peak, 8445, 3.3f, 1.61f },
            { FilterBand::Peak, 522, 0.7f, 1.02f },
            { FilterBand::Peak, 1298, -1.2f, 2.14f },
            { FilterBand::HighShelf, 10000, -3.1f, 0.70f },
            { FilterBand::Peak, 3158, -1.8f, 3.67f },
            { FilterBand::Peak, 2166, 0.9f, 3.32f },
            { FilterBand::Peak, 6639, 2.2f, 5.82f },
            { FilterBand::Peak, 5433, -1.2f, 5.70f }
        }
    },
    {
        "Sennheiser HD 650",
        "Warm open-back. +6.4dB sub shelf, +5.1dB air at 8.8kHz.",
        -6.1f,
        {
            { FilterBand::LowShelf, 105, 6.4f, 0.70f },
            { FilterBand::Peak, 8800, 5.1f, 1.42f },
            { FilterBand::Peak, 118, -3.1f, 0.50f },
            { FilterBand::Peak, 37, 0.7f, 3.96f },
            { FilterBand::Peak, 3169, -1.7f, 3.89f },
            { FilterBand::HighShelf, 10000, -2.1f, 0.70f },
            { FilterBand::Peak, 1227, -1.2f, 2.53f },
            { FilterBand::Peak, 2055, 1.2f, 3.23f },
            { FilterBand::Peak, 587, 0.4f, 1.19f },
            { FilterBand::Peak, 5332, -1.1f, 5.75f }
        }
    },
    {
        "Audio-Technica ATH-M50x",
        "Closed-back bass-forward. -5.2dB at 156Hz, +5.3dB at 326Hz.",
        -3.1f,
        {
            { FilterBand::LowShelf, 105, 0.6f, 0.70f },
            { FilterBand::Peak, 156, -5.2f, 0.73f },
            { FilterBand::Peak, 326, 5.3f, 1.59f },
            { FilterBand::Peak, 7077, 2.8f, 2.22f },
            { FilterBand::Peak, 3483, 2.1f, 5.82f },
            { FilterBand::HighShelf, 10000, -4.1f, 0.70f },
            { FilterBand::Peak, 45, -1.1f, 1.90f },
            { FilterBand::Peak, 66, 1.4f, 3.59f },
            { FilterBand::Peak, 787, -0.5f, 1.79f },
            { FilterBand::Peak, 1640, 0.9f, 3.41f }
        }
    },
    {
        "AKG K701",
        "Open-back wide soundstage. +7.5dB sub shelf, 2-6kHz sculpt.",
        -6.1f,
        {
            { FilterBand::LowShelf, 105, 7.5f, 0.70f },
            { FilterBand::Peak, 131, -4.1f, 0.21f },
            { FilterBand::Peak, 937, 3.7f, 0.65f },
            { FilterBand::Peak, 2378, -3.3f, 2.56f },
            { FilterBand::Peak, 3536, 3.6f, 3.04f },
            { FilterBand::HighShelf, 10000, -0.6f, 0.70f },
            { FilterBand::Peak, 5837, -2.9f, 5.42f },
            { FilterBand::Peak, 55, -0.9f, 1.90f },
            { FilterBand::Peak, 116, 1.2f, 3.55f },
            { FilterBand::Peak, 27, 0.7f, 3.27f }
        }
    },
    {
        "AKG K702",
        "Open-back flat reference. +7.1dB sub shelf, -4.7dB at 5.5kHz.",
        -6.1f,
        {
            { FilterBand::LowShelf, 105, 7.1f, 0.70f },
            { FilterBand::Peak, 119, -2.7f, 0.23f },
            { FilterBand::Peak, 9459, 3.2f, 3.19f },
            { FilterBand::Peak, 730, 3.2f, 1.22f },
            { FilterBand::Peak, 2241, -3.5f, 3.80f },
            { FilterBand::HighShelf, 10000, -1.7f, 0.70f },
            { FilterBand::Peak, 3721, 3.4f, 2.02f },
            { FilterBand::Peak, 5483, -4.7f, 4.79f },
            { FilterBand::Peak, 2633, -2.0f, 5.29f },
            { FilterBand::Peak, 56, -0.6f, 2.62f }
        }
    },
    {
        "Sony MDR-7506",
        "Closed-back broadcast standard. +10.3dB sub, -9.5dB at 48Hz.",
        -5.8f,
        {
            { FilterBand::LowShelf, 105, 10.3f, 0.70f },
            { FilterBand::Peak, 5435, -3.8f, 0.90f },
            { FilterBand::Peak, 847, 1.9f, 0.70f },
            { FilterBand::Peak, 230, 3.7f, 2.08f },
            { FilterBand::Peak, 48, -9.5f, 0.52f },
            { FilterBand::HighShelf, 10000, 3.8f, 0.70f },
            { FilterBand::Peak, 7530, -1.4f, 3.26f },
            { FilterBand::Peak, 2916, -1.9f, 5.57f },
            { FilterBand::Peak, 3748, 2.6f, 5.91f },
            { FilterBand::Peak, 4456, -1.6f, 6.00f }
        }
    },
    {
        "Shure SRH840",
        "Closed-back neutral-warm. -8.3dB at 107Hz, +6.2dB sub shelf.",
        -5.8f,
        {
            { FilterBand::LowShelf, 105, 6.2f, 0.70f },
            { FilterBand::Peak, 107, -8.3f, 0.80f },
            { FilterBand::Peak, 279, 2.9f, 0.36f },
            { FilterBand::Peak, 58, 2.4f, 4.17f },
            { FilterBand::Peak, 5991, -1.8f, 2.60f },
            { FilterBand::HighShelf, 10000, -2.0f, 0.70f },
            { FilterBand::Peak, 1946, -0.7f, 2.63f },
            { FilterBand::Peak, 39, -0.4f, 3.26f },
            { FilterBand::Peak, 261, 0.6f, 3.14f },
            { FilterBand::Peak, 332, -0.8f, 4.70f }
        }
    },
    {
        "Beyerdynamic DT 880",
        "Semi-open analytical. +7.4dB sub, -6.1dB at 5.8kHz.",
        -6.4f,
        {
            { FilterBand::LowShelf, 105, 7.4f, 0.70f },
            { FilterBand::Peak, 112, -3.4f, 0.30f },
            { FilterBand::Peak, 1320, 2.1f, 0.54f },
            { FilterBand::Peak, 5760, -6.1f, 4.97f },
            { FilterBand::Peak, 4480, 3.9f, 4.24f },
            { FilterBand::HighShelf, 10000, -3.1f, 0.70f },
            { FilterBand::Peak, 480, 0.5f, 2.40f },
            { FilterBand::Peak, 234, -0.4f, 2.41f },
            { FilterBand::Peak, 940, -0.6f, 4.04f },
            { FilterBand::Peak, 2939, -0.5f, 4.92f }
        }
    },
    {
        "Audio-Technica ATH-M20x",
        "Entry-level closed-back. +7.7dB at 4.4kHz, -7.0dB at 76Hz.",
        -6.8f,
        {
            { FilterBand::LowShelf, 105, 6.8f, 0.70f },
            { FilterBand::Peak, 1496, -2.9f, 0.94f },
            { FilterBand::Peak, 4437, 7.7f, 1.40f },
            { FilterBand::Peak, 76, -7.0f, 0.88f },
            { FilterBand::Peak, 2559, -3.1f, 1.82f },
            { FilterBand::HighShelf, 10000, -0.6f, 0.70f },
            { FilterBand::Peak, 248, 2.8f, 2.38f },
            { FilterBand::Peak, 137, -1.6f, 2.05f },
            { FilterBand::Peak, 76, 0.9f, 2.36f },
            { FilterBand::Peak, 49, -0.7f, 2.58f }
        }
    }
};

// ==============================================================================
// prepare
// ==============================================================================
void HeadphoneCalibration::prepare(double sr, int blockSize)
{
    sampleRate   = sr;
    maxBlockSize = blockSize;
    rebuildFilters();
}

// ==============================================================================
// process — cascade biquad stages with preamp gain first
// ==============================================================================
void HeadphoneCalibration::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled || filterStages.empty())
        return;

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    // Apply preamp gain (negative, prevents clipping from EQ boosts)
    if (preampGainDB != 0.0f)
    {
        const float preampLin = juce::Decibels::decibelsToGain(preampGainDB * gain);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            juce::FloatVectorOperations::multiply(data, preampLin, numSamples);
        }
    }

    // Cascade biquad filters
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    for (auto& stage : filterStages)
        stage.process(ctx);
}

void HeadphoneCalibration::reset()
{
    for (auto& stage : filterStages)
        stage.reset();
}

// ==============================================================================
// Profile switching
// ==============================================================================
void HeadphoneCalibration::setProfile(int index)
{
    jassert(index >= 0 && index < static_cast<int>(profiles.size()));
    currentProfile = index;
    rebuildFilters();
}

// ==============================================================================
// Build biquad stages from profile bands
// ==============================================================================
void HeadphoneCalibration::rebuildFilters()
{
    filterStages.clear();
    preampGainDB = 0.0f;

    if (currentProfile < 0 || currentProfile >= static_cast<int>(profiles.size()))
        return;

    const auto& profile = profiles[static_cast<size_t>(currentProfile)];
    preampGainDB = profile.preampDB;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels      = 2;

    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    for (const auto& band : profile.bands)
    {
        const float effectiveGain = band.gainDB * gain;

        Coeffs::Ptr b;
        switch (band.type)
        {
            case FilterBand::LowShelf:
                b = Coeffs::makeLowShelf(sampleRate, band.freqHz,
                                         1.0f / std::sqrt(band.Q),
                                         juce::Decibels::decibelsToGain(effectiveGain));
                break;
            case FilterBand::Peak:
                b = Coeffs::makePeakFilter(sampleRate, band.freqHz, band.Q,
                                           juce::Decibels::decibelsToGain(effectiveGain));
                break;
            case FilterBand::HighShelf:
                b = Coeffs::makeHighShelf(sampleRate, band.freqHz,
                                          1.0f / std::sqrt(band.Q),
                                          juce::Decibels::decibelsToGain(effectiveGain));
                break;
        }

        if (b != nullptr)
        {
            juce::dsp::IIR::Filter<float> stage;
            stage.prepare(spec);
            stage.coefficients = b;
            filterStages.push_back(std::move(stage));
        }
    }
}

// ==============================================================================
// Magnitude response at frequency (for UI graph, includes preamp)
// ==============================================================================
float HeadphoneCalibration::getMagnitudeDB(float freqHz) const
{
    if (currentProfile < 0 || currentProfile >= static_cast<int>(profiles.size()))
        return 0.0f;

    const auto& profile = profiles[static_cast<size_t>(currentProfile)];
    float totalDB = profile.preampDB * gain;

    for (const auto& band : profile.bands)
    {
        const float effectiveGain = band.gainDB * gain;
        const float w    = freqHz / band.freqHz;
        const float w2   = w * w;

        float bandDB = 0.0f;

        switch (band.type)
        {
            case FilterBand::Peak:
            {
                const float A     = std::pow(10.0f, effectiveGain / 40.0f);
                const float denom = (1.0f - w2) * (1.0f - w2) + w2 / (band.Q * band.Q);
                if (denom < 1e-10f) break;
                const float numA   = 1.0f + A * A * w2 / (band.Q * band.Q * denom);
                const float numInv = 1.0f + w2 / (A * A * band.Q * band.Q * denom);
                bandDB = 10.0f * std::log10(numA / numInv);
                break;
            }
            case FilterBand::LowShelf:
            {
                const float A     = std::pow(10.0f, effectiveGain / 40.0f);
                const float sqrtA = std::sqrt(A);
                const float wS    = w * band.Q;
                const float num   = A * A * w2 + 2.0f * sqrtA * A * wS + 1.0f;
                const float den   = w2 + 2.0f * sqrtA * wS + 1.0f;
                bandDB = 10.0f * std::log10(juce::jmax(num / den, 1e-6f));
                break;
            }
            case FilterBand::HighShelf:
            {
                const float A     = std::pow(10.0f, effectiveGain / 40.0f);
                const float sqrtA = std::sqrt(A);
                const float wS    = w * band.Q;
                const float num   = A * A + 2.0f * sqrtA * A * wS + w2;
                const float den   = 1.0f + 2.0f * sqrtA * wS + w2;
                bandDB = 10.0f * std::log10(juce::jmax(num / den, 1e-6f));
                break;
            }
        }

        totalDB += bandDB;
    }

    return totalDB;
}
