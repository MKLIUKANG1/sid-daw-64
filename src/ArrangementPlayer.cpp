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

void ArrangementPlayer::rebuild() {
    m_flatNotes.clear();
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
                fn.params = &pat->voiceParams[std::clamp(n.voice, 0, 2)];

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

void ArrangementPlayer::noteOn(const FlatNote &n) {
    int v = std::clamp(n.voice, 0, 2);
    int base = v * 7;
    const VoiceParams &vp = *n.params;

    double f = noteFrequency(n.pitch);
    uint32_t freq = uint32_t(std::round(f * 16777216.0 / m_sid->clockFrequency()));
    freq &= 0xFFFF;

    m_sid->write(base + 0, freq & 0xFF);
    m_sid->write(base + 1, (freq >> 8) & 0xFF);
    m_sid->write(base + 2, vp.pulseWidth & 0xFF);
    m_sid->write(base + 3, (vp.pulseWidth >> 8) & 0x0F);

    bool useEnv = !vp.volumeEnvelope.isEmpty();
    int atk = useEnv ? 0 : (vp.attack & 0x0F);
    int dec = useEnv ? 0 : (vp.decay & 0x0F);
    int sus;
    if (useEnv) {
        sus = vp.volumeEnvelope.value(0, 15);
    } else {
        sus = int(std::round(vp.sustain * vp.volume));
        sus = std::clamp(sus, 0, 15);
    }
    m_sid->write(base + 5, ((atk & 0x0F) << 4) | (dec & 0x0F));
    m_sid->write(base + 6, ((sus & 0x0F) << 4) | (vp.release & 0x0F));

    m_sid->write(base + 4, composeControl(vp, true));

    m_voices[v].active       = true;
    m_voices[v].pitch        = n.pitch;
    m_voices[v].endTick      = n.tick + n.length;
    m_voices[v].startTick    = n.tick;
    m_voices[v].lastEnvIndex = useEnv ? 0 : -1;
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
        if (vp.volumeEnvelope.isEmpty()) continue;

        int speed = std::max(1, vp.envelopeSpeed);
        double elapsed = tick - m_voices[v].startTick;
        if (elapsed < 0) elapsed = 0;
        int idx = int(elapsed * speed);
        if (idx >= vp.volumeEnvelope.size())
            idx = vp.volumeEnvelope.size() - 1;
        if (idx < 0) idx = 0;

        if (idx == m_voices[v].lastEnvIndex) continue;
        m_voices[v].lastEnvIndex = idx;

        int vol = vp.volumeEnvelope[idx];
        int base = v * 7;
        m_sid->write(base + 6, ((vol & 0x0F) << 4) | (vp.release & 0x0F));
    }
}

void ArrangementPlayer::processUntil(double tick) {
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
        processUntil(nextTick);
        m_sid->clock(1, out + offset);
        out[offset] = int16_t(float(out[offset]) * m_masterVolume);
        ++offset;
    }
}
