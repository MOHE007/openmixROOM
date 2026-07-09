#include "PluginEditor.h"

// ==============================================================================
// Waves Nx colour palette
// ==============================================================================
static const juce::Colour bgBase     (20, 20, 24);    // #141418
static const juce::Colour bgPanel    (28, 28, 34);    // #1c1c22
static const juce::Colour borderDim  (44, 44, 52);    // #2c2c34
static const juce::Colour accent     (232, 145, 58);  // #e8913a — Waves orange
static const juce::Colour accentDim  (196, 122, 46);  // #c47a2e
static const juce::Colour textHi     (212, 212, 216); // #d4d4d8
static const juce::Colour textMid    (138, 138, 144); // #8a8a90
static const juce::Colour textLo     (90, 90, 96);    // #5a5a60
static const juce::Colour trackBg    (42, 42, 48);    // #2a2a30
static const juce::Colour comboBg    (34, 34, 40);    // #222228
static const juce::Colour bypassRed  (210, 55, 45);

// ==============================================================================
// Helper: style a linear slider with thin track and colour accent
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::styleSlider(juce::Slider& s, juce::Colour thumb,
                                                   float v, float lo, float hi, float st,
                                                   const juce::String& sfx)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 20);
    s.setRange(lo, hi, st);
    s.setValue(v);
    s.setTextValueSuffix(sfx);
    s.setColour(juce::Slider::thumbColourId,           thumb);
    s.setColour(juce::Slider::trackColourId,           accent.withAlpha(0.6f));
    s.setColour(juce::Slider::backgroundColourId,      trackBg);
    s.setColour(juce::Slider::textBoxTextColourId,     textMid);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(24, 24, 28));
    s.setColour(juce::Slider::textBoxOutlineColourId,  juce::Colours::transparentBlack);
}

void OpenMixRoomAudioProcessorEditor::styleCombo(juce::ComboBox& cb, juce::Colour accentCol)
{
    cb.setColour(juce::ComboBox::backgroundColourId, comboBg);
    cb.setColour(juce::ComboBox::textColourId,       textHi);
    cb.setColour(juce::ComboBox::outlineColourId,    borderDim);
    cb.setColour(juce::ComboBox::arrowColourId,      accentCol);
    cb.setColour(juce::ComboBox::focusedOutlineColourId, accentCol.withAlpha(0.5f));
}

