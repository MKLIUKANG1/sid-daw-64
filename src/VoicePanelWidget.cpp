#include "VoicePanelWidget.h"
#include "EnvelopeEditorWidget.h"
#include "VoicePresetIO.h"
#include "PreviewPlayer.h"
#include "WaveformPreview.h"
#include "Commands.h"
#include "InstrumentPresets.h"
#include "EnvelopePresets.h"
#include <QInputDialog>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QUndoStack>
#include <QSettings>
#include <QFileInfo>
#include <cmath>

static QSlider *makeSlider(int min, int max, int def) {
    auto *s = new QSlider(Qt::Horizontal);
    s->setRange(min, max);
    s->setValue(def);
    s->setMinimumWidth(80);
    return s;
}

static void setTip(QWidget *w, const QString &text) {
    if (w) w->setToolTip(text);
}

VoicePanelWidget::VoicePanelWidget(QWidget *parent) : QWidget(parent) {
    m_preview = new PreviewPlayer(this);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);

    m_tabs = new QTabWidget;

    // ===== Sliders tab =====
    auto *slidersTab = new QWidget;
    auto *slidersLayout = new QVBoxLayout(slidersTab);
    slidersLayout->setContentsMargins(4, 4, 4, 4);
    slidersLayout->setSpacing(4);

    auto *waveRow = new QHBoxLayout;
    waveRow->addWidget(new QLabel("Wave:"));
    m_waveform = new QComboBox;
    m_waveform->addItem("Triangle", 0);
    m_waveform->addItem("Sawtooth", 1);
    m_waveform->addItem("Pulse",    2);
    m_waveform->addItem("Noise",    3);
    waveRow->addWidget(m_waveform, 1);
    m_wavePreview = new WaveformPreview;
    waveRow->addWidget(m_wavePreview);
    slidersLayout->addLayout(waveRow);
    setTip(m_waveform,
        "Форма волны SID:\n"
        "• Triangle — мягкая, флейтовая\n"
        "• Sawtooth — яркая, струнная\n"
        "• Pulse — квадрат (ширину задаёт PW)\n"
        "• Noise — шум (для барабанов и эффектов)");

    auto *pwRow = new QHBoxLayout;
    pwRow->addWidget(new QLabel("PW:"));
    m_pulseW = makeSlider(0, 4095, 2048);
    m_pulseWLab = new QLabel("2048");
    m_pulseWLab->setMinimumWidth(38);
    m_pulseWLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pwRow->addWidget(m_pulseW, 1);
    pwRow->addWidget(m_pulseWLab);
    slidersLayout->addLayout(pwRow);
    setTip(m_pulseW,
        "Pulse Width (ширина импульса) — только для Pulse:\n"
        "• 2048 (50%) — чистый квадрат\n"
        "• ближе к 0 или 4095 — тонкий, «носовой» звук");

    auto *adsrBox = new QGroupBox("ADSR — огибающая громкости");
    auto *adsr = new QGridLayout(adsrBox);
    adsr->setSpacing(2);
    adsr->setContentsMargins(4, 4, 4, 4);
    adsr->addWidget(new QLabel("A"), 0, 0);
    m_attack = makeSlider(0, 15, 1);
    adsr->addWidget(m_attack, 0, 1);
    setTip(m_attack, "Attack — время нарастания (0=мгновенно, 15=медленно)");
    adsr->addWidget(new QLabel("D"), 1, 0);
    m_decay = makeSlider(0, 15, 0);
    adsr->addWidget(m_decay, 1, 1);
    setTip(m_decay, "Decay — спад от пика до Sustain (0=быстро, 15=медленно)");
    adsr->addWidget(new QLabel("S"), 2, 0);
    m_sustain = makeSlider(0, 15, 15);
    adsr->addWidget(m_sustain, 2, 1);
    setTip(m_sustain, "Sustain — громкость, пока нота держится (0=стаккато)");
    adsr->addWidget(new QLabel("R"), 3, 0);
    m_release = makeSlider(0, 15, 3);
    adsr->addWidget(m_release, 3, 1);
    setTip(m_release, "Release — хвост затухания после отпускания ноты");
    slidersLayout->addWidget(adsrBox);

    auto *modRow = new QHBoxLayout;
    m_ring = new QCheckBox("Ring");
    m_sync = new QCheckBox("Sync");
    modRow->addWidget(m_ring);
    modRow->addWidget(m_sync);
    modRow->addStretch();
    slidersLayout->addLayout(modRow);
    setTip(m_ring, "Ring Modulation — «металлический» тембр");
    setTip(m_sync, "Hard Sync — резкий «звенящий» тембр");

    auto *mixBox = new QGroupBox("Mixer");
    auto *mix = new QGridLayout(mixBox);
    mix->setSpacing(2);
    mix->setContentsMargins(4, 4, 4, 4);
    mix->addWidget(new QLabel("Vol"), 0, 0);
    m_volume = makeSlider(0, 100, 100);
    mix->addWidget(m_volume, 0, 1);
    mix->addWidget(new QLabel("Pan"), 1, 0);
    m_pan = makeSlider(0, 100, 50);
    mix->addWidget(m_pan, 1, 1);
    slidersLayout->addWidget(mixBox);
    setTip(m_volume, "Громкость голоса 0..100%");
    setTip(m_pan, "Панорама L..R (заглушка для SID)");

    auto *presetRow = new QHBoxLayout;
    m_previewBtn = new QPushButton("Preview");
    m_previewBtn->setStyleSheet(
        "QPushButton { background-color: #2a7a2a; color: white; font-weight: bold; }");

    auto *libCombo = new QComboBox;
    libCombo->addItem("Load from library...", -1);
    for (const auto &p : InstrumentPresets::all())
        libCombo->addItem(p.name, -2);
    libCombo->setToolTip("Готовые пресеты: Bass, Lead, Pad, Pluck, Keys, Drum");
    connect(libCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, libCombo](int idx) {
        if (idx <= 0) return;
        auto presets = InstrumentPresets::all();
        int presetIdx = idx - 1;
        if (presetIdx < 0 || presetIdx >= presets.size()) return;

        VoiceParams *vp = currentParams();
        if (!vp) return;

        VoiceParams oldVP = *vp;
        // Сохраняем пользовательские огибающие — пресет их не трогает
        QVector<int> keepVol = vp->volumeEnvelope;
        QVector<int> keepPitch = vp->pitchEnvelope;
        QVector<int> keepDuty = vp->dutyEnvelope;
        QVector<int> keepArp = vp->arpeggioEnvelope;
        int keepSpeed = vp->envelopeSpeed;
        float keepVolume = vp->volume;
        float keepPan = vp->pan;

        *vp = presets[presetIdx].params;
        vp->volumeEnvelope = keepVol;
        vp->pitchEnvelope = keepPitch;
        vp->dutyEnvelope = keepDuty;
        vp->arpeggioEnvelope = keepArp;
        vp->envelopeSpeed = keepSpeed;
        vp->volume = keepVolume;
        vp->pan = keepPan;

        refreshFromModel();
        pushUndo(oldVP, *vp);
        emit paramsChanged();

        QMetaObject::invokeMethod(libCombo, [libCombo]() {
            libCombo->setCurrentIndex(0);
        }, Qt::QueuedConnection);
    });

    m_saveBtn = new QPushButton("Save");
    m_loadBtn = new QPushButton("Load");
    presetRow->addWidget(m_previewBtn);
    presetRow->addWidget(libCombo, 1);
    presetRow->addWidget(m_saveBtn);
    presetRow->addWidget(m_loadBtn);
    slidersLayout->addLayout(presetRow);
    setTip(libCombo,
        "Готовые пресеты инструментов.\n"
        "Применяются к текущему инструменту,\n"
        "огибающие сохраняются.");
    setTip(m_previewBtn, "Проиграть тестовую ноту C4 (0.8 сек)");
    slidersLayout->addStretch();

    m_tabs->addTab(slidersTab, "Sliders");

    // ===== Envelope tabs =====
    m_envVolume = new EnvelopeEditorWidget;
    m_envPitch  = new EnvelopeEditorWidget;
    m_envDuty   = new EnvelopeEditorWidget;
    m_envArp    = new EnvelopeEditorWidget;
    m_envVolume->setEnvelopeType(0);
    m_envPitch->setEnvelopeType(1);
    m_envDuty->setEnvelopeType(2);
    m_envArp->setEnvelopeType(3);

    m_tabs->addTab(m_envVolume, "Volume");
    m_tabs->addTab(m_envPitch,  "Pitch");
    m_tabs->addTab(m_envDuty,   "Duty");
    m_tabs->addTab(m_envArp,    "Arp");

    root->addWidget(m_tabs, 1);

    // ===== Control row: shape presets + speed + clear =====
    auto *ctlRow = new QHBoxLayout;
    // Pitch mode (Absolute/Relative) — виден только на вкладке Pitch
    m_pitchModeWidget = new QWidget;
    {
        auto *pm = new QHBoxLayout(m_pitchModeWidget);
        pm->setContentsMargins(0, 0, 0, 0);
        pm->addWidget(new QLabel("Pitch mode:"));
        m_pitchAbsoluteCombo = new QComboBox;
        m_pitchAbsoluteCombo->addItem("Relative (±8)", 0);
        m_pitchAbsoluteCombo->addItem("Absolute (+0..+15)", 1);
        pm->addWidget(m_pitchAbsoluteCombo);
    }
    m_pitchModeWidget->setVisible(false);
    ctlRow->addWidget(m_pitchModeWidget);
    ctlRow->addWidget(new QLabel("Shape:"));

    auto *flatBtn    = new QPushButton("Flat");
    auto *rampUpBtn  = new QPushButton("Ramp+");
    auto *rampDnBtn  = new QPushButton("Ramp-");
    auto *triBtn     = new QPushButton("Tri");
    auto *sineBtn    = new QPushButton("Sine");
    auto *pulseBtn   = new QPushButton("Pulse");
    for (auto *b : {flatBtn, rampUpBtn, rampDnBtn, triBtn, sineBtn, pulseBtn})
        ctlRow->addWidget(b);

    ctlRow->addStretch();
    ctlRow->addWidget(new QLabel("Speed:"));
    m_envSpeed = new QSpinBox;
    m_envSpeed->setRange(1, 16);
    m_envSpeed->setValue(4);
    ctlRow->addWidget(m_envSpeed);
    auto *clearBtn = new QPushButton("Clear");
    ctlRow->addWidget(clearBtn);
    root->addLayout(ctlRow);
    // ===== Envelope presets row =====
    auto *envPresetRow = new QHBoxLayout;
    envPresetRow->addWidget(new QLabel("Preset:"));
    auto *envPresetCombo = new QComboBox;
    envPresetCombo->addItem("(select preset...)", -1);
    {
        int idx = 0;
        for (const auto &p : EnvelopePresets::builtIn())
            envPresetCombo->addItem("[Built-in] " + p.first, idx++);
        auto user = EnvelopePresets::userPresets();
        for (auto it = user.begin(); it != user.end(); ++it)
            envPresetCombo->addItem("[User] " + it.key(), idx++);
    }
    envPresetRow->addWidget(envPresetCombo, 1);
    auto *saveEnvBtn = new QPushButton("Save as...");
    envPresetRow->addWidget(saveEnvBtn);
    root->addLayout(envPresetRow);

    connect(envPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, envPresetCombo](int idx) {
        if (idx <= 0) return;
        int tab = m_tabs->currentIndex() - 1;
        if (tab < 0 || tab > 3) return;

        QVector<int> env;
        int builtInCount = EnvelopePresets::builtIn().size();
        if (idx - 1 < builtInCount) {
            env = EnvelopePresets::builtIn()[idx - 1].second;
        } else {
            auto user = EnvelopePresets::userPresets();
            int userIdx = idx - 1 - builtInCount;
            auto it = user.begin();
            for (int i = 0; i < userIdx && it != user.end(); ++i) ++it;
            if (it != user.end()) env = it.value();
        }
        if (env.isEmpty()) return;

        VoiceParams *vp = currentParams();
        if (!vp) return;
        VoiceParams oldVP = *vp;
        EnvelopeEditorWidget *eds[4] = {m_envVolume, m_envPitch, m_envDuty, m_envArp};
        bool centers[4] = {false, true, true, false};
        int defs[4] = {15, 8, 8, 0};
        eds[tab]->setEnvelope(
            tab == 0 ? &vp->volumeEnvelope
                     : tab == 1 ? &vp->pitchEnvelope
                     : tab == 2 ? &vp->dutyEnvelope
                     : &vp->arpeggioEnvelope,
            15, defs[tab], centers[tab]);
        eds[tab]->setPoints(env);
        pushUndo(oldVP, *vp);
        emit paramsChanged();

        QMetaObject::invokeMethod(envPresetCombo, [envPresetCombo]() {
            envPresetCombo->setCurrentIndex(0);
        }, Qt::QueuedConnection);
    });

    connect(saveEnvBtn, &QPushButton::clicked, this, [this, envPresetCombo]() {
        int tab = m_tabs->currentIndex() - 1;
        if (tab < 0 || tab > 3) return;
        VoiceParams *vp = currentParams();
        if (!vp) return;
        QVector<int> *env = nullptr;
        switch (tab) {
            case 0: env = &vp->volumeEnvelope; break;
            case 1: env = &vp->pitchEnvelope; break;
            case 2: env = &vp->dutyEnvelope; break;
            case 3: env = &vp->arpeggioEnvelope; break;
        }
        if (!env || env->isEmpty()) return;
        bool ok = false;
        QString name = QInputDialog::getText(this, "Save Envelope Preset",
            "Preset name:", QLineEdit::Normal, "My Preset", &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        if (EnvelopePresets::saveUserPreset(name.trimmed(), *env)) {
            envPresetCombo->addItem("[User] " + name.trimmed());
        }
    });
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int idx) {
        if (m_pitchModeWidget) m_pitchModeWidget->setVisible(idx == 2);
    });
    connect(m_pitchAbsoluteCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        VoiceParams *vp = currentParams();
        if (!vp) return;
        VoiceParams oldVP = *vp;
        vp->pitchEnvelopeAbsolute = (idx == 1);
        pushUndo(oldVP, *vp);
        emit paramsChanged();
    });
    setTip(m_envSpeed, "Скорость огибающей: точек на тик (больше = быстрее)");
    setTip(flatBtn,   "Плоская линия — постоянное значение");
    setTip(rampUpBtn, "Нарастание от 0 до максимума");
    setTip(rampDnBtn, "Спад от максимума до 0");
    setTip(triBtn,    "Треугольник — нарастание и спад");
    setTip(sineBtn,   "Синусоида — плавные волны");
    setTip(pulseBtn,  "Пульс — квадратная волна (вкл/выкл)");
    setTip(clearBtn,  "Очистить активную огибающую");

    // ===== Connections =====
    connect(m_waveform, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
        if (m_wavePreview) m_wavePreview->setWaveform(idx);
        onAnyChange();
    });
    connect(m_pulseW, &QSlider::valueChanged, this, [this](int v) {
        m_pulseWLab->setText(QString::number(v));
        onAnyChange();
    });
    connect(m_attack,  &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_decay,   &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_sustain, &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_release, &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);

    // ===== Auto-preview: играть ноту при отпускании слайдера =====
    auto autoPrev = [this]() {
        if (m_autoPreviewCheck && m_autoPreviewCheck->isChecked())
            onPreview();
    };
    connect(m_attack,  &QSlider::sliderReleased, this, autoPrev);
    connect(m_decay,   &QSlider::sliderReleased, this, autoPrev);
    connect(m_sustain, &QSlider::sliderReleased, this, autoPrev);
    connect(m_release, &QSlider::sliderReleased, this, autoPrev);
    connect(m_pulseW,  &QSlider::sliderReleased, this, autoPrev);
    connect(m_volume,  &QSlider::sliderReleased, this, autoPrev);
    connect(m_ring,    &QCheckBox::toggled, this, &VoicePanelWidget::onAnyChange);
    connect(m_sync,    &QCheckBox::toggled, this, &VoicePanelWidget::onAnyChange);
    connect(m_volume,  &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_pan,     &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_envSpeed, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &VoicePanelWidget::onAnyChange);
    connect(m_previewBtn, &QPushButton::clicked, this, &VoicePanelWidget::onPreview);
    connect(m_saveBtn, &QPushButton::clicked, this, &VoicePanelWidget::onSavePreset);
    connect(m_loadBtn, &QPushButton::clicked, this, &VoicePanelWidget::onLoadPreset);

    connect(flatBtn,   &QPushButton::clicked, this, [this]() { applyEnvelopePreset(0); });
    connect(rampUpBtn, &QPushButton::clicked, this, [this]() { applyEnvelopePreset(1); });
    connect(rampDnBtn, &QPushButton::clicked, this, [this]() { applyEnvelopePreset(2); });
    connect(triBtn,    &QPushButton::clicked, this, [this]() { applyEnvelopePreset(3); });
    connect(sineBtn,   &QPushButton::clicked, this, [this]() { applyEnvelopePreset(4); });
    connect(pulseBtn,  &QPushButton::clicked, this, [this]() { applyEnvelopePreset(5); });

    wireEnvelope(m_envVolume, 0);
    wireEnvelope(m_envPitch,  1);
    wireEnvelope(m_envDuty,   2);
    wireEnvelope(m_envArp,    3);

    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        int which = m_tabs->currentIndex() - 1;
        if (which >= 0 && which < 4) onClearEnvelope(which);
    });
}

