#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/ShineChain.h"
#include "DSP/ShineParams.h"
#include "DSP/SaelinAdaptiveEngine.h"

class ShineAudioProcessor : public juce::AudioProcessor {
public:
    ShineAudioProcessor();
    ~ShineAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    float getInputLevel()  const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

    float getAdaptPresenceScale() const { return adaptPresenceScale.load(); }
    float getAdaptAirScale()      const { return adaptAirScale.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    juce::AudioProcessorValueTreeState apvts;

    shine::ShineChain dspChainL;
    shine::ShineChain dspChainR;

    shine::SaelinAdaptiveEngine sae;
    int saePresenceIdx = -1;
    int saeAirIdx = -1;

    std::atomic<float> inputLevel       {0.0f};
    std::atomic<float> outputLevel      {0.0f};
    std::atomic<float> adaptPresenceScale{0.5f};
    std::atomic<float> adaptAirScale    {0.5f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineAudioProcessor)
};
