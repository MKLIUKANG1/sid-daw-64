#pragma once
#include "Model.h"
#include <cstdint>

class SidEngine;

class PatternPlayer {
public:
    explicit PatternPlayer(SidEngine *sid);
    void setPattern(const Pattern *p) { m_pattern = p; }
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

    SidEngine     *m_sid = nullptr;
    const Pattern *m_pattern = nullptr;
    int     m_bpm = 120;
    double  m_currentTick = 0.0;
    int     m_sampleAccum = 0;
    int     m_sampleRate = 44100;

    struct VoiceState {
        bool   active = false;
        int    pitch  = 60;
        double endTick   = 0.0;
        double startTick = 0.0;
        int    lastEnvIndex = -1;
        const VoiceParams *params = nullptr;
    } m_voices[3];
};
