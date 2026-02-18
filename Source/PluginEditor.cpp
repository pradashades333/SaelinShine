#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace shine::ShineColours;

// ---------------------------------------------------------------------------
// Layout constants — all measurements in pixels, window is 520 x 380
// ---------------------------------------------------------------------------
namespace layout {
    constexpr int W = 520;
    constexpr int H = 380;

    // Header
    constexpr int headerH  = 50;

    // Tabs
    constexpr int tabY     = 56;
    constexpr int tabH     = 28;
    constexpr int tabW     = 140;
    constexpr int tabGap   = 10;

    // "• ADAPTIVE PROCESSING" label
    constexpr int adaptLabelY = 90;
    constexpr int adaptLabelH = 18;

    // Knobs — placed side by side, centred
    constexpr int knobAreaY  = 108;
    constexpr int knobAreaH  = 140;  // 90px knob + 50px labels
    constexpr int knobW      = 150;  // component width per knob

    // Adaptation bars section
    constexpr int adaptBarsY  = 256;
    constexpr int adaptBarsH  = 64;

    // I/O Meters
    constexpr int metersY    = 300;
    constexpr int metersH    = 64;
}

// ---------------------------------------------------------------------------
ShineAudioProcessorEditor::ShineAudioProcessorEditor(ShineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    setLookAndFeel(&lookAndFeel);

    // Knob children
    addAndMakeVisible(presenceKnob);
    addAndMakeVisible(airKnob);

    // Bypass
    bypassButton.setButtonText("");
    addAndMakeVisible(bypassButton);

    // Tabs — use setClickingTogglesState(false), we toggle manually
    for (auto* btn : { &tabVocalClarity, &tabAcousticDetail }) {
        btn->setClickingTogglesState(false);
        addAndMakeVisible(btn);
    }

    tabVocalClarity.onClick = [this] {
        applyPreset(0);
    };
    tabAcousticDetail.onClick = [this] {
        applyPreset(1);
    };

    // APVTS attachments
    presenceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "presence", presenceKnob.getSlider());
    airAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "air", airKnob.getSlider());
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "bypass", bypassButton);

    // Determine initial tab highlight from current param values
    float pres = audioProcessor.getAPVTS().getRawParameterValue("presence")->load();
    float air  = audioProcessor.getAPVTS().getRawParameterValue("air")->load();
    if (std::abs(pres - 4.5f) < 0.05f && std::abs(air - 0.20f) < 0.05f)
        selectedTab = 1;
    else
        selectedTab = 0;  // default to Vocal Clarity
    updateTabStates();

    startTimerHz(30);
    setSize(layout::W, layout::H);
}

ShineAudioProcessorEditor::~ShineAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::applyPreset(int presetIndex) {
    selectedTab = presetIndex;
    updateTabStates();

    if (presetIndex == 0) {
        // Vocal Clarity: P=3.0 / A=0.40
        audioProcessor.getAPVTS().getParameterAsValue("presence").setValue(3.0);
        audioProcessor.getAPVTS().getParameterAsValue("air").setValue(0.40);
    } else {
        // Acoustic Detail: P=4.5 / A=0.20 (provisional)
        audioProcessor.getAPVTS().getParameterAsValue("presence").setValue(4.5);
        audioProcessor.getAPVTS().getParameterAsValue("air").setValue(0.20);
    }
}

