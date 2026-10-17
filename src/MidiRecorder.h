#pragma once
#include <QObject>
#include <QVector>
#include <QHash>
#include <QString>
#include <mutex>
#include <atomic>
#include <vector>
#include "Model.h"

class RtMidiIn;

class MidiRecorder : public QObject {
    Q_OBJECT
public:
    explicit MidiRecorder(QObject *parent = nullptr);
    ~MidiRecorder();

    // Список доступных MIDI-портов (без открытия)
    QVector<QString> listPorts();

    bool start(int port);
    void stop();
    bool isRecording() const { return m_recording.load(); }

    void setPlayheadTick(double tick) { m_playheadTick.store(tick); }
    void setRecordVoice(int v)        { m_recordVoice.store(v); }

    // Забрать завершённые ноты (note-on + note-off). Вызывается из GUI-потока.
    QVector<Note> takeCompletedNotes();

private:
    static void midiCallback(double delta,
                             std::vector<unsigned char> *msg,
                             void *userData);
    void handleMessage(const std::vector<unsigned char> &msg);

    RtMidiIn *m_midiIn = nullptr;

    std::atomic<bool>   m_recording{false};
    std::atomic<double> m_playheadTick{0.0};
    std::atomic<int>    m_recordVoice{0};

    struct ActiveNote { int tick = 0; int velocity = 0; };
    std::mutex            m_mutex;
    QHash<int, ActiveNote> m_activeNotes;
    QVector<Note>          m_completed;
};