void VoicePanelWidget::wireEnvelope(EnvelopeEditorWidget *ed, int which) {
    connect(ed, &EnvelopeEditorWidget::envelopeChanged,
            this, &VoicePanelWidget::onEnvelopeChanged);
    connect(ed, &EnvelopeEditorWidget::envelopeChangeStarted,
            this, [this]() {
        VoiceParams *vp = currentParams();
        if (vp) { m_oldVPBeforeEdit = *vp; m_hasOldVP = true; }
    });
    connect(ed, &EnvelopeEditorWidget::envelopeChangeFinished,
            this, [this]() {
        VoiceParams *vp = currentParams();
        if (!vp || !m_hasOldVP) return;
        m_hasOldVP = false;
        pushUndo(m_oldVPBeforeEdit, *vp);
        emit paramsChanged();
    });
    Q_UNUSED(which);
}

void VoicePanelWidget::pushUndo(const VoiceParams &oldVP,
                                const VoiceParams &newVP) {
    if (!m_undoStack) return;
    if (m_instrument)
        m_undoStack->push(new ChangeInstrumentParamsCommand(
            m_instrument, oldVP, newVP));
    else if (m_pattern)
        m_undoStack->push(new ChangeVoiceParamCommand(
            m_pattern, m_voiceIndex, oldVP, newVP));
}

