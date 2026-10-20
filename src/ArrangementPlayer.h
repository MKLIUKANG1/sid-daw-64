#pragma once
#include <QVector>
#include <cstdint>
#include "Model.h"

class SidEngine;

class ArrangementPlayer {
public:
    explicit ArrangementPlayer(SidEngine *sid);

    void setProject(const Project *p) { m_project = p; }
    void setBpm(int bpm) { m_bpm = bpm; }
    void setPlayheadTick(double t) { m_currentTick = t; }
    void setMasterVolume(float v) { m_masterVolume = v; }
    float masterVolume() const { return m_masterVolume; }
    double playheadTick() const { return m_currentTick; }

    void setVoiceMute(int v, bool m);
    void setVoiceSolo(int v, bool s);
    bool isVoiceSilenced(int v) const;

    void rebuild();
    void render(int16_t *out, int frames);
    void resetVoices();

private:
    struct FlatNote {
        int tick;
        int pitch;
        int length;
        int voice;
        const VoiceParams *params;
    };

    struct VoiceState {
        bool active = false;
        int  pitch  = 60;
        int  basePitch = 60;
        double endTick   = 0.0;
        double startTick = 0.0;
        int    lastVolIdx  = -1;
        int    lastDutyIdx = -1;
        const VoiceParams *params = nullptr;
    };

    void processUntil(double tick);
    void applyEnvelopes(double tick);
    void noteOn(const FlatNote &n);
    void noteOff(int voice);
    double noteFrequency(int midiPitch) const;
    void writeFrequency(int voice, int basePitch, double semitoneShift);

    SidEngine         *m_sid = nullptr;
    const Project     *m_project = nullptr;
    int                m_bpm = 120;
    double             m_currentTick = 0.0;
    int                m_sampleRate = 44100;
    float              m_masterVolume = 1.0f;
    QVector<FlatNote>  m_flatNotes;
    VoiceState         m_voices[3];
    bool               m_mute[3] = {false, false, false};
    bool               m_solo[3] = {false, false, false};
};
