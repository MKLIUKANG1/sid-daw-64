#pragma once
#include <QString>
#include "Model.h"

namespace SidExporter {
    QString exportProject(const Project &project, int bpm, const QString &path);
}
