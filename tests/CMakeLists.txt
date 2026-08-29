# ==============================================================================
# DSPRegressionTest — standalone command-line regression test target
# Links only DSP modules, no PluginProcessor/PluginEditor/JUCE GUI.
# ==============================================================================

# Create a standalone console application (not a plugin)
juce_add_console_app(DSPRegressionTest
    PRODUCT_NAME           "DSPRegressionTest"
    COMPANY_NAME           "OpenMixAudio"
    BUNDLE_ID              "com.openmixaudio.dspregressiontest"
    NEEDS_JUCE_DSP         ON
)

target_sources(DSPRegressionTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/DSPRegressionTest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/dsp/RoomProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/dsp/CrossfeedProcessor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/dsp/HeadphoneCalibration.cpp"
)

target_include_directories(DSPRegressionTest PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/dsp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libmysofa-source/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libmysofa-source/src/hrtf"
    "${CMAKE_CURRENT_BINARY_DIR}/../third_party/libmysofa-source/src"
)

target_link_libraries(DSPRegressionTest PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
    mysofa-static
    z
)

# So that juce::UnitTestRunner can find and auto-register tests
target_compile_definitions(DSPRegressionTest PRIVATE
    JUCE_MODAL_LOOPS_PERMITTED=0
    JUCE_DISABLE_LEAK_DETECTOR=1
)

# Run tests as a post-build step (optional — commented out by default)
# add_custom_command(TARGET DSPRegressionTest POST_BUILD
#     COMMAND "$<TARGET_FILE:DSPRegressionTest>"
#     COMMENT "Running DSP regression tests..."
# )
（内容由AI生成，仅供参考）
