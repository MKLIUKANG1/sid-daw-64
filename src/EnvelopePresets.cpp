#include "EnvelopePresets.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {
QString presetsFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/envelopes.json";
}
}

QVector<QPair<QString, QVector<int>>> EnvelopePresets::builtIn() {
    QVector<QPair<QString, QVector<int>>> out;
    auto add = [&](const QString &n, QVector<int> v) { out.append({n, v}); };
    add("Flat 15",     {15,15,15,15,15,15,15,15});
    add("Flat 8",      {8,8,8,8,8,8,8,8});
    add("Attack",      {0,2,4,6,8,10,12,15});
    add("Decay",       {15,12,10,8,6,4,2,0});
    add("Adsr Short",  {0,15,8,4,4,4,4,0});
    add("Vibrato",     {8,9,10,9,8,7,6,7});
    add("Ping Pong",   {0,15,0,15,0,15,0,15});
    add("Slow Attack", {0,1,2,3,4,6,8,10,12,15});
    add("Pulse Up",    {4,10,4,10,4,10,4,10});
    return out;
}

QMap<QString, QVector<int>> EnvelopePresets::userPresets() {
    QMap<QString, QVector<int>> result;
    QFile f(presetsFilePath());
    if (!f.open(QIODevice::ReadOnly)) return result;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return result;
    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        QVector<int> env;
        for (const auto &v : it.value().toArray())
            env.append(v.toInt());
        result[it.key()] = env;
    }
    return result;
}

bool EnvelopePresets::saveUserPreset(const QString &name, const QVector<int> &env) {
    if (name.isEmpty()) return false;
    auto presets = userPresets();
    presets[name] = env;
    QJsonObject root;
    for (auto it = presets.begin(); it != presets.end(); ++it) {
        QJsonArray arr;
        for (int v : it.value()) arr.append(v);
        root[it.key()] = arr;
    }
    QFile f(presetsFilePath());
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

void EnvelopePresets::removeUserPreset(const QString &name) {
    auto presets = userPresets();
    presets.remove(name);
    QJsonObject root;
    for (auto it = presets.begin(); it != presets.end(); ++it) {
        QJsonArray arr;
        for (int v : it.value()) arr.append(v);
        root[it.key()] = arr;
    }
    QFile f(presetsFilePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    }
}
