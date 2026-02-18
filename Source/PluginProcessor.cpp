#include "PluginProcessor.h"
#include "PluginEditor.h"

ShineAudioProcessor::ShineAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameters()) {
}

ShineAudioProcessor::~ShineAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout ShineAudioProcessor::createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Amount knob scales the ML or preset output
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("amount", 1), "Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    // Preset: 0=Auto (ML), 1=Vocal Clarity, 2=Acoustic Detail
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("preset", 1), "Preset", 0, 2, 0));

    // Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));

    return { params.begin(), params.end() };
}

void ShineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);
    dspChainL.prepare(sampleRate);
    dspChainR.prepare(sampleRate);
    featureExtractor.reset();
    modelInference.resetSmoothing();
}

void ShineAudioProcessor::releaseResources() {
}

void ShineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    float amount = apvts.getRawParameterValue("amount")->load();
    int presetIndex = static_cast<int>(apvts.getRawParameterValue("preset")->load());
    bool bypass = apvts.getRawParameterValue("bypass")->load() > 0.5f;

    // Calculate input level
    float inLevel = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        inLevel = juce::jmax(inLevel, buffer.getMagnitude(ch, 0, numSamples));
    inputLevel.store(inLevel);

    if (bypass) {
        outputLevel.store(inLevel);
        return;
    }

    // Keep dry buffer for safety
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Determine DSP params: preset overrides ML, Auto uses ML
    shine::ShineParams scaledParams;
    auto preset = static_cast<shine::Preset>(presetIndex);

    if (preset == shine::Preset::Auto) {
        // ML-driven: extract features and predict
        const float* leftRead = buffer.getReadPointer(0);
        auto features = featureExtractor.extract(leftRead, numSamples);
        shine::ShineParams mlParams = modelInference.getSmoothedParams(features);
        scaledParams.presence = mlParams.presence * amount;
        scaledParams.air = mlParams.air * amount;
    } else {
        // Fixed preset values scaled by amount knob
        shine::ShineParams presetParams = shine::getPresetParams(preset);
        scaledParams.presence = presetParams.presence * amount;
        scaledParams.air = presetParams.air * amount;
    }

    currentPresence.store(scaledParams.presence);
    currentAir.store(scaledParams.air);

    dspChainL.setParams(scaledParams);
    dspChainR.setParams(scaledParams);

    // Process audio
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i) {
        leftChannel[i] = dspChainL.process(leftChannel[i]);
        if (rightChannel)
            rightChannel[i] = dspChainR.process(rightChannel[i]);
    }

    // Safety: if output is NaN/inf, restore dry signal
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
