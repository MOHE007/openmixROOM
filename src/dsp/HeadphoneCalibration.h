#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// ==============================================================================
// HeadphoneCalibration — parametric EQ-based headphone frequency response
// correction. Uses cascaded biquad filters to compensate for headphone
// coloration and match a flat studio reference target.
//
// Built-in profiles for 10 popular studio headphones, based on publicly
// available measurement data (Oratory1990 / AutoEq / Sonarworks).
// ==============================================================================
class HeadphoneCalibration
{
public:
    struct FilterBand
    {
        enum Type { LowShelf = 0, Peak = 1, HighShelf = 2 };
        Type   type;
        float  freqHz;
        float  gainDB;
        float  Q;
    };

    struct Profile
    {
        juce::String              name;
        juce::String              description;
        float                     preampDB = 0.0f;  // Negative gain to prevent clipping
        std::vector<FilterBand>   bands;
    };

    HeadphoneCalibration()  = default;
    ~HeadphoneCalibration() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    void setEnabled(bool e)       { enabled = e; }
    bool isEnabled() const        { return enabled; }

    void setProfile(int index);
    int  getCurrentProfile() const { return currentProfile; }
    int  getProfileCount() const   { return static_cast<int>(profiles.size()); }

    const Profile& getProfile(int index) const { return profiles[index]; }

    // Magnitude response at a given frequency (for UI graphing), returns dB
    float getMagnitudeDB(float freqHz) const;

    // Gain parameter (0..100% = strength of correction)
    void  setGain(float g) { gain = juce::jlimit(0.0f, 1.0f, g); rebuildFilters(); }
    float getGain() const  { return gain; }

private:
    void rebuildFilters();

    double   sampleRate     = 44100.0;
    int      maxBlockSize   = 512;
    bool     enabled        = false;
    float    gain           = 1.0f;
    float    preampGainDB   = 0.0f;
    int      currentProfile = -1;

    // Vector of stereo biquad stages — processed in sequence
    std::vector<juce::dsp::IIR::Filter<float>> filterStages;

    static const std::vector<Profile> profiles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeadphoneCalibration)
};
