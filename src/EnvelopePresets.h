#pragma once
#include <QString>
#include <QVector>
#include <QMap>

namespace EnvelopePresets {
    QVector<QPair<QString, QVector<int>>> builtIn();
    QMap<QString, QVector<int>> userPresets();
    bool saveUserPreset(const QString &name, const QVector<int> &env);
    void removeUserPreset(const QString &name);
}