// ==============================================================================
// Constructor
// ==============================================================================
OpenMixRoomAudioProcessorEditor::OpenMixRoomAudioProcessorEditor(
    OpenMixRoomAudioProcessor& processor)
    : juce::AudioProcessorEditor(processor)
    , audioProcessor(processor)
    , apvts(processor, nullptr, juce::Identifier("OpenMixRoomParams"),
            {
                std::make_unique<juce::AudioParameterFloat>("mix", "Mix",
                    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f),
                std::make_unique<juce::AudioParameterFloat>("crossfeed", "Crossfeed",
                    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f),
                std::make_unique<juce::AudioParameterFloat>("cutoff", "Cutoff",
                    juce::NormalisableRange<float>(100.0f, 2000.0f, 1.0f), 700.0f),
                std::make_unique<juce::AudioParameterChoice>("algorithm", "Algorithm",
                    juce::StringArray{"Bauer", "Meier", "Chu Moy", "HRTF"}, 0),
                std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false),
                std::make_unique<juce::AudioParameterFloat>("roomMix", "Room Mix",
                    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 30.0f),
                std::make_unique<juce::AudioParameterChoice>("roomType", "Room Type",
                    juce::StringArray{"Small", "Medium", "Large", "Extra Large"}, 1),
                std::make_unique<juce::AudioParameterBool>("roomEnabled", "Room On", false),
                std::make_unique<juce::AudioParameterFloat>("roomSize", "Room Size",
                    juce::NormalisableRange<float>(0.5f, 2.0f, 0.05f), 1.0f),
                std::make_unique<juce::AudioParameterFloat>("preDelay", "Pre-Delay",
                    juce::NormalisableRange<float>(0.0f, 50.0f, 1.0f), 20.0f),
                std::make_unique<juce::AudioParameterFloat>("roomDamp", "Damping",
                    juce::NormalisableRange<float>(2000.0f, 20000.0f, 100.0f), 7000.0f),
                std::make_unique<juce::AudioParameterFloat>("erLevel", "ER Level",
                    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f),
                std::make_unique<juce::AudioParameterBool>("calEnabled", "HP Cal", true),
                std::make_unique<juce::AudioParameterFloat>("calGain", "Cal Gain",
                    juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f),
                std::make_unique<juce::AudioParameterChoice>("calProfile", "HP Profile",
                    juce::StringArray{
                        "Beyerdynamic DT 770 Pro",
                        "Beyerdynamic DT 990 Pro",
                        "Sennheiser HD 600",
                        "Sennheiser HD 650",
                        "Audio-Technica ATH-M50x",
                        "AKG K701",
                        "AKG K702",
                        "Sony MDR-7506",
                        "Shure SRH840",
                        "Beyerdynamic DT 880",
                        "Audio-Technica ATH-M20x"
                    }, 0),
            })
{
    setSize(windowW, windowH);

    // ---- Header ----
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,  juce::Colour(40, 170, 80));
    bypassButton.setColour(juce::TextButton::buttonColourId,    juce::Colour(34, 34, 38));
    bypassButton.setColour(juce::TextButton::textColourOnId,    juce::Colours::white);
    bypassButton.setColour(juce::TextButton::textColourOffId,   textMid);
    bypassButton.onClick = [this]
    {
        bool on = bypassButton.getToggleState();
        bypassButton.setButtonText(on ? "ACTIVE" : "BYPASS");
        bypassButton.setColour(juce::TextButton::buttonOnColourId,
            on ? juce::Colour(40, 170, 80) : bypassRed);
    };
    addAndMakeVisible(bypassButton);

    titleLabel.setText("OpenMix Room", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, textHi);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // ---- FR Graph ----
    frGraph.setCalibration(&audioProcessor.getHeadphoneCal());
    addAndMakeVisible(frGraph);

    // ---- Left Panel: HEADPHONE CALIBRATION ----
    calSectionLabel.setText("HEADPHONE CALIBRATION", juce::dontSendNotification);
    calSectionLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    calSectionLabel.setColour(juce::Label::textColourId, accent);
    calSectionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(calSectionLabel);

    // Manual population — APVTS ComboBoxAttachment can fail with processor ref
    rebuildCalProfileCombo();
    calProfileCombo.setSelectedId(1);
    calProfileCombo.onChange = [this]
    {
        audioProcessor.setCalProfile(calProfileCombo.getSelectedItemIndex());
    };
    styleCombo(calProfileCombo, accent);
    addAndMakeVisible(calProfileCombo);

    // Import custom profile button
    importProfileButton.setButtonText("+");
    importProfileButton.setColour(juce::TextButton::buttonColourId, accent.withAlpha(0.25f));
    importProfileButton.setColour(juce::TextButton::textColourOffId, accent);
    importProfileButton.setTooltip("Import AutoEq ParametricEQ.txt");
    importProfileButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser>(
            "Import AutoEq PEQ File...",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
            "*.txt");
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result == juce::File{}) return;

            auto r = audioProcessor.importCalProfile(result.getFullPathName());
            if (r.failed())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,
                    "Import Failed",
                    r.getErrorMessage());
                return;
            }

            // Rebuild combo to include the new custom profile
            rebuildCalProfileCombo();
            calProfileCombo.setSelectedId(calProfileCombo.getNumItems()); // select last (new)
        });
    };
    addAndMakeVisible(importProfileButton);

    calToggle.setButtonText("CAL ON");
    calToggle.setClickingTogglesState(true);
    calToggle.setToggleState(true, juce::dontSendNotification);
    calToggle.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.85f));
    calToggle.setColour(juce::TextButton::buttonColourId, comboBg);
    calToggle.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    calToggle.setColour(juce::TextButton::textColourOffId, textMid);
    calToggle.onClick = [this] {
        bool on = calToggle.getToggleState();
        calToggle.setButtonText(on ? "CAL ON" : "CAL OFF");
        audioProcessor.setCalEnabled(on);
    };
    addAndMakeVisible(calToggle);

    calGainLabel.setText("Gain", juce::dontSendNotification);
    calGainLabel.setFont(juce::Font(10.0f));
    calGainLabel.setColour(juce::Label::textColourId, textLo);
    calGainLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(calGainLabel);

    styleSlider(calGainSlider, accent, 100.0f, 0.0f, 100.0f, 1.0f, " %");
    calGainSlider.onValueChange = [this] {
        audioProcessor.setCalGain(static_cast<float>(calGainSlider.getValue()));
    };
    addAndMakeVisible(calGainSlider);

    // ---- Right Panel: VIRTUAL MONITORING ----
    vmSectionLabel.setText("VIRTUAL MONITORING", juce::dontSendNotification);
    vmSectionLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    vmSectionLabel.setColour(juce::Label::textColourId, accent);
    vmSectionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(vmSectionLabel);

    roomToggle.setButtonText("ROOM OFF");
    roomToggle.setClickingTogglesState(true);
    roomToggle.setToggleState(false, juce::dontSendNotification);
    roomToggle.setColour(juce::TextButton::buttonOnColourId, accentDim.withAlpha(0.85f));
    roomToggle.setColour(juce::TextButton::buttonColourId, comboBg);
    roomToggle.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    roomToggle.setColour(juce::TextButton::textColourOffId, textMid);
    roomToggle.onClick = [this] {
        bool on = roomToggle.getToggleState();
        roomToggle.setButtonText(on ? "ROOM ON" : "ROOM OFF");
        audioProcessor.setRoomEnabled(on);
    };
    addAndMakeVisible(roomToggle);

    roomTypeCombo.addItemList({"Small", "Medium", "Large", "Extra Large"}, 1);
    roomTypeCombo.setSelectedId(2);
    styleCombo(roomTypeCombo, accentDim);
    roomTypeCombo.onChange = [this] {
        audioProcessor.setRoomType(roomTypeCombo.getSelectedItemIndex());
    };
    addAndMakeVisible(roomTypeCombo);

    roomSizeLabel.setText("Size", juce::dontSendNotification);
    roomSizeLabel.setFont(juce::Font(10.0f));
    roomSizeLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(roomSizeLabel);
    styleSlider(roomSizeSlider, accentDim, 1.0f, 0.5f, 2.0f, 0.05f, "x");
    roomSizeSlider.onValueChange = [this] {
        audioProcessor.setRoomSize(static_cast<float>(roomSizeSlider.getValue()));
    };
    addAndMakeVisible(roomSizeSlider);

    preDelayLabel.setText("Pre-Delay", juce::dontSendNotification);
    preDelayLabel.setFont(juce::Font(10.0f));
    preDelayLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(preDelayLabel);
    styleSlider(preDelaySlider, accentDim, 20.0f, 0.0f, 50.0f, 1.0f, " ms");
    preDelaySlider.onValueChange = [this] {
        audioProcessor.setPreDelay(static_cast<float>(preDelaySlider.getValue()));
    };
    addAndMakeVisible(preDelaySlider);

    erLevelLabel.setText("ER Level", juce::dontSendNotification);
    erLevelLabel.setFont(juce::Font(10.0f));
    erLevelLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(erLevelLabel);
    styleSlider(erLevelSlider, accentDim, 50.0f, 0.0f, 100.0f, 1.0f, " %");
    erLevelSlider.onValueChange = [this] {
        audioProcessor.setERLevel(static_cast<float>(erLevelSlider.getValue()));
    };
    addAndMakeVisible(erLevelSlider);

    roomDampLabel.setText("Damping", juce::dontSendNotification);
    roomDampLabel.setFont(juce::Font(10.0f));
    roomDampLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(roomDampLabel);
    styleSlider(roomDampSlider, accentDim, 7000.0f, 2000.0f, 20000.0f, 100.0f, " Hz");
    roomDampSlider.onValueChange = [this] {
        audioProcessor.setRoomDamp(static_cast<float>(roomDampSlider.getValue()));
    };
    addAndMakeVisible(roomDampSlider);

    roomMixLabel.setText("Room Mix", juce::dontSendNotification);
    roomMixLabel.setFont(juce::Font(10.0f));
    roomMixLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(roomMixLabel);
    styleSlider(roomMixSlider, accentDim, 30.0f, 0.0f, 100.0f, 1.0f, " %");
    roomMixSlider.onValueChange = [this] {
        audioProcessor.setRoomMix(static_cast<float>(roomMixSlider.getValue()));
    };
    addAndMakeVisible(roomMixSlider);

    crossfeedLabel.setText("Crossfeed", juce::dontSendNotification);
    crossfeedLabel.setFont(juce::Font(10.0f));
    crossfeedLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(crossfeedLabel);
    styleSlider(crossfeedSlider, accentDim, 50.0f, 0.0f, 100.0f, 1.0f, " %");
    addAndMakeVisible(crossfeedSlider);

    cutoffLabel.setText("Cutoff", juce::dontSendNotification);
    cutoffLabel.setFont(juce::Font(10.0f));
    cutoffLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(cutoffLabel);
    styleSlider(cutoffSlider, juce::Colour(160, 195, 120), 700.0f, 100.0f, 2000.0f, 1.0f, " Hz");
    addAndMakeVisible(cutoffSlider);

    algorithmCombo.addItemList({"Bauer", "Meier", "Chu Moy", "HRTF"}, 1);
    algorithmCombo.setSelectedId(1);
    styleCombo(algorithmCombo, accentDim);
    addAndMakeVisible(algorithmCombo);

    mixLabel.setText("Total Mix", juce::dontSendNotification);
    mixLabel.setFont(juce::Font(10.0f));
    mixLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(mixLabel);
    styleSlider(mixSlider, juce::Colour(140, 185, 235), 100.0f, 0.0f, 100.0f, 1.0f, " %");
    addAndMakeVisible(mixSlider);

    // ---- Room Visualizer ----
    addAndMakeVisible(roomResponseGraph);

    // ---- Status bar ----
    statusLabel.setFont(juce::Font(10.0f));
    statusLabel.setColour(juce::Label::textColourId, textLo);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // ---- APVTS attachments ----
    bypassA.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, "bypass", bypassButton));
    calEnA.reset  (new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, "calEnabled", calToggle));
    calGainA.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "calGain", calGainSlider));
    mixA.reset    (new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "mix", mixSlider));
    xfA.reset     (new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "crossfeed", crossfeedSlider));
    cutA.reset    (new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "cutoff", cutoffSlider));
    roomMixA.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "roomMix", roomMixSlider));
    roomSizeA.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "roomSize", roomSizeSlider));
    preDelayA.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "preDelay", preDelaySlider));
    erLevelA.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "erLevel", erLevelSlider));
    roomDampA.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, "roomDamp", roomDampSlider));
    roomTypeA.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, "roomType", roomTypeCombo));
    roomEnA.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, "roomEnabled", roomToggle));
    algA.reset    (new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, "algorithm", algorithmCombo));

    bypassButton.setToggleState(false, juce::dontSendNotification);
    bypassButton.setButtonText("BYPASS");

    startTimerHz(30);
}

