#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace shine::ShineColours;

namespace L {
    constexpr int W = 520;
    constexpr int H = 380;

    constexpr int headerH = 50;

    constexpr int tabY   = 64;
    constexpr int tabH   = 28;
    constexpr int tabW   = 148;
    constexpr int tabGap = 12;

    constexpr int adaptLabelY = 96;
    constexpr int adaptLabelH = 16;

    constexpr int knobAreaY = 114;
    constexpr int knobAreaH = 120;
    constexpr int knobW     = 130;
    constexpr int knobGap   = 24;

    constexpr int sep1Y = 236;
    constexpr int sep2Y = 293;

    constexpr int adaptBarsY = 239;
    constexpr int adaptBarsH = 52;

    constexpr int metersY = 296;
    constexpr int metersH = 76;
}

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

    int mode = static_cast<int>(audioProcessor.getAPVTS().getRawParameterValue("mode")->load());
    selectedTab = (mode == 1) ? 1 : 0;
    updateTabStates();

    startTimerHz(30);
    setSize(L::W, L::H);
}

ShineAudioProcessorEditor::~ShineAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

void ShineAudioProcessorEditor::applyPreset(int idx) {
    selectedTab = idx;
    updateTabStates();
    audioProcessor.getAPVTS().getParameterAsValue("mode").setValue(idx);
    if (idx == 0) {
        audioProcessor.getAPVTS().getParameterAsValue("presence").setValue(0.67);
        audioProcessor.getAPVTS().getParameterAsValue("air").setValue(0.89);
    } else {
        audioProcessor.getAPVTS().getParameterAsValue("presence").setValue(0.89);
        audioProcessor.getAPVTS().getParameterAsValue("air").setValue(0.44);
    }
}

void ShineAudioProcessorEditor::updateTabStates() {
    tabVocalClarity.setToggleState  (selectedTab == 0, juce::dontSendNotification);
    tabAcousticDetail.setToggleState(selectedTab == 1, juce::dontSendNotification);
    tabVocalClarity.repaint();
    tabAcousticDetail.repaint();
}

void ShineAudioProcessorEditor::timerCallback() {
    inputLevel     = audioProcessor.getInputLevel();
    outputLevel    = audioProcessor.getOutputLevel();
    adaptPresScale = audioProcessor.getAdaptPresenceScale();
    adaptAirScale  = audioProcessor.getAdaptAirScale();
    repaint();
}

void ShineAudioProcessorEditor::paint(juce::Graphics& g) {
    const int W = getWidth();
    const int H = getHeight();

    g.fillAll(bgDeep);

    juce::ColourGradient glow(accentGold.withAlpha(0.03f),
                               W * 0.5f, H * 0.38f,
                               juce::Colours::transparentBlack,
                               W * 0.5f, H * 0.38f + 180.0f, true);
    g.setGradientFill(glow);
    g.fillRect(getLocalBounds());

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

    const float dotX = W * 0.5f - 88.0f;
    const float dotY = L::adaptLabelY + L::adaptLabelH * 0.5f - 2.5f;
    g.setColour(accentGold);
    g.fillEllipse(dotX, dotY, 5.0f, 5.0f);

    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(9.0f));
    g.drawText("ADAPTIVE PROCESSING",
               static_cast<int>(dotX) + 9, L::adaptLabelY,
               180, L::adaptLabelH, juce::Justification::centredLeft);

    g.setColour(bgCard.withAlpha(0.8f));
    g.fillRect(0, L::sep1Y, W, 1);
    g.fillRect(0, L::sep2Y, W, 1);

    g.setColour(textMuted);
    g.setFont(lookAndFeel.getUIFont(8.5f));
    g.drawText("ADAPTATION", 0, L::adaptBarsY + 2, W, 14, juce::Justification::centred);

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

    const int bh[6]   = { 16, 22, 28, 36, 45, 56 };
    const int bw      = 4;
    const int bgap    = 3;
    const int meterW  = 6 * bw + 5 * bgap;
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

    const int inMeterX  = W / 2 - 75 - meterW / 2;
    const int outMeterX = W / 2 + 75 - meterW / 2;

    drawMeter(inMeterX,  inputLevel,  "INPUT");
    drawMeter(outMeterX, outputLevel, "OUTPUT");

    g.setColour(juce::Colour(0xffc9a866).withAlpha(0.2f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 12.0f, 1.0f);
}

void ShineAudioProcessorEditor::resized() {
    const int W = getWidth();

    bypassButton.setBounds(W - 46, 11, 28, 28);

    const int tabRowW   = L::tabW * 2 + L::tabGap;
    const int tabStartX = (W - tabRowW) / 2;
    tabVocalClarity.setBounds  (tabStartX,                      L::tabY, L::tabW, L::tabH);
    tabAcousticDetail.setBounds(tabStartX + L::tabW + L::tabGap, L::tabY, L::tabW, L::tabH);

    const int twoKnobsW  = L::knobW * 2 + L::knobGap;
    const int knobStartX = (W - twoKnobsW) / 2;
    presenceKnob.setBounds(knobStartX,                        L::knobAreaY, L::knobW, L::knobAreaH);
    airKnob.setBounds     (knobStartX + L::knobW + L::knobGap, L::knobAreaY, L::knobW, L::knobAreaH);
}
