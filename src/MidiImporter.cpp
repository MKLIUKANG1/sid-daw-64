#include "MidiImporter.h"
#include <QFile>
#include <QDataStream>
#include <QHash>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

quint32 readVarLen(QDataStream &s, bool &ok) {
    quint32 value = 0;
    quint8 c;
    ok = false;
    for (int i = 0; i < 4; ++i) {
        s >> c;
        if (s.status() != QDataStream::Ok) return 0;
        value = (value << 7) | (c & 0x7F);
        if (!(c & 0x80)) { ok = true; return value; }
    }
    return value;
}

void processTrack(QDataStream &s, quint32 trackLen, int /*division*/,
                  QVector<RawNote> &outNotes, int &outBpm,
                  quint32 &maxTickMidi) {
    quint32 ticks = 0;
    quint8 runningStatus = 0;
    QHash<int, quint32> activeNotes; // key = channel*128 + pitch -> startTick

    QIODevice *dev = s.device();
    quint32 startPos = quint32(dev->pos());
    quint32 endPos   = startPos + trackLen;

    while (quint32(dev->pos()) < endPos) {
        bool ok = false;
        quint32 delta = readVarLen(s, ok);
        if (!ok) break;
        ticks += delta;
        if (ticks > maxTickMidi) maxTickMidi = ticks;

        quint8 status = 0;
        s >> status;
        if (s.status() != QDataStream::Ok) break;

        if (status < 0x80) {
            // Running status — этот байт уже данные
            status = runningStatus;
            dev->seek(dev->pos() - 1);
        } else {
            runningStatus = status;
        }

        quint8 type    = status & 0xF0;
        quint8 channel = status & 0x0F;

        if (status == 0xFF) {
            // Meta
            quint8 metaType = 0;
            s >> metaType;
            quint32 len = readVarLen(s, ok);
            if (!ok) break;
            if (metaType == 0x51 && len == 3) {
                quint8 b1 = 0, b2 = 0, b3 = 0;
                s >> b1 >> b2 >> b3;
                quint32 usPerQuarter = (quint32(b1) << 16) | (quint32(b2) << 8) | b3;
                if (usPerQuarter > 0) {
                    int bpm = int(std::round(60000000.0 / double(usPerQuarter)));
                    if (bpm >= 20 && bpm <= 400) outBpm = bpm;
                }
            } else {
                if (len > 0) s.skipRawData(int(len));
            }
        } else if (status == 0xF0 || status == 0xF7) {
            quint32 len = readVarLen(s, ok);
            if (!ok) break;
            if (len > 0) s.skipRawData(int(len));
        } else if (type == 0x80 || type == 0x90) {
            quint8 pitch = 0, vel = 0;
            s >> pitch >> vel;
            int key = channel * 128 + pitch;
            bool isOn = (type == 0x90 && vel > 0);
            if (isOn) {
                activeNotes[key] = ticks;
            } else if (activeNotes.contains(key)) {
                quint32 start = activeNotes.take(key);
                if (ticks > start) {
                    RawNote n;
                    n.tick     = int(start);
                    n.pitch    = pitch;
                    n.length   = int(ticks - start);
                    n.velocity = vel;
                    n.channel  = channel;
                    outNotes.append(n);
                }
            }
        } else if (type == 0xA0 || type == 0xB0 || type == 0xE0) {
            quint8 a = 0, b = 0;
            s >> a >> b;
        } else if (type == 0xC0 || type == 0xD0) {
            quint8 a = 0;
            s >> a;
        } else {
            break;
        }
    }

    // Закрываем «висящие» ноты (некоторые файлы не шлют note-off в конце)
    for (auto it = activeNotes.constBegin(); it != activeNotes.constEnd(); ++it) {
        int key = it.key();
        if (maxTickMidi > it.value()) {
            RawNote n;
            n.tick     = int(it.value());
            n.pitch    = key % 128;
            n.length   = int(maxTickMidi - it.value());
            n.velocity = 100;
            n.channel  = key / 128;
            outNotes.append(n);
        }
    }

    if (quint32(dev->pos()) < endPos)
        dev->seek(endPos);
}

} // namespace