// ==============================================================================
// paint — Waves Nx: radial gradient dark bg, rounded graph panel, orange glow
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Radial gradient — darker at edges, subtle warmth in center
    juce::ColourGradient radial(
        bgPanel, b.getCentreX(), b.getCentreY(),
        bgBase,  b.getWidth() * 0.7f, b.getHeight() * 0.6f, true);
    g.setGradientFill(radial);
    g.fillAll();

    // Graph panel — rounded rect with subtle border
    auto gr = graphRect.toFloat();
    g.setColour(comboBg);
    g.fillRoundedRectangle(gr, 8.0f);
    g.setColour(borderDim);
    g.drawRoundedRectangle(gr, 8.0f, 1.0f);

    // Subtle orange glow behind graph (top edge)
    juce::ColourGradient glow(
        accent.withAlpha(0.04f), gr.getX(), gr.getY(),
        juce::Colours::transparentBlack, gr.getX(), gr.getY() + 80.0f, false);
    g.setGradientFill(glow);
    g.fillRoundedRectangle(gr, 8.0f);

    // Header separator
    g.setColour(borderDim);
    g.drawHorizontalLine(headerRect.getBottom(), 0.0f, static_cast<float>(b.getWidth()));

    // Status bar separator
    g.drawHorizontalLine(statusRect.getY(), 0.0f, static_cast<float>(b.getWidth()));

    // Left panel label underline
    g.setColour(accent.withAlpha(0.3f));
    g.drawHorizontalLine(calSectionLabel.getBottom() + 2,
                         static_cast<float>(leftPanelRect.getX()),
                         static_cast<float>(leftPanelRect.getRight()));

    // Right panel label underline
    g.drawHorizontalLine(vmSectionLabel.getBottom() + 2,
                         static_cast<float>(rightPanelRect.getX()),
                         static_cast<float>(rightPanelRect.getRight()));

    // ---- Draw graph tick values (log freq labels) ----
    g.setColour(textLo.withAlpha(0.6f));
    g.setFont(juce::Font(9.0f));
    const int gh   = graphRect.getHeight();
    const int gy   = graphRect.getBottom();
    const float gx = static_cast<float>(graphRect.getX()) + 8.0f;
    const float gw = static_cast<float>(graphRect.getWidth()) - 16.0f;
    const float hz[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const float hzMin = std::log10(20.0f);
    const float hzMax = std::log10(20000.0f);
    for (float f : hz)
    {
        float nx = (std::log10(f) - hzMin) / (hzMax - hzMin);
        float px = gx + nx * gw;
        if (f >= 1000)
            g.drawText(juce::String(static_cast<int>(f / 1000)) + "k",
                       juce::Rectangle<int>(static_cast<int>(px - 20), gy - 14, 40, 12),
                       juce::Justification::centred, false);
        else
            g.drawText(juce::String(static_cast<int>(f)),
                       juce::Rectangle<int>(static_cast<int>(px - 16), gy - 14, 32, 12),
                       juce::Justification::centred, false);
    }

    // 0 dB reference label
    g.drawText("0 dB", juce::Rectangle<int>(graphRect.getX(), graphRect.getY() + gh / 2 - 8, 36, 14),
               juce::Justification::centredRight, false);
}

