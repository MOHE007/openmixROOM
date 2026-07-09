#include "NeumorphicLookAndFeel.h"

// ==============================================================================
// Static color definitions
// ==============================================================================
const juce::Colour NeumorphicLookAndFeel::bgColor        (228, 230, 236);
const juce::Colour NeumorphicLookAndFeel::cardColor       (238, 240, 245);
const juce::Colour NeumorphicLookAndFeel::insetColor      (221, 224, 230);
const juce::Colour NeumorphicLookAndFeel::textColor       (42, 45, 54);
const juce::Colour NeumorphicLookAndFeel::textDimColor    (122, 125, 134);
const juce::Colour NeumorphicLookAndFeel::accentColor     (91, 125, 181);
const juce::Colour NeumorphicLookAndFeel::accentDimColor  (74, 106, 158);
const juce::Colour NeumorphicLookAndFeel::highlightColor  (255, 255, 255);
const juce::Colour NeumorphicLookAndFeel::shadowColor     (160, 165, 176);

// ==============================================================================
// Constructor
// NOTE: removed setDefaultSansSerifTypeface(createSystemTypefaceFor(...))
// which crashes on macOS 26.x / DAW hosts when FontOptions typeface is null.
// ==============================================================================
NeumorphicLookAndFeel::NeumorphicLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId,  bgColor);
    setColour(juce::TextButton::buttonColourId,           cardColor);
    setColour(juce::TextButton::buttonOnColourId,         accentColor);
    setColour(juce::TextButton::textColourOffId,          textColor);
    setColour(juce::TextButton::textColourOnId,           juce::Colours::white);
    setColour(juce::ComboBox::backgroundColourId,         cardColor);
    setColour(juce::ComboBox::textColourId,               textColor);
    setColour(juce::ComboBox::outlineColourId,            juce::Colours::transparentBlack);
    setColour(juce::ComboBox::arrowColourId,              accentColor);
    setColour(juce::ComboBox::focusedOutlineColourId,     accentColor.withAlpha(0.4f));
    setColour(juce::Label::textColourId,                  textColor);
    setColour(juce::Slider::thumbColourId,                accentColor);
    setColour(juce::Slider::trackColourId,                accentColor);
    setColour(juce::Slider::backgroundColourId,           insetColor);
    setColour(juce::Slider::textBoxTextColourId,          textColor);
    setColour(juce::Slider::textBoxBackgroundColourId,    cardColor);
    setColour(juce::Slider::textBoxOutlineColourId,       juce::Colours::transparentBlack);
    setColour(juce::PopupMenu::backgroundColourId,        cardColor);
    setColour(juce::PopupMenu::textColourId,              textColor);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentColor.withAlpha(0.2f));
    setColour(juce::PopupMenu::highlightedTextColourId,   accentColor);
}

float NeumorphicLookAndFeel::mapFloat(float v, float lo1, float hi1, float lo2, float hi2)
{
    return lo2 + (hi2 - lo2) * ((v - lo1) / (hi1 - lo1));
}

// ==============================================================================
// Shared drawing helpers
// ==============================================================================
void NeumorphicLookAndFeel::drawNeumorphicRaised(juce::Graphics& g,
                                                  juce::Rectangle<float> bounds,
                                                  float cornerSize, bool isPressed)
{
    if (isPressed)
    {
        g.setColour(insetColor);
        g.fillRoundedRectangle(bounds, cornerSize);
        auto innerRect = bounds.reduced(1.5f);
        g.setColour(shadowColor.withAlpha(0.35f));
        g.drawRoundedRectangle(innerRect, cornerSize - 0.5f, 1.0f);
        auto tl = bounds.reduced(2.0f);
        g.setColour(highlightColor.withAlpha(0.5f));
        g.drawRoundedRectangle(tl, cornerSize - 1.0f, 1.0f);
    }
    else
    {
        g.setColour(shadowColor.withAlpha(0.7f));
        g.fillRoundedRectangle(bounds.translated(2.0f, 2.5f), cornerSize);
        g.setColour(highlightColor.withAlpha(0.9f));
        g.fillRoundedRectangle(bounds.translated(-1.5f, -2.0f), cornerSize);
        g.setColour(cardColor);
        g.fillRoundedRectangle(bounds, cornerSize);
    }
}

void NeumorphicLookAndFeel::drawNeumorphicInset(juce::Graphics& g,
                                                 juce::Rectangle<float> bounds,
                                                 float cornerSize)
{
    g.setColour(shadowColor.withAlpha(0.4f));
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(highlightColor.withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.reduced(1.0f, 1.0f), cornerSize - 0.5f);
    g.setColour(insetColor);
    g.fillRoundedRectangle(bounds.reduced(2.0f, 2.0f), cornerSize - 1.0f);
}

void NeumorphicLookAndFeel::drawNeumorphicCard(juce::Graphics& g,
                                                juce::Rectangle<float> bounds,
                                                float cornerSize)
{
    g.setColour(shadowColor.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds.translated(3.0f, 4.0f), cornerSize);
    g.setColour(highlightColor.withAlpha(0.8f));
    g.fillRoundedRectangle(bounds.translated(-2.0f, -3.0f), cornerSize);
    g.setColour(cardColor);
    g.fillRoundedRectangle(bounds, cornerSize);
}

