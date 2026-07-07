#include "ShineKnob.h"
#include "../DSP/ShineParams.h"

namespace shine {

ShineKnob::ShineKnob(const juce::String& label, juce::Colour indicatorColour, ParamType type)
    : labelText(label.toUpperCase()), indicatorCol(indicatorColour), paramType(type) {

    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f,
                                true);

    slider.setColour(juce::Slider::rotarySliderFillColourId, indicatorColour);
    slider.setColour(juce::Slider::thumbColourId,            indicatorColour);

    slider.onValueChange = [this] { repaint(); };

    addAndMakeVisible(slider);
}

void ShineKnob::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    float val = static_cast<float>(slider.getValue());
    const char* valueWord = (paramType == ParamType::Presence)
                            ? getPresenceWord(val)
                            : getAirWord(val);

    auto labelArea = bounds.removeFromBottom(valueHeight + labelHeight + 6);
    g.setColour(ShineColours::textMuted);
    g.setFont(juce::Font(juce::FontOptions("Segoe UI").withHeight(10.0f)));
    g.drawText(labelText, labelArea.removeFromTop(labelHeight),
               juce::Justification::centred, false);

    g.setColour(indicatorCol.withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions("Segoe UI").withHeight(11.0f).withStyle("Italic")));
    g.drawText(valueWord, labelArea, juce::Justification::centredTop, false);
}

void ShineKnob::resized() {
    auto bounds = getLocalBounds();
    bounds.removeFromBottom(valueHeight + labelHeight + 6);

    const int cx = bounds.getX() + (bounds.getWidth()  - knobSize) / 2;
    const int cy = bounds.getY() + (bounds.getHeight() - knobSize) / 2;
    slider.setBounds(cx, cy, knobSize, knobSize);
}

} // namespace shine
