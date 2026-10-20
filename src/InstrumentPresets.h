#pragma once
#include <QString>
#include <QVector>
#include <QMap>
#include "Model.h"

struct InstrumentPreset {
    QString name;
    QString category;
    VoiceParams params;
};

namespace InstrumentPresets {
    QVector<InstrumentPreset> builtIn();
    QMap<QString, VoiceParams> userPresets();
    QVector<InstrumentPreset> all();
    bool saveUserPreset(const QString &name, const VoiceParams &vp);
    void removeUserPreset(const QString &name);
}
