#include "SidExporter.h"
#include "SidEngine.h"
#include <QFile>
#include <QCoreApplication>
#include <QVector>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace {

struct FrameEvent {
    int frame = 0;
    QVector<QPair<int,int>> writes;
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
    struct Flat { int tick, pitch, length, voice; const VoiceParams *params; };
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
                f.tick   = c.startTick + r * pat->lengthTicks + n.tick;
                f.pitch  = n.pitch;
                f.length = n.length;
                f.voice  = n.voice;
                f.params = &pat->voiceParams[std::clamp(n.voice, 0, 2)];
                int clipEnd = c.startTick + c.lengthTicks;
                if (f.tick >= clipEnd) continue;
                if (f.tick + f.length > clipEnd) f.length = clipEnd - f.tick;
                if (f.length > 0) flat.append(f);
            }
        }
    }

    auto tickToFrame = [&](int t) {
        return int(std::round(double(t) * FPS * 15.0 / double(bpm)));
    };

    QVector<FrameEvent> events;
    for (const auto &n : flat) {
        int v = std::clamp(n.voice, 0, 2);
        int base = v * 7;
        const VoiceParams &vp = *n.params;

        double f = noteFreq(n.pitch);
        uint32_t freq = uint32_t(std::round(f * 16777216.0 / CLOCK)) & 0xFFFF;
        uint8_t ctrlOn  = composeControl(vp, true);
        uint8_t ctrlOff = composeControl(vp, false);

        FrameEvent on;
        on.frame = tickToFrame(n.tick);
        on.writes.append({base + 0, int(freq & 0xFF)});
        on.writes.append({base + 1, int((freq >> 8) & 0xFF)});
        on.writes.append({base + 2, vp.pulseWidth & 0xFF});
        on.writes.append({base + 3, (vp.pulseWidth >> 8) & 0x0F});
        on.writes.append({base + 5, ((vp.attack & 0xF) << 4) | (vp.decay & 0xF)});
        on.writes.append({base + 6, ((vp.sustain & 0xF) << 4) | (vp.release & 0xF)});
        on.writes.append({base + 4, ctrlOn});
        events.append(on);

        FrameEvent off;
        off.frame = tickToFrame(n.tick + n.length);
        off.writes.append({base + 4, ctrlOff});
        events.append(off);
    }

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
    return merged;
}

QByteArray buildDataStream(const QVector<FrameEvent> &merged, int bpm, int totalTicks) {
    QByteArray d;
    if (merged.isEmpty()) return d;

    d.append(char(0));
    int firstFrame = merged[0].frame;
    d.append(char(firstFrame & 0xFF));
    d.append(char((firstFrame >> 8) & 0xFF));

    int endFrame = int(std::round(double(totalTicks) * FPS * 15.0 / double(bpm)));

    for (int i = 0; i < merged.size(); ++i) {
        const auto &e = merged[i];
        int delay;
        if (i + 1 < merged.size())
            delay = merged[i + 1].frame - e.frame;
        else
            delay = std::max(1, endFrame - e.frame) + 50;

        d.append(char(e.writes.size()));
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
    putName("2024");

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
    return {};
}
