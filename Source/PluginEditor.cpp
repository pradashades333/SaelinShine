#include "PluginEditor.h"

#include <BinaryData.h>
#include <limits>

namespace
{
constexpr int kNativeWidth = 520;
constexpr int kNativeHeight = 360;

const auto background = juce::Colour(0xFFF1E9D8);
const auto outline = juce::Colour::fromFloatRGBA(26.0f / 255.0f, 22.0f / 255.0f, 18.0f / 255.0f, 0.08f);
const auto lineSoft = juce::Colour::fromFloatRGBA(26.0f / 255.0f, 22.0f / 255.0f, 18.0f / 255.0f, 0.04f);
const auto ink = juce::Colour(0xFF1A1612);
const auto inkMuted = juce::Colour(0xFF6A6253);
const auto accent = juce::Colour(0xFFD4A042);         // Shine gold
const auto accentSoft = juce::Colour::fromFloatRGBA(212.0f / 255.0f, 160.0f / 255.0f, 66.0f / 255.0f, 0.12f);
const auto airAccent = juce::Colour(0xFF7A9AAA);      // Muted blue for Air
const auto meterOff = juce::Colour::fromFloatRGBA(26.0f / 255.0f, 22.0f / 255.0f, 18.0f / 255.0f, 0.06f);

float toRadians(float degrees)
{
    return degrees * juce::MathConstants<float>::pi / 180.0f;
}

juce::Font makeFont(const juce::String& name, float size, int styleFlags = juce::Font::plain, float kerning = 0.0f)
{
    return juce::Font(juce::FontOptions(name, size, styleFlags).withKerningFactor(kerning));
}

juce::Font makeSans(float size, bool bold = false, float kerning = 0.0f)
{
    return makeFont("DM Sans", size, bold ? juce::Font::bold : juce::Font::plain, kerning);
}

juce::Font makeSerif(float size, bool italic = false, float kerning = 0.0f)
{
    return makeFont("Newsreader", size, italic ? juce::Font::italic : juce::Font::plain, kerning);
}

juce::Font makeBrandSerif(float size, float kerning = 0.0f)
{
    return makeFont("Cormorant Garamond", size, juce::Font::bold, kerning);
}

float getUiScale(int width, int height)
{
    return std::min((float) width / (float) kNativeWidth, (float) height / (float) kNativeHeight);
}
}