void ShineAudioProcessorEditor::updateTabStates() {
    tabVocalClarity.setToggleState  (selectedTab == 0, juce::dontSendNotification);
    tabAcousticDetail.setToggleState(selectedTab == 1, juce::dontSendNotification);
    tabVocalClarity.repaint();
    tabAcousticDetail.repaint();
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::timerCallback() {
    inputLevel    = audioProcessor.getInputLevel();
    outputLevel   = audioProcessor.getOutputLevel();
    adaptPresScale = audioProcessor.getAdaptPresenceScale();
    adaptAirScale  = audioProcessor.getAdaptAirScale();
    repaint();
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::paint(juce::Graphics& g) {
    const int W = getWidth();
    const int H = getHeight();

    // --- Background ---
    g.fillAll(bgDeep);

    // Subtle warm radial glow from center
    juce::ColourGradient centerGlow(
        accentGold.withAlpha(0.04f),
        W * 0.5f, H * 0.45f,
        juce::Colours::transparentBlack,
        W * 0.5f, H * 0.45f + 180.0f, true);
    g.setGradientFill(centerGlow);
    g.fillRect(getLocalBounds());

    // --- Header ---
    g.setColour(juce::Colour(0xff070605));
    g.fillRect(0, 0, W, layout::headerH);
    g.setColour(accentGold.withAlpha(0.12f));
    g.fillRect(0, layout::headerH - 1, W, 1);

    // Logo: "SAELIN"
    g.setColour(textPrimary);
    g.setFont(lookAndFeel.getLogoFont(18.0f));
    g.drawText("SAELIN", 18, 15, 90, 22, juce::Justification::centredLeft);

    // "SHINE" subtitle
    g.setColour(textSecondary);
    g.setFont(lookAndFeel.getUIFont(9.5f));
    g.drawText("SHINE", 112, 19, 50, 14, juce::Justification::centredLeft);

    // --- "• ADAPTIVE PROCESSING" label ---
    g.setColour(accentGold);
    g.fillEllipse(W * 0.5f - 70.0f, static_cast<float>(layout::adaptLabelY) + 5.0f, 5.0f, 5.0f);
    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(9.0f));
    g.drawText("ADAPTIVE PROCESSING",
               static_cast<int>(W * 0.5f) - 60, layout::adaptLabelY, 130, layout::adaptLabelH,
               juce::Justification::centredLeft);

    // --- ADAPTATION section ---
    const int barSectionX = 80;
    const int barSectionW = W - 160;
    const int barY0 = layout::adaptBarsY + 18;
    const int barH  = 5;
    const int barSpacing = 22;

    // Section label
    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(8.5f));
    g.drawText("ADAPTATION", 0, layout::adaptBarsY, W, 14, juce::Justification::centred);

    // Thin separator
    g.setColour(bgCard);
    g.fillRect(barSectionX, layout::adaptBarsY + 14, barSectionW, 1);

    // Helper: draw one adaptation bar
    auto drawAdaptBar = [&](int bY, float fillFraction, juce::Colour barColour,
                             const juce::String& barLabel) {
        // Track
        g.setColour(bgCard);
        g.fillRoundedRectangle(static_cast<float>(barSectionX), static_cast<float>(bY),
                                static_cast<float>(barSectionW), static_cast<float>(barH), 2.5f);
        // Fill
        int fillW = static_cast<int>(barSectionW * juce::jlimit(0.0f, 1.0f, fillFraction));
        if (fillW > 0) {
            g.setColour(barColour.withAlpha(0.8f));
            g.fillRoundedRectangle(static_cast<float>(barSectionX), static_cast<float>(bY),
                                    static_cast<float>(fillW), static_cast<float>(barH), 2.5f);
        }
        // Label
        g.setColour(textMuted);
        g.setFont(lookAndFeel.getUIFont(8.0f));
        g.drawText(barLabel, barSectionX - 70, bY - 1, 65, barH + 2,
                   juce::Justification::centredRight);
    };

    drawAdaptBar(barY0,              adaptPresScale, accentGold, "PRESENCE");
    drawAdaptBar(barY0 + barSpacing, adaptAirScale,  accentAir,  "AIR");

    // --- I/O Meters (6-segment LED style) ---
    const int segCount   = 6;
    const int segW       = 4;
    const int segH       = 8;
    const int segGap     = 3;
    const int meterTotalH = segCount * (segH + segGap) - segGap;
    const int meterTotalW = segW;

    // Left meter (INPUT) — centred around x = W/2 - 70
    const int inMeterX  = W / 2 - 78;
    const int outMeterX = W / 2 + 78 - segW;
    const int meterTopY = layout::metersY + (layout::metersH - meterTotalH - 14) / 2;

    auto dbToNorm = [](float level) -> float {
        return juce::jlimit(0.0f, 1.0f,
            (20.0f * std::log10(juce::jmax(0.00001f, level)) + 48.0f) / 48.0f);
    };

    auto drawMeter = [&](int mx, float level, juce::Colour colour, const juce::String& mLabel) {
        float norm = dbToNorm(level);
        int litSegs = static_cast<int>(norm * segCount + 0.5f);

        for (int s = 0; s < segCount; ++s) {
            // s=0 is bottom, s=segCount-1 is top
            int sy = meterTopY + (segCount - 1 - s) * (segH + segGap);
            bool lit = s < litSegs;

            juce::Colour segCol;
            if (lit) {
                segCol = (s == segCount - 1) ? accentCopper : colour;  // top = copper/clip
            } else {
                segCol = bgCard;
            }
            g.setColour(segCol);
            g.fillRoundedRectangle(static_cast<float>(mx), static_cast<float>(sy),
                                    static_cast<float>(segW), static_cast<float>(segH), 1.5f);
        }

        // Meter label below
        g.setColour(textMuted);
        g.setFont(lookAndFeel.getUIFont(8.0f));
        g.drawText(mLabel, mx - 16, meterTopY + meterTotalH + 4, segW + 32, 12,
                   juce::Justification::centred);
    };

    drawMeter(inMeterX,  inputLevel,  accentGold, "INPUT");
    drawMeter(outMeterX, outputLevel, accentGold, "OUTPUT");
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::resized() {
    const int W = getWidth();

    // Bypass button — top right, gold circle
    bypassButton.setBounds(W - 46, 11, 28, 28);

    // Tabs — centred in row
    const int tabRowX = (W - (layout::tabW * 2 + layout::tabGap)) / 2;
    tabVocalClarity.setBounds  (tabRowX,                         layout::tabY, layout::tabW, layout::tabH);
    tabAcousticDetail.setBounds(tabRowX + layout::tabW + layout::tabGap, layout::tabY, layout::tabW, layout::tabH);

    // Knobs — side by side, centred
    const int knobsX = (W - layout::knobW * 2 - 20) / 2;
    presenceKnob.setBounds(knobsX,                     layout::knobAreaY, layout::knobW, layout::knobAreaH);
    airKnob.setBounds     (knobsX + layout::knobW + 20, layout::knobAreaY, layout::knobW, layout::knobAreaH);
}
