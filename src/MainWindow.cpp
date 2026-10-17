#include "MainWindow.h"
#include "PianoRollWidget.h"
#include "ArrangementWidget.h"
#include "VoicePanelWidget.h"
#include "InstrumentEditorDialog.h"
#include "InstrumentListWidget.h"
#include "NotePropertiesDialog.h"
#include "SidEngine.h"
#include "ArrangementPlayer.h"
#include "SidAudioDevice.h"
#include "WavExporter.h"
#include "MidiImporter.h"
#include "MidiImportDialog.h"
#include "ProjectIO.h"
#include "SidExporter.h"
#include "MidiRecorder.h"
#include "Commands.h"
#include "version.h"
#include "LanguageManager.h"
#include "AboutDialog.h"
#include <QSettings>

#include <QMenuBar>
#include <QToolBar>
#include <QSplitter>
#include <QDockWidget>
#include <QListWidget>
#include <QLabel>
#include <QFrame>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTimer>
#include <QTabWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>
#include <QAction>
#include <QUndoStack>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <cstring>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildDemoContent();
    m_undoStack = new QUndoStack(this);
    m_recorder = new MidiRecorder(this);

    setWindowTitle(QString("SID DAW 64 v%1 - prototype").arg(SIDDAW_VERSION));
    resize(1500, 900);

    // ===== File menu =====
    m_fileMenu = menuBar()->addMenu(LanguageManager::tr("File"));
    auto *actOpen = m_fileMenu->addAction(LanguageManager::tr("Open Project..."));
    connect(actOpen, &QAction::triggered, this, &MainWindow::onLoadProject);
    auto *actSave = m_fileMenu->addAction(LanguageManager::tr("Save Project As..."));
    connect(actSave, &QAction::triggered, this, &MainWindow::onSaveProject);
    m_fileMenu->addSeparator();
    auto *actImp = m_fileMenu->addAction(LanguageManager::tr("Import MIDI..."));
    connect(actImp, &QAction::triggered, this, &MainWindow::onImportMidi);
    m_fileMenu->addSeparator();
    auto *actSid = m_fileMenu->addAction(LanguageManager::tr("Export SID..."));
    connect(actSid, &QAction::triggered, this, &MainWindow::onExportSid);
    auto *actWav = m_fileMenu->addAction(LanguageManager::tr("Export WAV..."));
    connect(actWav, &QAction::triggered, this, &MainWindow::onExportWav);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(LanguageManager::tr("Quit"), this, &QWidget::close);

    // ===== Edit menu =====
    m_editMenu = menuBar()->addMenu(LanguageManager::tr("Edit"));
    m_actUndo = new QAction(LanguageManager::tr("Undo"), this);
    m_actUndo->setShortcut(QKeySequence::Undo);
    connect(m_actUndo, &QAction::triggered, this, [this]() {
        m_undoStack->undo(); refreshAllUI();
    });
    connect(m_undoStack, &QUndoStack::canUndoChanged, m_actUndo, &QAction::setEnabled);
    m_actUndo->setEnabled(false);
    m_editMenu->addAction(m_actUndo);
    m_actRedo = new QAction(LanguageManager::tr("Redo"), this);
    m_actRedo->setShortcuts({QKeySequence::Redo, QKeySequence("Ctrl+Shift+Z")});
    connect(m_actRedo, &QAction::triggered, this, [this]() {
        m_undoStack->redo(); refreshAllUI();
    });
    connect(m_undoStack, &QUndoStack::canRedoChanged, m_actRedo, &QAction::setEnabled);
    m_actRedo->setEnabled(false);
    m_editMenu->addAction(m_actRedo);

    // ===== Language menu =====
    m_languageMenu = menuBar()->addMenu(LanguageManager::tr("Language"));
    auto langs = LanguageManager::availableLanguages();
    for (int i = 0; i < langs.size(); ++i) {
        int langId = int(langs[i]);
        auto *act = m_languageMenu->addAction(LanguageManager::languageName(langs[i]));
        act->setCheckable(true);
        act->setChecked(langId == int(LanguageManager::instance().current()));
        connect(act, &QAction::triggered, this, [this, langId]() { onLanguageSelected(langId); });
        if (langId >= 0 && langId < 4) m_langActions[langId] = act;
    }

    // ===== Help menu =====
    m_helpMenu = menuBar()->addMenu(LanguageManager::tr("Help"));
    m_actAbout = m_helpMenu->addAction(LanguageManager::tr("About"));
    connect(m_actAbout, &QAction::triggered, this, &MainWindow::onAbout);

    // ===== Toolbar Main =====
    auto *tb = addToolBar("Main");
    m_playBtn = new QPushButton(LanguageManager::tr("Play"));
    tb->addWidget(m_playBtn);
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    m_stopBtn = new QPushButton(LanguageManager::tr("Stop"));
    tb->addWidget(m_stopBtn);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);
    tb->addSeparator();
    tb->addWidget(new QLabel(" " + LanguageManager::tr("BPM:") + " "));
    m_bpmSpin = new QSpinBox;
    m_bpmSpin->setRange(40, 300);
    m_bpmSpin->setValue(120);
    tb->addWidget(m_bpmSpin);
    tb->addSeparator();
    tb->addWidget(new QLabel(" " + LanguageManager::tr("Pattern:") + " "));
    m_patternBox = new QComboBox;
    for (size_t i = 0; i < m_project.patterns.size(); ++i)
        m_patternBox->addItem(m_project.patterns[i]->name, int(i));
    tb->addWidget(m_patternBox);
    auto *renameBtn = new QPushButton(LanguageManager::tr("Rename"));
    tb->addWidget(renameBtn);
    connect(renameBtn, &QPushButton::clicked, this, &MainWindow::onRenamePattern);
    auto *dupBtn = new QPushButton(LanguageManager::tr("Duplicate"));
    tb->addWidget(dupBtn);
    connect(dupBtn, &QPushButton::clicked, this, &MainWindow::onDuplicatePattern);
    auto *delBtn = new QPushButton(LanguageManager::tr("Delete"));
    tb->addWidget(delBtn);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::onDeletePattern);
    tb->addSeparator();
    tb->addWidget(new QLabel(" " + LanguageManager::tr("Master:") + " "));
    m_masterVol = new QSlider(Qt::Horizontal);
    m_masterVol->setRange(0, 100);
    m_masterVol->setValue(100);
    m_masterVol->setFixedWidth(100);
    tb->addWidget(m_masterVol);
    tb->addSeparator();
    m_posLabel = new QLabel("Bar 1.1.0");
    m_posLabel->setStyleSheet("color: #ddd; padding: 0 8px;");
    tb->addWidget(m_posLabel);

    // ===== Toolbar MIDI =====
    auto *recBar = addToolBar("MIDI");
    m_recBtn = new QPushButton(LanguageManager::tr("Record"));
    recBar->addWidget(m_recBtn);
    connect(m_recBtn, &QPushButton::clicked, this, &MainWindow::onToggleRecord);
    recBar->addSeparator();
    recBar->addWidget(new QLabel(" " + LanguageManager::tr("MIDI In:") + " "));
    m_midiPortBox = new QComboBox;
    m_midiPortBox->setMinimumWidth(200);
    recBar->addWidget(m_midiPortBox);
    auto *refreshBtn = new QPushButton(LanguageManager::tr("Refresh"));
    recBar->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshMidiPorts);
    recBar->addSeparator();
    recBar->addWidget(new QLabel(" " + LanguageManager::tr("Rec Voice:") + " "));
    m_recVoiceBox = new QComboBox;
    m_recVoiceBox->addItem(LanguageManager::tr("Voice 1 (lead)"), 0);
    m_recVoiceBox->addItem(LanguageManager::tr("Voice 2 (bass)"), 1);
    m_recVoiceBox->addItem(LanguageManager::tr("Voice 3 (harmony)"), 2);
    recBar->addWidget(m_recVoiceBox);
    onRefreshMidiPorts();

    // ===== Toolbar Instruments =====
    auto *instBar = addToolBar("Instruments");
    instBar->addWidget(new QLabel(" " + LanguageManager::tr("Instrument:") + " "));
    m_instrCombo = new QComboBox;
    m_instrCombo->setMinimumWidth(180);
    instBar->addWidget(m_instrCombo);
    connect(m_instrCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onInstrumentChanged);
    auto *newInstBtn = new QPushButton(LanguageManager::tr("New"));
    instBar->addWidget(newInstBtn);
    connect(newInstBtn, &QPushButton::clicked, this, &MainWindow::onNewInstrument);
    auto *editInstBtn = new QPushButton(LanguageManager::tr("Edit..."));
    instBar->addWidget(editInstBtn);
    connect(editInstBtn, &QPushButton::clicked, this, &MainWindow::onEditInstrument);
    auto *delInstBtn = new QPushButton(LanguageManager::tr("Delete"));
    instBar->addWidget(delInstBtn);
    connect(delInstBtn, &QPushButton::clicked, this, &MainWindow::onDeleteInstrument);

    // Dock with instrument list
    auto *instrDock = new QDockWidget(LanguageManager::tr("Instruments (drag to note)"), this);
    m_instrList = new InstrumentListWidget;
    m_instrList->setMinimumWidth(160);
    instrDock->setWidget(m_instrList);
    addDockWidget(Qt::RightDockWidgetArea, instrDock);

    // ===== Splitter =====
    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_arrangement = new ArrangementWidget;
    m_arrangement->setProject(&m_project);
    m_arrangement->setActivePattern(0);
    m_arrangement->setUndoStack(m_undoStack);
    m_arrangeScroll = new QScrollArea;
    m_arrangeScroll->setWidget(m_arrangement);
    m_arrangeScroll->setWidgetResizable(true);
    m_arrangeScroll->setFrameShape(QFrame::NoFrame);
    splitter->addWidget(m_arrangeScroll);
    connect(m_arrangement, &ArrangementWidget::panBy, this, [this](int dx, int dy) {
        if (!m_arrangeScroll) return;
        auto *h = m_arrangeScroll->horizontalScrollBar();
        auto *v = m_arrangeScroll->verticalScrollBar();
        h->setValue(h->value() + dx);
        v->setValue(v->value() + dy);
    });
    connect(m_arrangement, &ArrangementWidget::addPatternRequested,
            this, &MainWindow::onAddPatternToTrack);

    auto *bottomSplit = new QSplitter(Qt::Horizontal);
    m_pianoRoll = new PianoRollWidget;
    m_pianoRoll->setPattern(m_project.patterns[0].get());
    m_pianoRoll->setProject(&m_project);
    m_pianoRoll->setUndoStack(m_undoStack);
    m_pianoScroll = new QScrollArea;
    m_pianoScroll->setWidget(m_pianoRoll);
    m_pianoScroll->setWidgetResizable(true);
    m_pianoScroll->setFrameShape(QFrame::NoFrame);
    bottomSplit->addWidget(m_pianoScroll);
    connect(m_pianoRoll, &PianoRollWidget::panBy, this, [this](int dx, int dy) {
        if (!m_pianoScroll) return;
        auto *h = m_pianoScroll->horizontalScrollBar();
        auto *v = m_pianoScroll->verticalScrollBar();
        h->setValue(h->value() + dx);
        v->setValue(v->value() + dy);
    });
    connect(m_pianoRoll, &PianoRollWidget::noteInstrumentChanged,
            this, [this](int idx) {
        int comboIdx = idx + 1;
        if (comboIdx >= 0 && comboIdx < m_instrCombo->count())
            m_instrCombo->setCurrentIndex(comboIdx);
    });
    connect(m_pianoRoll, &PianoRollWidget::requestNoteDialog,
            this, &MainWindow::onRequestNoteDialog);
    connect(m_pianoRoll, &PianoRollWidget::instrumentDropped,
            this, &MainWindow::onInstrumentDropped);

    auto *tabs = new QTabWidget;
    for (int v = 0; v < 3; ++v) {
        m_voicePanels[v] = new VoicePanelWidget;
        m_voicePanels[v]->setPattern(m_project.patterns[0].get(), v);
        m_voicePanels[v]->setUndoStack(m_undoStack);
        tabs->addTab(m_voicePanels[v], LanguageManager::tr(QString("Voice %1").arg(v + 1)));
    }
    tabs->setMaximumWidth(280);
    bottomSplit->addWidget(tabs);
    bottomSplit->setStretchFactor(0, 1);
    bottomSplit->setStretchFactor(1, 0);
    splitter->addWidget(bottomSplit);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);

    // ===== Audio =====
    m_sidEngine = new SidEngine;
    m_sidEngine->setSampleRate(44100);
    m_player = new ArrangementPlayer(m_sidEngine);
    m_player->setProject(&m_project);
    m_player->setBpm(120);
    m_audioDevice = new SidAudioDevice(m_player, this);
    m_audioDevice->open(QIODevice::ReadOnly);
    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice outDev = QMediaDevices::defaultAudioOutput();
    if (!outDev.isFormatSupported(fmt)) {
        fmt = outDev.preferredFormat();
        fmt.setSampleRate(44100);
        fmt.setChannelCount(1);
        fmt.setSampleFormat(QAudioFormat::Int16);
    }
    m_audioSink = new QAudioSink(outDev, fmt, this);
    m_audioSink->setBufferSize(16384);
    connect(m_masterVol, &QSlider::valueChanged, this, [this](int v) {
        if (m_player) m_player->setMasterVolume(v / 100.0f);
    });

    connect(m_patternBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (idx < 0 || idx >= (int)m_project.patterns.size()) return;
        m_activePattern = idx;
        Pattern *pat = m_project.patterns[idx].get();
        m_pianoRoll->setPattern(pat);
        m_arrangement->setActivePattern(idx);
        for (int v = 0; v < 3; ++v)
            if (m_voicePanels[v]) m_voicePanels[v]->setPattern(pat, v);
    });
    connect(m_arrangement, &ArrangementWidget::seekRequested,
            this, [this](double t) {
        m_playheadTick = t;
        if (m_player) m_player->setPlayheadTick(t);
        if (m_sidEngine) m_sidEngine->reset();
        if (m_player) m_player->resetVoices();
        updatePosLabel();
    });
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTick);
    m_midiPollTimer = new QTimer(this);
    m_midiPollTimer->setInterval(16);
    connect(m_midiPollTimer, &QTimer::timeout, this, &MainWindow::onMidiPoll);

    refreshInstrumentCombo();
    refreshInstrumentList();
    statusBar()->showMessage("Ctrl+Z undo | Ctrl+wheel zoom | Middle-mouse pan/note dialog | F: note properties");
    updatePosLabel();
}

