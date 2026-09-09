cmake_minimum_required(VERSION 3.22)
project(SalekHightech VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.8
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SalekHightech
    COMPANY_NAME "SALEK"
    BUNDLE_ID "com.salek.hightech"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE SLKH
    PLUGIN_CODE SHTC
    FORMATS VST3 Standalone
    PRODUCT_NAME "SALEK HIGHTECH"
)

target_sources(SalekHightech PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(SalekHightech PRIVATE cxx_std_20)
target_compile_definitions(SalekHightech PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(SalekHightech PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)
