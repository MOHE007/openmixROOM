#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/SofaLoader.h"
#include "dsp/CrossfeedProcessor.h"
#include "dsp/RoomProcessor.h"
#include "dsp/HeadphoneCalibration.h"

// ==============================================================================
// OpenMixRoomAudioProcessor — virtual monitoring for headphones.
//
// Signal chain (v0.4):
//   Input → Headphone Cal EQ → Crossfeed → Room IR → Output
//
// Phase 4: Headphone Lab-inspired UI + headphone frequency response calibration.
// ==============================================================================
class OpenMixRoomAudioProcessor : public juce::AudioProcessor
{
public:
    OpenMixRoomAudioProcessor();
    ~OpenMixRoomAudioProcessor() override = default;

    // AudioProcessor overrides
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midiMessages) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    int    getNumPrograms() override                     { return 1; }
    int    getCurrentProgram() override                  { return 0; }
    void   setCurrentProgram (int) override              {}
    const juce::String getProgramName (int) override     { return "Default"; }
    void   changeProgramName (int, const juce::String&) override {}

    bool   acceptsMidi() const override    { return false; }
    bool   producesMidi() const override   { return false; }
    bool   isMidiEffect() const override   { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    const juce::String getName() const override { return "OpenMix Room"; }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Runtime info for editor
    double getCurrentSampleRate() const noexcept { return currentSampleRate; }
    int    getCurrentBlockSize()   const noexcept { return currentBlockSize; }
    const SofaLoader& getSofaLoader() const noexcept { return sofaLoader; }

    // DSP accessors
    const HeadphoneCalibration& getHeadphoneCal() const noexcept { return headphoneCal; }
          HeadphoneCalibration& getHeadphoneCal()       noexcept { return headphoneCal; }
    RoomProcessor& getRoomProcessor() noexcept { return room; }

    // Atomic setters: update both parameter tree and DSP in one call
    void setCalProfile(int index);
    void setCalEnabled(bool on);
    void setCalGain(float percent);
    void setRoomEnabled(bool on);
    void setRoomMix(float percent);
    void setRoomType(int index);

    // Custom profile import
    juce::Result importCalProfile(const juce::String& filePath);

private:
    // Internal parameter listener — bridges APVTS → DSP without multiple inheritance
    struct CalListener final : juce::AudioProcessorParameter::Listener
    {
        explicit CalListener(HeadphoneCalibration& hc) : cal(hc) {}
        void parameterValueChanged(int idx, float) override
        {
            if (profileParam && idx == profileParam->getParameterIndex())
                cal.setProfile(profileParam->getIndex());
            else if (enabledParam && idx == enabledParam->getParameterIndex())
                cal.setEnabled(enabledParam->get());
        }
        void parameterGestureChanged(int, bool) override {}

        HeadphoneCalibration& cal;
        juce::AudioParameterChoice* profileParam = nullptr;
        juce::AudioParameterBool*   enabledParam = nullptr;
    };
    std::unique_ptr<CalListener> calListener;

    // Parameters
    juce::AudioParameterFloat*  mixParam           = nullptr;
    juce::AudioParameterFloat*  crossfeedParam     = nullptr;
    juce::AudioParameterFloat*  cutoffParam        = nullptr;
    juce::AudioParameterChoice* algorithmParam     = nullptr;
    juce::AudioParameterBool*   bypassParam        = nullptr;
    juce::AudioParameterFloat*  roomMixParam        = nullptr;
    juce::AudioParameterChoice* roomTypeParam       = nullptr;
    juce::AudioParameterBool*   roomEnabledParam    = nullptr;  // Room bypass (default off)
    juce::AudioParameterChoice* calProfileParam     = nullptr;  // Headphone model
    juce::AudioParameterBool*   calEnabledParam     = nullptr;  // Calibration on/off
    juce::AudioParameterFloat*  calGainParam        = nullptr;  // Calibration strength

    // DSP modules
    SofaLoader            sofaLoader;
    HeadphoneCalibration  headphoneCal;
    CrossfeedProcessor    crossfeed;
    RoomProcessor         room;

    // Runtime state
    double currentSampleRate = 44100.0;
    int    currentBlockSize   = 512;

    juce::String getDefaultSofaPath() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenMixRoomAudioProcessor)
};
