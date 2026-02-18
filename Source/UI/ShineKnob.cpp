#include "ShineKnob.h"

namespace shine {

ShineKnob::ShineKnob(const juce::String& label, juce::Colour indicatorColour, int numDecimals)
    : labelText(label.toUpperCase()), indicatorCol(indicatorColour), decimals(numDecimals) {

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f,
                                true);

    // Tell LookAndFeel which colour to use for this knob's arc/indicator
    slider.setColour(juce::Slider::rotarySliderFillColourId, indicatorColour);
    slider.setColour(juce::Slider::thumbColourId,            indicatorColour);

    slider.onValueChange = [this] { repaint(); };

    addAndMakeVisible(slider);
}

void ShineKnob::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // Value text
    double val = slider.getValue();
    juce::String valueStr = juce::String(val, decimals);

    // Label
    auto labelArea = bounds.removeFromBottom(valueHeight + labelHeight + 6);
    g.setColour(ShineColours::textMuted);
    g.setFont(juce::Font("Segoe UI", 10.0f, juce::Font::plain));
    g.drawText(labelText, labelArea.removeFromTop(labelHeight),
               juce::Justification::centred, false);

    // Value
    g.setColour(indicatorCol.withAlpha(0.85f));
    g.setFont(juce::Font("Segoe UI", 11.0f, juce::Font::plain));
    g.drawText(valueStr, labelArea, juce::Justification::centredTop, false);
}

void ShineKnob::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(valueHeight + labelHeight + 6);

    const int cx = bounds.getX() + (bounds.getWidth()  - knobSize) / 2;
    const int cy = bounds.getY() + (bounds.getHeight() - knobSize) / 2;
    slider.setBounds(cx, cy, knobSize, knobSize);
}

} // namespace shine
