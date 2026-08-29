// ==============================================================================
// DSPRegressionTest — automated regression tests for OpenMix Room DSP modules.
//
// Run: $ ./DSPRegressionTest
// Exit code: 0 = all pass, non-zero = test failure.
//
// Uses explicit test registration (not global statics) to avoid static-init
// order issues in a headless console app.
// ==============================================================================

#include <cstdio>
#include <cmath>

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "HeadphoneCalibration.h"
#include "RoomProcessor.h"
#include "CrossfeedProcessor.h"

// ==============================================================================
// Helpers
// ==============================================================================

namespace TestHelpers
{
    inline void fillSine (juce::AudioBuffer<float>& buffer,
                          double sampleRate, float freqHz, float amplitude = 0.5f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                data[i] = amplitude * std::sin (2.0 * juce::MathConstants<double>::pi
                                                * freqHz * i / sampleRate);
        }
    }

    inline void fillLeftOnly (juce::AudioBuffer<float>& buffer,
                              double sampleRate, float freqHz, float amplitude = 0.5f)
    {
        auto* L = buffer.getWritePointer (0);
        buffer.clear (1, 0, buffer.getNumSamples());
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            L[i] = amplitude * std::sin (2.0 * juce::MathConstants<double>::pi
                                         * freqHz * i / sampleRate);
    }

    inline float peakAbs (const juce::AudioBuffer<float>& buffer, int channel = 0)
    {
        const auto* data = buffer.getReadPointer (channel);
        float peak = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = juce::jmax (peak, std::abs (data[i]));
        return peak;
    }

    inline bool hasNaNOrInf (const juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                if (! std::isfinite (data[i]))
                    return true;
        }
        return false;
    }
}

// ==============================================================================
// Test runner
// ==============================================================================

struct TestResult { const char* name; bool passed; const char* detail; };
static std::vector<TestResult> results;

static void record(const char* name, bool cond, const char* detail = "")
{
    results.push_back({name, cond, detail});
    printf("  [%s] %s%s%s\n",
           cond ? "PASS" : "FAIL",
           name,
           (detail && detail[0]) ? " — " : "",
           detail);
}

// ==============================================================================
// Test 1: Silence in → silence out (HeadphoneCalibration)
// ==============================================================================
static void test_cal_silence()
{
    HeadphoneCalibration cal;
    cal.prepare(44100.0, 256);
    cal.setEnabled(true);
    cal.setGain(1.0f);

    for (int i = 0; i < cal.getBuiltInCount(); ++i)
    {
        juce::AudioBuffer<float> buf(2, 256);
        buf.clear();
        cal.setProfile(i);
        cal.reset();
        cal.process(buf);

        float pk = TestHelpers::peakAbs(buf);
        record(juce::String("Silence profile " + cal.getProfileName(i)).toRawUTF8(),
               pk <= 1e-10f,
               pk > 1e-10f ? juce::String("peak=" + juce::String(pk)).toRawUTF8() : "");
    }
}

// ==============================================================================
// Test 2: RoomProcessor silence
// ==============================================================================
static void test_room_silence()
{
    RoomProcessor room;
    room.prepare(44100.0, 256);

    for (int t = 0; t < RoomProcessor::Count; ++t)
    {
        juce::AudioBuffer<float> buf(2, 256);
        buf.clear();
        room.loadPreset(t);
        room.reset();
        for (int blk = 0; blk < 20; ++blk)
            room.process(buf, 1.0f, true);

        float pk = TestHelpers::peakAbs(buf);
        record(juce::String("Room silence type=" + juce::String(t)).toRawUTF8(),
               pk < 1e-6f && !TestHelpers::hasNaNOrInf(buf),
               pk >= 1e-6f ? juce::String("peak=" + juce::String(pk)).toRawUTF8() : "");
    }
}

// ==============================================================================
// Test 3: Channel integrity (crossfeed off)
// ==============================================================================
static void test_channel_integrity()
{
    CrossfeedProcessor xf;
    xf.prepare(44100.0, 512);

    juce::AudioBuffer<float> buf(2, 512);
    juce::AudioBuffer<float> dry(2, 512);
    TestHelpers::fillLeftOnly(buf, 44100.0, 440.0f);
    dry.makeCopyOf(buf);

    xf.process(buf, dry, 0.0f, 700.0f, 0);  // Crossfeed OFF

    float pkR = TestHelpers::peakAbs(buf, 1);
    float pkL = TestHelpers::peakAbs(buf, 0);
    record("Channel integrity: R silent when XF off",
           pkR <= 1e-10f && pkL > 0.1f,
           pkR > 1e-10f ? "leak detected" : "");
}