// ==============================================================================
// resized — left panel | graph | right panel
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();

    const int pad = 10;

    // Header
    headerRect = b.removeFromTop(42);
    bypassButton.setBounds(headerRect.removeFromLeft(68).reduced(6, 7));
    titleLabel.setBounds(headerRect);

    // Status bar
    statusRect = b.removeFromBottom(22);
    statusLabel.setBounds(statusRect);

    // Content
    auto content = b.reduced(pad, pad);

    leftPanelRect  = content.removeFromLeft(190).reduced(0, 4);
    rightPanelRect = content.removeFromRight(190).reduced(0, 4);
    graphRect      = content.reduced(12, 4);

    // ---- LEFT PANEL ----
    auto lp = leftPanelRect.reduced(6, 0);
    calSectionLabel.setBounds(lp.removeFromTop(20));
    lp.removeFromTop(8);
    auto comboRow = lp.removeFromTop(26);
    importProfileButton.setBounds(comboRow.removeFromRight(26));
    calProfileCombo.setBounds(comboRow);
    lp.removeFromTop(8);
    calToggle.setBounds(lp.removeFromTop(24));
    lp.removeFromTop(12);

    auto gainRow = lp.removeFromTop(24);
    calGainLabel.setBounds(gainRow.removeFromLeft(32));
    calGainSlider.setBounds(gainRow);

    // ---- RIGHT PANEL ----
    auto rp = rightPanelRect.reduced(6, 0);
    vmSectionLabel.setBounds(rp.removeFromTop(20));
    rp.removeFromTop(6);
    roomToggle.setBounds(rp.removeFromTop(24));
    rp.removeFromTop(6);
    roomTypeCombo.setBounds(rp.removeFromTop(26));
    rp.removeFromTop(4);

    auto rSize = rp.removeFromTop(22);
    roomSizeLabel.setBounds(rSize.removeFromLeft(56));
    roomSizeSlider.setBounds(rSize);
    rp.removeFromTop(4);

    auto rPD = rp.removeFromTop(22);
    preDelayLabel.setBounds(rPD.removeFromLeft(56));
    preDelaySlider.setBounds(rPD);
    rp.removeFromTop(4);

    auto rER = rp.removeFromTop(22);
    erLevelLabel.setBounds(rER.removeFromLeft(56));
    erLevelSlider.setBounds(rER);
    rp.removeFromTop(4);

    auto rDamp = rp.removeFromTop(22);
    roomDampLabel.setBounds(rDamp.removeFromLeft(56));
    roomDampSlider.setBounds(rDamp);
    rp.removeFromTop(6);

    auto rRoomMix = rp.removeFromTop(22);
    roomMixLabel.setBounds(rRoomMix.removeFromLeft(56));
    roomMixSlider.setBounds(rRoomMix);
    rp.removeFromTop(6);

    auto rXf = rp.removeFromTop(22);
    crossfeedLabel.setBounds(rXf.removeFromLeft(56));
    crossfeedSlider.setBounds(rXf);
    rp.removeFromTop(6);

    auto rCut = rp.removeFromTop(22);
    cutoffLabel.setBounds(rCut.removeFromLeft(56));
    cutoffSlider.setBounds(rCut);
    rp.removeFromTop(6);

    algorithmCombo.setBounds(rp.removeFromTop(26));
    rp.removeFromTop(6);

    auto rMix = rp.removeFromTop(22);
    mixLabel.setBounds(rMix.removeFromLeft(56));
    mixSlider.setBounds(rMix);

    // ---- Room Visualizer ----
    roomVisRect = rp;
    roomResponseGraph.setBounds(roomVisRect);

    // ---- GRAPH occupies remaining center ----
    frGraph.setBounds(graphRect);
}

