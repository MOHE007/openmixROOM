cmake_minimum_required(VERSION 3.22)
project(OpenMixRoom VERSION 0.4.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------
# JUCE — expected as a git submodule under juce/
# -------------------------------------------------------------------
set(JUCE_VST2_SUPPORTED OFF CACHE BOOL "Disable VST2")
add_subdirectory(juce)

# -------------------------------------------------------------------
# macOS deployment target
# -------------------------------------------------------------------
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")

# -------------------------------------------------------------------
# libmysofa — SOFA/HRTF parser (static library, no tests)
# -------------------------------------------------------------------
set(BUILD_TESTS OFF CACHE BOOL "Skip libmysofa tests")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libmysofa as static")
add_subdirectory(third_party/libmysofa-source)

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

    # Additional JUCE modules
    NEEDS_JUCE_DSP ON
)

# Add source files separately (JUCE 8 requires target_sources, not SOURCES in juce_add_plugin)
target_sources("OpenMixRoom" PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PluginProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/PluginEditor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/SofaLoader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/CrossfeedProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/RoomProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/HeadphoneCalibration.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/FrequencyResponseGraph.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/RoomResponseGraph.cpp"
)
target_compile_definitions("OpenMixRoom" PRIVATE
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

# Link libmysofa
target_include_directories("OpenMixRoom" PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libmysofa-source/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libmysofa-source/src/hrtf"
    "${CMAKE_CURRENT_BINARY_DIR}/third_party/libmysofa-source/src"
)
target_link_libraries("OpenMixRoom" PRIVATE juce::juce_dsp mysofa-static z)

# Pass SOFA file path as compile definition for development builds
target_compile_definitions("OpenMixRoom" PRIVATE
    SOFA_BUNDLE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/share/MIT_KEMAR_normal_pinna.sofa"
)