void MainWindow::buildDemoContent() {
    auto p1 = std::make_unique<Pattern>();
    p1->name = "Pattern 1";
    p1->lengthTicks = 64;
    const int lead[16] = {72, 76, 79, 84, 79, 76, 72, 76, 79, 84, 88, 84, 79, 76, 72, 67};
    for (int i = 0; i < 16; ++i) {
        Note n; n.tick = i*4; n.pitch = lead[i]; n.length = 3; n.voice = 0;
        p1->notes.append(n);
    }
    const int bass[8] = {36, 36, 43, 43, 36, 36, 43, 43};
    for (int i = 0; i < 8; ++i) {
        Note n; n.tick = i*8; n.pitch = bass[i]; n.length = 7; n.voice = 1;
        p1->notes.append(n);
    }
    const int harm[8] = {60, 64, 67, 72, 67, 64, 60, 55};
    for (int i = 0; i < 8; ++i) {
        Note n; n.tick = i*8 + 4; n.pitch = harm[i]; n.length = 3; n.voice = 2;
        p1->notes.append(n);
    }
    m_project.patterns.push_back(std::move(p1));
    Clip c;
    c.patternIndex = 0;
    c.track = 0; c.startTick = 0;   c.lengthTicks = 64; m_project.clips.append(c);
    c.track = 1; c.startTick = 64;  c.lengthTicks = 64; m_project.clips.append(c);
    c.track = 2; c.startTick = 128; c.lengthTicks = 64; m_project.clips.append(c);
}

