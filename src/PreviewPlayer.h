#pragma once
#include <QObject>
#include <QByteArray>
#include "Model.h"

class QAudioSink;
class QBuffer;
class QAudioFormat;

class PreviewPlayer : public QObject {
    Q_OBJECT
public:
    explicit PreviewPlayer(QObject *parent = nullptr);
    ~PreviewPlayer();

    // Проигрывает одну ноту с данными параметрами.
    // Не мешает основному воспроизведению (отдельный QAudioSink).
    void playNote(const VoiceParams &vp, int midiPitch = 60,
                  int durationMs = 800);

private:
    QByteArray renderPcm(const VoiceParams &vp, int midiPitch, int durationMs);

    QAudioSink *m_sink = nullptr;
    QBuffer    *m_buffer = nullptr;
    class QAudioFormat *m_fmt = nullptr;
};
