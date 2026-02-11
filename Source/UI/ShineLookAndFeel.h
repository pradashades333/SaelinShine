#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace shine {

// Clean, minimal color palette
namespace ShineColours {
    const juce::Colour bgDark(0xff0c0c0c);
    const juce::Colour bgCard(0xff1a1a1a);
    const juce::Colour accentBlue(0xff5ba4e6);      // Bright blue for "shine"
    const juce::Colour accentCyan(0xff4ecdc4);      // Teal accent
    const juce::Colour textPrimary(0xffe8e8e8);
    const juce::Colour textMuted(0xff666666);
}

class ShineLookAndFeel : public juce::LookAndFeel_V4 {
public:
    ShineLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    juce::Font getUIFont(float height) const;
    juce::Font getLogoFont(float height) const;

private:
    juce::Font uiFont;
    juce::Font logoFont;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShineLookAndFeel)
};

} // namespace shine
