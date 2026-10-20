#pragma once
#include <QString>
#include <QMap>
#include "Model.h"

class MidiPresetManager {
public:
    MidiPresetManager();

    VoiceParams getParamsForMidiProgram(int programNumber) const;
    static QString nameForProgram(int programNumber);
    VoiceParams getDefaultParameters() const;
    bool loadFromJson(const QString &path);

private:
    void buildEmbeddedTable();
    QMap<int, VoiceParams> m_presets;
};
