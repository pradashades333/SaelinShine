#include "PluginProcessor.h"
#include "PluginEditor.h"

ShineAudioProcessorEditor::ShineAudioProcessorEditor(ShineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    setLookAndFeel(&lookAndFeel);

    // Amount knob
    addAndMakeVisible(amountKnob);

    // Bypass button
    bypassButton.setButtonText("");
    addAndMakeVisible(bypassButton);

    // Attach parameters
    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "amount", amountKnob.getSlider());
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "bypass", bypassButton);

    startTimerHz(30);

    // Compact size for a simple plugin
    setSize(280, 320);
}

ShineAudioProcessorEditor::~ShineAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

void ShineAudioProcessorEditor::timerCallback() {
    inputLevel = audioProcessor.getInputLevel();
    outputLevel = audioProcessor.getOutputLevel();
    repaint();
}

void ShineAudioProcessorEditor::paint(juce::Graphics& g) {
    // Background
    g.fillAll(shine::ShineColours::bgDark);

    // Subtle radial gradient from center
    juce::ColourGradient centerGlow(
        shine::ShineColours::accentBlue.withAlpha(0.05f),
        getWidth() / 2.0f, getHeight() / 2.0f - 20,
        juce::Colours::transparentBlack,
        getWidth() / 2.0f, getHeight() / 2.0f + 120, true);
    g.setGradientFill(centerGlow);
    g.fillRect(getLocalBounds());

    // Header background
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(0, 0, getWidth(), 50);

    // Header border
    g.setColour(shine::ShineColours::accentBlue.withAlpha(0.15f));
    g.fillRect(0, 49, getWidth(), 1);

    // Logo
    g.setColour(shine::ShineColours::textPrimary);
    g.setFont(lookAndFeel.getLogoFont(20.0f));
    g.drawText("SHINE", 16, 14, 100, 24, juce::Justification::centredLeft);

    // Tagline
    g.setColour(shine::ShineColours::textMuted);
    g.setFont(lookAndFeel.getUIFont(9.0f));
    g.drawText("BY SAELIN", 16, 34, 80, 12, juce::Justification::centredLeft);

    // Level meters (simple bars at bottom)
    const int meterWidth = 6;
    const int meterHeight = 40;
    const int meterY = getHeight() - 60;

    // Input meter
    g.setColour(shine::ShineColours::bgCard);
    g.fillRoundedRectangle(30.0f, static_cast<float>(meterY), static_cast<float>(meterWidth),
                            static_cast<float>(meterHeight), 3.0f);

    float inNorm = juce::jlimit(0.0f, 1.0f, (20.0f * std::log10(juce::jmax(0.00001f, inputLevel)) + 48.0f) / 48.0f);
    int inFillHeight = static_cast<int>(meterHeight * inNorm);
    if (inFillHeight > 0) {
        g.setColour(shine::ShineColours::accentCyan);
        g.fillRoundedRectangle(30.0f, static_cast<float>(meterY + meterHeight - inFillHeight),
                                static_cast<float>(meterWidth), static_cast<float>(inFillHeight), 3.0f);
    }

    // Output meter
    g.setColour(shine::ShineColours::bgCard);
    g.fillRoundedRectangle(static_cast<float>(getWidth() - 30 - meterWidth), static_cast<float>(meterY),
                            static_cast<float>(meterWidth), static_cast<float>(meterHeight), 3.0f);

    float outNorm = juce::jlimit(0.0f, 1.0f, (20.0f * std::log10(juce::jmax(0.00001f, outputLevel)) + 48.0f) / 48.0f);
    int outFillHeight = static_cast<int>(meterHeight * outNorm);
    if (outFillHeight > 0) {
        g.setColour(shine::ShineColours::accentBlue);
        g.fillRoundedRectangle(static_cast<float>(getWidth() - 30 - meterWidth),
                                static_cast<float>(meterY + meterHeight - outFillHeight),
                                static_cast<float>(meterWidth), static_cast<float>(outFillHeight), 3.0f);
    }

    // Meter labels
    g.setColour(shine::ShineColours::textMuted);
    g.setFont(lookAndFeel.getUIFont(8.0f));
    g.drawText("IN", 24, getHeight() - 16, 20, 12, juce::Justification::centred);
    g.drawText("OUT", getWidth() - 44, getHeight() - 16, 24, 12, juce::Justification::centred);

    // "FREE" badge
    g.setColour(shine::ShineColours::accentCyan.withAlpha(0.8f));
    g.setFont(lookAndFeel.getUIFont(8.0f));
    juce::Rectangle<float> badgeBounds(getWidth() - 50.0f, 16.0f, 32.0f, 14.0f);
    g.drawRoundedRectangle(badgeBounds, 4.0f, 1.0f);
    g.drawText("FREE", badgeBounds.toNearestInt(), juce::Justification::centred);
}

void ShineAudioProcessorEditor::resized() {
    // Bypass button (top right, next to FREE badge)
    bypassButton.setBounds(getWidth() - 90, 12, 22, 22);

    // Main knob centered
    const int knobWidth = 160;
    const int knobHeight = 160;
    amountKnob.setBounds((getWidth() - knobWidth) / 2, 70, knobWidth, knobHeight);
}
