cmake_minimum_required(VERSION 3.21)
project(SidDAW VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
find_package(rtmidi REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RESIDFP REQUIRED IMPORTED_TARGET libresidfp)

find_program(TASS_EXECUTABLE 64tass)
if(TASS_EXECUTABLE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/player.bin
        COMMAND ${TASS_EXECUTABLE} --nostart
                -o ${CMAKE_CURRENT_BINARY_DIR}/player.bin
                ${CMAKE_CURRENT_SOURCE_DIR}/player.asm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/player.asm
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Assembling player.bin with 64tass"
    )
    add_custom_target(player_bin ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/player.bin)
endif()

add_executable(SidDAW
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/PianoRollWidget.cpp
    src/PianoRollWidget.h
    src/ArrangementWidget.cpp
    src/ArrangementWidget.h
    src/VoicePanelWidget.cpp
    src/VoicePanelWidget.h
    src/EnvelopeEditorWidget.cpp
    src/EnvelopeEditorWidget.h
    src/VoicePresetIO.cpp
    src/VoicePresetIO.h
    src/WavExporter.cpp
    src/WavExporter.h
    src/MidiPresetManager.cpp
    src/MidiPresetManager.h
    src/MidiImporter.cpp
    src/MidiImporter.h
    src/MidiImportDialog.cpp
    src/MidiImportDialog.h
    src/MidiRecorder.cpp
    src/MidiRecorder.h
    src/ProjectIO.cpp
    src/ProjectIO.h
    src/Cpu6502.cpp
    src/Cpu6502.h
    src/RawSidStream.cpp
    src/RawSidStream.h
    src/RawSidDevice.cpp
    src/RawSidDevice.h
    src/SidImporter.cpp
    src/SidImporter.h
    src/SidExporter.cpp
    src/SidExporter.h
    src/WaveformPreview.cpp
    src/WaveformPreview.h
    src/EnvelopePresets.cpp
    src/EnvelopePresets.h
    src/InstrumentPresets.cpp
    src/InstrumentPresets.h
    src/InstrumentEditorPanel.cpp
    src/InstrumentEditorPanel.h
    src/PreviewPlayer.cpp
    src/PreviewPlayer.h
    src/SidEngine.cpp
    src/SidEngine.h
    src/PatternPlayer.cpp
    src/PatternPlayer.h
    src/ArrangementPlayer.cpp
    src/ArrangementPlayer.h
    src/SidAudioDevice.cpp
    src/SidAudioDevice.h
    src/InstrumentEditorDialog.cpp
    src/InstrumentEditorDialog.h
    src/NotePropertiesDialog.cpp
    src/NotePropertiesDialog.h
    src/InstrumentListWidget.cpp
    src/InstrumentListWidget.h
    src/ThemeManager.cpp
    src/ThemeManager.h
    src/LanguageManager.cpp
    src/LanguageManager.h
    src/DrumPresets.cpp
    src/DrumPresets.h
    src/DrumEditorWidget.cpp
    src/DrumEditorWidget.h
    src/DrumEditorDialog.cpp
    src/DrumEditorDialog.h
    src/AboutDialog.cpp
    src/AboutDialog.h
    src/Model.h
    src/version.h
    icon.rc
    resources.qrc
)

if(TASS_EXECUTABLE)
    add_dependencies(SidDAW player_bin)
endif()

if (WIN32)
    target_include_directories(SidDAW PRIVATE /mingw64/include)
    target_link_options(SidDAW PRIVATE -Wl,--subsystem,windows)
endif()

target_compile_definitions(SidDAW PRIVATE
    SIDDAW_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(SidDAW PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    RtMidi::rtmidi
    PkgConfig::RESIDFP
)
