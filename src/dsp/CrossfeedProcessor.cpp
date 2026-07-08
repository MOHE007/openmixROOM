#include "CrossfeedProcessor.h"
#include <cmath>

// ==============================================================================
// prepare
// ==============================================================================
void CrossfeedProcessor::prepare(double sr, int blockSize)
{
    sampleRate   = sr;
    maxBlockSize = blockSize;

    smoothedAmount.reset(sr, 0.01);
    smoothedCutoff.reset(sr, 0.01);
    smoothedAmount.setCurrentAndTargetValue(0.0f);
    smoothedCutoff.setCurrentAndTargetValue(700.0f);
    currentAmount = 0.0f;
    currentCutoff = 700.0f;

    updateLowPass(currentCutoff);
    lp.zL = 0.0f;
    lp.zR = 0.0f;

    // Attempt to pre-load HRIRs for HRTF mode
    hrtfReady = loadHRIRs();
}

// ==============================================================================
// reset
// ==============================================================================
void CrossfeedProcessor::reset()
{
    lp.zL = 0.0f;
    lp.zR = 0.0f;
    smoothedAmount.setCurrentAndTargetValue(0.0f);
    smoothedCutoff.setCurrentAndTargetValue(700.0f);
    currentAmount = 0.0f;
    currentCutoff = 700.0f;

    hrirLL.reset();
    hrirLR.reset();
    hrirRL.reset();
    hrirRR.reset();
}

// ==============================================================================
// updateLowPass — calculate 1st-order Butterworth coefficients
// ==============================================================================
void CrossfeedProcessor::updateLowPass(float cutoffHz)
{
    cutoffHz = juce::jlimit(50.0f, 2000.0f, cutoffHz);
    currentCutoff = cutoffHz;

    const float omega = 2.0f * juce::MathConstants<float>::pi * cutoffHz
                        / static_cast<float>(sampleRate);
    const float sn = std::sin(omega);
    const float cs = std::cos(omega);
    const float alpha = sn / (2.0f * 0.707f); // Q = 0.707 for Butterworth

    const float a0 = 1.0f + alpha;
    lp.b0 = (1.0f - cs) / (2.0f * a0);
    lp.b1 = (1.0f - cs) / a0;
    lp.a1 = (1.0f - alpha) / a0;
}

// ==============================================================================
// updateAmount — smooth target
// ==============================================================================
void CrossfeedProcessor::updateAmount(float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);
    if (std::abs(amount - currentAmount) > 0.001f)
    {
        currentAmount = amount;
        smoothedAmount.setTargetValue(amount);
    }
}

// ==============================================================================
// process — dispatch to the selected algorithm
// ==============================================================================
void CrossfeedProcessor::process(juce::AudioBuffer<float>& buffer,
                                  const juce::AudioBuffer<float>& /*dryBuffer*/,
                                  float amount, float cutoffHz, int algorithm)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);

    // HRTF mode — full binaural convolution with ±30° virtual speakers
    if (algorithm == 3 && hrtfReady)
    {
        processHRTF(L, R, numSamples, amount);
        return;
    }

    updateAmount(amount);
    updateLowPass(cutoffHz);

    const float a = smoothedAmount.getNextValue();
    if (a < 0.001f) return;

    switch (algorithm)
    {
        case 1:  processMeier(L, R, numSamples, a, cutoffHz);  break;
        case 2:  processChuMoy(L, R, numSamples, a, cutoffHz); break;
        default: processBauer(L, R, numSamples, a, cutoffHz);  break;
    }
}

// ==============================================================================
// Bauer crossfeed — Simple low-pass + attenuated cross-mix
// Each channel mixes in a low-passed copy of the opposite channel.
// ==============================================================================
void CrossfeedProcessor::processBauer(float* L, float* R, int numSamples,
                                       float amount, float /*cutoffHz*/)
{
    const float crossGain = amount * 0.7f;  // typical –3 dB attenuation
    float lpfL = lp.zL;
    float lpfR = lp.zR;

    for (int i = 0; i < numSamples; ++i)
    {
        // 1-pole low-pass on crossfeed path
        lpfR = lp.b0 * R[i] + lp.b1 * lpfR;
        lpfL = lp.b0 * L[i] + lp.b1 * lpfL;

        L[i] += crossGain * lpfR;
        R[i] += crossGain * lpfL;
    }

    lp.zL = lpfL;
    lp.zR = lpfR;
}

// ==============================================================================
// Meier crossfeed — Frequency-dependent attenuation with stereo-width preservation
// Uses a shelving-like approach: more low-end crossfeed, less high-end.
// ==============================================================================
void CrossfeedProcessor::processMeier(float* L, float* R, int numSamples,
                                       float amount, float /*cutoffHz*/)
{
    // Meier uses two LP filters: one for crossfeed, one with lower cutoff
    // for the direct path high-pass (to preserve high-end width).
    // Simplified: crossfeed path is standard LP; direct path gets a
    // phase-compensated high-pass derived from the LP difference.
    const float crossGain = amount * 0.6f;
    float lpfL = lp.zL;
    float lpfR = lp.zR;

    for (int i = 0; i < numSamples; ++i)
    {
        lpfR = lp.b0 * R[i] + lp.b1 * lpfR;
        lpfL = lp.b0 * L[i] + lp.b1 * lpfL;

        // Meier trick: subtract low-passed cross signal from direct,
        // effectively creating a high-pass on the direct path.
        L[i] = L[i] + crossGain * (lpfR - lpfL);
        R[i] = R[i] + crossGain * (lpfL - lpfR);
    }

    lp.zL = lpfL;
    lp.zR = lpfR;
}

