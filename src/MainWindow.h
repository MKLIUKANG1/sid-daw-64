#pragma once
#include <QMainWindow>
#include "Model.h"

class PianoRollWidget;
class ArrangementWidget;
class VoicePanelWidget;
class MidiRecorder;
class SidEngine;
class ArrangementPlayer;
class SidAudioDevice;
class QTimer;
class QSpinBox;
class QComboBox;
class QLabel;
class QPushButton;
class QAudioSink;
class QScrollArea;
class QSlider;
class QUndoStack;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onPlayPause();
    void onStop();
    void onTick();
    void onExportWav();
    void onExportSid();
    void onImportMidi();
    void onDuplicatePattern();
    void onRenamePattern();
    void onDeletePattern();
    void onSaveProject();
    void onLoadProject();
    void onToggleRecord();
    void onMidiPoll();
    void onRefreshMidiPorts();

private:
    void buildDemoContent();
    void updatePosLabel();
    void refreshPatternCombo();
    void refreshAllUI();

    Project            m_project;
    PianoRollWidget   *m_pianoRoll    = nullptr;
    ArrangementWidget *m_arrangement  = nullptr;
    VoicePanelWidget  *m_voicePanels[3] = {nullptr, nullptr, nullptr};
    QScrollArea       *m_pianoScroll   = nullptr;
    QScrollArea       *m_arrangeScroll = nullptr;
    SidEngine         *m_sidEngine    = nullptr;
    ArrangementPlayer *m_player       = nullptr;
    SidAudioDevice    *m_audioDevice  = nullptr;
    QAudioSink        *m_audioSink    = nullptr;
    QTimer            *m_timer        = nullptr;
    QTimer            *m_midiPollTimer = nullptr;
    QSpinBox          *m_bpmSpin      = nullptr;
    QComboBox         *m_patternBox   = nullptr;
    QComboBox         *m_midiPortBox  = nullptr;
    QComboBox         *m_recVoiceBox  = nullptr;
    QPushButton       *m_playBtn      = nullptr;
    QPushButton       *m_recBtn       = nullptr;
    QLabel            *m_posLabel     = nullptr;
    QSlider           *m_masterVol    = nullptr;
    QUndoStack        *m_undoStack    = nullptr;
    MidiRecorder      *m_recorder     = nullptr;

    int    m_activePattern = 0;
    double m_playheadTick  = 0.0;
    bool   m_playing       = false;
    bool   m_recording     = false;
};
