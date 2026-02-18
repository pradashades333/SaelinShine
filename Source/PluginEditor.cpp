#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace shine::ShineColours;

// ---------------------------------------------------------------------------
// Layout — portrait window, ~400 x 560 px
// ---------------------------------------------------------------------------
namespace L {
    constexpr int W = 400;
    constexpr int H = 560;

    // Header
    constexpr int headerH = 50;

    // Tabs (pill-shaped buttons)
    constexpr int tabY   = 58;
    constexpr int tabH   = 30;
    constexpr int tabW   = 136;
    constexpr int tabGap = 12;

    // "• ADAPTIVE PROCESSING" indicator
    constexpr int adaptLabelY = 96;
    constexpr int adaptLabelH = 18;

    // Knob area (two knobs side by side)
    constexpr int knobAreaY = 116;
    constexpr int knobAreaH = 165;  // 110px knob + 55px labels
    constexpr int knobW     = 148;
    constexpr int knobGap   = 16;

    // Horizontal separators
    constexpr int sep1Y = 287;   // below knobs
    constexpr int sep2Y = 362;   // below adaptation

    // Adaptation bars section
    constexpr int adaptBarsY  = 294;
    constexpr int adaptBarsH  = 62;

    // I/O Meters section
    constexpr int metersY = 368;
    constexpr int metersH = 160;
}

// ---------------------------------------------------------------------------
ShineAudioProcessorEditor::ShineAudioProcessorEditor(ShineAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(presenceKnob);
    addAndMakeVisible(airKnob);

    bypassButton.setButtonText("");
    addAndMakeVisible(bypassButton);

    for (auto* btn : { &tabVocalClarity, &tabAcousticDetail }) {
        btn->setClickingTogglesState(false);
        addAndMakeVisible(btn);
    }

    tabVocalClarity.onClick   = [this] { applyPreset(0); };
    tabAcousticDetail.onClick = [this] { applyPreset(1); };

    presenceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "presence", presenceKnob.getSlider());
    airAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "air", airKnob.getSlider());
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), "bypass", bypassButton);

    // Set initial tab highlight
    float pres = audioProcessor.getAPVTS().getRawParameterValue("presence")->load();
    float air  = audioProcessor.getAPVTS().getRawParameterValue("air")->load();
    selectedTab = (std::abs(pres - 4.5f) < 0.05f && std::abs(air - 0.20f) < 0.05f) ? 1 : 0;
    updateTabStates();

    startTimerHz(30);
    setSize(L::W, L::H);
}