// ==============================================================================
// Test 4: All built-in profiles load + process
// ==============================================================================
static void test_profile_regression()
{
    HeadphoneCalibration cal;
    cal.prepare(44100.0, 256);

    const int expectedCount = cal.getBuiltInCount();

    for (int i = 0; i < cal.getBuiltInCount(); ++i)
    {
        cal.setProfile(i);
        cal.setEnabled(true);
        cal.setGain(1.0f);

        juce::AudioBuffer<float> buf(2, 256);
        TestHelpers::fillSine(buf, 44100.0, 500.0f, 0.5f);
        cal.process(buf);

        bool ok = !TestHelpers::hasNaNOrInf(buf) && TestHelpers::peakAbs(buf) > 0.05f
                  && TestHelpers::peakAbs(buf) <= 4.0f && cal.getProfileName(i).isNotEmpty();
        record(juce::String("Profile " + cal.getProfileName(i)).toRawUTF8(), ok, "");
    }
}

// ==============================================================================
// Test 5: Room type switching
// ==============================================================================
static void test_room_switch()
{
    RoomProcessor room;
    room.prepare(44100.0, 256);

    const int types[] = {0, 1, 2, 3, 0, 2, 1, 3, 2, 0};
    bool allOk = true;

    for (int i = 0; i < 10; ++i)
    {
        juce::AudioBuffer<float> buf(2, 256);
        buf.clear();
        TestHelpers::fillSine(buf, 44100.0, 1000.0f, 0.3f);
        room.loadPreset(types[i]);
        room.process(buf, 0.5f, true);

        if (TestHelpers::hasNaNOrInf(buf) || TestHelpers::peakAbs(buf) > 5.0f)
            allOk = false;
    }
    record("Room type switching no burst/NaN", allOk, "");
}

// ==============================================================================
// Test 6: Parameter boundaries
// ==============================================================================
static void test_boundaries()
{
    bool allOk = true;

    // RoomProcessor extremes
    {
        RoomProcessor room;
        room.prepare(44100.0, 256);
        juce::AudioBuffer<float> buf(2, 256);
        room.setRoomSize(0.0f); room.setRoomSize(100.0f);
        room.setPreDelay(0.0f); room.setPreDelay(500.0f);
        room.setDamping(0.0f);  room.setDamping(1.0f);
        room.setERLevel(-1.0f); room.setERLevel(2.0f);
        TestHelpers::fillSine(buf, 44100.0, 440.0f, 0.5f);
        room.process(buf, 1.0f, true);
        if (TestHelpers::hasNaNOrInf(buf)) allOk = false;
    }

    // HeadphoneCalibration extremes
    {
        HeadphoneCalibration cal;
        cal.prepare(44100.0, 256);
        juce::AudioBuffer<float> buf(2, 256);
        cal.setGain(-1.0f); cal.setGain(2.0f);
        cal.setProfile(-1);
        TestHelpers::fillSine(buf, 44100.0, 440.0f, 0.5f);
        cal.process(buf);
        if (TestHelpers::hasNaNOrInf(buf)) allOk = false;
        cal.setProfile(999);
        cal.process(buf);
        if (TestHelpers::hasNaNOrInf(buf)) allOk = false;
    }

    // CrossfeedProcessor extremes
    {
        CrossfeedProcessor xf;
        xf.prepare(44100.0, 256);
        juce::AudioBuffer<float> buf(2, 256);
        juce::AudioBuffer<float> dry(2, 256);
        TestHelpers::fillSine(buf, 44100.0, 440.0f, 0.5f);
        dry.makeCopyOf(buf);
        xf.process(buf, dry, -1.0f, 700.0f, 0);
        xf.process(buf, dry, 2.0f, 700.0f, 0);
        xf.process(buf, dry, 0.5f, -100.0f, 0);
        xf.process(buf, dry, 0.5f, 30000.0f, 0);
        xf.process(buf, dry, 0.5f, 700.0f, -1);
        xf.process(buf, dry, 0.5f, 700.0f, 99);
        if (TestHelpers::hasNaNOrInf(buf)) allOk = false;
    }

    record("Parameter boundaries no crash/NaN", allOk, "");
}

