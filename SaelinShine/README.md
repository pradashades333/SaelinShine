# Saelin Shine v2 — Source

Saelin Shine is an adaptive clarity plugin built on the Saelin Adaptive Engine (SAE). It applies presence and air EQ with intelligent level-sensing modulation, replacing the ML model from v1 with a fast DSP-only adaptive engine.

## Requirements

- CMake 3.22+
- JUCE **8.0.12**
- Xcode 15+ (macOS) or Visual Studio 2022 (Windows)
- C++17

## Setup

1. Clone JUCE 8.0.12 into the `JUCE/` subdirectory:
   ```bash
   git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
   ```

2. Configure and build (Xcode):
   ```bash
   cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release --target SaelinShine_AU
   cmake --build build --config Release --target SaelinShine_VST3
   ```

   Or Visual Studio (Windows):
   ```bash
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release --target SaelinShine_VST3
   ```

3. With `COPY_PLUGIN_AFTER_BUILD TRUE` (default), the AU and VST3 are installed automatically to the system plugin folders after a successful build.

## Structure

```
SaelinShine/
  CMakeLists.txt
  JUCE/                        ← clone here (not included)
  Resources/
    fonts/
      CormorantGaramond-Medium.ttf
      DMSans-Regular.ttf
  Source/
    PluginProcessor.h/cpp      ← APVTS, SAE integration, processBlock
    PluginEditor.h/cpp         ← UI layout, knob wiring, tab presets
    DSP/
      ShineParams.h            ← knob mappings, vocabulary words
      BiquadFilter.h           ← HPF, LPF, LowShelf, HighShelf, PresenceBand
      ShineChain.h             ← DSP chain: HPF → presence EQ → air EQ/shelf → peak limiter
      SaelinAdaptiveEngine.h   ← SAE: level-sensing, intent modulation
      PeakLimiter.h            ← output peak limiter (~0.1ms attack, 50ms release)
    UI/
      ShineLookAndFeel.h/cpp   ← custom rotary slider, button, font rendering
      ShineKnob.h/cpp          ← vocabulary-word knob display
```

## Notes

- No `Info.plist` is included — JUCE's `juce_add_plugin()` generates it at build time. After building you will find it at `build/.../AU/Saelin Shine.component/Contents/Info.plist`.
- The JUCE library itself is not bundled. Clone tag `8.0.12` from the official JUCE repo. No patches to JUCE are needed.
- The v1 `ML/` folder (PyTorch model) is not present in v2. The SAE replaces it entirely.
