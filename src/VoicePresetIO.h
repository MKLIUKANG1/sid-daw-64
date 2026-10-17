#pragma once
#include <QString>
#include "Model.h"

namespace VoicePresetIO {
    QString save(const VoiceParams &vp, const QString &path);
    QString load(VoiceParams &vp, const QString &path);
}
