#include "PluginEditor.h"

// ==============================================================================
// Neumorphic colour aliases
// ==============================================================================
using LF = NeumorphicLookAndFeel;

// ==============================================================================
// styleSlider — neumorphic themed slider
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::styleSlider(juce::Slider& s,
                                                   float v, float lo, float hi, float st,
                                                   const juce::String& sfx)
{
    s.setLookAndFeel(&neumorphicLF);
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 20);
    s.setRange(lo, hi, st);
    s.setValue(v);
    s.setTextValueSuffix(sfx);
    s.setColour(juce::Slider::textBoxTextColourId,     LF::textColor);
    s.setColour(juce::Slider::textBoxBackgroundColourId, LF::cardColor);
    s.setColour(juce::Slider::textBoxOutlineColourId,   juce::Colours::transparentBlack);
}

void OpenMixRoomAudioProcessorEditor::styleCombo(juce::ComboBox& cb)
{
    cb.setLookAndFeel(&neumorphicLF);
    cb.setColour(juce::ComboBox::backgroundColourId, LF::cardColor);
    cb.setColour(juce::ComboBox::textColourId,       LF::textColor);
    cb.setColour(juce::ComboBox::outlineColourId,    juce::Colours::transparentBlack);
    cb.setColour(juce::ComboBox::arrowColourId,      LF::accentColor);
    cb.setColour(juce::ComboBox::focusedOutlineColourId, LF::accentColor.withAlpha(0.4f));
}

