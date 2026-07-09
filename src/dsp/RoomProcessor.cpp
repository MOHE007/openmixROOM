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
        case Small:       // Vocal booth / small room plate
            return { 0.6f, 0.6f, 10.0f, 6000.0f, 400.0f, 0.5f, 0.3f };
        case Large:       // Large hall plate
            return { 3.5f, 1.6f, 35.0f, 5000.0f, 120.0f, 0.8f, 0.6f };
        case ExtraLarge:  // Cathedral / ambient wash
            return { 6.0f, 2.0f, 50.0f, 4000.0f, 80.0f, 0.9f, 0.7f };
        case Medium:
        default:          // Studio plate — balanced
            return { 1.8f, 1.0f, 20.0f, 7000.0f, 200.0f, 0.65f, 0.5f };
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
    recalcERDelays();

    // Standard Hadamard-8 — row 0 all +1 preserves DC/low-frequency coupling
    // through the feedback path. Randomization was breaking feedback for
    // correlated signals (real music). Valhalla randomizes modulated delay
    // tap lengths, not the feedback matrix itself.
    for (int i = 0; i < fdnCount; ++i)
        for (int j = 0; j < fdnCount; ++j)
            randHadamard[i][j] = hadamard[i][j];
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
    std::memset(erBuffer, 0, sizeof(erBuffer));
    preDelayPos = 0;
    preDelayLen = 0;
    erWritePos  = 0;

    currentRt60   = targetRt60   = 1.5f;
    currentSize   = targetSize   = 1.0f;
    currentDampLp = targetDampLp = 7000.0f;
    currentDampHp = targetDampHp = 200.0f;
    currentERLevel = targetERLevel = 0.5f;
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
        // L/R decorrelation via fixed prime offset, not per-block random
        fdnR.delayLen[i] = len + (i * 7 + 3);  // 3, 10, 17, 24, 31, 38, 45, 52
        fdnR.delayLen[i] = juce::jlimit(16, maxDelay, fdnR.delayLen[i]);
    }

    // --- Clamp write positions & clear delay lines ---
    // When Size is reduced, delayLen[] shrinks. writePos[] from the previous
    // larger length may now be >= new delayLen[] (out of bounds). Without this
    // fix, the first process() frame reads garbage / stale feedback data,
    // producing a sharp click / crackle. We clamp writePos and zero the buffer
    // so the FDN restarts clean from the new length.
    for (int k = 0; k < fdnCount; ++k)
    {
        // Clamp write position into new delay line bounds
        if (fdnL.writePos[k] >= fdnL.delayLen[k])
            fdnL.writePos[k] = fdnL.delayLen[k] - 1;
        if (fdnR.writePos[k] >= fdnR.delayLen[k])
            fdnR.writePos[k] = fdnR.delayLen[k] - 1;

        // Zero the entire delay line buffer to flush stale feedback data
        std::memset(fdnL.delayLine[k], 0, maxDelay * sizeof(float));
        std::memset(fdnR.delayLine[k], 0, maxDelay * sizeof(float));
    }
}

// ==============================================================================
// loadPreset — set all target params from RoomType
// ==============================================================================
void RoomProcessor::loadPreset(int roomType)
{
    roomType = juce::jlimit(0, RoomType::Count - 1, roomType);
    const auto p = presetFor(roomType);
    const bool sizeChanged = (std::abs(targetSize - p.size) > 0.001f);
    targetRt60   = p.rt60;
    targetSize   = p.size;
    targetPreMs  = static_cast<int>(p.preDelayMs);
    targetDampLp = p.lpfHz;
    targetDampHp = p.hpfHz;
    targetERLevel = p.erLevel;

    applyPreDelayLength();

    if (sizeChanged)
    {
        recalcDelays();
        recalcERDelays();
    }
}

// ==============================================================================
// Individual parameter overrides
// ==============================================================================
void RoomProcessor::setRoomSize(float s)
{
    const float clamped = juce::jlimit(0.5f, 2.0f, s);
    if (std::abs(targetSize - clamped) > 0.001f)
    {
        targetSize = clamped;
        recalcDelays();
        recalcERDelays();
    }
}

void RoomProcessor::setPreDelay(float ms)
{
    targetPreMs = static_cast<int>(juce::jlimit(0.0f, 50.0f, ms));
    applyPreDelayLength();
}

