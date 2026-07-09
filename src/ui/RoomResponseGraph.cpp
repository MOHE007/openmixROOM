#include "RoomResponseGraph.h"
#include "NeumorphicLookAndFeel.h"

// Neumorphic palette alias
using LF = NeumorphicLookAndFeel;
static const juce::Colour hfColour  (100, 160, 210); // blue-ish for HF curve

// ==============================================================================
// setRoomParams
// ==============================================================================
void RoomResponseGraph::setRoomParams(float rt, float damp, float sz)
{
    rt60     = rt;
    dampLpHz = damp;
    size     = sz;
}

// ==============================================================================
// paint
// ==============================================================================
void RoomResponseGraph::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().reduced(2, 4);
    if (b.isEmpty()) return;

    // Neumorphic inset card
    LF::drawNeumorphicInset(g, b.toFloat(), static_cast<float>(cornerSize));

    drawGrid(g, b);
    drawDecayCurve(g, b);

    // Labels
    g.setColour(LF::textDimColor);
    g.setFont(juce::FontOptions(9.0f));
    g.drawText(juce::String::formatted("RT60 = %.1fs", rt60),
               b.reduced(6, 4).removeFromTop(14).removeFromRight(80),
               juce::Justification::centredRight, false);

    g.drawText(juce::String::formatted("LP @ %.0fHz", dampLpHz),
               juce::Rectangle<int>(b.getX() + 6, b.getY() + 4, 120, 14),
               juce::Justification::centredLeft, false);
}

// ==============================================================================
// drawGrid — dB scale on Y, time on X
// ==============================================================================
void RoomResponseGraph::drawGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    const int leftMargin   = 36;
    const int rightMargin  = 8;
    const int topMargin    = 18;
    const int bottomMargin = 14;

    auto plotArea = area.withTrimmedLeft(leftMargin).withTrimmedTop(topMargin)
                         .withTrimmedRight(rightMargin).withTrimmedBottom(bottomMargin);
    if (plotArea.isEmpty()) return;

    float plotH = static_cast<float>(plotArea.getHeight());
    float plotW = static_cast<float>(plotArea.getWidth());
    float plotX = static_cast<float>(plotArea.getX());
    float plotY = static_cast<float>(plotArea.getY());

    // Y-axis: horizontal dB grid lines
    const float dbSteps[] = { 0.0f, -10.0f, -20.0f, -30.0f, -40.0f, -50.0f, -60.0f };
    g.setFont(juce::FontOptions(8.0f));
    for (float db : dbSteps)
    {
        float norm = (maxDb - db) / (maxDb - minDb); // 0 at top, 1 at bottom
        float y = plotY + norm * plotH;

        g.setColour(LF::shadowColor.withAlpha(0.25f));
        g.drawHorizontalLine(static_cast<int>(y),
                             area.getX() + leftMargin - 4,
                             plotX + plotW + 4);

        g.setColour(LF::textDimColor);
        g.drawText(juce::String(static_cast<int>(db)),
                   juce::Rectangle<int>(area.getX(), static_cast<int>(y) - 6, leftMargin - 4, 12),
                   juce::Justification::centredRight, false);
    }

    // X-axis: time ticks
    if (rt60 <= 0.01f) return;

    float totalMs = rt60 * 1000.0f;
    float tickMs;
    if (totalMs <= 600.0f)
        tickMs = 100.0f;
    else if (totalMs <= 1500.0f)
        tickMs = 250.0f;
    else if (totalMs <= 3000.0f)
        tickMs = 500.0f;
    else
        tickMs = 1000.0f;

    for (float t = 0.0f; t <= totalMs + tickMs * 0.5f; t += tickMs)
    {
        float nx = t / totalMs;
        float x = plotX + nx * plotW;

        g.setColour(LF::shadowColor.withAlpha(0.25f));
        g.drawVerticalLine(static_cast<int>(x), plotY, plotY + plotH);

        g.setColour(LF::textDimColor);
        juce::String label;
        if (totalMs >= 1000.0f)
            label = juce::String::formatted("%.1fs", t / 1000.0f);
        else
            label = juce::String::formatted("%.0fms", t);
        g.drawText(label,
                   juce::Rectangle<int>(static_cast<int>(x) - 20,
                                        plotArea.getBottom() + 2, 40, 12),
                   juce::Justification::centred, false);
    }
}

