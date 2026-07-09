#include "RoomProcessor.h"
#include <cmath>
#include <cstring>

// ==============================================================================
// Prime-based delay lengths (in samples at 44.1k) — scaled by size parameter.
// Primes ensure minimal harmonic alignment between FDN channels.
// ==============================================================================
static constexpr int baseDelays[8] = {
    997, 1153, 1327, 1559,   // 22.6, 26.1, 30.1, 35.4 ms
    1801, 2099, 2393, 2711   // 40.8, 47.6, 54.3, 61.5 ms
};

// ==============================================================================
// Hadamard-8 mixing matrix (normalised by 1/√8)
// ==============================================================================
static constexpr float hadamard[8][8] = {
    { 1, 1, 1, 1, 1, 1, 1, 1},
    { 1,-1, 1,-1, 1,-1, 1,-1},
    { 1, 1,-1,-1, 1, 1,-1,-1},
    { 1,-1,-1, 1, 1,-1,-1, 1},
    { 1, 1, 1, 1,-1,-1,-1,-1},
    { 1,-1, 1,-1,-1, 1,-1, 1},
    { 1, 1,-1,-1,-1,-1, 1, 1},
    { 1,-1,-1, 1,-1, 1, 1,-1}
};
static constexpr float hmScale = 1.0f / 2.8284271247461903f; // 1/√8: unitary (energy-preserving)

// ==============================================================================
// Presets — classic plate reverb voicings
// ==============================================================================
RoomProcessor::Preset RoomProcessor::presetFor(int type)
{
    switch (type)
    {
        case Small:  // Vocal booth / small room plate
            return { 0.6f, 0.6f, 10.0f, 6000.0f, 400.0f, 0.5f };
        case Large:  // Large hall plate
            return { 3.5f, 1.6f, 35.0f, 5000.0f, 120.0f, 0.8f };
        case Medium:
        default:     // Studio plate — balanced
            return { 1.8f, 1.0f, 20.0f, 7000.0f, 200.0f, 0.65f };
    }
}

// ==============================================================================
// prepare
// ==============================================================================
void RoomProcessor::prepare(double sr, int blockSize)
{
    sampleRate   = sr;
    maxBlockSize = blockSize;
    reset();
    recalcDelays();

    // Randomize Hadamard: row/col sign flips break all-ones correlation
    // while preserving orthogonality (each flip is an orthogonal transform).
    // This is the Valhalla trick — unitary matrix with no DC amplification.
    for (int i = 0; i < fdnCount; ++i)
        for (int j = 0; j < fdnCount; ++j)
            randHadamard[i][j] = hadamard[i][j];

    juce::Random rng;
    for (int i = 0; i < fdnCount; ++i)
        if (rng.nextBool())
            for (int j = 0; j < fdnCount; ++j)
                randHadamard[i][j] *= -1.0f;
    for (int j = 0; j < fdnCount; ++j)
        if (rng.nextBool())
            for (int i = 0; i < fdnCount; ++i)
                randHadamard[i][j] *= -1.0f;
}

// ==============================================================================
// reset
// ==============================================================================
void RoomProcessor::reset()
{
    std::memset(&fdnL, 0, sizeof(fdnL));
    std::memset(&fdnR, 0, sizeof(fdnR));
    std::memset(apMemL, 0, sizeof(apMemL));
    std::memset(apMemR, 0, sizeof(apMemR));
    std::memset(preDelayL, 0, sizeof(preDelayL));
    std::memset(preDelayR, 0, sizeof(preDelayR));
    preDelayPos = 0;
    preDelayLen = 0;

    currentRt60   = targetRt60   = 1.5f;
    currentSize   = targetSize   = 1.0f;
    currentDampLp = targetDampLp = 7000.0f;
    currentDampHp = targetDampHp = 200.0f;
    currentPreMs  = targetPreMs  = 0;
}

