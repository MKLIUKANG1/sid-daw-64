#pragma once
#include <QString>
#include <QVector>
#include <memory>
#include <vector>

struct Note {
    int tick     = 0;
    int pitch    = 60;
    int length   = 4;
    int velocity = 12;
    int voice    = 0;
    int instrumentIndex = -1;
};

struct VoiceParams {
    int  waveform   = 1;
    int  pulseWidth = 2048;
    int  attack     = 1;
    int  decay      = 0;
    int  sustain    = 15;
    int  release    = 3;
    bool ringMod    = false;
    bool sync       = false;
    float volume    = 1.0f;
    float pan       = 0.0f;
    QVector<int> volumeEnvelope;
    QVector<int> pitchEnvelope;
    QVector<int> dutyEnvelope;
    QVector<int> arpeggioEnvelope;
    int envelopeSpeed = 4;
    bool pitchEnvelopeAbsolute = false;
};

struct Instrument {
    QString name = "Instrument";
    VoiceParams params;
};

struct DrumSlot {
    int     midiPitch = 36;
    QString name = "Kick";
    int     instrumentIndex = -1;
};

struct DrumKit {
    QString name = "Drum Kit";
    QList<DrumSlot> entries;
};

struct Pattern {
    QString     name = "Pattern 1";
    int         lengthTicks = 64;
    QVector<Note> notes;
    bool isDrumPattern = false;
    VoiceParams voiceParams[3];
};

struct Clip {
    int patternIndex = 0;
    int track        = 0;
    int startTick    = 0;
    int lengthTicks  = 64;
};

struct SidGlobal {
    int  filterCutoff    = 1024;   // 0..2047
    int  filterResonance = 8;      // 0..15
    int  filterMode      = 0;      // 0=LP, 1=BP, 2=HP
    bool filterEnabled   = true;
    int  masterVolume    = 15;     // 0..15
    int  filterVoices    = 0x07;   // битовая маска: бит 0=voice1, 1=voice2, 2=voice3
};

struct Project {
    std::vector<std::unique_ptr<Pattern>> patterns;
    std::vector<std::unique_ptr<Instrument>> instruments;
    DrumKit drumKit;
    QVector<Clip> clips;
    int trackCount = 8;
    int totalTicks = 256;
    SidGlobal sidGlobal;
};
