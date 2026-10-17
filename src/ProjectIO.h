#pragma once
#include <QString>
#include "Model.h"

namespace ProjectIO {

// Сохраняет проект в JSON. Возвращает пустую строку при успехе.
QString save(const Project &project, int bpm, const QString &path);

// Загружает проект из JSON. Возвращает пустую строку при успехе.
QString load(Project &project, int &bpm, const QString &path);

} // namespace ProjectIO