QString MidiImporter::importFile(const QString &path,
                                 int &outBpm,
                                 int &outLengthTicks,
                                 QVector<RawNote> &outNotes) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return "Cannot open file: " + path;

    QDataStream s(&f);
    s.setByteOrder(QDataStream::BigEndian);

    char magic[4] = {};
    if (s.readRawData(magic, 4) != 4 || std::memcmp(magic, "MThd", 4) != 0)
        return "Not a MIDI file (no MThd header)";

    quint32 headerLen = 0;
    s >> headerLen;
    if (headerLen < 6) return "Invalid MIDI header length";

    quint16 format = 0, nTracks = 0, division = 0;
    s >> format >> nTracks >> division;

    if (division & 0x8000)
        return "SMPTE time division is not supported";
    if (division == 0)
        return "Invalid time division (0)";

    if (headerLen > 6) s.skipRawData(int(headerLen - 6));

    outBpm = 120;
    quint32 maxTickMidi = 0;
    QVector<RawNote> rawNotes;

    for (int t = 0; t < nTracks; ++t) {
        if (s.readRawData(magic, 4) != 4 || std::memcmp(magic, "MTrk", 4) != 0)
            return QString("Expected MTrk chunk (track %1)").arg(t);
        quint32 trackLen = 0;
        s >> trackLen;
        if (s.status() != QDataStream::Ok)
            return "Unexpected end of MIDI stream";
        processTrack(s, trackLen, division, rawNotes, outBpm, maxTickMidi);
    }

    // ---- Масштабируем тики MIDI в наши (4 тика на четверть) ----
    double scale = 4.0 / double(division);
    for (auto &n : rawNotes) {
        n.tick   = int(std::round(n.tick * scale));
        n.length = std::max(1, int(std::round(n.length * scale)));
    }

    // Сортировка: сначала по времени, потом по убыванию высоты (для аккордов)
    std::sort(rawNotes.begin(), rawNotes.end(),
              [](const RawNote &a, const RawNote &b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.pitch > b.pitch;
    });

    outNotes = rawNotes;
    outLengthTicks = int(std::round(maxTickMidi * scale));
    outLengthTicks = ((outLengthTicks + 15) / 16) * 16; // до полного такта
    if (outLengthTicks < 16) outLengthTicks = 16;

    return {};
}

void MidiImporter::assignToPattern(const QVector<RawNote> &rawNotes, Pattern &outPattern) {
    struct VoiceEnd { int endTick = -1; };
    VoiceEnd voices[3];

    for (const auto &rn : rawNotes) {
        int freeMask = 0;
        for (int v = 0; v < 3; ++v)
            if (voices[v].endTick <= rn.tick) freeMask |= (1 << v);

        int target = -1;
        if (freeMask != 0) {
            int preferred;
            if (rn.pitch >= 72)      preferred = 0; // высокое → lead
            else if (rn.pitch < 48)  preferred = 1; // низкое → bass
            else                     preferred = 2; // середина → harmony

            if (freeMask & (1 << preferred))
                target = preferred;
            else {
                for (int v = 0; v < 3; ++v)
                    if (freeMask & (1 << v)) { target = v; break; }
            }
        } else {
            // Все голоса заняты — отнимаем у того, кто освободится раньше
            target = 0;
            for (int v = 1; v < 3; ++v)
                if (voices[v].endTick < voices[target].endTick)
                    target = v;

            // Укорачиваем предыдущую ноту на этом голосе
            for (auto &n : outPattern.notes) {
                if (n.voice == target &&
                    n.tick < rn.tick &&
                    n.tick + n.length > rn.tick)
                    n.length = std::max(1, rn.tick - n.tick);
            }
        }

        Note n;
        n.tick     = rn.tick;
        n.pitch    = rn.pitch;
        n.length   = rn.length;
        n.velocity = std::clamp(rn.velocity / 8, 1, 15);
        n.voice    = target;
        outPattern.notes.append(n);

        voices[target].endTick = rn.tick + rn.length;
    }

    std::sort(outPattern.notes.begin(), outPattern.notes.end(),
              [](const Note &a, const Note &b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.pitch < b.pitch;
    });
}
