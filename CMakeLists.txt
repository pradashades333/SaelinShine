cmake_minimum_required(VERSION 3.22)
project(SaelinShine VERSION 2.0.0)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE (git submodule)
set(JUCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE")
add_subdirectory("${JUCE_ROOT}" "${CMAKE_BINARY_DIR}/JUCE")

# Embedded font resources
juce_add_binary_data(SaelinShine_BinaryData SOURCES
    Resources/fonts/CormorantGaramond-Medium.ttf
    Resources/fonts/DMSans-Regular.ttf
    knob/saelin-knob-shine.svg
    knob/saelin-knob-template.svg
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
    BUNDLE_ID "com.saelin.shine"
    PLUGIN_CODE Sshn
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Saelin Shine"
)

juce_generate_juce_header(SaelinShine)

target_sources(SaelinShine PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/ShineParams.h
    Source/DSP/BiquadFilter.h
    Source/DSP/ShineChain.h
    Source/DSP/SaelinAdaptiveEngine.h
    Source/DSP/PeakLimiter.h
)

target_include_directories(SaelinShine PRIVATE
    Source
    Source/DSP
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
