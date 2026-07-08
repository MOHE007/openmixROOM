#include "RoomProcessor.h"
#include <random>
#include <cmath>

// ==============================================================================
// prepare
// ==============================================================================
void RoomProcessor::prepare(double sr, int blockSize)
{
    sampleRate   = sr;
    maxBlockSize = blockSize;
    currentRoom  = -1;

    // Pre-allocate mono buffers
    wetBufferL.setSize(1, maxBlockSize);
    wetBufferR.setSize(1, maxBlockSize);
}

// ==============================================================================
// reset
// ==============================================================================
void RoomProcessor::reset()
{
    convL.reset();
    convR.reset();
    currentRoom = -1;
}

// ==============================================================================
// process — apply room convolution with wet/dry mix
// ==============================================================================
void RoomProcessor::process(juce::AudioBuffer<float>& buffer,
                             float roomMix, int roomType)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0 || roomMix < 0.001f)
        return;

    roomMix  = juce::jlimit(0.0f, 1.0f, roomMix);
    roomType = juce::jlimit(0, RoomType::Count - 1, roomType);

    // Regenerate IR if room type changed
    if (roomType != currentRoom)
    {
        generateSyntheticIR(roomType);
        currentRoom = roomType;
    }

    // Copy stereo input to mono scratch buffers
    wetBufferL.copyFrom(0, 0, buffer, 0, 0, numSamples);
    wetBufferR.copyFrom(0, 0, buffer, 1, 0, numSamples);

    // Convolve each channel
    {
        juce::dsp::AudioBlock<float> blockL(wetBufferL);
        juce::dsp::ProcessContextReplacing<float> ctxL(blockL);
        convL.process(ctxL);
    }
    {
        juce::dsp::AudioBlock<float> blockR(wetBufferR);
        juce::dsp::ProcessContextReplacing<float> ctxR(blockR);
        convR.process(ctxR);
    }

    // Mix wet signal into the output buffer (additive: dry stays, wet is blended)
    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getWritePointer(1);
    const auto* wL = wetBufferL.getReadPointer(0);
    const auto* wR = wetBufferR.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        L[i] = L[i] * (1.0f - roomMix) + wL[i] * roomMix;
        R[i] = R[i] * (1.0f - roomMix) + wR[i] * roomMix;
    }
}

// ==============================================================================
// generateSyntheticIR — build a full stereo room IR
// ==============================================================================
void RoomProcessor::generateSyntheticIR(int roomType)
{
    // --- room parameters -----------------------------------------------------------------
    struct RoomCfg { float rt60; float hfDamp; std::vector<ERTap> taps; };

    const RoomCfg configs[RoomType::Count] = {
        // Small: ~0.4 s, modest ER
        { 0.4f, 0.6f, {
            { 2.0f, 0.50f, 6000.0f }, { 4.0f, 0.35f, 5500.0f },
            { 7.0f, 0.22f, 4800.0f }, { 11.0f, 0.14f, 4000.0f },
            { 16.0f, 0.08f, 3200.0f }, { 22.0f, 0.04f, 2500.0f }
        }},
        // Medium: ~0.9 s, denser ER
        { 0.9f, 0.5f, {
            { 4.0f, 0.45f, 5000.0f }, { 7.0f, 0.32f, 4500.0f },
            { 11.0f, 0.24f, 4000.0f }, { 15.0f, 0.18f, 3600.0f },
            { 20.0f, 0.13f, 3200.0f }, { 26.0f, 0.09f, 2800.0f },
            { 33.0f, 0.06f, 2400.0f }, { 41.0f, 0.04f, 2000.0f }
        }},
        // Large: ~1.8 s, long dense ER
        { 1.8f, 0.4f, {
            { 6.0f, 0.40f, 4500.0f }, { 10.0f, 0.30f, 4000.0f },
            { 15.0f, 0.23f, 3600.0f }, { 21.0f, 0.17f, 3200.0f },
            { 28.0f, 0.13f, 2800.0f }, { 36.0f, 0.09f, 2500.0f },
            { 45.0f, 0.07f, 2200.0f }, { 55.0f, 0.05f, 1900.0f },
            { 66.0f, 0.04f, 1600.0f }, { 78.0f, 0.03f, 1400.0f }
        }}
    };

    const auto& cfg = configs[roomType];
    const int irLength = static_cast<int>(sampleRate * cfg.rt60 * 1.2f); // 20% margin
    const int irLen    = juce::jmax(irLength, 256);

    std::vector<float> irL(irLen, 0.0f);
    std::vector<float> irR(irLen, 0.0f);

    // Stereo decorrelation: stagger L/R reflection delays by small random offsets
    std::mt19937 rng(static_cast<unsigned>(roomType * 12345));
    std::uniform_real_distribution<float> jitter(-0.5f, 0.5f);

    // Bake early reflections for left and right ears independently
    {
        std::vector<ERTap> tapsL = cfg.taps;
        std::vector<ERTap> tapsR = cfg.taps;
        for (auto& t : tapsL) t.delayMs += jitter(rng);
        for (auto& t : tapsR) t.delayMs += jitter(rng);

        bakeER(tapsL, irL.data(), irLen, sampleRate);
        bakeER(tapsR, irR.data(), irLen, sampleRate);
    }

    // Bake late reverb tail (start just after the last ER tap)
    const float lastTapMs = cfg.taps.back().delayMs + 4.0f;
    const int   tailStart = static_cast<int>(lastTapMs * sampleRate * 0.001);
    const int   tailStartClamped = juce::jmin(tailStart, irLen / 4);

    bakeTail(irL.data(), tailStartClamped, irLen, sampleRate, cfg.rt60, cfg.hfDamp);
    bakeTail(irR.data(), tailStartClamped, irLen, sampleRate, cfg.rt60, cfg.hfDamp);

    // Normalise IR to prevent clipping
    float peak = 0.0f;
    for (int i = 0; i < irLen; ++i)
    {
        peak = std::max(peak, std::abs(irL[i]));
        peak = std::max(peak, std::abs(irR[i]));
    }
    if (peak > 0.001f)
    {
        const float scale = 0.8f / peak;
        for (int i = 0; i < irLen; ++i) { irL[i] *= scale; irR[i] *= scale; }
    }

    // Load into convolution engines — use AudioBuffer to wrap IR data
    juce::AudioBuffer<float> irBufferL(1, irLen);
    juce::AudioBuffer<float> irBufferR(1, irLen);
    irBufferL.copyFrom(0, 0, irL.data(), irLen);
    irBufferR.copyFrom(0, 0, irR.data(), irLen);

    convL.loadImpulseResponse(std::move(irBufferL), sampleRate,
                              juce::dsp::Convolution::Stereo::no,
                              juce::dsp::Convolution::Trim::no,
                              juce::dsp::Convolution::Normalise::no);

    convR.loadImpulseResponse(std::move(irBufferR), sampleRate,
                              juce::dsp::Convolution::Stereo::no,
                              juce::dsp::Convolution::Trim::no,
                              juce::dsp::Convolution::Normalise::no);

    juce::Logger::writeToLog("RoomProcessor: loaded IR (" + juce::String(irLen)
                             + " samples) for room type " + juce::String(roomType));
}

