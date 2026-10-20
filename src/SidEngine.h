#pragma once
#include <cstdint>

namespace reSIDfp { class residfp; }

class SidEngine {
public:
    SidEngine();
    ~SidEngine();

    void setSampleRate(int sr);
    void reset();
    void write(int reg, uint8_t value);
    void clock(int n, int16_t *out);

    int sampleRate() const { return m_sampleRate; }
    double clockFrequency() const { return m_clockFreq; }

    void setFilterCutoff(int cutoff);
    void setFilterResonance(int res);
    void setFilterMode(int mode);
    void setFilterEnabled(bool en);
    void setSidVolume(int vol);

private:
    void updateFilterRegister23();

    reSIDfp::residfp *m_sid = nullptr;

    int     m_sampleRate = 44100;
    double  m_clockFreq  = 985248.0;
    double  m_cyclesPerSample = 0.0;
    double  m_cycleAccum = 0.0;

    int16_t m_lastSample = 0;

    double  m_dc_x1 = 0.0;
    double  m_dc_y1 = 0.0;
    double  m_dc_alpha = 0.9985;
    double  m_lp_y1  = 0.0;
    double  m_lp_alpha = 0.5;

    int  m_filterCutoff = 1024;
    int  m_filterRes     = 8;
    int  m_filterMode    = 0;
    bool m_filterEnabled = true;
    int  m_sidVolume     = 15;
};