class ShineAudioProcessorEditor::ShineLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ShineLookAndFeel()
    {
        shineKnobDrawable = juce::Drawable::createFromImageData(
            BinaryData::saelinknobshine_svg, (size_t) BinaryData::saelinknobshine_svgSize);

        juce::String templateStr (BinaryData::saelinknobtemplate_svg,
                                  BinaryData::saelinknobtemplate_svgSize);
        templateStr = templateStr.replace ("currentColor", "#7A9AAA");
        if (auto xml = juce::XmlDocument::parse (templateStr))
            airKnobDrawable = juce::Drawable::createFromSVG (*xml);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return makeSans((float) buttonHeight * 0.40f, false, 0.16f);
    }

    void drawButtonBackground(juce::Graphics& /*g*/, juce::Button& /*button*/,
                              const juce::Colour& /*backgroundColour*/,
                              bool /*shouldDrawButtonAsHighlighted*/,
                              bool /*shouldDrawButtonAsDown*/) override
    {
        // Intentionally empty — mode tab backgrounds are drawn in paint()
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const float cx = (float) x + (float) width  * 0.5f;
        const float cy = (float) y + (float) height * 0.5f;
        const float svgScale   = std::min ((float) width, (float) height) / 120.0f;
        const float uiScale    = svgScale / 0.75f;
        const float svgRadius  = 45.0f * svgScale;
        const float trackRadius = svgRadius + 3.5f * uiScale;

        const auto angle       = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const auto arcStart    = rotaryStartAngle + juce::MathConstants<float>::halfPi;
        const auto arcEnd      = rotaryEndAngle   + juce::MathConstants<float>::halfPi;
        const auto arcCurrent  = angle            + juce::MathConstants<float>::halfPi;

        const auto fillColour = juce::Colour ((juce::uint32) (int) slider.getProperties()
                                               .getWithDefault ("accent", (int) accent.getARGB()));

        juce::Path track;
        track.addCentredArc (cx, cy, trackRadius, trackRadius, 0.0f, arcStart, arcEnd, true);
        g.setColour (outline.withAlpha (0.22f));
        g.strokePath (track, juce::PathStrokeType (4.5f * uiScale, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        juce::Path valueArc;
        valueArc.addCentredArc (cx, cy, trackRadius, trackRadius, 0.0f, arcStart, arcCurrent, true);
        g.setColour (fillColour);
        g.strokePath (valueArc, juce::PathStrokeType (4.5f * uiScale, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        const juce::Colour airCol (0xFF7A9AAA);
        const bool isAir = (fillColour == airCol);
        auto* drawable = isAir ? airKnobDrawable.get() : shineKnobDrawable.get();
        if (drawable != nullptr)
        {
            const float svgAngle = angle + juce::MathConstants<float>::halfPi;
            const auto transform =
                juce::AffineTransform::rotation (svgAngle, 60.0f, 60.0f)
                    .scaled (svgScale)
                    .translated (cx - 60.0f * svgScale, cy - 60.0f * svgScale);
            drawable->draw (g, 1.0f, transform);
        }
    }

private:
    std::unique_ptr<juce::Drawable> shineKnobDrawable;
    std::unique_ptr<juce::Drawable> airKnobDrawable;
};

class ShineAudioProcessorEditor::ShineSizeConstrainer : public juce::ComponentBoundsConstrainer
{
public:
    ShineSizeConstrainer()
    {
        setFixedAspectRatio((double) kNativeWidth / (double) kNativeHeight);
        setSizeLimits(kNativeWidth, kNativeHeight, kNativeWidth * 2, kNativeHeight * 2);
    }

    void checkBounds(juce::Rectangle<int>& bounds,
                     const juce::Rectangle<int>& previousBounds,
                     const juce::Rectangle<int>& limits,
                     bool isStretchingTop,
                     bool isStretchingLeft,
                     bool isStretchingBottom,
                     bool isStretchingRight) override
    {
        ComponentBoundsConstrainer::checkBounds(bounds, previousBounds, limits,
                                                isStretchingTop, isStretchingLeft,
                                                isStretchingBottom, isStretchingRight);

        static constexpr std::array<double, 4> scales { 1.0, 1.25, 1.5, 2.0 };

        double bestScale = scales.front();
        int bestDistance = std::numeric_limits<int>::max();

        for (auto scale : scales)
        {
            const auto snappedWidth = juce::roundToInt((double) kNativeWidth * scale);
            const auto distance = std::abs(bounds.getWidth() - snappedWidth);

            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestScale = scale;
            }
        }

        const auto snappedWidth = juce::roundToInt((double) kNativeWidth * bestScale);
        const auto snappedHeight = juce::roundToInt((double) kNativeHeight * bestScale);

        if (isStretchingLeft && ! isStretchingRight)
            bounds.setLeft(bounds.getRight() - snappedWidth);
        else
            bounds.setWidth(snappedWidth);

        if (isStretchingTop && ! isStretchingBottom)
            bounds.setTop(bounds.getBottom() - snappedHeight);
        else
            bounds.setHeight(snappedHeight);

        ComponentBoundsConstrainer::checkBounds(bounds, previousBounds, limits,
                                                isStretchingTop, isStretchingLeft,
                                                isStretchingBottom, isStretchingRight);
    }
};

ShineAudioProcessorEditor::ShineAudioProcessorEditor(ShineAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    lookAndFeel = std::make_unique<ShineLookAndFeel>();
    sizeConstrainer = std::make_unique<ShineSizeConstrainer>();
    setLookAndFeel(lookAndFeel.get());
    setConstrainer(sizeConstrainer.get());

    auto configureLabel = [this](juce::Label& label, const juce::String& text, float size,
                                 juce::Colour colour, juce::Justification justification, bool bold = false)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(makeSans(size, bold));
        label.setColour(juce::Label::textColourId, colour);
        label.setJustificationType(justification);
        addAndMakeVisible(label);
    };

    configureLabel(brandLabel, "SAELIN", 22.0f, ink, juce::Justification::centredLeft, false);
    configureLabel(productLabel, "SHINE", 11.0f, accent, juce::Justification::centredLeft, true);
    configureLabel(adaptiveLabel, "ADAPTIVE PROCESSING", 10.0f, inkMuted, juce::Justification::centred);
    configureLabel(presenceLabel, "PRESENCE", 11.0f, ink, juce::Justification::centred, true);
    configureLabel(airLabel, "AIR", 11.0f, ink, juce::Justification::centred, true);
    configureLabel(presenceValueLabel, {}, 14.0f, inkMuted, juce::Justification::centred, false);
    configureLabel(airValueLabel, {}, 14.0f, inkMuted, juce::Justification::centred, false);
    configureLabel(adaptationTitleLabel, "ADAPTATION", 9.0f, inkMuted, juce::Justification::centred, true);
    configureLabel(adaptationPresenceLabel, "PRESENCE", 8.0f, inkMuted, juce::Justification::centred, true);
    configureLabel(adaptationAirLabel, "AIR", 8.0f, inkMuted, juce::Justification::centred, true);
    configureLabel(outputLabel, "OUTPUT", 10.0f, inkMuted, juce::Justification::centred, true);

    // Preset tabs
    for (int i = 0; i < (int) modeButtons.size(); ++i)
    {
        auto& button = modeButtons[(size_t) i];
        button.setButtonText(i == 0 ? "VOCAL CLARITY" : "ACOUSTIC DETAIL");
        button.setClickingTogglesState(true);
        button.setRadioGroupId(101);
        button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        button.setColour(juce::TextButton::textColourOffId, inkMuted);
        button.setColour(juce::TextButton::textColourOnId, accent);
        button.setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight | juce::Button::ConnectedOnTop | juce::Button::ConnectedOnBottom);
        button.onClick = [this, i] { applyPreset(i); };
        addAndMakeVisible(button);
    }

    auto configureSlider = [this](juce::Slider& slider, juce::Colour fillColour)
    {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setRotaryParameters(toRadians(130.0f), toRadians(410.0f), true);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setDoubleClickReturnValue(true, 0.5);
        slider.setColour(juce::Slider::rotarySliderFillColourId, fillColour);
        slider.getProperties().set("accent", (int) fillColour.getARGB());
        addAndMakeVisible(slider);
    };

    configureSlider(presenceSlider, accent);
    configureSlider(airSlider, airAccent);

    presenceAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), "presence", presenceSlider);
    airAttachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), "air", airSlider);

    // Detect initial preset from parameter values
    float pres = processorRef.getAPVTS().getRawParameterValue("presence")->load();
    float air  = processorRef.getAPVTS().getRawParameterValue("air")->load();
    selectedMode = (std::abs(pres - 4.0f) < 0.05f && std::abs(air - 0.20f) < 0.05f) ? 1 : 0;

    syncModeButtons();
    updateDynamicLabels();

    setResizable(true, true);
    setSize(kNativeWidth, kNativeHeight);
    startTimerHz(30);
}

