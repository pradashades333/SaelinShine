# Saelin Shine v2 — Source Delivery

This is the complete buildable source for Saelin Shine v2 (SAE-driven, ML model removed).

## What's in this package

```
SaelinShine-Source/
├── CMakeLists.txt          ← build configuration (equivalent of an Xcode project file)
├── Source/
│   ├── PluginProcessor.h/cpp
│   ├── PluginEditor.h/cpp
│   ├── DSP/
│   │   ├── ShineParams.h           knob mapping, presets, vocabulary words
│   │   ├── BiquadFilter.h          HPF / peaking EQ / shelf filters
│   │   ├── ShineChain.h            full signal chain (presence + air + limiter)
│   │   ├── SaelinAdaptiveEngine.h  SAE module (level envelope, inverted modulation)
│   │   └── PeakLimiter.h           output ceiling limiter
│   └── UI/
│       ├── ShineLookAndFeel.h/cpp  knob rendering, colours, fonts
│       └── ShineKnob.h/cpp         knob component + vocabulary word display
└── Resources/
    └── fonts/               DM Sans, Cormorant Garamond (embedded as binary data)
```

## What's NOT included, and why

**JUCE itself is not bundled.** This project builds against vanilla JUCE via CMake
(`add_subdirectory(JUCE)` in CMakeLists.txt) — there's no Saelin-specific fork or
patches. Bundling it would add ~70MB of files you can get directly from the source:

```
git clone --branch 7.0.9 https://github.com/juce-framework/JUCE.git
```

Place (or symlink) the cloned `JUCE` folder inside `SaelinShine-Source/` so the
directory layout is:

```
SaelinShine-Source/
├── CMakeLists.txt
├── JUCE/          ← cloned here
├── Source/
└── Resources/
```

**There's no static `Info.plist` file.** This project doesn't use an Xcode project —
it's CMake-based, and JUCE's `juce_add_plugin()` call in `CMakeLists.txt` (company
name, plugin codes, bundle ID, formats, etc.) generates the Info.plist automatically
at build time for the AU target. If you need to inspect it, it'll be written to
`build/SaelinShine_artefacts/Release/AU/Saelin Shine.component/Contents/Info.plist`
after building.

## Building

```bash
cd SaelinShine-Source
cmake -B build -G Xcode          # or "Unix Makefiles" for command-line builds
cmake --build build --config Release
```

This produces VST3, AU, and Standalone targets (see `FORMATS AU VST3 Standalone`
in CMakeLists.txt). Using the Xcode generator (`-G Xcode`) opens the same project
in Xcode if you prefer working there — CMake just generates the .xcodeproj for you
rather than us hand-maintaining one.

## Version notes

- JUCE version: **7.0.9** (pinned — newer JUCE/Xcode SDK combos break a macOS API
  this version still calls; see CI notes if upgrading)
- This is v2: the ML model (`shine.pt`) and inference code from v1 have been fully
  removed and replaced by the Saelin Adaptive Engine (`SaelinAdaptiveEngine.h`)