// ==============================================================================
// bakeER — render discrete early reflection taps into a buffer
// ==============================================================================
void RoomProcessor::bakeER(const std::vector<ERTap>& taps,
                            float* dest, int irLength, double sr)
{
    for (const auto& tap : taps)
    {
        const int   sampleIdx = static_cast<int>(tap.delayMs * sr * 0.001);
        if (sampleIdx >= irLength) continue;

        // Place impulse
        dest[sampleIdx] += tap.gain;

        // Simple 1-pole low-pass smoothing on the reflection tail
        const float alpha = std::exp(-2.0f * juce::MathConstants<float>::pi
                                     * tap.cutoffHz / static_cast<float>(sr));
        float state = tap.gain * (1.0f - alpha);

        for (int i = sampleIdx + 1; i < juce::jmin(sampleIdx + 40, irLength); ++i)
        {
            state *= alpha;
            dest[i] += state;
        }
    }
}

// ==============================================================================
// bakeTail — exponentially decaying filtered noise
// ==============================================================================
void RoomProcessor::bakeTail(float* dest, int startSample, int irLength,
                              double sr, float rt60, float hfDamping)
{
    const int tailLen = irLength - startSample;
    if (tailLen <= 0) return;

    std::mt19937 rng(42); // deterministic
    std::normal_distribution<float> noise(0.0f, 1.0f);

    // Decay envelope: -60 dB over RT60 seconds
    const float decayRate  = std::exp(-6.9078f / (rt60 * static_cast<float>(sr)));
    const float hfDecayRate = std::exp(-6.9078f / (rt60 * hfDamping * static_cast<float>(sr)));

    // One-pole LP filter state for HF damping
    const float lpCutoff = 8000.0f; // start with wide bandwidth
    const float lpAlpha  = std::exp(-2.0f * juce::MathConstants<float>::pi
                                    * lpCutoff / static_cast<float>(sr));
    float lpState = 0.0f;

    float envelope = 1.0f;   // full-band envelope
    for (int i = 0; i < tailLen; ++i)
    {
        // White noise → LP filter → envelope
        const float raw = noise(rng) * envelope;
        lpState = lpAlpha * lpState + (1.0f - lpAlpha) * raw;
        dest[startSample + i] += lpState * 0.4f; // scale to avoid dominating ER
        envelope *= decayRate;

        // Lower the LP cutoff over time (HF damping becomes more aggressive)
        const float dynamicAlpha = lpAlpha + (1.0f - lpAlpha) * (1.0f - envelope) * hfDamping;
        lpState = dynamicAlpha * lpState + (1.0f - dynamicAlpha) * raw;
    }
}