// ==============================================================================
// Constructor — no APVTS; processor uses raw addParameter().
// All controls bound via manual onClick/onValueChange callbacks.
// ==============================================================================
OpenMixRoomAudioProcessorEditor::OpenMixRoomAudioProcessorEditor(
    OpenMixRoomAudioProcessor& processor)
    : juce::AudioProcessorEditor(processor)
    , audioProcessor(processor)
{
    setLookAndFeel(&neumorphicLF);
    setSize(windowW, windowH);

    // ---- Header ----
    bypassButton.setLookAndFeel(&neumorphicLF);
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this]
    {
        bool on = bypassButton.getToggleState();
        bypassButton.setButtonText(on ? "ACTIVE" : "BYPASS");
    };
    addAndMakeVisible(bypassButton);

    titleLabel.setText("OpenMix Room", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, LF::textColor);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // ---- FR Graph ----
    frGraph.setCalibration(&audioProcessor.getHeadphoneCal());
    addAndMakeVisible(frGraph);

    // ---- Left Panel: HEADPHONE CALIBRATION ----
    calSectionLabel.setText("HEADPHONE CALIBRATION", juce::dontSendNotification);
    calSectionLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    calSectionLabel.setColour(juce::Label::textColourId, LF::accentColor);
    calSectionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(calSectionLabel);

    rebuildCalProfileCombo();
    calProfileCombo.setLookAndFeel(&neumorphicLF);
    calProfileCombo.setSelectedId(1);
    calProfileCombo.onChange = [this]
    {
        audioProcessor.setCalProfile(calProfileCombo.getSelectedItemIndex());
    };
    styleCombo(calProfileCombo);
    addAndMakeVisible(calProfileCombo);

    importProfileButton.setLookAndFeel(&neumorphicLF);
    importProfileButton.setButtonText("+");
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

            rebuildCalProfileCombo();
            calProfileCombo.setSelectedId(calProfileCombo.getNumItems());
        });
    };
    addAndMakeVisible(importProfileButton);

    calToggle.setLookAndFeel(&neumorphicLF);
    calToggle.setButtonText("CAL ON");
    calToggle.setClickingTogglesState(true);
    calToggle.setToggleState(true, juce::dontSendNotification);
    calToggle.onClick = [this] {
        bool on = calToggle.getToggleState();
        calToggle.setButtonText(on ? "CAL ON" : "CAL OFF");
        audioProcessor.setCalEnabled(on);
    };
    addAndMakeVisible(calToggle);

    calGainLabel.setText("Gain", juce::dontSendNotification);
    calGainLabel.setFont(juce::Font(10.0f));
    calGainLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    calGainLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(calGainLabel);

    styleSlider(calGainSlider, 100.0f, 0.0f, 100.0f, 1.0f, " %");
    calGainSlider.onValueChange = [this] {
        audioProcessor.setCalGain(static_cast<float>(calGainSlider.getValue()));
    };
    addAndMakeVisible(calGainSlider);

    // ---- Logo (left panel, below cal controls) ----
    // Pure juce::Path geometry — zero external file dependencies.
    addAndMakeVisible(logoComponent);

    // ---- Right Panel: VIRTUAL MONITORING ----
    vmSectionLabel.setText("VIRTUAL MONITORING", juce::dontSendNotification);
    vmSectionLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    vmSectionLabel.setColour(juce::Label::textColourId, LF::accentColor);
    vmSectionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(vmSectionLabel);

    roomToggle.setLookAndFeel(&neumorphicLF);
    roomToggle.setButtonText("ROOM OFF");
    roomToggle.setClickingTogglesState(true);
    roomToggle.setToggleState(false, juce::dontSendNotification);
    roomToggle.onClick = [this] {
        bool on = roomToggle.getToggleState();
        roomToggle.setButtonText(on ? "ROOM ON" : "ROOM OFF");
        audioProcessor.setRoomEnabled(on);
    };
    addAndMakeVisible(roomToggle);

    roomTypeCombo.setLookAndFeel(&neumorphicLF);
    roomTypeCombo.addItemList({"Small", "Medium", "Large", "Extra Large"}, 1);
    roomTypeCombo.setSelectedId(2);
    styleCombo(roomTypeCombo);
    roomTypeCombo.onChange = [this] {
        audioProcessor.setRoomType(roomTypeCombo.getSelectedItemIndex());
    };
    addAndMakeVisible(roomTypeCombo);

    roomSizeLabel.setText("Size", juce::dontSendNotification);
    roomSizeLabel.setFont(juce::Font(10.0f));
    roomSizeLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(roomSizeLabel);
    styleSlider(roomSizeSlider, 1.0f, 0.5f, 2.0f, 0.05f, "x");
    roomSizeSlider.onValueChange = [this] {
        audioProcessor.setRoomSize(static_cast<float>(roomSizeSlider.getValue()));
    };
    addAndMakeVisible(roomSizeSlider);

    preDelayLabel.setText("Pre-Delay", juce::dontSendNotification);
    preDelayLabel.setFont(juce::Font(10.0f));
    preDelayLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(preDelayLabel);
    styleSlider(preDelaySlider, 20.0f, 0.0f, 50.0f, 1.0f, " ms");
    preDelaySlider.onValueChange = [this] {
        audioProcessor.setPreDelay(static_cast<float>(preDelaySlider.getValue()));
    };
    addAndMakeVisible(preDelaySlider);

    erLevelLabel.setText("ER Level", juce::dontSendNotification);
    erLevelLabel.setFont(juce::Font(10.0f));
    erLevelLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(erLevelLabel);
    styleSlider(erLevelSlider, 50.0f, 0.0f, 100.0f, 1.0f, " %");
    erLevelSlider.onValueChange = [this] {
        audioProcessor.setERLevel(static_cast<float>(erLevelSlider.getValue()));
    };
    addAndMakeVisible(erLevelSlider);

    roomDampLabel.setText("Damping", juce::dontSendNotification);
    roomDampLabel.setFont(juce::Font(10.0f));
    roomDampLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(roomDampLabel);
    styleSlider(roomDampSlider, 7000.0f, 2000.0f, 20000.0f, 100.0f, " Hz");
    roomDampSlider.onValueChange = [this] {
        audioProcessor.setRoomDamp(static_cast<float>(roomDampSlider.getValue()));
    };
    addAndMakeVisible(roomDampSlider);

    roomMixLabel.setText("Room Mix", juce::dontSendNotification);
    roomMixLabel.setFont(juce::Font(10.0f));
    roomMixLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(roomMixLabel);
    styleSlider(roomMixSlider, 30.0f, 0.0f, 100.0f, 1.0f, " %");
    roomMixSlider.onValueChange = [this] {
        audioProcessor.setRoomMix(static_cast<float>(roomMixSlider.getValue()));
    };
    addAndMakeVisible(roomMixSlider);

    crossfeedLabel.setText("Crossfeed", juce::dontSendNotification);
    crossfeedLabel.setFont(juce::Font(10.0f));
    crossfeedLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(crossfeedLabel);
    styleSlider(crossfeedSlider, 50.0f, 0.0f, 100.0f, 1.0f, " %");
    crossfeedSlider.onValueChange = [this] {
        // crossfeed handled in processBlock via mixParam — no dedicated setter
    };
    addAndMakeVisible(crossfeedSlider);

    cutoffLabel.setText("Cutoff", juce::dontSendNotification);
    cutoffLabel.setFont(juce::Font(10.0f));
    cutoffLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(cutoffLabel);
    styleSlider(cutoffSlider, 700.0f, 100.0f, 2000.0f, 1.0f, " Hz");
    addAndMakeVisible(cutoffSlider);

    algorithmCombo.setLookAndFeel(&neumorphicLF);
    algorithmCombo.addItemList({"Bauer", "Meier", "Chu Moy", "HRTF"}, 1);
    algorithmCombo.setSelectedId(1);
    styleCombo(algorithmCombo);
    addAndMakeVisible(algorithmCombo);

    mixLabel.setText("Total Mix", juce::dontSendNotification);
    mixLabel.setFont(juce::Font(10.0f));
    mixLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    addAndMakeVisible(mixLabel);
    styleSlider(mixSlider, 100.0f, 0.0f, 100.0f, 1.0f, " %");
    addAndMakeVisible(mixSlider);

    // ---- Room Visualizer ----
    addAndMakeVisible(roomResponseGraph);

    // ---- Status bar ----
    statusLabel.setFont(juce::Font(10.0f));
    statusLabel.setColour(juce::Label::textColourId, LF::textDimColor);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    bypassButton.setToggleState(false, juce::dontSendNotification);
    bypassButton.setButtonText("BYPASS");

    startTimerHz(30);
}

