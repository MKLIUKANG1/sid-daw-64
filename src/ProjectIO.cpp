#include "ProjectIO.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {

QJsonArray vecToJson(const QVector<int> &v) {
    QJsonArray a;
    for (int x : v) a.append(x);
    return a;
}
QVector<int> jsonToVec(const QJsonArray &a) {
    QVector<int> v;
    for (const auto &x : a) v.append(x.toInt());
    return v;
}

QJsonObject voiceToJson(const VoiceParams &vp) {
    QJsonObject o;
    o["waveform"] = vp.waveform;
    o["pulseWidth"] = vp.pulseWidth;
    o["attack"] = vp.attack;
    o["decay"] = vp.decay;
    o["sustain"] = vp.sustain;
    o["release"] = vp.release;
    o["ringMod"] = vp.ringMod;
    o["sync"] = vp.sync;
    o["volume"] = double(vp.volume);
    o["pan"] = double(vp.pan);
    o["envelopeSpeed"] = vp.envelopeSpeed;
    o["pitchEnvelopeAbsolute"] = vp.pitchEnvelopeAbsolute;
    o["volumeEnvelope"] = vecToJson(vp.volumeEnvelope);
    o["pitchEnvelope"] = vecToJson(vp.pitchEnvelope);
    o["dutyEnvelope"] = vecToJson(vp.dutyEnvelope);
    o["arpeggioEnvelope"] = vecToJson(vp.arpeggioEnvelope);
    return o;
}

VoiceParams voiceFromJson(const QJsonObject &o) {
    VoiceParams vp;
    vp.waveform = o.value("waveform").toInt(1);
    vp.pulseWidth = o.value("pulseWidth").toInt(2048);
    vp.attack = o.value("attack").toInt(1);
    vp.decay = o.value("decay").toInt(0);
    vp.sustain = o.value("sustain").toInt(15);
    vp.release = o.value("release").toInt(3);
    vp.ringMod = o.value("ringMod").toBool(false);
    vp.sync = o.value("sync").toBool(false);
    vp.volume = float(o.value("volume").toDouble(1.0));
    vp.pan = float(o.value("pan").toDouble(0.0));
    vp.envelopeSpeed = o.value("envelopeSpeed").toInt(4);
    vp.pitchEnvelopeAbsolute = o.value("pitchEnvelopeAbsolute").toBool(false);
    vp.volumeEnvelope = jsonToVec(o.value("volumeEnvelope").toArray());
    vp.pitchEnvelope = jsonToVec(o.value("pitchEnvelope").toArray());
    vp.dutyEnvelope = jsonToVec(o.value("dutyEnvelope").toArray());
    vp.arpeggioEnvelope = jsonToVec(o.value("arpeggioEnvelope").toArray());
    return vp;
}

QJsonObject patternToJson(const Pattern &p) {
    QJsonObject o;
    o["name"] = p.name;
    o["lengthTicks"] = p.lengthTicks;
    o["isDrumPattern"] = p.isDrumPattern;
    QJsonArray notes;
    for (const auto &n : p.notes) {
        QJsonObject no;
        no["tick"] = n.tick; no["pitch"] = n.pitch;
        no["length"] = n.length; no["velocity"] = n.velocity;
        no["voice"] = n.voice; no["instrumentIndex"] = n.instrumentIndex;
        notes.append(no);
    }
    o["notes"] = notes;
    QJsonArray voices;
    for (int v = 0; v < 3; ++v) voices.append(voiceToJson(p.voiceParams[v]));
    o["voices"] = voices;
    return o;
}

