#pragma once
#include <QString>
#include <QVector>
#include "Model.h"

namespace DrumPresets {

enum Type {
    Kick, Snare, HiHatClosed, HiHatPedal, HiHatOpen,
    Crash, Ride, TomLow, TomMid, TomHigh, Clap, Cowbell, Generic
};

Type typeForPitch(int midiPitch);
QString nameForPitch(int midiPitch);
Instrument instrumentForPitch(int midiPitch);
QVector<DrumSlot> defaultKitSlots();

} // namespace DrumPresets