// ==============================================================================
// timerCallback — status bar
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::timerCallback()
{
    const auto sr   = audioProcessor.getCurrentSampleRate();
    const auto blk  = audioProcessor.getCurrentBlockSize();
    const auto sofa = audioProcessor.getSofaLoader().isLoaded() ? "SOFA" : "no SOFA";
    const auto cal  = audioProcessor.getHeadphoneCal().isEnabled()
        ? audioProcessor.getHeadphoneCal().getProfile(audioProcessor.getHeadphoneCal().getCurrentProfile()).name
        : "Cal Off";

    statusLabel.setText(
        juce::String(static_cast<int>(std::round(sr))) + " Hz  |  "
        + juce::String(blk) + " samples  |  "
        + sofa + "  |  "
        + cal,
        juce::dontSendNotification);

    // Keep FR graph live — reflects profile / gain changes in real time
    frGraph.recalcMagnitudes();
    frGraph.repaint();

    // Room decay visualization — live RT60 / damp / size
    auto& roomProc = audioProcessor.getRoomProcessor();
    roomResponseGraph.setRoomParams(
        roomProc.getCurrentRt60(),
        roomProc.getCurrentDampLp(),
        roomProc.getCurrentSize());
    roomResponseGraph.repaint();
}

// ==============================================================================
// rebuildCalProfileCombo — repopulate from built-in + custom profiles
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::rebuildCalProfileCombo()
{
    calProfileCombo.clear();
    const auto& hc = audioProcessor.getHeadphoneCal();
    for (int i = 0; i < hc.getProfileCount(); ++i)
        calProfileCombo.addItem(hc.getProfileName(i), i + 1);

    int cur = hc.getCurrentProfile();
    if (cur >= 0 && cur < hc.getProfileCount())
        calProfileCombo.setSelectedId(cur + 1, juce::dontSendNotification);
}
