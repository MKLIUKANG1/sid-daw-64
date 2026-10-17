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

    // Огибающие, 0..15
    QVector<int> volumeEnvelope;      // default 15
    QVector<int> pitchEnvelope;       // default 8 (center = 0 semitones)
    QVector<int> dutyEnvelope;        // default 8 (center = PW 2048)
    QVector<int> arpeggioEnvelope;    // default 0 (no shift)

    int envelopeSpeed = 4;
};

struct Instrument {
    QString name = "Instrument";
    VoiceParams params;
};

struct Pattern {
    QString     name = "Pattern 1";
    int         lengthTicks = 64;
    QVector<Note> notes;
    VoiceParams voiceParams[3];
};

struct Clip {
    int patternIndex = 0;
    int track        = 0;
    int startTick    = 0;
    int lengthTicks  = 64;
};

struct Project {
    std::vector<std::unique_ptr<Pattern>> patterns;
    std::vector<std::unique_ptr<Instrument>> instruments;
    QVector<Clip> clips;
    int trackCount = 4;
    int totalTicks = 256;
};
