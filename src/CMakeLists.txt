cmake_minimum_required(VERSION 3.22)

project(DigitalFilterVisualizer VERSION 0.0.1)

set(CMAKE_BUILD_TYPE Debug)

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
add_subdirectory(JUCE)

# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(...)

juce_add_plugin(
  ${PROJECT_NAME}
  IS_SYNTH                    FALSE
  FORMATS                     VST3 Standalone
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD     TRUE
  PLUGIN_MANUFACTURER_CODE    APWG
  PLUGIN_CODE                 Dfve
  COMPANY_NAME                AudioProjectWorkgroup
  PRODUCT_NAME                DigitalFilterVisualizer
)

# juce_generate_juce_header(AudioPluginExample)

target_sources(
  ${PROJECT_NAME}
  PRIVATE
  PluginEditor.cpp
  PluginProcessor.cpp
)

file(GLOB CPPSRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
file(GLOB HSRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
set(ALLSRCS "${CPPSRCS};${HSRCS}")
add_custom_target(ALLFILES SOURCES ${ALLSRCS})

target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_add_binary_data(
  ${PROJECT_NAME}_BinaryData
  SOURCES
    "Assets/Save.png"
    "Assets/Undo.png"
    "Assets/Redo.png"
    "Assets/Load.png"
    "Assets/Play.png"
    "Assets/Pause.png"
    "Assets/Stop.png"
    "Assets/Loop.png"
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  ${PROJECT_NAME}_BinaryData
  juce::juce_audio_utils
  juce::juce_dsp
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)

function(add_console_app target)
  # like `juce_add_console_app` but it doesn't redefine
  # `JUCE_STANDALONE_APPLICATION` when linking against shared plugin code
  cmake_parse_arguments(JUCE_ARG "_NO_RESOURCERC" "" "" ${ARGN})

  add_executable(${target})

  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(${target} PRIVATE _CONSOLE=1)
  endif()

  _juce_initialise_target(${target} ${ARGN})

  if(NOT JUCE_ARG__NO_RESOURCERC)
	set_target_properties(${target} PROPERTIES JUCE_TARGET_KIND_STRING "ConsoleApp")
	_juce_write_configure_time_info(${target})
	_juce_add_resources_rc(${target} ${target})
	_juce_add_xcode_entitlements(${target} ${target})
  endif()
endfunction()

  add_executable(TestGenerator ../tests/generator/generate.cpp)
  add_custom_target(
    GenerateTests ALL
    COMMAND TestGenerator ../tests/generator/
    DEPENDS TestGenerator
  )

  add_console_app(DigitalFilterVisualizer_Tests
    PRODUCT_NAME "DigitalFilterVisualizer_Tests"
  )

  add_dependencies(DigitalFilterVisualizer_Tests GenerateTests)

  set_target_properties(DigitalFilterVisualizer_Tests PROPERTIES
    OUTPUT_NAME "DigitalFilterVisualizer_Tests"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/DigitalFilterVisualizer_Tests_artefacts/$<CONFIG>"
  )

  target_sources(DigitalFilterVisualizer_Tests
    PRIVATE
    ../tests/Main.cpp
  )

  target_link_libraries(DigitalFilterVisualizer_Tests
      PRIVATE
          ${PROJECT_NAME}
          juce::juce_core
          juce::juce_events
          juce::juce_data_structures
          juce::juce_audio_basics
          juce::juce_audio_processors
          juce::juce_audio_utils
          juce::juce_gui_basics
          juce::juce_dsp
  )
