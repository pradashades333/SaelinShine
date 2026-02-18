#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace shine::ShineColours;

// ---------------------------------------------------------------------------
// Layout constants — 520 x 380 px (spec dimensions)
// ---------------------------------------------------------------------------
namespace L {
    constexpr int W = 520;
    constexpr int H = 380;

    // Header
    constexpr int headerH = 50;

    // Tabs
    constexpr int tabY   = 54;
    constexpr int tabH   = 28;
    constexpr int tabW   = 148;
    constexpr int tabGap = 12;

    // "• ADAPTIVE PROCESSING" label
    constexpr int adaptLabelY = 86;
    constexpr int adaptLabelH = 16;

    // Knob area — spec: 80px outer diameter knobs, 40px label area below
    constexpr int knobAreaY = 104;
    constexpr int knobAreaH = 120;  // 80px knob + 40px labels
    constexpr int knobW     = 130;
    constexpr int knobGap   = 24;

    // Separator lines
    constexpr int sep1Y = 226;
    constexpr int sep2Y = 283;

    // Adaptation bars section
    constexpr int adaptBarsY = 229;
    constexpr int adaptBarsH = 52;

    // I/O Meters section
    constexpr int metersY = 286;
    constexpr int metersH = 86;
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

    juce::ColourGradient glow(accentGold.withAlpha(0.03f),
                               W * 0.5f, H * 0.38f,
                               juce::Colours::transparentBlack,
                               W * 0.5f, H * 0.38f + 180.0f, true);
    g.setGradientFill(glow);
    g.fillRect(getLocalBounds());

    // ---- Header ------------------------------------------------------------
    g.setColour(juce::Colour(0xff070605));
    g.fillRect(0, 0, W, L::headerH);
    g.setColour(accentGold.withAlpha(0.15f));
    g.fillRect(0, L::headerH - 1, W, 1);

    g.setColour(textPrimary);
    g.setFont(lookAndFeel.getLogoFont(20.0f));
    g.drawText("SAELIN", 20, 14, 100, 24, juce::Justification::centredLeft);

    g.setColour(textSecondary);
    g.setFont(lookAndFeel.getUIFont(10.0f));
    g.drawText("SHINE", 124, 18, 52, 16, juce::Justification::centredLeft);

    // ---- "• ADAPTIVE PROCESSING" -------------------------------------------
    const float dotX = W * 0.5f - 88.0f;
    const float dotY = L::adaptLabelY + L::adaptLabelH * 0.5f - 2.5f;
    g.setColour(accentGold);
    g.fillEllipse(dotX, dotY, 5.0f, 5.0f);

    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(9.0f));
    g.drawText("ADAPTIVE PROCESSING",
               static_cast<int>(dotX) + 9, L::adaptLabelY,
               180, L::adaptLabelH, juce::Justification::centredLeft);

    // ---- Separator lines ---------------------------------------------------
    g.setColour(bgCard.withAlpha(0.8f));
    g.fillRect(0, L::sep1Y, W, 1);
    g.fillRect(0, L::sep2Y, W, 1);

    // ---- ADAPTATION --------------------------------------------------------
    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(8.5f));
    g.drawText("ADAPTATION", 0, L::adaptBarsY + 2, W, 14, juce::Justification::centred);

    // Two bars side by side (PRESENCE left, AIR right)
    const int barW      = 110;
    const int barH      = 5;
    const int barGapX   = 20;
    const int barsBlockW = barW * 2 + barGapX;
    const int barsLeft  = (W - barsBlockW) / 2;
    const int presBarX  = barsLeft;
    const int airBarX   = barsLeft + barW + barGapX;
    const int barY      = L::adaptBarsY + 22;

    auto drawAdaptBar = [&](int bx, float fill, juce::Colour barColour,
                             const juce::String& lbl) {
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
        g.drawText(lbl, bx, barY + barH + 5, barW, 12, juce::Justification::centred);
    };

    drawAdaptBar(presBarX, adaptPresScale, accentGold, "PRESENCE");
    drawAdaptBar(airBarX,  adaptAirScale,  accentAir,  "AIR");

    // ---- I/O Meters (staircase bars) ---------------------------------------
    const int bh[6]   = { 16, 22, 28, 36, 45, 56 };
    const int bw      = 5;
    const int bgap    = 3;
    const int meterW  = 6 * bw + 5 * bgap;         // 45px
    const int meterBottomY = L::metersY + L::metersH - 16;

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

            g.setColour(bgCard);
            g.fillRoundedRectangle(static_cast<float>(bx), static_cast<float>(bTopY),
                                    static_cast<float>(bw), static_cast<float>(bMaxH), 1.5f);

            int fillH = static_cast<int>(norm * bMaxH);
            if (fillH > 0) {
                bool isClip = (b == 5 && norm > 0.87f);
                g.setColour(isClip ? accentCopper : accentGold);
                g.fillRoundedRectangle(static_cast<float>(bx),
                                        static_cast<float>(meterBottomY - fillH),
                                        static_cast<float>(bw),
                                        static_cast<float>(fillH), 1.5f);
            }
        }
        g.setColour(textMuted);
        g.setFont(lookAndFeel.getUIFont(8.5f));
        g.drawText(lbl, leftX - 10, meterBottomY + 4, meterW + 20, 12,
                   juce::Justification::centred);
    };

    // Closer to centre per user request
    const int inMeterX  = W / 2 - 75 - meterW / 2;
    const int outMeterX = W / 2 + 75 - meterW / 2;

    drawMeter(inMeterX,  inputLevel,  "INPUT");
    drawMeter(outMeterX, outputLevel, "OUTPUT");
}

// ---------------------------------------------------------------------------
void ShineAudioProcessorEditor::resized() {
    const int W = getWidth();

    // Bypass button — top right
    bypassButton.setBounds(W - 46, 11, 28, 28);

    // Tabs — centred
    const int tabRowW   = L::tabW * 2 + L::tabGap;
    const int tabStartX = (W - tabRowW) / 2;
    tabVocalClarity.setBounds  (tabStartX,                      L::tabY, L::tabW, L::tabH);
    tabAcousticDetail.setBounds(tabStartX + L::tabW + L::tabGap, L::tabY, L::tabW, L::tabH);

    // Knobs — centred pair
    const int twoKnobsW  = L::knobW * 2 + L::knobGap;
    const int knobStartX = (W - twoKnobsW) / 2;
    presenceKnob.setBounds(knobStartX,                        L::knobAreaY, L::knobW, L::knobAreaH);
    airKnob.setBounds     (knobStartX + L::knobW + L::knobGap, L::knobAreaY, L::knobW, L::knobAreaH);
}
