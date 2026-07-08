#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// ==============================================================================
// OpenMixRoomAudioProcessor — core DSP processor for the OpenMix Room plugin.
//
// Phase 1: audio pass-through only.  The processBlock simply copies the input
// buffer to the output buffer.  Real DSP processing will be added in Phase 2+.
// ==============================================================================
class OpenMixRoomAudioProcessor : public juce::AudioProcessor
{
public:
    // --------------------------------------------------------------------------
    OpenMixRoomAudioProcessor();
    ~OpenMixRoomAudioProcessor() override = default;

    // --------------------------------------------------------------------------
    // AudioProcessor overrides
    // --------------------------------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Program management (single program for Phase 1)
    int    getNumPrograms() override                     { return 1; }
    int    getCurrentProgram() override                  { return 0; }
    void   setCurrentProgram (int /*index*/) override    {}
    const juce::String getProgramName (int /*index*/) override { return "Default"; }
    void   changeProgramName (int /*index*/, const juce::String& /*newName*/) override {}

    // MIDI — not used
    bool acceptsMidi() const override    { return false; }
    bool producesMidi() const override   { return false; }
    bool isMidiEffect() const override   { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Plugin name
    const juce::String getName() const override { return "OpenMix Room"; }

    // State persistence
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --------------------------------------------------------------------------
    // Public getters (used by the editor to display live info)
    // --------------------------------------------------------------------------
    double getCurrentSampleRate() const noexcept { return currentSampleRate; }
    int    getCurrentBlockSize()   const noexcept { return currentBlockSize; }

private:
    // --------------------------------------------------------------------------
    // Parameters
    // --------------------------------------------------------------------------
    juce::AudioParameterFloat* mixParam = nullptr;   // 0..100 %, default 100 %

    // --------------------------------------------------------------------------
    // Runtime state
    // --------------------------------------------------------------------------
    double currentSampleRate = 44100.0;
    int    currentBlockSize   = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenMixRoomAudioProcessor)
};
