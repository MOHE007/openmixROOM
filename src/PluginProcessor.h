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
class OpenMixRoomAudioProcessor : public juce::AudioProcessor,
                                       private juce::AudioProcessorParameter::Listener
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

private:
    // AudioProcessorParameter::Listener — syncs DSP state on parameter changes
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
    // Parameters
    juce::AudioParameterFloat*  mixParam           = nullptr;
    juce::AudioParameterFloat*  crossfeedParam     = nullptr;
    juce::AudioParameterFloat*  cutoffParam        = nullptr;
    juce::AudioParameterChoice* algorithmParam     = nullptr;
    juce::AudioParameterBool*   bypassParam        = nullptr;
    juce::AudioParameterFloat*  roomMixParam        = nullptr;
    juce::AudioParameterChoice* roomTypeParam       = nullptr;
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
