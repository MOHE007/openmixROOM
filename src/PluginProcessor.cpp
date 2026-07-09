#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// getDefaultSofaPath
// ==============================================================================
juce::String OpenMixRoomAudioProcessor::getDefaultSofaPath() const
{
    juce::File bundlePath = juce::File::getSpecialLocation(
        juce::File::currentApplicationFile);
    auto sofaFile = bundlePath.getChildFile("Contents/Resources")
                               .getChildFile("MIT_KEMAR_normal_pinna.sofa");
    if (sofaFile.existsAsFile())
        return sofaFile.getFullPathName();

#ifdef SOFA_BUNDLE_PATH
    juce::File devPath(SOFA_BUNDLE_PATH);
    if (devPath.existsAsFile())
        return devPath.getFullPathName();
#endif
    return {};
}

// ==============================================================================
// Constructor — register all parameters (v0.4: + calibration)
// ==============================================================================
OpenMixRoomAudioProcessor::OpenMixRoomAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Mix
    addParameter (mixParam = new juce::AudioParameterFloat (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
        "%", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "%"; }, nullptr));

    // Crossfeed
    addParameter (crossfeedParam = new juce::AudioParameterFloat (
        "crossfeed", "Crossfeed",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f,
        "%", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "%"; }, nullptr));

    // Cutoff
    addParameter (cutoffParam = new juce::AudioParameterFloat (
        "cutoff", "Cutoff",
        juce::NormalisableRange<float> (100.0f, 2000.0f, 1.0f), 700.0f,
        " Hz", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(static_cast<int>(v)) + " Hz"; }, nullptr));

    // Algorithm
    addParameter (algorithmParam = new juce::AudioParameterChoice (
        "algorithm", "Algorithm",
        juce::StringArray { "Bauer", "Meier", "Chu Moy", "HRTF" }, 0));

    // Bypass
    addParameter (bypassParam = new juce::AudioParameterBool (
        "bypass", "Bypass", false));

    // Room mix
    addParameter (roomMixParam = new juce::AudioParameterFloat (
        "roomMix", "Room Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f,
        "%", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "%"; }, nullptr));

    // Room type
    addParameter (roomTypeParam = new juce::AudioParameterChoice (
        "roomType", "Room", juce::StringArray { "Small", "Medium", "Large", "Extra Large" }, 1));

    // Room enable (default OFF — user must turn on manually)
    addParameter (roomEnabledParam = new juce::AudioParameterBool (
        "roomEnabled", "Room On", false));

    // Room size (0.5x – 2.0x, default 1.0x = Medium preset)
    addParameter (roomSizeParam = new juce::AudioParameterFloat (
        "roomSize", "Room Size",
        juce::NormalisableRange<float> (0.5f, 2.0f, 0.05f), 1.0f,
        "x", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 2) + "x"; }, nullptr));

    // Pre-delay (0–50ms, default 20ms = Medium preset)
    addParameter (preDelayParam = new juce::AudioParameterFloat (
        "preDelay", "Pre-Delay",
        juce::NormalisableRange<float> (0.0f, 50.0f, 1.0f), 20.0f,
        " ms", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(static_cast<int>(v)) + " ms"; }, nullptr));

    // Room damping LPF (2k–20kHz, default 7kHz = Medium preset)
    addParameter (roomDampParam = new juce::AudioParameterFloat (
        "roomDamp", "Damping",
        juce::NormalisableRange<float> (2000.0f, 20000.0f, 100.0f), 7000.0f,
        " Hz", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(static_cast<int>(v / 1000.0f * 10) / 10.0) + " kHz"; }, nullptr));

    // Early reflections level (0–100%, default 50%)
    addParameter (erLevelParam = new juce::AudioParameterFloat (
        "erLevel", "ER Level",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 50.0f,
        "%", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "%"; }, nullptr));

    // Headphone calibration profile
    juce::StringArray calProfiles;
    for (int i = 0; i < headphoneCal.getProfileCount(); ++i)
        calProfiles.add (headphoneCal.getProfile(i).name);
    addParameter (calProfileParam = new juce::AudioParameterChoice (
        "calProfile", "HP Profile", calProfiles, 0));

    // Calibration on/off
    addParameter (calEnabledParam = new juce::AudioParameterBool (
        "calEnabled", "HP Cal On", true));

    // Calibration gain (strength)
    addParameter (calGainParam = new juce::AudioParameterFloat (
        "calGain", "HP Cal Gain",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
        "%", juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "%"; }, nullptr));

    // Register parameter listener to bridge APVTS ↔ DSP
    calListener = std::make_unique<CalListener>(headphoneCal);
    calListener->profileParam = calProfileParam;
    calListener->enabledParam = calEnabledParam;
    calProfileParam->addListener(calListener.get());
    calEnabledParam->addListener(calListener.get());
}

