#include "MidiPresetManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

MidiPresetManager::MidiPresetManager() {
    buildEmbeddedTable();
}

VoiceParams MidiPresetManager::getDefaultParameters() const {
    VoiceParams vp;
    vp.waveform = 1;   // Sawtooth
    vp.attack = 1;
    vp.decay = 4;
    vp.sustain = 10;
    vp.release = 3;
    return vp;
}

static QString gmNameFor(int program) {
    static const char* names[128] = {
        "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano",
        "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavi",
        "Celesta", "Glockenspiel", "Music Box", "Vibraphone",
        "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
        "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ",
        "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
        "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)",
        "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics",
        "Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass",
        "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
        "Violin", "Viola", "Cello", "Contrabass",
        "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
        "String Ensemble 1", "String Ensemble 2", "Synth Strings 1", "Synth Strings 2",
        "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit",
        "Trumpet", "Trombone", "Tuba", "Muted Trumpet",
        "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2",
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax",
        "Oboe", "English Horn", "Bassoon", "Clarinet",
        "Piccolo", "Flute", "Recorder", "Pan Flute",
        "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
        "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)",
        "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass+lead)",
        "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)",
        "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
        "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)",
        "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)",
        "Sitar", "Banjo", "Shamisen", "Koto",
        "Kalimba", "Bag pipe", "Fiddle", "Shanai",
        "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock",
        "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
        "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet",
        "Telephone Ring", "Helicopter", "Applause", "Gunshot"
    };
    if (program < 0 || program > 127) return "Unknown";
    return QString::fromUtf8(names[program]);
}

QString MidiPresetManager::nameForProgram(int program) {
    return gmNameFor(program);
}

