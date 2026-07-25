#include "PluginProcessor.h"
#include "PluginEditor.h"

ShineAudioProcessor::ShineAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameters()) {
}

ShineAudioProcessor::~ShineAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout ShineAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("presence", 1), "Presence",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.67f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("air", 1), "Air",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.89f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("mode", 1), "Mode", 0, 1, 0));

    return { params.begin(), params.end() };
}

void ShineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    dspChainL.prepare(sampleRate);
    dspChainR.prepare(sampleRate);

    sae.prepare(sampleRate, samplesPerBlock);

    if (saePresenceIdx < 0) {
        saePresenceIdx = sae.registerParameter(
            "presence", 0.67f,
            {0.0f, 1.0f}, 0.10f, true);
        saeAirIdx = sae.registerParameter(
            "air", 0.89f,
            {0.0f, 1.0f}, 0.10f, true);
    }
}

void ShineAudioProcessor::releaseResources() {}

void ShineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    float presenceKnob = apvts.getRawParameterValue("presence")->load();
    float airKnob      = apvts.getRawParameterValue("air")->load();
    bool  bypass       = apvts.getRawParameterValue("bypass")->load() > 0.5f;

    float inLevel = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        inLevel = juce::jmax(inLevel, buffer.getMagnitude(ch, 0, numSamples));
    inputLevel.store(inLevel);

    if (bypass) {
        outputLevel.store(inLevel);
        return;
    }

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    sae.updateBaseValue(saePresenceIdx, presenceKnob);
    sae.updateBaseValue(saeAirIdx, airKnob);

    const float* leftRead = buffer.getReadPointer(0);
    sae.process(leftRead, numSamples);

    const float* presenceMod = sae.getModulatedBuffer(saePresenceIdx);
    const float* airMod      = sae.getModulatedBuffer(saeAirIdx);

    float intentLvl = sae.getIntentLevel();
    float invertedIntent = 1.0f - intentLvl;
    adaptPresenceScale.store(invertedIntent);
    adaptAirScale.store(invertedIntent);

    float* leftCh  = buffer.getWritePointer(0);
    float* rightCh = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    dspChainL.processBlock(leftCh, numSamples, presenceMod, airMod);
    if (rightCh)
        dspChainR.processBlock(rightCh, numSamples, presenceMod, airMod);

    bool outputBad = false;
    for (int ch = 0; ch < numChannels && !outputBad; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            if (std::isnan(data[i]) || std::isinf(data[i])) { outputBad = true; break; }
        }
    }
    if (outputBad) {
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.copyFrom(ch, 0, dryBuffer, ch, 0, numSamples);
    }

    float outLevel = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        outLevel = juce::jmax(outLevel, buffer.getMagnitude(ch, 0, numSamples));
    outputLevel.store(outLevel);
}

juce::AudioProcessorEditor* ShineAudioProcessor::createEditor() {
    return new ShineAudioProcessorEditor(*this);
}

void ShineAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ShineAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ShineAudioProcessor();
}