// ==============================================================================
// Test 7: Bypass passthrough
// ==============================================================================
static void test_bypass()
{
    juce::AudioBuffer<float> buf(2, 256);
    TestHelpers::fillSine(buf, 44100.0, 1000.0f, 0.25f);
    juce::AudioBuffer<float> ref(2, 256);
    ref.makeCopyOf(buf);

    RoomProcessor room;
    room.prepare(44100.0, 256);
    room.loadPreset(RoomProcessor::Small);
    room.process(buf, 0.0f, false);

    HeadphoneCalibration cal;
    cal.prepare(44100.0, 256);
    cal.setEnabled(false);
    cal.process(buf);

    double maxDiff = 0.0;
    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* refData = ref.getReadPointer(ch);
        const auto* outData = buf.getReadPointer(ch);
        for (int i = 0; i < 256; ++i)
            maxDiff = juce::jmax(maxDiff, std::abs(static_cast<double>(outData[i] - refData[i])));
    }
    record("Bypass passthrough (maxDiff < 1e-6)", maxDiff <= 1e-6,
           maxDiff > 1e-6 ? juce::String("maxDiff=" + juce::String(maxDiff)).toRawUTF8() : "");
}

// ==============================================================================
// Test 8: Sample rate switching
// ==============================================================================
static void test_sample_rates()
{
    const double rates[] = {44100.0, 48000.0, 96000.0};
    const char* labels[] = {"44.1k", "48k", "96k"};
    bool allOk = true;

    for (int ri = 0; ri < 3; ++ri)
    {
        double sr = rates[ri];
        int bs = 512;

        {
            RoomProcessor room;
            room.prepare(sr, bs);
            juce::AudioBuffer<float> buf(2, bs);
            TestHelpers::fillSine(buf, sr, 1000.0f, 0.3f);
            room.loadPreset(RoomProcessor::Medium);
            room.process(buf, 0.3f, true);
            if (TestHelpers::hasNaNOrInf(buf) || TestHelpers::peakAbs(buf) <= 1e-6f) allOk = false;
        }
        {
            HeadphoneCalibration cal;
            cal.prepare(sr, bs);
            juce::AudioBuffer<float> buf(2, bs);
            TestHelpers::fillSine(buf, sr, 500.0f, 0.5f);
            cal.setProfile(0);
            cal.setEnabled(true);
            cal.setGain(1.0f);
            cal.process(buf);
            if (TestHelpers::hasNaNOrInf(buf)) allOk = false;
        }
        {
            CrossfeedProcessor xf;
            xf.prepare(sr, bs);
            juce::AudioBuffer<float> buf(2, bs);
            juce::AudioBuffer<float> dry(2, bs);
            TestHelpers::fillSine(buf, sr, 440.0f, 0.5f);
            dry.makeCopyOf(buf);
            xf.process(buf, dry, 0.5f, 700.0f, 2);
            if (TestHelpers::hasNaNOrInf(buf)) allOk = false;
        }
    }
    record("Sample rate switching 44.1k/48k/96k", allOk, "");
}

// ==============================================================================
// Entry Point
// ==============================================================================
int main (int /*argc*/, char** /*argv*/)
{
    printf("Starting...\n");
    fflush(stdout);
    juce::ScopedJuceInitialiser_GUI init;
    printf("DSP Regression Tests\n====================\n\n");

    test_cal_silence();
    test_room_silence();
    test_channel_integrity();
    test_profile_regression();
    test_room_switch();
    test_boundaries();
    test_bypass();
    test_sample_rates();

    int passed = 0, failed = 0;
    for (auto& r : results) { if (r.passed) ++passed; else ++failed; }

    printf("\n--------------------\n");
    printf("Total: %zu  Passed: %d  Failed: %d\n", results.size(), passed, failed);

    if (failed > 0)
    {
        printf("\nFAILED TESTS:\n");
        for (auto& r : results)
            if (!r.passed) printf("  - %s\n", r.name);
    }

    printf("\n=== %s ===\n", failed == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    return failed == 0 ? 0 : 1;
}
