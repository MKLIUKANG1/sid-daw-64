#include "WavExporter.h"
#include "SidEngine.h"
#include "PatternPlayer.h"
#include <QFile>
#include <QDataStream>
#include <QVector>
#include <algorithm>

namespace {

void writeWav(const QString &path, const QByteArray &pcm,
              int sr, int channels, int bps) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;

    quint32 dataSize  = pcm.size();
    quint32 chunkSize = 36 + dataSize;
    quint16 nch       = channels;
    quint32 sampleRate = sr;
    quint32 byteRate   = sr * nch * bps / 8;
    quint16 blockAlign = nch * bps / 8;
    quint16 bitsPer    = bps;
    quint32 fmtSize    = 16;
    quint16 audioFmt   = 1;

    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);

    f.write("RIFF", 4);  ds << chunkSize;
    f.write("WAVE", 4);
    f.write("fmt ", 4);  ds << fmtSize;
    ds << audioFmt << nch << sampleRate << byteRate << blockAlign << bitsPer;
    f.write("data", 4);  ds << dataSize;
    f.write(pcm);
    f.close();
}

} // namespace

QString WavExporter::exportProject(const Project &project,
                                   int activePatternIndex,
                                   int bpm,
                                   const QString &path) {
    int totalTicks = 0;
    for (const auto &c : project.clips)
        totalTicks = std::max(totalTicks, c.startTick + c.lengthTicks);
    if (totalTicks <= 0)
        return "Project is empty (no clips)";

    QVector<Note> flat;
    for (const auto &c : project.clips) {
        if (c.patternIndex < 0 || c.patternIndex >= (int)project.patterns.size())
            continue;
        const Pattern *pat = project.patterns[c.patternIndex].get();
        if (pat->lengthTicks <= 0) continue;
        int reps = (c.lengthTicks + pat->lengthTicks - 1) / pat->lengthTicks;
        for (int r = 0; r < reps; ++r) {
            for (const auto &n : pat->notes) {
                Note copy = n;
                copy.tick = c.startTick + r * pat->lengthTicks + n.tick;
                int clipEnd = c.startTick + c.lengthTicks;
                if (copy.tick >= clipEnd) continue;
                if (copy.tick + copy.length > clipEnd)
                    copy.length = clipEnd - copy.tick;
                if (copy.length > 0) flat.append(copy);
            }
        }
    }
    if (flat.isEmpty()) return "No notes to render";

    std::sort(flat.begin(), flat.end(),
              [](const Note &a, const Note &b) { return a.tick < b.tick; });

    Pattern tmp;
    tmp.lengthTicks = totalTicks + 4;
    tmp.notes       = flat;
    if (activePatternIndex >= 0 && activePatternIndex < (int)project.patterns.size()) {
        const Pattern *src = project.patterns[activePatternIndex].get();
        for (int v = 0; v < 3; ++v) tmp.voiceParams[v] = src->voiceParams[v];
    }

    const int sampleRate = 44100;
    SidEngine sid;
    sid.setSampleRate(sampleRate);
    PatternPlayer player(&sid);
    player.setProject(&project);
    player.setPattern(&tmp);
    player.setBpm(bpm);

    double ticksPerSec = bpm / 15.0;
    double totalSec    = double(totalTicks) / ticksPerSec + 0.5;
    int totalSamples   = int(totalSec * sampleRate);
    if (totalSamples <= 0) return "Computed length is zero";

    QByteArray pcm(totalSamples * 2, 0);
    player.render(reinterpret_cast<int16_t*>(pcm.data()), totalSamples);

    writeWav(path, pcm, sampleRate, 1, 16);
    return {};
}
