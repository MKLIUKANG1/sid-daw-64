#include "ArrangementPlayer.h"
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

ArrangementPlayer::ArrangementPlayer(SidEngine *sid) : m_sid(sid) {
    m_sampleRate = sid->sampleRate();
}

void ArrangementPlayer::resetVoices() {
    for (auto &v : m_voices) { v = VoiceState{}; }
}

double ArrangementPlayer::noteFrequency(int midiPitch) const {
    return 440.0 * std::pow(2.0, (midiPitch - 69) / 12.0);
}

void ArrangementPlayer::setVoiceMute(int v, bool m) {
    if (v < 0 || v > 2) return;
    m_mute[v] = m;
}

void ArrangementPlayer::setVoiceSolo(int v, bool s) {
    if (v < 0 || v > 2) return;
    m_solo[v] = s;
}

bool ArrangementPlayer::isVoiceSilenced(int v) const {
    if (v < 0 || v > 2) return false;
    bool anySolo = m_solo[0] || m_solo[1] || m_solo[2];
    if (anySolo) return !m_solo[v];
    return m_mute[v];
}

void ArrangementPlayer::rebuild() {
    m_flatNotes.clear();
        if (m_project) {
        m_sid->setFilterEnabled(m_project->sidGlobal.filterEnabled);
        m_sid->setFilterCutoff(m_project->sidGlobal.filterCutoff);
        m_sid->setFilterResonance(m_project->sidGlobal.filterResonance);
        m_sid->setFilterMode(m_project->sidGlobal.filterMode);
        m_sid->setSidVolume(m_project->sidGlobal.masterVolume);
    }
    if (!m_project) return;

    for (const auto &c : m_project->clips) {
        if (c.patternIndex < 0 || c.patternIndex >= (int)m_project->patterns.size())
            continue;
        const Pattern *pat = m_project->patterns[c.patternIndex].get();
        if (pat->lengthTicks <= 0) continue;

        int reps = (c.lengthTicks + pat->lengthTicks - 1) / pat->lengthTicks;
        for (int r = 0; r < reps; ++r) {
            for (const auto &n : pat->notes) {
                FlatNote fn;
                fn.tick   = c.startTick + r * pat->lengthTicks + n.tick;
                fn.pitch  = n.pitch;
                fn.length = n.length;
                fn.voice  = n.voice;

                const VoiceParams *params = &pat->voiceParams[std::clamp(n.voice, 0, 2)];
                if (n.instrumentIndex >= 0 &&
                    n.instrumentIndex < (int)m_project->instruments.size()) {
                    params = &m_project->instruments[n.instrumentIndex]->params;
                }
                fn.params = params;

                int clipEnd = c.startTick + c.lengthTicks;
                if (fn.tick >= clipEnd) continue;
                if (fn.tick + fn.length > clipEnd)
                    fn.length = clipEnd - fn.tick;
                if (fn.length > 0) m_flatNotes.append(fn);
            }
        }
    }

    std::sort(m_flatNotes.begin(), m_flatNotes.end(),
              [](const FlatNote &a, const FlatNote &b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.pitch > b.pitch;
    });
}

void ArrangementPlayer::writeFrequency(int voice, int basePitch, double semitoneShift) {
    int base = voice * 7;
    double f = noteFrequency(basePitch) * std::pow(2.0, semitoneShift / 12.0);
    uint32_t freq = uint32_t(std::round(f * 16777216.0 / m_sid->clockFrequency()));
    freq &= 0xFFFF;
    m_sid->write(base + 0, freq & 0xFF);
    m_sid->write(base + 1, (freq >> 8) & 0xFF);
}

void ArrangementPlayer::noteOn(const FlatNote &n) {
    int v = std::clamp(n.voice, 0, 2);
    if (isVoiceSilenced(v)) return;

    int base = v * 7;
    const VoiceParams &vp = *n.params;

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
    m_voices[v].params       = n.params;
}

void ArrangementPlayer::noteOff(int voice) {
    int v = std::clamp(voice, 0, 2);
    int base = v * 7;
    const VoiceParams &vp = m_voices[v].params ? *m_voices[v].params : VoiceParams{};
    m_sid->write(base + 4, composeControl(vp, false));
    m_voices[v].active = false;
}

void ArrangementPlayer::applyEnvelopes(double tick) {
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

        auto sampleEnv = [&](const QVector<int> &env, int def) -> int {
            if (env.isEmpty()) return def;
            int i0 = std::clamp(idx, 0, int(env.size()) - 1);
            int i1 = std::clamp(idx + 1, 0, int(env.size()) - 1);
            double v0 = env[i0];
            double v1 = env[i1];
            return int(std::round(v0 + (v1 - v0) * frac));
        };

        if (!vp.volumeEnvelope.isEmpty()) {
            int vol = std::clamp(sampleEnv(vp.volumeEnvelope, 15), 0, 15);
            if (vol != m_voices[v].lastVolIdx) {
                m_voices[v].lastVolIdx = vol;
                m_sid->write(base + 6, ((vol & 0x0F) << 4) | (vp.release & 0x0F));
            }
        }

        int pitchShift = 0;
        if (!vp.pitchEnvelope.isEmpty()) {
            int ev = sampleEnv(vp.pitchEnvelope, vp.pitchEnvelopeAbsolute ? 0 : 8);
            pitchShift = vp.pitchEnvelopeAbsolute ? ev : (ev - 8);
        }
        int arpShift = 0;
        if (!vp.arpeggioEnvelope.isEmpty())
            arpShift = sampleEnv(vp.arpeggioEnvelope, 0);

        if (!vp.dutyEnvelope.isEmpty()) {
            int d = std::clamp(sampleEnv(vp.dutyEnvelope, 8), 0, 15);
            int pw = std::clamp(d * 273, 0, 4095);
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

void ArrangementPlayer::processUntil(double tick) {
    for (int v = 0; v < 3; ++v)
        if (m_voices[v].active && isVoiceSilenced(v)) noteOff(v);

    for (int v = 0; v < 3; ++v) {
        if (m_voices[v].active && tick >= m_voices[v].endTick) {
            bool stillScheduled = false;
            for (const auto &n : m_flatNotes) {
                if (n.voice == v && n.tick <= tick && n.tick + n.length > tick) {
                    stillScheduled = true; break;
                }
            }
            if (!stillScheduled) noteOff(v);
        }
    }
    for (const auto &n : m_flatNotes) {
        if (n.tick > m_currentTick && n.tick <= tick) noteOn(n);
    }
    applyEnvelopes(tick);
    m_currentTick = tick;
}

void ArrangementPlayer::render(int16_t *out, int frames) {
    double ticksPerSec = m_bpm / 15.0;
    double ticksPerSample = ticksPerSec / double(m_sampleRate);
    int offset = 0;
    while (offset < frames) {
        double nextTick = m_currentTick + ticksPerSample;

        // 1. Note-offs: всегда, даже если новая нота начинается сразу.
        for (int v = 0; v < 3; ++v) {
            if (m_voices[v].active &&
                (isVoiceSilenced(v) || nextTick >= m_voices[v].endTick)) {
                noteOff(v);
            }
        }

        // 2. Один сэмпл SID — за ~22 цикла gate-off успевает зафиксироваться
        m_sid->clock(1, out + offset);
        out[offset] = int16_t(float(out[offset]) * m_masterVolume);

        // 3. Note-ons
        for (const auto &n : m_flatNotes) {
            if (n.tick > m_currentTick && n.tick <= nextTick)
                noteOn(n);
        }

        applyEnvelopes(nextTick);
        m_currentTick = nextTick;
        ++offset;
    }
}
