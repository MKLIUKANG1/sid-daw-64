#include "SidEngine.h"
#include <residfp/residfp.h>
#include <cmath>
#include <algorithm>

SidEngine::SidEngine() {
    m_sid = new reSIDfp::residfp;
    setSampleRate(m_sampleRate);
}

SidEngine::~SidEngine() {
    delete m_sid;
}

void SidEngine::setSampleRate(int sr) {
    m_sampleRate = sr;
    m_cyclesPerSample = m_clockFreq / double(sr);

    m_dc_alpha = 1.0 - 2.0 * 3.14159265 * 10.0 / double(sr);

    double fc = 16000.0;
    m_lp_alpha = 1.0 / (1.0 + double(sr) / (2.0 * 3.14159265 * fc));

    m_sid->setSamplingParameters(m_clockFreq, reSIDfp::DECIMATE, double(sr));
    reset();
}

void SidEngine::reset() {
    m_sid->reset();
    m_cycleAccum = 0.0;
    m_lastSample = 0;
    m_dc_x1 = 0.0;
    m_dc_y1 = 0.0;
    m_lp_y1 = 0.0;

    setSidVolume(m_sidVolume);
    updateFilterRegister23();
}

void SidEngine::write(int reg, uint8_t value) {
    if (reg >= 0 && reg < 0x20)
        m_sid->write(reg, value);
}

void SidEngine::setFilterCutoff(int cutoff) {
    m_filterCutoff = std::clamp(cutoff, 0, 2047);
    m_sid->write(0x15, m_filterCutoff & 0xFF);
    m_sid->write(0x16, (m_filterCutoff >> 8) & 0x07);
}

void SidEngine::setFilterResonance(int res) {
    m_filterRes = std::clamp(res, 0, 15);
    updateFilterRegister23();
}

void SidEngine::setFilterMode(int mode) {
    m_filterMode = std::clamp(mode, 0, 2);
    updateFilterRegister23();
}

void SidEngine::setFilterEnabled(bool en) {
    m_filterEnabled = en;
    updateFilterRegister23();
}

void SidEngine::setSidVolume(int vol) {
    m_sidVolume = std::clamp(vol, 0, 15);
    updateFilterRegister23();
}

void SidEngine::updateFilterRegister23() {
    uint8_t reg23 = 0;
    reg23 |= (m_filterRes & 0x0F) << 4;
    if (m_filterEnabled) reg23 |= (1 << 3);
    reg23 |= (m_filterMode & 0x03) << 1;
    m_sid->write(0x17, reg23);
    m_sid->write(0x18, m_sidVolume & 0x0F);
}

void SidEngine::clock(int n, int16_t *out) {
    for (int i = 0; i < n; ++i) {
        m_cycleAccum += m_cyclesPerSample;
        int cycles = int(m_cycleAccum);
        m_cycleAccum -= cycles;
        if (cycles < 1) cycles = 1;

        short raw = m_lastSample;
        int produced = m_sid->clock((unsigned int)cycles, &raw);
        if (produced > 0)
            m_lastSample = int16_t(raw);

        double x = double(m_lastSample);
        double y = x - m_dc_x1 + m_dc_alpha * m_dc_y1;
        m_dc_x1 = x;
        m_dc_y1 = y;

        m_lp_y1 += m_lp_alpha * (y - m_lp_y1);

        double v = std::clamp(m_lp_y1, -32768.0, 32767.0);
        out[i] = int16_t(v);
    }
}
