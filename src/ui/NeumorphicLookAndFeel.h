#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ==============================================================================
// NeumorphicLookAndFeel — soft UI (neumorphism) style for OpenMix Room.
//
// Color palette:
//   bg         = #e4e6ec  (page background)
//   card       = #eef0f5  (raised card surface)
//   cardInset  = #dde0e6  (inset / pressed surface)
//   text       = #2a2d36  (primary text)
//   textDim    = #7a7d86  (secondary text)
//   accent     = #5b7db5  (primary accent, blue-grey)
//   accentDim  = #4a6a9e  (darker accent)
//   highlight  = #ffffff  (light source for raised effect)
//   shadow     = #a0a5b0  (dark shadow for raised effect)
// ==============================================================================
class NeumorphicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeumorphicLookAndFeel();
    ~NeumorphicLookAndFeel() override = default;

    // ---- Buttons ----
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    // ---- Sliders ----
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    void drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle style,
                               juce::Slider& slider) override;

    void drawLinearSliderBackground(juce::Graphics& g, int x, int y, int width, int height,
                                    float sliderPos, float minSliderPos, float maxSliderPos,
                                    juce::Slider::SliderStyle style,
                                    juce::Slider& slider) override;

    // ---- ComboBox ----
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    // ---- Labels (for section headers etc.) ----
    juce::Font getLabelFont(juce::Label& label) override;

    // ---- Shared drawing helpers ----
    static void drawNeumorphicRaised(juce::Graphics& g, juce::Rectangle<float> bounds,
                                     float cornerSize, bool isPressed);
    static void drawNeumorphicInset(juce::Graphics& g, juce::Rectangle<float> bounds,
                                    float cornerSize);
    static void drawNeumorphicCard(juce::Graphics& g, juce::Rectangle<float> bounds,
                                   float cornerSize);

    // ---- Colors (public for PluginEditor reuse) ----
    static const juce::Colour bgColor;
    static const juce::Colour cardColor;
    static const juce::Colour insetColor;
    static const juce::Colour textColor;
    static const juce::Colour textDimColor;
    static const juce::Colour accentColor;
    static const juce::Colour accentDimColor;
    static const juce::Colour highlightColor;
    static const juce::Colour shadowColor;

private:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override;

    static float mapFloat(float v, float lo1, float hi1, float lo2, float hi2);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeumorphicLookAndFeel)
};
