#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include "PluginProcessor.h"
#include "ui/FrequencyResponseGraph.h"
#include "ui/RoomResponseGraph.h"
#include "ui/NeumorphicLookAndFeel.h"
#include "ui/LogoComponent.h"

// ==============================================================================
// Neumorphic UI — soft, light design with raised/inset card panels.
// Left panel: headphone calibration + logo. Center: FR graph. Right: room.
// Parameter sync: all controls use manual onClick/onValueChange callbacks
// to the processor's setters; no APVTS needed — processor uses raw addParameter().
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

    // ---- Neumorphic LookAndFeel ----
    NeumorphicLookAndFeel neumorphicLF;

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
    juce::TextButton importProfileButton;
    juce::TextButton calToggle;
    juce::Label    calGainLabel;
    juce::Slider   calGainSlider;

    // ---- Logo (left panel, below cal controls) ----
    LogoComponent logoComponent;

    // ---- Right: Room + Crossfeed ----
    juce::Label    vmSectionLabel;
    juce::TextButton roomToggle;
    juce::ComboBox roomTypeCombo;
    juce::Label    roomSizeLabel;
    juce::Slider   roomSizeSlider;
    juce::Label    preDelayLabel;
    juce::Slider   preDelaySlider;
    juce::Label    erLevelLabel;
    juce::Slider   erLevelSlider;
    juce::Label    roomDampLabel;
    juce::Slider   roomDampSlider;
    juce::Label    roomMixLabel;
    juce::Slider   roomMixSlider;
    juce::Label    crossfeedLabel;
    juce::Slider   crossfeedSlider;
    juce::Label    cutoffLabel;
    juce::Slider   cutoffSlider;
    juce::ComboBox algorithmCombo;
    juce::Label    mixLabel;
    juce::Slider   mixSlider;

    // ---- Room Visualizer ----
    RoomResponseGraph roomResponseGraph;
    juce::Rectangle<int> roomVisRect;

    // ---- Status bar ----
    juce::Label statusLabel;

    // ---- File import ----
    std::unique_ptr<juce::FileChooser> chooser;

    void styleSlider(juce::Slider& s, float v, float lo, float hi, float st, const juce::String& sfx);
    void styleCombo(juce::ComboBox& cb);
    void rebuildCalProfileCombo();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenMixRoomAudioProcessorEditor)
};
