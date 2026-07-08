#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// ==============================================================================
// OpenMixRoomAudioProcessorEditor — Phase 2 GUI with Crossfeed controls.
//
// Layout (700×450):
//   ┌─────────────────────────────────────────────────────────────┐
//   │  OpenMix Room                              v0.2.0            │
//   │  Virtual Monitoring — Phase 2: Crossfeed                   │
//   ├──────┬──────────────┬──────────────────────────────────────┤
//   │ Mix  │ Crossfeed    │                                       │
//   │ 100% │   50%        │       Signal Path Diagram             │
//   │      │              │                                       │
//   │      │ [Bauer  ▼]  │   Input ──► [Crossfeed] ──► Output   │
//   │      │ 700 Hz       │                                       │
//   ├──────┴──────────────┴──────────────────────────────────────┤
//   │  Sample Rate: 48000 Hz | Buffer: 512 | SOFA: loaded        │
//   └─────────────────────────────────────────────────────────────┘
// ==============================================================================
class OpenMixRoomAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    explicit OpenMixRoomAudioProcessorEditor (OpenMixRoomAudioProcessor& processor);
    ~OpenMixRoomAudioProcessorEditor() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    OpenMixRoomAudioProcessor& audioProcessor;

    // --- widgets ---
    juce::Label   titleLabel;
    juce::Label   versionLabel;
    juce::Label   statusLabel;

    // Mix section
    juce::Label   mixLabel;
    juce::Slider  mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Crossfeed section
    juce::Label   crossfeedLabel;
    juce::Slider  crossfeedSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossfeedAttachment;

    juce::Label   cutoffLabel;
    juce::Slider  cutoffSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;

    juce::Label   algorithmLabel;
    juce::ComboBox algorithmCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algorithmAttachment;

    // Bypass toggle
    juce::TextButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // Room section
    juce::Label   roomLabel;
    juce::Label   roomTypeLabel;
    juce::ComboBox roomTypeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> roomTypeAttachment;

    juce::Label   roomMixLabel;
    juce::Slider  roomMixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomMixAttachment;

    // APVTS for parameter attachments
    juce::AudioProcessorValueTreeState apvts;

    // Layout constants
    static constexpr int statusBarHeight = 28;
    static constexpr int titleAreaHeight = 75;
    static constexpr int sliderAreaWidth = 140;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenMixRoomAudioProcessorEditor)
};
