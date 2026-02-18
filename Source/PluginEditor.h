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
    void applyPreset(int presetIndex);  // 0 = Vocal Clarity, 1 = Acoustic Detail
    void updateTabStates();

    ShineAudioProcessor& audioProcessor;

    shine::ShineLookAndFeel lookAndFeel;

    // Knobs (presence = gold, air = muted blue)
    shine::ShineKnob presenceKnob { "Presence", shine::ShineColours::accentGold, 1 };
    shine::ShineKnob airKnob      { "Air",      shine::ShineColours::accentAir,  2 };

    juce::ToggleButton bypassButton;

    // Preset tabs — pill style, toggle state drives highlight
    juce::TextButton tabVocalClarity   { "Vocal Clarity"   };
    juce::TextButton tabAcousticDetail { "Acoustic Detail" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> presenceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // Values read from processor each timer tick
    float inputLevel     = 0.0f;
    float outputLevel    = 0.0f;
    float adaptPresScale = 0.5f;
    float adaptAirScale  = 0.5f;

    // Which tab is visually active (-1 = none, 0 = Vocal Clarity, 1 = Acoustic Detail)
    int selectedTab = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineAudioProcessorEditor)
};
