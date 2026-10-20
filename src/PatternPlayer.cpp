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
        case 0: ctrl |= 0x10; break;   // Triangle
        case 1: ctrl |= 0x20; break;   // Sawtooth
        case 2: ctrl |= 0x40; break;   // Pulse
        case 3: ctrl |= 0x80; break;   // Noise
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

    // SID low-pass фильтр — глобальный, но даёт «характер C64».
    // Per-instrument cutoff/resonance — следующая итерация.
    if (m_project) {
        m_sid->setFilterEnabled(m_project->sidGlobal.filterEnabled);
        m_sid->setFilterCutoff(m_project->sidGlobal.filterCutoff);
        m_sid->setFilterResonance(m_project->sidGlobal.filterResonance);
        m_sid->setFilterMode(m_project->sidGlobal.filterMode);
        m_sid->setSidVolume(m_project->sidGlobal.masterVolume);
    }

    m_voices[v].active       = true;
    m_voices[v].pitch        = n.pitch;
    m_voices[v].basePitch    = n.pitch;
    m_voices[v].endTick      = n.tick + n.length;
    m_voices[v].startTick    = n.tick;
    m_voices[v].lastVolIdx   = useEnv ? sus : -1;
    m_voices[v].lastDutyIdx  = vp.pulseWidth;
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
        double pos = elapsed * speed;
        int idx = int(pos);
        double frac = pos - idx;
        int base = v * 7;

        // Линейная интерполяция между двумя соседними точками огибающей.
        auto sampleEnv = [&](const QVector<int> &env, int def) -> int {
            if (env.isEmpty()) return def;
            int i0 = std::clamp(idx, 0, int(env.size()) - 1);
            int i1 = std::clamp(idx + 1, 0, int(env.size()) - 1);
            double v0 = env[i0];
            double v1 = env[i1];
            return int(std::round(v0 + (v1 - v0) * frac));
        };

        // Volume (регистр 6, верхний ниббл sustain)
        if (!vp.volumeEnvelope.isEmpty()) {
            int vol = std::clamp(sampleEnv(vp.volumeEnvelope, 15), 0, 15);
            if (vol != m_voices[v].lastVolIdx) {
                m_voices[v].lastVolIdx = vol;
                m_sid->write(base + 6,
                             ((vol & 0x0F) << 4) | (vp.release & 0x0F));
            }
        }

        // Pitch (центр = 8, диапазон -8..+7 полутонов)
        int pitchShift = 0;
        if (!vp.pitchEnvelope.isEmpty()) {
            int ev = sampleEnv(vp.pitchEnvelope, vp.pitchEnvelopeAbsolute ? 0 : 8);
            pitchShift = vp.pitchEnvelopeAbsolute ? ev : (ev - 8);
        }

        // Arp (0..15 полутонов вверх)
        int arpShift = 0;
        if (!vp.arpeggioEnvelope.isEmpty())
            arpShift = sampleEnv(vp.arpeggioEnvelope, 0);

        // Duty (0..15 → 0..4095)
        if (!vp.dutyEnvelope.isEmpty()) {
            int d = std::clamp(sampleEnv(vp.dutyEnvelope, 8), 0, 15);
            int pw = d * 273;   // 15 * 273 = 4095
            pw = std::clamp(pw, 0, 4095);
            if (pw != m_voices[v].lastDutyIdx) {
                m_voices[v].lastDutyIdx = pw;
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
                    m_sid->write(base + 4,
                                 composeControl(*m_voices[v].params, false));
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

        // 1. Note-offs
        if (m_pattern) {
            for (int v = 0; v < 3; ++v) {
                if (m_voices[v].active && nextTick >= m_voices[v].endTick) {
                    int base = v * 7;
                    if (m_voices[v].params)
                        m_sid->write(base + 4,
                            composeControl(*m_voices[v].params, false));
                    m_voices[v].active = false;
                }
            }
        }

        // 2. Сэмпл SID — gate-off фиксируется
        m_sid->clock(1, out + offset);

        // 3. Note-ons
        if (m_pattern) {
            for (const auto &n : m_pattern->notes) {
                if (n.tick > m_currentTick && n.tick <= nextTick)
                    noteOn(n);
            }
        }

        applyEnvelopes(nextTick);
        m_currentTick = nextTick;
        ++offset;
    }
}
