
cmake_minimum_required(VERSION 3.22)

project(OfforSampleForge
    VERSION 1.0.0
    DESCRIPTION "OFFOR AUDIO sample design and batch processing workstation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE source directory
set(JUCE_SOURCE_DIR "" CACHE PATH "Path to JUCE source directory")

if(NOT JUCE_SOURCE_DIR)
    message(FATAL_ERROR
        "JUCE_SOURCE_DIR is not set. Configure with -DJUCE_SOURCE_DIR=\"C:/path/to/JUCE\"")
endif()

if(NOT EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE CMakeLists.txt not found at: ${JUCE_SOURCE_DIR}")
endif()

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Load JUCE from the supplied path
add_subdirectory(
    "${JUCE_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/JUCE"
)

juce_add_gui_app(OfforSampleForge
    PRODUCT_NAME "OFFOR SAMPLE FORGE"
    COMPANY_NAME "OFFOR AUDIO"
    COMPANY_WEBSITE "https://chechris.com"
    BUNDLE_ID "com.offoraudio.sampleforge"
    VERSION "${PROJECT_VERSION}"
)

juce_generate_juce_header(OfforSampleForge)

target_sources(OfforSampleForge
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
)

target_compile_features(OfforSampleForge PRIVATE cxx_std_17)

target_compile_definitions(OfforSampleForge
    PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(OfforSampleForge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    PREFIX "Source"
    FILES
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
)

add_custom_command(TARGET OfforSampleForge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
        "$<TARGET_FILE_DIR:OfforSampleForge>/Resources"
)