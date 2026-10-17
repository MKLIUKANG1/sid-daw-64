<p align="center"> <img src="icon.png" alt="SID DAW 64" width="128" height="128"> </p><h1 align="center">SID DAW 64</h1><p align="center"> <b>A DAW-style editor for the Commodore 64 SID chip</b> </p><p align="center"> <a href="https://github.com/MKLIUKANG1/sid-daw-64/releases/latest"> <img src="https://img.shields.io/github/v/release/MKLIUKANG1/sid-daw-64?style=flat-square&color=blue" alt="Latest release"> </a> <a href="https://github.com/MKLIUKANG1/sid-daw-64/blob/main/LICENSE"> <img src="https://img.shields.io/github/license/MKLIUKANG1/sid-daw-64?style=flat-square&color=green" alt="License"> </a> <img src="https://img.shields.io/badge/platform-Windows%20(x64)-lightgrey?style=flat-square" alt="Platform"> <img src="https://img.shields.io/badge/Qt-6-brightgreen?style=flat-square" alt="Qt6"> </p>
Download
Pre-built Windows 64-bit binaries are available on the Releases page.

Unpack the ZIP and run SidDAW.exe. No installation required.

Features
Instruments
Named instruments independent from patterns

Assign instrument to a note by dragging from the instrument dock

F key or middle-click on notes opens Note Properties dialog

Auto-created on MIDI import: Ch1 Lead, Ch2 Bass, Ch10 Drums, Ch3 Harmony

Envelope editor (FamiStudio-style)
Four hand-drawn envelopes per instrument, all active simultaneously:

Envelope	Range	Description
Volume	0..15	Note volume
Pitch	0..15	Bipolar semitone shift (center 8 = no shift)
Duty	0..15	Bipolar pulse width (center 8 = 50%)
Arp	0..15	Semitone offset for arpeggios
Adjustable speed (1..16 points per tick), per-track Clear button.

Composition
Arrangement / Playlist — place clips on tracks, resize, drag, delete

Piano Roll — draw, drag, resize, rubber-band select notes

Note preview inside clips (color = instrument)

Undo / Redo for every edit

Per-voice mixer (volume, pan) + master volume

Editing shortcuts
Action	Key
Select all notes	Ctrl+A
Rubber-band select	Shift + drag
Delete selection	Delete
Move (semitone / tick)	Arrow keys
Move (octave / quarter)	Shift + arrows
Note properties	F
Pan view	Middle mouse button
Zoom	Ctrl + mouse wheel
Undo / Redo	Ctrl+Z / Ctrl+Y
Localization
English, Russian, Chinese, Portuguese (BR)

Selected in Language menu, saved via QSettings

MIDI
Import .mid with channel selection dialog

Assign each MIDI channel to a SID voice

Choose single pattern or separate pattern per channel

Drum channel (Ch 10) auto-uses Noise waveform

Live recording from MIDI keyboard via RtMidi

Projects and Export
Save / load projects as .siddaw (JSON)

Voice presets as .json

Export WAV — offline render via reSIDfp

Export SID — PSID file with a 6502 player assembled by 64tass

Building on Windows (MSYS2 MINGW64)
Install dependencies
text
pacman -S --needed base-devel mingw-w64-x86_64-toolchain ^
    mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja ^
    mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-multimedia ^
    mingw-w64-x86_64-rtmidi mingw-w64-x86_64-libresidfp

pacman -S --needed mingw-w64-ucrt-x86_64-64tass
ln -sf /ucrt64/bin/64tass.exe /mingw64/bin/64tass.exe
Build
text
mkdir -p build && cd build
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/mingw64
cmake --build .
Run
text
cd build
windeployqt --no-translations SidDAW.exe
./SidDAW.exe
If the app complains about missing DLLs, copy them recursively with ldd (see deploy_all.sh in previous releases, or use windeployqt and manually copy libresidfp-0.dll, librtmidi-7.dll, libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll).

Architecture
Module	Purpose
src/Model.h	Data model (Note, VoiceParams, Instrument, Pattern, Clip, Project)
src/SidEngine	reSIDfp wrapper
src/ArrangementPlayer	Real-time playback across clips
src/PatternPlayer	Offline render for WAV export
src/SidAudioDevice	Bridges player to QAudioSink
src/MainWindow	Main window, menus, wiring
src/PianoRollWidget	Piano roll editor
src/ArrangementWidget	Timeline with clips and preview
src/VoicePanelWidget	Voice params UI (Sliders + 4 envelope tabs)
src/EnvelopeEditorWidget	Drawable envelope editor
src/InstrumentListWidget	Drag source for instrument assignment
src/InstrumentEditorDialog	Instrument editing dialog
src/NotePropertiesDialog	Per-note editing
src/Commands.h	QUndoCommand subclasses for all edits
src/MidiImporter, src/MidiImportDialog	MIDI parsing + channel selection
src/MidiRecorder	Live MIDI capture
src/ProjectIO	Project save/load (.siddaw)
src/VoicePresetIO	Voice preset save/load
src/WavExporter	Offline WAV render
src/SidExporter	PSID writer
src/LanguageManager	Localization (EN/RU/ZH/PT-BR)
src/AboutDialog	About window
player.asm	6502 SID player routine (assembled by 64tass)
icon.rc	Windows .exe icon resource
resources.qrc	Qt resource bundle (icon)
License
MIT — see LICENSE.

Author
MKLIUKANG1 — github.com/MKLIUKANG1

<p align="center"> <i>Built with love for the Commodore 64 scene</i> </p>
