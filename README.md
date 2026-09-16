# Saelin Shine v2

Adaptive clarity plugin built on the Saelin Adaptive Engine. It applies presence and air EQ with level-sensing DSP modulation instead of a runtime ML model.

## Stack

- C++17
- JUCE 8.0.12
- CMake
- VST3 / AU plugin targets

## Screenshot

![Saelin Shine UI asset](JUCE/examples/Assets/tile_background.png)

## What it does

- Adds presence and air with a DSP-only adaptive engine.
- Uses level-sensing modulation for cleaner vocal/instrument polish.
- Includes custom UI styling, APVTS parameters, and a peak limiter.

## Run locally

Clone JUCE into `JUCE/`, then build:

```bash
git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
cmake -B build -G Xcode -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SaelinShine_AU
cmake --build build --config Release --target SaelinShine_VST3
```

Windows:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target SaelinShine_VST3
```