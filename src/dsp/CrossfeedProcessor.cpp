#include "CrossfeedProcessor.h"
#include <cmath>
#include <cstring>

// ==============================================================================
// prepare
// ==============================================================================
void CrossfeedProcessor::prepare(double sr, int blockSize)
{
    sampleRate   = sr;
    maxBlockSize = blockSize;

    // ITD: ~0.3 ms for typical 30° speaker angle (head radius ≈ 8.7 cm)
    delayLen = juce::jlimit(4, maxDelaySamples,
                            static_cast<int>(sampleRate * 0.0003));
    delayWrite = 0;
    std::memset(delayRing, 0, sizeof(delayRing));

    smoothedAmount.reset(sr, 0.015);
    smoothedCutoff.reset(sr, 0.015);
    smoothedAmount.setCurrentAndTargetValue(0.0f);
    smoothedCutoff.setCurrentAndTargetValue(700.0f);
    currentAmount = 0.0f;
    currentCutoff = 700.0f;

    updateLowPass(currentCutoff);

    // HRTF
    hrtfReady = loadHRIRs();
}

// ==============================================================================
// reset
// ==============================================================================
void CrossfeedProcessor::reset()
{
    lp = LP2{};
    delayWrite = 0;
    std::memset(delayRing, 0, sizeof(delayRing));

    smoothedAmount.setCurrentAndTargetValue(0.0f);
    smoothedCutoff.setCurrentAndTargetValue(700.0f);
    currentAmount = 0.0f;
    currentCutoff = 700.0f;

    hrirLL.reset(); hrirLR.reset();
    hrirRL.reset(); hrirRR.reset();
}

// ==============================================================================
// updateLowPass — 2nd-order Butterworth biquad (Q = 0.707)
// ==============================================================================
void CrossfeedProcessor::updateLowPass(float cutoffHz)
{
    cutoffHz = juce::jlimit(80.0f, 2000.0f, cutoffHz);
    currentCutoff = cutoffHz;

    const float omega = 2.0f * juce::MathConstants<float>::pi
                        * cutoffHz / static_cast<float>(sampleRate);
    const float sn  = std::sin(omega);
    const float cs  = std::cos(omega);
    const float alpha = sn / (2.0f * 0.7071f);   // Q = 1/√2

    const float a0inv = 1.0f / (1.0f + alpha);
    lp.b0 = (1.0f - cs) * 0.5f * a0inv;
    lp.b1 = (1.0f - cs) * a0inv;
    lp.b2 = (1.0f - cs) * 0.5f * a0inv;
    lp.a1 = -2.0f * cs * a0inv;
    lp.a2 = (1.0f - alpha) * a0inv;

    // Reset state on coefficient change to prevent transients
    lp.z1L = lp.z2L = 0.0f;
    lp.z1R = lp.z2R = 0.0f;
}

// ==============================================================================
// updateAmount
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
// process — dispatch
// ==============================================================================
void CrossfeedProcessor::process(juce::AudioBuffer<float>& buffer,
                                  const juce::AudioBuffer<float>& dryBuffer,
                                  float amount, float cutoffHz, int algorithm)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;

    if (algorithm == 3 && hrtfReady)
    {
        processHRTF(buffer, dryBuffer, numSamples, amount);
        return;
    }

    updateAmount(amount);
    updateLowPass(cutoffHz);

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const float a = smoothedAmount.getNextValue();
    if (a < 0.002f) return;

    switch (algorithm)
    {
        case 1:  processMeier (L, R, numSamples, a, cutoffHz); break;
        case 2:  processChuMoy(L, R, numSamples, a, cutoffHz); break;
        default: processBauer (L, R, numSamples, a, cutoffHz); break;
    }
}

