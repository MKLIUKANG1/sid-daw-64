<p align="center">
  <img src="src/icon.png" alt="SID DAW 64" width="128" height="128">
</p>

<h1 align="center">SID DAW 64</h1>

<p align="center">
  <b>A DAW-style editor for the Commodore 64 SID chip</b>
</p>

<p align="center">
  <a href="https://github.com">
    <img src="https://shields.io" alt="Latest release">
  </a>
  <a href="https://github.com">
    <img src="https://shields.io" alt="License">
  </a>
  <img src="https://shields.io" alt="Platform">
  <img src="https://shields.io" alt="Qt6">
</p>

---

## 💾 Download

Pre-built Windows 64-bit binaries are available on the **[Releases](https://github.com)** page.

1. Unpack the downloaded ZIP archive.
2. Run `SidDAW.exe`.
3. *No installation required!*

---

## 🚀 Features

### 🎹 Instruments
* **Independent Modules:** Named instruments exist completely separate from patterns.
* **Drag-and-Drop:** Quickly assign an instrument to a note by dragging it from the instrument dock.
* **Fine-Tuning:** Press the `F` key or middle-click on notes to open the **Note Properties** dialog.
* **Smart MIDI Import:** Automatically creates standard tracks: `Ch1 Lead`, `Ch2 Bass`, `Ch10 Drums`, and `Ch3 Harmony`.

### 🎛️ Envelope Editor (FamiStudio-style)
Four hand-drawn envelopes per instrument, all active simultaneously:

| Envelope | Range | Description |
| :--- | :---: | :--- |
| **Volume** | `0..15` | Note volume |
| **Pitch** | `0..15` | Bipolar semitone shift (center 8 = no shift) |
| **Duty** | `0..15` | Bipolar pulse width (center 8 = 50%) |
| **Arp** | `0..15` | Semitone offset for arpeggios |

* Adjustable speed (`1..16` points per tick).
* Per-track **Clear** button.

### 🎼 Composition & Editing
* **Arrangement / Playlist:** Easily place clips on tracks, resize, drag, or delete them.
* **Advanced Piano Roll:** Draw, drag, resize, and rubber-band select multiple notes.
* **Visual Preview:** Note preview inside clips is color-coded by the assigned instrument.
* **Safe Workflow:** Full **Undo / Redo** support for every single edit operation.
* **Mixing:** Per-voice mixer (volume, pan) + master volume control.

---

## ⌨️ Editing Shortcuts

| Action | Key / Input |
| :--- | :--- |
| **Select all notes** | `Ctrl + A` |
| **Rubber-band select** | `Shift + Drag` |
| **Delete selection** | `Delete` |
| **Move (semitone / tick)** | `Arrow keys` |
| **Move (octave / quarter)** | `Shift + Arrows` |
| **Note properties** | `F` |
| **Pan view** | `Middle mouse button` |
| **Zoom** | `Ctrl + Mouse wheel` |
| **Undo / Redo** | `Ctrl + Z` / `Ctrl + Y` |

---

## 🌐 Localization

The interface supports the following languages:
* 🇬🇧 English
* 🇷🇺 Russian
* 🇨🇳 Chinese
* 🇧🇷 Portuguese (BR)

*Language can be selected in the **Language menu** and is automatically saved via `QSettings`.*

---

## 🔌 MIDI & Hardware Integration

* **Flexible Import:** Import `.mid` files with a dedicated channel selection dialog.
* **Voice Mapping:** Assign each MIDI channel directly to a specific SID voice.
* **Pattern Modes:** Choose between a single pattern layout or separate patterns per channel.
* **Automated Drums:** The drum channel (`Ch 10`) automatically utilizes the Noise waveform.
* **Live Recording:** Play and record in real-time from a MIDI keyboard via `RtMidi`.

---

## 📁 Projects and Export

* **Project Saves:** Save and load your projects as `.siddaw` (JSON format).
* **Presets:** Save individual voice configurations as `.json` presets.
* **WAV Export:** High-quality offline audio rendering powered by `reSIDfp`.
* **SID Export:** Generates standard `PSID` files with an native 6502 player assembled via `64tass`.

---

## 🛠️ Building on Windows (MSYS2 MINGW64)

### 1. Install Dependencies
Run the following commands in your MSYS2 MINGW64 terminal:

```bash
pacman -S --needed base-devel mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-multimedia \
    mingw-w64-x86_64-rtmidi mingw-w64-x86_64-libresidfp

pacman -S --needed mingw-w64-ucrt-x86_64-64tass
ln -sf /ucrt64/bin/64tass.exe /mingw64/bin/64tass.exe
```

### 2. Build the Project
```bash
mkdir -p build && cd build
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/mingw64
cmake --build .
```

### 3. Deploy and Run
```bash
cd build
windeployqt --no-translations SidDAW.exe
./SidDAW.exe
```
*Note: If the application complains about missing DLLs, copy them recursively with `ldd` (see `deploy_all.sh` in previous releases) or manually copy `libresidfp-0.dll`, `librtmidi-7.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, and `libwinpthread-1.dll`.*

---

## 🏗️ Architecture Overview

| Module | Purpose |
| :--- | :--- |
| **`src/Model.h`** | Core data model (Note, VoiceParams, Instrument, Pattern, Clip, Project) |
| **`src/SidEngine`** | Wrapper layer for the `reSIDfp` emulation library |
| **`src/ArrangementPlayer`** | Manages real-time playback across multiple timeline clips |
| **`src/PatternPlayer`** | Handles offline audio rendering for WAV exporting |
| **`src/SidAudioDevice`** | Bridges the player backend engine to Qt's `QAudioSink` |
| **`src/MainWindow`** | Main UI window, application wiring, and top menus |
| **`src/PianoRollWidget`** | Interactive piano roll note editor view |
| **`src/ArrangementWidget`** | Timeline layout featuring clip arrangement and visual previews |
| **`src/VoicePanelWidget`** | Voice parameters panel containing control sliders and 4 envelope tabs |
| **`src/EnvelopeEditorWidget`** | Canvas-based drawable envelope interface |
| **`src/InstrumentListWidget`** | List container serving as the drag-and-drop source for instruments |
| **`src/InstrumentEditorDialog`** | Pop-up configuration panel for instrument properties |
| **`src/NotePropertiesDialog`** | Detailed inspector for modifying individual note attributes |
| **`src/Commands.h`** | Custom `QUndoCommand` subclasses managing the edit history |
| **`src/MidiImporter` / `*Dialog`** | Standard MIDI parser and channel distribution selector |
| **`src/MidiRecorder`** | Intermediary buffer capturing incoming live MIDI events |
| **`src/ProjectIO` / `VoicePresetIO`** | Serializers handling project `.siddaw` and preset JSON files |
| **`src/WavExporter` / `SidExporter`** | Exporters producing finished `.wav` files and compiled `PSID` files |
| **`src/LanguageManager`** | Coordinates runtime interface localization (EN / RU / ZH / PT-BR) |
| **`player.asm`** | Native 6502 assembly SID playback routine (compiled with 64tass) |
| **`icon.rc` / `resources.qrc`** | Embedded executable icon resources and Qt bundle mappings |

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](https://github.com) file for details.

---

## 👤 Author

* **MKLIUKANG1** — [GitHub Profile](https://github.com)

<p align="center">
  <i>Built with 🎸 and love for the Commodore 64 demoscene</i>
</p>
