#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ShineLookAndFeel.h"

namespace shine {

class ShineKnob : public juce::Component {
public:
    // indicatorColour: arc + indicator line colour (gold for Presence, muted blue for Air)
    // numDecimals: how many decimal places to show below the knob (1 for Presence, 2 for Air)
    ShineKnob(const juce::String& label, juce::Colour indicatorColour, int numDecimals = 2);
    ~ShineKnob() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Slider& getSlider() { return slider; }

private:
    juce::Slider   slider;
    juce::String   labelText;
    juce::Colour   indicatorCol;
    int            decimals;

    static constexpr int knobSize    = 90;
    static constexpr int labelHeight = 18;
    static constexpr int valueHeight = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineKnob)
};

} // namespace shine
