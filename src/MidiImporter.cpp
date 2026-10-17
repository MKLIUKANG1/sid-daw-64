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

void processTrack(QDataStream &s, quint32 trackLen,
                  QVector<RawNote> &outNotes, int &outBpm,
                  quint32 &maxTickMidi) {
    quint32 ticks = 0;
    quint8 runningStatus = 0;
    QHash<int, quint32> activeNotes;

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
            status = runningStatus;
            dev->seek(dev->pos() - 1);
        } else {
            runningStatus = status;
        }

        quint8 type    = status & 0xF0;
        quint8 channel = status & 0x0F;

        if (status == 0xFF) {
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
                                 QVector<RawNote> &outNotes,
                                 QVector<ChannelInfo> &outChannels) {
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
        processTrack(s, trackLen, rawNotes, outBpm, maxTickMidi);
    }

    double scale = 4.0 / double(division);
    for (auto &n : rawNotes) {
        n.tick   = int(std::round(n.tick * scale));
        n.length = std::max(1, int(std::round(n.length * scale)));
    }

    std::sort(rawNotes.begin(), rawNotes.end(),
              [](const RawNote &a, const RawNote &b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.pitch > b.pitch;
    });

    // ---- Собираем информацию по каналам ----
    QHash<int, ChannelInfo> chMap;
    for (const auto &n : rawNotes) {
        ChannelInfo &ci = chMap[n.channel];
        ci.channel = n.channel;
        ci.noteCount++;
        if (n.pitch < ci.minPitch) ci.minPitch = n.pitch;
        if (n.pitch > ci.maxPitch) ci.maxPitch = n.pitch;
    }
    outChannels.clear();
    QList<int> keys = chMap.keys();
    std::sort(keys.begin(), keys.end());
    for (int k : keys) outChannels.append(chMap[k]);

    outNotes = rawNotes;
    outLengthTicks = int(std::round(maxTickMidi * scale));
    outLengthTicks = ((outLengthTicks + 15) / 16) * 16;
    if (outLengthTicks < 16) outLengthTicks = 16;

    return {};
}

void MidiImporter::buildPatternFromSelection(const QVector<RawNote> &rawNotes,
                                             const QHash<int,int> &channelToVoice,
                                             Pattern &outPattern,
                                             const QHash<int,int> &channelToInstrument) {
    QVector<RawNote> selected;
    for (const auto &n : rawNotes) {
        auto it = channelToVoice.find(n.channel);
        if (it == channelToVoice.end() || it.value() < 0) continue;
        selected.append(n);
    }
    std::sort(selected.begin(), selected.end(),
              [](const RawNote &a, const RawNote &b) {
        return a.tick < b.tick;
    });

    int lastNoteIdx[3] = {-1, -1, -1};
    int lastNoteEnd[3] = {-1, -1, -1};
    bool drumUsed[3]   = {false, false, false};

    for (const auto &rn : selected) {
        int voice = channelToVoice.value(rn.channel, -1);
        if (voice < 0 || voice > 2) continue;

        if (rn.channel == 9) drumUsed[voice] = true;

        if (lastNoteEnd[voice] > rn.tick) {
            int prevIdx = lastNoteIdx[voice];
            if (prevIdx >= 0 && prevIdx < outPattern.notes.size()) {
                Note &prev = outPattern.notes[prevIdx];
                prev.length = std::max(1, rn.tick - prev.tick);
            }
        }

        Note n;
        n.tick     = rn.tick;
        n.pitch    = rn.pitch;
        n.length   = rn.length;
        n.velocity = std::clamp(rn.velocity / 8, 1, 15);
        n.voice    = voice;
        n.instrumentIndex = channelToInstrument.value(rn.channel, -1);

        int newIdx = outPattern.notes.size();
        outPattern.notes.append(n);
        lastNoteIdx[voice] = newIdx;
        lastNoteEnd[voice] = rn.tick + rn.length;
    }

    // Fallback: если инструмент для drum-канала не задан —
    // ставим VoiceParams напрямую в Noise (сработает для нот без инструмента).
    for (int v = 0; v < 3; ++v) {
        if (!drumUsed[v]) continue;
        VoiceParams &vp = outPattern.voiceParams[v];
        vp.waveform      = 3;
        vp.attack        = 0;
        vp.decay         = 4;
        vp.sustain       = 0;
        vp.release       = 2;
        vp.volumeEnvelope.clear();
    }

    std::sort(outPattern.notes.begin(), outPattern.notes.end(),
              [](const Note &a, const Note &b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.pitch < b.pitch;
    });
}
