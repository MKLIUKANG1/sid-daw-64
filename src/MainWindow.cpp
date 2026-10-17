#include "MainWindow.h"
#include "PianoRollWidget.h"
#include "ArrangementWidget.h"
#include "VoicePanelWidget.h"
#include "SidEngine.h"
#include "ArrangementPlayer.h"
#include "SidAudioDevice.h"
#include "WavExporter.h"
#include "MidiImporter.h"
#include "ProjectIO.h"
#include "SidExporter.h"
#include "MidiRecorder.h"
#include "Commands.h"

#include <QMenuBar>
#include <QToolBar>
#include <QSplitter>
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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildDemoContent();
    m_undoStack = new QUndoStack(this);
    m_recorder = new MidiRecorder(this);

    setWindowTitle("SID DAW 64 - prototype");
    resize(1400, 900);

    // --- File menu ---
    auto *fileMenu = menuBar()->addMenu("&File");
    auto *actOpen = fileMenu->addAction("Open Project...");
    connect(actOpen, &QAction::triggered, this, &MainWindow::onLoadProject);
    auto *actSave = fileMenu->addAction("Save Project As...");
    connect(actSave, &QAction::triggered, this, &MainWindow::onSaveProject);
    fileMenu->addSeparator();
    auto *actImp = fileMenu->addAction("Import MIDI...");
    connect(actImp, &QAction::triggered, this, &MainWindow::onImportMidi);
    fileMenu->addSeparator();
    auto *actSid = fileMenu->addAction("Export SID...");
    connect(actSid, &QAction::triggered, this, &MainWindow::onExportSid);
    auto *actWav = fileMenu->addAction("Export WAV...");
    connect(actWav, &QAction::triggered, this, &MainWindow::onExportWav);
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", this, &QWidget::close);

    // --- Edit menu ---
    auto *editMenu = menuBar()->addMenu("&Edit");
    auto *undoAct = new QAction("Undo", this);
    undoAct->setShortcut(QKeySequence::Undo);
    connect(undoAct, &QAction::triggered, this, [this]() {
        m_undoStack->undo();
        refreshAllUI();
    });
    connect(m_undoStack, &QUndoStack::canUndoChanged,
            undoAct, &QAction::setEnabled);
    undoAct->setEnabled(false);
    editMenu->addAction(undoAct);

    auto *redoAct = new QAction("Redo", this);
    redoAct->setShortcuts({QKeySequence::Redo, QKeySequence("Ctrl+Shift+Z")});
    connect(redoAct, &QAction::triggered, this, [this]() {
        m_undoStack->redo();
        refreshAllUI();
    });
    connect(m_undoStack, &QUndoStack::canRedoChanged,
            redoAct, &QAction::setEnabled);
    redoAct->setEnabled(false);
    editMenu->addAction(redoAct);

    // --- Toolbar 1: playback & patterns ---
    auto *tb = addToolBar("Main");
    m_playBtn = new QPushButton("Play");
    tb->addWidget(m_playBtn);
    connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);

    auto *stopBtn = new QPushButton("Stop");
    tb->addWidget(stopBtn);
    connect(stopBtn, &QPushButton::clicked, this, &MainWindow::onStop);

    tb->addSeparator();
    tb->addWidget(new QLabel(" BPM: "));
    m_bpmSpin = new QSpinBox;
    m_bpmSpin->setRange(40, 300);
    m_bpmSpin->setValue(120);
    tb->addWidget(m_bpmSpin);

    tb->addSeparator();
    tb->addWidget(new QLabel(" Pattern: "));
    m_patternBox = new QComboBox;
    for (size_t i = 0; i < m_project.patterns.size(); ++i)
        m_patternBox->addItem(m_project.patterns[i]->name, int(i));
    tb->addWidget(m_patternBox);

    auto *renameBtn = new QPushButton("Rename");
    tb->addWidget(renameBtn);
    connect(renameBtn, &QPushButton::clicked, this, &MainWindow::onRenamePattern);

    auto *dupBtn = new QPushButton("Duplicate");
    tb->addWidget(dupBtn);
    connect(dupBtn, &QPushButton::clicked, this, &MainWindow::onDuplicatePattern);

    auto *delBtn = new QPushButton("Delete");
    tb->addWidget(delBtn);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::onDeletePattern);

    tb->addSeparator();
    tb->addWidget(new QLabel(" Master: "));
    m_masterVol = new QSlider(Qt::Horizontal);
    m_masterVol->setRange(0, 100);
    m_masterVol->setValue(100);
    m_masterVol->setFixedWidth(120);
    tb->addWidget(m_masterVol);

    tb->addSeparator();
    m_posLabel = new QLabel("Bar 1.1.0");
    m_posLabel->setStyleSheet("color: #ddd; padding: 0 8px;");
    tb->addWidget(m_posLabel);

    // --- Toolbar 2: MIDI recording ---
    auto *recBar = addToolBar("MIDI");
    m_recBtn = new QPushButton("Record");
    m_recBtn->setStyleSheet("QPushButton { padding: 4px 12px; }");
    recBar->addWidget(m_recBtn);
    connect(m_recBtn, &QPushButton::clicked, this, &MainWindow::onToggleRecord);

    recBar->addSeparator();
    recBar->addWidget(new QLabel(" MIDI In: "));
    m_midiPortBox = new QComboBox;
    m_midiPortBox->setMinimumWidth(220);
    recBar->addWidget(m_midiPortBox);

    auto *refreshBtn = new QPushButton("Refresh");
    recBar->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshMidiPorts);

    recBar->addSeparator();
    recBar->addWidget(new QLabel(" Rec Voice: "));
    m_recVoiceBox = new QComboBox;
    m_recVoiceBox->addItem("Voice 1 (lead)", 0);
    m_recVoiceBox->addItem("Voice 2 (bass)", 1);
    m_recVoiceBox->addItem("Voice 3 (harmony)", 2);
    recBar->addWidget(m_recVoiceBox);

    onRefreshMidiPorts();

    // --- Splitter ---
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

    auto *bottomSplit = new QSplitter(Qt::Horizontal);
    m_pianoRoll = new PianoRollWidget;
    m_pianoRoll->setPattern(m_project.patterns[0].get());
    m_pianoRoll->setUndoStack(m_undoStack);
    m_pianoScroll = new QScrollArea;
    m_pianoScroll->setWidget(m_pianoRoll);
    m_pianoScroll->setWidgetResizable(true);
    m_pianoScroll->setFrameShape(QFrame::NoFrame);
    bottomSplit->addWidget(m_pianoScroll);

    auto *tabs = new QTabWidget;
    for (int v = 0; v < 3; ++v) {
        m_voicePanels[v] = new VoicePanelWidget;
        m_voicePanels[v]->setPattern(m_project.patterns[0].get(), v);
        m_voicePanels[v]->setUndoStack(m_undoStack);
        tabs->addTab(m_voicePanels[v], QString("Voice %1").arg(v + 1));
    }
    tabs->setMaximumWidth(260);
    bottomSplit->addWidget(tabs);
    bottomSplit->setStretchFactor(0, 1);
    bottomSplit->setStretchFactor(1, 0);

    splitter->addWidget(bottomSplit);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);

    // --- Audio ---
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

    // --- Pattern combo ---
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

    statusBar()->showMessage("Ctrl+Z undo | Ctrl+wheel zoom | LMB add | RMB delete | Record → MIDI live");
    updatePosLabel();
}