// ==============================================================================
// drawDecayCurve — full-band (orange) + high-frequency damped (blue)
// ==============================================================================
void RoomResponseGraph::drawDecayCurve(juce::Graphics& g, juce::Rectangle<int> area)
{
    const int leftMargin   = 36;
    const int rightMargin  = 8;
    const int topMargin    = 18;
    const int bottomMargin = 14;

    auto plotArea = area.withTrimmedLeft(leftMargin).withTrimmedTop(topMargin)
                         .withTrimmedRight(rightMargin).withTrimmedBottom(bottomMargin);
    if (plotArea.isEmpty()) return;

    float plotH = static_cast<float>(plotArea.getHeight());
    float plotW = static_cast<float>(plotArea.getWidth());
    float plotX = static_cast<float>(plotArea.getX());
    float plotY = static_cast<float>(plotArea.getY());

    auto dbToY = [&](float db) {
        float norm = (maxDb - db) / (maxDb - minDb);
        return plotY + norm * plotH;
    };

    auto tToX = [&](float t) {
        return plotX + (t / juce::jmax(rt60, 0.01f)) * plotW;
    };

    const int numPts = 128;

    // ---- Full-band decay curve (orange) ----
    {
        juce::Path path;
        juce::Path fillPath;
        path.startNewSubPath(tToX(0.0f), dbToY(0.0f));
        fillPath.startNewSubPath(tToX(0.0f), dbToY(minDb));
        fillPath.lineTo(tToX(0.0f), dbToY(0.0f));

        for (int i = 1; i <= numPts; ++i)
        {
            float t = rt60 * static_cast<float>(i) / static_cast<float>(numPts);
            float db = -60.0f * (t / rt60);
            float x = tToX(t);
            float y = dbToY(db);
            path.lineTo(x, y);
            fillPath.lineTo(x, y);
        }

        fillPath.lineTo(tToX(rt60), dbToY(minDb));
        fillPath.closeSubPath();

        juce::ColourGradient grad(
            LF::accentColor.withAlpha(0.35f), plotX, dbToY(0.0f),
            LF::accentColor.withAlpha(0.05f), plotX, dbToY(-60.0f),
            false);
        g.setGradientFill(grad);
        g.fillPath(fillPath);

        g.setColour(LF::accentColor.withAlpha(0.9f));
        g.strokePath(path, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));
    }

    // ---- High-frequency (damping-affected) decay curve (blue) ----
    {
        float dampFactor = juce::jmin(1.0f, dampLpHz / 20000.0f);
        float hfRt60 = rt60 * dampFactor;

        // Only draw if visibly different from full-band
        if (hfRt60 >= rt60 * 0.95f)
            return;

        juce::Path path;
        juce::Path fillPath;
        path.startNewSubPath(tToX(0.0f), dbToY(0.0f));
        fillPath.startNewSubPath(tToX(0.0f), dbToY(minDb));
        fillPath.lineTo(tToX(0.0f), dbToY(0.0f));

        for (int i = 1; i <= numPts; ++i)
        {
            float t = rt60 * static_cast<float>(i) / static_cast<float>(numPts);
            float db;
            if (t <= hfRt60)
                db = -60.0f * (t / hfRt60);
            else
                db = -60.0f;
            float x = tToX(t);
            float y = dbToY(db);
            path.lineTo(x, y);
            fillPath.lineTo(x, y);
        }

        fillPath.lineTo(tToX(rt60), dbToY(minDb));
        fillPath.closeSubPath();

        juce::ColourGradient grad(
            hfColour.withAlpha(0.25f), plotX, dbToY(0.0f),
            hfColour.withAlpha(0.03f), plotX, dbToY(-60.0f),
            false);
        g.setGradientFill(grad);
        g.fillPath(fillPath);

        g.setColour(hfColour.withAlpha(0.7f));
        g.strokePath(path, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved));
    }
}