VoiceParams *VoicePanelWidget::currentParams() const {
    if (m_instrument) return const_cast<VoiceParams*>(&m_instrument->params);
    if (m_vp) return m_vp;
    if (m_pattern) return const_cast<VoiceParams*>(&m_pattern->voiceParams[m_voiceIndex]);
    return nullptr;
}

void VoicePanelWidget::setPattern(Pattern *pat, int voiceIndex) {
    m_pattern = pat; m_vp = nullptr; m_instrument = nullptr;
    m_voiceIndex = voiceIndex;
    refreshFromModel();
}

void VoicePanelWidget::setVoiceParams(VoiceParams *vp) {
    m_vp = vp; m_pattern = nullptr; m_instrument = nullptr; m_voiceIndex = 0;
    refreshFromModel();
}

void VoicePanelWidget::setInstrument(Instrument *instr, QUndoStack *undo) {
    m_instrument = instr; m_pattern = nullptr; m_vp = nullptr; m_voiceIndex = 0;
    m_undoStack = undo;
    refreshFromModel();
}

void VoicePanelWidget::selectTab(int index) {
    if (m_tabs) m_tabs->setCurrentIndex(index);
}

void VoicePanelWidget::triggerPreview() { onPreview(); }

void VoicePanelWidget::refreshFromModel() {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    m_updating = true;
    m_waveform->setCurrentIndex(vp->waveform);
    if (m_wavePreview) m_wavePreview->setWaveform(vp->waveform);
    m_pulseW->setValue(vp->pulseWidth);
    m_pulseWLab->setText(QString::number(vp->pulseWidth));
    m_attack->setValue(vp->attack);
    m_decay->setValue(vp->decay);
    m_sustain->setValue(vp->sustain);
    m_release->setValue(vp->release);
    m_ring->setChecked(vp->ringMod);
    m_sync->setChecked(vp->sync);
    m_volume->setValue(int(vp->volume * 100.0f));
    m_pan->setValue(int(vp->pan * 50.0f) + 50);
    m_envSpeed->setValue(vp->envelopeSpeed);
    if (m_pitchAbsoluteCombo)
        m_pitchAbsoluteCombo->setCurrentIndex(vp->pitchEnvelopeAbsolute ? 1 : 0);

    m_envVolume->setEnvelope(&vp->volumeEnvelope,   15, 15, false);
    m_envPitch ->setEnvelope(&vp->pitchEnvelope,    15,  8, true);
    m_envDuty  ->setEnvelope(&vp->dutyEnvelope,     15,  8, true);
    m_envArp   ->setEnvelope(&vp->arpeggioEnvelope, 15,  0, false);

    m_updating = false;
}

