cmake_minimum_required(VERSION 3.22)
project(jdrummer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emit compile_commands.json for editors / clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy built plugins to the per-user plugin directories after each build.
# Set to OFF when producing system packages.
option(COPY_PLUGIN_AFTER_BUILD "Copy JUCE Plugins after build" ON)

# When ON, skip the per-user/build-tree asset copy steps and instead
# install assets into the system prefix (e.g. /usr/share/jdrummer) so
# the build can be packaged for distribution. Linux-only.
option(LINUX_INSTALL_SYSTEM "Install assets to system prefix on Linux" OFF)

# Fetch JUCE via CPM (vendored cmake/CPM.cmake, upstream cpm-cmake v0.42.0)
include(cmake/CPM.cmake)
CPMAddPackage("gh:juce-framework/JUCE#8.0.11")
CPMAddPackage(
    NAME              clap-juce-extensions
    GITHUB_REPOSITORY free-audio/clap-juce-extensions
    GIT_TAG           0.26.0
    GIT_SUBMODULES_RECURSE TRUE
)

# Audio plugin formats. LV2 is Linux-only because no AU/Windows hosts
# cover it; CLAP, VST3, AU, and Standalone are handled per-OS by JUCE.
set(AUDIO_FORMATS AU VST3 CLAP Standalone)
if(UNIX AND NOT APPLE)
    list(APPEND AUDIO_FORMATS LV2)
endif()

# Define the plugin/standalone target
juce_add_plugin(jdrummer
    COMPANY_NAME "Justin Ehrlichman"
    BUNDLE_ID "com.justinehrlichman.jdrummer"
    PLUGIN_MANUFACTURER_CODE JEhr
    PLUGIN_CODE Jdrm
    FORMATS ${AUDIO_FORMATS}
    PRODUCT_NAME "jdrummer"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN_AFTER_BUILD}
    LV2URI https://github.com/jmantra/jdrummer
)

# CLAP wrapper (cross-platform)
clap_juce_extensions_plugin(TARGET jdrummer
    CLAP_ID "com.justinehrlichman.jdrummer"
    CLAP_FEATURES instrument "drum-machine")

# Source files
target_sources(jdrummer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/SoundFontManager.cpp
        Source/GrooveManager.cpp
        Source/GroovePatternDensity.cpp
        Source/AudioAnalyzer.cpp
        Source/Components/KitSelector.cpp
        Source/Components/DrumPad.cpp
        Source/Components/DrumPadGrid.cpp
        Source/Components/PadControls.cpp
        Source/Components/GrooveBrowser.cpp
        Source/Components/GrooveComposer.cpp
        Source/Components/GrooveInstrumentStrip.cpp
        Source/Components/GroovesPanel.cpp
        Source/Components/BandmatePanel.cpp
        libs/minibpm/src/MiniBpm.cpp
)

