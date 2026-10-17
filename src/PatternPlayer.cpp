#include "PatternPlayer.h"
#include "SidEngine.h"
#include <cmath>
#include <algorithm>

static uint8_t composeControl(const VoiceParams &vp, bool gate) {
    uint8_t ctrl = 0x00;
    if (gate)       ctrl |= 0x01;
    if (vp.sync)    ctrl |= 0x02;
    if (vp.ringMod) ctrl |= 0x04;
    switch (vp.waveform) {
        case 0: ctrl |= 0x10; break;
        case 1: ctrl |= 0x20; break;
        case 2: ctrl |= 0x40; break;
        case 3: ctrl |= 0x80; break;
    }
    return ctrl;
}

PatternPlayer::PatternPlayer(SidEngine *sid) : m_sid(sid) {
    m_sampleRate = sid->sampleRate();
    resetVoices();
}

void PatternPlayer::resetVoices() {
    for (auto &v : m_voices) { v = VoiceState{}; }
    m_currentTick = 0.0;
    m_sampleAccum = 0;
}

double PatternPlayer::noteFrequency(int midiPitch) const {
    return 440.0 * std::pow(2.0, (midiPitch - 69) / 12.0);
}

const VoiceParams *PatternPlayer::resolveParams(const Note &n) const {
    int v = std::clamp(n.voice, 0, 2);
    if (m_project && n.instrumentIndex >= 0 &&
        n.instrumentIndex < (int)m_project->instruments.size()) {
        return &m_project->instruments[n.instrumentIndex]->params;
    }
    if (m_pattern) return &m_pattern->voiceParams[v];
    return nullptr;
}

void PatternPlayer::writeFrequency(int voice, int basePitch, double semitoneShift) {
    int base = voice * 7;
    double f = noteFrequency(basePitch) * std::pow(2.0, semitoneShift / 12.0);
    uint32_t freq = uint32_t(std::round(f * 16777216.0 / m_sid->clockFrequency()));
    freq &= 0xFFFF;
    m_sid->write(base + 0, freq & 0xFF);
    m_sid->write(base + 1, (freq >> 8) & 0xFF);
}

void PatternPlayer::noteOn(const Note &n) {
    int v = std::clamp(n.voice, 0, 2);
    int base = v * 7;
    const VoiceParams *vpPtr = resolveParams(n);
    if (!vpPtr) return;
    const VoiceParams &vp = *vpPtr;

    writeFrequency(v, n.pitch, 0.0);
    m_sid->write(base + 2, vp.pulseWidth & 0xFF);
    m_sid->write(base + 3, (vp.pulseWidth >> 8) & 0x0F);

    bool useEnv = !vp.volumeEnvelope.isEmpty();
    int atk = useEnv ? 0 : (vp.attack & 0x0F);
    int dec = useEnv ? 0 : (vp.decay & 0x0F);
    int sus;
    if (useEnv) sus = vp.volumeEnvelope.value(0, 15);
    else {
        sus = int(std::round(vp.sustain * vp.volume));
        sus = std::clamp(sus, 0, 15);
    }
    m_sid->write(base + 5, ((atk & 0x0F) << 4) | (dec & 0x0F));
    m_sid->write(base + 6, ((sus & 0x0F) << 4) | (vp.release & 0x0F));
    m_sid->write(base + 4, composeControl(vp, true));

    m_voices[v].active       = true;
    m_voices[v].pitch        = n.pitch;
    m_voices[v].basePitch    = n.pitch;
    m_voices[v].endTick      = n.tick + n.length;
    m_voices[v].startTick    = n.tick;
    m_voices[v].lastVolIdx   = useEnv ? 0 : -1;
    m_voices[v].lastDutyIdx  = -1;
    m_voices[v].params       = vpPtr;
}

void PatternPlayer::noteOff(const Note &n) {
    int v = std::clamp(n.voice, 0, 2);
    int base = v * 7;
    const VoiceParams *vpPtr = resolveParams(n);
    if (!vpPtr) return;
    m_sid->write(base + 4, composeControl(*vpPtr, false));
    m_voices[v].active = false;
}

void PatternPlayer::applyEnvelopes(double tick) {
    for (int v = 0; v < 3; ++v) {
        if (!m_voices[v].active || !m_voices[v].params) continue;
        const VoiceParams &vp = *m_voices[v].params;
        int speed = std::max(1, vp.envelopeSpeed);
        double elapsed = tick - m_voices[v].startTick;
        if (elapsed < 0) elapsed = 0;
        int idx = int(elapsed * speed);
        int base = v * 7;

        if (!vp.volumeEnvelope.isEmpty()) {
            int vi = std::min(idx, int(vp.volumeEnvelope.size()) - 1);
            if (vi >= 0 && vi != m_voices[v].lastVolIdx) {
                m_voices[v].lastVolIdx = vi;
                int vol = vp.volumeEnvelope[vi];
                m_sid->write(base + 6, ((vol & 0x0F) << 4) | (vp.release & 0x0F));
            }
        }

        int pitchShift = 0;
        if (!vp.pitchEnvelope.isEmpty()) {
            int pi = std::min(idx, int(vp.pitchEnvelope.size()) - 1);
            if (pi >= 0) pitchShift = vp.pitchEnvelope[pi] - 8;
        }
        int arpShift = 0;
        if (!vp.arpeggioEnvelope.isEmpty()) {
            int ai = std::min(idx, int(vp.arpeggioEnvelope.size()) - 1);
            if (ai >= 0) arpShift = vp.arpeggioEnvelope[ai];
        }
        if (!vp.dutyEnvelope.isEmpty()) {
            int di = std::min(idx, int(vp.dutyEnvelope.size()) - 1);
            if (di >= 0 && di != m_voices[v].lastDutyIdx) {
                m_voices[v].lastDutyIdx = di;
                int pw = vp.dutyEnvelope[di] * 273;
                if (pw < 0) pw = 0;
                if (pw > 4095) pw = 4095;
                m_sid->write(base + 2, pw & 0xFF);
                m_sid->write(base + 3, (pw >> 8) & 0x0F);
            }
        }
        if (pitchShift != 0 || arpShift != 0)
            writeFrequency(v, m_voices[v].basePitch, pitchShift + arpShift);
    }
}

void PatternPlayer::processUntil(double tick) {
    if (!m_pattern) { m_currentTick = tick; return; }
    for (int v = 0; v < 3; ++v) {
        if (m_voices[v].active && tick >= m_voices[v].endTick) {
            bool found = false;
            for (const auto &n : m_pattern->notes) {
                if (n.voice == v && n.tick <= tick && n.tick + n.length > tick) {
                    found = true; break;
                }
            }
            if (!found) {
                int base = v * 7;
                if (m_voices[v].params)
                    m_sid->write(base + 4, composeControl(*m_voices[v].params, false));
                m_voices[v].active = false;
            }
        }
    }
    for (const auto &n : m_pattern->notes) {
        if (n.tick > m_currentTick && n.tick <= tick) noteOn(n);
    }
    applyEnvelopes(tick);
    m_currentTick = tick;
}

void PatternPlayer::render(int16_t *out, int frames) {
    double ticksPerSec = m_bpm / 15.0;
    double ticksPerSample = ticksPerSec / double(m_sampleRate);
    int offset = 0;
    while (offset < frames) {
        double nextTick = m_currentTick + ticksPerSample;
        processUntil(nextTick);
        m_sid->clock(1, out + offset);
        ++offset;
    }
}