void MainWindow::refreshPatternCombo() {
    m_patternBox->blockSignals(true);
    m_patternBox->clear();
    for (size_t i = 0; i < m_project.patterns.size(); ++i)
        m_patternBox->addItem(m_project.patterns[i]->name, int(i));
    m_patternBox->setCurrentIndex(m_activePattern);
    m_patternBox->blockSignals(false);
}

void MainWindow::refreshInstrumentCombo() {
    m_instrCombo->blockSignals(true);
    m_instrCombo->clear();
    m_instrCombo->addItem("Default (Voice 1)", -1);
    for (size_t i = 0; i < m_project.instruments.size(); ++i)
        m_instrCombo->addItem(m_project.instruments[i]->name, int(i));
    int target = m_currentInstrument;
    int idx = 0;
    for (int i = 0; i < m_instrCombo->count(); ++i)
        if (m_instrCombo->itemData(i).toInt() == target) { idx = i; break; }
    m_instrCombo->setCurrentIndex(idx);
    m_instrCombo->blockSignals(false);
    m_pianoRoll->setCurrentInstrument(target);
}

void MainWindow::refreshInstrumentList() {
    if (!m_instrList) return;
    m_instrList->clear();
    auto *defItem = new QListWidgetItem("Default (voice params)");
    defItem->setData(Qt::UserRole, -1);
    defItem->setForeground(QColor(160, 160, 160));
    m_instrList->addItem(defItem);

    static const QColor palette[8] = {
        QColor( 80, 160, 255), QColor(120, 220, 120),
        QColor(240, 160,  80), QColor(210, 120, 200),
        QColor( 90, 200, 200), QColor(255, 220, 100),
        QColor(200, 140, 255), QColor(150, 200, 130),
    };
    for (size_t i = 0; i < m_project.instruments.size(); ++i) {
        auto *item = new QListWidgetItem(m_project.instruments[i]->name);
        item->setData(Qt::UserRole, int(i));
        item->setForeground(palette[i % 8]);
        m_instrList->addItem(item);
    }
}

