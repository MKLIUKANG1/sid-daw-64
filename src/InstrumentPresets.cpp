#include "InstrumentPresets.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ===== Anonymous helpers (defined BEFORE namespace usage) =====
namespace {

VoiceParams makeVP(int wave, int pw, int a, int d, int s, int r,
                   bool ring = false, bool sync = false) {
    VoiceParams vp;
    vp.waveform = wave;
    vp.pulseWidth = pw;
    vp.attack = a;
    vp.decay = d;
    vp.sustain = s;
    vp.release = r;
    vp.ringMod = ring;
    vp.sync = sync;
    return vp;
}

QString instrumentsFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/instruments.json";
}

QJsonObject vpToJson(const VoiceParams &vp) {
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
    QJsonArray a1, a2, a3, a4;
    for (int v : vp.volumeEnvelope) a1.append(v);
    for (int v : vp.pitchEnvelope) a2.append(v);
    for (int v : vp.dutyEnvelope) a3.append(v);
    for (int v : vp.arpeggioEnvelope) a4.append(v);
    o["volumeEnvelope"] = a1;
    o["pitchEnvelope"] = a2;
    o["dutyEnvelope"] = a3;
    o["arpeggioEnvelope"] = a4;
    return o;
}

VoiceParams vpFromJson(const QJsonObject &o) {
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
    for (const auto &v : o.value("volumeEnvelope").toArray()) vp.volumeEnvelope.append(v.toInt());
    for (const auto &v : o.value("pitchEnvelope").toArray()) vp.pitchEnvelope.append(v.toInt());
    for (const auto &v : o.value("dutyEnvelope").toArray()) vp.dutyEnvelope.append(v.toInt());
    for (const auto &v : o.value("arpeggioEnvelope").toArray()) vp.arpeggioEnvelope.append(v.toInt());
    return vp;
}

} // anonymous namespace

// ===== InstrumentPresets namespace =====
namespace InstrumentPresets {

QVector<InstrumentPreset> builtIn() {
    QVector<InstrumentPreset> result;
    auto add = [&](const QString &name, const QString &cat, VoiceParams vp) {
        InstrumentPreset p;
        p.name = name;
        p.category = cat;
        p.params = vp;
        result.append(p);
    };

    // BASS (triangle)
    add("Bass — Sub",        "Bass", makeVP(0, 2048, 0, 3, 15, 2));
    add("Bass — Pluck",      "Bass", makeVP(0, 2048, 0, 5, 6, 3));
    add("Bass — Staccato",   "Bass", makeVP(0, 2048, 0, 4, 0, 2));
    add("Bass — Saw",        "Bass", makeVP(1, 2048, 1, 4, 12, 3));

    // LEAD
    add("Lead — Saw Bright", "Lead", makeVP(1, 2048, 0, 2, 15, 3));
    add("Lead — Saw Soft",   "Lead", makeVP(1, 2048, 3, 4, 13, 5));
    add("Lead — Square 50%", "Lead", makeVP(2, 2048, 1, 2, 13, 3));
    add("Lead — Square Thin","Lead", makeVP(2, 1024, 0, 2, 14, 3));
    add("Lead — PWM Wide",   "Lead", makeVP(2, 3072, 2, 3, 12, 4));
    add("Lead — Bell",       "Lead", makeVP(1, 2048, 0, 6, 0, 5));
    add("Lead — Sync",       "Lead", makeVP(1, 2048, 0, 3, 12, 3, false, true));
    add("Lead — Ring Metal", "Lead", makeVP(2, 2048, 0, 3, 10, 3, true, false));

    // PAD
    add("Pad — Soft Strings","Pad",  makeVP(1, 2048, 5, 6, 14, 7));
    add("Pad — Warm",        "Pad",  makeVP(1, 2048, 6, 7, 15, 8));
    add("Pad — Choir",       "Pad",  makeVP(0, 2048, 7, 7, 14, 8));
    add("Pad — Pulse Slow",  "Pad",  makeVP(2, 1536, 5, 6, 13, 8));

    // PLUCK
    add("Pluck — Short",     "Pluck", makeVP(1, 2048, 0, 3, 0, 2));
    add("Pluck — Long",      "Pluck", makeVP(1, 2048, 1, 5, 4, 4));
    add("Pluck — Triangle",  "Pluck", makeVP(0, 2048, 0, 3, 2, 2));

    // KEYS
    add("Keys — Piano",      "Keys", makeVP(2, 2048, 0, 6, 3, 5));
    add("Keys — Harpsichord","Keys", makeVP(2, 1024, 0, 4, 0, 3));
    add("Keys — Organ",      "Keys", makeVP(2, 1024, 1, 2, 15, 2));

    // DRUMS
    add("Drum — Kick",       "Drum", makeVP(3, 2048, 0, 3, 0, 1));
    add("Drum — Snare",      "Drum", makeVP(3, 2048, 0, 4, 0, 2));
    add("Drum — HiHat Cl",   "Drum", makeVP(3, 2048, 0, 1, 0, 1));
    add("Drum — HiHat Op",   "Drum", makeVP(3, 2048, 0, 5, 0, 3));
    add("Drum — Crash",      "Drum", makeVP(3, 2048, 1, 7, 0, 4));
    add("Drum — Tom",        "Drum", makeVP(0, 2048, 0, 4, 0, 2));
    add("Drum — Rim",        "Drum", makeVP(3, 2048, 0, 2, 0, 1));

    return result;
}

QMap<QString, VoiceParams> userPresets() {
    QMap<QString, VoiceParams> result;
    QFile f(instrumentsFilePath());
    if (!f.open(QIODevice::ReadOnly)) return result;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return result;
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it)
        result[it.key()] = vpFromJson(it.value().toObject());
    return result;
}

QVector<InstrumentPreset> all() {
    QVector<InstrumentPreset> result = builtIn();
    auto user = userPresets();
    for (auto it = user.begin(); it != user.end(); ++it) {
        InstrumentPreset p;
        p.name = it.key();
        p.category = "User";
        p.params = it.value();
        result.append(p);
    }
    return result;
}

bool saveUserPreset(const QString &name, const VoiceParams &vp) {
    if (name.isEmpty()) return false;
    auto presets = userPresets();
    presets[name] = vp;
    QJsonObject root;
    for (auto it = presets.begin(); it != presets.end(); ++it)
        root[it.key()] = vpToJson(it.value());
    QFile f(instrumentsFilePath());
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

void removeUserPreset(const QString &name) {
    auto presets = userPresets();
    presets.remove(name);
    QJsonObject root;
    for (auto it = presets.begin(); it != presets.end(); ++it)
        root[it.key()] = vpToJson(it.value());
    QFile f(instrumentsFilePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    }
}

} // namespace InstrumentPresets
