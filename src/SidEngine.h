#pragma once
#include <cstdint>
#include <vector>

namespace reSIDfp { class residfp; }

class SidEngine {
public:
    SidEngine();
    ~SidEngine();

    void setSampleRate(int sr);
    void reset();
    void write(int reg, uint8_t value);

    // Генерирует `n` сэмплов (int16, моно) в out
    void clock(int n, int16_t *out);

    int sampleRate() const { return m_sampleRate; }
    double clockFrequency() const { return m_clockFreq; }

private:
    reSIDfp::residfp *m_sid = nullptr;
    int     m_sampleRate = 44100;
    double  m_clockFreq  = 985248.0; // PAL C64
    double  m_cyclesPerSample = 0.0;
    double  m_cycleAccum = 0.0;
};