void MainWindow::recomputeProjectLength() {
    int maxTick = 0;
    for (const auto &c : m_project.clips)
        maxTick = std::max(maxTick, c.startTick + c.lengthTicks);
    maxTick = ((maxTick + 15) / 16) * 16;
    if (maxTick < 256) maxTick = 256;
    m_project.totalTicks = maxTick;
    for (const auto &c : m_project.clips)
        if (c.track + 1 > m_project.trackCount) m_project.trackCount = c.track + 1;
    if (m_arrangement) m_arrangement->setProject(&m_project);
}

void MainWindow::refreshAllUI() {
    recomputeProjectLength();
    if (m_activePattern >= (int)m_project.patterns.size())
        m_activePattern = std::max(0, int(m_project.patterns.size()) - 1);
    refreshPatternCombo();
    if (!m_project.patterns.empty()) {
        Pattern *pat = m_project.patterns[m_activePattern].get();
        m_pianoRoll->setPattern(pat);
        for (int v = 0; v < 3; ++v)
            m_voicePanels[v]->setPattern(pat, v);
    }
    m_pianoRoll->setProject(&m_project);
    m_arrangement->setProject(&m_project);
    m_arrangement->setActivePattern(m_activePattern);
    m_arrangement->update();
    m_pianoRoll->update();
    refreshInstrumentCombo();
    refreshInstrumentList();
}

void MainWindow::onInstrumentChanged(int comboIdx) {
    if (comboIdx < 0) return;
    int idx = m_instrCombo->itemData(comboIdx).toInt();
    m_currentInstrument = idx;
    m_pianoRoll->setCurrentInstrument(idx);
}