void MidiPresetManager::buildEmbeddedTable() {
    m_presets.clear();
    // Базовые пресеты по семействам GM (каждые 8 программ = одно семейство)
    for (int family = 0; family < 16; ++family) {
        VoiceParams vp;
        switch (family) {
            case 0:  vp.waveform = 2; vp.attack = 0; vp.decay = 6;  vp.sustain = 3;  vp.release = 4; vp.pulseWidth = 2048; break; // Piano
            case 1:  vp.waveform = 0; vp.attack = 0; vp.decay = 4;  vp.sustain = 2;  vp.release = 3; break; // Chromatic Perc
            case 2:  vp.waveform = 2; vp.attack = 1; vp.decay = 2;  vp.sustain = 14; vp.release = 2; vp.pulseWidth = 1024; break; // Organ
            case 3:  vp.waveform = 1; vp.attack = 0; vp.decay = 5;  vp.sustain = 3;  vp.release = 3; break; // Guitar
            case 4:  vp.waveform = 0; vp.attack = 0; vp.decay = 3;  vp.sustain = 12; vp.release = 3; break; // Bass
            case 5:  vp.waveform = 1; vp.attack = 3; vp.decay = 4;  vp.sustain = 13; vp.release = 4; break; // Strings
            case 6:  vp.waveform = 1; vp.attack = 4; vp.decay = 5;  vp.sustain = 13; vp.release = 5; break; // Ensemble
            case 7:  vp.waveform = 2; vp.attack = 2; vp.decay = 3;  vp.sustain = 12; vp.release = 3; vp.pulseWidth = 1536; break; // Brass
            case 8:  vp.waveform = 2; vp.attack = 2; vp.decay = 3;  vp.sustain = 12; vp.release = 3; vp.pulseWidth = 3072; break; // Reed
            case 9:  vp.waveform = 0; vp.attack = 2; vp.decay = 3;  vp.sustain = 14; vp.release = 3; break; // Pipe
            case 10: vp.waveform = 2; vp.attack = 0; vp.decay = 2;  vp.sustain = 14; vp.release = 3; vp.pulseWidth = 2048; break; // Synth Lead
            case 11: vp.waveform = 1; vp.attack = 5; vp.decay = 6;  vp.sustain = 14; vp.release = 6; break; // Synth Pad
            case 12: vp.waveform = 3; vp.attack = 3; vp.decay = 5;  vp.sustain = 8;  vp.release = 5; break; // Synth Effects
            case 13: vp.waveform = 2; vp.attack = 0; vp.decay = 4;  vp.sustain = 6;  vp.release = 3; break; // Ethnic
            case 14: vp.waveform = 0; vp.attack = 0; vp.decay = 3;  vp.sustain = 0;  vp.release = 2; break; // Percussive
            case 15: vp.waveform = 3; vp.attack = 0; vp.decay = 4;  vp.sustain = 0;  vp.release = 3; break; // Sound Effects
        }
        for (int i = 0; i < 8; ++i)
            m_presets[family * 8 + i] = vp;
    }
    // Точечные переопределения для узнаваемых инструментов
    auto set = [this](int prog, VoiceParams vp) { m_presets[prog] = vp; };

    { VoiceParams vp; vp.waveform = 2; vp.attack = 0; vp.decay = 6; vp.sustain = 3; vp.release = 5; vp.pulseWidth = 2048; set(0, vp); }   // Grand Piano
    { VoiceParams vp; vp.waveform = 0; vp.attack = 0; vp.decay = 4; vp.sustain = 2; vp.release = 4; set(9, vp); }   // Glockenspiel
    { VoiceParams vp; vp.waveform = 2; vp.attack = 1; vp.decay = 2; vp.sustain = 14; vp.release = 2; vp.pulseWidth = 1024; set(19, vp); } // Church Organ
    { VoiceParams vp; vp.waveform = 1; vp.attack = 0; vp.decay = 5; vp.sustain = 2; vp.release = 3; set(24, vp); }  // Nylon Guitar
    { VoiceParams vp; vp.waveform = 0; vp.attack = 0; vp.decay = 3; vp.sustain = 10; vp.release = 3; set(32, vp); } // Acoustic Bass
    { VoiceParams vp; vp.waveform = 1; vp.attack = 3; vp.decay = 4; vp.sustain = 13; vp.release = 4; set(40, vp); } // Violin
    { VoiceParams vp; vp.waveform = 1; vp.attack = 4; vp.decay = 5; vp.sustain = 13; vp.release = 5; set(48, vp); } // String Ensemble 1
    { VoiceParams vp; vp.waveform = 2; vp.attack = 2; vp.decay = 3; vp.sustain = 13; vp.release = 3; vp.pulseWidth = 1536; set(56, vp); } // Trumpet
    { VoiceParams vp; vp.waveform = 2; vp.attack = 2; vp.decay = 3; vp.sustain = 12; vp.release = 3; vp.pulseWidth = 3072; set(64, vp); } // Soprano Sax
    { VoiceParams vp; vp.waveform = 0; vp.attack = 2; vp.decay = 3; vp.sustain = 14; vp.release = 3; set(72, vp); } // Piccolo
    { VoiceParams vp; vp.waveform = 2; vp.attack = 0; vp.decay = 2; vp.sustain = 14; vp.release = 3; vp.pulseWidth = 2048; set(80, vp); } // Lead 1 (square)
    { VoiceParams vp; vp.waveform = 1; vp.attack = 0; vp.decay = 2; vp.sustain = 14; vp.release = 3; set(81, vp); } // Lead 2 (sawtooth)
    { VoiceParams vp; vp.waveform = 1; vp.attack = 5; vp.decay = 6; vp.sustain = 14; vp.release = 6; set(88, vp); } // Pad 1 (new age)
    { VoiceParams vp; vp.waveform = 3; vp.attack = 3; vp.decay = 5; vp.sustain = 8; vp.release = 5; set(96, vp); } // FX 1 (rain)
    { VoiceParams vp; vp.waveform = 2; vp.attack = 0; vp.decay = 4; vp.sustain = 6; vp.release = 3; set(104, vp); } // Sitar
    { VoiceParams vp; vp.waveform = 2; vp.attack = 0; vp.decay = 3; vp.sustain = 6; vp.release = 2; set(112, vp); } // Tinkle Bell
    { VoiceParams vp; vp.waveform = 3; vp.attack = 0; vp.decay = 2; vp.sustain = 0; vp.release = 1; set(120, vp); } // Fret Noise
}

VoiceParams MidiPresetManager::getParamsForMidiProgram(int programNumber) const {
    if (programNumber < 0 || programNumber > 127)
        return getDefaultParameters();
    auto it = m_presets.find(programNumber);
    if (it != m_presets.end()) return it.value();
    return getDefaultParameters();
}

bool MidiPresetManager::loadFromJson(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return false;

    QJsonObject root = doc.object();
    if (!root.contains("gm_instruments")) return false;
    QJsonArray arr = root["gm_instruments"].toArray();

    for (const auto &v : arr) {
        QJsonObject o = v.toObject();
        if (!o.contains("program_id")) continue;
        int prog = o["program_id"].toInt();
        if (prog < 0 || prog > 127) continue;

        VoiceParams vp = m_presets.value(prog, getDefaultParameters());
        if (o.contains("waveform"))    vp.waveform   = o["waveform"].toInt(vp.waveform);
        if (o.contains("attack"))      vp.attack     = o["attack"].toInt(vp.attack);
        if (o.contains("decay"))       vp.decay      = o["decay"].toInt(vp.decay);
        if (o.contains("sustain"))     vp.sustain    = o["sustain"].toInt(vp.sustain);
        if (o.contains("release"))     vp.release    = o["release"].toInt(vp.release);
        if (o.contains("pulse_width")) vp.pulseWidth = o["pulse_width"].toInt(vp.pulseWidth);
        m_presets[prog] = vp;
    }
    return true;
}
