#include "DrumPresets.h"

namespace DrumPresets {

Type typeForPitch(int p) {
    if (p == 35 || p == 36) return Kick;
    if (p == 37 || p == 38 || p == 40) return Snare;
    if (p == 39) return Clap;
    if (p == 41 || p == 43) return TomLow;
    if (p == 42) return HiHatClosed;
    if (p == 44) return HiHatPedal;
    if (p == 45 || p == 47) return TomMid;
    if (p == 46) return HiHatOpen;
    if (p == 48 || p == 50) return TomHigh;
    if (p == 49 || p == 52 || p == 55 || p == 57) return Crash;
    if (p == 51 || p == 53 || p == 59) return Ride;
    if (p == 56) return Cowbell;
    return Generic;
}

QString nameForPitch(int p) {
    switch (typeForPitch(p)) {
        case Kick:        return "Kick";
        case Snare:       return "Snare";
        case HiHatClosed: return "Hi-Hat Closed";
        case HiHatPedal:  return "Hi-Hat Pedal";
        case HiHatOpen:   return "Hi-Hat Open";
        case Crash:       return "Crash";
        case Ride:        return "Ride";
        case TomLow:      return "Tom Low";
        case TomMid:      return "Tom Mid";
        case TomHigh:     return "Tom High";
        case Clap:        return "Clap";
        case Cowbell:     return "Cowbell";
        default:          return QString("Perc %1").arg(p);
    }
}

Instrument instrumentForPitch(int p) {
    Instrument instr;
    instr.name = nameForPitch(p);
    VoiceParams &vp = instr.params;
    switch (typeForPitch(p)) {
        case Kick:
            vp.waveform = 3; vp.attack = 0; vp.decay = 3;
            vp.sustain = 0; vp.release = 1; break;
        case Snare:
            vp.waveform = 3; vp.attack = 0; vp.decay = 4;
            vp.sustain = 0; vp.release = 2; break;
        case HiHatClosed:
            vp.waveform = 3; vp.attack = 0; vp.decay = 1;
            vp.sustain = 0; vp.release = 1; break;
        case HiHatPedal:
            vp.waveform = 3; vp.attack = 0; vp.decay = 2;
            vp.sustain = 0; vp.release = 1; break;
        case HiHatOpen:
            vp.waveform = 3; vp.attack = 0; vp.decay = 5;
            vp.sustain = 0; vp.release = 3; break;
        case Crash:
            vp.waveform = 3; vp.attack = 1; vp.decay = 7;
            vp.sustain = 0; vp.release = 4; break;
        case Ride:
            vp.waveform = 3; vp.attack = 0; vp.decay = 4;
            vp.sustain = 1; vp.release = 2; break;
        case TomLow:
        case TomMid:
        case TomHigh:
            vp.waveform = 0; vp.attack = 0; vp.decay = 4;
            vp.sustain = 0; vp.release = 2; break;
        case Clap:
            vp.waveform = 3; vp.attack = 0; vp.decay = 2;
            vp.sustain = 0; vp.release = 1; break;
        case Cowbell:
            vp.waveform = 2; vp.pulseWidth = 1024;
            vp.attack = 0; vp.decay = 3; vp.sustain = 2; vp.release = 2; break;
        default:
            vp.waveform = 3; vp.attack = 0; vp.decay = 3;
            vp.sustain = 0; vp.release = 2; break;
    }
    return instr;
}

QVector<DrumSlot> defaultKitSlots() {
    QVector<DrumSlot> result;
    QVector<int> commonPitches = {36, 38, 39, 41, 42, 44, 45, 46, 48, 49, 51, 56};
    for (int p : commonPitches) {
        DrumSlot s;
        s.midiPitch = p;
        s.name = nameForPitch(p);
        s.instrumentIndex = -1;
        result.append(s);
    }
    return result;
}

} // namespace DrumPresets