// ==============================================================================
// drawButtonBackground
// ==============================================================================
void NeumorphicLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                  juce::Button& button,
                                                  const juce::Colour& /*backgroundColour*/,
                                                  bool isHighlighted,
                                                  bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.5f);
    const float corner = 8.0f;
    if (button.getToggleState())
    {
        g.setColour(accentColor.darker(0.15f));
        g.fillRoundedRectangle(bounds.translated(2.0f, 2.5f), corner);
        g.setColour(accentColor.brighter(0.2f).withAlpha(0.6f));
        g.fillRoundedRectangle(bounds.translated(-1.5f, -2.0f), corner);
        g.setColour(accentColor);
        g.fillRoundedRectangle(bounds, corner);
    }
    else
    {
        drawNeumorphicRaised(g, bounds, corner, isDown || isHighlighted);
        if (isHighlighted && !isDown)
        {
            g.setColour(accentColor.withAlpha(0.15f));
            g.fillRoundedRectangle(bounds, corner);
        }
    }
}

void NeumorphicLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                            bool /*isHighlighted*/, bool /*isDown*/)
{
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(button.getToggleState() ? juce::Colours::white : textColor);
    g.drawText(button.getButtonText(), button.getLocalBounds().toFloat(),
               juce::Justification::centred, false);
}

void NeumorphicLookAndFeel::drawLinearSliderBackground(juce::Graphics& g,
                                                        int x, int y, int width, int height,
                                                        float /*sliderPos*/,
                                                        float /*minSliderPos*/,
                                                        float /*maxSliderPos*/,
                                                        juce::Slider::SliderStyle /*style*/,
                                                        juce::Slider& /*slider*/)
{
    const float trackH = 6.0f;
    const float corner = 3.0f;
    float trackY = y + (height - trackH) * 0.5f;
    auto trackBounds = juce::Rectangle<float>(static_cast<float>(x), trackY,
                                              static_cast<float>(width), trackH);
    drawNeumorphicInset(g, trackBounds, corner);
}

void NeumorphicLookAndFeel::drawLinearSliderThumb(juce::Graphics& g,
                                                   int x, int y, int width, int height,
                                                   float sliderPos,
                                                   float /*minSliderPos*/,
                                                   float /*maxSliderPos*/,
                                                   juce::Slider::SliderStyle style,
                                                   juce::Slider& /*slider*/)
{
    if (style != juce::Slider::LinearHorizontal && style != juce::Slider::LinearVertical)
        return;
    const float thumbSize = 16.0f;
    float cx = (style == juce::Slider::LinearHorizontal) ? sliderPos : x + width * 0.5f;
    float cy = (style == juce::Slider::LinearHorizontal) ? y + height * 0.5f : sliderPos;
    auto thumbBounds = juce::Rectangle<float>(thumbSize, thumbSize).withCentre({cx, cy});
    drawNeumorphicRaised(g, thumbBounds, thumbSize * 0.5f, false);
    g.setColour(accentColor.withAlpha(0.5f));
    g.fillEllipse(thumbBounds.reduced(4.0f));
}

void NeumorphicLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                              int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos,
                                              float maxSliderPos,
                                              juce::Slider::SliderStyle style,
                                              juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearVertical)
    {
        drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos,
                                    maxSliderPos, style, slider);
        drawLinearSliderThumb(g, x, y, width, height, sliderPos, minSliderPos,
                               maxSliderPos, style, slider);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos,
                                          maxSliderPos, style, slider);
    }
}

void NeumorphicLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                          bool isButtonDown,
                                          int /*buttonX*/, int /*buttonY*/,
                                          int /*buttonW*/, int /*buttonH*/,
                                          juce::ComboBox& /*box*/)
{
    auto bounds = juce::Rectangle<float>(2.0f, 2.0f,
                                         static_cast<float>(width) - 4.0f,
                                         static_cast<float>(height) - 4.0f);
    drawNeumorphicRaised(g, bounds, 8.0f, isButtonDown);
    const float arrowX = static_cast<float>(width) - 18.0f;
    const float arrowY = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowX, arrowY - 3.0f, arrowX + 8.0f, arrowY - 3.0f, arrowX + 4.0f, arrowY + 3.0f);
    g.setColour(accentColor);
    g.fillPath(arrow);
}

void NeumorphicLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont(juce::Font(11.0f));
}

juce::Font NeumorphicLookAndFeel::getLabelFont(juce::Label& label)
{
    auto f = label.getFont();
    return f.withHeight(f.getHeight());
}

void NeumorphicLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                              int x, int y, int width, int height,
                                              float sliderPos, float startAngle,
                                              float endAngle, juce::Slider& slider)
{
    LookAndFeel_V4::drawRotarySlider(g, x, y, width, height, sliderPos,
                                      startAngle, endAngle, slider);
}
