#pragma once
#include <QVector>
#include <QPair>
#include <cstdint>
#include "SidImporter.h"

class SidEngine;

// Точное воспроизведение потока регистровых записей SID.
// Регистры применяются 50 раз в секунду (PAL VBI), как в оригинале.
class RawSidStream {
public:
    explicit RawSidStream(SidEngine *sid);

    void setWrites(const QVector<SidRegisterWrite> &writes);
    void reset();
    void render(int16_t *out, int frames);

    bool isFinished() const { return m_frameIndex >= m_totalFrames; }
    int  totalFrames() const { return m_totalFrames; }
    int  currentFrame() const { return m_frameIndex; }
    int  currentSecond() const { return m_frameIndex / 50; }

private:
    void applyFrameWrites(int frame);

    SidEngine *m_sid = nullptr;
    QVector<QVector<QPair<int,int>>> m_writesByFrame;
    int    m_totalFrames = 0;
    int    m_frameIndex = 0;
    double m_sampleRate = 44100.0;
    double m_samplesPerFrame = 882.0;
    double m_sampleAccum = 0.0;
    bool   m_firstFrameApplied = false;
};
