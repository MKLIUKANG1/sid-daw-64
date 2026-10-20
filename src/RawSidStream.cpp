#include "RawSidStream.h"
#include "SidEngine.h"
#include <algorithm>

RawSidStream::RawSidStream(SidEngine *sid) : m_sid(sid) {
    if (m_sid) {
        m_sampleRate = double(m_sid->sampleRate());
        m_samplesPerFrame = m_sampleRate / 50.0;
    }
}

void RawSidStream::setWrites(const QVector<SidRegisterWrite> &writes) {
    m_writesByFrame.clear();
    m_totalFrames = 0;

    for (const auto &w : writes) {
        if (w.frame < 0) continue;
        while (m_writesByFrame.size() <= w.frame)
            m_writesByFrame.append(QVector<QPair<int,int>>());
        m_writesByFrame[w.frame].append({w.reg, w.value});
        if (w.frame + 1 > m_totalFrames) m_totalFrames = w.frame + 1;
    }
    m_frameIndex = 0;
    m_sampleAccum = 0.0;
    m_firstFrameApplied = false;
}

void RawSidStream::applyFrameWrites(int frame) {
    if (!m_sid) return;
    if (frame < 0 || frame >= m_writesByFrame.size()) return;
    for (const auto &w : m_writesByFrame[frame])
        m_sid->write(w.first, w.second);
}

void RawSidStream::reset() {
    m_frameIndex = 0;
    m_sampleAccum = 0.0;
    m_firstFrameApplied = false;
    if (m_sid) m_sid->reset();
}

void RawSidStream::render(int16_t *out, int frames) {
    if (!m_sid) {
        std::fill(out, out + frames, int16_t(0));
        return;
    }

    // Применить кадр 0 перед первым сэмплом
    if (!m_firstFrameApplied) {
        applyFrameWrites(0);
        m_firstFrameApplied = true;
    }

    int offset = 0;
    while (offset < frames) {
        if (m_frameIndex >= m_totalFrames) {
            std::fill(out + offset, out + frames, int16_t(0));
            return;
        }

        int samplesLeftInFrame = int(m_samplesPerFrame - m_sampleAccum);
        if (samplesLeftInFrame <= 0) {
            ++m_frameIndex;
            m_sampleAccum -= m_samplesPerFrame;
            if (m_frameIndex < m_totalFrames)
                applyFrameWrites(m_frameIndex);
            continue;
        }

        int toRender = std::min(samplesLeftInFrame, frames - offset);
        for (int i = 0; i < toRender; ++i)
            m_sid->clock(1, out + offset + i);

        m_sampleAccum += toRender;
        offset += toRender;
    }
}
