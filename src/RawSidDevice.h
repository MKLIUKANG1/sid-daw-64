#pragma once
#include <QIODevice>
#include <QVector>
#include "SidImporter.h"

class SidEngine;
class RawSidStream;

// QIODevice-обёртка вокруг SidEngine + RawSidStream.
// Питает QAudioSink сэмплами из точного SID-потока.
class RawSidDevice : public QIODevice {
    Q_OBJECT
public:
    explicit RawSidDevice(QObject *parent = nullptr);
    ~RawSidDevice();

    void setWrites(const QVector<SidRegisterWrite> &writes);
    void resetStream();
    int  currentFrame() const;

private:
    bool m_finishedEmitted = false;

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64) override { return 0; }
    bool   isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return 8192 + QIODevice::bytesAvailable(); }

signals:
    void finished();

private:
    SidEngine     *m_sid = nullptr;
    RawSidStream  *m_stream = nullptr;
};