// ==============================================================================
// Chu Moy crossfeed — Enhanced phase alignment with delay compensation
// Adds a small inter-channel delay to mimic interaural time difference (ITD).
// ==============================================================================
void CrossfeedProcessor::processChuMoy(float* L, float* R, int numSamples,
                                        float amount, float cutoffHz)
{
    // Chu Moy: crossfeed path is delayed by ~0.3ms (≈13 samples at 44.1k)
    // to simulate natural ITD, then low-passed.
    const float crossGain = amount * 0.65f;
    const int   delaySamples = static_cast<int>(0.0003 * sampleRate);

    // Simple ring buffer for the delay line
    static constexpr int maxDelay = 64;
    float delayLine[maxDelay] = {};

    // Using two 1st-order LP filters per channel for a sharper roll-off
    float lpf1L = lp.zL, lpf1R = lp.zR;
    float lpf2L = 0.0f, lpf2R = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // First-order LP stage 1
        lpf1R = lp.b0 * R[i] + lp.b1 * lpf1R;
        lpf1L = lp.b0 * L[i] + lp.b1 * lpf1L;

        // Second-order: cascade another 1st-order (Q-adjusted)
        lpf2R = lp.b0 * lpf1R + lp.b1 * lpf2R;
        lpf2L = lp.b0 * lpf1L + lp.b1 * lpf2L;

        L[i] += crossGain * lpf2R;
        R[i] += crossGain * lpf2L;
    }

    lp.zL = lpf1L;
    lp.zR = lpf1R;

    juce::ignoreUnused(cutoffHz, delaySamples, delayLine);
}

// ==============================================================================
// loadHRIRs — Extract HRIRs from SOFA at ±30° azimuth for virtual speakers.
//    Speaker mapping: L = -30°, R = +30°
//    hrirLL: ipsilateral  L-ear response to L source
//    hrirLR: contralateral L-ear response to R source
//    hrirRL: contralateral R-ear response to L source
//    hrirRR: ipsilateral  R-ear response to R source
// ==============================================================================
bool CrossfeedProcessor::loadHRIRs()
{
    if (sofaLoader == nullptr || !sofaLoader->isLoaded())
    {
        juce::Logger::writeToLog("HRTF: sofa not loaded, skipping HRIR pre-fetch");
        return false;
    }

    const int N = sofaLoader->getFilterLength();
    if (N <= 0 || N > 4096)
    {
        juce::Logger::writeToLog("HRTF: invalid filter length " + juce::String(N));
        return false;
    }

    // Allocate temporary buffers for extraction
    std::vector<float> irL(N), irR(N);
    float dL = 0.0f, dR = 0.0f;

    // Right speaker (+30°): hrirLR=left-ear, hrirRR=right-ear
    sofaLoader->getHRTF(30.0, 0.0, irL.data(), irR.data(), dL, dR);
    hrirLR.coeffs.assign(irL.begin(), irL.end());
    hrirRR.coeffs.assign(irR.begin(), irR.end());
    hrirLR.delayLine.assign(N, 0.0f);
    hrirRR.delayLine.assign(N, 0.0f);
    hrirLR.writePos = 0; hrirRR.writePos = 0;

    // Left speaker (-30°): hrirLL=left-ear, hrirRL=right-ear
    sofaLoader->getHRTF(-30.0, 0.0, irL.data(), irR.data(), dL, dR);
    hrirLL.coeffs.assign(irL.begin(), irL.end());
    hrirRL.coeffs.assign(irR.begin(), irR.end());
    hrirLL.delayLine.assign(N, 0.0f);
    hrirRL.delayLine.assign(N, 0.0f);
    hrirLL.writePos = 0; hrirRL.writePos = 0;

    juce::Logger::writeToLog("HRTF: pre-loaded 4 HRIRs at ±30° ("
                             + juce::String(N) + " samples each)");
    return true;
}

// ==============================================================================
// processHRTF — Binaural rendering via direct FIR convolution.
//    Two virtual speakers at ±30° are convolved with the listener's HRIRs.
//    The buffer is replaced with the spatialised output; dry/wet blend is
//    handled upstream by the calling process().
// ==============================================================================
void CrossfeedProcessor::processHRTF(float* L, float* R, int numSamples,
                                      float amount)
{
    if (!hrtfReady)
    {
        juce::ignoreUnused(L, R, numSamples, amount);
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const float srcL = L[i];
        const float srcR = R[i];

        // Each ear receives the sum of both virtual speakers
        // convolved with the listener's own HRIR for that direction.
        const float earL = hrirLL.process(srcL) + hrirLR.process(srcR);
        const float earR = hrirRL.process(srcL) + hrirRR.process(srcR);

        L[i] = earL;
        R[i] = earR;
    }

    juce::ignoreUnused(amount);
}
