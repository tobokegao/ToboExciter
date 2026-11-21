cmake_minimum_required(VERSION 3.22)
project(ToboExciter VERSION 0.1.2)

# the C runtime is linked in, so the plugin loads on a PC without the Visual C++ redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(JUCE)

juce_add_plugin(ToboExciter
    COMPANY_NAME "Tobokegao"
    COMPANY_WEBSITE "https://github.com/tobokegao/ToboExciter"
    PLUGIN_MANUFACTURER_CODE Tbkg
    PLUGIN_CODE Te02
    FORMATS VST3
    PRODUCT_NAME "Tobo Exciter"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Filter Distortion
    COPY_PLUGIN_AFTER_BUILD FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

# the pictures of the panel, rendered from a 3D model
juce_add_binary_data(ExciterAssets SOURCES Assets/base.png Assets/knobs.png Assets/lamp_on.png)

target_sources(ToboExciter PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(ToboExciter PRIVATE cxx_std_17)
target_compile_definitions(ToboExciter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(ToboExciter
    PRIVATE
        ExciterAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# draws the panel to a PNG file without a host (test/panel_shot.cpp)
juce_add_console_app(PanelShot PRODUCT_NAME "PanelShot")
target_sources(PanelShot PRIVATE test/panel_shot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(PanelShot PRIVATE cxx_std_17)
target_compile_definitions(PanelShot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(PanelShot PRIVATE ExciterAssets juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)

# every ordinary use and every misuse we could think of, without a host or a screen
juce_add_console_app(StressTest PRODUCT_NAME "StressTest")
target_sources(StressTest PRIVATE test/stress.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_features(StressTest PRIVATE cxx_std_17)
target_compile_definitions(StressTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(StressTest PRIVATE ExciterAssets juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
