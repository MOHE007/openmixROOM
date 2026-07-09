#include "FrequencyResponseGraph.h"
#include "NeumorphicLookAndFeel.h"
#include <cmath>

FrequencyResponseGraph::FrequencyResponseGraph()
{
    // Precompute log-spaced frequency grid
    const float logMin = std::log10(minFreq);
    const float logMax = std::log10(maxFreq);
    const float step  = (logMax - logMin) / static_cast<float>(numPoints - 1);

    for (int i = 0; i < numPoints; ++i)
    {
        freqs[i]        = std::pow(10.0f, logMin + step * static_cast<float>(i));
        magnitudes[i]   = 0.0f;
    }
}

void FrequencyResponseGraph::setCalibration(const HeadphoneCalibration* cal)
{
    calibration = cal;
    recalcMagnitudes();
    repaint();
}

void FrequencyResponseGraph::recalcMagnitudes()
{
    if (calibration != nullptr)
    {
        for (int i = 0; i < numPoints; ++i)
            magnitudes[i] = calibration->getMagnitudeDB(freqs[i]);
    }
    else
    {
        for (int i = 0; i < numPoints; ++i)
            magnitudes[i] = 0.0f;
    }
}

float FrequencyResponseGraph::freqToX(float freqHz, float plotWidth) const
{
    const float logFreq = std::log10(freqHz);
    const float logMin  = std::log10(minFreq);
    const float logMax  = std::log10(maxFreq);
    return plotWidth * (logFreq - logMin) / (logMax - logMin);
}

void FrequencyResponseGraph::paint(juce::Graphics& g)
{
    using LF = NeumorphicLookAndFeel;

    auto bounds = getLocalBounds().toFloat();
    const float padLeft   = 40.0f;
    const float padRight  = 8.0f;
    const float padTop    = 8.0f;
    const float padBottom = 22.0f;

    const float plotX      = padLeft;
    const float plotY      = padTop;
    const float plotWidth  = bounds.getWidth()  - padLeft - padRight;
    const float plotHeight = bounds.getHeight() - padTop  - padBottom;

    // Transparent background — parent draws card
    g.fillAll(juce::Colours::transparentBlack);

    // Inset graph area
    auto graphBounds = juce::Rectangle<float>(plotX - 4.0f, plotY - 4.0f,
                                              plotWidth + 8.0f, plotHeight + 8.0f);
    LF::drawNeumorphicInset(g, graphBounds, 10.0f);

    // ---- Grid lines (horizontal, every 6 dB) ----
    g.setColour(LF::shadowColor.withAlpha(0.25f));
    for (int db = static_cast<int>(minDB); db <= static_cast<int>(maxDB); db += 6)
    {
        float normY = (static_cast<float>(db) - minDB) / (maxDB - minDB);
        float y = plotY + plotHeight * (1.0f - normY);
        g.drawHorizontalLine(static_cast<int>(y), plotX, plotX + plotWidth);

        // dB labels
        g.setColour(LF::textDimColor);
        g.setFont(juce::Font(9.0f));
        g.drawText(juce::String(db), 4.0f, y - 7.0f, 32.0f, 14.0f,
                   juce::Justification::centredRight, false);
        g.setColour(LF::shadowColor.withAlpha(0.25f));
    }

    // Centre 0 dB line
    float zeroY = plotY + plotHeight * (1.0f - (0.0f - minDB) / (maxDB - minDB));
    g.setColour(LF::textDimColor.withAlpha(0.4f));
    g.drawHorizontalLine(static_cast<int>(std::round(zeroY)), plotX, plotX + plotWidth);

    // ---- Frequency labels ----
    g.setColour(LF::textDimColor);
    g.setFont(juce::Font(9.0f));
    const float freqMarks[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const juce::String freqLabels[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    for (int i = 0; i < 10; ++i)
    {
        float x = plotX + freqToX(freqMarks[i], plotWidth);
        g.drawVerticalLine(static_cast<int>(x), plotY, plotY + plotHeight);
        g.drawText(freqLabels[i],
                   static_cast<int>(x - 20.0f),
                   static_cast<int>(plotY + plotHeight + 3.0f),
                   40, 16, juce::Justification::centred, false);
    }

    if (calibration == nullptr)
        return;

    // ---- Correction curve ----
    juce::Path curvePath;
    bool first = true;

    for (int i = 0; i < numPoints; ++i)
    {
        float x     = plotX + freqToX(freqs[i], plotWidth);
        float normY = (magnitudes[i] - minDB) / (maxDB - minDB);
        normY       = juce::jlimit(0.0f, 1.0f, normY);
        float y     = plotY + plotHeight * (1.0f - normY);

        if (first) { curvePath.startNewSubPath(x, y); first = false; }
        else       { curvePath.lineTo(x, y); }
    }

    // Draw filled area below curve — neumorphic accent gradient
    juce::Path fillPath = curvePath;
    fillPath.lineTo(plotX + plotWidth, zeroY);
    fillPath.lineTo(plotX, zeroY);
    fillPath.closeSubPath();

    juce::ColourGradient grad(
        LF::accentColor.withAlpha(0.45f), plotX, zeroY,
        LF::accentColor.withAlpha(0.08f), plotX, plotY,
        false);
    g.setGradientFill(grad);
    g.fillPath(fillPath);

    // Curve stroke
    g.setColour(LF::accentColor);
    g.strokePath(curvePath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));

    // ---- Profile name overlay ----
    const auto& profile = calibration->getProfile(calibration->getCurrentProfile());
    g.setColour(LF::accentColor.withAlpha(0.7f));
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText(profile.name,
               static_cast<int>(plotX + 8.0f),
               static_cast<int>(plotY + 4.0f),
               static_cast<int>(plotWidth - 16.0f),
               14, juce::Justification::topLeft, false);
}