Pattern patternFromJson(const QJsonObject &o) {
    Pattern p;
    p.name = o.value("name").toString("Pattern");
    p.lengthTicks = o.value("lengthTicks").toInt(64);
    for (const auto &nv : o.value("notes").toArray()) {
        QJsonObject no = nv.toObject();
        Note n;
        n.tick = no.value("tick").toInt();
        n.pitch = no.value("pitch").toInt(60);
        n.length = no.value("length").toInt(4);
        n.velocity = no.value("velocity").toInt(12);
        n.voice = no.value("voice").toInt(0);
        n.instrumentIndex = no.value("instrumentIndex").toInt(-1);
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
    root["version"] = 4;
    root["bpm"] = bpm;

    QJsonArray patterns;
    for (const auto &p : project.patterns) patterns.append(patternToJson(*p));
    root["patterns"] = patterns;

    QJsonArray instruments;
    for (const auto &instr : project.instruments) {
        QJsonObject io;
        io["name"] = instr->name;
        io["params"] = voiceToJson(instr->params);
        instruments.append(io);
    }
    root["instruments"] = instruments;

    QJsonObject drumKit;
    drumKit["name"] = project.drumKit.name;
    QJsonArray slotsArr;
    for (const auto &s : project.drumKit.entries) {
        QJsonObject so;
        so["midiPitch"] = s.midiPitch;
        so["name"] = s.name;
        so["instrumentIndex"] = s.instrumentIndex;
        slotsArr.append(so);
    }
    drumKit["slots"] = slotsArr;
    root["drumKit"] = drumKit;

    QJsonArray clips;
    for (const auto &c : project.clips) {
        QJsonObject co;
        co["patternIndex"] = c.patternIndex;
        co["track"] = c.track;
        co["startTick"] = c.startTick;
        co["lengthTicks"] = c.lengthTicks;
        clips.append(co);
    }
    root["clips"] = clips;
    QJsonObject sidG;
    sidG["filterCutoff"]    = project.sidGlobal.filterCutoff;
    sidG["filterResonance"] = project.sidGlobal.filterResonance;
    sidG["filterMode"]      = project.sidGlobal.filterMode;
    sidG["filterEnabled"]   = project.sidGlobal.filterEnabled;
    sidG["masterVolume"]    = project.sidGlobal.masterVolume;
    sidG["filterVoices"]    = project.sidGlobal.filterVoices;
    root["sidGlobal"] = sidG;

    root["trackCount"] = project.trackCount;
    root["totalTicks"] = project.totalTicks;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return "Cannot open for write: " + path;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return {};
}

QString ProjectIO::load(Project &project, int &bpm, const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return "Cannot open: " + path;
    QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return "JSON parse error: " + err.errorString();
    if (!doc.isObject()) return "Root is not an object";
    QJsonObject root = doc.object();
    if (root.value("format").toString() != "SidDAW-project")
        return "Not a SidDAW project file";

    bpm = root.value("bpm").toInt(120);

    Project fresh;
    fresh.trackCount = root.value("trackCount").toInt(4);
    fresh.totalTicks = root.value("totalTicks").toInt(256);

    QJsonObject sidG = root.value("sidGlobal").toObject();
    if (!sidG.isEmpty()) {
        fresh.sidGlobal.filterCutoff    = sidG.value("filterCutoff").toInt(1024);
        fresh.sidGlobal.filterResonance = sidG.value("filterResonance").toInt(8);
        fresh.sidGlobal.filterMode      = sidG.value("filterMode").toInt(0);
        fresh.sidGlobal.filterEnabled   = sidG.value("filterEnabled").toBool(true);
        fresh.sidGlobal.masterVolume    = sidG.value("masterVolume").toInt(15);
        fresh.sidGlobal.filterVoices    = sidG.value("filterVoices").toInt(0x07);
    }

    for (const auto &pv : root.value("patterns").toArray()) {
        auto p = std::make_unique<Pattern>(patternFromJson(pv.toObject()));
        fresh.patterns.push_back(std::move(p));
    }
    if (fresh.patterns.empty()) return "Project has no patterns";

    for (const auto &iv : root.value("instruments").toArray()) {
        QJsonObject io = iv.toObject();
        auto instr = std::make_unique<Instrument>();
        instr->name = io.value("name").toString("Instrument");
        instr->params = voiceFromJson(io.value("params").toObject());
        fresh.instruments.push_back(std::move(instr));
    }

    QJsonObject dk = root.value("drumKit").toObject();
    fresh.drumKit.name = dk.value("name").toString("Drum Kit");
    for (const auto &sv : dk.value("slots").toArray()) {
        QJsonObject so = sv.toObject();
        DrumSlot s;
        s.midiPitch = so.value("midiPitch").toInt(36);
        s.name = so.value("name").toString("Drum");
        s.instrumentIndex = so.value("instrumentIndex").toInt(-1);
        fresh.drumKit.entries.append(s);
    }

    for (const auto &cv : root.value("clips").toArray()) {
        QJsonObject co = cv.toObject();
        Clip c;
        c.patternIndex = co.value("patternIndex").toInt(0);
        c.track = co.value("track").toInt(0);
        c.startTick = co.value("startTick").toInt(0);
        c.lengthTicks = co.value("lengthTicks").toInt(64);
        fresh.clips.append(c);
    }

    project = std::move(fresh);
    return {};
}
