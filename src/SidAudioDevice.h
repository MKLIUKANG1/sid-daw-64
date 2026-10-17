#pragma once
#include <QIODevice>

class ArrangementPlayer;

class SidAudioDevice : public QIODevice {
    Q_OBJECT
public:
    explicit SidAudioDevice(ArrangementPlayer *player, QObject *parent = nullptr);

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *, qint64) override { return 0; }
    bool   isSequential() const override { return true; }
    qint64 bytesAvailable() const override { return 4096 + QIODevice::bytesAvailable(); }

private:
    ArrangementPlayer *m_player;
};
