#include "PluginEditor.h"

// ==============================================================================
// Constructor
// ==============================================================================
OpenMixRoomAudioProcessorEditor::OpenMixRoomAudioProcessorEditor (
    OpenMixRoomAudioProcessor& processor)
    : juce::AudioProcessorEditor (processor),
      audioProcessor (processor)
{
    // --------------------------------------------------------------------------
    // Window size
    // --------------------------------------------------------------------------
    setSize (700, 450);

    // --------------------------------------------------------------------------
    // Title label — bold, bright, centred at the top
    // --------------------------------------------------------------------------
    titleLabel.setText ("OpenMix Room", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (24.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::whitesmoke);
    titleLabel.setJustificationType (juce::Justification::centredTop);
    addAndMakeVisible (titleLabel);

    // --------------------------------------------------------------------------
    // Version label — lightweight, grey, sits right below the title
    // --------------------------------------------------------------------------
    versionLabel.setText ("v0.1.0 — Phase 1: Audio Pass-through",
                          juce::dontSendNotification);
    versionLabel.setFont (juce::Font (12.0f));
    versionLabel.setColour (juce::Label::textColourId,
                            juce::Colour (160, 160, 165));
    versionLabel.setJustificationType (juce::Justification::centredTop);
    addAndMakeVisible (versionLabel);

    // --------------------------------------------------------------------------
    // Mix slider — vertical, 0–100, default 100
    // --------------------------------------------------------------------------
    mixSlider.setSliderStyle (juce::Slider::LinearVertical);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    mixSlider.setRange (0.0, 100.0, 1.0);
    mixSlider.setValue (100.0);
    mixSlider.setColour (juce::Slider::thumbColourId,
                         juce::Colour (100, 180, 255));
    mixSlider.setColour (juce::Slider::trackColourId,
                         juce::Colour (60, 60, 65));
    addAndMakeVisible (mixSlider);

    // Attach the slider to the processor's parameter tree.
    // We construct a simple ValueTreeState wrapper on the fly.
    juce::AudioProcessorValueTreeState apvts (
        audioProcessor,
        nullptr,
        juce::Identifier ("OpenMixRoomParams"),
        { std::make_unique<juce::AudioParameterFloat> (
              "mix", "Mix",
              juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
              100.0f) });
    // Note: apvts is temporary; we only use it to create the attachment.
    // The attachment will keep the slider in sync via the parameter pointer.
    mixAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (
        apvts, "mix", mixSlider));

    // --------------------------------------------------------------------------
    // Status bar label — bottom strip with live DAW info
    // --------------------------------------------------------------------------
    statusLabel.setFont (juce::Font (11.0f));
    statusLabel.setColour (juce::Label::textColourId,
                           juce::Colour (140, 140, 145));
    statusLabel.setColour (juce::Label::backgroundColourId,
                           juce::Colour (22, 22, 24));
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    // --------------------------------------------------------------------------
    // Timer — refresh the status bar at ~30 fps
    // --------------------------------------------------------------------------
    startTimerHz (30);
}

// ==============================================================================
// paint — custom background + signal-path diagram
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // --- Dark background -----------------------------------------------
    g.fillAll (juce::Colour (28, 28, 30));

    // --- Signal-path diagram (central area) ----------------------------
    // Calculate the diagram area: below the title strip, above the status bar.
    const auto diagramArea = bounds
        .withTrimmedTop (titleAreaHeight)
        .withTrimmedBottom (statusBarHeight)
        .withTrimmedLeft  (sliderAreaWidth);

    const int boxWidth  = 150;
    const int boxHeight = 55;
    const int arrowGap  = 40;

    const int centreY = diagramArea.getCentreY();
    const int totalWidth = boxWidth * 3 + arrowGap * 2;
    const int startX = diagramArea.getCentreX() - totalWidth / 2;

    struct BoxDesc
    {
        juce::Rectangle<int> rect;
        juce::String text;
        juce::Colour fill;
    };

    // Three boxes: Input, OpenMix DSP, Output
    const BoxDesc boxes[] = {
        { { startX, centreY - boxHeight / 2, boxWidth, boxHeight },
          "Input", juce::Colour (50, 90, 140) },
        { { startX + boxWidth + arrowGap, centreY - boxHeight / 2, boxWidth, boxHeight },
          "OpenMix Room DSP", juce::Colour (60, 140, 100) },
        { { startX + (boxWidth + arrowGap) * 2, centreY - boxHeight / 2, boxWidth, boxHeight },
          "Output", juce::Colour (140, 80, 50) }
    };

    for (const auto& b : boxes)
    {
        g.setColour (b.fill.withAlpha (0.3f));
        g.fillRoundedRectangle (b.rect.toFloat(), 8.0f);
        g.setColour (b.fill.withAlpha (0.7f));
        g.drawRoundedRectangle (b.rect.toFloat(), 8.0f, 1.5f);
        g.setColour (juce::Colours::whitesmoke);
        g.setFont (juce::Font (15.0f, juce::Font::bold));
        g.drawText (b.text, b.rect, juce::Justification::centred, false);
    }

    // Arrows between boxes
    const auto drawArrow = [&](int x1, int x2, int y)
    {
        const juce::Line<float> line (static_cast<float> (x1), static_cast<float> (y),
                                       static_cast<float> (x2), static_cast<float> (y));
        g.setColour (juce::Colour (180, 180, 190));
        g.drawArrow (line, 6.0f, 10.0f, 10.0f);
    };

    const int arrowY = centreY;
    drawArrow (startX + boxWidth,
               startX + boxWidth + arrowGap,
               arrowY);
    drawArrow (startX + boxWidth * 2 + arrowGap,
               startX + boxWidth * 2 + arrowGap * 2,
               arrowY);

    // --- Bottom separator line ----------------------------------------
    g.setColour (juce::Colour (60, 60, 65));
    g.drawHorizontalLine (bounds.getBottom() - statusBarHeight,
                          0.0f,
                          static_cast<float> (bounds.getWidth()));
}

// ==============================================================================
// resized — layout widgets
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title & version
    auto titleArea = bounds.removeFromTop (titleAreaHeight);
    titleLabel.setBounds   (titleArea.removeFromTop (titleAreaHeight / 2));
    versionLabel.setBounds (titleArea);

    // Status bar
    statusLabel.setBounds (bounds.removeFromBottom (statusBarHeight));

    // Mix slider — left strip
    const auto sliderArea = bounds.removeFromLeft (sliderAreaWidth).reduced (12, 20);
    mixSlider.setBounds (sliderArea);

    // The remaining area is the signal-path diagram (handled in paint()),
    // so no child widgets need to be placed there.
}

// ==============================================================================
// timerCallback — update the status bar text every ~33 ms
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::timerCallback()
{
    const auto sr     = audioProcessor.getCurrentSampleRate();
    const auto blk    = audioProcessor.getCurrentBlockSize();
    const auto latency = 0;   // Phase 1 has zero latency

    statusLabel.setText (
        juce::String::formatted ("Sample Rate: %d Hz | Buffer: %d samples | Latency: %d samples",
                                 static_cast<int> (std::round (sr)),
                                 blk,
                                 latency),
        juce::dontSendNotification);
}