// ==============================================================================
// Atomic calibration setters — syncs parameter tree + DSP in one step
// ==============================================================================
void OpenMixRoomAudioProcessor::setCalProfile(int index)
{
    // Built-in profiles (0..10) are also stored in the APVTS parameter for DAW save/restore.
    // Custom profiles (11+) are DSP-only — they won't survive sessions, but that's OK.
    const int builtInCount = headphoneCal.getBuiltInCount();
    if (index >= 0 && index < builtInCount)
        *calProfileParam = index;
    headphoneCal.setProfile(index);
}

void OpenMixRoomAudioProcessor::setCalEnabled(bool on)
{
    *calEnabledParam = on;
    headphoneCal.setEnabled(on);
}

void OpenMixRoomAudioProcessor::setCalGain(float percent)
{
    *calGainParam = percent;
    headphoneCal.setGain(percent / 100.0f);
}

void OpenMixRoomAudioProcessor::setRoomEnabled(bool on)
{
    *roomEnabledParam = on;
}

void OpenMixRoomAudioProcessor::setRoomMix(float percent)
{
    *roomMixParam = percent;
}

void OpenMixRoomAudioProcessor::setRoomType(int index)
{
    *roomTypeParam = index;

    // Sync dependent params to preset values
    auto p = RoomProcessor::presetFor(index);
    *roomSizeParam = p.size;
    *preDelayParam = p.preDelayMs;
    *roomDampParam = p.lpfHz;
    *erLevelParam  = p.erLevel * 100.0f;

    room.loadPreset(index);
}

void OpenMixRoomAudioProcessor::setRoomSize(float s)
{
    *roomSizeParam = s;
    room.setRoomSize(s);
}

void OpenMixRoomAudioProcessor::setPreDelay(float ms)
{
    *preDelayParam = ms;
    room.setPreDelay(ms);
}

void OpenMixRoomAudioProcessor::setRoomDamp(float hz)
{
    *roomDampParam = hz;
    room.setDamping(hz);
}

void OpenMixRoomAudioProcessor::setERLevel(float percent)
{
    *erLevelParam = percent;
    room.setERLevel(percent / 100.0f);
}

juce::Result OpenMixRoomAudioProcessor::importCalProfile(const juce::String& filePath)
{
    HeadphoneCalibration::Profile profile;
    auto result = HeadphoneCalibration::parseAutoEqFile(filePath, profile);
    if (result.failed())
        return result;

    int idx = headphoneCal.addCustomProfile(std::move(profile));
    setCalProfile(idx);
    return juce::Result::ok();
}

// ==============================================================================
// prepareToPlay
// ==============================================================================
void OpenMixRoomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    // SOFA
    juce::String sofaPath = getDefaultSofaPath();
    if (sofaPath.isNotEmpty())
    {
        bool loaded = sofaLoader.load(sofaPath, sampleRate);
        crossfeed.setSofaLoader(loaded ? &sofaLoader : nullptr);
        juce::Logger::writeToLog("OpenMixRoom: SOFA "
            + juce::String(loaded ? "loaded" : "not found"));
    }
    else
    {
        crossfeed.setSofaLoader(nullptr);
    }

    // Headphone calibration
    headphoneCal.prepare(sampleRate, samplesPerBlock);
    headphoneCal.setProfile(calProfileParam->getIndex());
    headphoneCal.setEnabled(calEnabledParam->get());
    headphoneCal.setGain(calGainParam->get() / 100.0f);

    // Crossfeed
    crossfeed.prepare(sampleRate, samplesPerBlock);

    // Room
    room.prepare(sampleRate, samplesPerBlock);
}

// ==============================================================================
// releaseResources
// ==============================================================================
void OpenMixRoomAudioProcessor::releaseResources()
{
    headphoneCal.reset();
    crossfeed.reset();
    room.reset();
    sofaLoader.close();
}

