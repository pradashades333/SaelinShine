# SaelinShine

SaelinShine is an adaptive clarity and finishing plugin built with C++ and JUCE. It is designed as a lightweight VST3/AU audio effect for adding controlled shine, polish, and presence to a mix.

## What It Does

- Adds adaptive high-end clarity without harsh static EQ moves.
- Keeps the audio-plugin codebase focused around JUCE modules, CMake builds, and native plugin targets.
- Includes resources and extraction helpers used by the plugin build.

## Stack

- C++17
- JUCE
- CMake
- VST3 / AU plugin targets

## Live Link

No public web demo yet. Build locally as a desktop audio plugin.

## Screenshot

![Plugin asset](JUCE/examples/Assets/tile_background.png)

## How To Run

```bash
git clone https://github.com/pradashades333/SaelinShine.git
cd SaelinShine
git submodule update --init --recursive
cmake -S . -B build
cmake --build build --config Release
```

After building, load the generated plugin from the build output in a supported DAW.