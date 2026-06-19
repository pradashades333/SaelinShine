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
    void applyPreset(int presetIndex);
    void updateTabStates();

    ShineAudioProcessor& audioProcessor;

    shine::ShineLookAndFeel lookAndFeel;

    shine::ShineKnob presenceKnob { "Presence", shine::ShineColours::accentGold,
                                     shine::ShineKnob::ParamType::Presence };
    shine::ShineKnob airKnob      { "Air",      shine::ShineColours::accentAir,
                                     shine::ShineKnob::ParamType::Air };

    juce::ToggleButton bypassButton;

    juce::TextButton tabVocalClarity   { "Vocal Clarity"   };
    juce::TextButton tabAcousticDetail { "Acoustic Detail" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> presenceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    float inputLevel     = 0.0f;
    float outputLevel    = 0.0f;
    float adaptPresScale = 0.5f;
    float adaptAirScale  = 0.5f;

    int selectedTab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineAudioProcessorEditor)
};
