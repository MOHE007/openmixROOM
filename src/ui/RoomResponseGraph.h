#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ==============================================================================
// RoomResponseGraph — energy decay curve (EDC) visualizer for Room panel.
// Draws full-band and high-frequency (damping-affected) decay curves
// in a dark rounded rectangle at the bottom of the right panel.
// ==============================================================================
class RoomResponseGraph : public juce::Component
{
public:
    RoomResponseGraph() = default;

    void setRoomParams(float rt60, float dampLpHz, float size);
    void paint(juce::Graphics& g) override;

private:
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void drawDecayCurve(juce::Graphics& g, juce::Rectangle<int> area);

    float rt60      = 1.8f;
    float dampLpHz  = 7000.0f;
    float size      = 1.0f;

    static constexpr float maxDb      =  0.0f;
    static constexpr float minDb      = -60.0f;
    static constexpr int   cornerSize = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoomResponseGraph)
};
