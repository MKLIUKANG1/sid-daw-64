#pragma once
#include "Model.h"
#include <cstdint>

class SidEngine;

class PatternPlayer {
public:
    explicit PatternPlayer(SidEngine *sid);
    void setPattern(const Pattern *p) { m_pattern = p; }
    void setProject(const Project *p) { m_project = p; }
    void setBpm(int bpm) { m_bpm = bpm; }
    void setPlayheadTick(double t) { m_currentTick = t; }
    double playheadTick() const { return m_currentTick; }
    void render(int16_t *out, int frames);
    void resetVoices();

private:
    void processUntil(double tick);
    void applyEnvelopes(double tick);
    void noteOn(const Note &n);
    void noteOff(const Note &n);
    double noteFrequency(int midiPitch) const;
    const VoiceParams *resolveParams(const Note &n) const;
    void writeFrequency(int voice, int basePitch, double semitoneShift);

    SidEngine     *m_sid = nullptr;
    const Pattern *m_pattern = nullptr;
    const Project *m_project = nullptr;
    int     m_bpm = 120;
    double  m_currentTick = 0.0;
    int     m_sampleAccum = 0;
    int     m_sampleRate = 44100;

    struct VoiceState {
        bool   active = false;
        int    pitch  = 60;
        int    basePitch = 60;
        double endTick   = 0.0;
        double startTick = 0.0;
        int    lastVolIdx  = -1;   // текущее значение volume (не индекс)
        int    lastDutyIdx = -1;   // текущее значение pulse width
        const VoiceParams *params = nullptr;
    } m_voices[3];
};
