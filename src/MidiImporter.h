#pragma once
#include <QString>
#include <QVector>
#include <QHash>
#include "Model.h"

struct RawNote {
    int tick     = 0;
    int pitch    = 60;
    int length   = 1;
    int velocity = 100;
    int channel  = 0;
};

struct ChannelInfo {
    int channel   = 0;
    int noteCount = 0;
    int minPitch  = 127;
    int maxPitch  = 0;
};

namespace MidiImporter {

QString importFile(const QString &path,
                   int &outBpm,
                   int &outLengthTicks,
                   QVector<RawNote> &outNotes,
                   QVector<ChannelInfo> &outChannels,
                   QHash<int,int> *outChannelPrograms = nullptr);

void buildPatternFromSelection(const QVector<RawNote> &rawNotes,
                               const QHash<int,int> &channelToVoice,
                               Pattern &outPattern,
                               const QHash<int,int> &channelToInstrument = {},
                               const QHash<int,int> &drumPitchToInstrument = {});

} // namespace MidiImporter