void MainWindow::buildDemoContent() {
    auto p1 = std::make_unique<Pattern>();
    p1->name = "Pattern 1";
    p1->lengthTicks = 64;
    const int leadPitches[16] = {72, 76, 79, 84, 79, 76, 72, 76,
                                 79, 84, 88, 84, 79, 76, 72, 67};
    for (int i = 0; i < 16; ++i) {
        Note n; n.tick = i * 4; n.pitch = leadPitches[i]; n.length = 3; n.voice = 0;
        p1->notes.append(n);
    }
    const int bassPitches[8] = {36, 36, 43, 43, 36, 36, 43, 43};
    for (int i = 0; i < 8; ++i) {
        Note n; n.tick = i * 8; n.pitch = bassPitches[i]; n.length = 7; n.voice = 1;
        p1->notes.append(n);
    }
    const int harmPitches[8] = {60, 64, 67, 72, 67, 64, 60, 55};
    for (int i = 0; i < 8; ++i) {
        Note n; n.tick = i * 8 + 4; n.pitch = harmPitches[i]; n.length = 3; n.voice = 2;
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

void MainWindow::refreshAllUI() {
    if (m_activePattern >= (int)m_project.patterns.size())
        m_activePattern = int(m_project.patterns.size()) - 1;
    if (m_activePattern < 0) m_activePattern = 0;

    m_patternBox->blockSignals(true);
    m_patternBox->clear();
    for (size_t i = 0; i < m_project.patterns.size(); ++i)
        m_patternBox->addItem(m_project.patterns[i]->name, int(i));
    m_patternBox->setCurrentIndex(m_activePattern);
    m_patternBox->blockSignals(false);

    if (!m_project.patterns.empty()) {
        Pattern *pat = m_project.patterns[m_activePattern].get();
        m_pianoRoll->setPattern(pat);
        for (int v = 0; v < 3; ++v)
            m_voicePanels[v]->setPattern(pat, v);
    }
    m_arrangement->setProject(&m_project);
    m_arrangement->setActivePattern(m_activePattern);
    m_arrangement->update();
    m_pianoRoll->update();
}

void MainWindow::onRefreshMidiPorts() {
    if (!m_recorder) return;
    m_midiPortBox->clear();
    QVector<QString> ports = m_recorder->listPorts();
    if (ports.isEmpty()) {
        m_midiPortBox->addItem("(no MIDI devices)", -1);
    } else {
        for (int i = 0; i < ports.size(); ++i)
            m_midiPortBox->addItem(ports[i], i);
    }
}

void MainWindow::onToggleRecord() {
    if (!m_recorder) return;

    if (m_recording) {
        m_recorder->stop();
        m_midiPollTimer->stop();
        m_recording = false;
        m_recBtn->setText("Record");
        m_recBtn->setStyleSheet("QPushButton { padding: 4px 12px; }");
        statusBar()->showMessage("Recording stopped", 3000);
        return;
    }

    int port = m_midiPortBox->currentIndex();
    if (port < 0 || m_midiPortBox->currentData().toInt() < 0) {
        QMessageBox::information(this, "Record MIDI",
            "No MIDI input device found.\n"
            "Connect a MIDI keyboard and click Refresh.");
        return;
    }

    if (!m_playing) {
        // Автостарт воспроизведения, чтобы playhead двигался
        onPlayPause();
    }

    m_recorder->setRecordVoice(m_recVoiceBox->currentData().toInt());
    m_recorder->setPlayheadTick(m_playheadTick);

    if (!m_recorder->start(port)) {
        QMessageBox::warning(this, "Record MIDI",
            "Failed to open MIDI port. Check the device and try again.");
        return;
    }

    m_recording = true;
    m_midiPollTimer->start();
    m_recBtn->setText("STOP Rec");
    m_recBtn->setStyleSheet(
        "QPushButton { padding: 4px 12px; background-color: #cc3333; color: white; font-weight: bold; }");
    statusBar()->showMessage("Recording MIDI... Play or press Stop to finish", 5000);
}

void MainWindow::onMidiPoll() {
    if (!m_recording || !m_player) return;

    m_recorder->setPlayheadTick(m_player->playheadTick());
    m_recorder->setRecordVoice(m_recVoiceBox->currentData().toInt());

    QVector<Note> completed = m_recorder->takeCompletedNotes();
    if (completed.isEmpty()) return;

    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size())
        return;
    Pattern *pat = m_project.patterns[m_activePattern].get();

    for (const auto &n : completed)
        m_undoStack->push(new AddNoteCommand(pat, n));

    m_pianoRoll->update();
    statusBar()->showMessage(
        QString("Recorded %1 note(s)").arg(completed.size()), 2000);
}

void MainWindow::onPlayPause() {
    if (!m_audioSink || !m_player) return;
    if (m_playing) {
        m_audioSink->stop();
        m_timer->stop();
        m_playing = false;
        m_playBtn->setText("Play");
    } else {
        if (m_playheadTick >= m_project.totalTicks)
            m_playheadTick = 0.0;
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
        m_recorder->stop();
        m_midiPollTimer->stop();
        m_recording = false;
        m_recBtn->setText("Record");
        m_recBtn->setStyleSheet("QPushButton { padding: 4px 12px; }");
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
        int cursorX = 120 + int(m_playheadTick * m_arrangement->zoom());
        int viewLeft  = m_arrangeScroll->horizontalScrollBar()->value();
        int viewRight = viewLeft + m_arrangeScroll->viewport()->width();
        if (cursorX > viewRight - 60)
            m_arrangeScroll->horizontalScrollBar()->setValue(
                cursorX - m_arrangeScroll->viewport()->width() + 100);
    }
    updatePosLabel();
}

void MainWindow::onDuplicatePattern() {
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    Pattern *src = m_project.patterns[m_activePattern].get();
    Pattern copy = *src;
    copy.name = src->name + " copy";
    m_undoStack->push(new AddPatternCommand(&m_project, copy));
    m_activePattern = int(m_project.patterns.size()) - 1;
    refreshAllUI();
    statusBar()->showMessage(QString("Duplicated as '%1'").arg(copy.name), 4000);
}

void MainWindow::onRenamePattern() {
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    Pattern *p = m_project.patterns[m_activePattern].get();
    bool ok = false;
    QString newName = QInputDialog::getText(this, "Rename Pattern",
        "Pattern name:", QLineEdit::Normal, p->name, &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    QString trimmed = newName.trimmed();
    if (trimmed == p->name) return;
    m_undoStack->push(new RenamePatternCommand(p, p->name, trimmed));
    refreshAllUI();
    statusBar()->showMessage("Renamed to: " + trimmed, 3000);
}

void MainWindow::onDeletePattern() {
    if (m_project.patterns.size() <= 1) {
        QMessageBox::warning(this, "Delete Pattern",
            "Cannot delete the last pattern.");
        return;
    }
    if (m_activePattern < 0 || m_activePattern >= (int)m_project.patterns.size()) return;
    auto ret = QMessageBox::question(this, "Delete Pattern",
        QString("Delete pattern '%1'?").arg(m_project.patterns[m_activePattern]->name),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    onStop();
    int idx = m_activePattern;
    m_undoStack->push(new DeletePatternCommand(&m_project, idx));
    m_activePattern = std::min(idx, (int)m_project.patterns.size() - 1);
    refreshAllUI();
    statusBar()->showMessage("Pattern deleted", 3000);
}

void MainWindow::onImportMidi() {
    QString path = QFileDialog::getOpenFileName(
        this, "Import MIDI", QDir::homePath(),
        "MIDI files (*.mid *.midi)");
    if (path.isEmpty()) return;
    QVector<RawNote> rawNotes;
    int bpm = 120, lengthTicks = 64;
    QString err = MidiImporter::importFile(path, bpm, lengthTicks, rawNotes);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "MIDI import failed", err);
        return;
    }
    if (rawNotes.isEmpty()) {
        QMessageBox::warning(this, "MIDI import failed", "No notes found.");
        return;
    }
    onStop();
    Pattern tmp;
    tmp.name = QFileInfo(path).baseName();
    if (tmp.name.isEmpty()) tmp.name = "Imported";
    tmp.lengthTicks = lengthTicks;
    MidiImporter::assignToPattern(rawNotes, tmp);
    int noteCount = tmp.notes.size();

    m_undoStack->push(new AddPatternCommand(&m_project, tmp));
    m_activePattern = int(m_project.patterns.size()) - 1;
    m_bpmSpin->setValue(bpm);
    refreshAllUI();
    statusBar()->showMessage(
        QString("Imported '%1': %2 notes, BPM %3, %4 ticks")
            .arg(tmp.name).arg(noteCount).arg(bpm).arg(lengthTicks), 8000);
}

void MainWindow::onSaveProject() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project", QDir::homePath() + "/project.siddaw",
        "SidDAW project (*.siddaw)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".siddaw", Qt::CaseInsensitive)) path += ".siddaw";
    QString err = ProjectIO::save(m_project, m_bpmSpin->value(), path);
    if (err.isEmpty()) statusBar()->showMessage("Saved: " + path, 5000);
    else QMessageBox::warning(this, "Save failed", err);
}

