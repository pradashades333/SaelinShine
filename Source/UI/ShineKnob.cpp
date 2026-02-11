#include "ShineKnob.h"

namespace shine {

ShineKnob::ShineKnob(const juce::String& name)
    : labelText(name.toUpperCase()) {

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f,
                                true);
    addAndMakeVisible(slider);

    slider.onValueChange = [this]() {
        float val = static_cast<float>(slider.getValue());
        int percent = static_cast<int>(val * 100.0f);
        valueText = juce::String(percent) + "%";
        repaint();
    };

    valueText = "50%";
}

void ShineKnob::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // Label area at bottom
    auto labelArea = bounds.removeFromBottom(labelHeight + valueHeight + 8);

    // Label
    g.setColour(ShineColours::textPrimary);
    g.setFont(juce::Font("Segoe UI", 13.0f, juce::Font::plain));
    g.drawText(labelText, labelArea.removeFromTop(labelHeight + 4),
               juce::Justification::centred, false);

    // Value
    g.setColour(ShineColours::accentBlue);
    g.setFont(juce::Font("Segoe UI", 11.0f, juce::Font::plain));
    g.drawText(valueText, labelArea, juce::Justification::centredTop, false);
}

void ShineKnob::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(labelHeight + valueHeight + 8);

    const int knobX = (bounds.getWidth() - knobSize) / 2;
    const int knobY = (bounds.getHeight() - knobSize) / 2;
    slider.setBounds(knobX, knobY, knobSize, knobSize);
}

} // namespace shine
