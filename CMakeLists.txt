cmake_minimum_required(VERSION 3.22)
project(HomeKeys VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE : utilise un dossier local (-DJUCE_DIR=...) ou le telecharge automatiquement
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(HomeKeys
    COMPANY_NAME "HomeKey Audio"
    PRODUCT_NAME "HomeKey I"
    PLUGIN_MANUFACTURER_CODE Hmky
    PLUGIN_CODE Hkp1
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Piano
    FORMATS VST3 Standalone
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(HomeKeys)

# Pack de presets integre au plugin (installe automatiquement au premier lancement)
juce_add_binary_data(HomeKeysData SOURCES Resources/HomeKeyPresets.zip Resources/Samples.zip)

target_sources(HomeKeys PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PianoVoice.cpp
    Source/PresetManager.cpp
    Source/SampleBank.cpp
    Source/SoundDesigner.cpp)

target_compile_definitions(HomeKeys PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(HomeKeys
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        HomeKeysData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
