#include "ProjectIO.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {

QJsonObject voiceToJson(const VoiceParams &vp) {
    QJsonObject o;
    o["waveform"]   = vp.waveform;
    o["pulseWidth"] = vp.pulseWidth;
    o["attack"]     = vp.attack;
    o["decay"]      = vp.decay;
    o["sustain"]    = vp.sustain;
    o["release"]    = vp.release;
    o["ringMod"]    = vp.ringMod;
    o["sync"]       = vp.sync;
    o["volume"]     = double(vp.volume);
    o["pan"]        = double(vp.pan);
    o["envelopeSpeed"] = vp.envelopeSpeed;

    QJsonArray envArr;
    for (int v : vp.volumeEnvelope) envArr.append(v);
    o["volumeEnvelope"] = envArr;
    return o;
}

VoiceParams voiceFromJson(const QJsonObject &o) {
    VoiceParams vp;
    vp.waveform   = o.value("waveform").toInt(1);
    vp.pulseWidth = o.value("pulseWidth").toInt(2048);
    vp.attack     = o.value("attack").toInt(1);
    vp.decay      = o.value("decay").toInt(0);
    vp.sustain    = o.value("sustain").toInt(15);
    vp.release    = o.value("release").toInt(3);
    vp.ringMod    = o.value("ringMod").toBool(false);
    vp.sync       = o.value("sync").toBool(false);
    vp.volume     = float(o.value("volume").toDouble(1.0));
    vp.pan        = float(o.value("pan").toDouble(0.0));
    vp.envelopeSpeed = o.value("envelopeSpeed").toInt(4);

    for (const auto &val : o.value("volumeEnvelope").toArray())
        vp.volumeEnvelope.append(val.toInt());
    return vp;
}

QJsonObject patternToJson(const Pattern &p) {
    QJsonObject o;
    o["name"]        = p.name;
    o["lengthTicks"] = p.lengthTicks;

    QJsonArray notes;
    for (const auto &n : p.notes) {
        QJsonObject no;
        no["tick"]     = n.tick;
        no["pitch"]    = n.pitch;
        no["length"]   = n.length;
        no["velocity"] = n.velocity;
        no["voice"]    = n.voice;
        notes.append(no);
    }
    o["notes"] = notes;

    QJsonArray voices;
    for (int v = 0; v < 3; ++v)
        voices.append(voiceToJson(p.voiceParams[v]));
    o["voices"] = voices;
    return o;
}

Pattern patternFromJson(const QJsonObject &o) {
    Pattern p;
    p.name        = o.value("name").toString("Pattern");
    p.lengthTicks = o.value("lengthTicks").toInt(64);
    for (const auto &nv : o.value("notes").toArray()) {
        QJsonObject no = nv.toObject();
        Note n;
        n.tick     = no.value("tick").toInt();
        n.pitch    = no.value("pitch").toInt(60);
        n.length   = no.value("length").toInt(4);
        n.velocity = no.value("velocity").toInt(12);
        n.voice    = no.value("voice").toInt(0);
        p.notes.append(n);
    }
    QJsonArray voices = o.value("voices").toArray();
    for (int v = 0; v < 3 && v < voices.size(); ++v)
        p.voiceParams[v] = voiceFromJson(voices[v].toObject());
    return p;
}

} // namespace

QString ProjectIO::save(const Project &project, int bpm, const QString &path) {
    QJsonObject root;
    root["format"] = "SidDAW-project";
    root["version"] = 1;
    root["bpm"] = bpm;

    QJsonArray patterns;
    for (const auto &p : project.patterns)
        patterns.append(patternToJson(*p));
    root["patterns"] = patterns;

    QJsonArray clips;
    for (const auto &c : project.clips) {
        QJsonObject co;
        co["patternIndex"] = c.patternIndex;
        co["track"]        = c.track;
        co["startTick"]    = c.startTick;
        co["lengthTicks"]  = c.lengthTicks;
        clips.append(co);
    }
    root["clips"] = clips;
    root["trackCount"] = project.trackCount;
    root["totalTicks"] = project.totalTicks;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return "Cannot open for write: " + path;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return {};
}

QString ProjectIO::load(Project &project, int &bpm, const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return "Cannot open: " + path;
    QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError)
        return "JSON parse error: " + err.errorString();
    if (!doc.isObject())
        return "Root is not an object";
    QJsonObject root = doc.object();
    if (root.value("format").toString() != "SidDAW-project")
        return "Not a SidDAW project file";

    bpm = root.value("bpm").toInt(120);

    Project fresh;
    fresh.trackCount = root.value("trackCount").toInt(4);
    fresh.totalTicks = root.value("totalTicks").toInt(256);

    for (const auto &pv : root.value("patterns").toArray()) {
        auto p = std::make_unique<Pattern>(patternFromJson(pv.toObject()));
        fresh.patterns.push_back(std::move(p));
    }
    if (fresh.patterns.empty())
        return "Project has no patterns";

    for (const auto &cv : root.value("clips").toArray()) {
        QJsonObject co = cv.toObject();
        Clip c;
        c.patternIndex = co.value("patternIndex").toInt(0);
        c.track        = co.value("track").toInt(0);
        c.startTick    = co.value("startTick").toInt(0);
        c.lengthTicks  = co.value("lengthTicks").toInt(64);
        fresh.clips.append(c);
    }

    project = std::move(fresh);
    return {};
}