// ==============================================================================
// Bauer crossfeed — 2nd-order Butterworth LP + ITD delay on contralateral path.
//
//   L_out = L_direct + gain * LP(R_delayed)
//   R_out = R_direct + gain * LP(L_delayed)
//
// The ITD delay (~0.3 ms) mimics speaker-at-30° interaural time difference,
// producing a natural spatial cue that simple gain mixing lacks.
// ==============================================================================
void CrossfeedProcessor::processBauer(float* L, float* R, int numSamples,
                                       float amount, float /*cutoffHz*/)
{
    // Crossfeed gain: -3 dB at full amount, scaled by amount
    const float crossGain = amount * 0.5f;

    float z1L = lp.z1L, z2L = lp.z2L;
    float z1R = lp.z1R, z2R = lp.z2R;

    // Local copy of ring buffer state (avoid member mutation inside loop)
    int   wPos = delayWrite;
    const int dLen = delayLen;

    for (int i = 0; i < numSamples; ++i)
    {
        // --- Ring buffer: write current sample, read delayed sample ---
        delayRing[wPos] = R[i];
        const float rDel = delayRing[(wPos - dLen + maxDelaySamples) % maxDelaySamples];

        // 2nd-order LP on delayed contralateral
        const float lpOut = rDel * lp.b0 + z1R;
        z1R = rDel * lp.b1 - lp.a1 * lpOut + z2R;
        z2R = rDel * lp.b2 - lp.a2 * lpOut;

        L[i] += crossGain * lpOut;

        // Symmetric: delay L, LP, add to R
        delayRing[wPos] = L[i];
        const float lDel = delayRing[(wPos - dLen + maxDelaySamples) % maxDelaySamples];

        const float lpOut2 = lDel * lp.b0 + z1L;
        z1L = lDel * lp.b1 - lp.a1 * lpOut2 + z2L;
        z2L = lDel * lp.b2 - lp.a2 * lpOut2;

        R[i] += crossGain * lpOut2;

        wPos = (wPos + 1) % maxDelaySamples;
    }

    lp.z1L = z1L; lp.z2L = z2L;
    lp.z1R = z1R; lp.z2R = z2R;
    delayWrite = wPos;
}

// ==============================================================================
// Meier crossfeed — natural speaker crosstalk model.
//
// Real speaker crosstalk is frequency-dependent:
//   ILD:   ~0 dB below 500 Hz  →  -6 dB at 2 kHz  →  -10 dB at 4 kHz
//   ITD:   ~0 ms below 1.5 kHz  →  0.3 ms above
//
// This implementation splits each channel into two bands via complementary
// LP/HP (derived from the same 2nd-order LP) and applies different crossfeed
// gains per band — bass is nearly mono, treble stays wide.
// ==============================================================================
void CrossfeedProcessor::processMeier(float* L, float* R, int numSamples,
                                       float amount, float /*cutoffHz*/)
{
    // Bass crossfeed (strong)  → mono-ises low end
    // Treble crossfeed (weak)  → preserves stereo width above cutoff
    const float bassGain   = amount * 0.55f;
    const float trebleGain = amount * 0.15f;

    float z1L = lp.z1L, z2L = lp.z2L;
    float z1R = lp.z1R, z2R = lp.z2R;

    for (int i = 0; i < numSamples; ++i)
    {
        // 2nd-order LP on L/R → bass component
        const float lpL = L[i] * lp.b0 + z1L;
        z1L = L[i] * lp.b1 - lp.a1 * lpL + z2L;
        z2L = L[i] * lp.b2 - lp.a2 * lpL;

        const float lpR = R[i] * lp.b0 + z1R;
        z1R = R[i] * lp.b1 - lp.a1 * lpR + z2R;
        z2R = R[i] * lp.b2 - lp.a2 * lpR;

        // HP component = direct − LP (complementary, perfect reconstruction)
        const float hpL = L[i] - lpL;
        const float hpR = R[i] - lpR;

        // Blend contralateral bass + contralateral treble into each channel
        L[i] = L[i] + bassGain * (lpR - lpL) + trebleGain * (hpR - hpL);
        R[i] = R[i] + bassGain * (lpL - lpR) + trebleGain * (hpL - hpR);
    }

    lp.z1L = z1L; lp.z2L = z2L;
    lp.z1R = z1R; lp.z2R = z2R;
}

