cmake_minimum_required(VERSION 3.22)
project(SaelinShine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE (git submodule)
add_subdirectory(JUCE)

# Embedded font resources
juce_add_binary_data(SaelinShine_BinaryData SOURCES
    Resources/fonts/CormorantGaramond-Medium.ttf
    Resources/fonts/DMSans-Regular.ttf
)

# Plugin target
juce_add_plugin(SaelinShine
    COMPANY_NAME "Saelin Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Sael
    PLUGIN_CODE Sshn
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Saelin Shine"
)

target_sources(SaelinShine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/ShineParams.h
    Source/DSP/BiquadFilter.h
    Source/DSP/ShineChain.h
    Source/ML/ShineFeatureExtractor.h
    Source/ML/ShineModelInference.h
    Source/ML/ShineModelWeights.h
    Source/UI/ShineLookAndFeel.h
    Source/UI/ShineLookAndFeel.cpp
    Source/UI/ShineKnob.h
    Source/UI/ShineKnob.cpp
)

target_include_directories(SaelinShine PRIVATE
    Source
    Source/DSP
    Source/ML
    Source/UI
)

target_compile_definitions(SaelinShine PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(SaelinShine
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        SaelinShine_BinaryData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