void RoomProcessor::setDamping(float hz)
{
    targetDampLp = juce::jlimit(2000.0f, 20000.0f, hz);
}

void RoomProcessor::setERLevel(float level)
{
    targetERLevel = juce::jlimit(0.0f, 1.0f, level);
}

// ==============================================================================
// ER tap delays (ms) — staggered short reflections with prime spacing
// ==============================================================================
static constexpr float erTapMs[8] = {
    5.0f,  11.0f, 17.0f, 23.0f,   // left wall bounces
    31.0f, 41.0f, 53.0f, 67.0f    // right wall bounces
};

// ER per-tap gain: -3 dB per doubling (~natural reflection falloff)
static constexpr float erTapGain[8] = {
    0.707f, 0.500f, 0.354f, 0.250f,
    0.177f, 0.125f, 0.088f, 0.063f
};

// ER stereo panning — alternating L/R spread for spatial cues
static constexpr float erPanL[8] = { 1.00f, 0.85f, 0.60f, 0.40f, 0.20f, 0.70f, 0.85f, 0.45f };
static constexpr float erPanR[8] = { 0.00f, 0.15f, 0.40f, 0.60f, 0.80f, 0.30f, 0.15f, 0.55f };

void RoomProcessor::recalcERDelays()
{
    const float srScale = static_cast<float>(sampleRate) / 44100.0f;
    for (int i = 0; i < erTapCount; ++i)
    {
        int len = static_cast<int>(erTapMs[i] * srScale * currentSize * 0.001f * static_cast<float>(sampleRate));
        erDelaySamps[i] = juce::jlimit(1, erBufferSize - 1, len);
    }
}

// ==============================================================================
// applyPreDelayLength — convert ms to sample count
// ==============================================================================
void RoomProcessor::applyPreDelayLength()
{
    preDelayLen = static_cast<int>(static_cast<float>(targetPreMs) * 0.001f * static_cast<float>(sampleRate));
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
    currentERLevel += (targetERLevel - currentERLevel) * coeff;
    currentPreMs  += (targetPreMs  - currentPreMs  > 0 ? 1 : -1) * (std::abs(targetPreMs - currentPreMs) > 0 ? 1 : 0);
}

// ==============================================================================
// process — FDN plate reverb (no preset loading — params set externally)
// ==============================================================================
void RoomProcessor::process(juce::AudioBuffer<float>& buffer,
                             float roomMix, bool enabled)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0 || roomMix < 0.001f || !enabled)
        return;

    roomMix = juce::jlimit(0.0f, 1.0f, roomMix);

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

        // --- Early Reflections: 8-tap delay network ---
        // Fed from diffused input for density, parallel to FDN late reverb.
        // Taps use staggered delays with per-tap gain falloff and L/R panning.
        erBuffer[erWritePos] = inMono;
        erWritePos = (erWritePos + 1) % erBufferSize;

        float erOutL = 0.0f, erOutR = 0.0f;
        for (int t = 0; t < erTapCount; ++t)
        {
            const int rdIdx = (erWritePos - erDelaySamps[t] + erBufferSize) % erBufferSize;
            const float tap = erBuffer[rdIdx] * erTapGain[t];
            erOutL += tap * erPanL[t];
            erOutR += tap * erPanR[t];
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

            // Nested allpass for density (2 stages, g=0.618 golden ratio)
            {
                const float apG = 0.618f;
                const float wL = valL + apG * fdnL.apMem1[k];
                fdnL.apMem1[k] = wL * apG - valL;
                valL = wL;

                const float wR = valR + apG * fdnR.apMem1[k];
                fdnR.apMem1[k] = wR * apG - valR;
                valR = wR;
            }
            {
                const float apG = 0.618f;
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
                fdnL.delayLine[k][wpL2] = mixL * feedback + inMono * 0.354f;
                fdnR.delayLine[k][wpR2] = mixR * feedback + inMono * 0.354f;
            }

            // Advance write pointer (ring buffer)
            fdnL.writePos[k] = (fdnL.writePos[k] + 1) % fdnL.delayLen[k];
            fdnR.writePos[k] = (fdnR.writePos[k] + 1) % fdnR.delayLen[k];
        }

        // --- Additive wet mix (dry passes through at unity) ---
        // ER + Late Reverb summed independently
        L[i] += erOutL * currentERLevel + outL * roomMix;
        R[i] += erOutR * currentERLevel + outR * roomMix;
    }
}
