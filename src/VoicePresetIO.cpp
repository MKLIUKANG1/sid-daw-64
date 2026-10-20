#include "VoicePresetIO.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

static QJsonArray vecToJson(const QVector<int> &v) {
    QJsonArray a;
    for (int x : v) a.append(x);
    return a;
}
static QVector<int> jsonToVec(const QJsonArray &a) {
    QVector<int> v;
    for (const auto &x : a) v.append(x.toInt());
    return v;
}

QString VoicePresetIO::save(const VoiceParams &vp, const QString &path) {
    QJsonObject o;
    o["waveform"] = vp.waveform;
    o["pulseWidth"] = vp.pulseWidth;
    o["attack"] = vp.attack; o["decay"] = vp.decay;
    o["sustain"] = vp.sustain; o["release"] = vp.release;
    o["ringMod"] = vp.ringMod; o["sync"] = vp.sync;
    o["volume"] = double(vp.volume); o["pan"] = double(vp.pan);
    o["envelopeSpeed"] = vp.envelopeSpeed;
    o["pitchEnvelopeAbsolute"] = vp.pitchEnvelopeAbsolute;
    o["volumeEnvelope"]   = vecToJson(vp.volumeEnvelope);
    o["pitchEnvelope"]    = vecToJson(vp.pitchEnvelope);
    o["dutyEnvelope"]     = vecToJson(vp.dutyEnvelope);
    o["arpeggioEnvelope"] = vecToJson(vp.arpeggioEnvelope);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return "Cannot open for write: " + path;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.close();
    return {};
}

QString VoicePresetIO::load(VoiceParams &vp, const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return "Cannot open: " + path;
    QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return "JSON parse error: " + err.errorString();
    if (!doc.isObject()) return "Preset root is not an object";

    QJsonObject o = doc.object();
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
    vp.volumeEnvelope   = jsonToVec(o.value("volumeEnvelope").toArray());
    vp.pitchEnvelope    = jsonToVec(o.value("pitchEnvelope").toArray());
    vp.dutyEnvelope     = jsonToVec(o.value("dutyEnvelope").toArray());
    vp.arpeggioEnvelope = jsonToVec(o.value("arpeggioEnvelope").toArray());
    return {};
}