// ==============================================================================
// paint — neumorphic flat background, raised cards, subtle separators
// ==============================================================================
void OpenMixRoomAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Flat background
    g.setColour(LF::bgColor);
    g.fillAll();

    // ---- Graph card (raised, large) ----
    {
        auto gr = graphRect.toFloat().expanded(6.0f, 6.0f);
        LF::drawNeumorphicCard(g, gr, 12.0f);
    }

    // ---- Left panel card (raised) ----
    {
        auto lp = leftPanelRect.toFloat().expanded(6.0f, 6.0f);
        LF::drawNeumorphicCard(g, lp, 12.0f);
    }

    // ---- Right panel card (raised) ----
    {
        auto rp = rightPanelRect.toFloat().expanded(6.0f, 6.0f);
        LF::drawNeumorphicCard(g, rp, 12.0f);
    }

    // ---- Header separator ----
    g.setColour(LF::shadowColor.withAlpha(0.3f));
    g.drawHorizontalLine(headerRect.getBottom(), 0.0f, b.getWidth());

    // ---- Status bar separator ----
    g.drawHorizontalLine(statusRect.getY(), 0.0f, b.getWidth());

    // ---- Section label underlines ----
    g.setColour(LF::accentColor.withAlpha(0.3f));
    g.drawHorizontalLine(calSectionLabel.getBottom() + 2,
                         static_cast<float>(leftPanelRect.getX()),
                         static_cast<float>(leftPanelRect.getRight()));
    g.drawHorizontalLine(vmSectionLabel.getBottom() + 2,
                         static_cast<float>(rightPanelRect.getX()),
                         static_cast<float>(rightPanelRect.getRight()));

    // ---- Frequency axis labels (log freq) ----
    g.setColour(LF::textDimColor.withAlpha(0.7f));
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

    // ---- Logo fills remaining left panel space ----
    lp.removeFromTop(8);
    logoComponent.setBounds(lp);

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
// timerCallback — status bar + graph refresh
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

    // Keep FR graph live
    frGraph.recalcMagnitudes();
    frGraph.repaint();

    // Room decay visualization
    auto& roomProc = audioProcessor.getRoomProcessor();
    roomResponseGraph.setRoomParams(
        roomProc.getCurrentRt60(),
        roomProc.getCurrentDampLp(),
        roomProc.getCurrentSize());
    roomResponseGraph.repaint();
}

// ==============================================================================
// rebuildCalProfileCombo
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
