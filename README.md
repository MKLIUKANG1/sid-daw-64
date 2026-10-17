# SID DAW 64

A DAW-style editor for the Commodore 64 SID chip.

## Features

- DAW UI: arrangement timeline, piano roll, per-voice panels
- 3 SID voices (lead / bass / harmony) with waveform, ADSR, PW, ring mod, sync
- Envelope editor — draw volume envelopes by hand or use ADSR sliders
- MIDI import (.mid) with 3-voice auto-assignment
- Live MIDI recording via RtMidi
- Undo/Redo for all edits
- Project save/load (.siddaw, JSON)
- Voice presets (.json)
- Export: WAV (offline via reSIDfp), SID (PSID + 6502 player assembled by 64tass)

## Building on Windows (MSYS2 MINGW64)

Install dependencies:

    pacman -S --needed base-devel mingw-w64-x86_64-toolchain \
        mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
        mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-multimedia \
        mingw-w64-x86_64-rtmidi mingw-w64-x86_64-libresidfp

    pacman -S --needed mingw-w64-ucrt-x86_64-64tass
    ln -sf /ucrt64/bin/64tass.exe /mingw64/bin/64tass.exe

Build:

    mkdir -p build && cd build
    cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/mingw64
    cmake --build .

Run:

    cd build
    windeployqt --no-translations SidDAW.exe
    ./SidDAW.exe

## Architecture

See src/ for details. Key modules:

- src/Model.h — data model (Note, VoiceParams, Pattern, Clip, Project)
- src/SidEngine — reSIDfp wrapper
- src/ArrangementPlayer — real-time timeline player
- src/PatternPlayer — offline player for WAV export
- src/MainWindow — main window and wiring
- src/PianoRollWidget / ArrangementWidget — editors
- src/VoicePanelWidget / EnvelopeEditorWidget — voice params + envelope
- src/Commands.h — QUndoCommand subclasses
- src/MidiImporter / MidiRecorder — MIDI in
- src/ProjectIO / VoicePresetIO — project/preset save-load
- src/WavExporter / SidExporter — exporters
- player.asm — 6502 SID player (assembled by 64tass)

## License

MIT (see LICENSE)
