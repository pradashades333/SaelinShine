#include "ShineLookAndFeel.h"

namespace shine {

ShineLookAndFeel::ShineLookAndFeel() {
    // Load DM Sans (UI labels — spec: DM Sans 400, uppercase, 11px)
    auto dmSansTf = juce::Typeface::createSystemTypefaceFor(
        BinaryData::DMSansRegular_ttf, BinaryData::DMSansRegular_ttfSize);
    if (dmSansTf != nullptr)
        uiFont = juce::Font(dmSansTf).withHeight(11.0f);
    else
        uiFont = juce::Font("Segoe UI", 11.0f, juce::Font::plain);

    // Load Cormorant Garamond Medium (logo — spec: Cormorant Garamond 500)
    auto cgTf = juce::Typeface::createSystemTypefaceFor(
        BinaryData::CormorantGaramondMedium_ttf, BinaryData::CormorantGaramondMedium_ttfSize);
    if (cgTf != nullptr)
        logoFont = juce::Font(cgTf).withHeight(20.0f);
    else
        logoFont = juce::Font("Georgia", 20.0f, juce::Font::plain);

    setColour(juce::ResizableWindow::backgroundColourId, ShineColours::bgDeep);
}

void ShineLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider) {
    // Read the per-knob indicator colour set in ShineKnob constructor
    juce::Colour indicatorColour = slider.findColour(juce::Slider::rotarySliderFillColourId,
                                                     true);
    if (!indicatorColour.isOpaque())
        indicatorColour = ShineColours::accentGold;

    const float centreX = static_cast<float>(x) + static_cast<float>(width)  * 0.5f;
    const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;

    // Radii — spec: 80px outer diameter, 56px inner diameter
    const float outerR = static_cast<float>(juce::jmin(width, height)) * 0.5f - 1.0f;
    const float bodyR  = outerR * 0.74f;    // ~56px body at 80px total
    const float arcR   = outerR - 2.5f;     // arc sits just inside the outer edge

    // --- Body ---
    juce::ColourGradient bodyGrad(
        juce::Colour(0xff242018), centreX, centreY - bodyR,
        juce::Colour(0xff0f0e0c), centreX, centreY + bodyR, false);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(centreX - bodyR, centreY - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // Body border — spec: 2px solid #6b635a (textMuted)
    g.setColour(ShineColours::textMuted.withAlpha(0.6f));
    g.drawEllipse(centreX - bodyR, centreY - bodyR, bodyR * 2.0f, bodyR * 2.0f, 2.0f);

    // --- Arc track (full 270°) ---
    juce::Path trackArc;
    trackArc.addCentredArc(centreX, centreY, arcR, arcR,
                            0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(ShineColours::textMuted.withAlpha(0.25f));
    g.strokePath(trackArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // --- Filled arc ---
    if (sliderPos > 0.0f) {
        const float endAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        juce::Path fillArc;
        fillArc.addCentredArc(centreX, centreY, arcR, arcR,
                               0.0f, rotaryStartAngle, endAngle, true);
        g.setColour(indicatorColour.withAlpha(0.9f));
        g.strokePath(fillArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // --- Line indicator — spec: 3px wide x 16px long ---
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle)
                        - juce::MathConstants<float>::halfPi;
    // Fixed 16px length, centred slightly inside the body
    const float lineInner = bodyR * 0.11f;   // ~3px from centre at spec size
    const float lineOuter = bodyR * 0.68f;   // lineOuter-lineInner ≈ 16px at spec size

    const float x1 = centreX + std::cos(angle) * lineInner;
    const float y1 = centreY + std::sin(angle) * lineInner;
    const float x2 = centreX + std::cos(angle) * lineOuter;
    const float y2 = centreY + std::sin(angle) * lineOuter;

    // Indicator line — 3px stroke
    g.setColour(indicatorColour);
    juce::Path indLine;
    indLine.startNewSubPath(x1, y1);
    indLine.lineTo(x2, y2);
    g.strokePath(indLine, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

void ShineLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool /*shouldDrawButtonAsDown*/) {
    const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const float size    = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float centreX = bounds.getCentreX();
    const float centreY = bounds.getCentreY();
    const float radius  = size * 0.5f;

    // bypass=true (isOn) → plugin is bypassed → dark button
    // bypass=false (!isOn) → plugin active → gold button
    const bool bypassed = button.getToggleState();

    if (bypassed) {
        g.setColour(ShineColours::bgCard);
        g.fillEllipse(centreX - radius, centreY - radius, size, size);
        g.setColour(ShineColours::textMuted.withAlpha(0.5f));
        g.drawEllipse(centreX - radius, centreY - radius, size, size, 1.0f);
    } else {
        if (shouldDrawButtonAsHighlighted) {
            // Slight brightening on hover
            juce::ColourGradient hoverGrad(
                ShineColours::accentGold.brighter(0.15f), centreX, centreY - radius,
                ShineColours::accentCopper.brighter(0.15f), centreX, centreY + radius, false);
            g.setGradientFill(hoverGrad);
        } else {
            juce::ColourGradient goldGrad(
                ShineColours::accentGold, centreX, centreY - radius,
                ShineColours::accentCopper, centreX, centreY + radius, false);
            g.setGradientFill(goldGrad);
        }
        g.fillEllipse(centreX - radius, centreY - radius, size, size);
    }

    // Power icon
    const float iconSize = size * 0.28f;
    juce::Path powerIcon;
    powerIcon.addCentredArc(centreX, centreY + 1.0f, iconSize, iconSize,
                             0.0f,
                             -juce::MathConstants<float>::pi * 0.7f,
                              juce::MathConstants<float>::pi * 0.7f,
                             true);
    powerIcon.startNewSubPath(centreX, centreY - iconSize - 1.0f);
    powerIcon.lineTo(centreX, centreY - 1.0f);

    g.setColour(bypassed ? ShineColours::textMuted : ShineColours::bgDeep);
    g.strokePath(powerIcon, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
}

void ShineLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& /*backgroundColour*/,
                                              bool /*highlighted*/, bool /*down*/) {
    // Preset tab buttons — pill shape, gold border when active
    const auto bounds = button.getLocalBounds().toFloat();
    const bool active = button.getToggleState();

    g.setColour(active ? ShineColours::accentGold.withAlpha(0.12f) : juce::Colours::transparentBlack);
    g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);

    g.setColour(active ? ShineColours::accentGold.withAlpha(0.7f) : ShineColours::textMuted.withAlpha(0.3f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), bounds.getHeight() * 0.5f, 1.0f);
}

void ShineLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                       bool /*highlighted*/, bool /*down*/) {
    const bool active = button.getToggleState();
    g.setColour(active ? ShineColours::accentGold : ShineColours::textMuted);
    g.setFont(uiFont.withHeight(10.0f));
    g.drawText(button.getButtonText().toUpperCase(),
               button.getLocalBounds(),
               juce::Justification::centred, false);
}

juce::Font ShineLookAndFeel::getUIFont(float height) const {
    return uiFont.withHeight(height);
}

juce::Font ShineLookAndFeel::getLogoFont(float height) const {
    return logoFont.withHeight(height);
}

} // namespace shine
