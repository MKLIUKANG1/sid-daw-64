#pragma once
#include <QString>
#include <QVector>
#include <cstdint>
#include "Model.h"

struct SidHeader {
    quint32 magic       = 0;
    quint16 version     = 0;
    quint16 dataOffset  = 0x76;
    quint16 loadAddress = 0;
    quint16 initAddress = 0;
    quint16 playAddress = 0;
    quint16 songs       = 1;
    quint16 startSong   = 1;
    quint32 speed       = 0;
    QString name;
    QString author;
    QString released;
};

struct SidRegisterWrite {
    int frame = 0;
    int reg   = 0;
    int value = 0;
};

namespace SidImporter {

QString parseHeader(const QByteArray &data, SidHeader &out);

// Автономный импорт (без VICE): встроенный 6502-эмулятор
QString dumpRegistersEmbedded(const QString &sidPath, int maxSeconds,
                              QVector<SidRegisterWrite> &outWrites,
                              SidGlobal *outGlobal = nullptr);

QString buildPatternFromWrites(const QVector<SidRegisterWrite> &writes,
                               Pattern &outPattern,
                               QVector<Instrument> &outInstruments);

} // namespace SidImporter