void MainWindow::onLoadProject() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project", QDir::homePath(), "SidDAW project (*.siddaw)");
    if (path.isEmpty()) return;
    onStop();
    Project loaded;
    int bpm = 120;
    QString err = ProjectIO::load(loaded, bpm, path);
    if (!err.isEmpty()) {
        QMessageBox::warning(this, "Load failed", err);
        return;
    }
    m_project = std::move(loaded);
    m_activePattern = 0;
    m_bpmSpin->setValue(bpm);
    m_undoStack->clear();

    m_pianoRoll->setPattern(m_project.patterns[0].get());
    for (int v = 0; v < 3; ++v)
        m_voicePanels[v]->setPattern(m_project.patterns[0].get(), v);
    m_arrangement->setProject(&m_project);
    m_arrangement->setActivePattern(0);
    m_arrangement->setPlayheadTick(0.0);
    if (m_player) {
        m_player->setProject(&m_project);
        m_player->setBpm(bpm);
        m_player->resetVoices();
        m_player->setPlayheadTick(0.0);
    }
    m_playheadTick = 0.0;
    refreshAllUI();
    updatePosLabel();
    statusBar()->showMessage("Loaded: " + path, 5000);
}

void MainWindow::onExportWav() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export WAV", QDir::homePath() + "/sid_daw_export.wav",
        "WAV files (*.wav)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".wav", Qt::CaseInsensitive)) path += ".wav";
    statusBar()->showMessage("Rendering WAV...");
    QString err = WavExporter::exportProject(
        m_project, m_activePattern, m_bpmSpin->value(), path);
    if (err.isEmpty()) statusBar()->showMessage("Exported: " + path, 5000);
    else QMessageBox::warning(this, "Export failed", err);
}

void MainWindow::onExportSid() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export SID", QDir::homePath() + "/sid_daw_export.sid",
        "SID files (*.sid)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".sid", Qt::CaseInsensitive)) path += ".sid";
    statusBar()->showMessage("Building SID...");
    QString err = SidExporter::exportProject(m_project, m_bpmSpin->value(), path);
    if (err.isEmpty()) statusBar()->showMessage("Exported SID: " + path, 5000);
    else QMessageBox::warning(this, "SID export failed", err);
}

void MainWindow::updatePosLabel() {
    int t = int(m_playheadTick);
    int bar  = t / 16 + 1;
    int beat = (t % 16) / 4 + 1;
    int sub  = t % 4;
    if (m_posLabel) {
        QString suffix = m_recording ? " ●REC" : "";
        m_posLabel->setText(QString("Bar %1.%2.%3%4")
                                .arg(bar).arg(beat).arg(sub).arg(suffix));
    }
}
