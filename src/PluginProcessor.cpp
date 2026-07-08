#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// getDefaultSofaPath — bundled SOFA file
// ==============================================================================
juce::String OpenMixRoomAudioProcessor::getDefaultSofaPath() const
{
    // Try Resources directory first (macOS bundle)
    juce::File bundlePath = juce::File::getSpecialLocation(
        juce::File::currentApplicationFile);
    auto resourcesDir = bundlePath.getChildFile("Contents/Resources");
    auto sofaFile = resourcesDir.getChildFile("MIT_KEMAR_normal_pinna.sofa");
    if (sofaFile.existsAsFile())
        return sofaFile.getFullPathName();

    // Fallback: search relative to executable (development builds)
    // This is replaced by the SOFA_BUNDLE_PATH macro at build time.
#ifdef SOFA_BUNDLE_PATH
    juce::File devPath(SOFA_BUNDLE_PATH);
    if (devPath.existsAsFile())
        return devPath.getFullPathName();
#endif

    return {};
}

// ==============================================================================
// Constructor — register parameters
// ==============================================================================
OpenMixRoomAudioProcessor::OpenMixRoomAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // "Mix" parameter: dry/wet blend (0 % = fully dry, 100 % = full processing)
    addParameter (mixParam = new juce::AudioParameterFloat (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        100.0f,
        "%",
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + "%"; },
        nullptr));

    // Crossfeed amount: 0 = no crossfeed, 100 = max
    addParameter (crossfeedParam = new juce::AudioParameterFloat (
        "crossfeed", "Crossfeed",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        50.0f,
        "%",
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + "%"; },
        nullptr));

    // Crossfeed low-pass cutoff: 100–2000 Hz
    addParameter (cutoffParam = new juce::AudioParameterFloat (
        "cutoff", "Cutoff",
        juce::NormalisableRange<float> (100.0f, 2000.0f, 1.0f),
        700.0f,
        " Hz",
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (static_cast<int>(value)) + " Hz"; },
        nullptr));

    // Algorithm selector
    juce::StringArray algorithms = { "Bauer", "Meier", "Chu Moy", "HRTF" };
    addParameter (algorithmParam = new juce::AudioParameterChoice (
        "algorithm", "Algorithm", algorithms, 0));

    // Bypass toggle
    addParameter (bypassParam = new juce::AudioParameterBool (
        "bypass", "Bypass", false));

    // Room mix
    addParameter (roomMixParam = new juce::AudioParameterFloat (
        "roomMix", "Room Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        30.0f,
        "%",
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + "%"; },
        nullptr));

    // Room type
    juce::StringArray rooms = { "Small", "Medium", "Large" };
    addParameter (roomTypeParam = new juce::AudioParameterChoice (
        "roomType", "Room", rooms, 1));

}

// ==============================================================================
// prepareToPlay — called when playback starts or sample rate / block size change
// ==============================================================================
void OpenMixRoomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    // Load bundled SOFA file for HRTF mode (must precede crossfeed.prepare)
    juce::String sofaPath = getDefaultSofaPath();
    if (sofaPath.isNotEmpty())
    {
        bool loaded = sofaLoader.load(sofaPath, sampleRate);
        crossfeed.setSofaLoader(loaded ? &sofaLoader : nullptr);

        juce::Logger::writeToLog("OpenMixRoom: SOFA " + juce::String(loaded ? "loaded" : "not found")
                                 + " from " + sofaPath);
    }
    else
    {
        crossfeed.setSofaLoader(nullptr);
        juce::Logger::writeToLog("OpenMixRoom: No bundled SOFA file found");
    }

    // prepare crossfeed (now sofaLoader is available for HRIR pre-fetch)
    crossfeed.prepare(sampleRate, samplesPerBlock);

    // prepare room convolution engine
    room.prepare(sampleRate, samplesPerBlock);
}

// ==============================================================================
// releaseResources — free any dynamically allocated DSP objects
// ==============================================================================
void OpenMixRoomAudioProcessor::releaseResources()
{
    crossfeed.reset();
    room.reset();
    sofaLoader.close();
}

// ==============================================================================
// processBlock — Phase 2: Crossfeed with dry/wet mix
// ==============================================================================
void OpenMixRoomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Bus mismatch → silence extra outputs
    if (totalNumInputChannels != totalNumOutputChannels)
    {
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());
        return;
    }

    // Bypass — pass audio through unmodified
    if (bypassParam->get())
        return;

    const float mixValue       = mixParam->get() / 100.0f;
    const float crossfeedValue = crossfeedParam->get() / 100.0f;
    const float roomMixValue   = roomMixParam->get() / 100.0f;
    const float cutoffHz       = cutoffParam->get();
    const int   algorithm      = algorithmParam->getIndex();
    const int   roomType       = roomTypeParam->getIndex();

    // Fully dry — skip processing
    if (mixValue < 0.005f)
        return;

    // Save dry buffer for dry/wet blending
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Apply crossfeed to the wet path (in-place on buffer)
    if (crossfeedValue > 0.01f)
    {
        crossfeed.process(buffer, dryBuffer, crossfeedValue, cutoffHz, algorithm);
    }

    // Apply room convolution (early reflections + tail)
    if (roomMixValue > 0.005f)
    {
        room.process(buffer, roomMixValue, roomType);
    }

    // Dry/wet blend
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

    // Silence extra output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

// ==============================================================================
// createEditor — build the GUI
// ==============================================================================
juce::AudioProcessorEditor* OpenMixRoomAudioProcessor::createEditor()
{
    return new OpenMixRoomAudioProcessorEditor (*this);
}

// ==============================================================================
// State persistence — store / recall parameter values
// ==============================================================================
void OpenMixRoomAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement> ("OpenMixRoomState");
    xml->setAttribute ("mix",       mixParam->get());
    xml->setAttribute ("crossfeed", crossfeedParam->get());
    xml->setAttribute ("cutoff",    cutoffParam->get());
    xml->setAttribute ("algorithm", algorithmParam->getIndex());
    xml->setAttribute ("bypass",    bypassParam->get());
    xml->setAttribute ("roomMix",   roomMixParam->get());
    xml->setAttribute ("roomType",  roomTypeParam->getIndex());
    copyXmlToBinary (*xml, destData);
}

void OpenMixRoomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName ("OpenMixRoomState"))
    {
        *mixParam       = static_cast<float> (xml->getDoubleAttribute ("mix", 50.0));
        *crossfeedParam = static_cast<float> (xml->getDoubleAttribute ("crossfeed", 50.0));
        *cutoffParam    = static_cast<float> (xml->getDoubleAttribute ("cutoff", 700.0));
        *algorithmParam = xml->getIntAttribute ("algorithm", 0);
        *bypassParam    = xml->getBoolAttribute ("bypass", false);
        *roomMixParam   = static_cast<float> (xml->getDoubleAttribute ("roomMix", 30.0));
        *roomTypeParam  = xml->getIntAttribute ("roomType", 1);
    }
}

// ==============================================================================
// JUCE plugin entry point
// ==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OpenMixRoomAudioProcessor();
}
