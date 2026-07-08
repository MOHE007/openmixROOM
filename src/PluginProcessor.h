#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/SofaLoader.h"
#include "dsp/CrossfeedProcessor.h"
#include "dsp/RoomProcessor.h"

// ==============================================================================
// OpenMixRoomAudioProcessor — core DSP processor for the OpenMix Room plugin.
//
// Phase 2: HRTF loading + Crossfeed + dry/wet mix.
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

    // DSP accessors (for editor)
    const SofaLoader& getSofaLoader() const noexcept { return sofaLoader; }

private:
    // --------------------------------------------------------------------------
    // Parameters
    // --------------------------------------------------------------------------
    juce::AudioParameterFloat* mixParam    = nullptr;  // 0..100 %, default 100 %
    juce::AudioParameterFloat* crossfeedParam = nullptr; // 0..100 %, default 50 %
    juce::AudioParameterFloat* cutoffParam  = nullptr;  // 100..2000 Hz, default 700
    juce::AudioParameterChoice* algorithmParam = nullptr; // Bauer / Meier / Chu Moy / HRTF
    juce::AudioParameterBool*  bypassParam  = nullptr;  // true = bypass all processing
    juce::AudioParameterFloat* roomMixParam  = nullptr;  // 0..100 %, default 30 %
    juce::AudioParameterChoice* roomTypeParam = nullptr;  // Small / Medium / Large

    // --------------------------------------------------------------------------
    // DSP modules
    // --------------------------------------------------------------------------
    SofaLoader         sofaLoader;
    CrossfeedProcessor crossfeed;
    RoomProcessor       room;

    // --------------------------------------------------------------------------
    // Runtime state
    // --------------------------------------------------------------------------
    double currentSampleRate = 44100.0;
    int    currentBlockSize   = 512;

    // Internal helper
    juce::String getDefaultSofaPath() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenMixRoomAudioProcessor)
};
