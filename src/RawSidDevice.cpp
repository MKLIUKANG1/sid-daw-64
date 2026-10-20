#include "RawSidDevice.h"
#include "SidEngine.h"
#include "RawSidStream.h"

RawSidDevice::RawSidDevice(QObject *parent) : QIODevice(parent) {
    m_sid = new SidEngine;
    m_sid->setSampleRate(44100);
    m_stream = new RawSidStream(m_sid);
}

RawSidDevice::~RawSidDevice() {
    delete m_stream;
    delete m_sid;
}

void RawSidDevice::setWrites(const QVector<SidRegisterWrite> &writes) {
    if (m_stream) m_stream->setWrites(writes);
}

void RawSidDevice::resetStream() {
    if (m_stream) m_stream->reset();
    if (m_sid) m_sid->reset();
}

qint64 RawSidDevice::readData(char *data, qint64 maxlen) {
    if (!m_stream) return 0;
    int frames = int(maxlen / 2);
    if (frames <= 0) return 0;

    m_stream->render(reinterpret_cast<int16_t*>(data), frames);

    // Emit finished() only once, via QueuedConnection to avoid
    // re-entrancy (slot deletes this object while readData still on stack).
    if (m_stream->isFinished() && !m_finishedEmitted) {
        m_finishedEmitted = true;
        QMetaObject::invokeMethod(this, [this]() { emit finished(); },
                                  Qt::QueuedConnection);
    }

    return frames * 2;
}

int RawSidDevice::currentFrame() const {
    return m_stream ? m_stream->currentFrame() : 0;
}