ShineAudioProcessorEditor::~ShineAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ShineAudioProcessorEditor::applyPreset(int idx)
{
    selectedMode = idx;
    if (idx == 0)
    {
        processorRef.getAPVTS().getParameterAsValue("presence").setValue(3.0);
        processorRef.getAPVTS().getParameterAsValue("air").setValue(0.40);
    }
    else
    {
        processorRef.getAPVTS().getParameterAsValue("presence").setValue(4.0);
        processorRef.getAPVTS().getParameterAsValue("air").setValue(0.20);
    }
    syncModeButtons();
}

void ShineAudioProcessorEditor::syncModeButtons()
{
    for (int i = 0; i < (int) modeButtons.size(); ++i)
        modeButtons[(size_t) i].setToggleState(selectedMode == i, juce::dontSendNotification);
}

void ShineAudioProcessorEditor::updateDynamicLabels()
{
    presenceValueLabel.setText(getPresenceWord((float) presenceSlider.getValue()), juce::dontSendNotification);
    airValueLabel.setText(getAirWord((float) airSlider.getValue()), juce::dontSendNotification);
}

juce::String ShineAudioProcessorEditor::getPresenceWord(float value)
{
    // Presence range is 0-6 dB or similar; adjust as needed
    static const std::array<const char*, 5> words { "Soft", "Present", "Forward", "Bright", "Vivid" };
    const auto normalized = juce::jlimit(0.0f, 1.0f, value);
    const auto index = juce::jlimit(0, 4, (int) std::floor(normalized * 5.0f));
    return words[(size_t) index];
}

