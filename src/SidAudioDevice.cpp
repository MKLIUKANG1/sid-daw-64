#include "SidAudioDevice.h"
#include "ArrangementPlayer.h"

SidAudioDevice::SidAudioDevice(ArrangementPlayer *player, QObject *parent)
    : QIODevice(parent), m_player(player) {}

qint64 SidAudioDevice::readData(char *data, qint64 maxlen) {
    int frames = int(maxlen / 2);
    if (frames <= 0 || !m_player) return 0;
    m_player->render(reinterpret_cast<int16_t*>(data), frames);
    return frames * 2;
}