void MainWindow::onNewInstrument() {
    Instrument instr;
    instr.name = QString("Instrument %1").arg(m_project.instruments.size() + 1);
    m_undoStack->push(new AddInstrumentCommand(&m_project, instr));
    m_currentInstrument = int(m_project.instruments.size()) - 1;
    refreshInstrumentCombo();
    refreshInstrumentList();
}

void MainWindow::onEditInstrument() {
    if (m_currentInstrument < 0 || m_currentInstrument >= (int)m_project.instruments.size()) {
        QMessageBox::information(this, "Edit Instrument", "Select an instrument first.");
        return;
    }
    Instrument *instr = m_project.instruments[m_currentInstrument].get();
    InstrumentEditorDialog dlg(*instr, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Instrument result = dlg.result();
    VoiceParams oldVP = instr->params;
    QString oldName = instr->name;
    instr->name = result.name;
    instr->params = result.params;
    if (result.name != oldName)
        m_undoStack->push(new RenameInstrumentCommand(instr, oldName, result.name));
    if (std::memcmp(&result.params, &oldVP, sizeof(VoiceParams)) != 0)
        m_undoStack->push(new ChangeInstrumentParamsCommand(instr, oldVP, result.params));
    refreshInstrumentCombo();
    refreshInstrumentList();
}

void MainWindow::onDeleteInstrument() {
    if (m_currentInstrument < 0 || m_currentInstrument >= (int)m_project.instruments.size()) {
        QMessageBox::information(this, "Delete Instrument", "Select an instrument first.");
        return;
    }
    if (QMessageBox::question(this, "Delete Instrument",
        QString("Delete '%1'?").arg(m_project.instruments[m_currentInstrument]->name))
        != QMessageBox::Yes) return;
    m_undoStack->push(new RemoveInstrumentCommand(&m_project, m_currentInstrument));
    m_currentInstrument = std::min(m_currentInstrument, int(m_project.instruments.size()) - 1);
    if (m_currentInstrument < -1) m_currentInstrument = -1;
    refreshInstrumentCombo();
    refreshInstrumentList();
}

void MainWindow::onAddPatternToTrack(int track, int startTick) {
    Pattern p;
    p.name = QString("Pattern %1").arg(m_project.patterns.size() + 1);
    p.lengthTicks = 64;
    m_undoStack->push(new AddPatternCommand(&m_project, p));
    int patIdx = int(m_project.patterns.size()) - 1;
    Clip c;
    c.patternIndex = patIdx;
    c.track = track;
    c.startTick = startTick;
    c.lengthTicks = 64;
    m_undoStack->push(new AddClipCommand(&m_project, c));
    m_activePattern = patIdx;
    refreshAllUI();
}

void MainWindow::onRequestNoteDialog(const QVector<int> &indices) {
    if (indices.isEmpty()) return;
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    Pattern *pat = m_project.patterns[m_activePattern].get();

    QVector<Note> notes;
    for (int i : indices)
        if (i >= 0 && i < pat->notes.size()) notes.append(pat->notes[i]);
    if (notes.isEmpty()) return;

    QVector<QString> instrNames;
    for (const auto &instr : m_project.instruments) instrNames.append(instr->name);

    NotePropertiesDialog dlg(notes, instrNames, this);
    if (dlg.exec() != QDialog::Accepted) return;

    int newInstr = dlg.instrumentIndex();
    int newVoice = dlg.voiceIndex();
    int pitchShift = dlg.pitchShift();
    int lengthDelta = dlg.lengthDelta();

    QVector<Note> oldNotes, newNotes;
    for (int i : indices) {
        if (i < 0 || i >= pat->notes.size()) continue;
        Note oldN = pat->notes[i];
        Note newN = oldN;
        newN.instrumentIndex = newInstr;
        if (newVoice >= 0) newN.voice = newVoice;
        if (pitchShift != 0) newN.pitch = qBound(36, newN.pitch + pitchShift, 84);
        if (lengthDelta != 0) newN.length = qMax(1, newN.length + lengthDelta);
        oldNotes.append(oldN);
        newNotes.append(newN);
    }
    if (oldNotes.isEmpty()) return;
    m_undoStack->push(new MoveNotesCommand(pat, indices, oldNotes, newNotes));
    m_pianoRoll->update();
}

void MainWindow::onInstrumentDropped(const QVector<int> &indices, int instrIdx) {
    if (indices.isEmpty()) return;
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    Pattern *pat = m_project.patterns[m_activePattern].get();
    m_undoStack->push(new ChangeNoteInstrumentCommand(pat, indices, instrIdx));
    m_pianoRoll->update();
    QString name = "(default)";
    if (instrIdx >= 0 && instrIdx < (int)m_project.instruments.size())
        name = m_project.instruments[instrIdx]->name;
    statusBar()->showMessage(QString("Assigned '%1' to %2 note(s)").arg(name).arg(indices.size()), 3000);
}

void MainWindow::onPlayPause() {
    if (!m_audioSink || !m_player) return;
    if (m_playing) {
        m_audioSink->stop(); m_timer->stop();
        m_playing = false; m_playBtn->setText("Play");
    } else {
        if (m_playheadTick >= m_project.totalTicks) m_playheadTick = 0.0;
        m_player->rebuild();
        m_player->setPlayheadTick(m_playheadTick);
        m_player->setBpm(m_bpmSpin->value());
        m_player->resetVoices();
        m_sidEngine->reset();
        m_audioSink->start(m_audioDevice);
        m_timer->start(16);
        m_playing = true;
        m_playBtn->setText("Pause");
    }
}

void MainWindow::onStop() {
    if (m_recording) {
        m_recorder->stop(); m_midiPollTimer->stop();
        m_recording = false; m_recBtn->setText("Record"); m_recBtn->setStyleSheet("");
    }
    if (m_audioSink) m_audioSink->stop();
    if (m_timer) m_timer->stop();
    if (m_player) m_player->resetVoices();
    if (m_sidEngine) m_sidEngine->reset();
    m_playing = false;
    if (m_playBtn) m_playBtn->setText("Play");
    m_playheadTick = 0.0;
    if (m_player) m_player->setPlayheadTick(0.0);
    if (m_arrangement) m_arrangement->setPlayheadTick(0.0);
    updatePosLabel();
}



void MainWindow::onTick() {
    if (!m_playing || !m_player) return;
    m_playheadTick = m_player->playheadTick();
    if (m_playheadTick >= m_project.totalTicks) { onStop(); return; }
    m_arrangement->setPlayheadTick(m_playheadTick);
    if (m_arrangeScroll) {
        int cx = 120 + int(m_playheadTick * m_arrangement->zoom());
        int vl = m_arrangeScroll->horizontalScrollBar()->value();
        int vr = vl + m_arrangeScroll->viewport()->width();
        if (cx > vr - 60)
            m_arrangeScroll->horizontalScrollBar()->setValue(
                cx - m_arrangeScroll->viewport()->width() + 100);
    }
    updatePosLabel();
}

void MainWindow::onRefreshMidiPorts() {
    if (!m_recorder) return;
    m_midiPortBox->clear();
    QVector<QString> ports = m_recorder->listPorts();
    if (ports.isEmpty()) m_midiPortBox->addItem("(no MIDI devices)", -1);
    else for (int i = 0; i < ports.size(); ++i) m_midiPortBox->addItem(ports[i], i);
}

void MainWindow::onToggleRecord() {
    if (!m_recorder) return;
    if (m_recording) {
        m_recorder->stop(); m_midiPollTimer->stop();
        m_recording = false; m_recBtn->setText("Record"); m_recBtn->setStyleSheet("");
        return;
    }
    int port = m_midiPortBox->currentIndex();
    if (port < 0 || m_midiPortBox->currentData().toInt() < 0) {
        QMessageBox::information(this, "Record MIDI", "No MIDI input device.");
        return;
    }
    if (!m_playing) onPlayPause();
    m_recorder->setRecordVoice(m_recVoiceBox->currentData().toInt());
    m_recorder->setPlayheadTick(m_playheadTick);
    if (!m_recorder->start(port)) {
        QMessageBox::warning(this, "Record MIDI", "Failed to open MIDI port."); return;
    }
    m_recording = true;
    m_midiPollTimer->start();
    m_recBtn->setText("STOP Rec");
    m_recBtn->setStyleSheet("QPushButton { background-color: #cc3333; color: white; font-weight: bold; }");
}

void MainWindow::onMidiPoll() {
    if (!m_recording || !m_player) return;
    m_recorder->setPlayheadTick(m_player->playheadTick());
    m_recorder->setRecordVoice(m_recVoiceBox->currentData().toInt());
    QVector<Note> completed = m_recorder->takeCompletedNotes();
    if (completed.isEmpty()) return;
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    Pattern *pat = m_project.patterns[m_activePattern].get();
    for (const auto &n : completed) m_undoStack->push(new AddNoteCommand(pat, n));
    m_pianoRoll->update();
}

void MainWindow::onDuplicatePattern() {
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    Pattern *src = m_project.patterns[m_activePattern].get();
    Pattern copy = *src;
    copy.name = src->name + " copy";
    m_undoStack->push(new AddPatternCommand(&m_project, copy));
    m_activePattern = int(m_project.patterns.size()) - 1;
    refreshAllUI();
}

void MainWindow::onRenamePattern() {
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    Pattern *p = m_project.patterns[m_activePattern].get();
    bool ok = false;
    QString newName = QInputDialog::getText(this, "Rename Pattern",
        "Name:", QLineEdit::Normal, p->name, &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    m_undoStack->push(new RenamePatternCommand(p, p->name, newName.trimmed()));
    refreshAllUI();
}

void MainWindow::onDeletePattern() {
    if (m_project.patterns.size() <= 1) {
        QMessageBox::warning(this, "Delete Pattern", "Cannot delete the last pattern."); return;
    }
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    if (QMessageBox::question(this, "Delete Pattern",
        QString("Delete '%1'?").arg(m_project.patterns[m_activePattern]->name))
        != QMessageBox::Yes) return;
    onStop();
    m_undoStack->push(new DeletePatternCommand(&m_project, m_activePattern));
    m_activePattern = std::min(m_activePattern, int(m_project.patterns.size()) - 1);
    refreshAllUI();
}

void MainWindow::onImportMidi() {
    QString path = QFileDialog::getOpenFileName(this, "Import MIDI",
        QDir::homePath(), "MIDI files (*.mid *.midi)");
    if (path.isEmpty()) return;
    QVector<RawNote> rawNotes;
    QVector<ChannelInfo> channels;
    int bpm = 120, lengthTicks = 64;
    QString err = MidiImporter::importFile(path, bpm, lengthTicks, rawNotes, channels);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "MIDI import failed", err); return;
    }
    if (rawNotes.isEmpty() || channels.isEmpty()) {
        QMessageBox::warning(this, "MIDI import failed", "No notes found."); return;
    }
    onStop();
    MidiImportDialog dlg(path, bpm, lengthTicks, rawNotes.size(), channels, this);
    if (dlg.exec() != QDialog::Accepted) return;
    QHash<int,int> mapping = dlg.channelMapping();
    QString baseName = QFileInfo(path).baseName();
    if (baseName.isEmpty()) baseName = "Imported";
    m_bpmSpin->setValue(bpm);
    m_undoStack->beginMacro("Import MIDI");

    QHash<int,int> chToInstr;
    QList<int> selectedChannels;
    for (auto it = mapping.constBegin(); it != mapping.constEnd(); ++it)
        if (it.value() >= 0) selectedChannels.append(it.key());
    std::sort(selectedChannels.begin(), selectedChannels.end());

    for (int ch : selectedChannels) {
        int voice = mapping[ch];
        Instrument instr;
        VoiceParams &vp = instr.params;
        if (ch == 9) {
            instr.name = QString("Ch%1 Drums").arg(ch + 1);
            vp.waveform = 3; vp.attack = 0; vp.decay = 4;
            vp.sustain = 0; vp.release = 2; vp.volumeEnvelope.clear();
        } else if (voice == 0) {
            instr.name = QString("Ch%1 Lead").arg(ch + 1);
            vp.waveform = 2; vp.pulseWidth = 2048;
            vp.attack = 1; vp.decay = 2; vp.sustain = 12; vp.release = 4;
        } else if (voice == 1) {
            instr.name = QString("Ch%1 Bass").arg(ch + 1);
            vp.waveform = 0; vp.attack = 0; vp.decay = 3;
            vp.sustain = 13; vp.release = 3;
        } else {
            instr.name = QString("Ch%1 Harmony").arg(ch + 1);
            vp.waveform = 1; vp.attack = 2; vp.decay = 3;
            vp.sustain = 10; vp.release = 4;
        }
        int instrIdx = int(m_project.instruments.size());
        m_undoStack->push(new AddInstrumentCommand(&m_project, instr));
        chToInstr[ch] = instrIdx;
    }

    if (dlg.mode() == MidiImportDialog::SinglePattern) {
        Pattern p;
        p.name = baseName;
        p.lengthTicks = lengthTicks;
        MidiImporter::buildPatternFromSelection(rawNotes, mapping, p, chToInstr);
        if (p.notes.isEmpty()) {
            m_undoStack->endMacro();
            QMessageBox::warning(this, "MIDI import", "No notes in selected channels."); return;
        }
        m_undoStack->push(new AddPatternCommand(&m_project, p));
        int patIdx = int(m_project.patterns.size()) - 1;
        int freeTrack = 0;
        for (int tr = 0; tr < m_project.trackCount; ++tr) {
            bool occ = false;
            for (const auto &c : m_project.clips)
                if (c.track == tr && c.startTick < lengthTicks && c.startTick + c.lengthTicks > 0)
                    { occ = true; break; }
            if (!occ) { freeTrack = tr; break; }
        }
        Clip c;
        c.patternIndex = patIdx; c.track = freeTrack;
        c.startTick = 0; c.lengthTicks = lengthTicks;
        m_undoStack->push(new AddClipCommand(&m_project, c));
        m_activePattern = patIdx;
    } else {
        int trackIdx = 0;
        for (int ch : selectedChannels) {
            QHash<int,int> one; one[ch] = mapping[ch];
            QHash<int,int> oneI;
            if (chToInstr.contains(ch)) oneI[ch] = chToInstr[ch];
            Pattern p;
            p.name = QString("%1 ch%2").arg(baseName).arg(ch + 1);
            p.lengthTicks = lengthTicks;
            MidiImporter::buildPatternFromSelection(rawNotes, one, p, oneI);
            if (p.notes.isEmpty()) continue;
            int patIdx = int(m_project.patterns.size());
            m_undoStack->push(new AddPatternCommand(&m_project, p));
            Clip c;
            c.patternIndex = patIdx; c.track = trackIdx;
            c.startTick = 0; c.lengthTicks = lengthTicks;
            m_undoStack->push(new AddClipCommand(&m_project, c));
            trackIdx++;
            if (trackIdx >= m_project.trackCount) m_project.trackCount = trackIdx + 1;
        }
        if (trackIdx > 0) m_activePattern = int(m_project.patterns.size()) - 1;
    }
    m_undoStack->endMacro();
    refreshAllUI();
}

