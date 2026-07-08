#include "PluginEditor.h"

// ==============================================================================
// Constructor
// ==============================================================================
OpenMixRoomAudioProcessorEditor::OpenMixRoomAudioProcessorEditor (
    OpenMixRoomAudioProcessor& processor)
    : juce::AudioProcessorEditor (processor),
      audioProcessor (processor),
      apvts (processor, nullptr, juce::Identifier ("OpenMixRoomParams"),
             {
                 std::make_unique<juce::AudioParameterFloat> ("mix", "Mix",
                     juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f),
                 std::make_unique<juce::AudioParameterFloat> ("crossfeed", "Crossfeed",
                     juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f),
                 std::make_unique<juce::AudioParameterFloat> ("cutoff", "Cutoff",
                     juce::NormalisableRange<float> (100.0f, 2000.0f, 1.0f), 700.0f),
                 std::make_unique<juce::AudioParameterChoice> ("algorithm", "Algorithm",
                     juce::StringArray { "Bauer", "Meier", "Chu Moy", "HRTF" }, 0)
             })
{
    setSize (700, 450);

    // --------------------------------------------------------------------------
    // Title label
    // --------------------------------------------------------------------------
    titleLabel.setText ("OpenMix Room", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::whitesmoke);
    titleLabel.setJustificationType (juce::Justification::centredTop);
    addAndMakeVisible (titleLabel);

    // --------------------------------------------------------------------------
    // Version label
    // --------------------------------------------------------------------------
    versionLabel.setText ("v0.2.0 — Phase 2: Crossfeed",
                          juce::dontSendNotification);
    versionLabel.setFont (juce::Font (11.0f));
    versionLabel.setColour (juce::Label::textColourId, juce::Colour (150, 150, 155));
    versionLabel.setJustificationType (juce::Justification::centredTop);
    addAndMakeVisible (versionLabel);

    // --------------------------------------------------------------------------
    // Mix slider
    // --------------------------------------------------------------------------
    mixLabel.setText ("Mix", juce::dontSendNotification);
    mixLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    mixLabel.setColour (juce::Label::textColourId, juce::Colour (160, 160, 165));
    mixLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mixLabel);

    mixSlider.setSliderStyle (juce::Slider::LinearVertical);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    mixSlider.setRange (0.0, 100.0, 1.0);
    mixSlider.setValue (100.0);
    mixSlider.setColour (juce::Slider::thumbColourId, juce::Colour (100, 180, 255));
    mixSlider.setColour (juce::Slider::trackColourId, juce::Colour (60, 60, 65));
    addAndMakeVisible (mixSlider);

    mixAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (
        apvts, "mix", mixSlider));

    // --------------------------------------------------------------------------
    // Crossfeed slider
    // --------------------------------------------------------------------------
    crossfeedLabel.setText ("Crossfeed", juce::dontSendNotification);
    crossfeedLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    crossfeedLabel.setColour (juce::Label::textColourId, juce::Colour (160, 160, 165));
    crossfeedLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (crossfeedLabel);

    crossfeedSlider.setSliderStyle (juce::Slider::LinearVertical);
    crossfeedSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    crossfeedSlider.setRange (0.0, 100.0, 1.0);
    crossfeedSlider.setValue (50.0);
    crossfeedSlider.setColour (juce::Slider::thumbColourId, juce::Colour (180, 140, 100));
    crossfeedSlider.setColour (juce::Slider::trackColourId, juce::Colour (60, 60, 65));
    addAndMakeVisible (crossfeedSlider);

    crossfeedAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (
        apvts, "crossfeed", crossfeedSlider));

    // --------------------------------------------------------------------------
    // Algorithm combo box
    // --------------------------------------------------------------------------
    algorithmLabel.setText ("Algorithm", juce::dontSendNotification);
    algorithmLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    algorithmLabel.setColour (juce::Label::textColourId, juce::Colour (160, 160, 165));
    algorithmLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (algorithmLabel);

    algorithmCombo.addItemList ({ "Bauer", "Meier", "Chu Moy", "HRTF" }, 1);
    algorithmCombo.setSelectedId (1);
    algorithmCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour (40, 40, 45));
    algorithmCombo.setColour (juce::ComboBox::textColourId, juce::Colours::whitesmoke);
    algorithmCombo.setColour (juce::ComboBox::outlineColourId, juce::Colour (60, 60, 65));
    addAndMakeVisible (algorithmCombo);

    algorithmAttachment.reset (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (
        apvts, "algorithm", algorithmCombo));

    // --------------------------------------------------------------------------
    // Cutoff slider
    // --------------------------------------------------------------------------
    cutoffLabel.setText ("Cutoff", juce::dontSendNotification);
    cutoffLabel.setFont (juce::Font (11.0f, juce::Font::bold));
    cutoffLabel.setColour (juce::Label::textColourId, juce::Colour (160, 160, 165));
    cutoffLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (cutoffLabel);

    cutoffSlider.setSliderStyle (juce::Slider::LinearVertical);
    cutoffSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 20);
    cutoffSlider.setRange (100.0, 2000.0, 1.0);
    cutoffSlider.setValue (700.0);
    cutoffSlider.setTextValueSuffix (" Hz");
    cutoffSlider.setColour (juce::Slider::thumbColourId, juce::Colour (140, 160, 100));
    cutoffSlider.setColour (juce::Slider::trackColourId, juce::Colour (60, 60, 65));
    addAndMakeVisible (cutoffSlider);

    cutoffAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (
        apvts, "cutoff", cutoffSlider));

    // --------------------------------------------------------------------------
    // Status bar
    // --------------------------------------------------------------------------
    statusLabel.setFont (juce::Font (11.0f));
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (140, 140, 145));
    statusLabel.setColour (juce::Label::backgroundColourId, juce::Colour (22, 22, 24));
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    startTimerHz (30);
}

