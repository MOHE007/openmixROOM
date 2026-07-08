#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// Constructor — register parameters
// ==============================================================================
OpenMixRoomAudioProcessor::OpenMixRoomAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // "Mix" parameter: controls the dry/wet blend (0 % = fully dry, 100 % = full processing).
    // In Phase 1 processing is a straight pass-through so the knob is cosmetic,
    // but the logic already respects it: mix < 0.5 % → skip copy entirely.
    addParameter (mixParam = new juce::AudioParameterFloat (
        "mix",                                    // parameter ID
        "Mix",                                    // display name
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        100.0f,                                   // default
        "%",                                      // suffix
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String (value, 1) + "%"; },
        nullptr));
}

// ==============================================================================
// prepareToPlay — called when playback starts or sample rate / block size change
// ==============================================================================
void OpenMixRoomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;
}

// ==============================================================================
// releaseResources — free any dynamically allocated DSP objects
// ==============================================================================
void OpenMixRoomAudioProcessor::releaseResources()
{
    // Nothing to release in Phase 1.
}

// ==============================================================================
// processBlock — Phase 1: straight audio pass-through
// ==============================================================================
void OpenMixRoomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    // If the bus layout is mismatched, silence the output and bail out.
    if (totalNumInputChannels != totalNumOutputChannels)
    {
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear (i, 0, buffer.getNumSamples());
        return;
    }

    const float mixValue = mixParam->get();

    // Fully dry — skip processing entirely.
    if (mixValue < 0.5f)
        return;

    // Phase 1: straight pass-through — the buffer already contains input audio.
    // Simply silence any extra output channels that have no matching input.
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
    xml->setAttribute ("mix", mixParam->get());
    copyXmlToBinary (*xml, destData);
}

void OpenMixRoomAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName ("OpenMixRoomState"))
        *mixParam = static_cast<float> (xml->getDoubleAttribute ("mix", 50.0));
}

// ==============================================================================
// JUCE plugin entry point
// ==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OpenMixRoomAudioProcessor();
}