// ==============================================================================
// Chu Moy crossfeed — dual-cascade LP + explicit ITD ring-buffer.
//
// Unlike Bauer which delays-then-LP's, Chu Moy applies two cascaded 2nd-order
// LP stages (effective 4th-order = −24 dB/oct roll-off) for a more realistic
// frequency-dependent ILD, plus a dedicated ITD delay line for the contralateral
// path.  This better approximates the human auditory periphery's cochlear
// filtering + interaural time-coding.
// ==============================================================================
void CrossfeedProcessor::processChuMoy(float* L, float* R, int numSamples,
                                        float amount, float /*cutoffHz*/)
{
    const float crossGain = amount * 0.45f;

    // Use a second LP state for cascade (4th-order equivalent)
    // Shared state for the cascade; initialise from lp on first use
    float z1L2 = 0, z2L2 = 0;
    float z1R2 = 0, z2R2 = 0;

    float z1L1 = lp.z1L, z2L1 = lp.z2L;
    float z1R1 = lp.z1R, z2R1 = lp.z2R;

    int   wPos = delayWrite;
    const int dLen = delayLen;

    for (int i = 0; i < numSamples; ++i)
    {
        // --- ITD delay on contralateral signal ---
        delayRing[wPos] = R[i];
        const float rDel = delayRing[(wPos - dLen + maxDelaySamples) % maxDelaySamples];

        // Cascade stage 1 (2nd-order LP)
        const float s1 = rDel * lp.b0 + z1R1;
        z1R1 = rDel * lp.b1 - lp.a1 * s1 + z2R1;
        z2R1 = rDel * lp.b2 - lp.a2 * s1;

        // Cascade stage 2 (2nd-order LP) → 4th-order total
        const float s2 = s1 * lp.b0 + z1R2;
        z1R2 = s1 * lp.b1 - lp.a1 * s2 + z2R2;
        z2R2 = s1 * lp.b2 - lp.a2 * s2;

        L[i] += crossGain * s2;

        // Symmetric for R channel
        delayRing[wPos] = L[i];
        const float lDel = delayRing[(wPos - dLen + maxDelaySamples) % maxDelaySamples];

        const float s1b = lDel * lp.b0 + z1L1;
        z1L1 = lDel * lp.b1 - lp.a1 * s1b + z2L1;
        z2L1 = lDel * lp.b2 - lp.a2 * s1b;

        const float s2b = s1b * lp.b0 + z1L2;
        z1L2 = s1b * lp.b1 - lp.a1 * s2b + z2L2;
        z2L2 = s1b * lp.b2 - lp.a2 * s2b;

        R[i] += crossGain * s2b;

        wPos = (wPos + 1) % maxDelaySamples;
    }

    lp.z1L = z1L1; lp.z2L = z2L1;
    lp.z1R = z1R1; lp.z2R = z2R1;
    delayWrite = wPos;
}

// ==============================================================================
// loadHRIRs — extract 4 HRIRs from SOFA for ±30° virtual speakers.
// ==============================================================================
bool CrossfeedProcessor::loadHRIRs()
{
    if (sofaLoader == nullptr || !sofaLoader->isLoaded())
    {
        juce::Logger::writeToLog("HRTF: sofa not loaded, skipping");
        return false;
    }

    const int N = sofaLoader->getFilterLength();
    if (N <= 0 || N > 8192) return false;

    // Setup convolution engine
    convSpec.sampleRate       = sampleRate;
    convSpec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    convSpec.numChannels      = 1;

    std::vector<float> irL(N), irR(N);
    float dL = 0, dR = 0;

    using Stereo    = juce::dsp::Convolution::Stereo;
    using Trim      = juce::dsp::Convolution::Trim;
    using Normalise = juce::dsp::Convolution::Normalise;

    // HRIR pair for right speaker (+30°)
    sofaLoader->getHRTF(30.0, 0.0, irL.data(), irR.data(), dL, dR);
    {
        juce::AudioBuffer<float> buf(1, N);
        buf.copyFrom(0, 0, irL.data(), N);
        hrirLR.loadImpulseResponse(std::move(buf), sampleRate,
                                    Stereo::no, Trim::no, Normalise::no);
        hrirLR.prepare(convSpec);
    }
    {
        juce::AudioBuffer<float> buf(1, N);
        buf.copyFrom(0, 0, irR.data(), N);
        hrirRR.loadImpulseResponse(std::move(buf), sampleRate,
                                    Stereo::no, Trim::no, Normalise::no);
        hrirRR.prepare(convSpec);
    }

    // HRIR pair for left speaker (-30°)
    sofaLoader->getHRTF(-30.0, 0.0, irL.data(), irR.data(), dL, dR);
    {
        juce::AudioBuffer<float> buf(1, N);
        buf.copyFrom(0, 0, irL.data(), N);
        hrirLL.loadImpulseResponse(std::move(buf), sampleRate,
                                    Stereo::no, Trim::no, Normalise::no);
        hrirLL.prepare(convSpec);
    }
    {
        juce::AudioBuffer<float> buf(1, N);
        buf.copyFrom(0, 0, irR.data(), N);
        hrirRL.loadImpulseResponse(std::move(buf), sampleRate,
                                    Stereo::no, Trim::no, Normalise::no);
        hrirRL.prepare(convSpec);
    }

    juce::Logger::writeToLog("HRTF: 4× Convolution ready, IR length="
                             + juce::String(N) + " samples");
    return true;
}

