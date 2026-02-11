#include "ShineLookAndFeel.h"

namespace shine {

ShineLookAndFeel::ShineLookAndFeel() {
    uiFont = juce::Font("Segoe UI", 11.0f, juce::Font::plain);
    logoFont = juce::Font("Segoe UI", 24.0f, juce::Font::bold);

    setColour(juce::ResizableWindow::backgroundColourId, ShineColours::bgDark);
}

void ShineLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider) {
    juce::ignoreUnused(slider);

    const float radius = static_cast<float>(juce::jmin(width / 2, height / 2)) - 8.0f;
    const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Outer glow
    juce::ColourGradient glow(
        ShineColours::accentBlue.withAlpha(0.15f), centreX, centreY,
        juce::Colours::transparentBlack, centreX, centreY + radius + 20, true);
    g.setGradientFill(glow);
    g.fillEllipse(rx - 15, ry - 15, rw + 30, rw + 30);

    // Knob background
    juce::ColourGradient bgGrad(
        juce::Colour(0xff2a2a2a), centreX, centreY - radius,
        juce::Colour(0xff121212), centreX, centreY + radius, false);
    g.setGradientFill(bgGrad);
    g.fillEllipse(rx, ry, rw, rw);

    // Border
    g.setColour(ShineColours::accentBlue.withAlpha(0.2f));
    g.drawEllipse(rx, ry, rw, rw, 2.0f);

    // Track arc (background)
    juce::Path trackArc;
    trackArc.addCentredArc(centreX, centreY, radius - 10, radius - 10,
                            0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(ShineColours::textMuted.withAlpha(0.3f));
    g.strokePath(trackArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Value arc
    if (sliderPos > 0.0f) {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius - 10, radius - 10,
                                0.0f, rotaryStartAngle, angle, true);

        juce::ColourGradient arcGrad(
            ShineColours::accentCyan, centreX - radius, centreY,
            ShineColours::accentBlue, centreX + radius, centreY, false);
        g.setGradientFill(arcGrad);
        g.strokePath(valueArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // Indicator dot
    const float dotRadius = radius - 18.0f;
    const float dotX = centreX + std::cos(angle - juce::MathConstants<float>::halfPi) * dotRadius;
    const float dotY = centreY + std::sin(angle - juce::MathConstants<float>::halfPi) * dotRadius;

    // Dot glow
    g.setColour(ShineColours::accentBlue.withAlpha(0.5f));
    g.fillEllipse(dotX - 8, dotY - 8, 16, 16);

    // Dot
    g.setColour(ShineColours::accentBlue);
    g.fillEllipse(dotX - 5, dotY - 5, 10, 10);

    // Inner highlight
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.fillEllipse(dotX - 2, dotY - 2, 4, 4);
}

void ShineLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(shouldDrawButtonAsDown);

    const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const float size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float radius = size * 0.5f;

    const bool isOn = button.getToggleState();

    // Glow when active
    if (!isOn) {
        juce::ColourGradient glow(
            ShineColours::accentBlue.withAlpha(0.3f), centreX, centreY,
            juce::Colours::transparentBlack, centreX, centreY + radius + 4, true);
        g.setGradientFill(glow);
        g.fillEllipse(centreX - radius - 4, centreY - radius - 4, size + 8, size + 8);
    }

    // Button background
    if (isOn) {
        g.setColour(ShineColours::bgCard);
        g.fillEllipse(centreX - radius, centreY - radius, size, size);
        g.setColour(ShineColours::textMuted.withAlpha(0.5f));
        g.drawEllipse(centreX - radius, centreY - radius, size, size, 1.0f);
    } else {
        juce::ColourGradient blueGrad(
            ShineColours::accentBlue, centreX, centreY - radius,
            ShineColours::accentCyan, centreX, centreY + radius, false);
        g.setGradientFill(blueGrad);
        g.fillEllipse(centreX - radius, centreY - radius, size, size);

        if (shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.fillEllipse(centreX - radius, centreY - radius, size, size);
        }
    }

    // Power icon
    const float iconSize = size * 0.3f;
    juce::Path powerIcon;

    powerIcon.addCentredArc(centreX, centreY + 1.0f, iconSize, iconSize,
                            0.0f,
                            -juce::MathConstants<float>::pi * 0.7f,
                            juce::MathConstants<float>::pi * 0.7f,
                            true);

    powerIcon.startNewSubPath(centreX, centreY - iconSize - 1.0f);
    powerIcon.lineTo(centreX, centreY - 1.0f);

    g.setColour(isOn ? ShineColours::textMuted : ShineColours::bgDark);
    g.strokePath(powerIcon, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
}

juce::Font ShineLookAndFeel::getUIFont(float height) const {
    return uiFont.withHeight(height);
}

juce::Font ShineLookAndFeel::getLogoFont(float height) const {
    return logoFont.withHeight(height);
}

} // namespace shine