# Include directories
target_include_directories(jdrummer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/TinySoundFont
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/minibpm/src
        ${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_jdrummer/JuceLibraryCode
)

# Compile definitions
target_compile_definitions(jdrummer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        # Enable audio format support
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_USE_FLAC=1
        JUCE_USE_OGGVORBIS=1
)

# Link JUCE modules
target_link_libraries(jdrummer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(NOT LINUX_INSTALL_SYSTEM)
    # Copy soundfonts to build directory for standalone
    add_custom_command(TARGET jdrummer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/soundfonts
            $<TARGET_FILE_DIR:jdrummer>/soundfonts
        COMMENT "Copying soundfonts to standalone build directory"
    )

    # Copy Grooves to build directory for standalone
    add_custom_command(TARGET jdrummer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Grooves
            $<TARGET_FILE_DIR:jdrummer>/Grooves
        COMMENT "Copying Grooves to standalone build directory"
    )

    # Copy soundfonts to VST3 bundle Resources folder (after build)
    add_custom_command(TARGET jdrummer_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/VST3/jdrummer.vst3/Contents/Resources/soundfonts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/soundfonts"
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/VST3/jdrummer.vst3/Contents/Resources/soundfonts"
        COMMENT "Copying soundfonts to VST3 bundle"
    )

    # Copy Grooves to VST3 bundle Resources folder (after build)
    add_custom_command(TARGET jdrummer_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/VST3/jdrummer.vst3/Contents/Resources/Grooves"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Grooves"
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/VST3/jdrummer.vst3/Contents/Resources/Grooves"
        COMMENT "Copying Grooves to VST3 bundle"
    )
endif()

if(APPLE)
    # Copy soundfonts to AU bundle Resources folder (after build)
    add_custom_command(TARGET jdrummer_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/AU/jdrummer.component/Contents/Resources/soundfonts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/soundfonts"
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/AU/jdrummer.component/Contents/Resources/soundfonts"
        COMMENT "Copying soundfonts to AU bundle"
    )

    # Copy Grooves to AU bundle Resources folder (after build)
    add_custom_command(TARGET jdrummer_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/AU/jdrummer.component/Contents/Resources/Grooves"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Grooves"
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/AU/jdrummer.component/Contents/Resources/Grooves"
        COMMENT "Copying Grooves to AU bundle"
    )

    add_custom_command(TARGET jdrummer_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_BINARY_DIR}/jdrummer_artefacts/$<CONFIG>/AU/jdrummer.component"
            "$ENV{HOME}/Library/Audio/Plug-Ins/Components/jdrummer.component"
        COMMENT "Re-installing AU bundle with embedded assets"
    )
endif()

if(NOT LINUX_INSTALL_SYSTEM)
    # Installation targets for distribution
    # Determine platform-specific install location
    if(APPLE)
        set(SOUNDFONTS_INSTALL_DIR "$ENV{HOME}/Library/Application Support/jdrummer/soundfonts")
        set(GROOVES_INSTALL_DIR "$ENV{HOME}/Library/Application Support/jdrummer/Grooves")
    elseif(WIN32)
        set(SOUNDFONTS_INSTALL_DIR "$ENV{APPDATA}/jdrummer/soundfonts")
        set(GROOVES_INSTALL_DIR "$ENV{APPDATA}/jdrummer/Grooves")
    else()
        set(SOUNDFONTS_INSTALL_DIR "$ENV{HOME}/.local/share/jdrummer/soundfonts")
        set(GROOVES_INSTALL_DIR "$ENV{HOME}/.local/share/jdrummer/Grooves")
    endif()

    # Custom target to install soundfonts to user directory
    add_custom_target(install_soundfonts
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SOUNDFONTS_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/soundfonts
            "${SOUNDFONTS_INSTALL_DIR}"
        COMMENT "Installing soundfonts to ${SOUNDFONTS_INSTALL_DIR}"
    )

    # Custom target to install grooves to user directory
    add_custom_target(install_grooves
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GROOVES_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/Grooves
            "${GROOVES_INSTALL_DIR}"
        COMMENT "Installing Grooves to ${GROOVES_INSTALL_DIR}"
    )

    # Combined install target
    add_custom_target(install_assets DEPENDS install_soundfonts install_grooves)

    message(STATUS "Soundfonts will be installed to: ${SOUNDFONTS_INSTALL_DIR}")
    message(STATUS "Grooves will be installed to: ${GROOVES_INSTALL_DIR}")
endif()

# Linux system-wide install targets (used by distro packaging).
# Configure with -DLINUX_INSTALL_SYSTEM=ON and then run
# `cmake --install build --prefix /usr` (or DESTDIR-based packaging).
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)

    if(TARGET ${PROJECT_NAME}_Standalone)
        install(TARGETS ${PROJECT_NAME}_Standalone
                DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

    if(TARGET ${PROJECT_NAME}_VST3)
        get_target_property(vst3_output ${PROJECT_NAME}_VST3 JUCE_PLUGIN_ARTEFACT_FILE)
        install(DIRECTORY ${vst3_output}
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/vst3)
    endif()

    if(TARGET ${PROJECT_NAME}_LV2)
        get_target_property(lv2_output ${PROJECT_NAME}_LV2 JUCE_PLUGIN_ARTEFACT_FILE)
        install(DIRECTORY ${lv2_output}
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/lv2)
    endif()

    if(TARGET ${PROJECT_NAME}_CLAP)
        install(TARGETS ${PROJECT_NAME}_CLAP
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/clap)
    endif()

    if(LINUX_INSTALL_SYSTEM)
        message(STATUS "Installing assets to ${CMAKE_INSTALL_FULL_DATADIR}/jdrummer")
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/soundfonts
                DESTINATION ${CMAKE_INSTALL_DATADIR}/jdrummer)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Grooves
                DESTINATION ${CMAKE_INSTALL_DATADIR}/jdrummer)
    endif()
endif()

