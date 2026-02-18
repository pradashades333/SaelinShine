#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/ShineLookAndFeel.h"
#include "UI/ShineKnob.h"

class ShineAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer {
public:
    explicit ShineAudioProcessorEditor(ShineAudioProcessor&);
    ~ShineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updatePresetButtons();

    ShineAudioProcessor& audioProcessor;

    shine::ShineLookAndFeel lookAndFeel;

    shine::ShineKnob amountKnob{"Shine"};
    juce::ToggleButton bypassButton;

    // Preset buttons
    juce::TextButton presetAuto    { "Auto" };
    juce::TextButton presetVocal   { "Vocal Clarity" };
    juce::TextButton presetAcoustic{ "Acoustic Detail" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    float inputLevel = 0.0f;
    float outputLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineAudioProcessorEditor)
};
