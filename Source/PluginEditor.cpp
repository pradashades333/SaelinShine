#include "PluginProcessor.h"
#include "PluginEditor.h"

ShineAudioProcessorEditor::ShineAudioProcessorEditor(ShineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(amountKnob);

    bypassButton.setButtonText("");
    addAndMakeVisible(bypassButton);

    // Preset buttons
    for (auto* btn : { &presetAuto, &presetVocal, &presetAcoustic }) {
        btn->setClickingTogglesState(false);
        addAndMakeVisible(btn);
    }

    presetAuto.onClick = [this] {
        audioProcessor.getAPVTS().getParameterAsValue("preset").setValue(0);
        updatePresetButtons();
    };
    presetVocal.onClick = [this] {
        audioProcessor.getAPVTS().getParameterAsValue("preset").setValue(1);
        updatePresetButtons();
    };
    presetAcoustic.onClick = [this] {
        audioProcessor.getAPVTS().getParameterAsValue("preset").setValue(2);
        updatePresetButtons();
    };

    amountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "amount", amountKnob.getSlider());
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "bypass", bypassButton);

    updatePresetButtons();
    startTimerHz(30);
    setSize(280, 360);
}

ShineAudioProcessorEditor::~ShineAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

void ShineAudioProcessorEditor::timerCallback() {
    inputLevel = audioProcessor.getInputLevel();
    outputLevel = audioProcessor.getOutputLevel();
    // Sync preset button state if changed from DAW automation
    updatePresetButtons();
    repaint();
}

void ShineAudioProcessorEditor::updatePresetButtons() {
    int current = static_cast<int>(audioProcessor.getAPVTS().getRawParameterValue("preset")->load());
    presetAuto.setColour(juce::TextButton::buttonColourId,
        current == 0 ? shine::ShineColours::accentBlue : shine::ShineColours::bgCard);
    presetVocal.setColour(juce::TextButton::buttonColourId,
        current == 1 ? shine::ShineColours::accentBlue : shine::ShineColours::bgCard);
    presetAcoustic.setColour(juce::TextButton::buttonColourId,
        current == 2 ? shine::ShineColours::accentBlue : shine::ShineColours::bgCard);

    presetAuto.setColour(juce::TextButton::textColourOffId,
        current == 0 ? juce::Colours::black : shine::ShineColours::textMuted);
    presetVocal.setColour(juce::TextButton::textColourOffId,
        current == 1 ? juce::Colours::black : shine::ShineColours::textMuted);
    presetAcoustic.setColour(juce::TextButton::textColourOffId,
        current == 2 ? juce::Colours::black : shine::ShineColours::textMuted);
}

void ShineAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(shine::ShineColours::bgDark);

    // Subtle center glow
    juce::ColourGradient centerGlow(
        shine::ShineColours::accentBlue.withAlpha(0.05f),
        getWidth() / 2.0f, getHeight() / 2.0f - 20,
        juce::Colours::transparentBlack,
        getWidth() / 2.0f, getHeight() / 2.0f + 120, true);
    g.setGradientFill(centerGlow);
    g.fillRect(getLocalBounds());

    // Header
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(0, 0, getWidth(), 50);
    g.setColour(shine::ShineColours::accentBlue.withAlpha(0.15f));
    g.fillRect(0, 49, getWidth(), 1);

    g.setColour(shine::ShineColours::textPrimary);
    g.setFont(lookAndFeel.getLogoFont(20.0f));
    g.drawText("SHINE", 16, 14, 100, 24, juce::Justification::centredLeft);

    g.setColour(shine::ShineColours::textMuted);
    g.setFont(lookAndFeel.getUIFont(9.0f));
    g.drawText("BY SAELIN", 16, 34, 80, 12, juce::Justification::centredLeft);

    // Level meters
    const int meterWidth = 6;
    const int meterHeight = 40;
    const int meterY = getHeight() - 60;

    g.setColour(shine::ShineColours::bgCard);
    g.fillRoundedRectangle(30.0f, static_cast<float>(meterY),
                            static_cast<float>(meterWidth), static_cast<float>(meterHeight), 3.0f);
    float inNorm = juce::jlimit(0.0f, 1.0f, (20.0f * std::log10(juce::jmax(0.00001f, inputLevel)) + 48.0f) / 48.0f);
    int inFill = static_cast<int>(meterHeight * inNorm);
    if (inFill > 0) {
        g.setColour(shine::ShineColours::accentCyan);
        g.fillRoundedRectangle(30.0f, static_cast<float>(meterY + meterHeight - inFill),
                                static_cast<float>(meterWidth), static_cast<float>(inFill), 3.0f);
    }

    g.setColour(shine::ShineColours::bgCard);
    g.fillRoundedRectangle(static_cast<float>(getWidth() - 30 - meterWidth), static_cast<float>(meterY),
                            static_cast<float>(meterWidth), static_cast<float>(meterHeight), 3.0f);
    float outNorm = juce::jlimit(0.0f, 1.0f, (20.0f * std::log10(juce::jmax(0.00001f, outputLevel)) + 48.0f) / 48.0f);
    int outFill = static_cast<int>(meterHeight * outNorm);
    if (outFill > 0) {
        g.setColour(shine::ShineColours::accentBlue);
        g.fillRoundedRectangle(static_cast<float>(getWidth() - 30 - meterWidth),
                                static_cast<float>(meterY + meterHeight - outFill),
                                static_cast<float>(meterWidth), static_cast<float>(outFill), 3.0f);
    }

    g.setColour(shine::ShineColours::textMuted);
    g.setFont(lookAndFeel.getUIFont(8.0f));
    g.drawText("IN",  24, getHeight() - 16, 20, 12, juce::Justification::centred);
    g.drawText("OUT", getWidth() - 44, getHeight() - 16, 24, 12, juce::Justification::centred);

    // FREE badge
    g.setColour(shine::ShineColours::accentCyan.withAlpha(0.8f));
    g.setFont(lookAndFeel.getUIFont(8.0f));
    juce::Rectangle<float> badge(getWidth() - 50.0f, 16.0f, 32.0f, 14.0f);
    g.drawRoundedRectangle(badge, 4.0f, 1.0f);
    g.drawText("FREE", badge.toNearestInt(), juce::Justification::centred);
}

void ShineAudioProcessorEditor::resized() {
    bypassButton.setBounds(getWidth() - 90, 12, 22, 22);

    const int knobWidth = 160;
    const int knobHeight = 160;
    amountKnob.setBounds((getWidth() - knobWidth) / 2, 58, knobWidth, knobHeight);

    // Preset buttons row — three equal buttons
    const int btnY = 228;
    const int btnH = 22;
    const int pad = 8;
    const int totalW = getWidth() - pad * 2;
    const int btnW = (totalW - pad * 2) / 3;

    presetAuto.setBounds    (pad,               btnY, btnW, btnH);
    presetVocal.setBounds   (pad + btnW + pad,  btnY, btnW, btnH);
    presetAcoustic.setBounds(pad + btnW*2 + pad*2, btnY, btnW, btnH);
}