// ==============================================================================
// processHRTF — binaural rendering via FFT convolution + dry/wet blend.
//
// earL = srcL * HRIR_LL + srcR * HRIR_LR
// earR = srcL * HRIR_RL + srcR * HRIR_RR
//
// FFTConvolver operates in-place, so each source must be copied before convolving.
// ==============================================================================
void CrossfeedProcessor::processHRTF(juce::AudioBuffer<float>& buffer,
                                      const juce::AudioBuffer<float>& dry,
                                      int numSamples, float amount)
{
    if (!hrtfReady) return;
    amount = juce::jlimit(0.0f, 1.0f, amount);
    if (amount < 0.002f) return;

    // Preserve original stereo input as sources
    juce::AudioBuffer<float> srcL(1, numSamples);
    juce::AudioBuffer<float> srcR(1, numSamples);
    srcL.copyFrom(0, 0, buffer, 0, 0, numSamples);
    srcR.copyFrom(0, 0, buffer, 1, 0, numSamples);

    // ---- Compute earL = srcL * HRIR_LL + srcR * HRIR_LR ----
    juce::AudioBuffer<float> workL(1, numSamples);

    // Contribution 1: srcL → HRIR_LL
    workL.copyFrom(0, 0, srcL, 0, 0, numSamples);
    {
        juce::dsp::AudioBlock<float> blk(workL);
        juce::dsp::ProcessContextReplacing<float> ctx(blk);
        hrirLL.process(ctx);
    }

    // Contribution 2: srcR → HRIR_LR
    juce::AudioBuffer<float> tmpR(1, numSamples);
    tmpR.copyFrom(0, 0, srcR, 0, 0, numSamples);
    {
        juce::dsp::AudioBlock<float> blk(tmpR);
        juce::dsp::ProcessContextReplacing<float> ctx(blk);
        hrirLR.process(ctx);
    }
    for (int i = 0; i < numSamples; ++i)
        workL.setSample(0, i, workL.getSample(0, i) + tmpR.getSample(0, i));

    // ---- Compute earR = srcL * HRIR_RL + srcR * HRIR_RR ----
    juce::AudioBuffer<float> workR(1, numSamples);

    // Contribution 1: srcR → HRIR_RR
    workR.copyFrom(0, 0, srcR, 0, 0, numSamples);
    {
        juce::dsp::AudioBlock<float> blk(workR);
        juce::dsp::ProcessContextReplacing<float> ctx(blk);
        hrirRR.process(ctx);
    }

    // Contribution 2: srcL → HRIR_RL
    juce::AudioBuffer<float> tmpL(1, numSamples);
    tmpL.copyFrom(0, 0, srcL, 0, 0, numSamples);
    {
        juce::dsp::AudioBlock<float> blk(tmpL);
        juce::dsp::ProcessContextReplacing<float> ctx(blk);
        hrirRL.process(ctx);
    }
    for (int i = 0; i < numSamples; ++i)
        workR.setSample(0, i, workR.getSample(0, i) + tmpL.getSample(0, i));

    // ---- Dry/wet blend ----
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const auto* dryL = dry.getReadPointer(0);
    const auto* dryR = dry.getReadPointer(1);
    const auto* earL = workL.getReadPointer(0);
    const auto* earR = workR.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        L[i] = dryL[i] * (1.0f - amount) + earL[i] * amount;
        R[i] = dryR[i] * (1.0f - amount) + earR[i] * amount;
    }
}
