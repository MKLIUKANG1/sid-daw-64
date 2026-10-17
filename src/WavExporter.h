#pragma once
#include <QString>
#include "Model.h"

namespace WavExporter {
    QString exportProject(const Project &project,
                          int activePatternIndex,
                          int bpm,
                          const QString &path);
}
