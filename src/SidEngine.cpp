#include "SidEngine.h"
#include <residfp/residfp.h>
#include <cmath>
#include <algorithm>

SidEngine::SidEngine() {
    m_sid = new reSIDfp::residfp;
    setSampleRate(m_sampleRate);
}

SidEngine::~SidEngine() { delete m_sid; }

void SidEngine::setSampleRate(int sr) {
    m_sampleRate = sr;
    m_cyclesPerSample = m_clockFreq / double(sr);
    m_sid->setSamplingParameters(
        m_clockFreq,
        reSIDfp::DECIMATE,
        double(sr));
    reset();
}

void SidEngine::reset() {
    m_sid->reset();
    m_cycleAccum = 0.0;
    // Базовая громкость (register 24 = master volume + filter mode)
    m_sid->write(24, 0x0F);
}

void SidEngine::write(int reg, uint8_t value) {
    if (reg >= 0 && reg < 0x20)
        m_sid->write(reg, value);
}

void SidEngine::clock(int n, int16_t *out) {
    for (int i = 0; i < n; ++i) {
        m_cycleAccum += m_cyclesPerSample;
        int cycles = int(m_cycleAccum);
        m_cycleAccum -= cycles;
        if (cycles < 1) cycles = 1;
        m_sid->clock(cycles, out + i);
    }
}
