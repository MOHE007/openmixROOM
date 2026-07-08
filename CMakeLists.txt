cmake_minimum_required(VERSION 3.22)
project(OpenMixRoom VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------
# JUCE — expected as a git submodule under juce/
# Clone manually before first build:
#   git submodule add https://github.com/juce-framework/JUCE.git juce
#   cd juce && git checkout 8.0.x
# -------------------------------------------------------------------
set(JUCE_VST2_SUPPORTED OFF CACHE BOOL "Disable VST2")
add_subdirectory(juce)

# -------------------------------------------------------------------
# macOS deployment target
# -------------------------------------------------------------------
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

# -------------------------------------------------------------------
# JUCE plugin target
# -------------------------------------------------------------------
juce_add_plugin("OpenMixRoom"
    # Plugin metadata
    COMPANY_NAME           "OpenMixAudio"
    PRODUCT_NAME           "OpenMix Room"
    BUNDLE_ID              "com.openmixaudio.openmixroom"
    FORMATS                VST3 AU
    PLUGIN_MANUFACTURER_CODE  OpmA
    PLUGIN_CODE            OmRm

    # Keep VST2 compatibility off
    JUCE_VST3_CAN_REPLACE_VST2 OFF
)

# Add source files separately (JUCE 8 requires target_sources, not SOURCES in juce_add_plugin)
target_sources("OpenMixRoom" PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PluginProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PluginEditor.cpp"
)
target_compile_definitions("OpenMixRoom" PRIVATE
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)
