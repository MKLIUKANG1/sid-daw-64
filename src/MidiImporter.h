#pragma once
#include <QString>
#include <QVector>
#include "Model.h"

struct RawNote {
    int tick     = 0;
    int pitch    = 60;
    int length   = 1;
    int velocity = 100;
    int channel  = 0;
};

namespace MidiImporter {

// Парсит .mid файл. Заполняет outBpm, outLengthTicks (в наших тиках,
// 4 тика на четверть) и outNotes. Возвращает пустую строку при успехе.
QString importFile(const QString &path,
                   int &outBpm,
                   int &outLengthTicks,
                   QVector<RawNote> &outNotes);

// Раскладывает RawNote по трём голосам SID и заполняет Pattern.
void assignToPattern(const QVector<RawNote> &rawNotes, Pattern &outPattern);

} // namespace MidiImporter
