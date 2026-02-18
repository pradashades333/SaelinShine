#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace shine {

// Warm gold color palette matching the Saelin Shine spec
namespace ShineColours {
    const juce::Colour bgDeep       (0xff0a0908);
    const juce::Colour bgWarm       (0xff151210);
    const juce::Colour bgCard       (0xff1a1714);
    const juce::Colour accentGold   (0xffc9a866);
    const juce::Colour accentCopper (0xffb87333);
    const juce::Colour accentAir    (0xffa8c4d4);
    const juce::Colour textPrimary  (0xfff5f0eb);
    const juce::Colour textSecondary(0xffa89f94);
    const juce::Colour textMuted    (0xff6b635a);
    // Aliases used in older code paths
    const juce::Colour bgDark = bgDeep;
    const juce::Colour accentBlue = accentAir;
    const juce::Colour accentCyan = accentGold;
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

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
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