// ==============================================================================
// processBlock — v0.4 signal chain:
//   Input → Headphone Cal EQ → Crossfeed → Room IR → Dry/Wet Mix → Output
// ==============================================================================
void OpenMixRoomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numIn  = getTotalNumInputChannels();
    const auto numOut = getTotalNumOutputChannels();
    if (numIn != numOut)
    {
        for (auto i = numIn; i < numOut; ++i)
            buffer.clear(i, 0, buffer.getNumSamples());
        return;
    }

    if (bypassParam->get())
        return;

    const float mixValue       = mixParam->get()       / 100.0f;
    const float crossfeedValue = crossfeedParam->get() / 100.0f;
    const float roomMixValue   = roomMixParam->get()   / 100.0f;
    const float cutoffHz       = cutoffParam->get();
    const int   algorithm      = algorithmParam->getIndex();

    if (mixValue < 0.005f)
        return;

    // ---- Update headphone calibration state from APVTS ----
    // CalListener handles profile/enabled changes via parameter callbacks.
    // Only sync gain here since it changes per-frame from slider.
    headphoneCal.setGain(calGainParam->get() / 100.0f);

    // [1] Headphone calibration EQ (in-place)
    headphoneCal.process(buffer);

    // Save dry for blend
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // [2] Crossfeed
    if (crossfeedValue > 0.01f)
        crossfeed.process(buffer, dryBuffer, crossfeedValue, cutoffHz, algorithm);

    // [3] Room convolution (only if enabled)
    const bool roomEnabled = roomEnabledParam->get();
    if (roomEnabled && roomMixValue > 0.005f)
    {
        room.setRoomSize(roomSizeParam->get());
        room.setPreDelay(preDelayParam->get());
        room.setDamping(roomDampParam->get());
        room.setERLevel(erLevelParam->get() / 100.0f);
        room.process(buffer, roomMixValue, true);
    }

    // [4] Dry/wet blend
    if (mixValue < 0.995f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* dst = buffer.getWritePointer(ch);
            auto* src = dryBuffer.getReadPointer(ch);
            juce::FloatVectorOperations::multiply(dst, mixValue, buffer.getNumSamples());
            juce::FloatVectorOperations::addWithMultiply(dst, src, 1.0f - mixValue, buffer.getNumSamples());
        }
    }

    for (auto i = numIn; i < numOut; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // ---- [5] Soft limit final output (prevents digital clipping in chain cascade) ----
    // Use x / (1 + |x|) — softer knee than tanh, less harmonic distortion
    for (int ch = 0; ch < numOut; ++ch)
    {
        auto* dst = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float x = dst[i];
            dst[i] = x / (1.0f + std::abs(x));
        }
    }
}

// ==============================================================================
// createEditor
// ==============================================================================
juce::AudioProcessorEditor* OpenMixRoomAudioProcessor::createEditor()
{
    return new OpenMixRoomAudioProcessorEditor (*this);
}

// ==============================================================================
// State persistence
// ==============================================================================
void OpenMixRoomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement>("OpenMixRoomState");
    xml->setAttribute("mix",         mixParam->get());
    xml->setAttribute("crossfeed",   crossfeedParam->get());
    xml->setAttribute("cutoff",      cutoffParam->get());
    xml->setAttribute("algorithm",   algorithmParam->getIndex());
    xml->setAttribute("bypass",      bypassParam->get());
    xml->setAttribute("roomMix",     roomMixParam->get());
    xml->setAttribute("roomType",    roomTypeParam->getIndex());
    xml->setAttribute("roomEnabled", roomEnabledParam->get());
    xml->setAttribute("roomSize",    roomSizeParam->get());
    xml->setAttribute("preDelay",    preDelayParam->get());
    xml->setAttribute("roomDamp",    roomDampParam->get());
    xml->setAttribute("erLevel",     erLevelParam->get());
    xml->setAttribute("calProfile",  calProfileParam->getIndex());
    xml->setAttribute("calEnabled",  calEnabledParam->get());
    xml->setAttribute("calGain",     calGainParam->get());
    copyXmlToBinary(*xml, destData);
}

void OpenMixRoomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName("OpenMixRoomState"))
    {
        *mixParam       = static_cast<float>(xml->getDoubleAttribute("mix", 100.0));
        *crossfeedParam = static_cast<float>(xml->getDoubleAttribute("crossfeed", 50.0));
        *cutoffParam    = static_cast<float>(xml->getDoubleAttribute("cutoff", 700.0));
        *algorithmParam = xml->getIntAttribute("algorithm", 0);
        *bypassParam    = xml->getBoolAttribute("bypass", false);
        *roomMixParam   = static_cast<float>(xml->getDoubleAttribute("roomMix", 30.0));
        *roomTypeParam  = xml->getIntAttribute("roomType", 1);
        *roomEnabledParam = xml->getBoolAttribute("roomEnabled", false);
        *roomSizeParam   = static_cast<float>(xml->getDoubleAttribute("roomSize", 1.0));
        *preDelayParam   = static_cast<float>(xml->getDoubleAttribute("preDelay", 20.0));
        *roomDampParam   = static_cast<float>(xml->getDoubleAttribute("roomDamp", 7000.0));
        *erLevelParam    = static_cast<float>(xml->getDoubleAttribute("erLevel", 50.0));
        *calProfileParam= xml->getIntAttribute("calProfile", 0);
        *calEnabledParam= xml->getBoolAttribute("calEnabled", true);
        *calGainParam   = static_cast<float>(xml->getDoubleAttribute("calGain", 100.0));
    }
}

// ==============================================================================
// Plugin entry point
// ==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OpenMixRoomAudioProcessor();
}
