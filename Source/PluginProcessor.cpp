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

    // Presence knob: 0.0 - 6.0, default 3.0 (Vocal Clarity starting point)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("presence", 1), "Presence",
        juce::NormalisableRange<float>(0.0f, 6.0f, 0.01f), 3.0f));

    // Air knob: 0.0 - 1.0, default 0.40 (Vocal Clarity starting point)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("air", 1), "Air",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.40f));

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

    // Input level
    float inLevel = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        inLevel = juce::jmax(inLevel, buffer.getMagnitude(ch, 0, numSamples));
    inputLevel.store(inLevel);

    if (bypass) {
        outputLevel.store(inLevel);
        return;
    }

    // Dry buffer for safety
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // ML inference — presence output drives the adaptive scale for both bands
    // Model outputs presence in 0-6 range; any value ≥ 1.0 jlimits to 1.0 (full effect)
    const float* leftRead = buffer.getReadPointer(0);
    auto features = featureExtractor.extract(leftRead, numSamples);
    shine::ShineParams mlScales = modelInference.getSmoothedParams(features);

    // Presence adaptive scale: model output 0-6, clamped to 0-1
    float presScale = juce::jlimit(0.0f, 1.0f, mlScales.presence);

    // Adaptation meters: inverted dynamics (high when quiet, low when loud)
    // Uses same -48 dBFS floor as the I/O meter display
    float levelDb    = 20.0f * std::log10(juce::jmax(0.00001f, inLevel));
    float adaptMeter = 1.0f - juce::jlimit(0.0f, 1.0f, (levelDb + 48.0f) / 48.0f);
    adaptPresenceScale.store(adaptMeter);
    adaptAirScale.store(adaptMeter);

    // Final DSP params: both bands use the presence adaptive scale
    // Air previously used mlScales.air which stays near 0 (model bias ~0.03),
    // making airKnob * ~0 ≈ 0 regardless of knob position.
    // Using presScale gives air the same wiring quality as presence.
    shine::ShineParams scaledParams;
    scaledParams.presence = presenceKnob * presScale;
    scaledParams.air      = airKnob      * presScale;

    dspChainL.setParams(scaledParams);
    dspChainR.setParams(scaledParams);

    float* leftCh  = buffer.getWritePointer(0);
    float* rightCh = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i) {
        leftCh[i] = dspChainL.process(leftCh[i]);
        if (rightCh)
            rightCh[i] = dspChainR.process(rightCh[i]);
    }

    // Safety: restore dry if output is NaN/inf
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
