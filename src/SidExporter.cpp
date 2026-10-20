#include "SidExporter.h"
#include "SidEngine.h"
#include <QFile>
#include <QCoreApplication>
#include <QVector>
#include <QMap>
#include <QPair>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <QDebug>

namespace {

struct FrameEvent {
    int frame = 0;
    QVector<QPair<int,int>> writes;
};

struct Flat {
    int tick, pitch, length, voice;
    const VoiceParams *params;
    int startFrame, endFrame;
};

uint8_t composeControl(const VoiceParams &vp, bool gate) {
    uint8_t ctrl = 0;
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

double noteFreq(int midi) {
    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
}

constexpr double CLOCK = 985248.0;
constexpr int    FPS   = 50;

QVector<FrameEvent> buildFrames(const Project &project, int bpm) {
    // 1. Flatten notes with resolved params (same as ArrangementPlayer)
    QVector<Flat> flat;
    for (const auto &c : project.clips) {
        if (c.patternIndex < 0 || c.patternIndex >= (int)project.patterns.size())
            continue;
        const Pattern *pat = project.patterns[c.patternIndex].get();
        if (pat->lengthTicks <= 0) continue;
        int reps = (c.lengthTicks + pat->lengthTicks - 1) / pat->lengthTicks;
        for (int r = 0; r < reps; ++r) {
            for (const auto &n : pat->notes) {
                Flat f;
                f.tick = c.startTick + r * pat->lengthTicks + n.tick;
                f.pitch = n.pitch;
                f.length = n.length;
                f.voice = n.voice;

                const VoiceParams *vp = &pat->voiceParams[std::clamp(n.voice, 0, 2)];
                if (n.instrumentIndex >= 0 &&
                    n.instrumentIndex < (int)project.instruments.size()) {
                    vp = &project.instruments[n.instrumentIndex]->params;
                }
                f.params = vp;

                int clipEnd = c.startTick + c.lengthTicks;
                if (f.tick >= clipEnd) continue;
                if (f.tick + f.length > clipEnd) f.length = clipEnd - f.tick;
                if (f.length <= 0) continue;
                flat.append(f);
            }
        }
    }
    if (flat.isEmpty()) return {};

    std::sort(flat.begin(), flat.end(), [](const Flat &a, const Flat &b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.pitch > b.pitch;
    });

    // 2. Tick → frame
    auto tickToFrame = [&](int t) {
        return int(std::round(double(t) * FPS * 15.0 / double(bpm)));
    };
    for (auto &n : flat) {
        n.startFrame = tickToFrame(n.tick);
        n.endFrame   = tickToFrame(n.tick + n.length);
        if (n.endFrame <= n.startFrame) n.endFrame = n.startFrame + 1;
    }

    // 3. Build register event stream
    QVector<FrameEvent> events;
    auto appendEvent = [&](int frame, int reg, int val) {
        for (auto &e : events) {
            if (e.frame == frame) { e.writes.append({reg, val}); return; }
        }
        FrameEvent e;
        e.frame = frame;
        e.writes.append({reg, val});
        events.append(e);
    };

    // SID filter setup at frame 0 (matches live SidEngine: cutoff 1024, RES 8, LP)
    appendEvent(0, 0x15, 0x00);  // cutoff lo = 0x00
    appendEvent(0, 0x16, 0x04);  // cutoff hi = 0x04  (cutoff = 0x400 = 1024)
    appendEvent(0, 0x17, 0x88);  // RES=8, FILT=1, MODE=LP(0)
    appendEvent(0, 0x18, 0x0F);  // master volume = 15

    // Track per-voice last register values to avoid redundant writes
    int prevReg[3][7];
    for (int v = 0; v < 3; ++v)
        for (int r = 0; r < 7; ++r)
            prevReg[v][r] = -1;

    for (const auto &n : flat) {
        int v = std::clamp(n.voice, 0, 2);
        int base = v * 7;
        const VoiceParams &vp = *n.params;

        auto computeFreq = [&](double semitones, int &lo, int &hi) {
            double f = noteFreq(n.pitch) * std::pow(2.0, semitones / 12.0);
            uint32_t fi = uint32_t(std::round(f * 16777216.0 / CLOCK)) & 0xFFFF;
            lo = fi & 0xFF;
            hi = (fi >> 8) & 0xFF;
        };

        auto writeReg = [&](int frame, int regIdx, int val) {
            if (prevReg[v][regIdx] == val) return;
            prevReg[v][regIdx] = val;
            appendEvent(frame, base + regIdx, val);
        };

        // Force full write at note-on
        for (int r = 0; r < 7; ++r) prevReg[v][r] = -1;

        // -------- Note ON --------
        int fLo, fHi;
        computeFreq(0.0, fLo, fHi);
        writeReg(n.startFrame, 0, fLo);
        writeReg(n.startFrame, 1, fHi);

        int pw = vp.pulseWidth & 0x0FFF;
        writeReg(n.startFrame, 2, pw & 0xFF);
        writeReg(n.startFrame, 3, (pw >> 8) & 0x0F);

        int ad = ((vp.attack & 0x0F) << 4) | (vp.decay & 0x0F);
        int sr = ((vp.sustain & 0x0F) << 4) | (vp.release & 0x0F);
        writeReg(n.startFrame, 5, ad);
        writeReg(n.startFrame, 6, sr);

        uint8_t ctrlOn = composeControl(vp, true);
        writeReg(n.startFrame, 4, ctrlOn);

        // -------- Envelope updates during the note --------
        int speed = std::max(1, vp.envelopeSpeed);
        int prevIdx = -1;
        for (int frame = n.startFrame; frame <= n.endFrame; ++frame) {
            double elapsedTicks = double(frame - n.startFrame) * bpm / 750.0;
            int idx = int(elapsedTicks * speed);
            if (idx == prevIdx) continue;
            prevIdx = idx;

            // Volume (envelope or fallback sustain)
            int vol;
            if (!vp.volumeEnvelope.isEmpty()) {
                int vi = std::min(idx, int(vp.volumeEnvelope.size()) - 1);
                vol = std::clamp(vp.volumeEnvelope[vi], 0, 15);
            } else {
                vol = int(std::round(vp.sustain * vp.volume));
                vol = std::clamp(vol, 0, 15);
            }
            int srNew = ((vol & 0x0F) << 4) | (vp.release & 0x0F);
            writeReg(frame, 6, srNew);

            // Pitch + Arp → frequency
            double semitones = 0.0;
            if (!vp.pitchEnvelope.isEmpty()) {
                int pi = std::min(idx, int(vp.pitchEnvelope.size()) - 1);
                int ev = vp.pitchEnvelope[pi];
                semitones += vp.pitchEnvelopeAbsolute ? ev : (ev - 8);
            }
            if (!vp.arpeggioEnvelope.isEmpty()) {
                int ai = std::min(idx, int(vp.arpeggioEnvelope.size()) - 1);
                semitones += vp.arpeggioEnvelope[ai];
            }
            if (semitones != 0.0) {
                int nlo, nhi;
                computeFreq(semitones, nlo, nhi);
                writeReg(frame, 0, nlo);
                writeReg(frame, 1, nhi);
            }

            // Duty
            if (!vp.dutyEnvelope.isEmpty()) {
                int di = std::min(idx, int(vp.dutyEnvelope.size()) - 1);
                int dpw = std::clamp(vp.dutyEnvelope[di] * 273, 0, 4095);
                writeReg(frame, 2, dpw & 0xFF);
                writeReg(frame, 3, (dpw >> 8) & 0x0F);
            }
        }

        // -------- Note OFF --------
        // Если в этом же кадре на этом голосе начинается другая нота —
        // сдвигаем gate-off на 1 кадр раньше, чтобы SID успел сбросить атаку.
        int offFrame = n.endFrame;
        for (const auto &other : flat) {
            if (other.voice == n.voice &&
                other.startFrame == n.endFrame &&
                &other != &n) {
                offFrame = n.endFrame - 1;
                if (offFrame <= n.startFrame) offFrame = n.startFrame;
                break;
            }
        }
        // Для gate-off надо «забыть» предыдущие значения prevReg,
        // чтобы gate перезаписался
        prevReg[v][4] = -1;
        writeReg(offFrame, 4, ctrlOn & ~0x01);
    }

    // 4. Sort by frame, merge same-frame, dedupe (last write per reg wins)
    std::sort(events.begin(), events.end(),
              [](const FrameEvent &a, const FrameEvent &b) {
        return a.frame < b.frame;
    });

    QVector<FrameEvent> merged;
    for (const auto &e : events) {
        if (!merged.isEmpty() && merged.last().frame == e.frame)
            merged.last().writes.append(e.writes);
        else
            merged.append(e);
    }

    for (auto &e : merged) {
        QMap<int,int> map;
        for (const auto &w : e.writes) map[w.first] = w.second;
        e.writes.clear();
        for (auto it = map.begin(); it != map.end(); ++it)
            e.writes.append({it.key(), it.value()});
    }
    return merged;
}

QByteArray buildDataStream(const QVector<FrameEvent> &merged, int bpm,
                            int totalTicks) {
    QByteArray d;
    if (merged.isEmpty()) return d;

    // Off-by-one fix:
    // sid_play уменьшает wait перед проверкой, поэтому для исполнения
    // на кадре N нужно установить wait = N - 1.

    int firstFrame = merged[0].frame;
    int firstWait  = std::max(0, firstFrame - 1);
    d.append(char(0));                       // count = 0 (только подождать)
    d.append(char(firstWait & 0xFF));
    d.append(char((firstWait >> 8) & 0xFF));

    int endFrame = int(std::round(double(totalTicks) * FPS * 15.0 / double(bpm)));

    for (int i = 0; i < merged.size(); ++i) {
        const auto &e = merged[i];
        int delay;
        if (i + 1 < merged.size())
            delay = merged[i + 1].frame - e.frame - 1;   // ← -1
        else
            delay = endFrame - e.frame + 49;             // 50 - 1

        if (delay < 0) delay = 0;
        if (delay > 0xFFFF) delay = 0xFFFF;

        d.append(char(e.writes.size() & 0xFF));
        for (const auto &w : e.writes) {
            d.append(char(w.first & 0xFF));
            d.append(char(w.second & 0xFF));
        }
        d.append(char(delay & 0xFF));
        d.append(char((delay >> 8) & 0xFF));
    }

    d.append(char(0x80));
    return d;
}

void writePSID(QByteArray &out, const QByteArray &payload,
               uint16_t loadAddr, uint16_t initAddr, uint16_t playAddr) {
    auto put16 = [&](quint16 v) {
        out.append(char(v >> 8)); out.append(char(v & 0xFF));
    };
    auto put32 = [&](quint32 v) {
        out.append(char(v >> 24)); out.append(char((v >> 16) & 0xFF));
        out.append(char((v >> 8) & 0xFF)); out.append(char(v & 0xFF));
    };

    out.append("PSID", 4);
    put16(2);
    put16(0x7C);
    put16(0);
    put16(initAddr);
    put16(playAddr);
    put16(1);
    put16(1);
    put32(0);

    auto putName = [&](const char *s) {
        char buf[32]; std::memset(buf, 0, 32);
        std::memcpy(buf, s, std::min<size_t>(31, std::strlen(s)));
        out.append(buf, 32);
    };
    putName("SidDAW export");
    putName("SidDAW");
    putName("2025");

    put16(0);
    out.append(char(0));
    out.append(char(0));
    out.append(char(0));
    out.append(char(0));

    out.append(char(loadAddr & 0xFF));
    out.append(char((loadAddr >> 8) & 0xFF));
    out.append(payload);
}

} // namespace

QString SidExporter::exportProject(const Project &project, int bpm,
                                    const QString &path) {
                                        
    int totalTicks = 0;
    for (const auto &c : project.clips)
        totalTicks = std::max(totalTicks, c.startTick + c.lengthTicks);
    if (totalTicks <= 0) return "Project is empty";

    QString playerPath = QCoreApplication::applicationDirPath() + "/player.bin";
    QFile pf(playerPath);
    if (!pf.open(QIODevice::ReadOnly))
        return "player.bin not found at: " + playerPath;
    QByteArray playerBin = pf.readAll();
    pf.close();
    if (playerBin.isEmpty()) return "player.bin is empty";

    QVector<FrameEvent> frames = buildFrames(project, bpm);
    if (frames.isEmpty()) return "No notes to export";
    QByteArray data = buildDataStream(frames, bpm, totalTicks);

    QByteArray payload = playerBin + data;

    QByteArray file;
    writePSID(file, payload, 0x1000, 0x1000, 0x1003);

    QFile out(path);
    if (!out.open(QIODevice::WriteOnly))
        return "Cannot open for write: " + path;
    out.write(file);
    out.close();

    int expectedSec = (totalTicks * 60) / (bpm * 4);
    int endFrameEst = int(std::round(double(totalTicks) * 50.0 * 15.0 / double(bpm)));
    qDebug("SID export: bpm=%d ticks=%d endFrame=%d events=%d data=%d bytes, expected ~%d sec",
           bpm, totalTicks, endFrameEst,
           int(frames.size()), int(data.size()), expectedSec);

    return {};
}