// ==============================================================================
// recalcDelays — scale base delay lengths by size
// ==============================================================================
void RoomProcessor::recalcDelays()
{
    const float srScale = static_cast<float>(sampleRate) / 44100.0f;

    for (int i = 0; i < fdnCount; ++i)
    {
        int len = static_cast<int>(baseDelays[i] * srScale * currentSize);
        len = juce::jlimit(16, maxDelay, len);
        fdnL.delayLen[i] = len;
        fdnR.delayLen[i] = len + juce::Random::getSystemRandom().nextInt({1, 23}); // slight L/R decorrelation
        fdnR.delayLen[i] = juce::jlimit(16, maxDelay, fdnR.delayLen[i]);
    }
}

// ==============================================================================
// updateParameters
// ==============================================================================
void RoomProcessor::updateParameters(int roomType)
{
    const auto p = presetFor(roomType);
    targetRt60   = p.rt60;
    targetSize   = p.size;
    targetPreMs  = static_cast<int>(p.preDelayMs);
    targetDampLp = p.lpfHz;
    targetDampHp = p.hpfHz;

    preDelayLen = static_cast<int>(p.preDelayMs * 0.001f * static_cast<float>(sampleRate));
    preDelayLen = juce::jlimit(0, maxPreDelay, preDelayLen);
    preDelayPos = 0;
}

// ==============================================================================
// smoothParams — simple one-pole smoothing to avoid zipper noise
// ==============================================================================
void RoomProcessor::smoothParams()
{
    const float coeff = 0.02f;
    currentRt60   += (targetRt60   - currentRt60)   * coeff;
    currentSize   += (targetSize   - currentSize)   * coeff;
    currentDampLp += (targetDampLp - currentDampLp) * coeff;
    currentDampHp += (targetDampHp - currentDampHp) * coeff;
    currentPreMs  += (targetPreMs  - currentPreMs  > 0 ? 1 : -1) * (std::abs(targetPreMs - currentPreMs) > 0 ? 1 : 0);
}