// ==============================================================================
// paint
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Dark background
    g.fillAll (juce::Colour (28, 28, 30));

    // Signal-path diagram — to the right of the slider columns
    const auto diagramArea = bounds
        .withTrimmedTop (titleAreaHeight)
        .withTrimmedBottom (statusBarHeight)
        .withTrimmedLeft  (sliderAreaWidth * 2);

    const int boxWidth  = 120;
    const int boxHeight = 45;
    const int arrowGap  = 40;
    const int centreY   = diagramArea.getCentreY();
    const int totalW    = boxWidth * 3 + arrowGap * 2;
    const int startX    = diagramArea.getCentreX() - totalW / 2;

    struct Box { juce::Rectangle<int> r; juce::String t; juce::Colour c; };
    const Box boxes[] = {
        { { startX, centreY - boxHeight / 2, boxWidth, boxHeight },
          "Input", juce::Colour (50, 90, 140) },
        { { startX + boxWidth + arrowGap, centreY - boxHeight / 2, boxWidth, boxHeight },
          "Crossfeed", juce::Colour (60, 140, 100) },
        { { startX + (boxWidth + arrowGap) * 2, centreY - boxHeight / 2, boxWidth, boxHeight },
          "Output", juce::Colour (140, 80, 50) }
    };

    for (auto& b : boxes)
    {
        g.setColour (b.c.withAlpha (0.3f));
        g.fillRoundedRectangle (b.r.toFloat(), 8.0f);
        g.setColour (b.c.withAlpha (0.7f));
        g.drawRoundedRectangle (b.r.toFloat(), 8.0f, 1.5f);
        g.setColour (juce::Colours::whitesmoke);
        g.setFont (juce::Font (14.0f, juce::Font::bold));
        g.drawText (b.t, b.r, juce::Justification::centred, false);
    }

    // Arrows
    for (int i = 0; i < 2; ++i)
    {
        int x1 = startX + (boxWidth + arrowGap) * i + boxWidth;
        int x2 = x1 + arrowGap;
        juce::Line<float> line (static_cast<float>(x1), static_cast<float>(centreY),
                                 static_cast<float>(x2), static_cast<float>(centreY));
        g.setColour (juce::Colour (180, 180, 190));
        g.drawArrow (line, 6.0f, 9.0f, 9.0f);
    }

    // Bottom separator
    g.setColour (juce::Colour (60, 60, 65));
    g.drawHorizontalLine (bounds.getBottom() - statusBarHeight, 0.0f,
                          static_cast<float>(bounds.getWidth()));
}

// ==============================================================================
// resized
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title
    auto titleArea = bounds.removeFromTop (titleAreaHeight);
    titleLabel.setBounds (titleArea.removeFromTop (titleAreaHeight * 2 / 3));
    versionLabel.setBounds (titleArea);

    // Status bar
    statusLabel.setBounds (bounds.removeFromBottom (statusBarHeight));

    // Left column: Mix slider
    auto mixCol = bounds.removeFromLeft (sliderAreaWidth).reduced (10, 15);
    mixLabel.setBounds (mixCol.removeFromTop (18));
    mixSlider.setBounds (mixCol);

    // Right column: Crossfeed controls
    auto xfCol = bounds.removeFromLeft (sliderAreaWidth).reduced (10, 15);
    crossfeedLabel.setBounds (xfCol.removeFromTop (18));
    crossfeedSlider.setBounds (xfCol.removeFromTop (xfCol.getHeight() * 2 / 3));
    algorithmLabel.setBounds (xfCol.removeFromTop (18));
    algorithmCombo.setBounds (xfCol.removeFromTop (24));
    cutoffLabel.setBounds (xfCol.removeFromTop (18));
    cutoffSlider.setBounds (xfCol);
}

// ==============================================================================
// timerCallback
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::timerCallback()
{
    const auto sr   = audioProcessor.getCurrentSampleRate();
    const auto blk  = audioProcessor.getCurrentBlockSize();
    const auto sofa = audioProcessor.getSofaLoader().isLoaded() ? "loaded" : "none";

    statusLabel.setText (
        juce::String::formatted ("Sample Rate: %d Hz | Buffer: %d | SOFA: %s",
                                 static_cast<int>(std::round(sr)), blk, sofa),
        juce::dontSendNotification);
}
