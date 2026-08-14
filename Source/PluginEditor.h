#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ShineAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit ShineAudioProcessorEditor(ShineAudioProcessor&);
    ~ShineAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class ShineLookAndFeel;
    class ShineSizeConstrainer;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    ShineAudioProcessor& processorRef;
    std::unique_ptr<ShineLookAndFeel> lookAndFeel;
    std::unique_ptr<ShineSizeConstrainer> sizeConstrainer;

    // Preset tabs (2 modes: Vocal Clarity, Acoustic Detail)
    std::array<juce::TextButton, 2> modeButtons;
    int selectedMode { 0 };  // 0 = Vocal Clarity, 1 = Acoustic Detail

    // Knobs
    juce::Slider presenceSlider;
    juce::Slider airSlider;

    // Labels
    juce::Label brandLabel;
    juce::Label productLabel;
    juce::Label adaptiveLabel;
    juce::Label presenceLabel;
    juce::Label airLabel;
    juce::Label presenceValueLabel;
    juce::Label airValueLabel;
    juce::Label adaptationTitleLabel;
    juce::Label adaptationPresenceLabel;
    juce::Label adaptationAirLabel;
    juce::Label outputLabel;

    // Parameter attachments
    std::unique_ptr<SliderAttachment> presenceAttachment;
    std::unique_ptr<SliderAttachment> airAttachment;

    // Layout rectangles
    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> tabsBounds;
    juce::Rectangle<int> knobsBounds;
    juce::Rectangle<int> adaptationBounds;
    juce::Rectangle<int> outputBounds;

    float pulsePhase { 0.0f };
    float displayedOutputLevel { 0.0f };

    void timerCallback() override;
    void applyPreset(int idx);
    void syncModeButtons();
    void updateDynamicLabels();

    static juce::String getPresenceWord(float value);
    static juce::String getAirWord(float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineAudioProcessorEditor)
};