void MainWindow::onSaveProject() {
    QString path = QFileDialog::getSaveFileName(this, "Save Project",
        QDir::homePath() + "/project.siddaw", "SidDAW project (*.siddaw)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".siddaw", Qt::CaseInsensitive)) path += ".siddaw";
    QString err = ProjectIO::save(m_project, m_bpmSpin->value(), path);
    if (err.isEmpty()) statusBar()->showMessage("Saved: " + path, 5000);
    else QMessageBox::warning(this, "Save failed", err);
}

void MainWindow::onLoadProject() {
    QString path = QFileDialog::getOpenFileName(this, "Open Project",
        QDir::homePath(), "SidDAW project (*.siddaw)");
    if (path.isEmpty()) return;
    onStop();
    Project loaded;
    int bpm = 120;
    QString err = ProjectIO::load(loaded, bpm, path);
    if (!err.isEmpty()) { QMessageBox::warning(this, "Load failed", err); return; }
    m_project = std::move(loaded);
    m_activePattern = 0;
    m_currentInstrument = -1;
    m_bpmSpin->setValue(bpm);
    m_undoStack->clear();
    if (m_player) {
        m_player->setProject(&m_project);
        m_player->setBpm(bpm);
        m_player->resetVoices();
        m_player->setPlayheadTick(0.0);
    }
    m_playheadTick = 0.0;
    refreshAllUI();
    updatePosLabel();
}

void MainWindow::onExportWav() {
    QString path = QFileDialog::getSaveFileName(this, "Export WAV",
        QDir::homePath() + "/sid_daw_export.wav", "WAV files (*.wav)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".wav", Qt::CaseInsensitive)) path += ".wav";
    QString err = WavExporter::exportProject(m_project, m_activePattern, m_bpmSpin->value(), path);
    if (err.isEmpty()) statusBar()->showMessage("Exported: " + path, 5000);
    else QMessageBox::warning(this, "Export failed", err);
}

void MainWindow::onExportSid() {
    QString path = QFileDialog::getSaveFileName(this, "Export SID",
        QDir::homePath() + "/sid_daw_export.sid", "SID files (*.sid)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".sid", Qt::CaseInsensitive)) path += ".sid";
    QString err = SidExporter::exportProject(m_project, m_bpmSpin->value(), path);
    if (err.isEmpty()) statusBar()->showMessage("Exported SID: " + path, 5000);
    else QMessageBox::warning(this, "SID export failed", err);
}

void MainWindow::updatePosLabel() {
    int t = int(m_playheadTick);
    int bar = t / 16 + 1;
    int beat = (t % 16) / 4 + 1;
    int sub = t % 4;
    if (m_posLabel) {
        QString suffix = m_recording ? " ●REC" : "";
        m_posLabel->setText(QString("Bar %1.%2.%3%4").arg(bar).arg(beat).arg(sub).arg(suffix));
    }
}

void MainWindow::onAbout() {
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onLanguageSelected(int lang) {
    if (lang < 0 || lang >= LanguageManager::availableLanguages().size())
        return;
    LanguageManager::instance().setCurrent(
        static_cast<LanguageManager::Language>(lang));

    QSettings settings;
    settings.setValue("language", lang);

    for (int i = 0; i < 4; ++i)
        if (m_langActions[i]) m_langActions[i]->setChecked(i == lang);

    retranslateUi();

    QMessageBox::information(this,
        LanguageManager::tr("Language changed"),
        LanguageManager::tr("The interface language has been saved. "
            "Menu and About will update now. All other labels (dialogs, "
            "editors) will change after restarting the application."));
}

void MainWindow::retranslateUi() {
    if (m_fileMenu)     m_fileMenu->setTitle(LanguageManager::tr("File"));
    if (m_editMenu)     m_editMenu->setTitle(LanguageManager::tr("Edit"));
    if (m_languageMenu) m_languageMenu->setTitle(LanguageManager::tr("Language"));
    if (m_helpMenu)     m_helpMenu->setTitle(LanguageManager::tr("Help"));
    if (m_actAbout)     m_actAbout->setText(LanguageManager::tr("About"));
    if (m_actUndo)      m_actUndo->setText(LanguageManager::tr("Undo"));
    if (m_actRedo)      m_actRedo->setText(LanguageManager::tr("Redo"));
    if (m_playBtn)      m_playBtn->setText(m_playing ? LanguageManager::tr("Pause")
                                                    : LanguageManager::tr("Play"));
    if (m_stopBtn)      m_stopBtn->setText(LanguageManager::tr("Stop"));
    if (m_recBtn)       m_recBtn->setText(m_recording ? "STOP Rec"
                                                     : LanguageManager::tr("Record"));
}