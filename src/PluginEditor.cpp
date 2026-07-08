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
                    juce::StringArray{"Small", "Medium", "Large"}, 1),
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

    // ComboBox populated by APVTS attachment — no manual addItem needed
    styleCombo(calProfileCombo, accent);
    addAndMakeVisible(calProfileCombo);

    calToggle.setButtonText("CAL ON");
    calToggle.setClickingTogglesState(true);
    calToggle.setToggleState(true, juce::dontSendNotification);
    calToggle.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.85f));
    calToggle.setColour(juce::TextButton::buttonColourId, comboBg);
    calToggle.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    calToggle.setColour(juce::TextButton::textColourOffId, textMid);
    calToggle.onClick = [this] {
        calToggle.setButtonText(calToggle.getToggleState() ? "CAL ON" : "CAL OFF");
    };
    addAndMakeVisible(calToggle);

    calGainLabel.setText("Gain", juce::dontSendNotification);
    calGainLabel.setFont(juce::Font(10.0f));
    calGainLabel.setColour(juce::Label::textColourId, textLo);
    calGainLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(calGainLabel);

    styleSlider(calGainSlider, accent, 100.0f, 0.0f, 100.0f, 1.0f, " %");
    addAndMakeVisible(calGainSlider);

    // ---- Right Panel: VIRTUAL MONITORING ----
    vmSectionLabel.setText("VIRTUAL MONITORING", juce::dontSendNotification);
    vmSectionLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    vmSectionLabel.setColour(juce::Label::textColourId, accent);
    vmSectionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(vmSectionLabel);

    roomTypeCombo.addItemList({"Small", "Medium", "Large"}, 1);
    roomTypeCombo.setSelectedId(2);
    styleCombo(roomTypeCombo, accentDim);
    addAndMakeVisible(roomTypeCombo);

    roomMixLabel.setText("Room Mix", juce::dontSendNotification);
    roomMixLabel.setFont(juce::Font(10.0f));
    roomMixLabel.setColour(juce::Label::textColourId, textLo);
    addAndMakeVisible(roomMixLabel);
    styleSlider(roomMixSlider, accentDim, 30.0f, 0.0f, 100.0f, 1.0f, " %");
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
    calProfA.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, "calProfile", calProfileCombo));
    roomTypeA.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, "roomType", roomTypeCombo));
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
    calProfileCombo.setBounds(lp.removeFromTop(26));
    lp.removeFromTop(8);
    calToggle.setBounds(lp.removeFromTop(24));
    lp.removeFromTop(12);

    auto gainRow = lp.removeFromTop(24);
    calGainLabel.setBounds(gainRow.removeFromLeft(32));
    calGainSlider.setBounds(gainRow);

    // ---- RIGHT PANEL ----
    auto rp = rightPanelRect.reduced(6, 0);
    vmSectionLabel.setBounds(rp.removeFromTop(20));
    rp.removeFromTop(8);
    roomTypeCombo.setBounds(rp.removeFromTop(26));
    rp.removeFromTop(12);

    auto rRoomMix = rp.removeFromTop(22);
    roomMixLabel.setBounds(rRoomMix.removeFromLeft(56));
    roomMixSlider.setBounds(rRoomMix);
    rp.removeFromTop(8);

    auto rXf = rp.removeFromTop(22);
    crossfeedLabel.setBounds(rXf.removeFromLeft(56));
    crossfeedSlider.setBounds(rXf);
    rp.removeFromTop(8);

    auto rCut = rp.removeFromTop(22);
    cutoffLabel.setBounds(rCut.removeFromLeft(56));
    cutoffSlider.setBounds(rCut);
    rp.removeFromTop(12);

    algorithmCombo.setBounds(rp.removeFromTop(26));
    rp.removeFromTop(12);

    auto rMix = rp.removeFromTop(22);
    mixLabel.setBounds(rMix.removeFromLeft(56));
    mixSlider.setBounds(rMix);

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
}
