#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
// OpenMixRoomAudioProcessorEditor — Phase 1 GUI scaffold.
//
// Layout (700×450):
//   ┌────────────────────────────────────────────────┐
//   │  OpenMix Room                       (title)     │
//   │  v0.1.0 — Phase 1: Audio Pass-through (version) │
//   ├────────┬───────────────────────────────────────┤
//   │  Mix   │                                       │
//   │ slider │      Input ──► [OpenMix DSP] ──► Out  │
//   │        │            (signal path diagram)       │
//   │        │                                       │
//   ├────────┴───────────────────────────────────────┤
//   │  Sample Rate: -- | Buffer: -- | Latency: 0     │
//   └────────────────────────────────────────────────┘
// ==============================================================================
class OpenMixRoomAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    explicit OpenMixRoomAudioProcessorEditor (OpenMixRoomAudioProcessor& processor);
    ~OpenMixRoomAudioProcessorEditor() override = default;

    // --------------------------------------------------------------------------
    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    OpenMixRoomAudioProcessor& audioProcessor;

    // --- widgets ---
    juce::Label  titleLabel;
    juce::Label  versionLabel;
    juce::Label  statusLabel;
    juce::Slider mixSlider;

    // Parameter attachment for the Mix slider
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Layout constants
    static constexpr int statusBarHeight = 28;
    static constexpr int titleAreaHeight = 80;
    static constexpr int sliderAreaWidth = 140;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenMixRoomAudioProcessorEditor)
};
