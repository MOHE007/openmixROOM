#include "LogoComponent.h"
#include "NeumorphicLookAndFeel.h"

LogoComponent::LogoComponent() {}

void LogoComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    if (b.getWidth() < 10.0f || b.getHeight() < 10.0f)
        return;

    // Neumorphic inset background
    NeumorphicLookAndFeel::drawNeumorphicInset(g, b, 12.0f);

    // Calculate logo area — centered square in the available space
    const float side = juce::jmin(b.getWidth(), b.getHeight()) * 0.48f;
    const float cx = b.getCentreX();
    const float cy = b.getCentreY();
    const float half = side * 0.5f;

    // ---- Outer rounded square (monitor frame) ----
    juce::Path frame;
    const float corner = side * 0.12f;
    const float inset  = side * 0.08f;
    frame.addRoundedRectangle(cx - half + inset, cy - half + inset,
                              side - inset * 2.0f, side - inset * 2.0f, corner);

    g.setColour(juce::Colour(91, 125, 181));  // accent blue
    g.strokePath(frame, juce::PathStrokeType(2.0f));

    // ---- Inner triangle (play arrow / speaker) ----
    juce::Path arrow;
    const float triH = side * 0.22f;
    const float triW = side * 0.18f;
    const float triX = cx - side * 0.1f;
    const float triY = cy;
    arrow.addTriangle(triX - triW * 0.5f, triY - triH,
                      triX - triW * 0.5f, triY + triH,
                      triX + triW,          triY);
    g.setColour(juce::Colour(91, 125, 181));
    g.fillPath(arrow);

    // ---- Sound wave arcs (right side of arrow) ----
    const float arcBaseX = triX + triW + side * 0.08f;
    const float arcRadii[] = { side * 0.1f, side * 0.17f, side * 0.24f };
    for (float r : arcRadii)
    {
        juce::Path arc;
        const float angle = juce::MathConstants<float>::pi / 4.0f;  // ±45 degrees
        arc.addCentredArc(arcBaseX, cy, r, r, 0.0f, -angle, angle, true);
        g.setColour(juce::Colour(91, 125, 181).withAlpha(0.7f));
        g.strokePath(arc, juce::PathStrokeType(2.2f));
    }

    // ---- "OpenMix" text below the icon ----
    g.setColour(NeumorphicLookAndFeel::textColor);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("OpenMix", juce::Rectangle<float>(b.getX(), cy + half - side * 0.04f,
                                                   b.getWidth(), side * 0.20f),
               juce::Justification::centred, false);
}
