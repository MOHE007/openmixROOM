#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"
#include "ui/FrequencyResponseGraph.h"

// ==============================================================================
// Waves Nx–inspired UI: dark radial gradient background, central FR graph
// as the hero element, orange accent (#e8913a), slim parameter controls.
// ==============================================================================
class OpenMixRoomAudioProcessorEditor final
    : public juce::AudioProcessorEditor
    , public juce::Timer
{
public:
    explicit OpenMixRoomAudioProcessorEditor(OpenMixRoomAudioProcessor&);
    ~OpenMixRoomAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    OpenMixRoomAudioProcessor& audioProcessor;

    static constexpr int windowW = 820;
    static constexpr int windowH = 530;

    // ---- Sections ----
    juce::Rectangle<int> headerRect;
    juce::Rectangle<int> graphRect;
    juce::Rectangle<int> leftPanelRect;
    juce::Rectangle<int> rightPanelRect;
    juce::Rectangle<int> statusRect;

    // ---- Header ----
    juce::TextButton bypassButton;
    juce::Label      titleLabel;

    // ---- FR Graph ----
    FrequencyResponseGraph frGraph;

    // ---- Left: Calibration ----
    juce::Label    calSectionLabel;
    juce::ComboBox calProfileCombo;
    juce::TextButton calToggle;
    juce::Label    calGainLabel;
    juce::Slider   calGainSlider;

    // ---- Right: Room + Crossfeed ----
    juce::Label    vmSectionLabel;
    juce::ComboBox roomTypeCombo;
    juce::Label    roomMixLabel;
    juce::Slider   roomMixSlider;
    juce::Label    crossfeedLabel;
    juce::Slider   crossfeedSlider;
    juce::Label    cutoffLabel;
    juce::Slider   cutoffSlider;
    juce::ComboBox algorithmCombo;
    juce::Label    mixLabel;
    juce::Slider   mixSlider;

    // ---- Status bar ----
    juce::Label statusLabel;

    // ---- APVTS ----
    juce::AudioProcessorValueTreeState apvts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> calGainA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xfA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomMixA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> calProfA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> roomTypeA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> calEnA;

    void styleSlider(juce::Slider& s, juce::Colour thumb, float v, float lo, float hi, float st, const juce::String& sfx);
    void styleCombo(juce::ComboBox& cb, juce::Colour accent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenMixRoomAudioProcessorEditor)
};
