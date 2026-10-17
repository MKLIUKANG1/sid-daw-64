#include "MidiRecorder.h"
#include <rtmidi/RtMidi.h>
#include <QDebug>

MidiRecorder::MidiRecorder(QObject *parent) : QObject(parent) {
    try {
        m_midiIn = new RtMidiIn();
    } catch (RtMidiError &e) {
        qWarning() << "RtMidi init error:"
                   << QString::fromStdString(e.getMessage());
    }
}

MidiRecorder::~MidiRecorder() {
    stop();
    delete m_midiIn;
}

QVector<QString> MidiRecorder::listPorts() {
    QVector<QString> result;
    try {
        RtMidiIn probe;
        unsigned int n = probe.getPortCount();
        for (unsigned int i = 0; i < n; ++i)
            result.append(QString::fromStdString(probe.getPortName(i)));
    } catch (RtMidiError &e) {
        qWarning() << "RtMidi port list error:"
                   << QString::fromStdString(e.getMessage());
    }
    return result;
}

bool MidiRecorder::start(int port) {
    if (!m_midiIn) return false;
    try {
        if (port < 0 || port >= (int)m_midiIn->getPortCount()) return false;
        m_midiIn->openPort(port);
        m_midiIn->setCallback(&MidiRecorder::midiCallback, this);
        m_midiIn->ignoreTypes(false, true, true); // sysex off, timing off, sensing off
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_activeNotes.clear();
            m_completed.clear();
        }
        m_recording.store(true);
        return true;
    } catch (RtMidiError &e) {
        qWarning() << "RtMidi open error:"
                   << QString::fromStdString(e.getMessage());
        return false;
    }
}

void MidiRecorder::stop() {
    if (m_midiIn && m_recording.load()) {
        try {
            m_midiIn->closePort();
            m_midiIn->cancelCallback();
        } catch (...) {}
    }
    m_recording.store(false);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeNotes.clear();
}

void MidiRecorder::midiCallback(double,
                                std::vector<unsigned char> *msg,
                                void *userData) {
    auto *self = static_cast<MidiRecorder*>(userData);
    if (self && self->m_recording.load() && msg && msg->size() >= 3)
        self->handleMessage(*msg);
}

void MidiRecorder::handleMessage(const std::vector<unsigned char> &msg) {
    unsigned char status = msg[0] & 0xF0;
    int pitch = msg[1];
    int vel   = msg[2];
    if (pitch > 127) return;

    std::lock_guard<std::mutex> lock(m_mutex);

    if (status == 0x90 && vel > 0) {
        ActiveNote an;
        an.tick     = int(m_playheadTick.load());
        an.velocity = vel;
        m_activeNotes[pitch] = an;
    } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
        if (m_activeNotes.contains(pitch)) {
            ActiveNote an = m_activeNotes.take(pitch);
            int endTick = int(m_playheadTick.load());
            int length  = endTick - an.tick;
            if (length < 1) length = 1;
            Note n;
            n.tick     = an.tick;
            n.pitch    = pitch;
            n.length   = length;
            n.velocity = qBound(1, an.velocity / 8, 15);
            n.voice    = m_recordVoice.load();
            m_completed.append(n);
        }
    }
}

QVector<Note> MidiRecorder::takeCompletedNotes() {
    std::lock_guard<std::mutex> lock(m_mutex);
    QVector<Note> r = m_completed;
    m_completed.clear();
    return r;
}
