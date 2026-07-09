#include "RoomProcessor.h"
#include <random>
#include <cmath>
#include <algorithm>

// ==============================================================================
// Room presets — physically plausible dimensions and absorption coefficients
// ==============================================================================
RoomProcessor::RoomGeometry RoomProcessor::roomPreset(int type)
{
    RoomGeometry g;

    switch (type)
    {
        case Small:
        default:
            // Control room / small studio: 3×4×2.5m
            g.width  = 3.0f;  g.depth  = 4.0f;  g.height = 2.5f;
            // Surfaces: front, back, left, right, floor, ceiling
            // Small room tends to have more absorptive surfaces (acoustic treatment)
            g.wallAbsorption[0] = 0.30f;  // front (mixing desk + monitors)
            g.wallAbsorption[1] = 0.25f;  // back
            g.wallAbsorption[2] = 0.20f;  // left
            g.wallAbsorption[3] = 0.20f;  // right
            g.wallAbsorption[4] = 0.15f;  // floor (carpet)
            g.wallAbsorption[5] = 0.25f;  // ceiling (acoustic tiles)
            // Listener at 2/3 depth, centered
            g.listenerX = g.width  * 0.50f;
            g.listenerY = g.depth  * 0.67f;
            g.listenerZ = g.height * 0.40f;  // ear height sitting
            // Virtual speakers at ±30° (≈ 1.15m apart for 2m listening distance)
            g.speakerLx = g.listenerX - 0.58f;
            g.speakerRx = g.listenerX + 0.58f;
            break;

        case Medium:
            // Mid-size studio / listening room: 5×7×3m
            g.width  = 5.0f;  g.depth  = 7.0f;  g.height = 3.0f;
            g.wallAbsorption[0] = 0.20f;
            g.wallAbsorption[1] = 0.20f;
            g.wallAbsorption[2] = 0.15f;
            g.wallAbsorption[3] = 0.15f;
            g.wallAbsorption[4] = 0.12f;
            g.wallAbsorption[5] = 0.18f;
            g.listenerX = g.width  * 0.50f;
            g.listenerY = g.depth  * 0.67f;
            g.listenerZ = g.height * 0.40f;
            g.speakerLx = g.listenerX - 0.58f;
            g.speakerRx = g.listenerX + 0.58f;
            break;

        case Large:
            // Concert hall / large room: 8×12×4m
            g.width  = 8.0f;  g.depth  = 12.0f;  g.height = 4.0f;
            g.wallAbsorption[0] = 0.15f;
            g.wallAbsorption[1] = 0.15f;
            g.wallAbsorption[2] = 0.12f;
            g.wallAbsorption[3] = 0.12f;
            g.wallAbsorption[4] = 0.10f;  // hardwood floor
            g.wallAbsorption[5] = 0.15f;
            g.listenerX = g.width  * 0.50f;
            g.listenerY = g.depth  * 0.67f;
            g.listenerZ = g.height * 0.40f;
            g.speakerLx = g.listenerX - 0.58f;
            g.speakerRx = g.listenerX + 0.58f;
            break;
    }

    return g;
}

// ==============================================================================
// Frequency-dependent absorption interpolation (3-band model)
// ==============================================================================
float RoomProcessor::RoomGeometry::absorptionAt(float freqHz) const
{
    // Use front wall as reference for simplicity
    const float aLow  = wallAbsorption[0] * 0.7f;   // below 500 Hz: bass passes through more
    const float aMid  = wallAbsorption[0];           // 500Hz–4kHz: nominal
    const float aHigh = wallAbsorption[0] * 1.4f;   // above 4kHz: HF absorbed more

    // Smooth interpolation using logistic sigmoid
    auto sigmoid = [](float x, float center, float width) {
        return 1.0f / (1.0f + std::exp(-(x - center) / width));
    };

    const float wLow  = 1.0f - sigmoid(freqHz, 500.0f, 200.0f);
    const float wHigh = sigmoid(freqHz, 4000.0f, 1000.0f);
    const float wMid  = 1.0f - wLow - wHigh;

    return aLow * wLow + aMid * wMid + aHigh * wHigh;
}

// ==============================================================================
// prepare
// ==============================================================================
void RoomProcessor::prepare(double sr, int blockSize)
{
    sampleRate   = sr;
    maxBlockSize = blockSize;
    currentRoom  = -1;

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
// process
// ==============================================================================
void RoomProcessor::process(juce::AudioBuffer<float>& buffer,
                             float roomMix, int roomType)
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0 || roomMix < 0.001f)
        return;

    roomMix  = juce::jlimit(0.0f, 1.0f, roomMix);
    roomType = juce::jlimit(0, RoomType::Count - 1, roomType);

    if (roomType != currentRoom)
    {
        generateISM_IR(roomType);
        currentRoom = roomType;
    }

    wetBufferL.copyFrom(0, 0, buffer, 0, 0, numSamples);
    wetBufferR.copyFrom(0, 0, buffer, 1, 0, numSamples);

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

    auto* L  = buffer.getWritePointer(0);
    auto* R  = buffer.getWritePointer(1);
    const auto* wL = wetBufferL.getReadPointer(0);
    const auto* wR = wetBufferR.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        L[i] = L[i] * (1.0f - roomMix) + wL[i] * roomMix;
        R[i] = R[i] * (1.0f - roomMix) + wR[i] * roomMix;
    }
}

