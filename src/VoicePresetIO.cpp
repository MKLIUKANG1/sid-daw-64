#include "VoicePresetIO.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QString VoicePresetIO::save(const VoiceParams &vp, const QString &path) {
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

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return "Cannot open for write: " + path;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    f.close();
    return {};
}

QString VoicePresetIO::load(VoiceParams &vp, const QString &path) {
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
        return "Preset root is not an object";

    QJsonObject o = doc.object();
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

    vp.volumeEnvelope.clear();
    for (const auto &val : o.value("volumeEnvelope").toArray())
        vp.volumeEnvelope.append(val.toInt());

    return {};
}
