#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ==============================================================================
// LogoComponent — draws OpenMix icon using native juce::Path geometry.
// Zero external file dependencies — no SVG, no bundle Resources.
// ==============================================================================
class LogoComponent : public juce::Component
{
public:
    LogoComponent();
    ~LogoComponent() override = default;
    void paint(juce::Graphics& g) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogoComponent)
};