// ==============================================================================
// generateISM_IR — build stereo room IR via Image Source Method
// ==============================================================================
void RoomProcessor::generateISM_IR(int roomType)
{
    const auto& room = roomPreset(roomType);

    // RT60 estimate from Sabine formula: RT60 ≈ 0.161 * V / (S * alpha_avg)
    const float V = room.width * room.depth * room.height;
    const float S = 2.0f * (room.width * room.depth + room.width * room.height + room.depth * room.height);
    float alphaAvg = 0.0f;
    for (int i = 0; i < 6; ++i) alphaAvg += room.wallAbsorption[i];
    alphaAvg /= 6.0f;

    // Sabine with Eyring correction for higher absorption rooms
    const float alphaEyring = -std::log(1.0f - juce::jmin(alphaAvg, 0.95f));
    const float rt60Estimate = alphaEyring > 0.001f
        ? 0.161f * V / (S * alphaEyring)
        : 2.0f;  // fallback for very dead rooms

    const float rt60 = juce::jlimit(0.2f, 5.0f, rt60Estimate);
    const int   irLength = static_cast<int>(sampleRate * rt60 * 1.3f);
    const int   irLen    = juce::jmax(irLength, 1024);

    std::vector<float> irL(irLen, 0.0f);
    std::vector<float> irR(irLen, 0.0f);

    // Trace image sources for left and right virtual speakers
    std::vector<ImageSource> sourcesL, sourcesR;

    // Use slightly offset receiver positions per ear for stereo decorrelation
    // (human ears are ~0.15m apart; we offset receiver (±0.07m) from center)
    auto roomL = room;  roomL.listenerX -= 0.07f;
    auto roomR = room;  roomR.listenerX += 0.07f;

    // Place virtual speaker at L position for left-ear image sources
    {
        auto roomSpk = roomL;
        roomSpk.speakerLx = roomSpk.listenerX + 0.07f;  // undo ear offset for source placement
        roomSpk.speakerRx = roomSpk.listenerX + 0.07f;
        RoomGeometry spkRoom = roomL;
        spkRoom.listenerX = roomL.speakerLx;  // source at speaker L pos → listener at ear L
        traceImageSources(spkRoom, sourcesL, 2);
    }
    {
        auto roomSpk = roomR;
        roomSpk.speakerLx = roomSpk.listenerX - 0.07f;
        roomSpk.speakerRx = roomSpk.listenerX - 0.07f;
        RoomGeometry spkRoom = roomR;
        spkRoom.listenerX = roomR.speakerRx;  // source at speaker R pos → listener at ear R
        traceImageSources(spkRoom, sourcesR, 2);
    }

    // Bake into IR buffers
    bakeImageSources(sourcesL, -0.07f, irL.data(), irLen, sampleRate);
    bakeImageSources(sourcesR,  0.07f, irR.data(), irLen, sampleRate);

    // Late reverb tail: FDN (Feedback Delay Network) style filtered noise
    const float tailStartFrac = juce::jmin(0.15f, 50.0f / (rt60 * 1000.0f));
    const int   tailStart = static_cast<int>(irLen * tailStartFrac);
    const float hfRatio = juce::jlimit(0.3f, 0.8f, 1.0f - alphaAvg * 0.7f);

    bakeFDNTail(irL.data(), tailStart, irLen, sampleRate, rt60, hfRatio);
    bakeFDNTail(irR.data(), tailStart, irLen, sampleRate, rt60, hfRatio);

    // Normalise
    float peak = 0.0f;
    for (int i = 0; i < irLen; ++i)
    {
        peak = std::max(peak, std::abs(irL[i]));
        peak = std::max(peak, std::abs(irR[i]));
    }
    if (peak > 0.001f)
    {
        const float scale = 0.85f / peak;
        for (int i = 0; i < irLen; ++i) { irL[i] *= scale; irR[i] *= scale; }
    }

    // Load into convolution engines
    juce::AudioBuffer<float> irBufL(1, irLen), irBufR(1, irLen);
    irBufL.copyFrom(0, 0, irL.data(), irLen);
    irBufR.copyFrom(0, 0, irR.data(), irLen);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels      = 1;

    using Conv = juce::dsp::Convolution;
    convL.loadImpulseResponse(std::move(irBufL), sampleRate,
                              Conv::Stereo::no, Conv::Trim::no, Conv::Normalise::no);
    convL.prepare(spec);
    convR.loadImpulseResponse(std::move(irBufR), sampleRate,
                              Conv::Stereo::no, Conv::Trim::no, Conv::Normalise::no);
    convR.prepare(spec);

    juce::Logger::writeToLog("RoomProcessor: ISM IR (" + juce::String(irLen)
                             + " samp, RT60≈" + juce::String(rt60, 2)
                             + "s) for room type " + juce::String(roomType));
}