juce::String ShineAudioProcessorEditor::getAirWord(float value)
{
    static const std::array<const char*, 5> words { "Warm", "Balanced", "Open", "Airy", "Crisp" };
    const auto normalized = juce::jlimit(0.0f, 1.0f, value);
    const auto index = juce::jlimit(0, 4, (int) std::floor(normalized * 5.0f));
    return words[(size_t) index];
}

void ShineAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);

    const auto scale = getUiScale(getWidth(), getHeight());
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    for (float x = 20.0f * scale; x < (float) getWidth(); x += 22.0f * scale)
        g.drawVerticalLine((int) std::round(x), 0.0f, (float) getHeight());
    for (float y = 20.0f * scale; y < (float) getHeight(); y += 22.0f * scale)
        g.drawHorizontalLine((int) std::round(y), 0.0f, (float) getWidth());

    auto bounds = getLocalBounds().toFloat().reduced(0.5f * scale);
    g.setColour(outline);
    g.drawRoundedRectangle(bounds, 14.0f * scale, 1.0f * scale);

    g.setColour(lineSoft);
    g.drawLine((float) headerBounds.getX(), (float) headerBounds.getBottom(),
               (float) headerBounds.getRight(), (float) headerBounds.getBottom(), 1.0f);

    // Adaptive processing dot with pulse
    const auto pulseAlpha = juce::jmap(std::sin(pulsePhase), -1.0f, 1.0f, 0.50f, 1.0f);
    const auto adaptiveLabelBounds = adaptiveLabel.getBounds().toFloat();
    const auto dotCentre = juce::Point<float>(adaptiveLabelBounds.getX() - 8.0f * scale,
                                              adaptiveLabelBounds.getCentreY());
    const auto ringRadius = juce::jmap(std::sin(pulsePhase), -1.0f, 1.0f, 3.5f, 6.5f) * scale;
    g.setColour(accent.withAlpha(0.12f * pulseAlpha));
    g.fillEllipse(dotCentre.x - ringRadius, dotCentre.y - ringRadius, ringRadius * 2.0f, ringRadius * 2.0f);
    g.setColour(accent.withAlpha(pulseAlpha));
    const auto dotRadius = 2.5f * scale;
    g.fillEllipse(dotCentre.x - dotRadius, dotCentre.y - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);

    // Shared container pill background behind both tabs
    const auto tabsAreaFloat = tabsBounds.toFloat();
    const auto containerHeight = tabsAreaFloat.getHeight() * 0.85f;
    const auto containerY = tabsAreaFloat.getY() + (tabsAreaFloat.getHeight() - containerHeight) * 0.5f;
    juce::Rectangle<float> containerPill;
    {
        const auto firstButton = modeButtons[0].getBounds().toFloat();
        const auto lastButton  = modeButtons[modeButtons.size() - 1].getBounds().toFloat();
        containerPill = juce::Rectangle<float>(firstButton.getX() - 2.0f * scale,
                                               containerY,
                                               (lastButton.getRight() + 2.0f * scale) - (firstButton.getX() - 6.0f * scale),
                                               containerHeight);
    }
    g.setColour(juce::Colour(0xFFE8DFCD));
    g.fillRoundedRectangle(containerPill, containerPill.getHeight() * 0.5f);

    // Active tab highlight
    for (int i = 0; i < (int) modeButtons.size(); ++i)
    {
        auto buttonBounds = modeButtons[(size_t) i].getBounds().toFloat();
        if (i == selectedMode)
        {
            auto activePill = buttonBounds;
            activePill.setY(containerY + 2.0f * scale);
            activePill.setHeight(containerHeight - 4.0f * scale);
            activePill = activePill.reduced(2.0f * scale, 0.0f);
            g.setColour(juce::Colour(0xFFFFFFFF));
            g.fillRoundedRectangle(activePill, activePill.getHeight() * 0.5f);
            g.setColour(accent.withAlpha(0.20f));
            g.drawRoundedRectangle(activePill, activePill.getHeight() * 0.5f, 1.0f);
        }
    }

    // Adaptation bars
    auto drawAdaptBar = [&g](juce::Rectangle<float> area, juce::Colour fillColour, float amount)
    {
        g.setColour(meterOff);
        g.fillRoundedRectangle(area, area.getHeight() * 0.5f);
        g.setColour(fillColour);
        auto fill = area;
        fill.setWidth(area.getWidth() * juce::jlimit(0.0f, 1.0f, amount));
        g.fillRoundedRectangle(fill, fill.getHeight() * 0.5f);
    };

    auto adaptationArea = adaptationBounds.toFloat();
    const auto barWidth = 60.0f * scale;
    const auto barHeight = 4.0f * scale;
    const auto barGap = 24.0f * scale;
    const auto totalBarWidth = barWidth * 2.0f + barGap;
    const auto barStartX = adaptationArea.getCentreX() - totalBarWidth * 0.5f;
    const auto barY = adaptationArea.getY() + 14.0f * scale;
    const auto presAmount = juce::jlimit(0.0f, 1.0f, processorRef.getAdaptPresenceScale());
    const auto airAmount = juce::jlimit(0.0f, 1.0f, processorRef.getAdaptAirScale());
    drawAdaptBar({ barStartX, barY, barWidth, barHeight }, accent, presAmount);
    drawAdaptBar({ barStartX + barWidth + barGap, barY, barWidth, barHeight }, airAccent, airAmount);

    // Output meter (6-bar staircase)
    auto outputMeterArea = outputBounds.toFloat();
    outputMeterArea.removeFromBottom(17.0f * scale);

    auto drawMeter = [&g, this](juce::Rectangle<float> area)
    {
        static constexpr std::array<float, 6> heights { 12.0f, 15.0f, 18.0f, 22.0f, 26.0f, 30.0f };
        const auto scaleLocal = (float) getHeight() / (float) kNativeHeight;
        const auto barWidthLocal = 3.0f * scaleLocal;
        const auto gapLocal = 2.0f * scaleLocal;
        const auto totalWidthLocal = barWidthLocal * 6.0f + gapLocal * 5.0f;
        float x = area.getCentreX() - totalWidthLocal * 0.5f;
        const auto level = juce::jlimit(0.0f, 1.0f, displayedOutputLevel);

        for (int i = 0; i < 6; ++i)
        {
            const auto litThreshold = (float) (i + 1) / 6.0f;
            const auto height = heights[(size_t) i] * scaleLocal;
            const auto bar = juce::Rectangle<float>(x, area.getBottom() - height, barWidthLocal, height);
            g.setColour(meterOff);
            g.fillRoundedRectangle(bar, barWidthLocal * 0.5f);

            if (level >= litThreshold - 0.08f)
            {
                auto fillColour = i == 5 ? accent.interpolatedWith(inkMuted, 0.25f) : accent;
                g.setColour(fillColour);
                g.fillRoundedRectangle(bar, barWidthLocal * 0.5f);
            }

            x += barWidthLocal + gapLocal;
        }
    };

    drawMeter(outputMeterArea);
}