// ==============================================================================
// process — FDN plate reverb
// ==============================================================================
void RoomProcessor::process(juce::AudioBuffer<float>& buffer,
                             float roomMix, int roomType, bool enabled)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0 || roomMix < 0.001f || !enabled)
        return;

    roomMix  = juce::jlimit(0.0f, 1.0f, roomMix);
    roomType = juce::jlimit(0, RoomType::Count - 1, roomType);

    updateParameters(roomType);
    recalcDelays();

    auto* L  = buffer.getWritePointer(0);
    auto* R  = buffer.getWritePointer(1);

    // --- Per-sample processing loop ---
    for (int i = 0; i < numSamples; ++i)
    {
        smoothParams();

        // --- Feedback gain from RT60 ---
        // RT60 = -3 * T_delay / log10(gain)  →  gain = 10^(-3 * T_delay / RT60)
        // For FDN: average T_delay across 8 channels
        float avgDelay = 0.0f;
        for (int k = 0; k < fdnCount; ++k)
            avgDelay += static_cast<float>(fdnL.delayLen[k]);
        avgDelay /= (fdnCount * static_cast<float>(sampleRate));

        const float feedback = std::pow(0.001f, avgDelay / juce::jmax(currentRt60, 0.05f));

        // --- Damping coefficients ---
        // One-pole form: y[n] = y[n-1] + coeff * (x[n] - y[n-1])
        // coeff = 1 - a, where a = exp(-2π·fc/fs)
        const float lpCoeff = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi
                                              * currentDampLp / static_cast<float>(sampleRate));
        const float hpCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi
                                       * currentDampHp / static_cast<float>(sampleRate));

        // --- Pre-delay ---
        float pdL = 0.0f, pdR = 0.0f;
        if (preDelayLen > 0)
        {
            pdL = preDelayL[preDelayPos];
            pdR = preDelayR[preDelayPos];
            preDelayL[preDelayPos] = L[i];
            preDelayR[preDelayPos] = R[i];
            preDelayPos = (preDelayPos + 1) % preDelayLen;
        }

        // --- Input sum (mono) for FDN ---
        const float inL = preDelayLen > 0 ? pdL : L[i];
        const float inR = preDelayLen > 0 ? pdR : R[i];
        float inMono = (inL + inR) * 0.5f;

        // --- 4-stage allpass diffuser (Schroeder-style) ---
        // This creates dense, smeared early reflections from the dry input
        for (int a = 0; a < apCount; ++a)
        {
            const float apGain = 0.618f;  // golden ratio — maximum diffusion
            const float wet = inMono + apGain * apMemL[a];
            apMemL[a] = wet * apGain - inMono;
            inMono = wet;
        }

        // --- FDN: accumulate feedback from all channels ---
        float outL = 0.0f, outR = 0.0f;

        // Read from all delay lines, apply damping, accumulate output
        float tapOutL[fdnCount], tapOutR[fdnCount];

        for (int k = 0; k < fdnCount; ++k)
        {
            // Read from delay line (2D array: tap index, then position)
            int wpL = fdnL.writePos[k];
            int wpR = fdnR.writePos[k];
            float valL = fdnL.delayLine[k][wpL];
            float valR = fdnR.delayLine[k][wpR];

            // Nested allpass for density (2 stages)
            {
                const float apG = 0.5f;
                const float wL = valL + apG * fdnL.apMem1[k];
                fdnL.apMem1[k] = wL * apG - valL;
                valL = wL;

                const float wR = valR + apG * fdnR.apMem1[k];
                fdnR.apMem1[k] = wR * apG - valR;
                valR = wR;
            }
            {
                const float apG = 0.5f;
                const float wL = valL + apG * fdnL.apMem2[k];
                fdnL.apMem2[k] = wL * apG - valL;
                valL = wL;

                const float wR = valR + apG * fdnR.apMem2[k];
                fdnR.apMem2[k] = wR * apG - valR;
                valR = wR;
            }

            // Damping: one-pole lowpass in feedback (gain-neutral, ≤ 1.0)
            // This is the standard FDN approach — attenuate high frequencies
            // per feedback cycle to simulate air absorption.
            fdnL.lpMem[k] += lpCoeff * (valL - fdnL.lpMem[k]);
            fdnR.lpMem[k] += lpCoeff * (valR - fdnR.lpMem[k]);

            tapOutL[k] = fdnL.lpMem[k];
            tapOutR[k] = fdnR.lpMem[k];

            outL += tapOutL[k];
            outR += tapOutR[k];
        }

        outL *= 0.125f; // 1/8: unity-average across 8 taps
        outR *= 0.125f;

        // --- Hadamard mix: input + feedback → new tap inputs ---
        for (int k = 0; k < fdnCount; ++k)
        {
            float mixL = 0.0f, mixR = 0.0f;
            for (int j = 0; j < fdnCount; ++j)
            {
                mixL += randHadamard[k][j] * tapOutL[j] * hmScale;
                mixR += randHadamard[k][j] * tapOutR[j] * hmScale;
            }

            // Inject: Hadamard-mixed feedback + fresh input
            {
                int wpL2 = fdnL.writePos[k];
                int wpR2 = fdnR.writePos[k];
                fdnL.delayLine[k][wpL2] = mixL * feedback + inMono * 0.15f;
                fdnR.delayLine[k][wpR2] = mixR * feedback + inMono * 0.15f;
            }

            // Advance write pointer (ring buffer)
            fdnL.writePos[k] = (fdnL.writePos[k] + 1) % fdnL.delayLen[k];
            fdnR.writePos[k] = (fdnR.writePos[k] + 1) % fdnR.delayLen[k];
        }

        // --- Dry/wet mix ---
        L[i] = L[i] * (1.0f - roomMix) + outL * roomMix;
        R[i] = R[i] * (1.0f - roomMix) + outR * roomMix;
    }
}
