#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ShineLookAndFeel.h"

namespace shine {

class ShineKnob : public juce::Component {
public:
    enum class ParamType { Presence, Air };

    ShineKnob(const juce::String& label, juce::Colour indicatorColour, ParamType type);
    ~ShineKnob() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Slider& getSlider() { return slider; }

private:
    juce::Slider   slider;
    juce::String   labelText;
    juce::Colour   indicatorCol;
    ParamType      paramType;

    static constexpr int knobSize    = 80;
    static constexpr int labelHeight = 18;
    static constexpr int valueHeight = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineKnob)
};

} // namespace shine