void ShineAudioProcessorEditor::resized()
{
    const auto scale = getUiScale(getWidth(), getHeight());
    auto sc = [scale](float v) { return juce::roundToInt(v * scale); };

    auto bounds = getLocalBounds().reduced(sc(20.0f));

    brandLabel.setFont(makeBrandSerif(22.0f * scale, 0.18f));
    productLabel.setFont(makeBrandSerif(22.0f * scale, 0.18f));
    adaptiveLabel.setFont(makeSans(10.0f * scale, true, 0.16f));
    presenceLabel.setFont(makeSans(11.0f * scale, true, 0.16f));
    airLabel.setFont(makeSans(11.0f * scale, true, 0.16f));
    presenceValueLabel.setFont(makeSerif(14.0f * scale, true, 0.01f));
    airValueLabel.setFont(makeSerif(14.0f * scale, true, 0.01f));
    adaptationTitleLabel.setFont(makeSans(9.0f * scale, true, 0.12f));
    adaptationPresenceLabel.setFont(makeSans(8.0f * scale, true, 0.08f));
    adaptationAirLabel.setFont(makeSans(8.0f * scale, true, 0.08f));
    outputLabel.setFont(makeSans(10.0f * scale, true, 0.16f));

    const auto sectionGap = sc(8.0f);

    headerBounds = bounds.removeFromTop(sc(32.0f));
    bounds.removeFromTop(sectionGap);
    tabsBounds = bounds.removeFromTop(sc(28.0f));
    bounds.removeFromTop(sectionGap);
    knobsBounds = bounds.removeFromTop(sc(130.0f));
    bounds.removeFromTop(sectionGap);
    adaptationBounds = bounds.removeFromTop(sc(31.0f));
    bounds.removeFromTop(sectionGap);
    outputBounds = bounds.removeFromTop(sc(47.0f));

    auto headerLeft = headerBounds;
    brandLabel.setBounds(headerLeft.removeFromLeft(sc(95.0f)));
    headerLeft.removeFromLeft(sc(8.0f));
    productLabel.setBounds(headerLeft.removeFromLeft(sc(140.0f)));

    // Tabs
    auto tabsArea = tabsBounds.withSizeKeepingCentre(sc(280.0f), juce::roundToInt(tabsBounds.getHeight() * 0.85f));
    const auto tabWidth = (tabsArea.getWidth() - sc(8.0f)) / 2;
    for (int i = 0; i < (int) modeButtons.size(); ++i)
    {
        modeButtons[(size_t) i].setBounds(tabsArea.removeFromLeft(tabWidth));
        if (i == 0)
            tabsArea.removeFromLeft(sc(8.0f));
    }

    adaptiveLabel.setBounds(headerBounds.getRight() - sc(160.0f),
                            headerBounds.getY() + sc(11.0f),
                            sc(155.0f),
                            sc(13.0f));

    // Knobs
    const auto knobSize = juce::roundToInt(90.0f * (float) getWidth() / (float) kNativeWidth);
    const auto knobGap = juce::roundToInt(56.0f * (float) getWidth() / (float) kNativeWidth);
    const auto totalKnobWidth = knobSize * 2 + knobGap;
    const auto knobStartX = knobsBounds.getCentreX() - totalKnobWidth / 2;
    const auto knobY = knobsBounds.getY();

    presenceSlider.setBounds(knobStartX, knobY, knobSize, knobSize);
    airSlider.setBounds(knobStartX + knobSize + knobGap, knobY, knobSize, knobSize);

    presenceLabel.setBounds(knobStartX - sc(12.0f), knobY + knobSize + sc(8.0f), knobSize + sc(24.0f), sc(13.0f));
    airLabel.setBounds(knobStartX + knobSize + knobGap - sc(12.0f), knobY + knobSize + sc(8.0f), knobSize + sc(24.0f), sc(13.0f));
    presenceValueLabel.setBounds(knobStartX - sc(12.0f), knobY + knobSize + sc(25.0f), knobSize + sc(24.0f), sc(15.0f));
    airValueLabel.setBounds(knobStartX + knobSize + knobGap - sc(12.0f), knobY + knobSize + sc(25.0f), knobSize + sc(24.0f), sc(15.0f));

    // Adaptation bars
    adaptationTitleLabel.setBounds(adaptationBounds.getX(), adaptationBounds.getY(), adaptationBounds.getWidth(), sc(11.0f));

    const auto barWidthPx = sc(60.0f);
    const auto barGapPx = sc(24.0f);
    const auto totalBarWidthPx = barWidthPx * 2 + barGapPx;
    const auto barStartX = adaptationBounds.getCentreX() - totalBarWidthPx / 2;
    const auto adaptLabelY = adaptationBounds.getY() + sc(21.0f);
    adaptationPresenceLabel.setBounds(barStartX, adaptLabelY, barWidthPx, sc(10.0f));
    adaptationAirLabel.setBounds(barStartX + barWidthPx + barGapPx, adaptLabelY, barWidthPx, sc(10.0f));

    auto outputLabelBounds = outputBounds;
    outputLabel.setBounds(outputLabelBounds.removeFromBottom(sc(11.0f)));
}

void ShineAudioProcessorEditor::timerCallback()
{
    pulsePhase += 0.12f;

    const auto peak = processorRef.getOutputLevel();
    const auto db = juce::Decibels::gainToDecibels(peak, -48.0f);
    const auto meterLevel = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -48.0f, 0.0f, 0.0f, 1.0f));
    displayedOutputLevel += (meterLevel - displayedOutputLevel) * 0.25f;

    updateDynamicLabels();
    repaint();
}