// ==============================================================================
// traceImageSources — compute all image sources up to maxOrder
// ==============================================================================
void RoomProcessor::traceImageSources(const RoomGeometry& room,
                                       std::vector<ImageSource>& outSources,
                                       int maxOrder)
{
    outSources.clear();
    outSources.reserve(static_cast<size_t>(std::pow(7, maxOrder)));

    // Surface normal directions and positions (6 surfaces)
    // indices: 0=front, 1=back, 2=left, 3=right, 4=floor, 5=ceiling
    struct Surface { float pos; int axis; float sign; };

    const Surface surfaces[6] = {
        { room.listenerY,          1,  1.0f },   // front wall (y = listenerY)
        { room.depth - room.listenerY, 1, -1.0f },  // back wall
        { room.listenerX,          0,  1.0f },   // left wall
        { room.width - room.listenerX,  0, -1.0f },  // right wall
        { room.listenerZ,          2,  1.0f },   // floor
        { room.height - room.listenerZ, 2, -1.0f },  // ceiling
    };

    // Source position (virtual speaker L) relative to listener
    const float srcPos[3] = { room.speakerLx, room.listenerY, room.listenerZ };

    // Brute-force: enumerate all combinations of surface reflections up to maxOrder
    // For order n, generate (6 choose n) * 2 combinations per surface pair...

    // Use iterative BFS: start with direct path, then reflect across each surface
    struct PathState
    {
        float pos[3];        // current image source position
        float dist;          // cumulative distance
        float amp;           // cumulative amplitude
        int   wallMask;      // bitmask of walls hit (for higher-order)
        int   order;
    };

    std::vector<PathState> queue;
    queue.reserve(static_cast<size_t>(std::pow(6, maxOrder + 1)));

    // Direct path
    {
        float dx = srcPos[0] - room.listenerX;
        float dy = 0.0f;
        float dz = srcPos[2] - room.listenerZ;
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

        outSources.push_back({
            dist / 343.0f,          // delay (speed of sound)
            1.0f / (dist + 0.1f),   // amplitude (1/r falloff)
            0.0f,                    // azimuth
            0                        // order 0 = direct
        });
    }

    // Seed queue with 1st-order reflections: mirror source across each surface
    for (int s = 0; s < 6; ++s)
    {
        const auto& surf = surfaces[s];

        float mirrored[3];
        mirrored[0] = srcPos[0]; mirrored[1] = srcPos[1]; mirrored[2] = srcPos[2];
        mirrored[surf.axis] = 2.0f * surf.pos - mirrored[surf.axis] + srcPos[surf.axis] - room.listenerX;

        // Recalculate: mirror position = reflect across surface plane
        // Surface plane equation: x_axis = wall position
        // Image source: x' = 2*wall - x
        // Wait, I need to be more careful with the coordinate system.
    }

    // --- Cleaner implementation: direct enumeration of all image positions ---

    // Coordinate system: listener at origin (0,0,0).
    // Room extends: -listenerX to (width - listenerX) in X,
    //               -listenerY to (depth - listenerY) in Y,
    //               -listenerZ to (height - listenerZ) in Z.
    // Source at: (speakerLx - listenerX, 0, speakerZ - listenerZ)

    const float lx = room.listenerX, ly = room.listenerY, lz = room.listenerZ;
    const float sx = room.speakerLx - lx;
    const float sy = 0.0f;
    const float sz = room.listenerZ - lz;  // speaker at listener height

    const float walls[6][2] = {
        { -lx,         1.0f },  // left wall:   x = -lx, normal = +x
        { room.width - lx, -1.0f },  // right wall:  x = +width - lx, normal = -x
        { -ly,         1.0f },  // front wall:  y = -ly
        { room.depth - ly, -1.0f },  // back wall:   y = +depth - ly
        { -lz,         1.0f },  // floor:       z = -lz
        { room.height - lz, -1.0f },  // ceiling:     z = +height - lz
    };

    // Add direct path
    outSources.clear();
    {
        float dx = sx;
        float dy = sy;
        float dz = sz;
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        float delay = dist / 343.0f;
        float amp   = 1.0f / (dist + 0.1f);
        float azim  = std::atan2(dx, dy);
        outSources.push_back({ delay, amp, azim, 0 });
    }

    // Enumerate all image sources: for each axis (x,y,z), reflection index n
    // Image position formula: x_img = (-1)^nx * (2*nx*W +/- x_src)
    // For simplicity, brute-force small integer combinations
    const int maxR = maxOrder;
    for (int nx = -maxR; nx <= maxR; ++nx)
    {
        for (int ny = -maxR; ny <= maxR; ++ny)
        {
            for (int nz = -maxR; nz <= maxR; ++nz)
            {
                if (nx == 0 && ny == 0 && nz == 0) continue;

                const int order = std::abs(nx) + std::abs(ny) + std::abs(nz);
                if (order > maxOrder) continue;

                const float W = room.width, D = room.depth, H = room.height;

                float px = (nx % 2 == 0) ? (nx * W + sx) : (nx * W + (W - sx - lx) + lx);
                float py = (ny % 2 == 0) ? (ny * D + sy) : (ny * D + (D - sy - ly) + ly);
                float pz = (nz % 2 == 0) ? (nz * H + sz) : (nz * H + (H - sz - lz) + lz);

                // Distance from listener (origin)
                float dist = std::sqrt(px*px + py*py + pz*pz);
                if (dist < 0.01f) continue;

                // Amplitude: 1/r falloff + wall absorption per reflection
                float amp = 1.0f / (dist + 0.1f);

                // Wall absorption per hit (simplified: average absorption^order)
                float alpha = 0.0f;
                for (int i = 0; i < 6; ++i) alpha += room.wallAbsorption[i];
                alpha /= 6.0f;
                amp *= std::pow(1.0f - alpha, static_cast<float>(order));

                // Air absorption: ~0.5 dB/m at 4kHz, negligible at low freq
                // Apply a mild high-frequency damping proportional to distance
                amp *= std::exp(-0.005f * dist);

                float delay = dist / 343.0f;
                float azim  = std::atan2(px, py);

                outSources.push_back({ delay, amp, azim, order });
            }
        }
    }

    // Sort by arrival time
    std::sort(outSources.begin(), outSources.end(),
              [](const ImageSource& a, const ImageSource& b) {
                  return a.delaySec < b.delaySec;
              });
}

