# 
#     File: CMakeLists.txt
#     Project: MiniSynth
#     Revision: 1.0.0
#     Date: 2025-08-19
#     Description:
#         CMake build configuration for MiniSynth. Declares plugin targets,
#         embeds factory presets as binary data, and links JUCE modules.


cmake_minimum_required(VERSION 3.22)
project(MiniSynth VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  # For universal: set to "arm64;x86_64"
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
endif()

add_subdirectory(JUCE)

juce_add_plugin(MiniSynth
    COMPANY_NAME "Chrno"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    PLUGIN_CODE MnSt
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "MiniSynth")

# ---- Factory presets embedded (glob all json) ----
file(GLOB_RECURSE MS_PRESET_FILES
     "${CMAKE_SOURCE_DIR}/Resources/Presets/Factory/*.minisynth.json")

if (NOT MS_PRESET_FILES)
  message(WARNING "No factory presets found in Resources/Presets/Factory (BinaryData will be empty).")
endif()

juce_add_binary_data(MiniSynthData
    SOURCES
        ${MS_PRESET_FILES}
)

juce_add_binary_data(MiniSynthAssets
    SOURCES
        ${CMAKE_SOURCE_DIR}/Resources/im/logo.png
)
target_link_libraries(MiniSynth PRIVATE MiniSynthAssets)

set(MS_SRC
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/dsp/SynthVoice.cpp
    Source/dsp/SynthVoice.h
    Source/dsp/PolyBLEPOsc.h
    Source/dsp/PulseOsc.h
    Source/dsp/Noise.h
    Source/presets/PresetManager.cpp
    Source/presets/PresetManager.h
    Source/JuceIncludes.h)



target_sources(MiniSynth PRIVATE ${MS_SRC})


target_include_directories(MiniSynth PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/dsp
    ${CMAKE_SOURCE_DIR}/Source/presets
)

# JUCE options
target_compile_definitions(MiniSynth PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Link
target_link_libraries(MiniSynth PRIVATE
    MiniSynthData
    juce::juce_audio_utils
    juce::juce_dsp)