void VoicePanelWidget::onAnyChange() {
    if (m_updating) return;
    VoiceParams *vp = currentParams();
    if (!vp) return;

    VoiceParams oldVP = *vp;
    vp->waveform   = m_waveform->currentIndex();
    vp->pulseWidth = m_pulseW->value();
    vp->attack     = m_attack->value();
    vp->decay      = m_decay->value();
    vp->sustain    = m_sustain->value();
    vp->release    = m_release->value();
    vp->ringMod    = m_ring->isChecked();
    vp->sync       = m_sync->isChecked();
    vp->volume     = m_volume->value() / 100.0f;
    vp->pan        = (m_pan->value() - 50) / 50.0f;
    vp->envelopeSpeed = m_envSpeed->value();

    pushUndo(oldVP, *vp);
    emit paramsChanged();
}

void VoicePanelWidget::onPreview() {
    VoiceParams *vp = currentParams();
    if (!vp || !m_preview) return;
    m_preview->playNote(*vp, 60, 800);
}

void VoicePanelWidget::onEnvelopeChanged() { emit paramsChanged(); }

void VoicePanelWidget::applyEnvelopePreset(int shape) {
    VoiceParams *vp = currentParams();
    if (!vp) return;

    int tab = m_tabs->currentIndex() - 1;
    if (tab < 0 || tab > 3) return;
    // для Absolute Pitch — диапазон 0..15 всё равно, но default = 0

    VoiceParams oldVP = *vp;

    QVector<int> *env = nullptr;
    int maxV = 15, defV = 15;
    switch (tab) {
        case 0: env = &vp->volumeEnvelope;   maxV = 15; defV = 15; break;
        case 1: env = &vp->pitchEnvelope;    maxV = 15; defV = 8;  break;
        case 2: env = &vp->dutyEnvelope;     maxV = 15; defV = 8;  break;
        case 3: env = &vp->arpeggioEnvelope; maxV = 15; defV = 0;  break;
    }
    if (!env) return;

    int len = 16;
    QVector<int> pts(len);
    for (int i = 0; i < len; ++i) {
        double t = (len > 1) ? double(i) / (len - 1) : 0.0;
        int v = defV;
        switch (shape) {
            case 0: v = defV; break;
            case 1: v = int(t * maxV); break;
            case 2: v = int((1.0 - t) * maxV); break;
            case 3: v = int((t < 0.5 ? t * 2.0 : (1.0 - t) * 2.0) * maxV); break;
            case 4: v = int((std::sin(t * 2.0 * 3.14159265) * 0.5 + 0.5) * maxV); break;
            case 5: v = ((i / 4) % 2) ? maxV : 0; break;
        }
        pts[i] = std::clamp(v, 0, maxV);
    }

    EnvelopeEditorWidget *eds[4]  = {m_envVolume, m_envPitch, m_envDuty, m_envArp};
    bool centers[4]               = {false, true, true, false};
    int defs[4]                   = {15, 8, 8, 0};
    eds[tab]->setEnvelope(env, maxV, defs[tab], centers[tab]);
    eds[tab]->setPoints(pts);

    pushUndo(oldVP, *vp);
    emit paramsChanged();
}