// ==============================================================================
// bakeImageSources — render image sources into an IR buffer (mono per ear)
// ==============================================================================
void RoomProcessor::bakeImageSources(const std::vector<ImageSource>& sources,
                                      float /*earOffset*/, float* dest, int irLength,
                                      double sr)
{
    for (const auto& src : sources)
    {
        int sampleIdx = static_cast<int>(src.delaySec * sr);
        if (sampleIdx >= irLength) continue;

        // Place impulse
        dest[sampleIdx] += src.amplitude;

        // Simple low-pass spreading over next few samples (accounts for
        // the fact that real reflections aren't perfect impulses)
        const float spreadAlpha = 0.55f;
        float state = src.amplitude * (1.0f - spreadAlpha);
        for (int i = 1; i <= 6 && (sampleIdx + i) < irLength; ++i)
        {
            state *= spreadAlpha;
            dest[sampleIdx + i] += state;
        }
    }
}

// ==============================================================================
// bakeFDNTail — frequency-dependent decaying filtered noise
// ==============================================================================
void RoomProcessor::bakeFDNTail(float* dest, int startSample, int irLength,
                                 double sr, float rt60, float hfRatio)
{
    const int tailLen = irLength - startSample;
    if (tailLen <= 0) return;

    std::mt19937 rng(42);
    std::normal_distribution<float> noise(0.0f, 1.0f);

    const float decayRate = std::exp(-6.9078f / (rt60 * static_cast<float>(sr)));

    // Two-band LP: LF and HF with different decay times
    const float lfCutoff = 400.0f;   // bass decays slower
    const float hfCutoff = 5000.0f;  // treble decays faster
    const float lfAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * lfCutoff / static_cast<float>(sr));
    const float hfAlpha = std::exp(-2.0f * juce::MathConstants<float>::pi * hfCutoff / static_cast<float>(sr));

    float lpLf = 0.0f, lpHf = 0.0f;
    float env = 1.0f;

    for (int i = 0; i < tailLen; ++i)
    {
        const float raw = noise(rng) * env;
        lpLf = lfAlpha * lpLf + (1.0f - lfAlpha) * raw;
        lpHf = hfAlpha * lpHf + (1.0f - hfAlpha) * raw;

        // Blend low and high frequency components with HF damping
        const float hfGain = hfRatio * env + (1.0f - env) * 0.3f;
        const float mixed = lpLf * 0.3f + lpHf * hfGain * 0.7f;

        dest[startSample + i] += mixed * 0.35f;

        env *= decayRate;
    }
}
