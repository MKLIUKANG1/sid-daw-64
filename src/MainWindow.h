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
class InstrumentListWidget;
class QTimer;
class QSpinBox;
class QComboBox;
class QLabel;
class QPushButton;
class QAudioSink;
class QScrollArea;
class QSlider;
class QUndoStack;
class QMenu;
class QAction;

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
    void onInstrumentChanged(int idx);
    void onNewInstrument();
    void onEditInstrument();
    void onDeleteInstrument();
    void onAddPatternToTrack(int track, int startTick);
    void onRequestNoteDialog(const QVector<int> &indices);
    void onInstrumentDropped(const QVector<int> &indices, int instrumentIndex);
    void onAbout();
    void onLanguageSelected(int lang);
    void retranslateUi();

private:
    void buildDemoContent();
    void updatePosLabel();
    void refreshPatternCombo();
    void refreshInstrumentCombo();
    void refreshInstrumentList();
    void refreshAllUI();
    void recomputeProjectLength();

    Project            m_project;
    PianoRollWidget   *m_pianoRoll    = nullptr;
    ArrangementWidget *m_arrangement  = nullptr;
    VoicePanelWidget  *m_voicePanels[3] = {nullptr, nullptr, nullptr};
    InstrumentListWidget *m_instrList = nullptr;
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
    QComboBox         *m_instrCombo   = nullptr;
    QPushButton       *m_playBtn      = nullptr;
    QPushButton       *m_recBtn       = nullptr;
    QPushButton       *m_stopBtn      = nullptr;
    QLabel            *m_posLabel     = nullptr;
    QSlider           *m_masterVol    = nullptr;
    QUndoStack        *m_undoStack    = nullptr;
    MidiRecorder      *m_recorder     = nullptr;

    // Language menu
    QMenu             *m_fileMenu     = nullptr;
    QMenu             *m_editMenu     = nullptr;
    QMenu             *m_languageMenu = nullptr;
    QMenu             *m_helpMenu     = nullptr;
    QAction           *m_actAbout     = nullptr;
    QAction           *m_actUndo      = nullptr;
    QAction           *m_actRedo      = nullptr;
    QAction           *m_langActions[4] = {nullptr, nullptr, nullptr, nullptr};

    int    m_activePattern = 0;
    int    m_currentInstrument = -1;
    double m_playheadTick  = 0.0;
    bool   m_playing       = false;
    bool   m_recording     = false;
};