void VoicePanelWidget::onClearEnvelope(int which) {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    VoiceParams oldVP = *vp;
    switch (which) {
        case 0: vp->volumeEnvelope.clear();   m_envVolume->setEnvelope(&vp->volumeEnvelope,   15, 15, false); break;
        case 1: vp->pitchEnvelope.clear();    m_envPitch ->setEnvelope(&vp->pitchEnvelope,    15,  8, true);  break;
        case 2: vp->dutyEnvelope.clear();     m_envDuty  ->setEnvelope(&vp->dutyEnvelope,     15,  8, true);  break;
        case 3: vp->arpeggioEnvelope.clear(); m_envArp   ->setEnvelope(&vp->arpeggioEnvelope, 15,  0, false); break;
    }
    pushUndo(oldVP, *vp);
    emit paramsChanged();
}

void VoicePanelWidget::onSavePreset() {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    QString path = QFileDialog::getSaveFileName(this, "Save Voice Preset",
        QSettings().value("lastDir", QDir::homePath()).toString() + "/voice_preset.json", "Voice preset (*.json)");
    if (path.isEmpty()) return;
        {
        QSettings s;
        s.setValue("lastDir", QFileInfo(path).absolutePath());
    }
    if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".json";
    QString err = VoicePresetIO::save(*vp, path);
    if (!err.isEmpty()) QMessageBox::warning(this, "Save failed", err);
}

void VoicePanelWidget::onLoadPreset() {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    QString path = QFileDialog::getOpenFileName(this, "Load Voice Preset",
        QDir::homePath(), "Voice preset (*.json)");
    if (path.isEmpty()) return;
        {
        QSettings s;
        s.setValue("lastDir", QFileInfo(path).absolutePath());
    }
    VoiceParams oldVP = *vp;
    VoiceParams loaded = oldVP;
    QString err = VoicePresetIO::load(loaded, path);
    if (!err.isEmpty()) { QMessageBox::warning(this, "Load failed", err); return; }
    *vp = loaded;
    refreshFromModel();
    pushUndo(oldVP, *vp);
    emit paramsChanged();
}