ShineAudioProcessorEditor::~ShineAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::applyPreset(int idx) {
    selectedTab = idx;
    updateTabStates();
    if (idx == 0) {
        audioProcessor.getAPVTS().getParameterAsValue("presence").setValue(3.0);
        audioProcessor.getAPVTS().getParameterAsValue("air").setValue(0.40);
    } else {
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
    inputLevel     = audioProcessor.getInputLevel();
    outputLevel    = audioProcessor.getOutputLevel();
    adaptPresScale = audioProcessor.getAdaptPresenceScale();
    adaptAirScale  = audioProcessor.getAdaptAirScale();
    repaint();
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::paint(juce::Graphics& g) {
    const int W = getWidth();
    const int H = getHeight();

    // ---- Background --------------------------------------------------------
    g.fillAll(bgDeep);

    // Subtle warm centre glow
    juce::ColourGradient glow(accentGold.withAlpha(0.035f),
                               W * 0.5f, H * 0.40f,
                               juce::Colours::transparentBlack,
                               W * 0.5f, H * 0.40f + 200.0f, true);
    g.setGradientFill(glow);
    g.fillRect(getLocalBounds());

    // ---- Header ------------------------------------------------------------
    g.setColour(juce::Colour(0xff070605));
    g.fillRect(0, 0, W, L::headerH);
    g.setColour(accentGold.withAlpha(0.15f));
    g.fillRect(0, L::headerH - 1, W, 1);

    // "SAELIN" logo
    g.setColour(textPrimary);
    g.setFont(lookAndFeel.getLogoFont(20.0f));
    g.drawText("SAELIN", 18, 14, 100, 24, juce::Justification::centredLeft);

    // "SHINE" sub-label
    g.setColour(textSecondary);
    g.setFont(lookAndFeel.getUIFont(10.0f));
    g.drawText("SHINE", 122, 18, 52, 16, juce::Justification::centredLeft);

    // ---- "• ADAPTIVE PROCESSING" -------------------------------------------
    const float dotX = W * 0.5f - 82.0f;
    const float dotY = L::adaptLabelY + L::adaptLabelH * 0.5f - 2.5f;
    g.setColour(accentGold);
    g.fillEllipse(dotX, dotY, 5.0f, 5.0f);

    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(9.0f));
    g.drawText("ADAPTIVE PROCESSING",
               static_cast<int>(dotX) + 9, L::adaptLabelY,
               172, L::adaptLabelH, juce::Justification::centredLeft);

    // ---- Separator lines ---------------------------------------------------
    g.setColour(bgCard.withAlpha(0.8f));
    g.fillRect(0, L::sep1Y, W, 1);
    g.fillRect(0, L::sep2Y, W, 1);

    // ---- ADAPTATION section ------------------------------------------------
    // Title
    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(8.5f));
    g.drawText("ADAPTATION", 0, L::adaptBarsY + 2, W, 14, juce::Justification::centred);

    // Two horizontal bars, side by side
    const int barW  = 100;   // width of each bar track
    const int barH  = 5;
    const int barGapX = 16;  // gap between the two bars
    const int barsBlockW = barW * 2 + barGapX;  // 216px
    const int barsLeft = (W - barsBlockW) / 2;   // 92px
    const int presBarX = barsLeft;
    const int airBarX  = barsLeft + barW + barGapX;
    const int barY     = L::adaptBarsY + 22;

    auto drawAdaptBar = [&](int bx, float fill, juce::Colour barColour, const juce::String& lbl) {
        g.setColour(bgCard);
        g.fillRoundedRectangle(static_cast<float>(bx), static_cast<float>(barY),
                                static_cast<float>(barW), static_cast<float>(barH), 2.5f);
        int fw = static_cast<int>(juce::jlimit(0.0f, 1.0f, fill) * barW);
        if (fw > 0) {
            g.setColour(barColour.withAlpha(0.85f));
            g.fillRoundedRectangle(static_cast<float>(bx), static_cast<float>(barY),
                                    static_cast<float>(fw), static_cast<float>(barH), 2.5f);
        }
        g.setColour(textMuted);
        g.setFont(lookAndFeel.getUIFont(8.0f));
        g.drawText(lbl, bx, barY + barH + 6, barW, 12, juce::Justification::centred);
    };

    drawAdaptBar(presBarX, adaptPresScale, accentGold, "PRESENCE");
    drawAdaptBar(airBarX,  adaptAirScale,  accentAir,  "AIR");

    // ---- I/O Meters (staircase bars) ---------------------------------------
    // 6 bars with increasing heights (left = short, right = tall)
    const int bh[6] = { 18, 24, 30, 38, 47, 58 };   // bar heights in px
    const int bw    = 5;                              // bar width
    const int bgap  = 3;                              // gap between bars
    const int meterW = 6 * bw + 5 * bgap;            // 45px total
    const int meterSpacing = 180;                     // centre-to-centre distance
    const int meterBottomY = L::metersY + L::metersH - 26;

    auto dbToNorm = [](float level) -> float {
        return juce::jlimit(0.0f, 1.0f,
            (20.0f * std::log10(juce::jmax(0.00001f, level)) + 48.0f) / 48.0f);
    };

    auto drawMeter = [&](int leftX, float level, const juce::String& lbl) {
        float norm = dbToNorm(level);

        for (int b = 0; b < 6; ++b) {
            const int bx    = leftX + b * (bw + bgap);
            const int bMaxH = bh[b];
            const int bTopY = meterBottomY - bMaxH;

            // Background (full height of this bar, dark)
            g.setColour(bgCard);
            g.fillRoundedRectangle(static_cast<float>(bx), static_cast<float>(bTopY),
                                    static_cast<float>(bw), static_cast<float>(bMaxH), 1.5f);

            // Fill from bottom
            int fillH = static_cast<int>(norm * bMaxH);
            if (fillH > 0) {
                bool isClip = (b == 5 && norm > 0.87f);
                g.setColour(isClip ? accentCopper : accentGold);
                g.fillRoundedRectangle(static_cast<float>(bx),
                                        static_cast<float>(meterBottomY - fillH),
                                        static_cast<float>(bw), static_cast<float>(fillH), 1.5f);
            }
        }

        // Label below
        g.setColour(textMuted);
        g.setFont(lookAndFeel.getUIFont(8.5f));
        g.drawText(lbl, leftX - 10, meterBottomY + 6, meterW + 20, 12,
                   juce::Justification::centred);
    };

    const int inMeterX  = W / 2 - meterSpacing / 2 - meterW / 2;
    const int outMeterX = W / 2 + meterSpacing / 2 - meterW / 2;

    drawMeter(inMeterX,  inputLevel,  "INPUT");
    drawMeter(outMeterX, outputLevel, "OUTPUT");
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::resized() {
    const int W = getWidth();

    // Bypass button — top right corner
    bypassButton.setBounds(W - 46, 11, 28, 28);

    // Tabs — centred
    const int tabRowW  = L::tabW * 2 + L::tabGap;
    const int tabStartX = (W - tabRowW) / 2;
    tabVocalClarity.setBounds  (tabStartX,              L::tabY, L::tabW, L::tabH);
    tabAcousticDetail.setBounds(tabStartX + L::tabW + L::tabGap, L::tabY, L::tabW, L::tabH);

    // Knobs — two side by side, centred
    const int twoKnobsW = L::knobW * 2 + L::knobGap;
    const int knobStartX = (W - twoKnobsW) / 2;
    presenceKnob.setBounds(knobStartX,                  L::knobAreaY, L::knobW, L::knobAreaH);
    airKnob.setBounds     (knobStartX + L::knobW + L::knobGap, L::knobAreaY, L::knobW, L::knobAreaH);
}
